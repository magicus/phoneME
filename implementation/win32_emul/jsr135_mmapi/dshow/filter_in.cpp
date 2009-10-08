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

#include <vfwmsgs.h>
#include "filter_in.hpp"

#define write_level 0

#if write_level > 0
#include "writer.hpp"
#endif


const nat32 null = 0;


class filter_in_pin;
class filter_in_filter;

class filter_in_enum_media_types : public IEnumMediaTypes
{
    friend filter_in_pin;

    nat32 reference_count;
    AM_MEDIA_TYPE amt;
    nat32 index;

    filter_in_enum_media_types();
    ~filter_in_enum_media_types();
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

class filter_in_pin : public IPin, IAsyncReader
{
    friend filter_in_filter;

    filter_in_filter *pfilter;
    AM_MEDIA_TYPE amt;

    CRITICAL_SECTION cs_data;
    int64 stream_len;
    nat32 requests;
    IMediaSample *req_pms[1024];
    DWORD_PTR req_user[1024];

    IPin *pconnected;
    bool flushing;

    filter_in_pin();
    ~filter_in_pin();
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
    // IAsyncReader
    virtual HRESULT __stdcall RequestAllocator(IMemAllocator *pPreferred, ALLOCATOR_PROPERTIES *pProps, IMemAllocator **ppActual);
    virtual HRESULT __stdcall Request(IMediaSample *pSample, DWORD_PTR dwUser);
    virtual HRESULT __stdcall WaitForNext(DWORD dwTimeout, IMediaSample **ppSample, DWORD_PTR *pdwUser);
    virtual HRESULT __stdcall SyncReadAligned(IMediaSample *pSample);
    virtual HRESULT __stdcall SyncRead(LONGLONG llPosition, LONG lLength, BYTE *pBuffer);
    virtual HRESULT __stdcall Length(LONGLONG *pTotal, LONGLONG *pAvailable);
    // virtual HRESULT __stdcall BeginFlush();
    // virtual HRESULT __stdcall EndFlush();
};

class filter_in_enum_pins : public IEnumPins
{
    friend filter_in_filter;

    nat32 reference_count;
    filter_in_pin *ppin;
    nat32 index;

    filter_in_enum_pins();
    ~filter_in_enum_pins();
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

class filter_in_filter : public filter_in, IAMFilterMiscFlags
{
    friend filter_in_pin;

    CRITICAL_SECTION cs_filter;
    nat32 reference_count;
    player_callback *pcallback;
    filter_in_pin pin;
    IFilterGraph *pgraph;
    IReferenceClock *pclock;
    char16 name[MAX_FILTER_NAME];
    FILTER_STATE state;

    filter_in_filter();
    ~filter_in_filter();
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
    // filter_in
    virtual bool set_stream_length(int64 length);
    // IAMFilterMiscFlags
    virtual ULONG __stdcall GetMiscFlags();

    static bool create(const AM_MEDIA_TYPE *pamt, player_callback *pcallback, filter_in_filter **ppfilter);
};

//----------------------------------------------------------------------------
// filter_in_enum_media_types
//----------------------------------------------------------------------------

filter_in_enum_media_types::filter_in_enum_media_types()
{
#if write_level > 1
    print("filter_in_enum_media_types::filter_in_enum_media_types called...\n");
#endif
}

filter_in_enum_media_types::~filter_in_enum_media_types()
{
#if write_level > 1
    print("filter_in_enum_media_types::~filter_in_enum_media_types called...\n");
#endif
}

HRESULT __stdcall filter_in_enum_media_types::QueryInterface(REFIID riid, void **ppvObject)
{
#if write_level > 0
    print("filter_in_enum_media_types::QueryInterface called...\n");
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

ULONG __stdcall filter_in_enum_media_types::AddRef()
{
#if write_level > 1
    print("filter_in_enum_media_types::AddRef called...\n");
#endif
    return ++reference_count;
}

ULONG __stdcall filter_in_enum_media_types::Release()
{
#if write_level > 1
    print("filter_in_enum_media_types::Release called...\n");
#endif
    if(reference_count == 1)
    {
        if(amt.pUnk) amt.pUnk->Release();
        if(amt.cbFormat) delete[] (bits8 *)amt.pbFormat;
        delete this;
        return 0;
    }
    return --reference_count;
}

HRESULT __stdcall filter_in_enum_media_types::Next(ULONG cMediaTypes, AM_MEDIA_TYPE **ppMediaTypes, ULONG *pcFetched)
{
#if write_level > 0
    print("filter_in_enum_media_types::Next called...\n");
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

HRESULT __stdcall filter_in_enum_media_types::Skip(ULONG cMediaTypes)
{
#if write_level > 0
    print("filter_in_enum_media_types::Skip called...\n");
#endif
    if(index + cMediaTypes > 1)
    {
        if(index != 1) index = 1;
        return S_FALSE;
    }
    index += cMediaTypes;
    return S_OK;
}

HRESULT __stdcall filter_in_enum_media_types::Reset()
{
#if write_level > 0
    print("filter_in_enum_media_types::Reset called...\n");
#endif
    index = 0;
    return S_OK;
}

HRESULT __stdcall filter_in_enum_media_types::Clone(IEnumMediaTypes **ppEnum)
{
#if write_level > 0
    print("filter_in_enum_media_types::Clone called...\n");
#endif
    if(!ppEnum) return E_POINTER;
    filter_in_enum_media_types *penum_media_types = new filter_in_enum_media_types;
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
// filter_in_pin
//----------------------------------------------------------------------------

filter_in_pin::filter_in_pin()
{
#if write_level > 1
    print("filter_in_pin::filter_in_pin called...\n");
#endif
}

filter_in_pin::~filter_in_pin()
{
#if write_level > 1
    print("filter_in_pin::~filter_in_pin called...\n");
#endif
}

HRESULT __stdcall filter_in_pin::QueryInterface(REFIID riid, void **ppvObject)
{
#if write_level > 0
    print("filter_in_pin::QueryInterface(");
    print(riid);
    print(", 0x%p) called...\n", ppvObject);
#endif
    if(!ppvObject) return E_POINTER;
    if(riid == IID_IUnknown)
    {
        *(IUnknown **)ppvObject = (IPin *)this;
        ((IUnknown *)*ppvObject)->AddRef();
        return S_OK;
    }
    if(riid == IID_IPin)
    {
        *(IPin **)ppvObject = this;
        ((IPin *)*ppvObject)->AddRef();
        return S_OK;
    }
    if(riid == IID_IAsyncReader)
    {
        *(IAsyncReader **)ppvObject = this;
        ((IAsyncReader *)*ppvObject)->AddRef();
        return S_OK;
    }
    *ppvObject = null;
    return E_NOINTERFACE;
}

ULONG __stdcall filter_in_pin::AddRef()
{
#if write_level > 1
    print("filter_in_pin::AddRef called...\n");
#endif
    return pfilter->AddRef();
}

ULONG __stdcall filter_in_pin::Release()
{
#if write_level > 1
    print("filter_in_pin::Release called...\n");
#endif
    return pfilter->Release();
}

HRESULT __stdcall filter_in_pin::Connect(IPin *pReceivePin, const AM_MEDIA_TYPE *pmt)
{
#if write_level > 0
    print("filter_in_pin::Connect called...\n");
#endif
    EnterCriticalSection(&pfilter->cs_filter);
    if(!pReceivePin)
    {
        LeaveCriticalSection(&pfilter->cs_filter);
        return E_POINTER;
    }
    if(pconnected)
    {
        LeaveCriticalSection(&pfilter->cs_filter);
        return VFW_E_ALREADY_CONNECTED;
    }
    if(pfilter->state != State_Stopped)
    {
        LeaveCriticalSection(&pfilter->cs_filter);
        return VFW_E_NOT_STOPPED;
    }

    if(pmt)
    {
        if(pmt->majortype != GUID_NULL && pmt->majortype != amt.majortype)
        {
            LeaveCriticalSection(&pfilter->cs_filter);
            return VFW_E_TYPE_NOT_ACCEPTED;
        }
        if(pmt->subtype != GUID_NULL && pmt->subtype != amt.subtype)
        {
            LeaveCriticalSection(&pfilter->cs_filter);
            return VFW_E_TYPE_NOT_ACCEPTED;
        }
        if(pmt->formattype != GUID_NULL && pmt->formattype != amt.formattype)
        {
            LeaveCriticalSection(&pfilter->cs_filter);
            return VFW_E_TYPE_NOT_ACCEPTED;
        }
    }

#if write_level > 0
    PIN_INFO pi;
    pReceivePin->QueryPinInfo(&pi);
    dump_filter(pi.pFilter, 0);
    dump_media_type(&amt);
    print("\n");
    if(pmt)
    {
        dump_media_type(pmt);
        print("\n");
    }
#endif
    HRESULT hr = pReceivePin->ReceiveConnection(this, &amt);
#if write_level > 0
    error(hr);
#endif
    if(hr == VFW_E_TYPE_NOT_ACCEPTED)
    {
        LeaveCriticalSection(&pfilter->cs_filter);
        return VFW_E_NO_ACCEPTABLE_TYPES;
    }
    if(hr != S_OK)
    {
        LeaveCriticalSection(&pfilter->cs_filter);
        return hr;
    }
    pconnected = pReceivePin;
    pconnected->AddRef();
    LeaveCriticalSection(&pfilter->cs_filter);
    return S_OK;
}

HRESULT __stdcall filter_in_pin::ReceiveConnection(IPin *pConnector, const AM_MEDIA_TYPE *pmt)
{
#if write_level > 0
    print("filter_in_pin::ReceiveConnection called...\n");
#endif
    EnterCriticalSection(&pfilter->cs_filter);
    if(!pConnector || !pmt)
    {
        LeaveCriticalSection(&pfilter->cs_filter);
        return E_POINTER;
    }
    LeaveCriticalSection(&pfilter->cs_filter);
    return E_UNEXPECTED;
}

HRESULT __stdcall filter_in_pin::Disconnect()
{
#if write_level > 0
    print("filter_in_pin::Disconnect called...\n");
#endif
    EnterCriticalSection(&pfilter->cs_filter);
    if(pfilter->state != State_Stopped)
    {
        LeaveCriticalSection(&pfilter->cs_filter);
        return VFW_E_NOT_STOPPED;
    }
    if(!pconnected)
    {
        LeaveCriticalSection(&pfilter->cs_filter);
        return S_FALSE;
    }
    pconnected->Release();
    pconnected = null;
    LeaveCriticalSection(&pfilter->cs_filter);
    return S_OK;
}

HRESULT __stdcall filter_in_pin::ConnectedTo(IPin **pPin)
{
#if write_level > 0
    print("filter_in_pin::ConnectedTo called...\n");
#endif
    EnterCriticalSection(&pfilter->cs_filter);
    if(!pPin)
    {
        LeaveCriticalSection(&pfilter->cs_filter);
        return E_POINTER;
    }
    if(!pconnected)
    {
        *pPin = null;
        LeaveCriticalSection(&pfilter->cs_filter);
        return VFW_E_NOT_CONNECTED;
    }
    *pPin = pconnected;
    (*pPin)->AddRef();
    LeaveCriticalSection(&pfilter->cs_filter);
    return S_OK;
}

HRESULT __stdcall filter_in_pin::ConnectionMediaType(AM_MEDIA_TYPE *pmt)
{
#if write_level > 0
    print("filter_in_pin::ConnectionMediaType called...\n");
#endif
    EnterCriticalSection(&pfilter->cs_filter);
    if(!pmt)
    {
        LeaveCriticalSection(&pfilter->cs_filter);
        return E_POINTER;
    }
    if(!pconnected)
    {
        memset(pmt, 0, sizeof(AM_MEDIA_TYPE));
        LeaveCriticalSection(&pfilter->cs_filter);
        return VFW_E_NOT_CONNECTED;
    }
    if(amt.cbFormat)
    {
        pmt->pbFormat = (bits8 *)CoTaskMemAlloc(amt.cbFormat);
        if(!pmt->pbFormat)
        {
            LeaveCriticalSection(&pfilter->cs_filter);
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
    LeaveCriticalSection(&pfilter->cs_filter);
    return S_OK;
}

HRESULT __stdcall filter_in_pin::QueryPinInfo(PIN_INFO *pInfo)
{
#if write_level > 0
    print("filter_in_pin::QueryPinInfo called...\n");
#endif
    EnterCriticalSection(&pfilter->cs_filter);
    if(!pInfo)
    {
        LeaveCriticalSection(&pfilter->cs_filter);
        return E_POINTER;
    }
    pInfo->pFilter = pfilter;
    pInfo->pFilter->AddRef();
    pInfo->dir = PINDIR_OUTPUT;
    wcscpy_s(pInfo->achName, MAX_PIN_NAME, L"Output");
    LeaveCriticalSection(&pfilter->cs_filter);
    return S_OK;
}

HRESULT __stdcall filter_in_pin::QueryDirection(PIN_DIRECTION *pPinDir)
{
#if write_level > 0
    print("filter_in_pin::QueryDirection called...\n");
#endif
    if(!pPinDir) return E_POINTER;
    *pPinDir = PINDIR_OUTPUT;
    return S_OK;
}

HRESULT __stdcall filter_in_pin::QueryId(LPWSTR *Id)
{
#if write_level > 0
    print("filter_in_pin::QueryId called...\n");
#endif
    if(!Id) return E_POINTER;
    *Id = (char16 *)CoTaskMemAlloc(sizeof(char16) * 7);
    if(!*Id) return E_OUTOFMEMORY;
    memcpy(*Id, L"Output", sizeof(char16) * 7);
    return S_OK;
}

HRESULT __stdcall filter_in_pin::QueryAccept(const AM_MEDIA_TYPE * /*pmt*/)
{
#if write_level > 0
    print("filter_in_pin::QueryAccept called...\n");
#endif
    return S_OK;
}

HRESULT __stdcall filter_in_pin::EnumMediaTypes(IEnumMediaTypes **ppEnum)
{
#if write_level > 0
    print("filter_in_pin::EnumMediaTypes called...\n");
#endif
    EnterCriticalSection(&pfilter->cs_filter);
    if(!ppEnum)
    {
        LeaveCriticalSection(&pfilter->cs_filter);
        return E_POINTER;
    }
    filter_in_enum_media_types *penum_media_types = new filter_in_enum_media_types;
    if(!penum_media_types)
    {
        LeaveCriticalSection(&pfilter->cs_filter);
        return E_OUTOFMEMORY;
    }
    if(amt.cbFormat)
    {
        penum_media_types->amt.pbFormat = new bits8[amt.cbFormat];
        if(!penum_media_types->amt.pbFormat)
        {
            delete penum_media_types;
            LeaveCriticalSection(&pfilter->cs_filter);
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
    LeaveCriticalSection(&pfilter->cs_filter);
    return S_OK;
}

HRESULT __stdcall filter_in_pin::QueryInternalConnections(IPin **apPin, ULONG *nPin)
{
#if write_level > 0
    print("filter_in_pin::QueryInternalConnections called...\n");
#endif
    if(!apPin || !nPin) return E_POINTER;
    return E_NOTIMPL;
}

HRESULT __stdcall filter_in_pin::EndOfStream()
{
#if write_level > 0
    print("filter_in_pin::EndOfStream called...\n");
#endif
    return E_UNEXPECTED;
}

HRESULT __stdcall filter_in_pin::BeginFlush()
{
#if write_level > 0
    print("filter_in_pin::BeginFlush called...\n");
#endif
    EnterCriticalSection(&pfilter->cs_filter);
    if(flushing)
    {
        LeaveCriticalSection(&pfilter->cs_filter);
        return S_FALSE;
    }
    flushing = true;
    LeaveCriticalSection(&pfilter->cs_filter);
    return S_OK;
}

HRESULT __stdcall filter_in_pin::EndFlush()
{
#if write_level > 0
    print("filter_in_pin::EndFlush called...\n");
#endif
    EnterCriticalSection(&pfilter->cs_filter);
    if(!flushing)
    {
        LeaveCriticalSection(&pfilter->cs_filter);
        return S_FALSE;
    }
    flushing = false;
    LeaveCriticalSection(&pfilter->cs_filter);
    return S_OK;
}

HRESULT __stdcall filter_in_pin::NewSegment(REFERENCE_TIME /*tStart*/, REFERENCE_TIME /*tStop*/, double /*dRate*/)
{
#if write_level > 0
    print("filter_in_pin::NewSegment called...\n");
#endif
    return S_OK;
}

HRESULT __stdcall filter_in_pin::RequestAllocator(IMemAllocator *pPreferred, ALLOCATOR_PROPERTIES *pProps, IMemAllocator **ppActual)
{
#if write_level > 0
    print("filter_in_pin::RequestAllocator called...\n");
#endif
    EnterCriticalSection(&pfilter->cs_filter);
    if(!pProps || !ppActual)
    {
        LeaveCriticalSection(&pfilter->cs_filter);
        return E_POINTER;
    }
    if(!pPreferred)
    {
        LeaveCriticalSection(&pfilter->cs_filter);
        return E_FAIL;
    }
    ALLOCATOR_PROPERTIES apr;
    ALLOCATOR_PROPERTIES apa;
    apr.cBuffers = pProps->cBuffers;
    apr.cbBuffer = pProps->cbBuffer;
    apr.cbAlign = 1;
    apr.cbPrefix = pProps->cbPrefix;
    HRESULT hr = pPreferred->SetProperties(&apr, &apa);
    if(hr != S_OK)
    {
        LeaveCriticalSection(&pfilter->cs_filter);
        return E_FAIL;
    }
    *ppActual = pPreferred;
    (*ppActual)->AddRef();
    LeaveCriticalSection(&pfilter->cs_filter);
    return S_OK;
}

HRESULT __stdcall filter_in_pin::Request(IMediaSample *pSample, DWORD_PTR dwUser)
{
#if write_level > 0
    print("filter_in_pin::Request called...\n");
#endif
    EnterCriticalSection(&pfilter->cs_filter);
    if(!pSample)
    {
        LeaveCriticalSection(&pfilter->cs_filter);
        return E_POINTER;
    }
    if(flushing)
    {
        LeaveCriticalSection(&pfilter->cs_filter);
        return VFW_E_WRONG_STATE;
    }
    if(requests >= 1024)
    {
        LeaveCriticalSection(&pfilter->cs_filter);
        return E_OUTOFMEMORY;
    }
    req_pms[requests] = pSample;
    req_user[requests] = dwUser;
    requests++;
    LeaveCriticalSection(&pfilter->cs_filter);
    return S_OK;
}

HRESULT __stdcall filter_in_pin::WaitForNext(DWORD dwTimeout, IMediaSample **ppSample, DWORD_PTR *pdwUser)
{
#if write_level > 0
    print("filter_in_pin::WaitForNext called...\n");
#endif
    EnterCriticalSection(&pfilter->cs_filter);
    if(!ppSample)
    {
#if write_level > 0
        print("filter_in_pin::WaitForNext returns E_POINTER.\n");
#endif
        LeaveCriticalSection(&pfilter->cs_filter);
        return E_POINTER;
    }

    for(; !requests; )
    {
        if(flushing)
        {
            *ppSample = null;
#if write_level > 0
            print("filter_in_pin::WaitForNext returns VFW_E_WRONG_STATE.\n");
#endif
            LeaveCriticalSection(&pfilter->cs_filter);
            return VFW_E_WRONG_STATE;
        }

        if(!dwTimeout)
        {
            *ppSample = null;
#if write_level > 0
            print("filter_in_pin::WaitForNext returns VFW_E_TIMEOUT.\n");
#endif
            LeaveCriticalSection(&pfilter->cs_filter);
            return VFW_E_TIMEOUT;
        }

        LeaveCriticalSection(&pfilter->cs_filter);
        Sleep(1);
        EnterCriticalSection(&pfilter->cs_filter);

        if(dwTimeout != INFINITE) dwTimeout--;
    }

    IMediaSample *p_sample = req_pms[0];
    *ppSample = p_sample;
    *pdwUser = req_user[0];
    for(nat32 i = 0; i < requests - 1; i++)
    {
        req_pms[i] = req_pms[i + 1];
        req_user[i] = req_user[i + 1];
    }
    requests--;
    if(flushing)
    {
#if write_level > 0
        print("filter_in_pin::WaitForNext returns VFW_E_WRONG_STATE.\n");
#endif
        LeaveCriticalSection(&pfilter->cs_filter);
        return VFW_E_WRONG_STATE;
    }
    else
    {
        LeaveCriticalSection(&pfilter->cs_filter);
        HRESULT r = SyncReadAligned(p_sample);
        if(r != S_OK)
        {
#if write_level > 0
            print("filter_in_pin::WaitForNext returns 0x%x.\n", r);
#endif
            return r;
        }
#if write_level > 0
        print("filter_in_pin::WaitForNext returns S_OK.\n");
#endif
        return S_OK;
    }
}

HRESULT __stdcall filter_in_pin::SyncReadAligned(IMediaSample *pSample)
{
#if write_level > 0
    print("filter_in_pin::SyncReadAligned called...\n");
#endif
    if(!pSample) return E_POINTER;

    int64 tstart;
    int64 tend;
    HRESULT r = pSample->GetTime(&tstart, &tend);
    if(r != S_OK)
    {
        if(FAILED(r)) return r;
        return VFW_E_SAMPLE_TIME_NOT_SET;
    }
    if(tstart < 0) return E_INVALIDARG;
    if(tend < tstart) return VFW_E_START_TIME_AFTER_END;
    if(tend - tstart > 0x7fffffffi64 * 10000000i64) return E_INVALIDARG;
    if(tstart % 10000000 || tend % 10000000) return E_INVALIDARG;

    int64 pos = int64(tstart / 10000000);
    int32 len = int32(tend / 10000000 - pos);

    EnterCriticalSection(&cs_data);

    if(flushing)
    {
        r = VFW_E_TIMEOUT;
    }
    else
    {
        bits8 *pb;
        HRESULT r2 = pSample->GetPointer(&pb);
        if(r2 != S_OK)
        {
            if(FAILED(r2)) r = r2;
            else r = VFW_E_RUNTIME_ERROR;
        }
        else
        {
            int32 total_len = 0;
            for(;;)
            {
                int32 block_len;
                player_callback::result r2 = pfilter->pcallback->data(
                    pos + total_len,
                    len - total_len,
                    pb + total_len,
                    &block_len);
                if(r2 == player_callback::result_success)
                {
                    total_len += block_len;
                    if(total_len == len)
                    {
                        r = S_OK;
                        break;
                    }
                }
                else
                {
                    r = E_FAIL;
                    break;
                }
            }
        }
    }

    LeaveCriticalSection(&cs_data);

    if(FAILED(r)) return r;

    HRESULT r2 = pSample->SetActualDataLength(len);
    if(r2 != S_OK)
    {
        if(FAILED(r2)) return r2;
        return VFW_E_RUNTIME_ERROR;
    }
    if(r == S_FALSE)
    {
        tend = tstart + len * 10000000i64;
        r2 = pSample->SetTime(&tstart, &tend);
        if(r2 != S_OK)
        {
            if(FAILED(r2)) return r2;
            return VFW_E_RUNTIME_ERROR;
        }
    }
    return r;
}

HRESULT __stdcall filter_in_pin::SyncRead(LONGLONG llPosition, LONG lLength, BYTE *pBuffer)
{
#if write_level > 0
    print("filter_in_pin::SyncRead(%I64i, %i, 0x%p) called...\n", llPosition, lLength, pBuffer);
#endif
    if(!pBuffer) return E_POINTER;

    if(llPosition < 0) return E_INVALIDARG;
    if(lLength < 0) return VFW_E_START_TIME_AFTER_END;

    if(llPosition > stream_len) return S_FALSE;

    HRESULT r;

    EnterCriticalSection(&cs_data);

    if(flushing)
    {
        r = VFW_E_TIMEOUT;
    }
    else
    {
        int32 total_len = 0;
        for(;;)
        {
            int32 block_len;
            player_callback::result r2 = pfilter->pcallback->data(
                llPosition + total_len,
                lLength - total_len,
                pBuffer + total_len,
                &block_len);
            if(r2 == player_callback::result_success)
            {
                total_len += block_len;
                if(total_len == lLength)
                {
                    r = S_OK;
                    break;
                }
            }
            else
            {
                r = E_FAIL;
                break;
            }
        }
    }

    LeaveCriticalSection(&cs_data);

    return r;
}

HRESULT __stdcall filter_in_pin::Length(LONGLONG *pTotal, LONGLONG *pAvailable)
{
#if write_level > 0
    print("filter_in_pin::Length called...\n");
#endif
    EnterCriticalSection(&pfilter->cs_filter);
    if(!pTotal || !pAvailable)
    {
        LeaveCriticalSection(&pfilter->cs_filter);
        return E_POINTER;
    }
    *pTotal = stream_len;
    *pAvailable = stream_len;
    LeaveCriticalSection(&pfilter->cs_filter);
    return S_OK;
}

//----------------------------------------------------------------------------
// filter_in_enum_pins
//----------------------------------------------------------------------------

filter_in_enum_pins::filter_in_enum_pins()
{
#if write_level > 1
    print("filter_in_enum_pins::filter_in_enum_pins called...\n");
#endif
}

filter_in_enum_pins::~filter_in_enum_pins()
{
#if write_level > 1
    print("filter_in_enum_pins::~filter_in_enum_pins called...\n");
#endif
}

HRESULT __stdcall filter_in_enum_pins::QueryInterface(REFIID riid, void **ppvObject)
{
#if write_level > 0
    print("filter_in_enum_pins::QueryInterface called...\n");
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

ULONG __stdcall filter_in_enum_pins::AddRef()
{
#if write_level > 1
    print("filter_in_enum_pins::AddRef called...\n");
#endif
    return ++reference_count;
}

ULONG __stdcall filter_in_enum_pins::Release()
{
#if write_level > 1
    print("filter_in_enum_pins::Release called...\n");
#endif
    if(reference_count == 1)
    {
        ppin->Release();
        delete this;
        return 0;
    }
    return --reference_count;
}

HRESULT __stdcall filter_in_enum_pins::Next(ULONG cPins, IPin **ppPins, ULONG *pcFetched)
{
#if write_level > 0
    print("filter_in_enum_pins::Next called...\n");
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

HRESULT __stdcall filter_in_enum_pins::Skip(ULONG cPins)
{
#if write_level > 0
    print("filter_in_enum_pins::Skip called...\n");
#endif
    if(index + cPins > 1)
    {
        if(index != 1) index = 1;
        return S_FALSE;
    }
    index += cPins;
    return S_OK;
}

HRESULT __stdcall filter_in_enum_pins::Reset()
{
#if write_level > 0
    print("filter_in_enum_pins::Reset called...\n");
#endif
    index = 0;
    return S_OK;
}

HRESULT __stdcall filter_in_enum_pins::Clone(IEnumPins **ppEnum)
{
#if write_level > 0
    print("filter_in_enum_pins::Clone called...\n");
#endif
    if(!ppEnum) return E_POINTER;
    filter_in_enum_pins *penum_pins = new filter_in_enum_pins;
    if(!penum_pins) return E_OUTOFMEMORY;
    penum_pins->reference_count = 1;
    penum_pins->ppin = ppin;
    penum_pins->ppin->AddRef();
    penum_pins->index = index;
    *ppEnum = penum_pins;
    return S_OK;
}

//----------------------------------------------------------------------------
// filter_in_filter
//----------------------------------------------------------------------------

filter_in_filter::filter_in_filter()
{
#if write_level > 1
    print("filter_in_filter::filter_in_filter called...\n");
#endif
}

filter_in_filter::~filter_in_filter()
{
#if write_level > 1
    print("filter_in_filter::~filter_in_filter called...\n");
#endif
}

HRESULT __stdcall filter_in_filter::QueryInterface(REFIID riid, void **ppvObject)
{
#if write_level > 0
    print("filter_in_filter::QueryInterface(");
    print(riid);
    print(", 0x%p) called...\n", ppvObject);
#endif
    EnterCriticalSection(&cs_filter);
    if(!ppvObject)
    {
        LeaveCriticalSection(&cs_filter);
        return E_POINTER;
    }
    if(riid == IID_IUnknown)
    {
        *(IUnknown **)ppvObject = (IBaseFilter *)this;
        ((IUnknown *)*ppvObject)->AddRef();
        LeaveCriticalSection(&cs_filter);
        return S_OK;
    }
    if(riid == IID_IPersist)
    {
        *(IPersist **)ppvObject = this;
        ((IPersist *)*ppvObject)->AddRef();
        LeaveCriticalSection(&cs_filter);
        return S_OK;
    }
    if(riid == IID_IMediaFilter)
    {
        *(IMediaFilter **)ppvObject = this;
        ((IMediaFilter *)*ppvObject)->AddRef();
        LeaveCriticalSection(&cs_filter);
        return S_OK;
    }
    if(riid == IID_IBaseFilter)
    {
        *(IBaseFilter **)ppvObject = this;
        ((IBaseFilter *)*ppvObject)->AddRef();
        LeaveCriticalSection(&cs_filter);
        return S_OK;
    }
    if(riid == IID_IAMFilterMiscFlags)
    {
        *(IAMFilterMiscFlags **)ppvObject = this;
        ((IAMFilterMiscFlags *)*ppvObject)->AddRef();
        LeaveCriticalSection(&cs_filter);
        return S_OK;
    }
    *ppvObject = null;
    LeaveCriticalSection(&cs_filter);
    return E_NOINTERFACE;
}

ULONG __stdcall filter_in_filter::AddRef()
{
#if write_level > 1
    print("filter_in_filter::AddRef called...\n");
#endif
    EnterCriticalSection(&cs_filter);
    nat32 rc = ++reference_count;
    LeaveCriticalSection(&cs_filter);
    return rc;
}

ULONG __stdcall filter_in_filter::Release()
{
#if write_level > 1
    print("filter_in_filter::Release called...\n");
#endif
    EnterCriticalSection(&cs_filter);
    nat32 rc = reference_count;
    if(rc == 1)
    {
        if(pclock) pclock->Release();
        if(pin.pconnected) pin.pconnected->Release();
        DeleteCriticalSection(&pin.cs_data);
        if(pin.amt.pUnk) pin.amt.pUnk->Release();
        if(pin.amt.cbFormat) delete[] (bits8 *)pin.amt.pbFormat;
        LeaveCriticalSection(&cs_filter);
        DeleteCriticalSection(&cs_filter);
        delete this;
        return 0;
    }
    rc--;
    reference_count = rc;
    LeaveCriticalSection(&cs_filter);
    return rc;
}

HRESULT __stdcall filter_in_filter::GetClassID(CLSID *pClassID)
{
#if write_level > 0
    print("filter_in_filter::GetClassID called...\n");
#endif
    if(!pClassID) return E_POINTER;
    // {6D6DE50C-5029-41E8-AB0D-7995C726ED37}
    static const GUID guid =
    { 0x6d6de50c, 0x5029, 0x41e8, { 0xab, 0x0d, 0x79, 0x95, 0xc7, 0x26, 0xed, 0x37 } };
    *pClassID = guid;
    return S_OK;
}

HRESULT __stdcall filter_in_filter::Stop()
{
#if write_level > 0
    print("filter_in_filter::Stop called...\n");
#endif
    EnterCriticalSection(&cs_filter);
    state = State_Stopped;
    LeaveCriticalSection(&cs_filter);
    return S_OK;
}

HRESULT __stdcall filter_in_filter::Pause()
{
#if write_level > 0
    print("filter_in_filter::Pause called...\n");
#endif
    EnterCriticalSection(&cs_filter);
    state = State_Paused;
    LeaveCriticalSection(&cs_filter);
    return S_OK;
}

HRESULT __stdcall filter_in_filter::Run(REFERENCE_TIME /*tStart*/)
{
#if write_level > 0
    print("filter_in_filter::Run called...\n");
#endif
    EnterCriticalSection(&cs_filter);
    state = State_Running;
    LeaveCriticalSection(&cs_filter);
    return S_OK;
}

HRESULT __stdcall filter_in_filter::GetState(DWORD /*dwMilliSecsTimeout*/, FILTER_STATE *State)
{
#if write_level > 0
    print("filter_in_filter::GetState called, state is %u...\n", state);
#endif
    EnterCriticalSection(&cs_filter);
    if(!State)
    {
        LeaveCriticalSection(&cs_filter);
        return E_POINTER;
    }
    *State = state;
    LeaveCriticalSection(&cs_filter);
    return S_OK;
}

HRESULT __stdcall filter_in_filter::SetSyncSource(IReferenceClock *pClock)
{
#if write_level > 0
    print("filter_in_filter::SetSyncSource called...\n");
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

HRESULT __stdcall filter_in_filter::GetSyncSource(IReferenceClock **pClock)
{
#if write_level > 0
    print("filter_in_filter::GetSyncSource called...\n");
#endif
    EnterCriticalSection(&cs_filter);
    if(!pClock)
    {
        LeaveCriticalSection(&cs_filter);
        return E_POINTER;
    }
    *pClock = pclock;
    if(*pClock) (*pClock)->AddRef();
    LeaveCriticalSection(&cs_filter);
    return S_OK;
}

HRESULT __stdcall filter_in_filter::EnumPins(IEnumPins **ppEnum)
{
#if write_level > 0
    print("filter_in_filter::EnumPins called...\n");
#endif
    EnterCriticalSection(&cs_filter);
    if(!ppEnum)
    {
        LeaveCriticalSection(&cs_filter);
        return E_POINTER;
    }
    filter_in_enum_pins *penum_pins = new filter_in_enum_pins;
    if(!penum_pins)
    {
        LeaveCriticalSection(&cs_filter);
        return E_OUTOFMEMORY;
    }
    penum_pins->reference_count = 1;
    penum_pins->ppin = &pin;
    penum_pins->ppin->AddRef();
    penum_pins->index = 0;
    *ppEnum = penum_pins;
    LeaveCriticalSection(&cs_filter);
    return S_OK;
}

HRESULT __stdcall filter_in_filter::FindPin(LPCWSTR Id, IPin **ppPin)
{
#if write_level > 0
    print("filter_in_filter::FindPin called...\n");
#endif
    EnterCriticalSection(&cs_filter);
    if(!ppPin)
    {
        LeaveCriticalSection(&cs_filter);
        return E_POINTER;
    }
    if(!wcscmp(Id, L"Output"))
    {
        *ppPin = &pin;
        (*ppPin)->AddRef();
        LeaveCriticalSection(&cs_filter);
        return S_OK;
    }
    *ppPin = null;
    LeaveCriticalSection(&cs_filter);
    return VFW_E_NOT_FOUND;
}

HRESULT __stdcall filter_in_filter::QueryFilterInfo(FILTER_INFO *pInfo)
{
#if write_level > 0
    print("filter_in_filter::QueryFilterInfo called...\n");
#endif
    EnterCriticalSection(&cs_filter);
    if(!pInfo)
    {
        LeaveCriticalSection(&cs_filter);
        return E_POINTER;
    }
    wcscpy_s(pInfo->achName, MAX_FILTER_NAME, name);
    pInfo->pGraph = pgraph;
    if(pInfo->pGraph) pInfo->pGraph->AddRef();
    LeaveCriticalSection(&cs_filter);
    return S_OK;
}

HRESULT __stdcall filter_in_filter::JoinFilterGraph(IFilterGraph *pGraph, LPCWSTR pName)
{
#if write_level > 0
    print("filter_in_filter::JoinFilterGraph called...\n");
#endif
    EnterCriticalSection(&cs_filter);
    pgraph = pGraph;
    if(pName) wcscpy_s(name, MAX_FILTER_NAME, pName);
    else wcscpy_s(name, MAX_FILTER_NAME, L"");
    LeaveCriticalSection(&cs_filter);
    return S_OK;
}

HRESULT __stdcall filter_in_filter::QueryVendorInfo(LPWSTR *pVendorInfo)
{
#if write_level > 0
    print("filter_in_filter::QueryVendorInfo called...\n");
#endif
    if(!pVendorInfo) return E_POINTER;
    return E_NOTIMPL;
}

ULONG __stdcall filter_in_filter::GetMiscFlags()
{
#if write_level > 0
    print("filter_in_filter::GetMiscFlags called, returns AM_FILTER_MISC_FLAGS_IS_SOURCE.\n");
#endif
    return AM_FILTER_MISC_FLAGS_IS_SOURCE;
}

bool filter_in_filter::set_stream_length(int64 length)
{
    EnterCriticalSection(&cs_filter);
    pin.stream_len = length;
    LeaveCriticalSection(&cs_filter);
    return true;
}

bool filter_in_filter::create(const AM_MEDIA_TYPE *pamt, player_callback *pcallback, filter_in_filter **ppfilter)
{
#if write_level > 1
    print("filter_in_filter::create called...\n");
#endif
    if(!pamt || !pcallback || !ppfilter) return false;
    filter_in_filter *pfilter = new filter_in_filter;
    if(!pfilter) return false;

    if(!InitializeCriticalSectionAndSpinCount(&pfilter->cs_filter, 0x80000000))
    {
        delete pfilter;
        return false;
    }
    if(!InitializeCriticalSectionAndSpinCount(&pfilter->pin.cs_data, 0x80000000))
    {
        DeleteCriticalSection(&pfilter->cs_filter);
        delete pfilter;
        return false;
    }
    if(pamt->cbFormat)
    {
        pfilter->pin.amt.pbFormat = new bits8[pamt->cbFormat];
        if(!pfilter->pin.amt.pbFormat)
        {
            DeleteCriticalSection(&pfilter->pin.cs_data);
            DeleteCriticalSection(&pfilter->cs_filter);
            delete pfilter;
            return false;
        }
        memcpy(pfilter->pin.amt.pbFormat, pamt->pbFormat, pamt->cbFormat);
    }
    else pfilter->pin.amt.pbFormat = null;
    pfilter->reference_count = 1;
    pfilter->pcallback = pcallback;
    pfilter->pin.pfilter = pfilter;
    pfilter->pin.amt.majortype = pamt->majortype;
    pfilter->pin.amt.subtype = pamt->subtype;
    pfilter->pin.amt.bFixedSizeSamples = pamt->bFixedSizeSamples;
    pfilter->pin.amt.bTemporalCompression = pamt->bTemporalCompression;
    pfilter->pin.amt.lSampleSize = pamt->lSampleSize;
    pfilter->pin.amt.formattype = pamt->formattype;
    pfilter->pin.amt.pUnk = pamt->pUnk;
    if(pfilter->pin.amt.pUnk) pfilter->pin.amt.pUnk->AddRef();
    pfilter->pin.amt.cbFormat = pamt->cbFormat;
    pfilter->pin.stream_len = 0;
    pfilter->pin.requests = 0;
    pfilter->pin.pconnected = null;
    pfilter->pin.flushing = false;
    pfilter->pgraph = null;
    pfilter->pclock = null;
    wcscpy_s(pfilter->name, MAX_FILTER_NAME, L"");
    pfilter->state = State_Stopped;
    *ppfilter =  pfilter;
    return true;
}

//----------------------------------------------------------------------------
// filter_in
//----------------------------------------------------------------------------

bool create_filter_in(const AM_MEDIA_TYPE *pamt, player_callback *pcallback, filter_in **ppfilter)
{
    if(!pamt || !pcallback || !ppfilter) return false;
    filter_in_filter *pfilter;
    if(!filter_in_filter::create(pamt, pcallback, &pfilter)) return false;
    *ppfilter = pfilter;
    return true;
}
