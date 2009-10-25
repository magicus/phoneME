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

#define write_level 1

#if write_level > 0
#include "writer.hpp"
#endif

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
#undef ENABLE_JSR_135_DSHOW_AUDIO_OUTPUT_FILTER
#undef ENABLE_JSR_135_DSHOW_VIDEO_OUTPUT_FILTER


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
    nat32 locator_len;
    char16 *plocator;
    player_callback *pcallback;
    player::state state;
    int64 media_time;
    IGraphBuilder *pgb;
    IMediaControl *pmc;
    IMediaSeeking *pms;
    filter_in *pfi;
#ifdef ENABLE_JSR_135_DSHOW_AUDIO_OUTPUT_FILTER
    IBaseFilter *pfo_a;
#endif
#ifdef ENABLE_JSR_135_DSHOW_VIDEO_OUTPUT_FILTER
    IBaseFilter *pfo_v;
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
    }

    result get_state(player::state *p_state)
    {
#if write_level > 0
        print("player_dshow::get_state called...\n");
#endif
        *p_state = state;

        return result_success;
    }

    result stop()
    {
#if write_level > 0
        print("player_dshow::stop called...\n");
#endif
        HRESULT hr = pmc->Stop();
        if(hr != S_OK && hr != S_FALSE)
        {
#if write_level > 0
            error("IMediaControl::Stop", hr);
#endif
            return result_media;
        }

        state = stopped;

        return result_success;
    }

    result pause()
    {
#if write_level > 0
        print("player_dshow::pause called...\n");
#endif
        HRESULT hr = pmc->Pause();
        if(hr != S_OK && hr != S_FALSE)
        {
#if write_level > 0
            error("IMediaControl::Pause", hr);
#endif
            return result_media;
        }

        state = paused;

        return result_success;
    }

    result run()
    {
#if write_level > 0
        print("player_dshow::run called...\n");
#endif
        HRESULT hr = pmc->Run();
        if(hr != S_OK && hr != S_FALSE)
        {
#if write_level > 0
            error("IMediaControl::Run", hr);
#endif
            return result_media;
        }

        state = running;

        return result_success;
    }

    result destroy()
    {
#if write_level > 0
        print("player_dshow::destroy called...\n");
        pgb->AddRef();
        print("Reference count is %u...\n", pgb->Release());
#endif
#if write_level > 0
        if(!pgb)
        {
            print("player_dshow::destroy called twice!\n");
            return result_illegal_state;
        }
#endif
        pmc->Stop();
        Sleep(100);
#ifdef ENABLE_JSR_135_FMT_VP6_DSHOW_INT
        pbf_flv_dec->Release();
#endif
#ifdef ENABLE_JSR_135_CONT_FLV_DSHOW_INT
        pbf_flv_split->Release();
#endif
        if(!locator_len) pp->Release();
        if(!locator_len) pfi->Release();
        pms->Release();
        pmc->Release();
#if write_level > 0
        pgb->AddRef();
        nat32 rc = pgb->Release();
        print("Reference count is %u...\n", rc);
#endif
        pgb->Release();
        pgb = null;
#ifdef ENABLE_JSR_135_DSHOW_VIDEO_OUTPUT_FILTER
        pfo_v->Release();
#endif
#ifdef ENABLE_JSR_135_DSHOW_AUDIO_OUTPUT_FILTER
        pfo_a->Release();
#endif
        CoUninitialize();
        if(locator_len) delete[] plocator;
        delete this;

        return result_success;
    }

    result get_media_time(int64 *p_time)
    {
#if write_level > 0
        print("player_dshow::get_media_time called...\n");
#endif
        LONGLONG cur;
        HRESULT hr = pms->GetCurrentPosition(&cur);
        if(hr != S_OK)
        {
#if write_level > 0
            error("IMediaSeeking::GetCurrentPosition", hr);
#endif
            *p_time = media_time;
            return result_media;
        }

        cur /= 10;
        media_time = cur;
        *p_time = cur;

        return result_success;
    }

    result set_media_time(int64 time_requested, int64 *p_time_actual)
    {
#if write_level > 0
        print("player_dshow::set_media_time called...\n");
#endif
        LONGLONG cur = time_requested * 10;
        HRESULT hr = pms->SetPositions(&cur, AM_SEEKING_AbsolutePositioning, null, AM_SEEKING_NoPositioning);
        if(hr != S_OK)
        {
#if write_level > 0
            error("IMediaSeeking::SetPositions", hr);
#endif
            *p_time_actual = media_time;
            return result_media;
        }

        media_time = time_requested;
        *p_time_actual = time_requested;

        return result_success;
    }

    result get_duration(int64 *p_duration)
    {
#if write_level > 0
        print("player_dshow::get_duration called...\n");
#endif
        LONGLONG duration;
        HRESULT hr = pms->GetDuration(&duration);
        if(hr != S_OK)
        {
#if write_level > 0
            error("IMediaSeeking::GetDuration", hr);
#endif
            return result_media;
        }

        *p_duration = duration / 10;

        return result_success;
    }

    result set_loop_count(int32 count)
    {
#if write_level > 0
        print("player_dshow::set_loop_count called...\n");
#endif
        return result_media;
    }

    result set_stream_length(int64 length)
    {
#if write_level > 0
        print("player_dshow::set_stream_length called...\n");
#endif
        if(pfi->set_stream_length(length))
            return result_success;
        return result_media;
    }

    friend bool create_locator_player_dshow(nat32 len, const char16 *plocator, player_callback *pcallback, player **ppplayer);
    friend bool create_stream_player_dshow(nat32 len, const char16 *pformat, bool stream_length_known, int64 stream_length, player_callback *pcallback, player **ppplayer);
};

bool create_locator_player_dshow(nat32 len, const char16 *plocator, player_callback *pcallback, player **ppplayer)
{
#if write_level > 0
    print(L"player_dshow::create_locator_player_dshow(%s) called...\n", len ? plocator : L"");
#endif
    player_dshow *pplayer;
    if(len > 0x7fffffff || !plocator || !pcallback || !ppplayer)
    {
        return false;
    }

    pplayer = new player_dshow;
    if(!pplayer) return false;

    pplayer->locator_len = len + 1;
    pplayer->plocator = new char16[len + 1];
    if(!pplayer->plocator)
    {
        delete pplayer;
        return false;
    }
    memcpy(pplayer->plocator, plocator, sizeof(char16) * len);
    pplayer->plocator[len] = 0;

    pplayer->pcallback = pcallback;
    pplayer->state = player::stopped;
    pplayer->media_time = player::time_unknown;

    bool r = false;

    HRESULT hr = CoInitializeEx(null, COINIT_MULTITHREADED);
    if(FAILED(hr))
    {
#if write_level > 0
        error("CoInitializeEx", hr);
#endif
    }
    else
    {
        hr = CoCreateInstance(CLSID_FilterGraph, null, CLSCTX_INPROC_SERVER,
            IID_IGraphBuilder, (void **)&pplayer->pgb);
        if(hr != S_OK)
        {
#if write_level > 0
            error("CoCreateInstance", hr);
#endif
        }
        else
        {
            hr = pplayer->pgb->QueryInterface(IID_IMediaControl, (void **)&pplayer->pmc);
            if(hr != S_OK)
            {
#if write_level > 0
                error("IGraphBuilder::QueryInterface(IID_IMediaControl)", hr);
#endif
            }
            else
            {
                hr = pplayer->pgb->QueryInterface(IID_IMediaSeeking, (void **)&pplayer->pms);
                if(hr != S_OK)
                {
#if write_level > 0
                    error("IGraphBuilder::QueryInterface(IID_IMediaSeeking)", hr);
#endif
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
                    if(!create_filter_out(&amt_a, pcallback, &pplayer->pfo_a))
                    {
#if write_level > 0
                        error("filter_out::create(Audio)", 0);
#endif
                    }
                    else
#endif
                    {
#ifdef ENABLE_JSR_135_DSHOW_VIDEO_OUTPUT_FILTER
                        AM_MEDIA_TYPE amt_v;
                        amt_v.majortype = MEDIATYPE_Video;
                        amt_v.subtype = MEDIASUBTYPE_RGB565;
                        amt_v.bFixedSizeSamples = TRUE;
                        amt_v.bTemporalCompression = FALSE;
                        amt_v.lSampleSize = 1;
                        amt_v.formattype = GUID_NULL;
                        amt_v.pUnk = null;
                        amt_v.cbFormat = 0;
                        amt_v.pbFormat = null;
                        if(!create_filter_out(&amt_v, pcallback, &pplayer->pfo_v))
                        {
#if write_level > 0
                            error("filter_out::create", 0);
#endif
                        }
                        else
#endif
                        {
#ifdef ENABLE_JSR_135_CONT_FLV_DSHOW_INT
                            hr = flv_splitter_create(null, IID_IBaseFilter,
                                (void **)&pplayer->pbf_flv_split);
                            if(hr != S_OK)
                            {
#if write_level > 0
                                error("FlvSplitCreateInstance", hr);
#endif
                            }
                            else
#endif
                            {
#ifdef ENABLE_JSR_135_FMT_VP6_DSHOW_INT
                                hr = flv_decoder_create(null, IID_IBaseFilter,
                                    (void **)&pplayer->pbf_flv_dec);
                                if(hr != S_OK)
                                {
#if write_level > 0
                                    error("FlvDecVP6CreateInstance", hr);
#endif
                                }
                                else
#endif
                                {
#ifdef ENABLE_JSR_135_DSHOW_AUDIO_OUTPUT_FILTER
                                    hr = pplayer->pgb->AddFilter(pplayer->pfo_a, L"Output audio filter");
                                    if(hr != S_OK)
                                    {
#if write_level > 0
                                        error("IGraphBuilder::AddFilter(Output audio filter)", hr);
#endif
                                    }
                                    else
#endif
                                    {
#ifdef ENABLE_JSR_135_DSHOW_VIDEO_OUTPUT_FILTER
                                        hr = pplayer->pgb->AddFilter(pplayer->pfo_v, L"Output video filter");
                                        if(hr != S_OK)
                                        {
#if write_level > 0
                                            error("IGraphBuilder::AddFilter(Output video filter)", hr);
#endif
                                        }
                                        else
#endif
                                        {
#ifdef ENABLE_JSR_135_CONT_FLV_DSHOW_INT
                                            hr = pplayer->pgb->AddFilter(pplayer->pbf_flv_split, L"FLV splitter");
                                            if(hr != S_OK)
                                            {
#if write_level > 0
                                                error("IGraphBuilder::AddFilter(FLV splitter)", hr);
#endif
                                            }
                                            else
#endif
                                            {
#ifdef ENABLE_JSR_135_FMT_VP6_DSHOW_INT
                                                hr = pplayer->pgb->AddFilter(pplayer->pbf_flv_dec, L"FLV decoder");
                                                if(hr != S_OK)
                                                {
#if write_level > 0
                                                    error("IGraphBuilder::AddFilter(FLV decoder)", hr);
#endif
                                                }
                                                else
#endif
                                                {
                                                    hr = pplayer->pgb->RenderFile(plocator, null);
                                                    if(hr != S_OK)
                                                    {
#if write_level > 0
                                                        error("IGraphBuilder::RenderFile", hr);
#endif
                                                    }
                                                    else
                                                    {
                                                        hr = pplayer->pms->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME);
                                                        if(hr != S_OK)
                                                        {
#if write_level > 0
                                                            error("IMediaSeeking::SetTimeFormat", hr);
#endif
                                                        }
                                                        else
                                                        {
                                                            r = true;
                                                        }
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
#ifdef ENABLE_JSR_135_DSHOW_VIDEO_OUTPUT_FILTER
                            if(!r) pplayer->pfo_v->Release();
#endif
                        }
#ifdef ENABLE_JSR_135_DSHOW_AUDIO_OUTPUT_FILTER
                        if(!r) pplayer->pfo_a->Release();
#endif
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

bool create_stream_player_dshow(nat32 len, const char16 *pformat, bool stream_length_known, int64 stream_length, player_callback *pcallback, player **ppplayer)
{
#if write_level > 0
    print(L"player_dshow::create_stream_player_dshow(%s, %s, %I64i) called...\n", len ? pformat : L"", stream_length_known ? L"true" : L"false", stream_length);
#endif
    player_dshow *pplayer;
    AM_MEDIA_TYPE amt;
    if(len > 0x7fffffff || !pformat || !pcallback || !ppplayer)
    {
        return false;
    }
#ifdef ENABLE_JSR_135_CONT_3GP_DSHOW_EXT
    else if(len >= wcslen(L"video/3gpp") && !wcsncmp(pformat, L"video/3gpp", wcslen(L"video/3gpp")))
    {
        pplayer = new player_dshow;
        if(!pplayer) return false;

        amt.majortype = MEDIATYPE_Stream;
        amt.subtype = GUID_NULL;
        amt.bFixedSizeSamples = TRUE;
        amt.bTemporalCompression = FALSE;
        amt.lSampleSize = 1;
        amt.formattype = GUID_NULL;
        amt.pUnk = null;
        amt.cbFormat = 0;
        amt.pbFormat = null;
    }
#endif
#ifdef ENABLE_JSR_135_CONT_AMR_DSHOW_EXT
    else if(len >= wcslen(L"audio/amr") && !wcsncmp(pformat, L"audio/amr", wcslen(L"audio/amr")))
    {
        pplayer = new player_dshow;
        if(!pplayer) return false;

        amt.majortype = MEDIATYPE_Stream;
        amt.subtype = GUID_NULL;
        amt.bFixedSizeSamples = TRUE;
        amt.bTemporalCompression = FALSE;
        amt.lSampleSize = 1;
        amt.formattype = GUID_NULL;
        amt.pUnk = null;
        amt.cbFormat = 0;
        amt.pbFormat = null;
    }
#endif
#ifdef ENABLE_JSR_135_CONT_AVI_DSHOW_EXT
    else if(len >= wcslen(L"video/avi") && !wcsncmp(pformat, L"video/avi", wcslen(L"video/avi")))
    {
        pplayer = new player_dshow;
        if(!pplayer) return false;

        amt.majortype = MEDIATYPE_Stream;
        amt.subtype = GUID_NULL;
        amt.bFixedSizeSamples = TRUE;
        amt.bTemporalCompression = FALSE;
        amt.lSampleSize = 1;
        amt.formattype = GUID_NULL;
        amt.pUnk = null;
        amt.cbFormat = 0;
        amt.pbFormat = null;
    }
#endif
#if defined ENABLE_JSR_135_CONT_FLV_DSHOW_EXT || defined ENABLE_JSR_135_CONT_FLV_DSHOW_INT
    else if(len >= wcslen(L"video/x-flv") && !wcsncmp(pformat, L"video/x-flv", wcslen(L"video/x-flv")))
    {
        pplayer = new player_dshow;
        if(!pplayer) return false;

        amt.majortype = MEDIATYPE_Stream;
        amt.subtype = GUID_NULL;
        amt.bFixedSizeSamples = TRUE;
        amt.bTemporalCompression = FALSE;
        amt.lSampleSize = 1;
        amt.formattype = GUID_NULL;
        amt.pUnk = null;
        amt.cbFormat = 0;
        amt.pbFormat = null;
    }
#endif
#ifdef ENABLE_JSR_135_CONT_MP3_DSHOW_EXT
    else if(len >= wcslen(L"audio/mpeg") && !wcsncmp(pformat, L"audio/mpeg", wcslen(L"audio/mpeg")))
    {
        pplayer = new player_dshow;
        if(!pplayer) return false;

        amt.majortype = MEDIATYPE_Stream;
        amt.subtype = MEDIASUBTYPE_MPEG1Audio;
        amt.bFixedSizeSamples = TRUE;
        amt.bTemporalCompression = FALSE;
        amt.lSampleSize = 1;
        amt.formattype = GUID_NULL;
        amt.pUnk = null;
        amt.cbFormat = 0;
        amt.pbFormat = null;
    }
#endif
#ifdef ENABLE_JSR_135_CONT_MP4_DSHOW_EXT
    else if(len >= wcslen(L"video/mp4") && !wcsncmp(pformat, L"video/mp4", wcslen(L"video/mp4")))
    {
        pplayer = new player_dshow;
        if(!pplayer) return false;

        amt.majortype = MEDIATYPE_Stream;
        amt.subtype = GUID_NULL;
        amt.bFixedSizeSamples = TRUE;
        amt.bTemporalCompression = FALSE;
        amt.lSampleSize = 1;
        amt.formattype = GUID_NULL;
        amt.pUnk = null;
        amt.cbFormat = 0;
        amt.pbFormat = null;
    }
#endif
#ifdef ENABLE_JSR_135_CONT_MPG_DSHOW_EXT
    else if(len >= wcslen(L"video/mpeg") && !wcsncmp(pformat, L"video/mpeg", wcslen(L"video/mpeg")))
    {
        pplayer = new player_dshow;
        if(!pplayer) return false;

        amt.majortype = MEDIATYPE_Stream;
        amt.subtype = MEDIASUBTYPE_MPEG1System;
        amt.bFixedSizeSamples = TRUE;
        amt.bTemporalCompression = FALSE;
        amt.lSampleSize = 1;
        amt.formattype = GUID_NULL;
        amt.pUnk = null;
        amt.cbFormat = 0;
        amt.pbFormat = null;
    }
#endif
#ifdef ENABLE_JSR_135_CONT_OGG_DSHOW_EXT
    else if(len >= wcslen(L"video/ogg") && !wcsncmp(pformat, L"video/ogg", wcslen(L"video/ogg")))
    {
        pplayer = new player_dshow;
        if(!pplayer) return false;

        amt.majortype = MEDIATYPE_Stream;
        amt.subtype = GUID_NULL;
        amt.bFixedSizeSamples = TRUE;
        amt.bTemporalCompression = FALSE;
        amt.lSampleSize = 1;
        amt.formattype = GUID_NULL;
        amt.pUnk = null;
        amt.cbFormat = 0;
        amt.pbFormat = null;
    }
#endif
#ifdef ENABLE_JSR_135_CONT_WAV_DSHOW_EXT
    else if(len >= wcslen(L"audio/wav") && !wcsncmp(pformat, L"audio/wav", wcslen(L"audio/wav")))
    {
        pplayer = new player_dshow;
        if(!pplayer) return false;

        amt.majortype = MEDIATYPE_Stream;
        amt.subtype = MEDIASUBTYPE_WAVE;
        amt.bFixedSizeSamples = TRUE;
        amt.bTemporalCompression = FALSE;
        amt.lSampleSize = 1;
        amt.formattype = GUID_NULL;
        amt.pUnk = null;
        amt.cbFormat = 0;
        amt.pbFormat = null;
    }
#endif
    else
    {
        return false;
    }

    pplayer->locator_len = 0;
    pplayer->pcallback = pcallback;
    pplayer->state = player::stopped;
    pplayer->media_time = player::time_unknown;

    bool r = false;

    HRESULT hr = CoInitializeEx(null, COINIT_MULTITHREADED);
    if(FAILED(hr))
    {
#if write_level > 0
        error("CoInitializeEx", hr);
#endif
    }
    else
    {
        hr = CoCreateInstance(CLSID_FilterGraph, null, CLSCTX_INPROC_SERVER,
            IID_IGraphBuilder, (void **)&pplayer->pgb);
        if(hr != S_OK)
        {
#if write_level > 0
            error("CoCreateInstance", hr);
#endif
        }
        else
        {
            hr = pplayer->pgb->QueryInterface(IID_IMediaControl, (void **)&pplayer->pmc);
            if(hr != S_OK)
            {
#if write_level > 0
                error("IGraphBuilder::QueryInterface(IID_IMediaControl)", hr);
#endif
            }
            else
            {
                hr = pplayer->pgb->QueryInterface(IID_IMediaSeeking, (void **)&pplayer->pms);
                if(hr != S_OK)
                {
#if write_level > 0
                    error("IGraphBuilder::QueryInterface(IID_IMediaSeeking)", hr);
#endif
                }
                else
                {
                    if(!create_filter_in(&amt, pcallback, &pplayer->pfi))
                    {
#if write_level > 0
                        error("filter_in::create", 0);
#endif
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
                        if(!create_filter_out(&amt_a, pcallback, &pplayer->pfo_a))
                        {
#if write_level > 0
                            error("filter_out::create(Audio)", 0);
#endif
                        }
                        else
#endif
                        {
#ifdef ENABLE_JSR_135_DSHOW_VIDEO_OUTPUT_FILTER
                            AM_MEDIA_TYPE amt_v;
                            amt_v.majortype = MEDIATYPE_Video;
                            amt_v.subtype = MEDIASUBTYPE_RGB565;
                            amt_v.bFixedSizeSamples = TRUE;
                            amt_v.bTemporalCompression = FALSE;
                            amt_v.lSampleSize = 1;
                            amt_v.formattype = GUID_NULL;
                            amt_v.pUnk = null;
                            amt_v.cbFormat = 0;
                            amt_v.pbFormat = null;
                            if(!create_filter_out(&amt_v, pcallback, &pplayer->pfo_v))
                            {
#if write_level > 0
                                error("filter_out::create", 0);
#endif
                            }
                            else
#endif
                            {
                                hr = pplayer->pfi->FindPin(L"Output", &pplayer->pp);
                                if(hr != S_OK)
                                {
#if write_level > 0
                                    error("filter_in::FindPin", hr);
#endif
                                }
                                else
                                {
#ifdef ENABLE_JSR_135_CONT_FLV_DSHOW_INT
                                    hr = flv_splitter_create(null, IID_IBaseFilter,
                                        (void **)&pplayer->pbf_flv_split);
                                    if(hr != S_OK)
                                    {
#if write_level > 0
                                        error("FlvSplitCreateInstance", hr);
#endif
                                    }
                                    else
#endif
                                    {
#ifdef ENABLE_JSR_135_FMT_VP6_DSHOW_INT
                                        hr = flv_decoder_create(null, IID_IBaseFilter,
                                            (void **)&pplayer->pbf_flv_dec);
                                        if(hr != S_OK)
                                        {
#if write_level > 0
                                            error("FlvDecVP6CreateInstance", hr);
#endif
                                        }
                                        else
#endif
                                        {
                                            hr = pplayer->pgb->AddFilter(pplayer->pfi, L"Input filter");
                                            if(hr != S_OK)
                                            {
#if write_level > 0
                                                error("IGraphBuilder::AddFilter(Input filter)", hr);
#endif
                                            }
                                            else
                                            {
#ifdef ENABLE_JSR_135_DSHOW_AUDIO_OUTPUT_FILTER
                                                hr = pplayer->pgb->AddFilter(pplayer->pfo_a, L"Output audio filter");
                                                if(hr != S_OK)
                                                {
#if write_level > 0
                                                    error("IGraphBuilder::AddFilter(Output audio filter)", hr);
#endif
                                                }
                                                else
#endif
                                                {
#ifdef ENABLE_JSR_135_DSHOW_VIDEO_OUTPUT_FILTER
                                                    hr = pplayer->pgb->AddFilter(pplayer->pfo_v, L"Output video filter");
                                                    if(hr != S_OK)
                                                    {
#if write_level > 0
                                                        error("IGraphBuilder::AddFilter(Output video filter)", hr);
#endif
                                                    }
                                                    else
#endif
                                                    {
#ifdef ENABLE_JSR_135_CONT_FLV_DSHOW_INT
                                                        hr = pplayer->pgb->AddFilter(pplayer->pbf_flv_split, L"FLV splitter");
                                                        if(hr != S_OK)
                                                        {
#if write_level > 0
                                                            error("IGraphBuilder::AddFilter(FLV splitter)", hr);
#endif
                                                        }
                                                        else
#endif
                                                        {
#ifdef ENABLE_JSR_135_FMT_VP6_DSHOW_INT
                                                            hr = pplayer->pgb->AddFilter(pplayer->pbf_flv_dec, L"FLV decoder");
                                                            if(hr != S_OK)
                                                            {
#if write_level > 0
                                                                error("IGraphBuilder::AddFilter(FLV decoder)", hr);
#endif
                                                            }
                                                            else
#endif
                                                            {
                                                                if(stream_length_known && !pplayer->pfi->set_stream_length(stream_length))
                                                                {
                                                                }
                                                                else
                                                                {
                                                                    hr = pplayer->pgb->Render(pplayer->pp);
                                                                    if(hr != S_OK)
                                                                    {
#if write_level > 0
                                                                        error("IGraphBuilder::Render", hr);
#endif
                                                                    }
                                                                    else
                                                                    {
                                                                        hr = pplayer->pms->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME);
                                                                        if(hr != S_OK)
                                                                        {
#if write_level > 0
                                                                            error("IMediaSeeking::SetTimeFormat", hr);
#endif
                                                                        }
                                                                        else
                                                                        {
                                                                            r = true;
                                                                        }
                                                                    }
                                                                }
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
