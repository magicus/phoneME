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

#include <amvideo.h>
#include <control.h>
#include <stdio.h>
#include <uuids.h>
#include <wmsdkidl.h>
#include "filter_in.hpp"
#include "filter_out.hpp"
#include "player.hpp"
#include "writer.hpp"

#pragma comment(lib, "strmiids.lib")
#pragma comment(lib, "winmm.lib")


nat32 const null = 0;


//#define ENABLE_MMAPI_CONT_FLV_DS_ON2
#define ENABLE_MMAPI_CONT_MP3_DS_EXT
//#define ENABLE_MMAPI_FMT_MPEG1L3_DS_EXT
//#define ENABLE_MMAPI_FMT_VP6_DS_ON2
#define ENABLE_MMAPI_VIDEO_OUTPUT_FILTER


#if defined(ENABLE_MMAPI_CONT_FLV_DS_ON2) || defined(ENABLE_MMAPI_FMT_VP6_DS_ON2)
    #pragma comment(linker, "/nodefaultlib:libcmt")
    #pragma comment(linker, "/nodefaultlib:msvcrt")

    extern "C" FILE _iob[3] = {__iob_func()[0], __iob_func()[1], __iob_func()[2]};

    namespace On2FlvSDK
    {
        #ifdef ENABLE_MMAPI_CONT_FLV_DS_ON2
            HRESULT FlvSplitCreateInstance(IUnknown *, IID const &, void **);
        #endif

        #ifdef ENABLE_MMAPI_FMT_VP6_DS_ON2
            HRESULT FlvDecVP6CreateInstance(IUnknown *, IID const &, void **);
        #endif
    }

    #ifdef ENABLE_MMAPI_CONT_FLV_DS_ON2
        // {59333afb-9992-4aa3-8c31-7fb03f6ffdf3}
        DEFINE_GUID(MEDIASUBTYPE_FLV,
        0x59333afb, 0x9992, 0x4aa3, 0x8c, 0x31, 0x7f, 0xb0, 0x3f, 0x6f, 0xfd, 0xf3);
    #endif
#endif


class player_dshow : public player
{
    AM_MEDIA_TYPE amt;
    player_callback *pcallback;
    int32 state;
    int64 media_time;
    IGraphBuilder *pgb;
    IMediaControl *pmc;
    IMediaSeeking *pms;
    filter_in *pfi;
    #ifdef ENABLE_MMAPI_VIDEO_OUTPUT_FILTER
        filter_out *pfo;
    #endif
    IPin *pp;
    #ifdef ENABLE_MMAPI_CONT_FLV_DS_ON2
        IBaseFilter *pbf_flv_split;
    #endif
    #ifdef ENABLE_MMAPI_FMT_VP6_DS_ON2
        IBaseFilter *pbf_flv_dec;
    #endif

    player_dshow()
    {
    }

    ~player_dshow()
    {
        close();
    }

    result realize()
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
            print( "illegal state %i\n", state );
            return result_illegal_state;
        }

        HRESULT hr = CoInitializeEx(null, COINIT_MULTITHREADED);
        if(FAILED(hr))
        {
            error( "CoInitializeEx", hr );
            return result_media;
        }

        hr = CoCreateInstance(CLSID_FilterGraph, null, CLSCTX_INPROC_SERVER,
            IID_IGraphBuilder, (void **)&pgb);
        if(hr != S_OK)
        {
            error( "CoCreateInstance", hr );
            CoUninitialize();
            return result_media;
        }

        hr = pgb->QueryInterface(IID_IMediaControl, (void **)&pmc);
        if(hr != S_OK)
        {
            error( "IID_IMediaControl", hr );
            pgb->Release();
            CoUninitialize();
            return result_media;
        }

        hr = pgb->QueryInterface(IID_IMediaSeeking, (void **)&pms);
        if(hr != S_OK)
        {
            error( "IID_IMediaSeeking", hr );
            pmc->Release();
            pgb->Release();
            CoUninitialize();
            return result_media;
        }

        if(!filter_in::create(&amt, pcallback, &pfi))
        {
            error( "filter_in::create", 0 );
            pms->Release();
            pmc->Release();
            pgb->Release();
            CoUninitialize();
            return result_media;
        }

        #ifdef ENABLE_MMAPI_VIDEO_OUTPUT_FILTER
            //majortype=MEDIATYPE_Audio, subtype=WMMEDIASUBTYPE_MP3, bFixedSizeSamples=TRUE, bTemporalCompression=FALSE, lSampleSize=1,
            //formattype=FORMAT_WaveFormatEx, pUnk=0x00000000, cbFormat=30, pbFormat=0x00164c60, wFormatTag=WAVE_FORMAT_MPEGLAYER3, nChannels=1, nSamplesPerSec=44100, nAvgBytesPerSec=10000, nBlockAlign=1, wBitsPerSample=0, cbSize=12, wID=1, fdwFlags=0, nBlockSize=1, nFramesPerBlock=1, nCodecDelay=0
            AM_MEDIA_TYPE amt2;
            /*amt2.majortype = MEDIATYPE_Video;
            amt2.subtype = MEDIASUBTYPE_RGB565;
            amt2.bFixedSizeSamples = TRUE;
            amt2.bTemporalCompression = FALSE;
            amt2.lSampleSize = 1;
            amt2.formattype = GUID_NULL;
            amt2.pUnk = null;
            amt2.cbFormat = 0;
            amt2.pbFormat = null;*/

            MPEGLAYER3WAVEFORMAT ml3wf;
            amt2.majortype = MEDIATYPE_Audio;
            amt2.subtype = WMMEDIASUBTYPE_MP3;
            amt2.bFixedSizeSamples = TRUE;
            amt2.bTemporalCompression = FALSE;
            amt2.lSampleSize = 1;
            amt2.formattype = FORMAT_WaveFormatEx;
            amt2.pUnk = null;
            amt2.cbFormat = sizeof(MPEGLAYER3WAVEFORMAT);
            amt2.pbFormat = (BYTE*)&ml3wf;
            ml3wf.wfx.wFormatTag = WAVE_FORMAT_MPEGLAYER3;
            ml3wf.wfx.nChannels = 2;
            ml3wf.wfx.nSamplesPerSec = 44100;
            ml3wf.wfx.nAvgBytesPerSec = 16000;
            ml3wf.wfx.nBlockAlign = 1;
            ml3wf.wfx.wBitsPerSample = 0;
            ml3wf.wfx.cbSize = MPEGLAYER3_WFX_EXTRA_BYTES;
            ml3wf.wID = 1;
            ml3wf.fdwFlags = 0;
            ml3wf.nBlockSize = 1;
            ml3wf.nFramesPerBlock = 1;
            ml3wf.nCodecDelay = 0;

            if(!filter_out::create(&amt2, pcallback, &pfo))
            {
                error( "filter_out::create", 0 );
                pfi->Release();
                pms->Release();
                pmc->Release();
                pgb->Release();
                CoUninitialize();
                return result_media;
            }
        #endif

        hr = pfi->FindPin(L"Output", &pp);
        if(hr != S_OK)
        {
            error( "FindPin", hr );
            #ifdef ENABLE_MMAPI_VIDEO_OUTPUT_FILTER
                pfo->Release();
            #endif
            pfi->Release();
            pms->Release();
            pmc->Release();
            pgb->Release();
            CoUninitialize();
            return result_media;
        }

        #ifdef ENABLE_MMAPI_CONT_FLV_DS_ON2
            hr = On2FlvSDK::FlvSplitCreateInstance(null, IID_IBaseFilter,
                (void **)&pbf_flv_split);
            if(hr != S_OK)
            {
                error( "FlvSplitCreateInstance", hr );
                pp->Release();
                #ifdef ENABLE_MMAPI_VIDEO_OUTPUT_FILTER
                    pfo->Release();
                #endif
                pfi->Release();
                pms->Release();
                pmc->Release();
                pgb->Release();
                CoUninitialize();
                return result_media;
            }
        #endif

        #ifdef ENABLE_MMAPI_FMT_VP6_DS_ON2
            hr = On2FlvSDK::FlvDecVP6CreateInstance(null, IID_IBaseFilter,
                (void **)&pbf_flv_dec);
            if(hr != S_OK)
            {
                error( "FlvDecVP6CreateInstance", hr );
                #ifdef ENABLE_MMAPI_CONT_FLV_DS_ON2
                    pbf_flv_split->Release();
                #endif
                pp->Release();
                #ifdef ENABLE_MMAPI_VIDEO_OUTPUT_FILTER
                    pfo->Release();
                #endif
                pfi->Release();
                pms->Release();
                pmc->Release();
                pgb->Release();
                CoUninitialize();
                return result_media;
            }
        #endif

        hr = pgb->AddFilter(pfi, L"Input filter");
        if(hr != S_OK)
        {
            error( "AddFilter(I)", hr );
            #ifdef ENABLE_MMAPI_FMT_VP6_DS_ON2
                pbf_flv_dec->Release();
            #endif
            #ifdef ENABLE_MMAPI_CONT_FLV_DS_ON2
                pbf_flv_split->Release();
            #endif
            pp->Release();
            #ifdef ENABLE_MMAPI_VIDEO_OUTPUT_FILTER
                pfo->Release();
            #endif
            pfi->Release();
            pms->Release();
            pmc->Release();
            pgb->Release();
            CoUninitialize();
            return result_media;
        }

         #ifdef ENABLE_MMAPI_VIDEO_OUTPUT_FILTER
            hr = pgb->AddFilter(pfo, L"Output filter");
            if(hr != S_OK)
            {
                error( "AddFilter(O)", hr );
                #ifdef ENABLE_MMAPI_FMT_VP6_DS_ON2
                    pbf_flv_dec->Release();
                #endif
                #ifdef ENABLE_MMAPI_CONT_FLV_DS_ON2
                    pbf_flv_split->Release();
                #endif
                pp->Release();
                #ifdef ENABLE_MMAPI_VIDEO_OUTPUT_FILTER
                    pfo->Release();
                #endif
                pfi->Release();
                pms->Release();
                pmc->Release();
                pgb->Release();
                CoUninitialize();
                return result_media;
            }
        #endif

        #ifdef ENABLE_MMAPI_CONT_FLV_DS_ON2
            hr = pgb->AddFilter(pbf_flv_split, L"FLV splitter");
            if(hr != S_OK)
            {
                error( "AddFilter(S)", hr );
                #ifdef ENABLE_MMAPI_FMT_VP6_DS_ON2
                    pbf_flv_dec->Release();
                #endif
                #ifdef ENABLE_MMAPI_CONT_FLV_DS_ON2
                    pbf_flv_split->Release();
                #endif
                pp->Release();
                #ifdef ENABLE_MMAPI_VIDEO_OUTPUT_FILTER
                    pfo->Release();
                #endif
                pfi->Release();
                pms->Release();
                pmc->Release();
                pgb->Release();
                CoUninitialize();
                return result_media;
            }
        #endif

        #ifdef ENABLE_MMAPI_FMT_VP6_DS_ON2
            hr = pgb->AddFilter(pbf_flv_dec, L"FLV decoder");
            if(hr != S_OK)
            {
                error( "AddFilter(D)", hr );
                #ifdef ENABLE_MMAPI_FMT_VP6_DS_ON2
                    pbf_flv_dec->Release();
                #endif
                #ifdef ENABLE_MMAPI_CONT_FLV_DS_ON2
                    pbf_flv_split->Release();
                #endif
                pp->Release();
                #ifdef ENABLE_MMAPI_VIDEO_OUTPUT_FILTER
                    pfo->Release();
                #endif
                pfi->Release();
                pms->Release();
                pmc->Release();
                pgb->Release();
                CoUninitialize();
                return result_media;
            }
        #endif

        state = realized;

        return result_success;
    }

    result prefetch()
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
            error("IGraphBuilder::Render", hr);
            return result_media;
        }


        dump_filter_graph(pgb);

        int64 tc = 40200000;
        pms->SetPositions(&tc, AM_SEEKING_AbsolutePositioning, null, 0);

        hr = pmc->Pause();
        if(FAILED(hr))
        {
            error("IMediaControl::Pause", hr);
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

    result start()
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

    result stop()
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

    result deallocate()
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

    void close()
    {
        if(state == unrealized)
        {
            state = closed;
        }
        else if(state == realized || state == prefetched || state == started)
        {
            pmc->Stop();
            Sleep(100);
            #ifdef ENABLE_MMAPI_FMT_VP6_DS_ON2
                pbf_flv_dec->Release();
            #endif
            #ifdef ENABLE_MMAPI_CONT_FLV_DS_ON2
                pbf_flv_split->Release();
            #endif
            pp->Release();
            #ifdef ENABLE_MMAPI_VIDEO_OUTPUT_FILTER
                pfo->Release();
            #endif
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

    //result set_time_base(time_base *master)
    //time_base *get_time_base(result *presult = 0)

    int64 set_media_time(int64 /*now*/, result *presult)
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

    int64 get_media_time(result *presult)
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

    int32 get_state()
    {
        return state;
    }

    int64 get_duration(result *presult)
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

    //string16c get_content_type(result *presult = 0)

    result set_loop_count(int32 count)
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

    //result add_player_listener(player_listener *pplayer_listener)
    //result remove_player_listener(player_listener *pplayer_listener)

    bool data(nat32 len, void const *pdata)
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

    friend bool create_player_dshow(nat32 len, char16 const *pformat, player_callback *pcallback, player **ppplayer);
};

bool create_player_dshow(nat32 len, char16 const *pformat, player_callback *pcallback, player **ppplayer)
{
    player_dshow *pplayer;
    if(!pformat || !pcallback || !ppplayer)
    {
        return false;
    }
#ifdef ENABLE_MMAPI_CONT_FLV_DS_ON2
    else if(len >= wcslen(L"video/x-flv") && !wcsncmp(pformat, L"video/x-flv", wcslen(L"video/x-flv")))
    {
        pplayer = new player_dshow;
        if(!pplayer) return false;

        pplayer->amt.majortype = MEDIATYPE_Stream;
        pplayer->amt.subtype = MEDIASUBTYPE_FLV;
        pplayer->amt.bFixedSizeSamples = TRUE;
        pplayer->amt.bTemporalCompression = FALSE;
        pplayer->amt.lSampleSize = 1;
        pplayer->amt.formattype = GUID_NULL;
        pplayer->amt.pUnk = null;
        pplayer->amt.cbFormat = 0;
        pplayer->amt.pbFormat = null;
    }
#endif
#ifdef ENABLE_MMAPI_CONT_MP3_DS_EXT
    else if(len >= wcslen(L"audio/mpeg") && !wcsncmp(pformat, L"audio/mpeg", wcslen(L"audio/mpeg")))
    {
        pplayer = new player_dshow;
        if(!pplayer) return false;

        pplayer->amt.majortype = MEDIATYPE_Stream;
        pplayer->amt.subtype = MEDIASUBTYPE_MPEG1Audio;
        pplayer->amt.bFixedSizeSamples = TRUE;
        pplayer->amt.bTemporalCompression = FALSE;
        pplayer->amt.lSampleSize = 1;
        pplayer->amt.formattype = GUID_NULL;
        pplayer->amt.pUnk = null;
        pplayer->amt.cbFormat = 0;
        pplayer->amt.pbFormat = null;
    }
#endif
    else
    {
        return false;
    }
    pplayer->pcallback = pcallback;
    pplayer->state = player::unrealized;
    pplayer->media_time = player::time_unknown;
    *ppplayer = pplayer;
    return true;
}
