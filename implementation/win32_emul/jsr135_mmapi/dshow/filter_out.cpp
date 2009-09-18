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

#define write_level 1

#if write_level > 0
#include "writer.hpp"
#endif


const nat32 null = 0;


class filter_out_allocator;
class filter_out_pin;
class filter_out_filter;

class filter_out_sample : public IMediaSample
{
    friend filter_out_allocator;

    const static bits32 flag_time_set           = 0x01;
    const static bits32 flag_stop_time_set      = 0x02;
    const static bits32 flag_sync_point         = 0x04;
    const static bits32 flag_preroll            = 0x08;
    const static bits32 flag_media_type_changed = 0x10;
    const static bits32 flag_discontinuity      = 0x20;
    const static bits32 flag_media_time_set     = 0x40;

    nat32 reference_count;
    IMemAllocator *p_allocator;
    bits8 *p_data;
    int32 length;
    bits32 flags;
    int64 time_start;
    int64 time_end;
    int32 actual_length;
    AM_MEDIA_TYPE media_type;
    int64 media_time_start;
    int64 media_time_end;

    filter_out_sample();
    ~filter_out_sample();
public:
    // IUnknown
    virtual HRESULT __stdcall QueryInterface(REFIID riid, void **ppvObject);
    virtual ULONG __stdcall AddRef();
    virtual ULONG __stdcall Release();
    // IMediaSample
    virtual HRESULT __stdcall GetPointer(BYTE **ppBuffer);
    virtual long __stdcall GetSize();
    virtual HRESULT __stdcall GetTime(REFERENCE_TIME *pTimeStart, REFERENCE_TIME *pTimeEnd);
    virtual HRESULT __stdcall SetTime(REFERENCE_TIME *pTimeStart, REFERENCE_TIME *pTimeEnd);
    virtual HRESULT __stdcall IsSyncPoint();
    virtual HRESULT __stdcall SetSyncPoint(BOOL bIsSyncPoint);
    virtual HRESULT __stdcall IsPreroll();
    virtual HRESULT __stdcall SetPreroll(BOOL bIsPreroll);
    virtual long __stdcall GetActualDataLength();
    virtual HRESULT __stdcall SetActualDataLength(long lLen);
    virtual HRESULT __stdcall GetMediaType(AM_MEDIA_TYPE **ppMediaType);
    virtual HRESULT __stdcall SetMediaType(AM_MEDIA_TYPE *pMediaType);
    virtual HRESULT __stdcall IsDiscontinuity();
    virtual HRESULT __stdcall SetDiscontinuity(BOOL bDiscontinuity);
    virtual HRESULT __stdcall GetMediaTime(LONGLONG *pTimeStart, LONGLONG *pTimeEnd);
    virtual HRESULT __stdcall SetMediaTime(LONGLONG *pTimeStart, LONGLONG *pTimeEnd);
};

class filter_out_allocator : public IMemAllocator
{
    CRITICAL_SECTION cs_allocator;
    nat32 reference_count;
    ALLOCATOR_PROPERTIES ap;
    bool committed;
    nat32 free_samples;
    filter_out_sample **pp_samples;
    HANDLE event_get_buffer;

    // Possible allocator states:
    //
    // reference_count - references
    // committed == false
    // free_samples == ap.cBuffers
    // pp_samples - not used
    // event_get_buffer - set
    //
    // reference_count - references + 1
    // committed == false
    // free_samples < ap.cBuffers
    // pp_samples - not used
    // event_get_buffer - set
    //
    // reference_count - references
    // ap.cBuffers > 0 && ap.cbBuffer > 0
    // committed == true
    // free_samples == ap.cBuffers
    // pp_samples - used [ap.cBuffers]
    // event_get_buffer - set
    //
    // reference_count - references + 1
    // ap.cBuffers > 0 && ap.cbBuffer > 0
    // committed == true
    // free_samples > 0 && free_samples < ap.cBuffers
    // pp_samples - used [ap.cBuffers]
    // event_get_buffer - set
    //
    // reference_count - references + 1
    // ap.cBuffers > 0 && ap.cbBuffer > 0
    // committed == true
    // free_samples == 0
    // pp_samples - used [ap.cBuffers]
    // event_get_buffer - not set

    filter_out_allocator();
    ~filter_out_allocator();
public:
    // IUnknown
    virtual HRESULT __stdcall QueryInterface(REFIID riid, void **ppvObject);
    virtual ULONG __stdcall AddRef();
    virtual ULONG __stdcall Release();
    // IMemAllocator
    virtual HRESULT __stdcall SetProperties(ALLOCATOR_PROPERTIES *pRequest, ALLOCATOR_PROPERTIES *pActual);
    virtual HRESULT __stdcall GetProperties(ALLOCATOR_PROPERTIES *pProps);
    virtual HRESULT __stdcall Commit();
    virtual HRESULT __stdcall Decommit();
    virtual HRESULT __stdcall GetBuffer(IMediaSample **ppBuffer, REFERENCE_TIME *pStartTime, REFERENCE_TIME *pEndTime, DWORD dwFlags);
    virtual HRESULT __stdcall ReleaseBuffer(IMediaSample *pBuffer);

    static bool create(filter_out_allocator **pp_allocator);
};

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
    IMemAllocator *p_allocator;

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

class filter_out_filter : public filter_out, IAMFilterMiscFlags, IMediaSeeking
{
    friend filter_out_pin;

    CRITICAL_SECTION cs_filter;
    nat32 reference_count;
    player_callback *pcallback;
    filter_out_pin pin;
    IFilterGraph *pgraph;
    IReferenceClock *pclock;
    char16 name[MAX_FILTER_NAME];
    FILTER_STATE state;
    FILTER_STATE state2;
    int64 t_start;
    HANDLE event_state_set;
    HANDLE event_not_paused;

    // Possible filter states:
    //
    // state == State_Stopped
    // state2 == State_Stopped
    // event_state_set - set
    // event_not_paused - set
    //
    // state == State_Stopped
    // state2 == State_Paused
    // event_state_set - not set
    // event_not_paused - set
    // For video renderer only, waiting for single frame.
    //
    // state == State_Paused
    // state2 == State_Paused
    // event_state_set - set
    // event_not_paused - not set
    //
    // state == State_Running
    // state2 == State_Running
    // event_state_set - set
    // event_not_paused - set

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
    // IAMFilterMiscFlags
    virtual ULONG __stdcall GetMiscFlags();
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
// filter_out_sample
//----------------------------------------------------------------------------

filter_out_sample::filter_out_sample()
{
#if write_level > 1
    print("filter_out_sample::filter_out_sample called...\n");
#endif
}

filter_out_sample::~filter_out_sample()
{
#if write_level > 1
    print("filter_out_sample::~filter_out_sample called...\n");
#endif
}

HRESULT __stdcall filter_out_sample::QueryInterface(REFIID riid, void **ppvObject)
{
#if write_level > 0
    print("filter_out_sample::QueryInterface called...\n");
#endif
    if(!ppvObject) return E_POINTER;
    if(riid == IID_IUnknown)
    {
        *(IUnknown **)ppvObject = this;
        ((IUnknown *)*ppvObject)->AddRef();
        return S_OK;
    }
    if(riid == IID_IMediaSample)
    {
        *(IMediaSample **)ppvObject = this;
        ((IMediaSample *)*ppvObject)->AddRef();
        return S_OK;
    }
    *ppvObject = null;
    return E_NOINTERFACE;
}

ULONG __stdcall filter_out_sample::AddRef()
{
#if write_level > 1
    print("filter_out_sample::AddRef called...\n");
#endif
    return InterlockedIncrement((LONG *)&reference_count);
}

ULONG __stdcall filter_out_sample::Release()
{
#if write_level > 1
    print("filter_out_sample::Release called...\n");
#endif
    nat32 rc;
    if(reference_count == 1)
    {
        reference_count = 0;
        rc = 0;
    }
    else
    {
        rc = InterlockedDecrement((LONG *)&reference_count);
    }
    if(!rc)
    {
        if(flags & flag_media_type_changed)
        {
            if(media_type.cbFormat)
            {
                CoTaskMemFree(media_type.pbFormat);
            }
        }
        flags = 0;
        actual_length = 0;
        p_allocator->ReleaseBuffer(this);
    }
    return rc;
}

HRESULT __stdcall filter_out_sample::GetPointer(BYTE **ppBuffer)
{
#if write_level > 0
    print("filter_out_sample::GetPointer called...\n");
#endif
    if(!ppBuffer) return E_POINTER;
    *ppBuffer = p_data;
    return S_OK;
}

long __stdcall filter_out_sample::GetSize()
{
#if write_level > 0
    print("filter_out_sample::GetSize called...\n");
#endif
    return length;
}

HRESULT __stdcall filter_out_sample::GetTime(REFERENCE_TIME *pTimeStart, REFERENCE_TIME *pTimeEnd)
{
#if write_level > 0
    print("filter_out_sample::GetTime called...\n");
#endif
    if(!pTimeStart || !pTimeEnd) return E_POINTER;
    if(!(flags & flag_time_set)) return VFW_E_SAMPLE_TIME_NOT_SET;
    *pTimeStart = time_start;
    if(flags & flag_stop_time_set)
    {
        *pTimeEnd = time_end;
        return S_OK;
    }
    else
    {
        *pTimeEnd = time_start + 1;
        return VFW_S_NO_STOP_TIME;
    }
}

HRESULT __stdcall filter_out_sample::SetTime(REFERENCE_TIME *pTimeStart, REFERENCE_TIME *pTimeEnd)
{
#if write_level > 0
    print("filter_out_sample::SetTime called...\n");
#endif
    if(pTimeStart)
    {
        time_start = *pTimeStart;
        if(pTimeEnd)
        {
            flags |= flag_time_set | flag_stop_time_set;
            time_end = *pTimeEnd;
        }
        else
        {
            flags = (flags | flag_time_set) & ~flag_stop_time_set;
        }
        return S_OK;
    }
    else
    {
        if(pTimeEnd)
        {
            return E_INVALIDARG;
        }
        else
        {
            flags &= ~(flag_time_set | flag_stop_time_set);
            return S_OK;
        }
    }
}

HRESULT __stdcall filter_out_sample::IsSyncPoint()
{
#if write_level > 0
    print("filter_out_sample::IsSyncPoint called...\n");
#endif
    return flags & flag_sync_point ? S_OK : S_FALSE;
}

HRESULT __stdcall filter_out_sample::SetSyncPoint(BOOL bIsSyncPoint)
{
#if write_level > 0
    print("filter_out_sample::SetSyncPoint called...\n");
#endif
    flags = bIsSyncPoint ? flags | flag_sync_point : flags & ~flag_sync_point;
    return S_OK;
}

HRESULT __stdcall filter_out_sample::IsPreroll()
{
#if write_level > 0
    print("filter_out_sample::IsPreroll called...\n");
#endif
    return flags & flag_preroll ? S_OK : S_FALSE;
}

HRESULT __stdcall filter_out_sample::SetPreroll(BOOL bIsPreroll)
{
#if write_level > 0
    print("filter_out_sample::SetPreroll called...\n");
#endif
    flags = bIsPreroll ? flags | flag_preroll : flags & ~flag_preroll;
    return S_OK;
}

long __stdcall filter_out_sample::GetActualDataLength()
{
#if write_level > 0
    print("filter_out_sample::GetActualDataLength called...\n");
#endif
    return actual_length;
}

HRESULT __stdcall filter_out_sample::SetActualDataLength(long lLen)
{
#if write_level > 0
    print("filter_out_sample::SetActualDataLength called...\n");
#endif
    if(lLen > length) return VFW_E_BUFFER_OVERFLOW;
    actual_length = lLen;
    return S_OK;
}

HRESULT __stdcall filter_out_sample::GetMediaType(AM_MEDIA_TYPE **ppMediaType)
{
#if write_level > 0
    print("filter_out_sample::GetMediaType called...\n");
#endif
    if(!ppMediaType) return E_POINTER;
    if(!(flags & flag_media_type_changed))
    {
        *ppMediaType = null;
        return S_FALSE;
    }
    AM_MEDIA_TYPE *p_media_type = (AM_MEDIA_TYPE *)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
    if(!p_media_type)
    {
        *ppMediaType = null;
        return E_OUTOFMEMORY;
    }
    if(media_type.cbFormat)
    {
        p_media_type->pbFormat = (bits8 *)CoTaskMemAlloc(media_type.cbFormat);
        if(!p_media_type->pbFormat)
        {
            CoTaskMemFree(p_media_type);
            *ppMediaType = null;
            return E_OUTOFMEMORY;
        }
        memcpy(p_media_type->pbFormat, media_type.pbFormat, media_type.cbFormat);
    }
    else p_media_type->pbFormat = null;
    p_media_type->majortype = media_type.majortype;
    p_media_type->subtype = media_type.subtype;
    p_media_type->bFixedSizeSamples = media_type.bFixedSizeSamples;
    p_media_type->bTemporalCompression = media_type.bTemporalCompression;
    p_media_type->lSampleSize = media_type.lSampleSize;
    p_media_type->formattype = media_type.formattype;
    p_media_type->pUnk = media_type.pUnk;
    if(media_type.pUnk) p_media_type->pUnk->AddRef();
    p_media_type->cbFormat = media_type.cbFormat;
    *ppMediaType = p_media_type;
    return S_OK;
}

HRESULT __stdcall filter_out_sample::SetMediaType(AM_MEDIA_TYPE *pMediaType)
{
#if write_level > 0
    print("filter_out_sample::SetMediaType called...\n");
#endif
    if(pMediaType)
    {
        bits8 *p_format;
        if(pMediaType->cbFormat)
        {
            p_format = (bits8 *)CoTaskMemAlloc(pMediaType->cbFormat);
            if(!p_format)
            {
                return E_OUTOFMEMORY;
            }
            memcpy(p_format, pMediaType->pbFormat, pMediaType->cbFormat);
        }
        else
        {
            p_format = null;
        }
        if(flags & flag_media_type_changed)
        {
            if(media_type.cbFormat)
            {
                CoTaskMemFree(media_type.pbFormat);
            }
        }
        else
        {
            flags |= flag_media_type_changed;
        }
        media_type.majortype = pMediaType->majortype;
        media_type.subtype = pMediaType->subtype;
        media_type.bFixedSizeSamples = pMediaType->bFixedSizeSamples;
        media_type.bTemporalCompression = pMediaType->bTemporalCompression;
        media_type.lSampleSize = pMediaType->lSampleSize;
        media_type.formattype = pMediaType->formattype;
        media_type.pUnk = pMediaType->pUnk;
        if(pMediaType->pUnk) media_type.pUnk->AddRef();
        media_type.cbFormat = pMediaType->cbFormat;
        if(pMediaType->cbFormat) media_type.pbFormat = p_format;
    }
    else
    {
        if(flags & flag_media_type_changed)
        {
            if(media_type.cbFormat)
            {
                CoTaskMemFree(media_type.pbFormat);
            }
            flags &= ~flag_media_type_changed;
        }
    }
    return S_OK;
}

HRESULT __stdcall filter_out_sample::IsDiscontinuity()
{
#if write_level > 0
    print("filter_out_sample::IsDiscontinuity called...\n");
#endif
    return flags & flag_discontinuity ? S_OK : S_FALSE;
}

HRESULT __stdcall filter_out_sample::SetDiscontinuity(BOOL bDiscontinuity)
{
#if write_level > 0
    print("filter_out_sample::SetDiscontinuity called...\n");
#endif
    flags = bDiscontinuity ? flags | flag_discontinuity : flags & ~flag_discontinuity;
    return S_OK;
}

HRESULT __stdcall filter_out_sample::GetMediaTime(LONGLONG *pTimeStart, LONGLONG *pTimeEnd)
{
#if write_level > 0
    print("filter_out_sample::GetMediaTime called...\n");
#endif
    if(!pTimeStart || !pTimeEnd) return E_POINTER;
    if(!(flags & flag_media_time_set)) return VFW_E_MEDIA_TIME_NOT_SET;
    *pTimeStart = media_time_start;
    *pTimeEnd = media_time_end;
    return S_OK;
}

HRESULT __stdcall filter_out_sample::SetMediaTime(LONGLONG *pTimeStart, LONGLONG *pTimeEnd)
{
#if write_level > 0
    print("filter_out_sample::SetMediaTime called...\n");
#endif
    if(pTimeStart)
    {
        if(pTimeEnd)
        {
            flags |= flag_media_time_set;
            media_time_start = *pTimeStart;
            media_time_end = *pTimeEnd;
            return S_OK;
        }
        else
        {
            return E_POINTER;
        }
    }
    else
    {
        if(pTimeEnd)
        {
            return E_INVALIDARG;
        }
        else
        {
            flags &= ~flag_media_time_set;
            return S_OK;
        }
    }
}

//----------------------------------------------------------------------------
// filter_out_allocator
//----------------------------------------------------------------------------

filter_out_allocator::filter_out_allocator()
{
#if write_level > 1
    print("filter_out_allocator::filter_out_allocator called...\n");
#endif
}

filter_out_allocator::~filter_out_allocator()
{
#if write_level > 1
    print("filter_out_allocator::~filter_out_allocator called...\n");
#endif
}

HRESULT __stdcall filter_out_allocator::QueryInterface(REFIID riid, void **ppvObject)
{
#if write_level > 0
    print("filter_out_allocator::QueryInterface(");
    print(riid);
    print(", 0x%p) called...\n", ppvObject);
#endif
    if(!ppvObject) return E_POINTER;
    if(riid == IID_IUnknown)
    {
        *(IUnknown **)ppvObject = this;
        ((IUnknown *)*ppvObject)->AddRef();
        return S_OK;
    }
    if(riid == IID_IMemAllocator)
    {
        *(IMemAllocator **)ppvObject = this;
        ((IMemAllocator *)*ppvObject)->AddRef();
        return S_OK;
    }
    *ppvObject = null;
    return E_NOINTERFACE;
}

ULONG __stdcall filter_out_allocator::AddRef()
{
#if write_level > 1
    print("filter_out_allocator::AddRef called...\n");
#endif
    return InterlockedIncrement((LONG *)&reference_count);
}

ULONG __stdcall filter_out_allocator::Release()
{
#if write_level > 1
    print("filter_out_allocator::Release called...\n");
#endif
    nat32 rc;
    if(reference_count == 1)
    {
        reference_count = 0;
        rc = 0;
    }
    else
    {
        rc = InterlockedDecrement((LONG *)&reference_count);
    }
    if(!rc)
    {
        if(committed)
        {
            for(int32 i = 0; i < ap.cBuffers; i++)
            {
                delete[] pp_samples[ap.cBuffers - 1 - i]->p_data;
                delete pp_samples[ap.cBuffers - 1 - i];
            }
            delete[] pp_samples;
        }
        CloseHandle(event_get_buffer);
        DeleteCriticalSection(&cs_allocator);
        delete this;
    }
    return rc;
}

HRESULT __stdcall filter_out_allocator::SetProperties(ALLOCATOR_PROPERTIES *pRequest, ALLOCATOR_PROPERTIES *pActual)
{
#if write_level > 0
    print("filter_out_allocator::SetProperties called...\n");
#endif
    if(!pRequest || !pActual) return E_POINTER;
    EnterCriticalSection(&cs_allocator);
    if(committed)
    {
        LeaveCriticalSection(&cs_allocator);
        return VFW_E_ALREADY_COMMITTED;
    }
    if(ap.cBuffers != (int32)free_samples)
    {
        LeaveCriticalSection(&cs_allocator);
        return VFW_E_BUFFERS_OUTSTANDING;
    }
    ap.cBuffers = pRequest->cBuffers;
    ap.cbBuffer = pRequest->cbBuffer;
    ap.cbAlign = pRequest->cbAlign;
    ap.cbPrefix = pRequest->cbPrefix;
    free_samples = ap.cBuffers;
    pActual->cBuffers = ap.cBuffers;
    pActual->cbBuffer = ap.cbBuffer;
    pActual->cbAlign = ap.cbAlign;
    pActual->cbPrefix = ap.cbPrefix;
    LeaveCriticalSection(&cs_allocator);
    return S_OK;
}

HRESULT __stdcall filter_out_allocator::GetProperties(ALLOCATOR_PROPERTIES *pProps)
{
#if write_level > 0
    print("filter_out_allocator::GetProperties called...\n");
#endif
    if(!pProps) return E_POINTER;
    EnterCriticalSection(&cs_allocator);
    pProps->cBuffers = ap.cBuffers;
    pProps->cbBuffer = ap.cbBuffer;
    pProps->cbAlign = ap.cbAlign;
    pProps->cbPrefix = ap.cbPrefix;
    LeaveCriticalSection(&cs_allocator);
    return S_OK;
}

HRESULT __stdcall filter_out_allocator::Commit()
{
#if write_level > 0
    print("filter_out_allocator::Commit called...\n");
#endif
    EnterCriticalSection(&cs_allocator);
    if(!ap.cBuffers || !ap.cbBuffer)
    {
        LeaveCriticalSection(&cs_allocator);
        return VFW_E_SIZENOTSET;
    }
    if(committed)
    {
        LeaveCriticalSection(&cs_allocator);
        return S_OK;
    }
    pp_samples = new filter_out_sample *[ap.cBuffers];
    for(nat32 i = 0; i < free_samples; i++)
    {
        pp_samples[i] = new filter_out_sample;
        if(pp_samples[i])
        {
            pp_samples[i]->p_data = new bits8[ap.cbBuffer];
            if(pp_samples[i]->p_data)
            {
                pp_samples[i]->reference_count = 1;
                pp_samples[i]->p_allocator = this;
                pp_samples[i]->length = ap.cbBuffer;
                pp_samples[i]->flags = 0;
                pp_samples[i]->actual_length = 0;
                continue;
            }
            delete pp_samples[i];
        }
        for(nat32 j = 0; j < i; j++)
        {
            delete[] pp_samples[i - 1 - j]->p_data;
            delete pp_samples[i - 1 - j];
        }
        delete[] pp_samples;
        LeaveCriticalSection(&cs_allocator);
        return E_OUTOFMEMORY;
    }
    if(!free_samples)
    {
        if(!ResetEvent(event_get_buffer))
        {
            delete[] pp_samples;
            LeaveCriticalSection(&cs_allocator);
            return VFW_E_RUNTIME_ERROR;
        }
    }
    committed = true;
    LeaveCriticalSection(&cs_allocator);
    return S_OK;
}

HRESULT __stdcall filter_out_allocator::Decommit()
{
#if write_level > 0
    print("filter_out_allocator::Decommit called...\n");
#endif
    EnterCriticalSection(&cs_allocator);
    if(!committed)
    {
        LeaveCriticalSection(&cs_allocator);
        return S_OK;
    }
    if(!free_samples)
    {
        if(!SetEvent(event_get_buffer))
        {
            LeaveCriticalSection(&cs_allocator);
            return VFW_E_RUNTIME_ERROR;
        }
    }
    committed = false;
    for(nat32 i = 0; i < free_samples; i++)
    {
        delete[] pp_samples[free_samples - 1 - i]->p_data;
        delete pp_samples[free_samples - 1 - i];
    }
    delete[] pp_samples;
    LeaveCriticalSection(&cs_allocator);
    return S_OK;
}

HRESULT __stdcall filter_out_allocator::GetBuffer(IMediaSample **ppBuffer, REFERENCE_TIME *pStartTime, REFERENCE_TIME *pEndTime, DWORD dwFlags)
{
#if write_level > 0
    print("filter_out_allocator::GetBuffer called...\n");
#endif
    if(!ppBuffer) return E_POINTER;
    for(;;)
    {
        EnterCriticalSection(&cs_allocator);
        if(!committed)
        {
            LeaveCriticalSection(&cs_allocator);
            return VFW_E_NOT_COMMITTED;
        }
        if(free_samples)
        {
            if(free_samples == 1)
            {
                if(!ResetEvent(event_get_buffer))
                {
                    LeaveCriticalSection(&cs_allocator);
                    return VFW_E_RUNTIME_ERROR;
                }
            }
            filter_out_sample *p_sample = pp_samples[free_samples - 1];
            p_sample->reference_count = 1;
            *ppBuffer = p_sample;
            bool a = (int32)free_samples == ap.cBuffers;
            free_samples--;
            LeaveCriticalSection(&cs_allocator);
            if(a) AddRef();
            return S_OK;
        }
        LeaveCriticalSection(&cs_allocator);
        if(dwFlags & AM_GBF_NOWAIT) return VFW_E_TIMEOUT;
        nat32 r = WaitForSingleObject(event_get_buffer, INFINITE);
        if(r != WAIT_OBJECT_0)
        {
            return VFW_E_RUNTIME_ERROR;
        }
    }
}

HRESULT __stdcall filter_out_allocator::ReleaseBuffer(IMediaSample *pBuffer)
{
#if write_level > 0
    print("filter_out_allocator::ReleaseBuffer called...\n");
#endif
    if(!pBuffer) return E_POINTER;
    filter_out_sample *p_sample = (filter_out_sample *)pBuffer;
    EnterCriticalSection(&cs_allocator);
    if(committed)
    {
        if(!free_samples)
        {
            if(!SetEvent(event_get_buffer))
            {
                LeaveCriticalSection(&cs_allocator);
                return VFW_E_RUNTIME_ERROR;
            }
        }
        pp_samples[free_samples] = p_sample;
    }
    else
    {
        delete[] p_sample->p_data;
        delete p_sample;
    }
    free_samples++;
    bool r = (int32)free_samples == ap.cBuffers;
    LeaveCriticalSection(&cs_allocator);
    if(r) Release();
    return S_OK;
}

bool filter_out_allocator::create(filter_out_allocator **pp_allocator)
{
    if(!pp_allocator) return false;
    filter_out_allocator *p_allocator = new filter_out_allocator;
    if(!p_allocator) return false;
    if(!InitializeCriticalSectionAndSpinCount(&p_allocator->cs_allocator, 4000 | 0x80000000))
    {
        delete p_allocator;
        return false;
    }
    p_allocator->event_get_buffer = CreateEvent(null, true, true, null);
    if(!p_allocator->event_get_buffer)
    {
        DeleteCriticalSection(&p_allocator->cs_allocator);
        delete p_allocator;
        return false;
    }
    p_allocator->reference_count = 1;
    p_allocator->ap.cBuffers = 0;
    p_allocator->ap.cbBuffer = 0;
    p_allocator->ap.cbAlign = 0;
    p_allocator->ap.cbPrefix = 0;
    p_allocator->committed = false;
    p_allocator->free_samples = 0;
    *pp_allocator = p_allocator;
    return true;
}

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
    print("filter_out_pin::GetAllocator called...");
#endif
    if(!ppAllocator) return E_POINTER;
    return VFW_E_NO_ALLOCATOR;
    /**ppAllocator = p_allocator;
    (*ppAllocator)->AddRef();
#if write_level > 0
    print(" Returns 0x%p, 0x%x.\n", *ppAllocator, S_OK);
#endif
    return S_OK;*/
}

HRESULT __stdcall filter_out_pin::NotifyAllocator(IMemAllocator *pAllocator, BOOL /*bReadOnly*/)
{
#if write_level > 0
    print("filter_out_pin::NotifyAllocator(0x%p) called...\n", pAllocator);
#endif
    if(!pAllocator) return E_POINTER;
    p_allocator->Release();
    p_allocator = pAllocator;
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
        print("filter_out_pin::Receive returns VFW_E_RUNTIME_ERROR.\n");
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
            print("filter_out_pin::Receive returns S_FALSE.\n");
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
                print("filter_out_pin::Receive returns VFW_E_WRONG_STATE.\n");
#endif
                return VFW_E_WRONG_STATE;
            }
            else
            {
                pfilter->state = State_Paused;
                SetEvent(pfilter->event_state_set);
                ResetEvent(pfilter->event_not_paused);
                if(amt.majortype == MEDIATYPE_Video)
                {
                    bits8 *pb;
                    if(pSample->GetPointer(&pb) != S_OK)
                    {
                        LeaveCriticalSection(&pfilter->cs_filter);
                        LeaveCriticalSection(&cs_pin);
                        LeaveCriticalSection(&cs_receive);
#if write_level > 0
                        print("filter_out_pin::Receive returns VFW_E_RUNTIME_ERROR.\n");
#endif
                        return VFW_E_RUNTIME_ERROR;
                    }
                    pfilter->pcallback->frame_ready((bits16 *)pb);
                    LeaveCriticalSection(&pfilter->cs_filter);
                    LeaveCriticalSection(&cs_pin);
                    LeaveCriticalSection(&cs_receive);
#if write_level > 0
                    print("filter_out_pin::Receive returns S_OK.\n");
#endif
                    return S_OK;
                }
                LeaveCriticalSection(&pfilter->cs_filter);
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
                print("filter_out_pin::Receive returns VFW_E_RUNTIME_ERROR.\n");
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
                    print("filter_out_pin::Receive returns returns VFW_E_RUNTIME_ERROR.\n");
#endif
                    return VFW_E_RUNTIME_ERROR;
                }
                if(amt.majortype == MEDIATYPE_Audio)
                {
                    pfilter->pcallback->sample_ready(pSample->GetActualDataLength(), (bits16 *)pb);
                }
                else if(amt.majortype == MEDIATYPE_Video)
                {
                    pfilter->pcallback->frame_ready((bits16 *)pb);
                }
                LeaveCriticalSection(&pfilter->cs_filter);
                LeaveCriticalSection(&cs_pin);
                LeaveCriticalSection(&cs_receive);
#if write_level > 0
                print("filter_out_pin::Receive returns returns S_OK.\n");
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
    if(riid == IID_IAMFilterMiscFlags)
    {
        *(IAMFilterMiscFlags **)ppvObject = this;
        ((IAMFilterMiscFlags *)*ppvObject)->AddRef();
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

        DeleteCriticalSection(&pin.cs_receive);
        CloseHandle(pin.event_flushing);
        if(pin.pconnected)
        {
            if(pin.pconnected_media_seeking) pin.pconnected_media_seeking->Release();
            pin.pconnected->Release();
        }
        if(pin.amt.pUnk) pin.amt.pUnk->Release();
        if(pin.amt.cbFormat) delete[] pin.amt.pbFormat;
        DeleteCriticalSection(&pin.cs_pin);

        DeleteCriticalSection(&cs_filter);
        // delete ppin;
        delete this;
        return 0;
    }
    nat32 r = --reference_count;
    LeaveCriticalSection(&cs_filter);
    return r;
}

HRESULT __stdcall filter_out_filter::GetClassID(CLSID *pClassID)
{
#if write_level > 0
    print("filter_out_filter::GetClassID called...\n");
#endif
    if(!pClassID) return E_POINTER;
    // {681B4202-3E20-4955-9C0A-CA4EB274E431}
    const GUID guid =
    { 0x681b4202, 0x3e20, 0x4955, { 0x9c, 0x0a, 0xca, 0x4e, 0xb2, 0x74, 0xe4, 0x31 } };
    *pClassID = guid;
    return S_OK;
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
        if(state2 == State_Stopped)
        {
            state = State_Running;
            state2 = State_Running;
        }
        else
        {
            state = State_Running;
            state2 = State_Running;
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
    print("filter_out_filter::GetState(%u) called...", dwMilliSecsTimeout);
#endif
    if(!State)
    {
#if write_level > 0
        print(" Returns E_POINTER.\n");
#endif
        return E_POINTER;
    }
    EnterCriticalSection(&cs_filter);
    if(state2 != state)
    {
        nat32 r = WaitForSingleObject(event_state_set, dwMilliSecsTimeout);
        if(r == WAIT_OBJECT_0)
        {
        }
        else if(r == WAIT_TIMEOUT)
        {
            *State = state2;
            LeaveCriticalSection(&cs_filter);
#if write_level > 0
            print(" Returns %i, VFW_S_STATE_INTERMEDIATE.\n", *State);
#endif
            return VFW_S_STATE_INTERMEDIATE;
        }
        else
        {
            LeaveCriticalSection(&cs_filter);
#if write_level > 0
            print(" Returns VFW_E_RUNTIME_ERROR.\n");
#endif
            return VFW_E_RUNTIME_ERROR;
        }
    }
    *State = state2;
    LeaveCriticalSection(&cs_filter);
#if write_level > 0
    print(" Returns %i, S_OK.\n", *State);
#endif
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
    penum_pins->ppin = &pin;
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
        *ppPin = &pin;
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

ULONG __stdcall filter_out_filter::GetMiscFlags()
{
#if write_level > 0
    print("filter_out_filter::GetMiscFlags called, returns AM_FILTER_MISC_FLAGS_IS_RENDERER.\n");
#endif
    return AM_FILTER_MISC_FLAGS_IS_RENDERER;
}

HRESULT __stdcall filter_out_filter::GetCapabilities(DWORD *pCapabilities)
{
#if write_level > 0
    print("filter_out_filter::GetCapabilities called...\n");
#endif
    HRESULT r;
    // EnterCriticalSection(&pin.cs_pin);
    // EnterCriticalSection(&cs_filter);
    if(pin.pconnected)
    {
        if(pin.pconnected_media_seeking)
        {
            r = pin.pconnected_media_seeking->GetCapabilities(pCapabilities);
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
    // LeaveCriticalSection(&pin.cs_pin);
    return r;
}

HRESULT __stdcall filter_out_filter::CheckCapabilities(DWORD *pCapabilities)
{
#if write_level > 0
    print("filter_out_filter::CheckCapabilities called...\n");
#endif
    HRESULT r;
    // EnterCriticalSection(&pin.cs_pin);
    // EnterCriticalSection(&cs_filter);
    if(pin.pconnected)
    {
        if(pin.pconnected_media_seeking)
        {
            r = pin.pconnected_media_seeking->CheckCapabilities(pCapabilities);
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
    // LeaveCriticalSection(&pin.cs_pin);
    return r;
}

HRESULT __stdcall filter_out_filter::IsFormatSupported(const GUID *pFormat)
{
#if write_level > 0
    print("filter_out_filter::IsFormatSupported(");
    print(*pFormat);
    print(") called...");
#endif
    HRESULT r;
    // EnterCriticalSection(&pin.cs_pin);
    // EnterCriticalSection(&cs_filter);
    if(pin.pconnected)
    {
        if(pin.pconnected_media_seeking)
        {
            r = pin.pconnected_media_seeking->IsFormatSupported(pFormat);
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
    // LeaveCriticalSection(&pin.cs_pin);
#if write_level > 0
    print(" Returns 0x%x.\n", r);
#endif
    return r;
}

HRESULT __stdcall filter_out_filter::QueryPreferredFormat(GUID *pFormat)
{
#if write_level > 0
    print("filter_out_filter::QueryPreferredFormat called...\n");
#endif
    HRESULT r;
    // EnterCriticalSection(&pin.cs_pin);
    // EnterCriticalSection(&cs_filter);
    if(pin.pconnected)
    {
        if(pin.pconnected_media_seeking)
        {
            r = pin.pconnected_media_seeking->QueryPreferredFormat(pFormat);
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
    // LeaveCriticalSection(&pin.cs_pin);
    return r;
}

HRESULT __stdcall filter_out_filter::GetTimeFormat(GUID *pFormat)
{
#if write_level > 0
    print("filter_out_filter::GetTimeFormat called...\n");
#endif
    HRESULT r;
    // EnterCriticalSection(&pin.cs_pin);
    // EnterCriticalSection(&cs_filter);
    if(pin.pconnected)
    {
        if(pin.pconnected_media_seeking)
        {
            r = pin.pconnected_media_seeking->GetTimeFormat(pFormat);
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
    // LeaveCriticalSection(&pin.cs_pin);
    return r;
}

HRESULT __stdcall filter_out_filter::IsUsingTimeFormat(const GUID *pFormat)
{
#if write_level > 0
    print("filter_out_filter::IsUsingTimeFormat called...\n");
#endif
    HRESULT r;
    // EnterCriticalSection(&pin.cs_pin);
    // EnterCriticalSection(&cs_filter);
    if(pin.pconnected)
    {
        if(pin.pconnected_media_seeking)
        {
            r = pin.pconnected_media_seeking->IsUsingTimeFormat(pFormat);
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
    // LeaveCriticalSection(&pin.cs_pin);
    return r;
}

HRESULT __stdcall filter_out_filter::SetTimeFormat(const GUID *pFormat)
{
#if write_level > 0
    print("filter_out_filter::SetTimeFormat called...\n");
#endif
    HRESULT r;
    // EnterCriticalSection(&pin.cs_pin);
    // EnterCriticalSection(&cs_filter);
    if(pin.pconnected)
    {
        if(pin.pconnected_media_seeking)
        {
            r = pin.pconnected_media_seeking->SetTimeFormat(pFormat);
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
    // LeaveCriticalSection(&pin.cs_pin);
    return r;
}

HRESULT __stdcall filter_out_filter::GetDuration(LONGLONG *pDuration)
{
#if write_level > 0
    print("filter_out_filter::GetDuration called...\n");
#endif
    HRESULT r;
    // EnterCriticalSection(&pin.cs_pin);
    // EnterCriticalSection(&cs_filter);
    if(pin.pconnected)
    {
        if(pin.pconnected_media_seeking)
        {
            r = pin.pconnected_media_seeking->GetDuration(pDuration);
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
    // LeaveCriticalSection(&pin.cs_pin);
    return r;
}

HRESULT __stdcall filter_out_filter::GetStopPosition(LONGLONG *pStop)
{
#if write_level > 0
    print("filter_out_filter::GetStopPosition called...\n");
#endif
    HRESULT r;
    // EnterCriticalSection(&pin.cs_pin);
    // EnterCriticalSection(&cs_filter);
    if(pin.pconnected)
    {
        if(pin.pconnected_media_seeking)
        {
            r = pin.pconnected_media_seeking->GetStopPosition(pStop);
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
    // LeaveCriticalSection(&pin.cs_pin);
    return r;
}

HRESULT __stdcall filter_out_filter::GetCurrentPosition(LONGLONG *pCurrent)
{
#if write_level > 0
    print("filter_out_filter::GetCurrentPosition called...\n");
#endif
    HRESULT r;
    // EnterCriticalSection(&pin.cs_pin);
    // EnterCriticalSection(&cs_filter);
    if(pin.pconnected)
    {
        if(pin.pconnected_media_seeking)
        {
            r = pin.pconnected_media_seeking->GetCurrentPosition(pCurrent);
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
    // LeaveCriticalSection(&pin.cs_pin);
    return r;
}

HRESULT __stdcall filter_out_filter::ConvertTimeFormat(LONGLONG *pTarget, const GUID *pTargetFormat, LONGLONG Source, const GUID *pSourceFormat)
{
#if write_level > 0
    print("filter_out_filter::ConvertTimeFormat called...\n");
#endif
    HRESULT r;
    // EnterCriticalSection(&pin.cs_pin);
    // EnterCriticalSection(&cs_filter);
    if(pin.pconnected)
    {
        if(pin.pconnected_media_seeking)
        {
            r = pin.pconnected_media_seeking->ConvertTimeFormat(pTarget, pTargetFormat, Source, pSourceFormat);
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
    // LeaveCriticalSection(&pin.cs_pin);
    return r;
}

HRESULT __stdcall filter_out_filter::SetPositions(LONGLONG *pCurrent, DWORD dwCurrentFlags, LONGLONG *pStop, DWORD dwStopFlags)
{
#if write_level > 0
    print("filter_out_filter::SetPositions called...\n");
#endif
    HRESULT r;
    // EnterCriticalSection(&pin.cs_pin);
    // EnterCriticalSection(&cs_filter);
    if(pin.pconnected)
    {
        if(pin.pconnected_media_seeking)
        {
            r = pin.pconnected_media_seeking->SetPositions(pCurrent, dwCurrentFlags, pStop, dwStopFlags);
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
    // LeaveCriticalSection(&pin.cs_pin);
    return r;
}

HRESULT __stdcall filter_out_filter::GetPositions(LONGLONG *pCurrent, LONGLONG *pStop)
{
#if write_level > 0
    print("filter_out_filter::GetPositions called...\n");
#endif
    HRESULT r;
    // EnterCriticalSection(&pin.cs_pin);
    // EnterCriticalSection(&cs_filter);
    if(pin.pconnected)
    {
        if(pin.pconnected_media_seeking)
        {
            r = pin.pconnected_media_seeking->GetPositions(pCurrent, pStop);
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
    // LeaveCriticalSection(&pin.cs_pin);
    return r;
}

HRESULT __stdcall filter_out_filter::GetAvailable(LONGLONG *pEarliest, LONGLONG *pLatest)
{
#if write_level > 0
    print("filter_out_filter::GetAvailable called...\n");
#endif
    HRESULT r;
    // EnterCriticalSection(&pin.cs_pin);
    // EnterCriticalSection(&cs_filter);
    if(pin.pconnected)
    {
        if(pin.pconnected_media_seeking)
        {
            r = pin.pconnected_media_seeking->GetAvailable(pEarliest, pLatest);
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
    // LeaveCriticalSection(&pin.cs_pin);
    return r;
}

HRESULT __stdcall filter_out_filter::SetRate(double dRate)
{
#if write_level > 0
    print("filter_out_filter::SetRate called...\n");
#endif
    HRESULT r;
    // EnterCriticalSection(&pin.cs_pin);
    // EnterCriticalSection(&cs_filter);
    if(pin.pconnected)
    {
        if(pin.pconnected_media_seeking)
        {
            r = pin.pconnected_media_seeking->SetRate(dRate);
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
    // LeaveCriticalSection(&pin.cs_pin);
    return r;
}

HRESULT __stdcall filter_out_filter::GetRate(double *pdRate)
{
#if write_level > 0
    print("filter_out_filter::GetRate called...\n");
#endif
    HRESULT r;
    // EnterCriticalSection(&pin.cs_pin);
    // EnterCriticalSection(&cs_filter);
    if(pin.pconnected)
    {
        if(pin.pconnected_media_seeking)
        {
            r = pin.pconnected_media_seeking->GetRate(pdRate);
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
    // LeaveCriticalSection(&pin.cs_pin);
    return r;
}

HRESULT __stdcall filter_out_filter::GetPreroll(LONGLONG *pllPreroll)
{
#if write_level > 0
    print("filter_out_filter::GetPreroll called...\n");
#endif
    HRESULT r;
    // EnterCriticalSection(&pin.cs_pin);
    // EnterCriticalSection(&cs_filter);
    if(pin.pconnected)
    {
        if(pin.pconnected_media_seeking)
        {
            r = pin.pconnected_media_seeking->GetPreroll(pllPreroll);
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
    // LeaveCriticalSection(&pin.cs_pin);
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

    filter_out_allocator *p_allocator;
    if(!filter_out_allocator::create(&p_allocator))
    {
        delete pfilter;
        return false;
    }
    pfilter->pin.p_allocator = p_allocator;

    if(!InitializeCriticalSectionAndSpinCount(&pfilter->cs_filter, 0x80000000))
    {
        pfilter->pin.p_allocator->Release();
        delete pfilter;
        return false;
    }

    if(!InitializeCriticalSectionAndSpinCount(&pfilter->pin.cs_pin, 0x80000000))
    {
        DeleteCriticalSection(&pfilter->cs_filter);
        pfilter->pin.p_allocator->Release();
        delete pfilter;
        return false;
    }

    pfilter->pin.event_flushing = CreateEvent(null, true, false, null);
    if(!pfilter->pin.event_flushing)
    {
        DeleteCriticalSection(&pfilter->pin.cs_pin);
        DeleteCriticalSection(&pfilter->cs_filter);
        pfilter->pin.p_allocator->Release();
        delete pfilter;
        return false;
    }

    if(!InitializeCriticalSectionAndSpinCount(&pfilter->pin.cs_receive, 0x80000000))
    {
        CloseHandle(pfilter->pin.event_flushing);
        DeleteCriticalSection(&pfilter->pin.cs_pin);
        DeleteCriticalSection(&pfilter->cs_filter);
        pfilter->pin.p_allocator->Release();
        delete pfilter;
        return false;
    }

    pfilter->event_state_set = CreateEvent(null, true, true, null);
    if(!pfilter->event_state_set)
    {
        DeleteCriticalSection(&pfilter->pin.cs_receive);
        CloseHandle(pfilter->pin.event_flushing);
        DeleteCriticalSection(&pfilter->pin.cs_pin);
        DeleteCriticalSection(&pfilter->cs_filter);
        pfilter->pin.p_allocator->Release();
        delete pfilter;
        return false;
    }

    pfilter->event_not_paused = CreateEvent(null, true, true, null);
    if(!pfilter->event_not_paused)
    {
        CloseHandle(pfilter->event_state_set);
        DeleteCriticalSection(&pfilter->pin.cs_receive);
        CloseHandle(pfilter->pin.event_flushing);
        DeleteCriticalSection(&pfilter->pin.cs_pin);
        DeleteCriticalSection(&pfilter->cs_filter);
        pfilter->pin.p_allocator->Release();
        delete pfilter;
        return false;
    }

    if(pamt->cbFormat)
    {
        pfilter->pin.amt.pbFormat = new bits8[pamt->cbFormat];
        if(!pfilter->pin.amt.pbFormat)
        {
            CloseHandle(pfilter->event_state_set);
            DeleteCriticalSection(&pfilter->pin.cs_receive);
            CloseHandle(pfilter->pin.event_flushing);
            DeleteCriticalSection(&pfilter->pin.cs_pin);
            DeleteCriticalSection(&pfilter->cs_filter);
            pfilter->pin.p_allocator->Release();
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
    pfilter->pin.pconnected = null;
    pfilter->pin.flushing = false;
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
