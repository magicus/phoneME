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
#include <uuids.h>
#include "filter_in.hpp"
#include "filter_out.hpp"
#include "player.hpp"
#include "writer.hpp"

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "strmiids.lib")


const nat32 null = 0;


// #define ENABLE_JSR_135_CONT_3G2_DSHOW_EXT  // video/3gpp, audio/3gpp, video/3gpp2, audio/3gpp2; .3gp, .3g2
// #define ENABLE_JSR_135_CONT_3GP_DSHOW_EXT  // video/3gpp, audio/3gpp, video/3gpp2, audio/3gpp2; .3gp, .3g2
// #define ENABLE_JSR_135_CONT_AMR_DSHOW_EXT  // audio/amr; .amr
// #define ENABLE_JSR_135_CONT_ASF_DSHOW_EXT  // video/x-ms-asf, application/vnd.ms-asf; .asf
// #define ENABLE_JSR_135_CONT_AVI_DSHOW_EXT  // video/avi, video/msvideo, video/x-msvideo; .avi
// #define ENABLE_JSR_135_CONT_FLV_DSHOW_EXT  // video/x-flv, video/mp4, video/x-m4v, audio/mp4a-latm, video/3gpp, video/quicktime, audio/mp4; .flv, .f4v, .f4p, .f4a, .f4b
// #define ENABLE_JSR_135_CONT_FLV_DSHOW_INT
// #define ENABLE_JSR_135_CONT_MKA_DSHOW_EXT  // video/x-matroska, audio/x-matroska; .mkv, .mka, .mks
// #define ENABLE_JSR_135_CONT_MKV_DSHOW_EXT  // video/x-matroska, audio/x-matroska; .mkv, .mka, .mks
// #define ENABLE_JSR_135_CONT_MOV_DSHOW_EXT  // video/quicktime; .mov, .qt
// #define ENABLE_JSR_135_CONT_MP2_DSHOW_EXT  // audio/mpeg; .mp2
// #define ENABLE_JSR_135_CONT_MP3_DSHOW_EXT  // audio/mpeg; .mp3
// #define ENABLE_JSR_135_CONT_MP4_DSHOW_EXT  // video/mp4, audio/mp4, application/mp4; .mp4
// #define ENABLE_JSR_135_CONT_MPEG_DSHOW_EXT // audio/mpeg, video/mpeg; .mpg, .mpeg, .mp1, .mp2, .mp3, .m1v, .m1a, .m2a, .mpa, .mpv
// #define ENABLE_JSR_135_CONT_MPG_DSHOW_EXT  // audio/mpeg, video/mpeg; .mpg, .mpeg, .mp1, .mp2, .mp3, .m1v, .m1a, .m2a, .mpa, .mpv
// #define ENABLE_JSR_135_CONT_OGA_DSHOW_EXT  // audio/ogg; .ogg, .oga
// #define ENABLE_JSR_135_CONT_OGG_DSHOW_EXT  // video/ogg, audio/ogg, application/ogg; .ogv, .oga, .ogx, .ogg, .spx
// #define ENABLE_JSR_135_CONT_OGV_DSHOW_EXT  // video/ogg; .ogv
// #define ENABLE_JSR_135_CONT_QT_DSHOW_EXT   // video/quicktime; .mov, .qt
// #define ENABLE_JSR_135_CONT_RMVB_DSHOW_EXT // application/vnd.rn-realmedia-vbr; .rmvb
// #define ENABLE_JSR_135_CONT_WAV_DSHOW_EXT  // audio/wav, audio/wave, audio/x-wav; .wav
// #define ENABLE_JSR_135_CONT_WMA_DSHOW_EXT  // audio/x-ms-wma; .wma
// #define ENABLE_JSR_135_CONT_WMV_DSHOW_EXT  // video/x-ms-wmv; .wmv
// #define ENABLE_JSR_135_FMT_VP6_DSHOW_INT
// #define ENABLE_JSR_135_DSHOW_AUDIO_OUTPUT_FILTER
// #define ENABLE_JSR_135_DSHOW_VIDEO_OUTPUT_FILTER


// #include <initguid.h>

#ifdef ENABLE_JSR_135_CONT_3GP_DSHOW_EXT
    // {08e22ada-b715-45ed-9d20-7b87750301d4}
    DEFINE_GUID(MEDIASUBTYPE_MP4,
    0x08e22ada, 0xb715, 0x45ed, 0x9d, 0x20, 0x7b, 0x87, 0x75, 0x03, 0x01, 0xd4);
#endif

#if defined ENABLE_JSR_135_CONT_FLV_DSHOW_EXT || defined ENABLE_JSR_135_CONT_FLV_DSHOW_INT
    // {59333afb-9992-4aa3-8c31-7fb03f6ffdf3}
    DEFINE_GUID(MEDIASUBTYPE_FLV,
    0x59333afb, 0x9992, 0x4aa3, 0x8c, 0x31, 0x7f, 0xb0, 0x3f, 0x6f, 0xfd, 0xf3);
#endif

#ifdef ENABLE_JSR_135_CONT_FLV_DSHOW_INT
    HRESULT __stdcall flv_splitter_create(IUnknown *, const IID &, void **);
#endif

#ifdef ENABLE_JSR_135_FMT_VP6_DSHOW_INT
    HRESULT __stdcall flv_decoder_create(IUnknown *, const IID &, void **);
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
#ifdef ENABLE_JSR_135_DSHOW_AUDIO_OUTPUT_FILTER
    filter_out *pfo_a;
#endif
#ifdef ENABLE_JSR_135_DSHOW_VIDEO_OUTPUT_FILTER
    filter_out *pfo_v;
#endif
    IPin *pp;
#ifdef ENABLE_JSR_135_CONT_FLV_DSHOW_INT
    IBaseFilter *pbf_flv_split;
#endif
#ifdef ENABLE_JSR_135_FMT_VP6_DSHOW_INT
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
            print("illegal state %i\n", state);
            return result_illegal_state;
        }

        HRESULT hr = pgb->Render(pp);
        if(hr != S_OK)
        {
            error("IGraphBuilder::Render", hr);
            return result_media;
        }

        // dump_filter_graph(pgb);

        // int64 tc = 40200000;
        // pms->SetPositions(&tc, AM_SEEKING_AbsolutePositioning, null, 0);

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
#ifdef ENABLE_JSR_135_FMT_VP6_DSHOW_INT
                pbf_flv_dec->Release();
#endif
#ifdef ENABLE_JSR_135_CONT_FLV_DSHOW_INT
                pbf_flv_split->Release();
#endif
            pp->Release();
#ifdef ENABLE_JSR_135_DSHOW_VIDEO_OUTPUT_FILTER
                pfo_v->Release();
#endif
#ifdef ENABLE_JSR_135_DSHOW_AUDIO_OUTPUT_FILTER
                pfo_a->Release();
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

    // result set_time_base(time_base *master)
    // time_base *get_time_base(result *presult = 0)

    int64 set_media_time(int64 now, result *presult)
    {
        if(state == realized || state == prefetched || state == started)
        {
        }
        else
        {
            *presult = result_illegal_state;
            return media_time;
        }

        LONGLONG cur = now * 10;
        HRESULT hr = pms->SetPositions(&cur, AM_SEEKING_AbsolutePositioning, null, AM_SEEKING_NoPositioning);
        if(hr != S_OK)
        {
            *presult = result_media;
            return media_time;
        }

        media_time = now;
        *presult = result_success;
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
        if(hr != S_OK)
        {
            *presult = result_media;
            return media_time;
        }

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

        LONGLONG cur;
        HRESULT hr = pms->GetDuration(&cur);
        if(hr != S_OK)
        {
            *presult = result_media;
            return media_time;
        }

        *presult = result_success;
        return cur / 10;
    }

    // string16c get_content_type(result *presult = 0)

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

    // result add_player_listener(player_listener *pplayer_listener)
    // result remove_player_listener(player_listener *pplayer_listener)

    bool data(nat32 len, const void *pdata)
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

    friend bool create_player_dshow(nat32 len, const char16 *pformat, player_callback *pcallback, player **ppplayer);
};

bool create_player_dshow(nat32 len, const char16 *pformat, player_callback *pcallback, player **ppplayer)
{
    player_dshow *pplayer;
    if(len > 0x7fffffff || !pformat || !pcallback || !ppplayer)
    {
        return false;
    }
#ifdef ENABLE_JSR_135_CONT_3GP_DSHOW_EXT
    else if(len >= wcslen(L"video/3gpp") && !wcsncmp(pformat, L"video/3gpp", wcslen(L"video/3gpp")))
    {
        pplayer = new player_dshow;
        if(!pplayer) return false;

        pplayer->amt.majortype = MEDIATYPE_Stream;
        pplayer->amt.subtype = GUID_NULL;
        pplayer->amt.bFixedSizeSamples = TRUE;
        pplayer->amt.bTemporalCompression = FALSE;
        pplayer->amt.lSampleSize = 1;
        pplayer->amt.formattype = GUID_NULL;
        pplayer->amt.pUnk = null;
        pplayer->amt.cbFormat = 0;
        pplayer->amt.pbFormat = null;
    }
#endif
#ifdef ENABLE_JSR_135_CONT_AMR_DSHOW_EXT
    else if(len >= wcslen(L"audio/amr") && !wcsncmp(pformat, L"audio/amr", wcslen(L"audio/amr")))
    {
        pplayer = new player_dshow;
        if(!pplayer) return false;

        pplayer->amt.majortype = MEDIATYPE_Stream;
        pplayer->amt.subtype = GUID_NULL;
        pplayer->amt.bFixedSizeSamples = TRUE;
        pplayer->amt.bTemporalCompression = FALSE;
        pplayer->amt.lSampleSize = 1;
        pplayer->amt.formattype = GUID_NULL;
        pplayer->amt.pUnk = null;
        pplayer->amt.cbFormat = 0;
        pplayer->amt.pbFormat = null;
    }
#endif
#ifdef ENABLE_JSR_135_CONT_AVI_DSHOW_EXT
    else if(len >= wcslen(L"video/avi") && !wcsncmp(pformat, L"video/avi", wcslen(L"video/avi")))
    {
        pplayer = new player_dshow;
        if(!pplayer) return false;

        pplayer->amt.majortype = MEDIATYPE_Stream;
        pplayer->amt.subtype = GUID_NULL;
        pplayer->amt.bFixedSizeSamples = TRUE;
        pplayer->amt.bTemporalCompression = FALSE;
        pplayer->amt.lSampleSize = 1;
        pplayer->amt.formattype = GUID_NULL;
        pplayer->amt.pUnk = null;
        pplayer->amt.cbFormat = 0;
        pplayer->amt.pbFormat = null;
    }
#endif
#if defined ENABLE_JSR_135_CONT_FLV_DSHOW_EXT || defined ENABLE_JSR_135_CONT_FLV_DSHOW_INT
    else if(len >= wcslen(L"video/x-flv") && !wcsncmp(pformat, L"video/x-flv", wcslen(L"video/x-flv")))
    {
        pplayer = new player_dshow;
        if(!pplayer) return false;

        pplayer->amt.majortype = MEDIATYPE_Stream;
        pplayer->amt.subtype = GUID_NULL;
        pplayer->amt.bFixedSizeSamples = TRUE;
        pplayer->amt.bTemporalCompression = FALSE;
        pplayer->amt.lSampleSize = 1;
        pplayer->amt.formattype = GUID_NULL;
        pplayer->amt.pUnk = null;
        pplayer->amt.cbFormat = 0;
        pplayer->amt.pbFormat = null;
    }
#endif
#ifdef ENABLE_JSR_135_CONT_MP3_DSHOW_EXT
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
#ifdef ENABLE_JSR_135_CONT_MP4_DSHOW_EXT
    else if(len >= wcslen(L"video/mp4") && !wcsncmp(pformat, L"video/mp4", wcslen(L"video/mp4")))
    {
        pplayer = new player_dshow;
        if(!pplayer) return false;

        pplayer->amt.majortype = MEDIATYPE_Stream;
        pplayer->amt.subtype = GUID_NULL;
        pplayer->amt.bFixedSizeSamples = TRUE;
        pplayer->amt.bTemporalCompression = FALSE;
        pplayer->amt.lSampleSize = 1;
        pplayer->amt.formattype = GUID_NULL;
        pplayer->amt.pUnk = null;
        pplayer->amt.cbFormat = 0;
        pplayer->amt.pbFormat = null;
    }
#endif
#ifdef ENABLE_JSR_135_CONT_MPG_DSHOW_EXT
    else if(len >= wcslen(L"video/mpeg") && !wcsncmp(pformat, L"video/mpeg", wcslen(L"video/mpeg")))
    {
        pplayer = new player_dshow;
        if(!pplayer) return false;

        pplayer->amt.majortype = MEDIATYPE_Stream;
        pplayer->amt.subtype = MEDIASUBTYPE_MPEG1System;
        pplayer->amt.bFixedSizeSamples = TRUE;
        pplayer->amt.bTemporalCompression = FALSE;
        pplayer->amt.lSampleSize = 1;
        pplayer->amt.formattype = GUID_NULL;
        pplayer->amt.pUnk = null;
        pplayer->amt.cbFormat = 0;
        pplayer->amt.pbFormat = null;
    }
#endif
#ifdef ENABLE_JSR_135_CONT_OGG_DSHOW_EXT
    else if(len >= wcslen(L"video/ogg") && !wcsncmp(pformat, L"video/ogg", wcslen(L"video/ogg")))
    {
        pplayer = new player_dshow;
        if(!pplayer) return false;

        pplayer->amt.majortype = MEDIATYPE_Stream;
        pplayer->amt.subtype = GUID_NULL;
        pplayer->amt.bFixedSizeSamples = TRUE;
        pplayer->amt.bTemporalCompression = FALSE;
        pplayer->amt.lSampleSize = 1;
        pplayer->amt.formattype = GUID_NULL;
        pplayer->amt.pUnk = null;
        pplayer->amt.cbFormat = 0;
        pplayer->amt.pbFormat = null;
    }
#endif
#ifdef ENABLE_JSR_135_CONT_WAV_DSHOW_EXT
    else if(len >= wcslen(L"audio/wav") && !wcsncmp(pformat, L"audio/wav", wcslen(L"audio/wav")))
    {
        pplayer = new player_dshow;
        if(!pplayer) return false;

        pplayer->amt.majortype = MEDIATYPE_Stream;
        pplayer->amt.subtype = MEDIASUBTYPE_WAVE;
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

    bool r = false;

    HRESULT hr = CoInitializeEx(null, COINIT_MULTITHREADED);
    if(FAILED(hr))
    {
        error("CoInitializeEx", hr);
    }
    else
    {
        hr = CoCreateInstance(CLSID_FilterGraph, null, CLSCTX_INPROC_SERVER,
            IID_IGraphBuilder, (void **)&pplayer->pgb);
        if(hr != S_OK)
        {
            error("CoCreateInstance", hr);
        }
        else
        {
            hr = pplayer->pgb->QueryInterface(IID_IMediaControl, (void **)&pplayer->pmc);
            if(hr != S_OK)
            {
                error("IGraphBuilder::QueryInterface(IID_IMediaControl)", hr);
            }
            else
            {
                hr = pplayer->pgb->QueryInterface(IID_IMediaSeeking, (void **)&pplayer->pms);
                if(hr != S_OK)
                {
                    error("IGraphBuilder::QueryInterface(IID_IMediaSeeking)", hr);
                }
                else
                {
                    if(!filter_in::create(&pplayer->amt, pcallback, &pplayer->pfi))
                    {
                        error("filter_in::create", 0);
                    }
                    else
                    {
#ifdef ENABLE_JSR_135_DSHOW_AUDIO_OUTPUT_FILTER
                        AM_MEDIA_TYPE amt_a;
                        amt_a.majortype = MEDIATYPE_Audio;
                        amt_a.subtype = MEDIASUBTYPE_PCM;
                        amt_a.bFixedSizeSamples = TRUE;
                        amt_a.bTemporalCompression = FALSE;
                        amt_a.lSampleSize = 1;
                        amt_a.formattype = GUID_NULL;
                        amt_a.pUnk = null;
                        amt_a.cbFormat = 0;
                        amt_a.pbFormat = null;
                        if(!filter_out::create(&amt_a, pcallback, &pplayer->pfo_a))
                        {
                            error("filter_out::create(Audio)", 0);
                        }
                        else
#endif
                        {
#ifdef ENABLE_JSR_135_DSHOW_VIDEO_OUTPUT_FILTER
                            AM_MEDIA_TYPE amt2;
                            amt2.majortype = MEDIATYPE_Video;
                            amt2.subtype = MEDIASUBTYPE_RGB565;
                            amt2.bFixedSizeSamples = TRUE;
                            amt2.bTemporalCompression = FALSE;
                            amt2.lSampleSize = 1;
                            amt2.formattype = GUID_NULL;
                            amt2.pUnk = null;
                            amt2.cbFormat = 0;
                            amt2.pbFormat = null;
                            if(!filter_out::create(&amt2, pcallback, &pplayer->pfo_v))
                            {
                                error("filter_out::create", 0);
                            }
                            else
#endif
                            {
                                hr = pplayer->pfi->FindPin(L"Output", &pplayer->pp);
                                if(hr != S_OK)
                                {
                                    error("filter_in::FindPin", hr);
                                }
                                else
                                {
#ifdef ENABLE_JSR_135_CONT_FLV_DSHOW_INT
                                    hr = flv_splitter_create(null, IID_IBaseFilter,
                                        (void **)&pplayer->pbf_flv_split);
                                    if(hr != S_OK)
                                    {
                                        error("FlvSplitCreateInstance", hr);
                                    }
                                    else
#endif
                                    {
#ifdef ENABLE_JSR_135_FMT_VP6_DSHOW_INT
                                        hr = flv_decoder_create(null, IID_IBaseFilter,
                                            (void **)&pplayer->pbf_flv_dec);
                                        if(hr != S_OK)
                                        {
                                            error("FlvDecVP6CreateInstance", hr);
                                        }
                                        else
#endif
                                        {
                                            hr = pplayer->pgb->AddFilter(pplayer->pfi, L"Input filter");
                                            if(hr != S_OK)
                                            {
                                                error("IGraphBuilder::AddFilter(Input filter)", hr);
                                            }
                                            else
                                            {
#ifdef ENABLE_JSR_135_DSHOW_AUDIO_OUTPUT_FILTER
                                                hr = pplayer->pgb->AddFilter(pplayer->pfo_a, L"Output audio filter");
                                                if(hr != S_OK)
                                                {
                                                    error("IGraphBuilder::AddFilter(Output audio filter)", hr);
                                                }
                                                else
#endif
                                                {
#ifdef ENABLE_JSR_135_DSHOW_VIDEO_OUTPUT_FILTER
                                                    hr = pplayer->pgb->AddFilter(pplayer->pfo_v, L"Output video filter");
                                                    if(hr != S_OK)
                                                    {
                                                        error("IGraphBuilder::AddFilter(Output video filter)", hr);
                                                    }
                                                    else
#endif
                                                    {
#ifdef ENABLE_JSR_135_CONT_FLV_DSHOW_INT
                                                        hr = pplayer->pgb->AddFilter(pplayer->pbf_flv_split, L"FLV splitter");
                                                        if(hr != S_OK)
                                                        {
                                                            error("IGraphBuilder::AddFilter(FLV splitter)", hr);
                                                        }
                                                        else
#endif
                                                        {
#ifdef ENABLE_JSR_135_FMT_VP6_DSHOW_INT
                                                            hr = pplayer->pgb->AddFilter(pplayer->pbf_flv_dec, L"FLV decoder");
                                                            if(hr != S_OK)
                                                            {
                                                                error("IGraphBuilder::AddFilter(FLV decoder)", hr);
                                                            }
                                                            else
#endif
                                                            {
                                                                r = true;
                                                            }
                                                        }
                                                    }
                                                }
                                            }
#ifdef ENABLE_JSR_135_FMT_VP6_DSHOW_INT
                                            if(!r) pplayer->pbf_flv_dec->Release();
#endif
                                        }
#ifdef ENABLE_JSR_135_CONT_FLV_DSHOW_INT
                                        if(!r) pplayer->pbf_flv_split->Release();
#endif
                                    }
                                    if(!r) pplayer->pp->Release();
                                }
#ifdef ENABLE_JSR_135_DSHOW_VIDEO_OUTPUT_FILTER
                                if(!r) pplayer->pfo_v->Release();
#endif
                            }
#ifdef ENABLE_JSR_135_DSHOW_AUDIO_OUTPUT_FILTER
                            if(!r) pplayer->pfo_a->Release();
#endif
                        }
                        if(!r) pplayer->pfi->Release();
                    }
                    if(!r) pplayer->pms->Release();
                }
                if(!r) pplayer->pmc->Release();
            }
            if(!r) pplayer->pgb->Release();
        }
        if(!r) CoUninitialize();
    }
    if(!r)
    {
        delete pplayer;
        return false;
    }
    *ppplayer = pplayer;
    return true;
}
