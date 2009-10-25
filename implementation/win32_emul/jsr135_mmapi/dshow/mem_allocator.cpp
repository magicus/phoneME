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

#include <strmif.h>
#include <vfwmsgs.h>
#include "types.hpp"

#define write_level 0

#if write_level > 0
#include "writer.hpp"
#endif


const nat32 null = 0;


class mem_allocator;

class media_sample : public IMediaSample
{
    friend mem_allocator;

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

    media_sample();
    ~media_sample();
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

class mem_allocator : public IMemAllocator
{
    CRITICAL_SECTION cs_allocator;
    nat32 reference_count;
    ALLOCATOR_PROPERTIES ap;
    bool committed;
    nat32 free_samples;
    media_sample **pp_samples;
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

    mem_allocator();
    ~mem_allocator();
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

    static bool create(mem_allocator **pp_allocator);
};

//----------------------------------------------------------------------------
// media_sample
//----------------------------------------------------------------------------

media_sample::media_sample()
{
#if write_level > 2
    print("media_sample::media_sample called...\n");
#endif
}

media_sample::~media_sample()
{
#if write_level > 2
    print("media_sample::~media_sample called...\n");
#endif
}

HRESULT __stdcall media_sample::QueryInterface(REFIID riid, void **ppvObject)
{
#if write_level > 1
    print("media_sample::QueryInterface called...\n");
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

ULONG __stdcall media_sample::AddRef()
{
#if write_level > 2
    print("media_sample::AddRef called...\n");
#endif
    return InterlockedIncrement((LONG *)&reference_count);
}

ULONG __stdcall media_sample::Release()
{
#if write_level > 2
    print("media_sample::Release called...\n");
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

HRESULT __stdcall media_sample::GetPointer(BYTE **ppBuffer)
{
#if write_level > 1
    print("media_sample::GetPointer called...\n");
#endif
    if(!ppBuffer) return E_POINTER;
    *ppBuffer = p_data;
    return S_OK;
}

long __stdcall media_sample::GetSize()
{
#if write_level > 1
    print("media_sample::GetSize called...\n");
#endif
    return length;
}

HRESULT __stdcall media_sample::GetTime(REFERENCE_TIME *pTimeStart, REFERENCE_TIME *pTimeEnd)
{
#if write_level > 1
    print("media_sample::GetTime called...\n");
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

HRESULT __stdcall media_sample::SetTime(REFERENCE_TIME *pTimeStart, REFERENCE_TIME *pTimeEnd)
{
#if write_level > 1
    print("media_sample::SetTime called...\n");
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

HRESULT __stdcall media_sample::IsSyncPoint()
{
#if write_level > 1
    print("media_sample::IsSyncPoint called...\n");
#endif
    return flags & flag_sync_point ? S_OK : S_FALSE;
}

HRESULT __stdcall media_sample::SetSyncPoint(BOOL bIsSyncPoint)
{
#if write_level > 1
    print("media_sample::SetSyncPoint called...\n");
#endif
    flags = bIsSyncPoint ? flags | flag_sync_point : flags & ~flag_sync_point;
    return S_OK;
}

HRESULT __stdcall media_sample::IsPreroll()
{
#if write_level > 1
    print("media_sample::IsPreroll called...\n");
#endif
    return flags & flag_preroll ? S_OK : S_FALSE;
}

HRESULT __stdcall media_sample::SetPreroll(BOOL bIsPreroll)
{
#if write_level > 1
    print("media_sample::SetPreroll called...\n");
#endif
    flags = bIsPreroll ? flags | flag_preroll : flags & ~flag_preroll;
    return S_OK;
}

long __stdcall media_sample::GetActualDataLength()
{
#if write_level > 1
    print("media_sample::GetActualDataLength called...\n");
#endif
    return actual_length;
}

HRESULT __stdcall media_sample::SetActualDataLength(long lLen)
{
#if write_level > 1
    print("media_sample::SetActualDataLength called...\n");
#endif
    if(lLen > length) return VFW_E_BUFFER_OVERFLOW;
    actual_length = lLen;
    return S_OK;
}

HRESULT __stdcall media_sample::GetMediaType(AM_MEDIA_TYPE **ppMediaType)
{
#if write_level > 1
    print("media_sample::GetMediaType called...\n");
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

HRESULT __stdcall media_sample::SetMediaType(AM_MEDIA_TYPE *pMediaType)
{
#if write_level > 1
    print("media_sample::SetMediaType called...\n");
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

HRESULT __stdcall media_sample::IsDiscontinuity()
{
#if write_level > 1
    print("media_sample::IsDiscontinuity called...\n");
#endif
    return flags & flag_discontinuity ? S_OK : S_FALSE;
}

HRESULT __stdcall media_sample::SetDiscontinuity(BOOL bDiscontinuity)
{
#if write_level > 1
    print("media_sample::SetDiscontinuity called...\n");
#endif
    flags = bDiscontinuity ? flags | flag_discontinuity : flags & ~flag_discontinuity;
    return S_OK;
}

HRESULT __stdcall media_sample::GetMediaTime(LONGLONG *pTimeStart, LONGLONG *pTimeEnd)
{
#if write_level > 1
    print("media_sample::GetMediaTime called...\n");
#endif
    if(!pTimeStart || !pTimeEnd) return E_POINTER;
    if(!(flags & flag_media_time_set)) return VFW_E_MEDIA_TIME_NOT_SET;
    *pTimeStart = media_time_start;
    *pTimeEnd = media_time_end;
    return S_OK;
}

HRESULT __stdcall media_sample::SetMediaTime(LONGLONG *pTimeStart, LONGLONG *pTimeEnd)
{
#if write_level > 1
    print("media_sample::SetMediaTime called...\n");
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
// mem_allocator
//----------------------------------------------------------------------------

mem_allocator::mem_allocator()
{
#if write_level > 2
    print("mem_allocator::mem_allocator called...\n");
#endif
}

mem_allocator::~mem_allocator()
{
#if write_level > 2
    print("mem_allocator::~mem_allocator called...\n");
#endif
}

HRESULT __stdcall mem_allocator::QueryInterface(REFIID riid, void **ppvObject)
{
#if write_level > 1
    print("mem_allocator::QueryInterface(");
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

ULONG __stdcall mem_allocator::AddRef()
{
#if write_level > 2
    print("mem_allocator::AddRef called...\n");
#endif
    return InterlockedIncrement((LONG *)&reference_count);
}

ULONG __stdcall mem_allocator::Release()
{
#if write_level > 2
    print("mem_allocator::Release called...\n");
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

HRESULT __stdcall mem_allocator::SetProperties(ALLOCATOR_PROPERTIES *pRequest, ALLOCATOR_PROPERTIES *pActual)
{
#if write_level > 1
    print("mem_allocator::SetProperties called...\n");
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

HRESULT __stdcall mem_allocator::GetProperties(ALLOCATOR_PROPERTIES *pProps)
{
#if write_level > 1
    print("mem_allocator::GetProperties called...\n");
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

HRESULT __stdcall mem_allocator::Commit()
{
#if write_level > 1
    print("mem_allocator::Commit called...\n");
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
    pp_samples = new media_sample *[ap.cBuffers];
    for(nat32 i = 0; i < free_samples; i++)
    {
        pp_samples[i] = new media_sample;
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

HRESULT __stdcall mem_allocator::Decommit()
{
#if write_level > 1
    print("mem_allocator::Decommit called...\n");
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

HRESULT __stdcall mem_allocator::GetBuffer(IMediaSample **ppBuffer, REFERENCE_TIME *pStartTime, REFERENCE_TIME *pEndTime, DWORD dwFlags)
{
#if write_level > 1
    print("mem_allocator::GetBuffer called...\n");
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
            media_sample *p_sample = pp_samples[free_samples - 1];
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

HRESULT __stdcall mem_allocator::ReleaseBuffer(IMediaSample *pBuffer)
{
#if write_level > 1
    print("mem_allocator::ReleaseBuffer called...\n");
#endif
    if(!pBuffer) return E_POINTER;
    media_sample *p_sample = (media_sample *)pBuffer;
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

bool mem_allocator::create(mem_allocator **pp_allocator)
{
    if(!pp_allocator) return false;
    mem_allocator *p_allocator = new mem_allocator;
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
