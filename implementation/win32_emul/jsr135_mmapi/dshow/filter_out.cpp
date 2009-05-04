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
#include <dvdmedia.h>
#include <uuids.h>
#include <vfwmsgs.h>
#include "filter_out.hpp"

#define write_level 0

#if write_level > 0
#include "writer.hpp"
#endif


const nat32 null = 0;


class filter_out_filter;
class filter_out_pin;

class filter_out_enum_media_types : public IEnumMediaTypes
{
    friend filter_out_pin;

    nat32 reference_count;
    AM_MEDIA_TYPE amt;
    nat32 index;

    filter_out_enum_media_types();
    ~filter_out_enum_media_types();
public:
    // IUnknown
    virtual HRESULT __stdcall QueryInterface(REFIID riid, void **ppvObject);
    virtual ULONG __stdcall AddRef();
    virtual ULONG __stdcall Release();
    // IEnumMediaTypes
    virtual HRESULT __stdcall Next(ULONG cMediaTypes, AM_MEDIA_TYPE **ppMediaTypes, ULONG *pcFetched);
    virtual HRESULT __stdcall Skip(ULONG cMediaTypes);
    virtual HRESULT __stdcall Reset();
    virtual HRESULT __stdcall Clone(IEnumMediaTypes **ppEnum);
};

class filter_out_pin : public IPin, IMemInputPin
{
    friend filter_out_filter;

    CRITICAL_SECTION cs_pin;
    filter_out_filter *pfilter;
    AM_MEDIA_TYPE amt;
    IPin *pconnected;
    IMediaSeeking *pconnected_media_seeking;
    bool flushing;
    HANDLE event_flushing;
    CRITICAL_SECTION cs_receive;

    filter_out_pin();
    ~filter_out_pin();
public:
    // IUnknown
    virtual HRESULT __stdcall QueryInterface(REFIID riid, void **ppvObject);
    virtual ULONG __stdcall AddRef();
    virtual ULONG __stdcall Release();
    // IPin
    virtual HRESULT __stdcall Connect(IPin *pReceivePin, const AM_MEDIA_TYPE *pmt);
    virtual HRESULT __stdcall ReceiveConnection(IPin *pConnector, const AM_MEDIA_TYPE *pmt);
    virtual HRESULT __stdcall Disconnect();
    virtual HRESULT __stdcall ConnectedTo(IPin **pPin);
    virtual HRESULT __stdcall ConnectionMediaType(AM_MEDIA_TYPE *pmt);
    virtual HRESULT __stdcall QueryPinInfo(PIN_INFO *pInfo);
    virtual HRESULT __stdcall QueryDirection(PIN_DIRECTION *pPinDir);
    virtual HRESULT __stdcall QueryId(LPWSTR *Id);
    virtual HRESULT __stdcall QueryAccept(const AM_MEDIA_TYPE *pmt);
    virtual HRESULT __stdcall EnumMediaTypes(IEnumMediaTypes **ppEnum);
    virtual HRESULT __stdcall QueryInternalConnections(IPin **apPin, ULONG *nPin);
    virtual HRESULT __stdcall EndOfStream();
    virtual HRESULT __stdcall BeginFlush();
    virtual HRESULT __stdcall EndFlush();
    virtual HRESULT __stdcall NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
    // IMemInputPin
    virtual HRESULT __stdcall GetAllocator(IMemAllocator **ppAllocator);
    virtual HRESULT __stdcall NotifyAllocator(IMemAllocator *pAllocator, BOOL bReadOnly);
    virtual HRESULT __stdcall GetAllocatorRequirements(ALLOCATOR_PROPERTIES *pProps);
    virtual HRESULT __stdcall Receive(IMediaSample *pSample);
    virtual HRESULT __stdcall ReceiveMultiple(IMediaSample **pSamples, long nSamples, long *nSamplesProcessed);
    virtual HRESULT __stdcall ReceiveCanBlock();
};

class filter_out_enum_pins : public IEnumPins
{
    friend filter_out_filter;

    nat32 reference_count;
    filter_out_pin *ppin;
    nat32 index;

    filter_out_enum_pins();
    ~filter_out_enum_pins();
public:
    // IUnknown
    virtual HRESULT __stdcall QueryInterface(REFIID riid, void **ppvObject);
    virtual ULONG __stdcall AddRef();
    virtual ULONG __stdcall Release();
    // IEnumPins
    virtual HRESULT __stdcall Next(ULONG cPins, IPin **ppPins, ULONG *pcFetched);
    virtual HRESULT __stdcall Skip(ULONG cPins);
    virtual HRESULT __stdcall Reset();
    virtual HRESULT __stdcall Clone(IEnumPins **ppEnum);
};

class filter_out_filter : public filter_out, IMediaSeeking
{
    friend filter_out_pin;

    CRITICAL_SECTION cs_filter;
    nat32 reference_count;
    player_callback *pcallback;
    filter_out_pin *ppin;
    IFilterGraph *pgraph;
    IReferenceClock *pclock;
    char16 name[MAX_FILTER_NAME];
    FILTER_STATE state;
    FILTER_STATE state2;
    int64 t_start;
    HANDLE event_state_set;
    HANDLE event_not_paused;

    filter_out_filter();
    ~filter_out_filter();
    inline static nat32 round(nat32 n);
public:
    // IUnknown
    virtual HRESULT __stdcall QueryInterface(REFIID riid, void **ppvObject);
    virtual ULONG __stdcall AddRef();
    virtual ULONG __stdcall Release();
    // IPersist
    virtual HRESULT __stdcall GetClassID(CLSID *pClassID);
    // IMediaFilter
    virtual HRESULT __stdcall Stop();
    virtual HRESULT __stdcall Pause();
    virtual HRESULT __stdcall Run(REFERENCE_TIME tStart);
    virtual HRESULT __stdcall GetState(DWORD dwMilliSecsTimeout, FILTER_STATE *State);
    virtual HRESULT __stdcall SetSyncSource(IReferenceClock *pClock);
    virtual HRESULT __stdcall GetSyncSource(IReferenceClock **pClock);
    // IBaseFilter
    virtual HRESULT __stdcall EnumPins(IEnumPins **ppEnum);
    virtual HRESULT __stdcall FindPin(LPCWSTR Id, IPin **ppPin);
    virtual HRESULT __stdcall QueryFilterInfo(FILTER_INFO *pInfo);
    virtual HRESULT __stdcall JoinFilterGraph(IFilterGraph *pGraph, LPCWSTR pName);
    virtual HRESULT __stdcall QueryVendorInfo(LPWSTR *pVendorInfo);
    // IMediaSeeking
    virtual HRESULT __stdcall GetCapabilities(DWORD *pCapabilities);
    virtual HRESULT __stdcall CheckCapabilities(DWORD *pCapabilities);
    virtual HRESULT __stdcall IsFormatSupported(const GUID *pFormat);
    virtual HRESULT __stdcall QueryPreferredFormat(GUID *pFormat);
    virtual HRESULT __stdcall GetTimeFormat(GUID *pFormat);
    virtual HRESULT __stdcall IsUsingTimeFormat(const GUID *pFormat);
    virtual HRESULT __stdcall SetTimeFormat(const GUID *pFormat);
    virtual HRESULT __stdcall GetDuration(LONGLONG *pDuration);
    virtual HRESULT __stdcall GetStopPosition(LONGLONG *pStop);
    virtual HRESULT __stdcall GetCurrentPosition(LONGLONG *pCurrent);
    virtual HRESULT __stdcall ConvertTimeFormat(LONGLONG *pTarget, const GUID *pTargetFormat, LONGLONG Source, const GUID *pSourceFormat);
    virtual HRESULT __stdcall SetPositions(LONGLONG *pCurrent, DWORD dwCurrentFlags, LONGLONG *pStop, DWORD dwStopFlags);
    virtual HRESULT __stdcall GetPositions(LONGLONG *pCurrent, LONGLONG *pStop);
    virtual HRESULT __stdcall GetAvailable(LONGLONG *pEarliest, LONGLONG *pLatest);
    virtual HRESULT __stdcall SetRate(double dRate);
    virtual HRESULT __stdcall GetRate(double *pdRate);
    virtual HRESULT __stdcall GetPreroll(LONGLONG *pllPreroll);

    static bool create(const AM_MEDIA_TYPE *pamt, player_callback *pcallback, filter_out_filter **ppfilter);
};

//----------------------------------------------------------------------------
// filter_out_enum_media_types
//----------------------------------------------------------------------------

filter_out_enum_media_types::filter_out_enum_media_types()
{
#if write_level > 1
    print("filter_out_enum_media_types::filter_out_enum_media_types called...\n");
#endif
}

filter_out_enum_media_types::~filter_out_enum_media_types()
{
#if write_level > 1
    print("filter_out_enum_media_types::~filter_out_enum_media_types called...\n");
#endif
}

HRESULT __stdcall filter_out_enum_media_types::QueryInterface(REFIID riid, void **ppvObject)
{
#if write_level > 0
    print("filter_out_enum_media_types::QueryInterface called...\n");
#endif
    if(!ppvObject) return E_POINTER;
    if(riid == IID_IUnknown)
    {
        *(IUnknown **)ppvObject = this;
        ((IUnknown *)*ppvObject)->AddRef();
        return S_OK;
    }
    if(riid == IID_IEnumMediaTypes)
    {
        *(IEnumMediaTypes **)ppvObject = this;
        ((IEnumMediaTypes *)*ppvObject)->AddRef();
        return S_OK;
    }
    *ppvObject = null;
    return E_NOINTERFACE;
}

ULONG __stdcall filter_out_enum_media_types::AddRef()
{
#if write_level > 1
    print("filter_out_enum_media_types::AddRef called...\n");
#endif
    return ++reference_count;
}

ULONG __stdcall filter_out_enum_media_types::Release()
{
#if write_level > 1
    print("filter_out_enum_media_types::Release called...\n");
#endif
    if(reference_count == 1)
    {
        delete this;
        return 0;
    }
    return --reference_count;
}

HRESULT __stdcall filter_out_enum_media_types::Next(ULONG cMediaTypes, AM_MEDIA_TYPE **ppMediaTypes, ULONG *pcFetched)
{
#if write_level > 0
    print("filter_out_enum_media_types::Next called...\n");
#endif
    if(!ppMediaTypes) return E_POINTER;
    if(cMediaTypes != 1 && !pcFetched) return E_INVALIDARG;
    nat32 i;
    for(i = 0; i < cMediaTypes; i++)
    {
        if(index >= 1)
        {
            if(pcFetched) *pcFetched = i;
            return S_FALSE;
        }
        AM_MEDIA_TYPE *pamt = (AM_MEDIA_TYPE *)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
        if(!pamt)
        {
            if(pcFetched) *pcFetched = i;
            return E_OUTOFMEMORY;
        }
        if(amt.cbFormat)
        {
            pamt->pbFormat = (bits8 *)CoTaskMemAlloc(amt.cbFormat);
            if(!pamt->pbFormat)
            {
                CoTaskMemFree(pamt);
                if(pcFetched) *pcFetched = i;
                return E_OUTOFMEMORY;
            }
            memcpy(pamt->pbFormat, amt.pbFormat, amt.cbFormat);
        }
        else pamt->pbFormat = null;
        pamt->majortype = amt.majortype;
        pamt->subtype = amt.subtype;
        pamt->bFixedSizeSamples = amt.bFixedSizeSamples;
        pamt->bTemporalCompression = amt.bTemporalCompression;
        pamt->lSampleSize = amt.lSampleSize;
        pamt->formattype = amt.formattype;
        pamt->pUnk = amt.pUnk;
        if(pamt->pUnk) pamt->pUnk->AddRef();
        pamt->cbFormat = amt.cbFormat;
        ppMediaTypes[i] = pamt;
        index++;
    }
    if(pcFetched) *pcFetched = i;
    return S_OK;
}

HRESULT __stdcall filter_out_enum_media_types::Skip(ULONG cMediaTypes)
{
#if write_level > 0
    print("filter_out_enum_media_types::Skip called...\n");
#endif
    if(index + cMediaTypes > 1)
    {
        if(index != 1) index = 1;
        return S_FALSE;
    }
    index += cMediaTypes;
    return S_OK;
}

HRESULT __stdcall filter_out_enum_media_types::Reset()
{
#if write_level > 0
    print("filter_out_enum_media_types::Reset called...\n");
#endif
    index = 0;
    return S_OK;
}

HRESULT __stdcall filter_out_enum_media_types::Clone(IEnumMediaTypes **ppEnum)
{
#if write_level > 0
    print("filter_out_enum_media_types::Clone called...\n");
#endif
    if(!ppEnum) return E_POINTER;
    filter_out_enum_media_types *penum_media_types = new filter_out_enum_media_types;
    if(!penum_media_types) return E_OUTOFMEMORY;
    if(amt.cbFormat)
    {
        penum_media_types->amt.pbFormat = new bits8[amt.cbFormat];
        if(!penum_media_types->amt.pbFormat)
        {
            delete penum_media_types;
            return E_OUTOFMEMORY;
        }
        memcpy(penum_media_types->amt.pbFormat, amt.pbFormat, amt.cbFormat);
    }
    else penum_media_types->amt.pbFormat = null;
    penum_media_types->reference_count = 1;
    penum_media_types->amt.majortype = amt.majortype;
    penum_media_types->amt.subtype = amt.subtype;
    penum_media_types->amt.bFixedSizeSamples = amt.bFixedSizeSamples;
    penum_media_types->amt.bTemporalCompression = amt.bTemporalCompression;
    penum_media_types->amt.lSampleSize = amt.lSampleSize;
    penum_media_types->amt.formattype = amt.formattype;
    penum_media_types->amt.pUnk = amt.pUnk;
    if(penum_media_types->amt.pUnk) penum_media_types->amt.pUnk->AddRef();
    penum_media_types->amt.cbFormat = amt.cbFormat;
    penum_media_types->index = index;
    *ppEnum = penum_media_types;
    return S_OK;
}

//----------------------------------------------------------------------------
// filter_out_pin
//----------------------------------------------------------------------------

filter_out_pin::filter_out_pin()
{
#if write_level > 1
    print("filter_out_pin::filter_out_pin called...\n");
#endif
}

filter_out_pin::~filter_out_pin()
{
#if write_level > 1
    print("filter_out_pin::~filter_out_pin called...\n");
#endif
}

HRESULT __stdcall filter_out_pin::QueryInterface(REFIID riid, void **ppvObject)
{
#if write_level > 0
    print("filter_out_pin::QueryInterface(");
    print(riid);
    print(", 0x%p) called...\n", ppvObject);
#endif
    if(!ppvObject) return E_POINTER;
    if(riid == IID_IUnknown)
    {
        *(IUnknown **)ppvObject = (IPin *) this;
        ((IUnknown *)*ppvObject)->AddRef();
        return S_OK;
    }
    if(riid == IID_IPin)
    {
        *(IPin **)ppvObject = this;
        ((IPin *)*ppvObject)->AddRef();
        return S_OK;
    }
    if(riid == IID_IMemInputPin)
    {
        *(IMemInputPin **)ppvObject = this;
        ((IMemInputPin *)*ppvObject)->AddRef();
        return S_OK;
    }
    *ppvObject = null;
    return E_NOINTERFACE;
}

ULONG __stdcall filter_out_pin::AddRef()
{
#if write_level > 1
    print("filter_out_pin::AddRef called...\n");
#endif
    EnterCriticalSection(&cs_pin);
    nat32 r = pfilter->AddRef();
    LeaveCriticalSection(&cs_pin);
    return r;
}

ULONG __stdcall filter_out_pin::Release()
{
#if write_level > 1
    print("filter_out_pin::Release called...\n");
#endif
    EnterCriticalSection(&cs_pin);
    nat32 r = pfilter->Release();
    if(r) LeaveCriticalSection(&cs_pin);
    return r;
}

HRESULT __stdcall filter_out_pin::Connect(IPin *pReceivePin, const AM_MEDIA_TYPE * /*pmt*/)
{
#if write_level > 0
    print("filter_out_pin::Connect called...\n");
#endif
    if(!pReceivePin) return E_POINTER;
    return E_UNEXPECTED;
}

HRESULT __stdcall filter_out_pin::ReceiveConnection(IPin *pConnector, const AM_MEDIA_TYPE *pmt)
{
#if write_level > 0
    print("filter_out_pin::ReceiveConnection(0x%p", pConnector);
    print(", ");
    dump_media_type(pmt);
    print(") called...\n");
#endif
    if(!pConnector || !pmt) return E_POINTER;
    EnterCriticalSection(&cs_pin);
    if(pfilter->state != State_Stopped)
    {
        LeaveCriticalSection(&cs_pin);
        return VFW_E_NOT_STOPPED;
    }
    if(pconnected)
    {
        LeaveCriticalSection(&cs_pin);
        return VFW_E_ALREADY_CONNECTED;
    }
    if(pmt->majortype != amt.majortype ||
        pmt->subtype != amt.subtype)
    {
        LeaveCriticalSection(&cs_pin);
        return VFW_E_TYPE_NOT_ACCEPTED;
    }

    if(pmt->majortype == MEDIATYPE_Audio)
    {
        if(pmt->formattype == FORMAT_WaveFormatEx && pmt->cbFormat >= sizeof(WAVEFORMATEX))
        {
            const WAVEFORMATEX *pwfe = (const WAVEFORMATEX *)pmt->pbFormat;
            if(pwfe->wFormatTag == WAVE_FORMAT_PCM)
            {
#if write_level > 0
                print("Audio format - %u %u %u\n", pwfe->nSamplesPerSec, pwfe->nChannels, pwfe->wBitsPerSample);
#endif
                pfilter->pcallback->audio_format_changed(pwfe->nSamplesPerSec, pwfe->nChannels, pwfe->wBitsPerSample);
            }
            else
            {
                LeaveCriticalSection(&cs_pin);
                return VFW_E_TYPE_NOT_ACCEPTED;
            }
        }
        else
        {
            LeaveCriticalSection(&cs_pin);
            return VFW_E_TYPE_NOT_ACCEPTED;
        }
    }
    else if(pmt->majortype == MEDIATYPE_Video && pmt->cbFormat >= sizeof(VIDEOINFOHEADER))
    {
        if(pmt->formattype == FORMAT_VideoInfo)
        {
            const VIDEOINFOHEADER *vih = (const VIDEOINFOHEADER *)pmt->pbFormat;
#if write_level > 0
            print("Frame size - %i %i\n", vih->bmiHeader.biWidth, vih->bmiHeader.biHeight);
#endif
            pfilter->pcallback->size_changed(int16(vih->bmiHeader.biWidth), int16(vih->bmiHeader.biHeight));
        }
        else if(pmt->formattype == FORMAT_VideoInfo2 && pmt->cbFormat >= sizeof(VIDEOINFOHEADER2))
        {
            const VIDEOINFOHEADER2 *vih2 = (const VIDEOINFOHEADER2 *)pmt->pbFormat;
#if write_level > 0
            print("Frame size - %i %i\n", vih2->bmiHeader.biWidth, vih2->bmiHeader.biHeight);
#endif
            pfilter->pcallback->size_changed(int16(vih2->bmiHeader.biWidth), int16(vih2->bmiHeader.biHeight));
        }
        else
        {
            LeaveCriticalSection(&cs_pin);
            return VFW_E_TYPE_NOT_ACCEPTED;
        }
    }

    pconnected = pConnector;
    pconnected->AddRef();
    if(pconnected->QueryInterface(IID_IMediaSeeking, (void **)&pconnected_media_seeking) != S_OK) pconnected_media_seeking = null;
    LeaveCriticalSection(&cs_pin);
#if write_level > 0
        print("Connected.\n");
#endif
    return S_OK;
}

HRESULT __stdcall filter_out_pin::Disconnect()
{
#if write_level > 0
    print("filter_out_pin::Disconnect called...\n");
#endif
    EnterCriticalSection(&cs_pin);
    if(pfilter->state != State_Stopped)
    {
        LeaveCriticalSection(&cs_pin);
        return VFW_E_NOT_STOPPED;
    }
    if(!pconnected)
    {
        LeaveCriticalSection(&cs_pin);
        return S_FALSE;
    }
    if(pconnected_media_seeking) pconnected_media_seeking->Release();
    pconnected->Release();
    pconnected = null;
    LeaveCriticalSection(&cs_pin);
    return S_OK;
}

HRESULT __stdcall filter_out_pin::ConnectedTo(IPin **pPin)
{
#if write_level > 0
    print("filter_out_pin::ConnectedTo called...\n");
#endif
    if(!pPin) return E_POINTER;
    EnterCriticalSection(&cs_pin);
    if(!pconnected)
    {
        LeaveCriticalSection(&cs_pin);
        *pPin = null;
        return VFW_E_NOT_CONNECTED;
    }
    *pPin = pconnected;
    (*pPin)->AddRef();
    LeaveCriticalSection(&cs_pin);
    return S_OK;
}

HRESULT __stdcall filter_out_pin::ConnectionMediaType(AM_MEDIA_TYPE *pmt)
{
#if write_level > 0
    print("filter_out_pin::ConnectionMediaType called...\n");
#endif
    if(!pmt) return E_POINTER;
    EnterCriticalSection(&cs_pin);
    if(!pconnected)
    {
        LeaveCriticalSection(&cs_pin);
        memset(pmt, 0, sizeof(AM_MEDIA_TYPE));
        return VFW_E_NOT_CONNECTED;
    }
    if(amt.cbFormat)
    {
        pmt->pbFormat = (bits8 *)CoTaskMemAlloc(amt.cbFormat);
        if(!pmt->pbFormat)
        {
            LeaveCriticalSection(&cs_pin);
            return E_OUTOFMEMORY;
        }
        memcpy(pmt->pbFormat, amt.pbFormat, amt.cbFormat);
    }
    else pmt->pbFormat = null;
    pmt->majortype = amt.majortype;
    pmt->subtype = amt.subtype;
    pmt->bFixedSizeSamples = amt.bFixedSizeSamples;
    pmt->bTemporalCompression = amt.bTemporalCompression;
    pmt->lSampleSize = amt.lSampleSize;
    pmt->formattype = amt.formattype;
    pmt->pUnk = amt.pUnk;
    if(pmt->pUnk) pmt->pUnk->AddRef();
    pmt->cbFormat = amt.cbFormat;
    LeaveCriticalSection(&cs_pin);
    return S_OK;
}

HRESULT __stdcall filter_out_pin::QueryPinInfo(PIN_INFO *pInfo)
{
#if write_level > 0
    print("filter_out_pin::QueryPinInfo called...\n");
#endif
    if(!pInfo) return E_POINTER;
    EnterCriticalSection(&cs_pin);
    pInfo->pFilter = pfilter;
    pInfo->pFilter->AddRef();
    pInfo->dir = PINDIR_INPUT;
    wcscpy_s(pInfo->achName, MAX_PIN_NAME, L"Input");
    LeaveCriticalSection(&cs_pin);
    return S_OK;
}

HRESULT __stdcall filter_out_pin::QueryDirection(PIN_DIRECTION *pPinDir)
{
#if write_level > 0
    print("filter_out_pin::QueryDirection called...\n");
#endif
    if(!pPinDir) return E_POINTER;
    *pPinDir = PINDIR_INPUT;
    return S_OK;
}

HRESULT __stdcall filter_out_pin::QueryId(LPWSTR *Id)
{
#if write_level > 0
    print("filter_out_pin::QueryId called...\n");
#endif
    if(!Id) return E_POINTER;
    *Id = (WCHAR *)CoTaskMemAlloc(sizeof(WCHAR) * 7);
    if(!*Id) return E_OUTOFMEMORY;
    memcpy(*Id, L"Output", sizeof(WCHAR) * 7);
    return S_OK;
}

HRESULT __stdcall filter_out_pin::QueryAccept(const AM_MEDIA_TYPE * /*pmt*/)
{
#if write_level > 0
    print("filter_out_pin::QueryAccept called...\n");
#endif
    return S_OK;
}

HRESULT __stdcall filter_out_pin::EnumMediaTypes(IEnumMediaTypes **ppEnum)
{
#if write_level > 0
    print("filter_out_pin::EnumMediaTypes called...\n");
#endif
    if(!ppEnum) return E_POINTER;
    EnterCriticalSection(&cs_pin);
    filter_out_enum_media_types *penum_media_types = new filter_out_enum_media_types;
    if(!penum_media_types)
    {
        LeaveCriticalSection(&cs_pin);
        return E_OUTOFMEMORY;
    }
    if(amt.cbFormat)
    {
        penum_media_types->amt.pbFormat = new bits8[amt.cbFormat];
        if(!penum_media_types->amt.pbFormat)
        {
            LeaveCriticalSection(&cs_pin);
            delete penum_media_types;
            return E_OUTOFMEMORY;
        }
        memcpy(penum_media_types->amt.pbFormat, amt.pbFormat, amt.cbFormat);
    }
    else penum_media_types->amt.pbFormat = null;
    penum_media_types->reference_count = 1;
    penum_media_types->amt.majortype = amt.majortype;
    penum_media_types->amt.subtype = amt.subtype;
    penum_media_types->amt.bFixedSizeSamples = amt.bFixedSizeSamples;
    penum_media_types->amt.bTemporalCompression = amt.bTemporalCompression;
    penum_media_types->amt.lSampleSize = amt.lSampleSize;
    penum_media_types->amt.formattype = amt.formattype;
    penum_media_types->amt.pUnk = amt.pUnk;
    if(penum_media_types->amt.pUnk) penum_media_types->amt.pUnk->AddRef();
    penum_media_types->amt.cbFormat = amt.cbFormat;
    penum_media_types->index = 0;
    *ppEnum = penum_media_types;
    LeaveCriticalSection(&cs_pin);
    return S_OK;
}

HRESULT __stdcall filter_out_pin::QueryInternalConnections(IPin **apPin, ULONG *nPin)
{
#if write_level > 0
    print("filter_out_pin::QueryInternalConnections called...\n");
#endif
    if(!apPin || !nPin) return E_POINTER;
    return E_NOTIMPL;
}

HRESULT __stdcall filter_out_pin::EndOfStream()
{
#if write_level > 0
    print("filter_out_pin::EndOfStream called...\n");
#endif
    EnterCriticalSection(&cs_pin);
    EnterCriticalSection(&pfilter->cs_filter);
    pfilter->pcallback->playback_finished();
    LeaveCriticalSection(&pfilter->cs_filter);
    LeaveCriticalSection(&cs_pin);
    return S_OK;
}

HRESULT __stdcall filter_out_pin::BeginFlush()
{
#if write_level > 0
    print("filter_out_pin::BeginFlush called...\n");
#endif
    EnterCriticalSection(&cs_pin);
    if(flushing)
    {
        LeaveCriticalSection(&cs_pin);
    }
    else
    {
        flushing = true;
        SetEvent(event_flushing);
        LeaveCriticalSection(&cs_pin);
        EnterCriticalSection(&cs_receive);
        LeaveCriticalSection(&cs_receive);
    }
    return S_OK;
}

HRESULT __stdcall filter_out_pin::EndFlush()
{
#if write_level > 0
    print("filter_out_pin::EndFlush called...\n");
#endif
    EnterCriticalSection(&cs_pin);
    flushing = false;
    LeaveCriticalSection(&cs_pin);
    return S_OK;
}

HRESULT __stdcall filter_out_pin::NewSegment(REFERENCE_TIME /*tStart*/, REFERENCE_TIME /*tStop*/, double /*dRate*/)
{
#if write_level > 0
    print("filter_out_pin::NewSegment called...\n");
#endif
    return E_UNEXPECTED;
}

HRESULT __stdcall filter_out_pin::GetAllocator(IMemAllocator **ppAllocator)
{
#if write_level > 0
    print("filter_out_pin::GetAllocator called...\n");
#endif
    if(!ppAllocator) return E_POINTER;
    return VFW_E_NO_ALLOCATOR;
}

HRESULT __stdcall filter_out_pin::NotifyAllocator(IMemAllocator *pAllocator, BOOL /*bReadOnly*/)
{
#if write_level > 0
    print("filter_out_pin::NotifyAllocator called...\n");
#endif
    if(!pAllocator) return E_POINTER;
    return S_OK;
}

HRESULT __stdcall filter_out_pin::GetAllocatorRequirements(ALLOCATOR_PROPERTIES *pProps)
{
#if write_level > 0
    print("filter_out_pin::GetAllocatorRequirements called...\n");
#endif
    if(!pProps) return E_POINTER;
    return E_NOTIMPL;
}

HRESULT __stdcall filter_out_pin::Receive(IMediaSample *pSample)
{
#if write_level > 0
    print("filter_out_pin::Receive called...\n");
#endif
    if(!pSample) return E_POINTER;

#if write_level > 0
    print("Actual data length=%i\n", pSample->GetActualDataLength());
#endif

    int64 tstart;
    int64 tend;
    if(FAILED(pSample->GetTime(&tstart, &tend)))
    {
#if write_level > 0
        print("returns VFW_E_RUNTIME_ERROR\n");
#endif
        return VFW_E_RUNTIME_ERROR;
    }

    EnterCriticalSection(&cs_receive);
    EnterCriticalSection(&cs_pin);

    for(;;)
    {
        if(flushing)
        {
            LeaveCriticalSection(&cs_pin);
            LeaveCriticalSection(&cs_receive);

#if write_level > 0
            print("returns S_FALSE\n");
#endif
            return S_FALSE;
        }

        EnterCriticalSection(&pfilter->cs_filter);

        if(pfilter->state == State_Stopped)
        {
            if(pfilter->state2 == State_Stopped)
            {
                LeaveCriticalSection(&pfilter->cs_filter);
                LeaveCriticalSection(&cs_pin);
                LeaveCriticalSection(&cs_receive);

#if write_level > 0
                print("returns VFW_E_WRONG_STATE\n");
#endif
                return VFW_E_WRONG_STATE;
            }
            else
            {
                bits8 *pb;
                if(pSample->GetPointer(&pb) != S_OK)
                {
                    LeaveCriticalSection(&pfilter->cs_filter);
                    LeaveCriticalSection(&cs_pin);
                    LeaveCriticalSection(&cs_receive);

#if write_level > 0
                    print("returns VFW_E_RUNTIME_ERROR\n");
#endif
                    return VFW_E_RUNTIME_ERROR;
                }
                if(amt.majortype == MEDIATYPE_Audio)
                {
                    pfilter->pcallback->sample_ready(pSample->GetActualDataLength(), (bits16 *)pb);
                }
                else
                {
                    pfilter->pcallback->frame_ready((bits16 *)pb);
                }

                pfilter->state = State_Paused;
                SetEvent(pfilter->event_state_set);
                ResetEvent(pfilter->event_not_paused);

                LeaveCriticalSection(&pfilter->cs_filter);
                LeaveCriticalSection(&cs_pin);
                LeaveCriticalSection(&cs_receive);

#if write_level > 0
                print("returns S_OK\n");
#endif
                return S_OK;
            }
        }
        else if(pfilter->state == State_Paused)
        {
            HANDLE events[2] = { pfilter->event_not_paused, event_flushing };

            LeaveCriticalSection(&pfilter->cs_filter);
            LeaveCriticalSection(&cs_pin);

            WaitForMultipleObjects(2, events, false, INFINITE);

            EnterCriticalSection(&cs_pin);
        }
        else if(pfilter->state == State_Running)
        {
            int64 t;
            if(FAILED(pfilter->pclock->GetTime(&t)))
            {
                LeaveCriticalSection(&pfilter->cs_filter);
                LeaveCriticalSection(&cs_pin);
                LeaveCriticalSection(&cs_receive);

#if write_level > 0
                print("returns VFW_E_RUNTIME_ERROR\n");
#endif
                return VFW_E_RUNTIME_ERROR;
            }

            if(t - pfilter->t_start < tstart)
            {
                LeaveCriticalSection(&pfilter->cs_filter);
                LeaveCriticalSection(&cs_pin);

                Sleep(1);

                EnterCriticalSection(&cs_pin);
            }
            else
            {
                bits8 *pb;
                if(pSample->GetPointer(&pb) != S_OK)
                {
                    LeaveCriticalSection(&pfilter->cs_filter);
                    LeaveCriticalSection(&cs_pin);
                    LeaveCriticalSection(&cs_receive);

#if write_level > 0
                    print("returns VFW_E_RUNTIME_ERROR\n");
#endif
                    return VFW_E_RUNTIME_ERROR;
                }
                if(amt.majortype == MEDIATYPE_Audio)
                {
                    pfilter->pcallback->sample_ready(pSample->GetActualDataLength(), (bits16 *)pb);
                }
                else
                {
                    pfilter->pcallback->frame_ready((bits16 *)pb);
                }

                LeaveCriticalSection(&pfilter->cs_filter);
                LeaveCriticalSection(&cs_pin);
                LeaveCriticalSection(&cs_receive);

#if write_level > 0
                print("returns S_OK\n");
#endif
                return S_OK;
            }
        }
    }
}

HRESULT __stdcall filter_out_pin::ReceiveMultiple(IMediaSample **pSamples, long /*nSamples*/, long *nSamplesProcessed)
{
#if write_level > 0
    print("filter_out_pin::ReceiveMultiple called...\n");
#endif
    if(!pSamples || !nSamplesProcessed) return E_POINTER;
    EnterCriticalSection(&cs_pin);
    EnterCriticalSection(&pfilter->cs_filter);
    if(pfilter->state == State_Stopped)
    {
        LeaveCriticalSection(&pfilter->cs_filter);
        LeaveCriticalSection(&cs_pin);
        return VFW_E_WRONG_STATE;
    }
    LeaveCriticalSection(&pfilter->cs_filter);
    LeaveCriticalSection(&cs_pin);
    return VFW_E_RUNTIME_ERROR;
}

HRESULT __stdcall filter_out_pin::ReceiveCanBlock()
{
#if write_level > 0
    print("filter_out_pin::ReceiveCanBlock called...\n");
#endif
    return S_OK;
}

//----------------------------------------------------------------------------
// filter_out_enum_pins
//----------------------------------------------------------------------------

filter_out_enum_pins::filter_out_enum_pins()
{
#if write_level > 1
    print("filter_out_enum_pins::filter_out_enum_pins called...\n");
#endif
}

filter_out_enum_pins::~filter_out_enum_pins()
{
#if write_level > 1
    print("filter_out_enum_pins::~filter_out_enum_pins called...\n");
#endif
}

HRESULT __stdcall filter_out_enum_pins::QueryInterface(REFIID riid, void **ppvObject)
{
#if write_level > 0
    print("filter_out_enum_pins::QueryInterface called...\n");
#endif
    if(!ppvObject) return E_POINTER;
    if(riid == IID_IUnknown)
    {
        *(IUnknown **)ppvObject = this;
        ((IUnknown *)*ppvObject)->AddRef();
        return S_OK;
    }
    if(riid == IID_IEnumPins)
    {
        *(IEnumPins **)ppvObject = this;
        ((IEnumPins *)*ppvObject)->AddRef();
        return S_OK;
    }
    *ppvObject = null;
    return E_NOINTERFACE;
}

ULONG __stdcall filter_out_enum_pins::AddRef()
{
#if write_level > 1
    print("filter_out_enum_pins::AddRef called...\n");
#endif
    return ++reference_count;
}

ULONG __stdcall filter_out_enum_pins::Release()
{
#if write_level > 1
    print("filter_out_enum_pins::Release called...\n");
#endif
    if(reference_count == 1)
    {
        ppin->Release();
        delete this;
        return 0;
    }
    return --reference_count;
}

HRESULT __stdcall filter_out_enum_pins::Next(ULONG cPins, IPin **ppPins, ULONG *pcFetched)
{
#if write_level > 0
    print("filter_out_enum_pins::Next called...\n");
#endif
    if(!ppPins) return E_POINTER;
    if(cPins != 1 && !pcFetched) return E_INVALIDARG;
    nat32 i;
    for(i = 0; i < cPins; i++)
    {
        if(index >= 1)
        {
            if(pcFetched) *pcFetched = i;
            return S_FALSE;
        }
        ppPins[i] = ppin;
        ppPins[i]->AddRef();
        index++;
    }
    if(pcFetched) *pcFetched = i;
    return S_OK;
}

HRESULT __stdcall filter_out_enum_pins::Skip(ULONG cPins)
{
#if write_level > 0
    print("filter_out_enum_pins::Skip called...\n");
#endif
    if(index + cPins > 1)
    {
        if(index != 1) index = 1;
        return S_FALSE;
    }
    index += cPins;
    return S_OK;
}

HRESULT __stdcall filter_out_enum_pins::Reset()
{
#if write_level > 0
    print("filter_out_enum_pins::Reset called...\n");
#endif
    index = 0;
    return S_OK;
}

HRESULT __stdcall filter_out_enum_pins::Clone(IEnumPins **ppEnum)
{
#if write_level > 0
    print("filter_out_enum_pins::Clone called...\n");
#endif
    if(!ppEnum) return E_POINTER;
    filter_out_enum_pins *penum_pins = new filter_out_enum_pins;
    if(!penum_pins) return E_OUTOFMEMORY;
    penum_pins->reference_count = 1;
    penum_pins->ppin = ppin;
    penum_pins->ppin->AddRef();
    penum_pins->index = index;
    *ppEnum = penum_pins;
    return S_OK;
}

//----------------------------------------------------------------------------
// filter_out_filter
//----------------------------------------------------------------------------

filter_out_filter::filter_out_filter()
{
#if write_level > 1
    print("filter_out_filter::filter_out_filter called...\n");
#endif
}

filter_out_filter::~filter_out_filter()
{
#if write_level > 1
    print("filter_out_filter::~filter_out_filter called...\n");
#endif
}

HRESULT __stdcall filter_out_filter::QueryInterface(REFIID riid, void **ppvObject)
{
#if write_level > 0
    print("filter_out_filter::QueryInterface(");
    print(riid);
    print(", 0x%p) called...\n", ppvObject);
#endif
    if(!ppvObject) return E_POINTER;
    if(riid == IID_IUnknown)
    {
        *(IUnknown **)ppvObject = (IBaseFilter *)this;
        ((IUnknown *)*ppvObject)->AddRef();
        return S_OK;
    }
    if(riid == IID_IPersist)
    {
        *(IPersist **)ppvObject = this;
        ((IPersist *)*ppvObject)->AddRef();
        return S_OK;
    }
    if(riid == IID_IMediaFilter)
    {
        *(IMediaFilter **)ppvObject = this;
        ((IMediaFilter *)*ppvObject)->AddRef();
        return S_OK;
    }
    if(riid == IID_IBaseFilter)
    {
        *(IBaseFilter **)ppvObject = this;
        ((IBaseFilter *)*ppvObject)->AddRef();
        return S_OK;
    }
    if(riid == IID_IMediaSeeking)
    {
        *(IMediaSeeking **)ppvObject = this;
        ((IMediaSeeking *)*ppvObject)->AddRef();
        return S_OK;
    }
    *ppvObject = null;
    return E_NOINTERFACE;
}

ULONG __stdcall filter_out_filter::AddRef()
{
#if write_level > 1
    print("filter_out_filter::AddRef called...\n");
#endif
    EnterCriticalSection(&cs_filter);
    nat32 r = ++reference_count;
    LeaveCriticalSection(&cs_filter);
    return r;
}

ULONG __stdcall filter_out_filter::Release()
{
#if write_level > 1
    print("filter_out_filter::Release called...\n");
#endif
    EnterCriticalSection(&cs_filter);
    if(reference_count == 1)
    {
        CloseHandle(event_not_paused);
        CloseHandle(event_state_set);
        if(pclock) pclock->Release();

        DeleteCriticalSection(&ppin->cs_receive);
        CloseHandle(ppin->event_flushing);
        if(ppin->pconnected)
        {
            if(ppin->pconnected_media_seeking) ppin->pconnected_media_seeking->Release();
            ppin->pconnected->Release();
        }
        if(ppin->amt.pUnk) ppin->amt.pUnk->Release();
        if(ppin->amt.cbFormat) delete[] ppin->amt.pbFormat;
        DeleteCriticalSection(&ppin->cs_pin);

        DeleteCriticalSection(&cs_filter);
        delete ppin;
        delete this;
        return 0;
    }
    nat32 r = --reference_count;
    LeaveCriticalSection(&cs_filter);
    return r;
}

HRESULT __stdcall filter_out_filter::GetClassID(CLSID * /*pClassID*/)
{
#if write_level > 0
    print("filter_out_filter::GetClassID called...\n");
#endif
    return E_FAIL;
}

HRESULT __stdcall filter_out_filter::Stop()
{
#if write_level > 0
    print("filter_out_filter::Stop called...\n");
#endif
    EnterCriticalSection(&cs_filter);
    if(state == State_Stopped)
    {
        if(state2 == State_Stopped)
        {
        }
        else
        {
            state2 = State_Stopped;
            SetEvent(event_state_set);
        }
    }
    else if(state == State_Paused)
    {
        state = State_Stopped;
        state2 = State_Stopped;
        SetEvent(event_not_paused);
    }
    else
    {
        state = State_Stopped;
        state2 = State_Stopped;
    }
    LeaveCriticalSection(&cs_filter);
    return S_OK;
}

HRESULT __stdcall filter_out_filter::Pause()
{
#if write_level > 0
    print("filter_out_filter::Pause called...\n");
#endif
    HRESULT r;
    EnterCriticalSection(&cs_filter);
    if(state == State_Stopped)
    {
        if(state2 == State_Stopped)
        {
            state2 = State_Paused;
            ResetEvent(event_state_set);
        }
        else
        {
        }
        r = S_FALSE;
    }
    else if(state == State_Paused)
    {
        r = S_OK;
    }
    else
    {
        state = State_Paused;
        state2 = State_Paused;
        ResetEvent(event_not_paused);
        r = S_OK;
    }
    LeaveCriticalSection(&cs_filter);
    return r;
}

HRESULT __stdcall filter_out_filter::Run(REFERENCE_TIME tStart)
{
#if write_level > 0
    print("filter_out_filter::Run called...\n");
#endif
    EnterCriticalSection(&cs_filter);
    if(state == State_Stopped)
    {
        state = State_Running;
        state2 = State_Running;
        if(state2 == State_Stopped)
        {
        }
        else
        {
            SetEvent(event_state_set);
        }
    }
    else if(state == State_Paused)
    {
        state = State_Running;
        state2 = State_Running;
        SetEvent(event_not_paused);
    }
    else
    {
    }
    t_start = tStart;
    LeaveCriticalSection(&cs_filter);
    return S_OK;
}

HRESULT __stdcall filter_out_filter::GetState(DWORD dwMilliSecsTimeout, FILTER_STATE *State)
{
#if write_level > 0
    print("filter_out_filter::GetState called...\n");
#endif
    if(!State) return E_POINTER;
    EnterCriticalSection(&cs_filter);
    if(state2 != state)
    {
        nat32 r = WaitForSingleObject(event_state_set, dwMilliSecsTimeout);
        if(r == WAIT_OBJECT_0)
        {
        }
        else if(r == WAIT_TIMEOUT)
        {
            LeaveCriticalSection(&cs_filter);
            return VFW_S_STATE_INTERMEDIATE;
        }
        else
        {
            LeaveCriticalSection(&cs_filter);
            return VFW_E_RUNTIME_ERROR;
        }
    }
    *State = state;
    LeaveCriticalSection(&cs_filter);
    return S_OK;
}

HRESULT __stdcall filter_out_filter::SetSyncSource(IReferenceClock *pClock)
{
#if write_level > 0
    print("filter_out_filter::SetSyncSource(0x%p) called...\n", pClock);
#endif
    EnterCriticalSection(&cs_filter);
    if(pClock != pclock)
    {
        if(pclock) pclock->Release();
        pclock = pClock;
        if(pclock) pclock->AddRef();
    }
    LeaveCriticalSection(&cs_filter);
    return S_OK;
}

HRESULT __stdcall filter_out_filter::GetSyncSource(IReferenceClock **pClock)
{
#if write_level > 0
    print("filter_out_filter::GetSyncSource called...\n");
#endif
    if(!pClock) return E_POINTER;
    EnterCriticalSection(&cs_filter);
    *pClock = pclock;
    if(*pClock) (*pClock)->AddRef();
    LeaveCriticalSection(&cs_filter);
    return S_OK;
}

HRESULT __stdcall filter_out_filter::EnumPins(IEnumPins **ppEnum)
{
#if write_level > 0
    print("filter_out_filter::EnumPins called...\n");
#endif
    if(!ppEnum) return E_POINTER;
    EnterCriticalSection(&cs_filter);
    filter_out_enum_pins *penum_pins = new filter_out_enum_pins;
    if(!penum_pins)
    {
        LeaveCriticalSection(&cs_filter);
        return E_OUTOFMEMORY;
    }
    penum_pins->reference_count = 1;
    penum_pins->ppin = ppin;
    penum_pins->ppin->AddRef();
    penum_pins->index = 0;
    *ppEnum = penum_pins;
    LeaveCriticalSection(&cs_filter);
    return S_OK;
}

HRESULT __stdcall filter_out_filter::FindPin(LPCWSTR Id, IPin **ppPin)
{
#if write_level > 0
    print("filter_out_filter::FindPin called...\n");
#endif
    if(!ppPin) return E_POINTER;
    EnterCriticalSection(&cs_filter);
    if(!wcscmp(Id, L"Output"))
    {
        *ppPin = ppin;
        LeaveCriticalSection(&cs_filter);
        (*ppPin)->AddRef();
        return S_OK;
    }
    LeaveCriticalSection(&cs_filter);
    *ppPin = null;
    return VFW_E_NOT_FOUND;
}

HRESULT __stdcall filter_out_filter::QueryFilterInfo(FILTER_INFO *pInfo)
{
#if write_level > 0
    print("filter_out_filter::QueryFilterInfo called...\n");
#endif
    if(!pInfo) return E_POINTER;
    EnterCriticalSection(&cs_filter);
    wcscpy_s(pInfo->achName, MAX_FILTER_NAME, name);
    pInfo->pGraph = pgraph;
    LeaveCriticalSection(&cs_filter);
    if(pInfo->pGraph) pInfo->pGraph->AddRef();
    return S_OK;
}

HRESULT __stdcall filter_out_filter::JoinFilterGraph(IFilterGraph *pGraph, LPCWSTR pName)
{
#if write_level > 0
    print("filter_out_filter::JoinFilterGraph called...\n");
#endif
    EnterCriticalSection(&cs_filter);
    pgraph = pGraph;
    if(pName) wcscpy_s(name, MAX_FILTER_NAME, pName);
    else wcscpy_s(name, MAX_FILTER_NAME, L"");
    LeaveCriticalSection(&cs_filter);
    return S_OK;
}

HRESULT __stdcall filter_out_filter::QueryVendorInfo(LPWSTR *pVendorInfo)
{
#if write_level > 0
    print("filter_out_filter::QueryVendorInfo called...\n");
#endif
    if(!pVendorInfo) return E_POINTER;
    return E_NOTIMPL;
}

HRESULT __stdcall filter_out_filter::GetCapabilities(DWORD *pCapabilities)
{
#if write_level > 0
    print("filter_out_filter::GetCapabilities called...\n");
#endif
    HRESULT r;
    // EnterCriticalSection(&ppin->cs_pin);
    // EnterCriticalSection(&cs_filter);
    if(ppin->pconnected)
    {
        if(ppin->pconnected_media_seeking)
        {
            r = ppin->pconnected_media_seeking->GetCapabilities(pCapabilities);
        }
        else
        {
            r = E_NOTIMPL;
        }
    }
    else
    {
        r = VFW_E_NOT_CONNECTED;
    }
    // LeaveCriticalSection(&cs_filter);
    // LeaveCriticalSection(&ppin->cs_pin);
    return r;
}

HRESULT __stdcall filter_out_filter::CheckCapabilities(DWORD *pCapabilities)
{
#if write_level > 0
    print("filter_out_filter::CheckCapabilities called...\n");
#endif
    HRESULT r;
    // EnterCriticalSection(&ppin->cs_pin);
    // EnterCriticalSection(&cs_filter);
    if(ppin->pconnected)
    {
        if(ppin->pconnected_media_seeking)
        {
            r = ppin->pconnected_media_seeking->CheckCapabilities(pCapabilities);
        }
        else
        {
            r = E_NOTIMPL;
        }
    }
    else
    {
        r = VFW_E_NOT_CONNECTED;
    }
    // LeaveCriticalSection(&cs_filter);
    // LeaveCriticalSection(&ppin->cs_pin);
    return r;
}

HRESULT __stdcall filter_out_filter::IsFormatSupported(const GUID *pFormat)
{
#if write_level > 0
    print("filter_out_filter::IsFormatSupported called...\n");
#endif
    HRESULT r;
    // EnterCriticalSection(&ppin->cs_pin);
    // EnterCriticalSection(&cs_filter);
    if(ppin->pconnected)
    {
        if(ppin->pconnected_media_seeking)
        {
            r = ppin->pconnected_media_seeking->IsFormatSupported(pFormat);
        }
        else
        {
            r = E_NOTIMPL;
        }
    }
    else
    {
        r = VFW_E_NOT_CONNECTED;
    }
    // LeaveCriticalSection(&cs_filter);
    // LeaveCriticalSection(&ppin->cs_pin);
    return r;
}

HRESULT __stdcall filter_out_filter::QueryPreferredFormat(GUID *pFormat)
{
#if write_level > 0
    print("filter_out_filter::QueryPreferredFormat called...\n");
#endif
    HRESULT r;
    // EnterCriticalSection(&ppin->cs_pin);
    // EnterCriticalSection(&cs_filter);
    if(ppin->pconnected)
    {
        if(ppin->pconnected_media_seeking)
        {
            r = ppin->pconnected_media_seeking->QueryPreferredFormat(pFormat);
        }
        else
        {
            r = E_NOTIMPL;
        }
    }
    else
    {
        r = VFW_E_NOT_CONNECTED;
    }
    // LeaveCriticalSection(&cs_filter);
    // LeaveCriticalSection(&ppin->cs_pin);
    return r;
}

HRESULT __stdcall filter_out_filter::GetTimeFormat(GUID *pFormat)
{
#if write_level > 0
    print("filter_out_filter::GetTimeFormat called...\n");
#endif
    HRESULT r;
    // EnterCriticalSection(&ppin->cs_pin);
    // EnterCriticalSection(&cs_filter);
    if(ppin->pconnected)
    {
        if(ppin->pconnected_media_seeking)
        {
            r = ppin->pconnected_media_seeking->GetTimeFormat(pFormat);
        }
        else
        {
            r = E_NOTIMPL;
        }
    }
    else
    {
        r = VFW_E_NOT_CONNECTED;
    }
    // LeaveCriticalSection(&cs_filter);
    // LeaveCriticalSection(&ppin->cs_pin);
    return r;
}

HRESULT __stdcall filter_out_filter::IsUsingTimeFormat(const GUID *pFormat)
{
#if write_level > 0
    print("filter_out_filter::IsUsingTimeFormat called...\n");
#endif
    HRESULT r;
    // EnterCriticalSection(&ppin->cs_pin);
    // EnterCriticalSection(&cs_filter);
    if(ppin->pconnected)
    {
        if(ppin->pconnected_media_seeking)
        {
            r = ppin->pconnected_media_seeking->IsUsingTimeFormat(pFormat);
        }
        else
        {
            r = E_NOTIMPL;
        }
    }
    else
    {
        r = VFW_E_NOT_CONNECTED;
    }
    // LeaveCriticalSection(&cs_filter);
    // LeaveCriticalSection(&ppin->cs_pin);
    return r;
}

HRESULT __stdcall filter_out_filter::SetTimeFormat(const GUID *pFormat)
{
#if write_level > 0
    print("filter_out_filter::SetTimeFormat called...\n");
#endif
    HRESULT r;
    // EnterCriticalSection(&ppin->cs_pin);
    // EnterCriticalSection(&cs_filter);
    if(ppin->pconnected)
    {
        if(ppin->pconnected_media_seeking)
        {
            r = ppin->pconnected_media_seeking->SetTimeFormat(pFormat);
        }
        else
        {
            r = E_NOTIMPL;
        }
    }
    else
    {
        r = VFW_E_NOT_CONNECTED;
    }
    // LeaveCriticalSection(&cs_filter);
    // LeaveCriticalSection(&ppin->cs_pin);
    return r;
}

HRESULT __stdcall filter_out_filter::GetDuration(LONGLONG *pDuration)
{
#if write_level > 0
    print("filter_out_filter::GetDuration called...\n");
#endif
    HRESULT r;
    // EnterCriticalSection(&ppin->cs_pin);
    // EnterCriticalSection(&cs_filter);
    if(ppin->pconnected)
    {
        if(ppin->pconnected_media_seeking)
        {
            r = ppin->pconnected_media_seeking->GetDuration(pDuration);
        }
        else
        {
            r = E_NOTIMPL;
        }
    }
    else
    {
        r = VFW_E_NOT_CONNECTED;
    }
    // LeaveCriticalSection(&cs_filter);
    // LeaveCriticalSection(&ppin->cs_pin);
    return r;
}

HRESULT __stdcall filter_out_filter::GetStopPosition(LONGLONG *pStop)
{
#if write_level > 0
    print("filter_out_filter::GetStopPosition called...\n");
#endif
    HRESULT r;
    // EnterCriticalSection(&ppin->cs_pin);
    // EnterCriticalSection(&cs_filter);
    if(ppin->pconnected)
    {
        if(ppin->pconnected_media_seeking)
        {
            r = ppin->pconnected_media_seeking->GetStopPosition(pStop);
        }
        else
        {
            r = E_NOTIMPL;
        }
    }
    else
    {
        r = VFW_E_NOT_CONNECTED;
    }
    // LeaveCriticalSection(&cs_filter);
    // LeaveCriticalSection(&ppin->cs_pin);
    return r;
}

HRESULT __stdcall filter_out_filter::GetCurrentPosition(LONGLONG *pCurrent)
{
#if write_level > 0
    print("filter_out_filter::GetCurrentPosition called...\n");
#endif
    HRESULT r;
    // EnterCriticalSection(&ppin->cs_pin);
    // EnterCriticalSection(&cs_filter);
    if(ppin->pconnected)
    {
        if(ppin->pconnected_media_seeking)
        {
            r = ppin->pconnected_media_seeking->GetCurrentPosition(pCurrent);
        }
        else
        {
            r = E_NOTIMPL;
        }
    }
    else
    {
        r = VFW_E_NOT_CONNECTED;
    }
    // LeaveCriticalSection(&cs_filter);
    // LeaveCriticalSection(&ppin->cs_pin);
    return r;
}

HRESULT __stdcall filter_out_filter::ConvertTimeFormat(LONGLONG *pTarget, const GUID *pTargetFormat, LONGLONG Source, const GUID *pSourceFormat)
{
#if write_level > 0
    print("filter_out_filter::ConvertTimeFormat called...\n");
#endif
    HRESULT r;
    // EnterCriticalSection(&ppin->cs_pin);
    // EnterCriticalSection(&cs_filter);
    if(ppin->pconnected)
    {
        if(ppin->pconnected_media_seeking)
        {
            r = ppin->pconnected_media_seeking->ConvertTimeFormat(pTarget, pTargetFormat, Source, pSourceFormat);
        }
        else
        {
            r = E_NOTIMPL;
        }
    }
    else
    {
        r = VFW_E_NOT_CONNECTED;
    }
    // LeaveCriticalSection(&cs_filter);
    // LeaveCriticalSection(&ppin->cs_pin);
    return r;
}

HRESULT __stdcall filter_out_filter::SetPositions(LONGLONG *pCurrent, DWORD dwCurrentFlags, LONGLONG *pStop, DWORD dwStopFlags)
{
#if write_level > 0
    print("filter_out_filter::SetPositions called...\n");
#endif
    HRESULT r;
    // EnterCriticalSection(&ppin->cs_pin);
    // EnterCriticalSection(&cs_filter);
    if(ppin->pconnected)
    {
        if(ppin->pconnected_media_seeking)
        {
            r = ppin->pconnected_media_seeking->SetPositions(pCurrent, dwCurrentFlags, pStop, dwStopFlags);
        }
        else
        {
            r = E_NOTIMPL;
        }
    }
    else
    {
        r = VFW_E_NOT_CONNECTED;
    }
    // LeaveCriticalSection(&cs_filter);
    // LeaveCriticalSection(&ppin->cs_pin);
    return r;
}

HRESULT __stdcall filter_out_filter::GetPositions(LONGLONG *pCurrent, LONGLONG *pStop)
{
#if write_level > 0
    print("filter_out_filter::GetPositions called...\n");
#endif
    HRESULT r;
    // EnterCriticalSection(&ppin->cs_pin);
    // EnterCriticalSection(&cs_filter);
    if(ppin->pconnected)
    {
        if(ppin->pconnected_media_seeking)
        {
            r = ppin->pconnected_media_seeking->GetPositions(pCurrent, pStop);
        }
        else
        {
            r = E_NOTIMPL;
        }
    }
    else
    {
        r = VFW_E_NOT_CONNECTED;
    }
    // LeaveCriticalSection(&cs_filter);
    // LeaveCriticalSection(&ppin->cs_pin);
    return r;
}

HRESULT __stdcall filter_out_filter::GetAvailable(LONGLONG *pEarliest, LONGLONG *pLatest)
{
#if write_level > 0
    print("filter_out_filter::GetAvailable called...\n");
#endif
    HRESULT r;
    // EnterCriticalSection(&ppin->cs_pin);
    // EnterCriticalSection(&cs_filter);
    if(ppin->pconnected)
    {
        if(ppin->pconnected_media_seeking)
        {
            r = ppin->pconnected_media_seeking->GetAvailable(pEarliest, pLatest);
        }
        else
        {
            r = E_NOTIMPL;
        }
    }
    else
    {
        r = VFW_E_NOT_CONNECTED;
    }
    // LeaveCriticalSection(&cs_filter);
    // LeaveCriticalSection(&ppin->cs_pin);
    return r;
}

HRESULT __stdcall filter_out_filter::SetRate(double dRate)
{
#if write_level > 0
    print("filter_out_filter::SetRate called...\n");
#endif
    HRESULT r;
    // EnterCriticalSection(&ppin->cs_pin);
    // EnterCriticalSection(&cs_filter);
    if(ppin->pconnected)
    {
        if(ppin->pconnected_media_seeking)
        {
            r = ppin->pconnected_media_seeking->SetRate(dRate);
        }
        else
        {
            r = E_NOTIMPL;
        }
    }
    else
    {
        r = VFW_E_NOT_CONNECTED;
    }
    // LeaveCriticalSection(&cs_filter);
    // LeaveCriticalSection(&ppin->cs_pin);
    return r;
}

HRESULT __stdcall filter_out_filter::GetRate(double *pdRate)
{
#if write_level > 0
    print("filter_out_filter::GetRate called...\n");
#endif
    HRESULT r;
    // EnterCriticalSection(&ppin->cs_pin);
    // EnterCriticalSection(&cs_filter);
    if(ppin->pconnected)
    {
        if(ppin->pconnected_media_seeking)
        {
            r = ppin->pconnected_media_seeking->GetRate(pdRate);
        }
        else
        {
            r = E_NOTIMPL;
        }
    }
    else
    {
        r = VFW_E_NOT_CONNECTED;
    }
    // LeaveCriticalSection(&cs_filter);
    // LeaveCriticalSection(&ppin->cs_pin);
    return r;
}

HRESULT __stdcall filter_out_filter::GetPreroll(LONGLONG *pllPreroll)
{
#if write_level > 0
    print("filter_out_filter::GetPreroll called...\n");
#endif
    HRESULT r;
    // EnterCriticalSection(&ppin->cs_pin);
    // EnterCriticalSection(&cs_filter);
    if(ppin->pconnected)
    {
        if(ppin->pconnected_media_seeking)
        {
            r = ppin->pconnected_media_seeking->GetPreroll(pllPreroll);
        }
        else
        {
            r = E_NOTIMPL;
        }
    }
    else
    {
        r = VFW_E_NOT_CONNECTED;
    }
    // LeaveCriticalSection(&cs_filter);
    // LeaveCriticalSection(&ppin->cs_pin);
    return r;
}

inline nat32 filter_out_filter::round(nat32 n)
{
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;
    return n;
}

bool filter_out_filter::create(const AM_MEDIA_TYPE *pamt, player_callback *pcallback, filter_out_filter **ppfilter)
{
#if write_level > 1
    print("filter_out_filter::create called...\n");
#endif
    if(!pamt || !pcallback || !ppfilter) return false;

    filter_out_filter *pfilter = new filter_out_filter;
    if(!pfilter) return false;

    pfilter->ppin = new filter_out_pin;
    if(!pfilter->ppin)
    {
        delete pfilter;
        return false;
    }

    if(!InitializeCriticalSectionAndSpinCount(&pfilter->cs_filter, 0x80000000))
    {
        delete pfilter->ppin;
        delete pfilter;
        return false;
    }

    if(!InitializeCriticalSectionAndSpinCount(&pfilter->ppin->cs_pin, 0x80000000))
    {
        DeleteCriticalSection(&pfilter->cs_filter);
        delete pfilter->ppin;
        delete pfilter;
        return false;
    }

    pfilter->ppin->event_flushing = CreateEvent(null, true, false, null);
    if(!pfilter->ppin->event_flushing)
    {
        DeleteCriticalSection(&pfilter->ppin->cs_pin);
        DeleteCriticalSection(&pfilter->cs_filter);
        delete pfilter->ppin;
        delete pfilter;
        return false;
    }

    if(!InitializeCriticalSectionAndSpinCount(&pfilter->ppin->cs_receive, 0x80000000))
    {
        CloseHandle(pfilter->ppin->event_flushing);
        DeleteCriticalSection(&pfilter->ppin->cs_pin);
        DeleteCriticalSection(&pfilter->cs_filter);
        delete pfilter->ppin;
        delete pfilter;
        return false;
    }

    pfilter->event_state_set = CreateEvent(null, true, true, null);
    if(!pfilter->event_state_set)
    {
        DeleteCriticalSection(&pfilter->ppin->cs_receive);
        CloseHandle(pfilter->ppin->event_flushing);
        DeleteCriticalSection(&pfilter->ppin->cs_pin);
        DeleteCriticalSection(&pfilter->cs_filter);
        delete pfilter->ppin;
        delete pfilter;
        return false;
    }

    pfilter->event_not_paused = CreateEvent(null, true, true, null);
    if(!pfilter->event_not_paused)
    {
        CloseHandle(pfilter->event_state_set);
        DeleteCriticalSection(&pfilter->ppin->cs_receive);
        CloseHandle(pfilter->ppin->event_flushing);
        DeleteCriticalSection(&pfilter->ppin->cs_pin);
        DeleteCriticalSection(&pfilter->cs_filter);
        delete pfilter->ppin;
        delete pfilter;
        return false;
    }

    if(pamt->cbFormat)
    {
        pfilter->ppin->amt.pbFormat = new bits8[pamt->cbFormat];
        if(!pfilter->ppin->amt.pbFormat)
        {
            CloseHandle(pfilter->event_state_set);
            DeleteCriticalSection(&pfilter->ppin->cs_receive);
            CloseHandle(pfilter->ppin->event_flushing);
            DeleteCriticalSection(&pfilter->ppin->cs_pin);
            DeleteCriticalSection(&pfilter->cs_filter);
            delete pfilter->ppin;
            delete pfilter;
            return false;
        }
        memcpy(pfilter->ppin->amt.pbFormat, pamt->pbFormat, pamt->cbFormat);
    }
    else pfilter->ppin->amt.pbFormat = null;
    pfilter->reference_count = 1;
    pfilter->pcallback = pcallback;
    pfilter->ppin->pfilter = pfilter;
    pfilter->ppin->amt.majortype = pamt->majortype;
    pfilter->ppin->amt.subtype = pamt->subtype;
    pfilter->ppin->amt.bFixedSizeSamples = pamt->bFixedSizeSamples;
    pfilter->ppin->amt.bTemporalCompression = pamt->bTemporalCompression;
    pfilter->ppin->amt.lSampleSize = pamt->lSampleSize;
    pfilter->ppin->amt.formattype = pamt->formattype;
    pfilter->ppin->amt.pUnk = pamt->pUnk;
    if(pfilter->ppin->amt.pUnk) pfilter->ppin->amt.pUnk->AddRef();
    pfilter->ppin->amt.cbFormat = pamt->cbFormat;
    pfilter->ppin->pconnected = null;
    pfilter->ppin->flushing = false;
    pfilter->pgraph = null;
    pfilter->pclock = null;
    wcscpy_s(pfilter->name, MAX_FILTER_NAME, L"");
    pfilter->state = State_Stopped;
    pfilter->state2 = State_Stopped;
    *ppfilter =  pfilter;
    return true;
}

//----------------------------------------------------------------------------
// filter_out
//----------------------------------------------------------------------------

bool filter_out::create(const AM_MEDIA_TYPE *pamt, player_callback *pcallback, filter_out **ppfilter)
{
    if(!pamt || !pcallback || !ppfilter) return false;
    filter_out_filter *pfilter;
    if(!filter_out_filter::create(pamt, pcallback, &pfilter)) return false;
    *ppfilter = pfilter;
    return true;
}
