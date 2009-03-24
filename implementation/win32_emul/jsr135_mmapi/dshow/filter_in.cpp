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

#define write_level 1

#if write_level > 0
#include "writer.hpp"
#endif


nat32 const null = 0;


class filter_in_filter;
class filter_in_pin;

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

class filter_in_pin : public IPin, public IAsyncReader
{
    friend filter_in_filter;

    filter_in_filter *pfilter;
    AM_MEDIA_TYPE amt;

    CRITICAL_SECTION data_cs;
    nat32 data_l;
    void *data_p;
    nat32 data_a;

    IPin *pconnected;

    filter_in_pin();
    ~filter_in_pin();
public:
    // IUnknown
    virtual HRESULT __stdcall QueryInterface(REFIID riid, void **ppvObject);
    virtual ULONG __stdcall AddRef();
    virtual ULONG __stdcall Release();
    // IPin
    virtual HRESULT __stdcall Connect(IPin *pReceivePin, AM_MEDIA_TYPE const *pmt);
    virtual HRESULT __stdcall ReceiveConnection(IPin *pConnector, AM_MEDIA_TYPE const *pmt);
    virtual HRESULT __stdcall Disconnect();
    virtual HRESULT __stdcall ConnectedTo(IPin **pPin);
    virtual HRESULT __stdcall ConnectionMediaType(AM_MEDIA_TYPE *pmt);
    virtual HRESULT __stdcall QueryPinInfo(PIN_INFO *pInfo);
    virtual HRESULT __stdcall QueryDirection(PIN_DIRECTION *pPinDir);
    virtual HRESULT __stdcall QueryId(LPWSTR *Id);
    virtual HRESULT __stdcall QueryAccept(AM_MEDIA_TYPE const *pmt);
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
    //virtual HRESULT __stdcall BeginFlush();
    //virtual HRESULT __stdcall EndFlush();
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

class filter_in_filter : public filter_in
{
    friend filter_in_pin;

    nat32 reference_count;
    filter_in_pin *ppin;
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
    virtual bool data(nat32 len, void const *pdata);

    static bool create(AM_MEDIA_TYPE const *pamt, filter_in_filter **ppfilter);
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
            pamt->pbFormat = (BYTE *)CoTaskMemAlloc(amt.cbFormat);
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
        penum_media_types->amt.pbFormat = new BYTE[amt.cbFormat];
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
    print(", %p) called...\n", ppvObject);
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

HRESULT __stdcall filter_in_pin::Connect(IPin *pReceivePin, AM_MEDIA_TYPE const * /*pmt*/)
{
#if write_level > 0
    print("filter_in_pin::Connect called...\n");
#endif
    if(!pReceivePin) return E_POINTER;
    if(pconnected) return VFW_E_ALREADY_CONNECTED;
    if(pfilter->state != State_Stopped) return VFW_E_NOT_STOPPED;

    print("wp1...\n");
    PIN_INFO pi;
    pReceivePin->QueryPinInfo(&pi);
#if write_level > 0
    dump_filter(pi.pFilter, 0);
#endif
    HRESULT hr = pReceivePin->ReceiveConnection(this, &amt);
#if write_level > 0
    error(hr);
#endif
    if(hr == VFW_E_TYPE_NOT_ACCEPTED) return VFW_E_NO_ACCEPTABLE_TYPES;
    if(hr != S_OK) return hr;
    pconnected = pReceivePin;
    pconnected->AddRef();
    print("wp2...\n");
    return S_OK;
}

HRESULT __stdcall filter_in_pin::ReceiveConnection(IPin *pConnector, AM_MEDIA_TYPE const *pmt)
{
#if write_level > 0
    print("filter_in_pin::ReceiveConnection called...\n");
#endif
    if(!pConnector || !pmt) return E_POINTER;
    return E_UNEXPECTED;
}

HRESULT __stdcall filter_in_pin::Disconnect()
{
#if write_level > 0
    print("filter_in_pin::Disconnect called...\n");
#endif
    if(pfilter->state != State_Stopped) return VFW_E_NOT_STOPPED;
    if(!pconnected) return S_FALSE;
    pconnected->Release();
    pconnected = null;
    return S_OK;
}

HRESULT __stdcall filter_in_pin::ConnectedTo(IPin **pPin)
{
#if write_level > 0
    print("filter_in_pin::ConnectedTo called...\n");
#endif
    if(!pPin) return E_POINTER;
    if(!pconnected)
    {
        *pPin = null;
        return VFW_E_NOT_CONNECTED;
    }
    *pPin = pconnected;
    (*pPin)->AddRef();
    return S_OK;
}

HRESULT __stdcall filter_in_pin::ConnectionMediaType(AM_MEDIA_TYPE *pmt)
{
#if write_level > 0
    print("filter_in_pin::ConnectionMediaType called...\n");
#endif
    if(!pmt) return E_POINTER;
    if(!pconnected)
    {
        memset(pmt, 0, sizeof(AM_MEDIA_TYPE));
        return VFW_E_NOT_CONNECTED;
    }
    if(amt.cbFormat)
    {
        pmt->pbFormat = (BYTE *)CoTaskMemAlloc(amt.cbFormat);
        if(!pmt->pbFormat) return E_OUTOFMEMORY;
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
    return S_OK;
}

HRESULT __stdcall filter_in_pin::QueryPinInfo(PIN_INFO *pInfo)
{
#if write_level > 0
    print("filter_in_pin::QueryPinInfo called...\n");
#endif
    if(!pInfo) return E_POINTER;
    pInfo->pFilter = pfilter;
    pInfo->pFilter->AddRef();
    pInfo->dir = PINDIR_OUTPUT;
    wcscpy_s(pInfo->achName, MAX_PIN_NAME, L"Output");
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
    *Id = (WCHAR *)CoTaskMemAlloc(sizeof(WCHAR) * 7);
    if(!*Id) return E_OUTOFMEMORY;
    memcpy(*Id, L"Output", sizeof(WCHAR) * 7);
    return S_OK;
}

HRESULT __stdcall filter_in_pin::QueryAccept(AM_MEDIA_TYPE const * /*pmt*/)
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
    if(!ppEnum) return E_POINTER;
    filter_in_enum_media_types *penum_media_types = new filter_in_enum_media_types;
    if(!penum_media_types) return E_OUTOFMEMORY;
    if(amt.cbFormat)
    {
        penum_media_types->amt.pbFormat = new BYTE[amt.cbFormat];
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
    penum_media_types->index = 0;
    *ppEnum = penum_media_types;
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
    return E_UNEXPECTED;
}

HRESULT __stdcall filter_in_pin::EndFlush()
{
#if write_level > 0
    print("filter_in_pin::EndFlush called...\n");
#endif
    return E_UNEXPECTED;
}

HRESULT __stdcall filter_in_pin::NewSegment(REFERENCE_TIME /*tStart*/, REFERENCE_TIME /*tStop*/, double /*dRate*/)
{
#if write_level > 0
    print("filter_in_pin::NewSegment called...\n");
#endif
    return E_UNEXPECTED;
}

HRESULT __stdcall filter_in_pin::RequestAllocator(IMemAllocator * /*pPreferred*/, ALLOCATOR_PROPERTIES * /*pProps*/, IMemAllocator ** /*ppActual*/)
{
#if write_level > 0
    print("filter_in_pin::RequestAllocator called...\n");
#endif
    return E_FAIL;
}

HRESULT __stdcall filter_in_pin::Request(IMediaSample * /*pSample*/, DWORD_PTR /*dwUser*/)
{
#if write_level > 0
    print("filter_in_pin::Request called...\n");
#endif
    return E_FAIL;
}

HRESULT __stdcall filter_in_pin::WaitForNext(DWORD /*dwTimeout*/, IMediaSample ** /*ppSample*/, DWORD_PTR * /*pdwUser*/)
{
#if write_level > 0
    print("filter_in_pin::WaitForNext called...\n");
#endif
    return E_FAIL;
}

HRESULT __stdcall filter_in_pin::SyncReadAligned(IMediaSample * /*pSample*/)
{
#if write_level > 0
    print("filter_in_pin::SyncReadAligned called...\n");
#endif
    return E_FAIL;
}

HRESULT __stdcall filter_in_pin::SyncRead(LONGLONG llPosition, LONG lLength, BYTE *pBuffer)
{
#if write_level > 0
    print("filter_in_pin::SyncRead(%I64i, %i, %p) called...\n", llPosition, lLength, pBuffer);
#endif
    if(!pBuffer) return E_POINTER;
    HRESULT r;
    EnterCriticalSection(&data_cs);
    if(data_l < llPosition)
    {
        r = S_FALSE;
    }
    else if(data_l < llPosition + lLength)
    {
        memcpy(pBuffer, (bits8 *)data_p + nat32(llPosition), data_l - nat32(llPosition));
        r = S_FALSE;
    }
    else
    {
        memcpy(pBuffer, (bits8 *)data_p + nat32(llPosition), lLength);
        r = S_OK;
    }
    LeaveCriticalSection(&data_cs);
    return r;
}

HRESULT __stdcall filter_in_pin::Length(LONGLONG *pTotal, LONGLONG *pAvailable)
{
#if write_level > 0
    print("filter_in_pin::Length called...\n");
#endif
    if(!pTotal || !pAvailable) return E_POINTER;
    EnterCriticalSection(&data_cs);
    *pTotal = 0x7fffffff;
    *pAvailable = data_l;
    LeaveCriticalSection(&data_cs);
    return VFW_S_ESTIMATED;
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
    print(", %p) called...\n", ppvObject);
#endif
    if(!ppvObject) return E_POINTER;
    if(riid == IID_IUnknown)
    {
        *(IUnknown **)ppvObject = this;
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
    *ppvObject = null;
    return E_NOINTERFACE;
}

ULONG __stdcall filter_in_filter::AddRef()
{
#if write_level > 1
    print("filter_in_filter::AddRef called...\n");
#endif
    return ++reference_count;
}

ULONG __stdcall filter_in_filter::Release()
{
#if write_level > 1
    print("filter_in_filter::Release called...\n");
#endif
    if(reference_count == 1)
    {
        if(pclock) pclock->Release();
        if(ppin->pconnected) ppin->pconnected->Release();
        if(ppin->data_a) delete[] (bits8 *)ppin->data_p;
        DeleteCriticalSection(&ppin->data_cs);
        if(ppin->amt.pUnk) ppin->amt.pUnk->Release();
        if(ppin->amt.cbFormat) delete[] ppin->amt.pbFormat;
        delete ppin;
        delete this;
        return 0;
    }
    return --reference_count;
}

HRESULT __stdcall filter_in_filter::GetClassID(CLSID * /*pClassID*/)
{
#if write_level > 0
    print("filter_in_filter::GetClassID called...\n");
#endif
    return E_FAIL;
}

HRESULT __stdcall filter_in_filter::Stop()
{
#if write_level > 0
    print("filter_in_filter::Stop called...\n");
#endif
    state = State_Stopped;
    return S_OK;
}

HRESULT __stdcall filter_in_filter::Pause()
{
#if write_level > 0
    print("filter_in_filter::Pause called...\n");
#endif
    state = State_Paused;
    return S_OK;
}

HRESULT __stdcall filter_in_filter::Run(REFERENCE_TIME /*tStart*/)
{
#if write_level > 0
    print("filter_in_filter::Run called...\n");
#endif
    state = State_Running;
    return S_OK;
}

HRESULT __stdcall filter_in_filter::GetState(DWORD /*dwMilliSecsTimeout*/, FILTER_STATE *State)
{
#if write_level > 0
    print("filter_in_filter::GetState called...\n");
#endif
    if(!State) return E_POINTER;
    if(state == State_Paused) return VFW_S_CANT_CUE;
    *State = state;
    return S_OK;
}

HRESULT __stdcall filter_in_filter::SetSyncSource(IReferenceClock *pClock)
{
#if write_level > 0
    print("filter_in_filter::SetSyncSource called...\n");
#endif
    if(pClock != pclock)
    {
        if(pclock) pclock->Release();
        pclock = pClock;
        if(pclock) pclock->AddRef();
    }
    return S_OK;
}

HRESULT __stdcall filter_in_filter::GetSyncSource(IReferenceClock **pClock)
{
#if write_level > 0
    print("filter_in_filter::GetSyncSource called...\n");
#endif
    if(!pClock) return E_POINTER;
    *pClock = pclock;
    if(*pClock) (*pClock)->AddRef();
    return S_OK;
}

HRESULT __stdcall filter_in_filter::EnumPins(IEnumPins **ppEnum)
{
#if write_level > 0
    print("filter_in_filter::EnumPins called...\n");
#endif
    if(!ppEnum) return E_POINTER;
    filter_in_enum_pins *penum_pins = new filter_in_enum_pins;
    if(!penum_pins) return E_OUTOFMEMORY;
    penum_pins->reference_count = 1;
    penum_pins->ppin = ppin;
    penum_pins->ppin->AddRef();
    penum_pins->index = 0;
    *ppEnum = penum_pins;
    return S_OK;
}

HRESULT __stdcall filter_in_filter::FindPin(LPCWSTR Id, IPin **ppPin)
{
#if write_level > 0
    print("filter_in_filter::FindPin called...\n");
#endif
    if(!ppPin) return E_POINTER;
    if(!wcscmp(Id, L"Output"))
    {
        *ppPin = ppin;
        (*ppPin)->AddRef();
        return S_OK;
    }
    *ppPin = null;
    return VFW_E_NOT_FOUND;
}

HRESULT __stdcall filter_in_filter::QueryFilterInfo(FILTER_INFO *pInfo)
{
#if write_level > 0
    print("filter_in_filter::QueryFilterInfo called...\n");
#endif
    if(!pInfo) return E_POINTER;
    wcscpy_s(pInfo->achName, MAX_FILTER_NAME, name);
    pInfo->pGraph = pgraph;
    if(pInfo->pGraph) pInfo->pGraph->AddRef();
    return S_OK;
}

HRESULT __stdcall filter_in_filter::JoinFilterGraph(IFilterGraph *pGraph, LPCWSTR pName)
{
#if write_level > 0
    print("filter_in_filter::JoinFilterGraph called...\n");
#endif
    pgraph = pGraph;
    if(pName) wcscpy_s(name, MAX_FILTER_NAME, pName);
    else wcscpy_s(name, MAX_FILTER_NAME, L"");
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

inline nat32 filter_in_filter::round(nat32 n)
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

bool filter_in_filter::data(nat32 len, void const *pdata)
{
    EnterCriticalSection(&ppin->data_cs);
    nat32 l2 = ppin->data_l + len;
    if(ppin->data_a < l2)
    {
        nat32 a = round(l2);
        void *p = new bits8[a];
        if(ppin->data_a)
        {
            if(ppin->data_l) memcpy(p, ppin->data_p, ppin->data_l);
            delete[] (bits8 *)ppin->data_p;
        }
        ppin->data_p = p;
        ppin->data_a = a;
    }
    memcpy((bits8 *)ppin->data_p + ppin->data_l, pdata, len);
    ppin->data_l = l2;
    LeaveCriticalSection(&ppin->data_cs);
    return true;
}

bool filter_in_filter::create(AM_MEDIA_TYPE const *pamt, filter_in_filter **ppfilter)
{
#if write_level > 1
    print("filter_in_filter::create called...\n");
#endif
    if(!pamt || !ppfilter) return false;
    filter_in_filter *pfilter = new filter_in_filter;
    if(!pfilter) return false;
    pfilter->ppin = new filter_in_pin;
    if(!pfilter->ppin)
    {
        delete pfilter;
        return false;
    }
    if(!InitializeCriticalSectionAndSpinCount(&pfilter->ppin->data_cs, 0x80000000))
    {
        delete pfilter->ppin;
        delete pfilter;
        return false;
    }
    if(pamt->cbFormat)
    {
        pfilter->ppin->amt.pbFormat = new BYTE[pamt->cbFormat];
        if(!pfilter->ppin->amt.pbFormat)
        {
            DeleteCriticalSection(&pfilter->ppin->data_cs);
            delete pfilter->ppin;
            delete pfilter;
            return false;
        }
        memcpy(pfilter->ppin->amt.pbFormat, pamt->pbFormat, pamt->cbFormat);
    }
    else pfilter->ppin->amt.pbFormat = null;
    pfilter->reference_count = 1;
    pfilter->pgraph = null;
    pfilter->pclock = null;
    wcscpy_s(pfilter->name, MAX_FILTER_NAME, L"");
    pfilter->state = State_Stopped;
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
    pfilter->ppin->data_l = 0;
    pfilter->ppin->data_a = 0;
    pfilter->ppin->pconnected = null;
    *ppfilter =  pfilter;
    return true;
}

//----------------------------------------------------------------------------
// filter_in
//----------------------------------------------------------------------------

bool filter_in::create(AM_MEDIA_TYPE const *pamt, filter_in **ppfilter)
{
    if(!pamt || !ppfilter) return false;
    filter_in_filter *pfilter;
    if(!filter_in_filter::create(pamt, &pfilter)) return false;
    *ppfilter = pfilter;
    return true;
}
