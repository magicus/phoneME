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
#include "player_dshow.hpp"
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


player_dshow::player_dshow()
{
    pgb = null;
}

player_dshow::~player_dshow()
{
    if(pgb) shutdown();
}

bool player_dshow::init1(nat32 /*len*/, char16 * /*format*/)
{
    if(pgb) return false;

    HRESULT hr = CoInitializeEx(null, COINIT_MULTITHREADED);
    if(FAILED(hr)) return false;

    hr = CoCreateInstance(CLSID_FilterGraph, null, CLSCTX_INPROC_SERVER,
        IID_IGraphBuilder, (void **)&pgb);
    if(hr != S_OK)
    {
        CoUninitialize();
        return false;
    }

    hr = pgb->QueryInterface(IID_IMediaControl, (void **)&pmc);
    if(hr != S_OK)
    {
        pgb->Release();
        pgb = null;
        CoUninitialize();
        return false;
    }

    hr = pgb->QueryInterface(IID_IMediaSeeking, (void **)&pms);
    if(hr != S_OK)
    {
        pmc->Release();
        pgb->Release();
        pgb = null;
        CoUninitialize();
        return false;
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
        pgb = null;
        CoUninitialize();
        return false;
    }

    hr = pfi->FindPin(L"Output", &pp);
    if(hr != S_OK)
    {
        pfi->Release();
        pms->Release();
        pmc->Release();
        pgb->Release();
        pgb = null;
        CoUninitialize();
        return false;
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
        pgb = null;
        CoUninitialize();
        return false;
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
        pgb = null;
        CoUninitialize();
        return false;
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
        pgb = null;
        CoUninitialize();
        return false;
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
        pgb = null;
        CoUninitialize();
        return false;
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
        pgb = null;
        CoUninitialize();
        return false;
    }

    return true;
}

bool player_dshow::init2()
{
    if(!pgb) return false;

    print("Callind IGraphBuilder::Render...\n");

    HRESULT hr = pgb->Render(pp);
    if(hr != S_OK)
    {
        print("IGraphBuilder::Render failed.\n");
        return false;
    }

    print("IGraphBuilder::Render succeeded.\n");

    dump_filter_graph(pgb, 0);

    hr = pmc->Pause();
    if(FAILED(hr)) return false;

    hr = pms->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME);
    if(hr != S_OK) return false;

    return true;
}

bool player_dshow::data(nat32 len, void const *pdata)
{
    if(!pgb) return false;

    return pfi->data(len, pdata);
}

bool player_dshow::play()
{
    if(!pgb) return false;

    return SUCCEEDED(pmc->Run());
}

bool player_dshow::stop()
{
    if(!pgb) return false;

    return SUCCEEDED(pmc->Pause());
}

bool player_dshow::seek(double /*time*/)
{
    return false;
}

bool player_dshow::tell(double *time)
{
    if(!pgb) return false;

    LONGLONG cur;
    HRESULT hr = pms->GetCurrentPosition(&cur);
    if(hr != S_OK)
    {
        *time = -1;
        return false;
    }
    else
    {
        *time = double(cur) / 1e7;
        return true;
    }
}

bool player_dshow::shutdown()
{
    if(!pgb) return false;

    pbf_flv_dec->Release();
    pbf_flv_split->Release();
    pp->Release();
    pfi->Release();
    pms->Release();
    pmc->Release();
    pgb->Release();
    pgb = null;
    CoUninitialize();

    return true;
}

bool player_dshow::get_video_size(long* pw, long* ph)
{
    IBasicVideo* pbv = NULL;
    if( NULL != pgb && S_OK == pgb->QueryInterface( IID_IBasicVideo, (void**)&pbv ) )
    {
        HRESULT hr = pbv->GetVideoSize( pw, ph );
        return ( S_OK == hr );
    }
    else
    {
        return false;
    }
}
