/*
* Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
* DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License version
* 2 only, as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* General Public License version 2 for more details (a copy is
* included at /legal/license.txt).
*
* You should have received a copy of the GNU General Public License
* version 2 along with this work; if not, write to the Free Software
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
* 02110-1301 USA
*
* Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
* Clara, CA 95054 or visit www.sun.com if you need additional
* information or have any questions.
*/

#include <control.h>
#include <stdio.h>
#include <uuids.h>
#include "player.hpp"
#include "player_callback.hpp"
#include "filter_in.hpp"
#include "writer.hpp"

#pragma comment(lib, "strmiids.lib")
#pragma comment(lib, "winmm.lib")

#pragma comment(linker, "/nodefaultlib:libcmt")
#pragma comment(linker, "/nodefaultlib:msvcrt")


nat32 const null = 0;


extern "C" FILE _iob[3] = {__iob_func()[0], __iob_func()[1], __iob_func()[2]};


namespace On2FlvSDK
{
    HRESULT FlvSplitCreateInstance(IUnknown *, IID const &, void **);
    HRESULT FlvDecVP6CreateInstance(IUnknown *, IID const &, void **);
}

// {59333afb-9992-4aa3-8c31-7fb03f6ffdf3}
DEFINE_GUID(MEDIASUBTYPE_FLV,
0x59333afb, 0x9992, 0x4aa3, 0x8c, 0x31, 0x7f, 0xb0, 0x3f, 0x6f, 0xfd, 0xf3);



#include "types.hpp"

struct IBaseFilter;
struct IGraphBuilder;
struct IMediaControl;
struct IMediaSeeking;
struct IPin;
class filter_in;

class player_dshow : public player
{
    int32 state;
    int64 media_time;
    IGraphBuilder *pgb;
    IMediaControl *pmc;
    IMediaSeeking *pms;
    filter_in *pfi;
    IPin *pp;
    IBaseFilter *pbf_flv_split;
    IBaseFilter *pbf_flv_dec;
public:
    player_dshow();
    virtual ~player_dshow();
    virtual result realize();
    virtual result prefetch();
    virtual result start();
    virtual result stop();
    virtual result deallocate();
    virtual void close();
    //virtual result set_time_base(time_base *master);
    //virtual time_base *get_time_base(result *presult = 0);
    virtual int64 set_media_time(int64 now, result *presult = 0);
    virtual int64 get_media_time(result *presult = 0);
    virtual int32 get_state();
    virtual int64 get_duration(result *presult = 0);
    //virtual string16c get_content_type(result *presult = 0);
    virtual result set_loop_count(int32 count);
    //virtual result add_player_listener(player_listener *pplayer_listener);
    //virtual result remove_player_listener(player_listener *pplayer_listener);

    virtual bool data(nat32 len, void const *pdata);
};

player_dshow::player_dshow()
{
    state = unrealized;
    media_time = time_unknown;
}

player_dshow::~player_dshow()
{
    close();
}

player::result player_dshow::realize()
{
    if(state == unrealized)
    {
    }
    else if(state == realized || state == prefetched || state == started)
    {
        return result_success;
    }
    else
    {
        return result_illegal_state;
    }

    HRESULT hr = CoInitializeEx(null, COINIT_MULTITHREADED);
    if(FAILED(hr)) return result_media;

    hr = CoCreateInstance(CLSID_FilterGraph, null, CLSCTX_INPROC_SERVER,
        IID_IGraphBuilder, (void **)&pgb);
    if(hr != S_OK)
    {
        CoUninitialize();
        return result_media;
    }

    hr = pgb->QueryInterface(IID_IMediaControl, (void **)&pmc);
    if(hr != S_OK)
    {
        pgb->Release();
        CoUninitialize();
        return result_media;
    }

    hr = pgb->QueryInterface(IID_IMediaSeeking, (void **)&pms);
    if(hr != S_OK)
    {
        pmc->Release();
        pgb->Release();
        CoUninitialize();
        return result_media;
    }

    AM_MEDIA_TYPE amt;
    amt.majortype = MEDIATYPE_Stream;
    amt.subtype = MEDIASUBTYPE_FLV;
    amt.bFixedSizeSamples = TRUE;
    amt.bTemporalCompression = FALSE;
    amt.lSampleSize = 0;
    amt.formattype = FORMAT_None;
    amt.pUnk = null;
    amt.cbFormat = 0;
    amt.pbFormat = null;
    if(!filter_in::create(&amt, &pfi))
    {
        pms->Release();
        pmc->Release();
        pgb->Release();
        CoUninitialize();
        return result_media;
    }

    hr = pfi->FindPin(L"Output", &pp);
    if(hr != S_OK)
    {
        pfi->Release();
        pms->Release();
        pmc->Release();
        pgb->Release();
        CoUninitialize();
        return result_media;
    }

    hr = On2FlvSDK::FlvSplitCreateInstance(null, IID_IBaseFilter,
        (void **)&pbf_flv_split);
    if(hr != S_OK)
    {
        pp->Release();
        pfi->Release();
        pms->Release();
        pmc->Release();
        pgb->Release();
        CoUninitialize();
        return result_media;
    }

    hr = On2FlvSDK::FlvDecVP6CreateInstance(null, IID_IBaseFilter,
        (void **)&pbf_flv_dec);
    if(hr != S_OK)
    {
        pbf_flv_split->Release();
        pp->Release();
        pfi->Release();
        pms->Release();
        pmc->Release();
        pgb->Release();
        CoUninitialize();
        return result_media;
    }

    hr = pgb->AddFilter(pfi, L"Source Filter");
    if(hr != S_OK)
    {
        pbf_flv_dec->Release();
        pbf_flv_split->Release();
        pp->Release();
        pfi->Release();
        pms->Release();
        pmc->Release();
        pgb->Release();
        CoUninitialize();
        return result_media;
    }

    hr = pgb->AddFilter(pbf_flv_split, L"FLV splitter");
    if(hr != S_OK)
    {
        pbf_flv_dec->Release();
        pbf_flv_split->Release();
        pp->Release();
        pfi->Release();
        pms->Release();
        pmc->Release();
        pgb->Release();
        CoUninitialize();
        return result_media;
    }

    hr = pgb->AddFilter(pbf_flv_dec, L"FLV decoder");
    if(hr != S_OK)
    {
        pbf_flv_dec->Release();
        pbf_flv_split->Release();
        pp->Release();
        pfi->Release();
        pms->Release();
        pmc->Release();
        pgb->Release();
        CoUninitialize();
        return result_media;
    }

    state = realized;

    return result_success;
}

player::result player_dshow::prefetch()
{
    if(state == unrealized)
    {
        result r = realize();
        if(r != result_success) return r;
    }
    else if(state == realized)
    {
    }
    else if(state == prefetched || state == started)
    {
        return result_success;
    }
    else
    {
        return result_illegal_state;
    }

    HRESULT hr = pgb->Render(pp);
    if(hr != S_OK)
    {
        return result_media;
    }

    dump_filter_graph(pgb, 0);

    hr = pmc->Pause();
    if(FAILED(hr))
    {
        return result_media;
    }

    hr = pms->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME);
    if(hr != S_OK)
    {
        return result_media;
    }

    state = prefetched;

    return result_success;
}

player::result player_dshow::start()
{
    if(state == unrealized || state == realized)
    {
        result r = prefetch();
        if(r != result_success) return r;
    }
    else if(state == prefetched)
    {
    }
    else if(state == started)
    {
        return result_success;
    }
    else
    {
        return result_illegal_state;
    }

    HRESULT hr = pmc->Run();
    if(FAILED(hr)) return result_media;

    state = started;

    return result_success;
}

player::result player_dshow::stop()
{
    if(state == unrealized || state == realized || state == prefetched)
    {
        return result_success;
    }
    else if(state == started)
    {
    }
    else
    {
        return result_illegal_state;
    }

    HRESULT hr = pmc->Pause();
    if(FAILED(hr)) return result_media;

    state = prefetched;

    return result_success;
}

player::result player_dshow::deallocate()
{
    if(state == unrealized || state == realized)
    {
        return result_success;
    }
    else if(state == prefetched)
    {
    }
    else if(state == started)
    {
        stop();
    }
    else
    {
        return result_illegal_state;
    }

    state = realized;

    return result_success;
}

void player_dshow::close()
{
    if(state == unrealized)
    {
        state = closed;
    }
    else if(state == realized || state == prefetched || state == started)
    {
        pbf_flv_dec->Release();
        pbf_flv_split->Release();
        pp->Release();
        pfi->Release();
        pms->Release();
        pmc->Release();
        pgb->Release();
        CoUninitialize();
        state = closed;
    }
    else
    {
    }
}

//result player_dshow::set_time_base(time_base *master)
//time_base *player_dshow::get_time_base(result *presult = 0)

int64 player_dshow::set_media_time(int64 /*now*/, result *presult)
{
    if(state == realized || state == prefetched || state == started)
    {
    }
    else
    {
        *presult = result_illegal_state;
        return media_time;
    }

    *presult = result_media;
    return media_time;
}

int64 player_dshow::get_media_time(result *presult)
{
    if(state == unrealized || state == realized || state == prefetched ||
        state == started)
    {
    }
    else
    {
        *presult = result_illegal_state;
        return media_time;
    }

    LONGLONG cur;
    HRESULT hr = pms->GetCurrentPosition(&cur);
    if(hr != S_OK) return media_time;

    cur /= 10;
    media_time = cur;
    *presult = result_success;
    return media_time;
}

int32 player_dshow::get_state()
{
    return state;
}

int64 player_dshow::get_duration(result *presult)
{
    if(state == unrealized || state == realized || state == prefetched ||
        state == started)
    {
    }
    else
    {
        *presult = result_illegal_state;
        return time_unknown;
    }

    *presult = result_success;
    return time_unknown;
}

//string16c player_dshow::get_content_type(result *presult = 0)

player::result player_dshow::set_loop_count(int32 count)
{
    if(!count) return result_illegal_argument;

    if(state == unrealized || state == realized || state == prefetched)
    {
    }
    else
    {
        return result_illegal_state;
    }

    return result_success;
}

//result player_dshow::add_player_listener(player_listener *pplayer_listener)
//result player_dshow::remove_player_listener(player_listener *pplayer_listener)

bool player_dshow::data(nat32 len, void const *pdata)
{
    if(state == unrealized || state == realized || state == prefetched ||
        state == started)
    {
    }
    else
    {
        return false;
    }

    return pfi->data(len, pdata);
}

bool create_player_dshow(nat32 /*len*/, char16 const * /*pformat*/, player_callback *pcallback, player **ppplayer)
{
    if(!pcallback || !ppplayer) return false;
    player_dshow *pplayer = new player_dshow;
    if(!pplayer) return false;
    *ppplayer = pplayer;
    return true;
}
