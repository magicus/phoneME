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
#include <wmsdkidl.h>

#include <initguid.h>

#pragma warning(push)
#pragma warning(disable : 4201) // nonstandard extension used : nameless struct/union
#include <dxva.h>
#pragma warning(pop)

#include <wmcodecdsp.h>

#include "types.hpp"


void print(const char8 *fmt, ...)
{
    char str8[1024];
    va_list args;

    va_start(args, fmt);
    vsprintf_s(str8, 1024, fmt, args);
    va_end(args);

    // printf("%s", str8);
    // printf("%x %s", GetCurrentThreadId(), str8);
    // OutputDebugStringA(str8);
}

void print(const char16 *fmt, ...)
{
    WCHAR str16[1024];
    va_list args;

    va_start(args, fmt);
    vswprintf_s(str16, 1024, fmt, args);
    va_end(args);

    // wprintf(L"%s", str16);
    // wprintf(L"%x %s", GetCurrentThreadId(), str16);
    // OutputDebugStringW(str16);
}

void error(HRESULT hr)
{
    print("Errorcode: %x.", hr);
    WCHAR *buf;
    DWORD l = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_IGNORE_INSERTS |
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, hr, 0, (WCHAR *)&buf, 0, NULL);
    if(l)
    {
        print(" ");
        for(DWORD i = 0; i < l; i++)
        {
            if(buf[i] >= 32) print(L"%c", buf[i]);
        }
        LocalFree(buf);
    }
    print("\n");
}

void error(const char8 *str, HRESULT hr)
{
    print("%s. Errorcode: %x.", str, hr);
    WCHAR *buf;
    DWORD l = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_IGNORE_INSERTS | 
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, hr, 0, (WCHAR *)&buf, 0, NULL);
    if(l)
    {
        print(" ");
        for(DWORD i = 0; i < l; i++)
        {
            if(buf[i] >= 32) print(L"%c", buf[i]);
        }
        LocalFree(buf);
    }
    print("\n");
}


// {ebe1fb08-3957-47ca-af13-5827e5442e56}
DEFINE_GUID(IID_IDirectVobSub,
0xebe1fb08, 0x3957, 0x47ca, 0xaf, 0x13, 0x58, 0x27, 0xe5, 0x44, 0x2e, 0x56);

// {da395fa3-4a3e-4d85-805e-0beff53d4bcd}
DEFINE_GUID(IID_IStreamSwitcherInputPin,
0xda395fa3, 0x4a3e, 0x4d85, 0x80, 0x5e, 0x0b, 0xef, 0xf5, 0x3d, 0x4b, 0xcd);

// {6ddb4ee7-45a0-4459-a508-bd77b32c91b2}
DEFINE_GUID(IID_ISyncReader,
0x6ddb4ee7, 0x45a0, 0x4459, 0xa5, 0x08, 0xbd, 0x77, 0xb3, 0x2c, 0x91, 0xb2);

// {08e22ada-b715-45ed-9d20-7b87750301d4}
DEFINE_GUID(MEDIASUBTYPE_MP4,
0x08e22ada, 0xb715, 0x45ed, 0x9d, 0x20, 0x7b, 0x87, 0x75, 0x03, 0x01, 0xd4);

// {59333afb-9992-4aa3-8c31-7fb03f6ffdf3}
DEFINE_GUID(MEDIASUBTYPE_FLV,
0x59333afb, 0x9992, 0x4aa3, 0x8c, 0x31, 0x7f, 0xb0, 0x3f, 0x6f, 0xfd, 0xf3);

// {31564c46-0000-0010-8000-00aa00389b71}
DEFINE_GUID(MEDIASUBTYPE_FLV1,
0x31564c46, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

// {34564c46-0000-0010-8000-00aa00389b71}
DEFINE_GUID(MEDIASUBTYPE_FLV4,
0x34564c46, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

// {35564c46-0000-0010-8000-00aa00389b71}
DEFINE_GUID(MEDIASUBTYPE_FLV5,
0x35564c46, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

// {35564c46-0000-0010-8000-00aa00389b71}
DEFINE_GUID(MEDIASUBTYPE_S263,
0x33363253, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

// {35564c46-0000-0010-8000-00aa00389b71}
DEFINE_GUID(MEDIASUBTYPE_s263,
0x33363273, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

// {726d6173-0000-0010-8000-00aa00389b71}
DEFINE_GUID(MEDIASUBTYPE_samr,
0x726d6173, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);


struct guid_item
{
    GUID guid;
    const CHAR *name;
};

#define define_guid_item(name) name, #name

static const guid_item guid_items[]=
{
    define_guid_item(DXVA_ModeWMV9_A                ),
    define_guid_item(DXVA_ModeWMV9_B                ),
    define_guid_item(DXVA_ModeWMV9_C                ),
    define_guid_item(FORMAT_MPEGVideo               ),
    define_guid_item(FORMAT_None                    ),
    define_guid_item(FORMAT_VideoInfo               ),
    define_guid_item(FORMAT_VideoInfo2              ),
    define_guid_item(FORMAT_WaveFormatEx            ),
    define_guid_item(GUID_NULL                      ),
    define_guid_item(IID_IAMDeviceRemoval           ),
    define_guid_item(IID_IAMFilterMiscFlags         ),
    define_guid_item(IID_IAMOpenProgress            ),
    define_guid_item(IID_IAMPushSource              ),
    define_guid_item(IID_IAMVfwCaptureDialogs       ),
    define_guid_item(IID_IAsyncReader               ),
    define_guid_item(IID_IBasicAudio                ),
    define_guid_item(IID_IBasicVideo                ),
    define_guid_item(IID_IDirectVobSub              ),
    define_guid_item(IID_IFileSourceFilter          ),
    define_guid_item(IID_IKsPropertySet             ),
    define_guid_item(IID_IMediaPosition             ),
    define_guid_item(IID_IMediaSeeking              ),
    define_guid_item(IID_IMemInputPin               ),
    define_guid_item(IID_IPersist                   ),
    define_guid_item(IID_IReferenceClock            ),
    define_guid_item(IID_IStream                    ),
    define_guid_item(IID_IStreamBuilder             ),
    define_guid_item(IID_IStreamSwitcherInputPin    ),
    define_guid_item(IID_ISyncReader                ),
    define_guid_item(IID_IUnknown                   ),
    define_guid_item(IID_IVideoWindow               ),
    define_guid_item(MEDIASUBTYPE_AIFF              ),
    define_guid_item(MEDIASUBTYPE_ARGB32            ),
    define_guid_item(MEDIASUBTYPE_AU                ),
    define_guid_item(MEDIASUBTYPE_Asf               ),
    define_guid_item(MEDIASUBTYPE_Avi               ),
    define_guid_item(MEDIASUBTYPE_CLJR              ),
    define_guid_item(MEDIASUBTYPE_DOLBY_AC3         ),
    define_guid_item(MEDIASUBTYPE_DOLBY_AC3_SPDIF   ),
    define_guid_item(MEDIASUBTYPE_DRM_Audio         ),
    define_guid_item(MEDIASUBTYPE_DVD_LPCM_AUDIO    ),
    define_guid_item(MEDIASUBTYPE_DssAudio          ),
    define_guid_item(MEDIASUBTYPE_DssVideo          ),
    define_guid_item(MEDIASUBTYPE_FLV               ),
    define_guid_item(MEDIASUBTYPE_FLV1              ),
    define_guid_item(MEDIASUBTYPE_FLV4              ),
    define_guid_item(MEDIASUBTYPE_FLV5              ),
    define_guid_item(MEDIASUBTYPE_IEEE_FLOAT        ),
    define_guid_item(MEDIASUBTYPE_MP4               ),
    define_guid_item(MEDIASUBTYPE_MPEG1Audio        ),
    define_guid_item(MEDIASUBTYPE_MPEG1AudioPayload ),
    define_guid_item(MEDIASUBTYPE_MPEG1Packet       ),
    define_guid_item(MEDIASUBTYPE_MPEG1Payload      ),
    define_guid_item(MEDIASUBTYPE_MPEG1System       ),
    define_guid_item(MEDIASUBTYPE_MPEG1Video        ),
    define_guid_item(MEDIASUBTYPE_MPEG1VideoCD      ),
    define_guid_item(MEDIASUBTYPE_MPEG2_AUDIO       ),
    define_guid_item(MEDIASUBTYPE_MSAUDIO1          ),
    define_guid_item(MEDIASUBTYPE_NV11              ),
    define_guid_item(MEDIASUBTYPE_NV12              ),
    define_guid_item(MEDIASUBTYPE_None              ),
    define_guid_item(MEDIASUBTYPE_PCM               ),
    define_guid_item(MEDIASUBTYPE_RAW_SPORT         ),
    define_guid_item(MEDIASUBTYPE_RGB24             ),
    define_guid_item(MEDIASUBTYPE_RGB32             ),
    define_guid_item(MEDIASUBTYPE_RGB555            ),
    define_guid_item(MEDIASUBTYPE_RGB565            ),
    define_guid_item(MEDIASUBTYPE_RGB8              ),
    define_guid_item(MEDIASUBTYPE_S263              ),
    define_guid_item(MEDIASUBTYPE_SPDIF_TAG_241h    ),
    define_guid_item(MEDIASUBTYPE_samr              ),
    define_guid_item(MEDIASUBTYPE_UYVY              ),
    define_guid_item(MEDIASUBTYPE_WAVE              ),
    define_guid_item(MEDIASUBTYPE_WMVR              ),
    define_guid_item(MEDIASUBTYPE_Y41P              ),
    define_guid_item(MEDIASUBTYPE_YUY2              ),
    define_guid_item(MEDIASUBTYPE_YV12              ),
    define_guid_item(MEDIASUBTYPE_YVYU              ),
    define_guid_item(MEDIASUBTYPE_s263              ),
    define_guid_item(MEDIATYPE_AUXLine21Data        ),
    define_guid_item(MEDIATYPE_AnalogAudio          ),
    define_guid_item(MEDIATYPE_AnalogVideo          ),
    define_guid_item(MEDIATYPE_Audio                ),
    define_guid_item(MEDIATYPE_File                 ),
    define_guid_item(MEDIATYPE_Interleaved          ),
    define_guid_item(MEDIATYPE_LMRT                 ),
    define_guid_item(MEDIATYPE_MPEG2_PES            ),
    define_guid_item(MEDIATYPE_MPEG2_SECTIONS       ),
    define_guid_item(MEDIATYPE_Midi                 ),
    define_guid_item(MEDIATYPE_ScriptCommand        ),
    define_guid_item(MEDIATYPE_Stream               ),
    define_guid_item(MEDIATYPE_Text                 ),
    define_guid_item(MEDIATYPE_Timecode             ),
    define_guid_item(MEDIATYPE_URL_STREAM           ),
    define_guid_item(MEDIATYPE_Video                ),
    define_guid_item(WMMEDIASUBTYPE_ACELPnet        ),
    define_guid_item(WMMEDIASUBTYPE_MP3             ),
    define_guid_item(WMMEDIASUBTYPE_WMAudioV8       ),
    define_guid_item(WMMEDIASUBTYPE_WMAudioV9       ),
    define_guid_item(WMMEDIASUBTYPE_WMAudio_Lossless),
    define_guid_item(WMMEDIASUBTYPE_WMSP1           ),
    define_guid_item(WMMEDIASUBTYPE_WMV1            ),
    define_guid_item(WMMEDIASUBTYPE_WMV2            ),
    define_guid_item(WMMEDIASUBTYPE_WMV3            ),
    define_guid_item(WMMEDIASUBTYPE_WMVA            ),
    define_guid_item(WMMEDIASUBTYPE_WMVP            ),
    define_guid_item(WMMEDIASUBTYPE_WVC1            ),
    define_guid_item(WMMEDIASUBTYPE_WVP2            )
};

void print(GUID guid)
{
    for(SIZE_T i = 0; i < sizeof(guid_items) / sizeof(guid_items[0]); i++)
    {
        if(guid_items[i].guid == guid)
        {
            print("%s", guid_items[i].name);
            return;
        }
    }
    print("unknown (%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x)",
        guid.Data1, guid.Data2, guid.Data3,
        guid.Data4[0], guid.Data4[1],
        guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
}


void dump_media_type(const AM_MEDIA_TYPE *pamt)
{
    print("majortype=");
    print(pamt->majortype);
    print(", subtype=");
    print(pamt->subtype);
    print(", bFixedSizeSamples=%s", pamt->bFixedSizeSamples ? "TRUE" : "FALSE");
    print(", bTemporalCompression=%s", pamt->bTemporalCompression ? "TRUE" : "FALSE");
    print(", lSampleSize=%u", pamt->lSampleSize);
    print(", formattype=");
    print(pamt->formattype);
    print(", pUnk=0x%08x", UINT32(UINT64(pamt->pUnk)));
    print(", cbFormat=%u", pamt->cbFormat);
    print(", pbFormat=0x%08x", UINT32(UINT64(pamt->pbFormat)));
    if(pamt->formattype == FORMAT_WaveFormatEx && pamt->cbFormat >= sizeof(WAVEFORMATEX) && pamt->pbFormat)
    {
        const WAVEFORMATEX *pwfe = (const WAVEFORMATEX *)pamt->pbFormat;
        if(pwfe->wFormatTag == WAVE_FORMAT_MPEG && pamt->cbFormat >= sizeof(MPEG1WAVEFORMAT))
        {
            const MPEG1WAVEFORMAT *pm1wf = (const MPEG1WAVEFORMAT *)pamt->pbFormat;
            print(", wFormatTag=WAVE_FORMAT_MPEG");
            print(", nChannels=%u"      , pm1wf->wfx.nChannels      );
            print(", nSamplesPerSec=%u" , pm1wf->wfx.nSamplesPerSec );
            print(", nAvgBytesPerSec=%u", pm1wf->wfx.nAvgBytesPerSec);
            print(", nBlockAlign=%u"    , pm1wf->wfx.nBlockAlign    );
            print(", wBitsPerSample=%u" , pm1wf->wfx.wBitsPerSample );
            print(", cbSize=%u"         , pm1wf->wfx.cbSize         );
            print(", fwHeadLayer=%u"    , pm1wf->fwHeadLayer        );
            print(", dwHeadBitrate=%u"  , pm1wf->dwHeadBitrate      );
            print(", fwHeadMode=%u"     , pm1wf->fwHeadMode         );
            print(", fwHeadModeExt=%u"  , pm1wf->fwHeadModeExt      );
            print(", wHeadEmphasis=%u"  , pm1wf->wHeadEmphasis      );
            print(", fwHeadFlags=%u"    , pm1wf->fwHeadFlags        );
            print(", dwPTSLow=%u"       , pm1wf->dwPTSLow           );
            print(", dwPTSHigh=%u"      , pm1wf->dwPTSHigh          );
        }
        else if(pwfe->wFormatTag == WAVE_FORMAT_MPEGLAYER3 && pamt->cbFormat >= sizeof(MPEGLAYER3WAVEFORMAT))
        {
            const MPEGLAYER3WAVEFORMAT *pml3wf = (const MPEGLAYER3WAVEFORMAT *)pamt->pbFormat;
            print(", wFormatTag=WAVE_FORMAT_MPEGLAYER3");
            print(", nChannels=%u"      , pml3wf->wfx.nChannels      );
            print(", nSamplesPerSec=%u" , pml3wf->wfx.nSamplesPerSec );
            print(", nAvgBytesPerSec=%u", pml3wf->wfx.nAvgBytesPerSec);
            print(", nBlockAlign=%u"    , pml3wf->wfx.nBlockAlign    );
            print(", wBitsPerSample=%u" , pml3wf->wfx.wBitsPerSample );
            print(", cbSize=%u"         , pml3wf->wfx.cbSize         );
            print(", wID=%u"            , pml3wf->wID                );
            print(", fdwFlags=%u"       , pml3wf->fdwFlags           );
            print(", nBlockSize=%u"     , pml3wf->nBlockSize         );
            print(", nFramesPerBlock=%u", pml3wf->nFramesPerBlock    );
            print(", nCodecDelay=%u"    , pml3wf->nCodecDelay        );
        }
        else
        {
            if(pwfe->wFormatTag == WAVE_FORMAT_PCM)
            {
                print(", wFormatTag=WAVE_FORMAT_PCM");
            }
            else if(pwfe->wFormatTag == WAVE_FORMAT_WMAUDIO2)
            {
                print(", wFormatTag=WAVE_FORMAT_WMAUDIO2");
            }
            else
            {
                print(", wFormatTag=%u", pwfe->wFormatTag);
            }
            print(", nChannels=%u"      , pwfe->nChannels      );
            print(", nSamplesPerSec=%u" , pwfe->nSamplesPerSec );
            print(", nAvgBytesPerSec=%u", pwfe->nAvgBytesPerSec);
            print(", nBlockAlign=%u"    , pwfe->nBlockAlign    );
            print(", wBitsPerSample=%u" , pwfe->wBitsPerSample );
            print(", cbSize=%u"         , pwfe->cbSize         );
        }
    }
}

bool dump_media_types(IPin *pp, nat32 indent)
{
    bool r = false;
    HRESULT hr;
    IEnumMediaTypes *pemt;
    hr = pp->EnumMediaTypes(&pemt);
    if(hr != S_OK)
    {
        // error("IPin::EnumMediaTypes failed", hr);
        r = true;
    }
    else
    {
        AM_MEDIA_TYPE *pamt;
        for(;;)
        {
            hr = pemt->Next(1, &pamt, NULL);
            if(hr != S_OK)
            {
                if(hr == S_FALSE) r = true;
                else error("EnumMediaTypes::Next failed", hr);
                break;
            }
            for(UINT32 i = 0; i < indent; i++) print(" ");
            dump_media_type(pamt);
            print("\n");
            if(pamt->pUnk) pamt->pUnk->Release();
            if(pamt->cbFormat) CoTaskMemFree(pamt->pbFormat);
            CoTaskMemFree(pamt);
        }
        ULONG rc = pemt->Release();
        if(rc)
        {
            print("IEnumMediaTypes::Release returns %u\n", rc);
            r = false;
        }
    }
    return r;
}

bool dump_pin(IPin *pp, nat32 indent)
{
    bool r = false;
    HRESULT hr;
    LPWSTR id;
    hr = pp->QueryId(&id);
    if(hr != S_OK)
    {
        error("IPin::QueryId failed", hr);
    }
    else
    {
        PIN_INFO pi;
        hr = pp->QueryPinInfo(&pi);
        if(hr != S_OK)
        {
            error("IPin::QueryPinInfo failed", hr);
        }
        else
        {
            for(UINT32 i = 0; i < indent; i++) print(" ");
            print("0x%08x ", UINT32(UINT64(pp)));
            print(L"%s", id);
            IAMFilterMiscFlags *pamfmf;
            if(pp->QueryInterface(IID_IAMFilterMiscFlags, (void **)&pamfmf) == S_OK)
            {
                print(" IAMFilterMiscFlags");
                pamfmf->Release();
            }
            IAsyncReader *par;
            if(pp->QueryInterface(IID_IAsyncReader, (void **)&par) == S_OK)
            {
                par->Release();
                print(" IAsyncReader");
            }
            IBasicAudio *pba;
            if(pp->QueryInterface(IID_IBasicAudio, (void **)&pba) == S_OK)
            {
                print(" IBasicAudio");
                pba->Release();
            }
            IBasicVideo *pbv;
            if(pp->QueryInterface(IID_IBasicVideo, (void **)&pbv) == S_OK)
            {
                print(" IBasicVideo");
                pbv->Release();
            }
            IKsPropertySet *pkps;
            if(pp->QueryInterface(IID_IKsPropertySet, (void **)&pkps) == S_OK)
            {
                print(" IKsPropertySet");
                pkps->Release();
            }
            IMediaPosition *pmp;
            if(pp->QueryInterface(IID_IMediaPosition, (void **)&pmp) == S_OK)
            {
                print(" IMediaPosition");
                pmp->Release();
            }
            IMediaSeeking *pms;
            if(pp->QueryInterface(IID_IMediaSeeking, (void **)&pms) == S_OK)
            {
                print(" IMediaSeeking");
                pms->Release();
            }
            IMemInputPin *pmip;
            if(pp->QueryInterface(IID_IMemInputPin, (void **)&pmip) == S_OK)
            {
                print(" IMemInputPin");
                if(pmip->ReceiveCanBlock() == S_OK) print("(blocking)");
                pmip->Release();
            }
            IReferenceClock *prc;
            if(pp->QueryInterface(IID_IReferenceClock, (void **)&prc) == S_OK)
            {
                print(" IReferenceClock");
                prc->Release();
            }
            IVideoWindow *pvw;
            if(pp->QueryInterface(IID_IVideoWindow, (void **)&pvw) == S_OK)
            {
                print(" IVideoWindow");
                pvw->Release();
            }
            print(" 0x%08x", UINT32(UINT64(pi.pFilter)));
            print(" %u ", pi.dir);
            print(L"%s", pi.achName);
            if(pi.pFilter) pi.pFilter->Release();
            IPin *pp2;
            hr = pp->ConnectedTo(&pp2);
            if(hr == S_OK)
            {
                print(" 0x%08x", UINT32(UINT64(pp2)));
                pp2->Release();
            }
            AM_MEDIA_TYPE amt;
            hr = pp->ConnectionMediaType(&amt);
            if(hr == S_OK)
            {
                print(" ");
                dump_media_type(&amt);
                if(amt.pUnk) amt.pUnk->Release();
                if(amt.cbFormat) CoTaskMemFree(amt.pbFormat);
            }
            print("\n");
            if(dump_media_types(pp, indent + 4)) r = true;
        }
        CoTaskMemFree(id);
    }
    return r;
}

bool dump_pins(IBaseFilter *pbf, nat32 indent)
{
    bool r = false;
    HRESULT hr;
    IEnumPins *pep;
    hr = pbf->EnumPins(&pep);
    if(hr != S_OK)
    {
        error("IBaseFilter::EnumPins failed", hr);
    }
    else
    {
        IPin *pp;
        for(;;)
        {
            hr = pep->Next(1, &pp, NULL);
            if(hr != S_OK)
            {
                if(hr == S_FALSE) r = true;
                else error("IEnumPins::Next failed", hr);
                break;
            }
            if(!dump_pin(pp, indent))
            {
                pp->Release();
                break;
            }
            pp->Release();
        }
        ULONG rc = pep->Release();
        if(rc)
        {
            print("IEnumPins::Release returns %u\n", rc);
            r = false;
        }
    }
    return r;
}

bool dump_filter(IBaseFilter *pbf, nat32 indent)
{
    bool r = false;
    HRESULT hr;
    FILTER_INFO fi;
    hr = pbf->QueryFilterInfo(&fi);
    if(hr != S_OK)
    {
        error("IBaseFilter::QueryFilterInfo failed", hr);
    }
    else
    {
        for(UINT32 i = 0; i < indent; i++) print(" ");
        print("0x%08x", UINT32(UINT64(pbf)));
        print(L" %s", fi.achName);
        CLSID clsid;
        if(pbf->GetClassID(&clsid) == S_OK)
        {
            print(" ");
            print(clsid);
        }
        IAMFilterMiscFlags *pamfmf;
        if(pbf->QueryInterface(IID_IAMFilterMiscFlags, (void **)&pamfmf) == S_OK)
        {
            print(" IAMFilterMiscFlags");
            pamfmf->Release();
        }
        IAsyncReader *par;
        if(pbf->QueryInterface(IID_IAsyncReader, (void **)&par) == S_OK)
        {
            par->Release();
            print(" IAsyncReader");
        }
        IBasicAudio *pba;
        if(pbf->QueryInterface(IID_IBasicAudio, (void **)&pba) == S_OK)
        {
            print(" IBasicAudio");
            pba->Release();
        }
        IBasicVideo *pbv;
        if(pbf->QueryInterface(IID_IBasicVideo, (void **)&pbv) == S_OK)
        {
            print(" IBasicVideo");
            pbv->Release();
        }
        IKsPropertySet *pkps;
        if(pbf->QueryInterface(IID_IKsPropertySet, (void **)&pkps) == S_OK)
        {
            print(" IKsPropertySet");
            pkps->Release();
        }
        IMediaPosition *pmp;
        if(pbf->QueryInterface(IID_IMediaPosition, (void **)&pmp) == S_OK)
        {
            print(" IMediaPosition");
            pmp->Release();
        }
        IMediaSeeking *pms;
        if(pbf->QueryInterface(IID_IMediaSeeking, (void **)&pms) == S_OK)
        {
            print(" IMediaSeeking");
            pms->Release();
        }
        IMemInputPin *pmip;
        if(pbf->QueryInterface(IID_IMemInputPin, (void **)&pmip) == S_OK)
        {
            print(" IMemInputPin");
            if(pmip->ReceiveCanBlock() == S_OK) print("(blocking)");
            pmip->Release();
        }
        IReferenceClock *prc;
        if(pbf->QueryInterface(IID_IReferenceClock, (void **)&prc) == S_OK)
        {
            print(" IReferenceClock");
            prc->Release();
        }
        IVideoWindow *pvw;
        if(pbf->QueryInterface(IID_IVideoWindow, (void **)&pvw) == S_OK)
        {
            print(" IVideoWindow");
            pvw->Release();
        }
        print(" 0x%08x", UINT32(UINT64(fi.pGraph)));
        if(fi.pGraph) fi.pGraph->Release();
        LPWSTR vi;
        hr = pbf->QueryVendorInfo(&vi);
        if(hr == S_OK)
        {
            print(L" %s", vi);
            CoTaskMemFree(vi);
        }
        print("\n");
        if(dump_pins(pbf, indent + 4)) r = true;
    }
    return r;
}

bool dump_filters(IFilterGraph *pfg, nat32 indent)
{
    bool r = false;
    HRESULT hr;
    IEnumFilters *pef;
    hr = pfg->EnumFilters(&pef);
    if(hr != S_OK)
    {
        error("IFilterGraph::EnumFilters failed", hr);
    }
    else
    {
        IBaseFilter *pbf;
        for(;;)
        {
            hr = pef->Next(1, &pbf, NULL);
            if(hr != S_OK)
            {
                if(hr == S_FALSE) r = true;
                else error("EnumFilters::Next failed", hr);
                break;
            }
            if(!dump_filter(pbf, indent))
            {
                pbf->Release();
                break;
            }
            pbf->Release();
        }
        ULONG rc = pef->Release();
        if(rc)
        {
            print("EnumFilters::Release returns %u\n", rc);
            r = false;
        }
    }
    return r;
}

bool dump_filter_graph(IFilterGraph *pfg, nat32 indent)
{
    bool r = false;
    for(UINT32 i = 0; i < indent; i++) print(" ");
    print("0x%08x\n", UINT32(UINT64(pfg)));
    if(dump_filters(pfg, indent+4)) r = true;
    return r;
}
