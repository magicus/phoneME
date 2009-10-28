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
#include <evcode.h>
#include <process.h>
#include <uuids.h>
#include <vfwmsgs.h>
#include "enum_media_types_single.hpp"
#include "enum_pins_single.hpp"
#include "player_callback.hpp"

#define write_level 0

#if write_level > 0
#include "writer.hpp"
#endif


const nat32 null = 0;


class filter_out_filter;

class filter_out_pin : public IPin, IMemInputPin
{
    friend filter_out_filter;

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

    friend bool create_filter_out(const AM_MEDIA_TYPE *pamt, player_callback *pcallback, IBaseFilter **ppfilter);
};

class filter_out_filter : public IBaseFilter, IAMFilterMiscFlags, IMediaSeeking, IReferenceClock
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
    int64 t_last;
    HANDLE event_state_set;
    HANDLE event_not_paused;
    HANDLE thread_worker;
    int64 perf_freq;
    int64 t_perf_prev;
    int64 t_prev;

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
    // IReferenceClock
    virtual HRESULT __stdcall GetTime(REFERENCE_TIME *pTime);
    virtual HRESULT __stdcall AdviseTime(REFERENCE_TIME baseTime, REFERENCE_TIME streamTime, HEVENT hEvent, DWORD_PTR *pdwAdviseCookie);
    virtual HRESULT __stdcall AdvisePeriodic(REFERENCE_TIME startTime, REFERENCE_TIME periodTime, HSEMAPHORE hSemaphore, DWORD_PTR *pdwAdviseCookie);
    virtual HRESULT __stdcall Unadvise(DWORD_PTR dwAdviseCookie);

    static nat32 __stdcall worker_thread(void *param);

    friend bool create_filter_out(const AM_MEDIA_TYPE *pamt, player_callback *pcallback, IBaseFilter **ppfilter);
};

//----------------------------------------------------------------------------
// filter_out_pin
//----------------------------------------------------------------------------

filter_out_pin::filter_out_pin()
{
#if write_level > 2
    print("filter_out_pin::filter_out_pin called...\n");
#endif
}

filter_out_pin::~filter_out_pin()
{
#if write_level > 2
    print("filter_out_pin::~filter_out_pin called...\n");
#endif
}

HRESULT __stdcall filter_out_pin::QueryInterface(REFIID riid, void **ppvObject)
{
#if write_level > 1
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
#if write_level > 2
    print("filter_out_pin::AddRef called...\n");
#endif
    return pfilter->AddRef();
}

ULONG __stdcall filter_out_pin::Release()
{
#if write_level > 2
    print("filter_out_pin::Release called...\n");
#endif
    return pfilter->Release();
}

HRESULT __stdcall filter_out_pin::Connect(IPin *pReceivePin, const AM_MEDIA_TYPE * /*pmt*/)
{
#if write_level > 1
    print("filter_out_pin::Connect called...\n");
#endif
    if(!pReceivePin) return E_POINTER;
    return E_UNEXPECTED;
}

HRESULT __stdcall filter_out_pin::ReceiveConnection(IPin *pConnector, const AM_MEDIA_TYPE *pmt)
{
#if write_level > 1
    print("filter_out_pin::ReceiveConnection(0x%p", pConnector);
    print(", ");
    dump_media_type(pmt);
    print(") called...\n");
#endif
    if(!pConnector || !pmt) return E_POINTER;
    EnterCriticalSection(&pfilter->cs_filter);
    if(pfilter->state != State_Stopped)
    {
        LeaveCriticalSection(&pfilter->cs_filter);
        return VFW_E_NOT_STOPPED;
    }
    if(pconnected)
    {
        LeaveCriticalSection(&pfilter->cs_filter);
        return VFW_E_ALREADY_CONNECTED;
    }
    if(pmt->majortype != amt.majortype ||
        pmt->subtype != amt.subtype)
    {
        LeaveCriticalSection(&pfilter->cs_filter);
        return VFW_E_TYPE_NOT_ACCEPTED;
    }

    if(pmt->majortype == MEDIATYPE_Audio)
    {
        if(pmt->formattype == FORMAT_WaveFormatEx && pmt->cbFormat >= sizeof(WAVEFORMATEX))
        {
            const WAVEFORMATEX *pwfe = (const WAVEFORMATEX *)pmt->pbFormat;
            if(pwfe->wFormatTag == WAVE_FORMAT_PCM)
            {
#if write_level > 1
                print("Audio format - %u %u %u\n", pwfe->nSamplesPerSec, pwfe->nChannels, pwfe->wBitsPerSample);
#endif
                pfilter->pcallback->audio_format_changed(pwfe->nSamplesPerSec, pwfe->nChannels, pwfe->wBitsPerSample);
            }
            else
            {
                LeaveCriticalSection(&pfilter->cs_filter);
                return VFW_E_TYPE_NOT_ACCEPTED;
            }
        }
        else
        {
            LeaveCriticalSection(&pfilter->cs_filter);
            return VFW_E_TYPE_NOT_ACCEPTED;
        }
    }
    else if(pmt->majortype == MEDIATYPE_Video && pmt->cbFormat >= sizeof(VIDEOINFOHEADER))
    {
        if(pmt->formattype == FORMAT_VideoInfo)
        {
            const VIDEOINFOHEADER *vih = (const VIDEOINFOHEADER *)pmt->pbFormat;
#if write_level > 1
            print("Frame size - %i %i\n", vih->bmiHeader.biWidth, vih->bmiHeader.biHeight);
#endif
            pfilter->pcallback->size_changed(int16(vih->bmiHeader.biWidth), int16(vih->bmiHeader.biHeight));
        }
        else if(pmt->formattype == FORMAT_VideoInfo2 && pmt->cbFormat >= sizeof(VIDEOINFOHEADER2))
        {
            const VIDEOINFOHEADER2 *vih2 = (const VIDEOINFOHEADER2 *)pmt->pbFormat;
#if write_level > 1
            print("Frame size - %i %i\n", vih2->bmiHeader.biWidth, vih2->bmiHeader.biHeight);
#endif
            pfilter->pcallback->size_changed(int16(vih2->bmiHeader.biWidth), int16(vih2->bmiHeader.biHeight));
        }
        else
        {
            LeaveCriticalSection(&pfilter->cs_filter);
            return VFW_E_TYPE_NOT_ACCEPTED;
        }
    }

    pconnected = pConnector;
    pconnected->AddRef();
    if(pconnected->QueryInterface(IID_IMediaSeeking, (void **)&pconnected_media_seeking) != S_OK) pconnected_media_seeking = null;
    LeaveCriticalSection(&pfilter->cs_filter);
#if write_level > 1
        print("Connected.\n");
#endif
    return S_OK;
}

HRESULT __stdcall filter_out_pin::Disconnect()
{
#if write_level > 1
    print("filter_out_pin::Disconnect called...\n");
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
    if(pconnected_media_seeking) pconnected_media_seeking->Release();
    pconnected->Release();
    pconnected = null;
    LeaveCriticalSection(&pfilter->cs_filter);
    return S_OK;
}

HRESULT __stdcall filter_out_pin::ConnectedTo(IPin **pPin)
{
#if write_level > 1
    print("filter_out_pin::ConnectedTo called...\n");
#endif
    if(!pPin) return E_POINTER;
    EnterCriticalSection(&pfilter->cs_filter);
    if(!pconnected)
    {
        LeaveCriticalSection(&pfilter->cs_filter);
        *pPin = null;
        return VFW_E_NOT_CONNECTED;
    }
    *pPin = pconnected;
    (*pPin)->AddRef();
    LeaveCriticalSection(&pfilter->cs_filter);
    return S_OK;
}

HRESULT __stdcall filter_out_pin::ConnectionMediaType(AM_MEDIA_TYPE *pmt)
{
#if write_level > 1
    print("filter_out_pin::ConnectionMediaType called...\n");
#endif
    if(!pmt) return E_POINTER;
    EnterCriticalSection(&pfilter->cs_filter);
    if(!pconnected)
    {
        LeaveCriticalSection(&pfilter->cs_filter);
        memset(pmt, 0, sizeof(AM_MEDIA_TYPE));
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

HRESULT __stdcall filter_out_pin::QueryPinInfo(PIN_INFO *pInfo)
{
#if write_level > 1
    print("filter_out_pin::QueryPinInfo called...\n");
#endif
    if(!pInfo) return E_POINTER;
    EnterCriticalSection(&pfilter->cs_filter);
    pInfo->pFilter = pfilter;
    LeaveCriticalSection(&pfilter->cs_filter);
    pInfo->pFilter->AddRef();
    pInfo->dir = PINDIR_INPUT;
    wcscpy_s(pInfo->achName, MAX_PIN_NAME, L"Input");
    return S_OK;
}

HRESULT __stdcall filter_out_pin::QueryDirection(PIN_DIRECTION *pPinDir)
{
#if write_level > 1
    print("filter_out_pin::QueryDirection called...\n");
#endif
    if(!pPinDir) return E_POINTER;
    *pPinDir = PINDIR_INPUT;
    return S_OK;
}

HRESULT __stdcall filter_out_pin::QueryId(LPWSTR *Id)
{
#if write_level > 1
    print("filter_out_pin::QueryId called...\n");
#endif
    if(!Id) return E_POINTER;
    *Id = (WCHAR *)CoTaskMemAlloc(sizeof(WCHAR) * 6);
    if(!*Id) return E_OUTOFMEMORY;
    memcpy(*Id, L"Input", sizeof(WCHAR) * 6);
    return S_OK;
}

HRESULT __stdcall filter_out_pin::QueryAccept(const AM_MEDIA_TYPE * /*pmt*/)
{
#if write_level > 1
    print("filter_out_pin::QueryAccept called...\n");
#endif
    return S_OK;
}

HRESULT __stdcall filter_out_pin::EnumMediaTypes(IEnumMediaTypes **ppEnum)
{
#if write_level > 1
    print("filter_out_pin::EnumMediaTypes called...\n");
#endif
    if(!ppEnum) return E_POINTER;
    EnterCriticalSection(&pfilter->cs_filter);
    HRESULT r = create_enum_media_types_single(&amt, ppEnum) ? S_OK : E_FAIL;
    LeaveCriticalSection(&pfilter->cs_filter);
    return r;
}

HRESULT __stdcall filter_out_pin::QueryInternalConnections(IPin **apPin, ULONG *nPin)
{
#if write_level > 1
    print("filter_out_pin::QueryInternalConnections called...\n");
#endif
    if(!apPin || !nPin) return E_POINTER;
    return E_NOTIMPL;
}

HRESULT __stdcall filter_out_pin::EndOfStream()
{
#if write_level > 1
    print("filter_out_pin::EndOfStream called...\n");
#endif
    EnterCriticalSection(&pfilter->cs_filter);
    pfilter->pcallback->playback_finished();
    LeaveCriticalSection(&pfilter->cs_filter);
    return S_OK;
}

HRESULT __stdcall filter_out_pin::BeginFlush()
{
#if write_level > 1
    print("filter_out_pin::BeginFlush called...\n");
#endif
    EnterCriticalSection(&pfilter->cs_filter);
    if(flushing)
    {
        LeaveCriticalSection(&pfilter->cs_filter);
    }
    else
    {
        flushing = true;
        SetEvent(event_flushing);
        LeaveCriticalSection(&pfilter->cs_filter);
        EnterCriticalSection(&cs_receive);
        LeaveCriticalSection(&cs_receive);
    }
    return S_OK;
}

HRESULT __stdcall filter_out_pin::EndFlush()
{
#if write_level > 1
    print("filter_out_pin::EndFlush called...\n");
#endif
    EnterCriticalSection(&pfilter->cs_filter);
    flushing = false;
    LeaveCriticalSection(&pfilter->cs_filter);
    return S_OK;
}

#if write_level > 1
HRESULT __stdcall filter_out_pin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
    print("filter_out_pin::NewSegment(%I64i, %I64i, %f) called...\n", tStart, tStop, dRate);
#else
HRESULT __stdcall filter_out_pin::NewSegment(REFERENCE_TIME /*tStart*/, REFERENCE_TIME /*tStop*/, double /*dRate*/)
{
#endif
    EnterCriticalSection(&pfilter->cs_filter);
    LeaveCriticalSection(&pfilter->cs_filter);
    return S_OK;
}

HRESULT __stdcall filter_out_pin::GetAllocator(IMemAllocator **ppAllocator)
{
#if write_level > 1
    print("filter_out_pin::GetAllocator called...");
#endif
    if(!ppAllocator) return E_POINTER;
    return VFW_E_NO_ALLOCATOR;
    /**ppAllocator = p_allocator;
    (*ppAllocator)->AddRef();
#if write_level > 1
    print(" Returns 0x%p, 0x%x.\n", *ppAllocator, S_OK);
#endif
    return S_OK;*/
}

HRESULT __stdcall filter_out_pin::NotifyAllocator(IMemAllocator *pAllocator, BOOL /*bReadOnly*/)
{
#if write_level > 1
    print("filter_out_pin::NotifyAllocator(0x%p) called...\n", pAllocator);
#endif
    if(!pAllocator) return E_POINTER;
    if(p_allocator) p_allocator->Release();
    p_allocator = pAllocator;
    return S_OK;
}

HRESULT __stdcall filter_out_pin::GetAllocatorRequirements(ALLOCATOR_PROPERTIES *pProps)
{
#if write_level > 1
    print("filter_out_pin::GetAllocatorRequirements called...\n");
#endif
    if(!pProps) return E_POINTER;
    return E_NOTIMPL;
}

HRESULT __stdcall filter_out_pin::Receive(IMediaSample *pSample)
{
#if write_level > 1
    print("filter_out_pin::Receive called...\n");
#endif
    if(!pSample)
    {
#if write_level > 1
        print("filter_out_pin::Receive returns E_POINTER.\n");
#endif
        return E_POINTER;
    }
#if write_level > 1
    print("Actual data length=%i\n", pSample->GetActualDataLength());
#endif
    int64 tstart;
    int64 tend;
#if write_level > 1
    if(pSample->GetMediaTime(&tstart, &tend) == S_OK)
    {
        print("Start media time=%I64x, end media time=%I64x\n", tstart, tend);
    }
#endif
    HRESULT r = pSample->GetTime(&tstart, &tend);
    if(r != S_OK && r != VFW_S_NO_STOP_TIME)
    {
#if write_level > 1
        print("filter_out_pin::Receive returns VFW_E_RUNTIME_ERROR.\n");
#endif
        return VFW_E_RUNTIME_ERROR;
    }
#if write_level > 0
    if(amt.majortype == MEDIATYPE_Audio)
    {
        print("Audio: Start time=%I64i, end time=%I64i\n", tstart, tend);
    }
    else if(amt.majortype == MEDIATYPE_Video)
    {
        print("Video: Start time=%I64i, end time=%I64i\n", tstart, tend);
    }
    else
    {
        print("Start time=%I64i, end time=%I64i\n", tstart, tend);
    }
#endif
#if write_level > 1
    int64 t;
    r = pfilter->pclock->GetTime(&t);
    if(r == S_OK || r == S_FALSE)
    {
        print("Reference time=%I64i, stream time=%I64i\n", t, t - pfilter->t_start);
    }
#endif
    bool delivered = false;
    EnterCriticalSection(&cs_receive);
    EnterCriticalSection(&pfilter->cs_filter);
    pfilter->t_last = tend;
    for(;;)
    {
        if(flushing)
        {
            LeaveCriticalSection(&pfilter->cs_filter);
            LeaveCriticalSection(&cs_receive);
#if write_level > 1
            print("filter_out_pin::Receive returns S_FALSE.\n");
#endif
            return S_FALSE;
        }
        if(pfilter->state == State_Stopped)
        {
            if(pfilter->state2 == State_Stopped)
            {
                LeaveCriticalSection(&pfilter->cs_filter);
                LeaveCriticalSection(&cs_receive);
#if write_level > 1
                print("filter_out_pin::Receive returns VFW_E_WRONG_STATE.\n");
#endif
                return VFW_E_WRONG_STATE;
            }
            else
            {
                pfilter->state = State_Paused;
                SetEvent(pfilter->event_state_set);
                ResetEvent(pfilter->event_not_paused);
            }
        }
        else if(pfilter->state == State_Paused)
        {
            if(!delivered && amt.majortype == MEDIATYPE_Video)
            {
                bits8 *pb;
                if(pSample->GetPointer(&pb) != S_OK)
                {
                    LeaveCriticalSection(&pfilter->cs_filter);
                    LeaveCriticalSection(&cs_receive);
#if write_level > 1
                    print("filter_out_pin::Receive returns VFW_E_RUNTIME_ERROR.\n");
#endif
                    return VFW_E_RUNTIME_ERROR;
                }
                pfilter->pcallback->frame_ready((bits16 *)pb);
                delivered = true;
            }
            HANDLE events[2] = { pfilter->event_not_paused, event_flushing };
            LeaveCriticalSection(&pfilter->cs_filter);
            WaitForMultipleObjects(2, events, false, INFINITE);
            EnterCriticalSection(&pfilter->cs_filter);
        }
        else if(pfilter->state == State_Running)
        {
            if(delivered)
            {
                LeaveCriticalSection(&pfilter->cs_filter);
                LeaveCriticalSection(&cs_receive);
#if write_level > 1
                print("filter_out_pin::Receive returns S_OK.\n");
#endif
                return S_OK;
            }
            int64 t;
            HRESULT r = pfilter->pclock->GetTime(&t);
            if(r != S_OK && r != S_FALSE)
            {
                LeaveCriticalSection(&pfilter->cs_filter);
                LeaveCriticalSection(&cs_receive);
#if write_level > 1
                print("filter_out_pin::Receive returns VFW_E_RUNTIME_ERROR.\n");
#endif
                return VFW_E_RUNTIME_ERROR;
            }
            if(t - pfilter->t_start > tstart + 50000)
            {
                LeaveCriticalSection(&pfilter->cs_filter);
                LeaveCriticalSection(&cs_receive);
                return S_OK;
            }
            else if(t - pfilter->t_start < tstart)
            {
                LeaveCriticalSection(&pfilter->cs_filter);
                Sleep(1);
                EnterCriticalSection(&pfilter->cs_filter);
            }
            else
            {
                bits8 *pb;
                if(pSample->GetPointer(&pb) != S_OK)
                {
                    LeaveCriticalSection(&pfilter->cs_filter);
                    LeaveCriticalSection(&cs_receive);
#if write_level > 1
                    print("filter_out_pin::Receive returns VFW_E_RUNTIME_ERROR.\n");
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
                LeaveCriticalSection(&cs_receive);
#if write_level > 1
                print("filter_out_pin::Receive returns S_OK.\n");
#endif
                return S_OK;
            }
        }
    }
}

HRESULT __stdcall filter_out_pin::ReceiveMultiple(IMediaSample **pSamples, long /*nSamples*/, long *nSamplesProcessed)
{
#if write_level > 1
    print("filter_out_pin::ReceiveMultiple called...\n");
#endif
    if(!pSamples || !nSamplesProcessed) return E_POINTER;
    EnterCriticalSection(&pfilter->cs_filter);
    if(pfilter->state == State_Stopped)
    {
        LeaveCriticalSection(&pfilter->cs_filter);
        return VFW_E_WRONG_STATE;
    }
    LeaveCriticalSection(&pfilter->cs_filter);
    return VFW_E_RUNTIME_ERROR;
}

HRESULT __stdcall filter_out_pin::ReceiveCanBlock()
{
#if write_level > 1
    print("filter_out_pin::ReceiveCanBlock called...\n");
#endif
    return S_OK;
}

//----------------------------------------------------------------------------
// filter_out_filter
//----------------------------------------------------------------------------

filter_out_filter::filter_out_filter()
{
#if write_level > 2
    print("filter_out_filter::filter_out_filter called...\n");
#endif
}

filter_out_filter::~filter_out_filter()
{
#if write_level > 2
    print("filter_out_filter::~filter_out_filter called...\n");
#endif
}

HRESULT __stdcall filter_out_filter::QueryInterface(REFIID riid, void **ppvObject)
{
#if write_level > 1
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
    if(riid == IID_IReferenceClock)
    {
        *(IReferenceClock **)ppvObject = this;
        ((IReferenceClock *)*ppvObject)->AddRef();
        return S_OK;
    }
    *ppvObject = null;
    return E_NOINTERFACE;
}

ULONG __stdcall filter_out_filter::AddRef()
{
#if write_level > 2
    print("filter_out_filter::AddRef called...\n");
#endif
    EnterCriticalSection(&cs_filter);
    nat32 r = ++reference_count;
    LeaveCriticalSection(&cs_filter);
    return r;
}

ULONG __stdcall filter_out_filter::Release()
{
#if write_level > 2
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

        reference_count = 0;
        LeaveCriticalSection(&cs_filter);

        WaitForSingleObject(thread_worker, INFINITE);

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
#if write_level > 1
    print("filter_out_filter::GetClassID called...\n");
#endif
    if(!pClassID) return E_POINTER;
    // {681B4202-3E20-4955-9C0A-CA4EB274E431}
    static const GUID guid =
    { 0x681b4202, 0x3e20, 0x4955, { 0x9c, 0x0a, 0xca, 0x4e, 0xb2, 0x74, 0xe4, 0x31 } };
    *pClassID = guid;
    return S_OK;
}

HRESULT __stdcall filter_out_filter::Stop()
{
#if write_level > 1
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
#if write_level > 1
    print("filter_out_filter::Pause called...\n");
#endif
    HRESULT r;
    EnterCriticalSection(&cs_filter);
    if(state == State_Stopped)
    {
        if(state2 == State_Stopped)
        {
            if(pin.pconnected)
            {
                state2 = State_Paused;
                ResetEvent(event_state_set);
                r = S_FALSE;
            }
            else
            {
                state = State_Paused;
                state2 = State_Paused;
                ResetEvent(event_not_paused);
                r = S_OK;
            }
        }
        else
        {
            r = S_FALSE;
        }
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
#if write_level > 1
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
    t_last = 0;
    LeaveCriticalSection(&cs_filter);
    return S_OK;
}

HRESULT __stdcall filter_out_filter::GetState(DWORD dwMilliSecsTimeout, FILTER_STATE *State)
{
#if write_level > 1
    print("filter_out_filter::GetState(%u) called...", dwMilliSecsTimeout);
#endif
    if(!State)
    {
#if write_level > 1
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
#if write_level > 1
            print(" Returns %i, VFW_S_STATE_INTERMEDIATE.\n", *State);
#endif
            return VFW_S_STATE_INTERMEDIATE;
        }
        else
        {
            LeaveCriticalSection(&cs_filter);
#if write_level > 1
            print(" Returns VFW_E_RUNTIME_ERROR.\n");
#endif
            return VFW_E_RUNTIME_ERROR;
        }
    }
    *State = state2;
    LeaveCriticalSection(&cs_filter);
#if write_level > 1
    print(" Returns %i, S_OK.\n", *State);
#endif
    return S_OK;
}

HRESULT __stdcall filter_out_filter::SetSyncSource(IReferenceClock *pClock)
{
#if write_level > 1
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
#if write_level > 1
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
#if write_level > 1
    print("filter_out_filter::EnumPins called...\n");
#endif
    if(!ppEnum) return E_POINTER;
    EnterCriticalSection(&cs_filter);
    HRESULT r = create_enum_pins_single(&pin, ppEnum) ? S_OK : E_FAIL;
    LeaveCriticalSection(&cs_filter);
    return r;
}

HRESULT __stdcall filter_out_filter::FindPin(LPCWSTR Id, IPin **ppPin)
{
#if write_level > 1
    print("filter_out_filter::FindPin called...\n");
#endif
    if(!ppPin) return E_POINTER;
    EnterCriticalSection(&cs_filter);
    if(!wcscmp(Id, L"Input"))
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
#if write_level > 1
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
#if write_level > 1
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
#if write_level > 1
    print("filter_out_filter::QueryVendorInfo called...\n");
#endif
    if(!pVendorInfo) return E_POINTER;
    return E_NOTIMPL;
}

ULONG __stdcall filter_out_filter::GetMiscFlags()
{
#if write_level > 1
    print("filter_out_filter::GetMiscFlags called, returns AM_FILTER_MISC_FLAGS_IS_RENDERER.\n");
#endif
    return AM_FILTER_MISC_FLAGS_IS_RENDERER;
}

HRESULT __stdcall filter_out_filter::GetCapabilities(DWORD *pCapabilities)
{
#if write_level > 1
    print("filter_out_filter::GetCapabilities called...\n");
#endif
    HRESULT r;
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
    return r;
}

HRESULT __stdcall filter_out_filter::CheckCapabilities(DWORD *pCapabilities)
{
#if write_level > 1
    print("filter_out_filter::CheckCapabilities called...\n");
#endif
    HRESULT r;
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
    return r;
}

HRESULT __stdcall filter_out_filter::IsFormatSupported(const GUID *pFormat)
{
#if write_level > 1
    print("filter_out_filter::IsFormatSupported(");
    print(*pFormat);
    print(") called...");
#endif
    HRESULT r;
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
#if write_level > 1
    print(" Returns 0x%x.\n", r);
#endif
    return r;
}

HRESULT __stdcall filter_out_filter::QueryPreferredFormat(GUID *pFormat)
{
#if write_level > 1
    print("filter_out_filter::QueryPreferredFormat called...\n");
#endif
    HRESULT r;
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
    return r;
}

HRESULT __stdcall filter_out_filter::GetTimeFormat(GUID *pFormat)
{
#if write_level > 1
    print("filter_out_filter::GetTimeFormat called...\n");
#endif
    HRESULT r;
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
    return r;
}

HRESULT __stdcall filter_out_filter::IsUsingTimeFormat(const GUID *pFormat)
{
#if write_level > 1
    print("filter_out_filter::IsUsingTimeFormat called...\n");
#endif
    HRESULT r;
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
    return r;
}

HRESULT __stdcall filter_out_filter::SetTimeFormat(const GUID *pFormat)
{
#if write_level > 1
    print("filter_out_filter::SetTimeFormat called...\n");
#endif
    HRESULT r;
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
    return r;
}

HRESULT __stdcall filter_out_filter::GetDuration(LONGLONG *pDuration)
{
#if write_level > 1
    print("filter_out_filter::GetDuration called...\n");
#endif
    HRESULT r;
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
    return r;
}

HRESULT __stdcall filter_out_filter::GetStopPosition(LONGLONG *pStop)
{
#if write_level > 1
    print("filter_out_filter::GetStopPosition called...\n");
#endif
    HRESULT r;
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
    return r;
}

HRESULT __stdcall filter_out_filter::GetCurrentPosition(LONGLONG *pCurrent)
{
#if write_level > 1
    print("filter_out_filter::GetCurrentPosition called...\n");
#endif
    HRESULT r;
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
    return r;
}

HRESULT __stdcall filter_out_filter::ConvertTimeFormat(LONGLONG *pTarget, const GUID *pTargetFormat, LONGLONG Source, const GUID *pSourceFormat)
{
#if write_level > 1
    print("filter_out_filter::ConvertTimeFormat called...\n");
#endif
    HRESULT r;
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
    return r;
}

HRESULT __stdcall filter_out_filter::SetPositions(LONGLONG *pCurrent, DWORD dwCurrentFlags, LONGLONG *pStop, DWORD dwStopFlags)
{
#if write_level > 1
    print("filter_out_filter::SetPositions called...\n");
#endif
    HRESULT r;
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
    return r;
}

HRESULT __stdcall filter_out_filter::GetPositions(LONGLONG *pCurrent, LONGLONG *pStop)
{
#if write_level > 1
    print("filter_out_filter::GetPositions called...\n");
#endif
    HRESULT r;
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
    return r;
}

HRESULT __stdcall filter_out_filter::GetAvailable(LONGLONG *pEarliest, LONGLONG *pLatest)
{
#if write_level > 1
    print("filter_out_filter::GetAvailable called...\n");
#endif
    HRESULT r;
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
    return r;
}

HRESULT __stdcall filter_out_filter::SetRate(double dRate)
{
#if write_level > 1
    print("filter_out_filter::SetRate called...\n");
#endif
    HRESULT r;
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
    return r;
}

HRESULT __stdcall filter_out_filter::GetRate(double *pdRate)
{
#if write_level > 1
    print("filter_out_filter::GetRate called...\n");
#endif
    HRESULT r;
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
    return r;
}

HRESULT __stdcall filter_out_filter::GetPreroll(LONGLONG *pllPreroll)
{
#if write_level > 1
    print("filter_out_filter::GetPreroll called...\n");
#endif
    HRESULT r;
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
    return r;
}

HRESULT __stdcall filter_out_filter::GetTime(REFERENCE_TIME *pTime)
{
#if write_level > 1
    print("filter_out_filter::GetTime called...\n");
#endif
#if write_level > 2
    if(pin.amt.majortype == MEDIATYPE_Audio)
    {
        print("Audio.\n");
    }
    else if(pin.amt.majortype == MEDIATYPE_Video)
    {
        print("Video.\n");
    }
#endif
    if(!pTime) return E_POINTER;
    int64 t_abs;
    QueryPerformanceCounter((LARGE_INTEGER *)&t_abs);
    int64 t_step = t_abs - t_perf_prev;
    t_perf_prev = t_abs;
    t_step = (int64)((real64)t_step * 10000000. / (real64)perf_freq);

    if(state == State_Running)
    {
        t_step = max(min(t_step, t_start + t_last - t_prev), 0);
#if write_level > 1
        print("t_start=%I64i, t_last=%I64i, t_prev=%I64i\n", t_start, t_last, t_prev);
#endif
    }

    if(t_step)
    {
        int64 t = t_prev + t_step;
        t_prev = t;
        *pTime = t;
        return S_OK;
    }
    else
    {
        *pTime = t_prev;
        return S_FALSE;
    }
}

HRESULT __stdcall filter_out_filter::AdviseTime(REFERENCE_TIME baseTime, REFERENCE_TIME streamTime, HEVENT hEvent, DWORD_PTR *pdwAdviseCookie)
{
#if write_level > 0
    print("filter_out_filter::AdviseTime called...\n");
#endif
    return E_OUTOFMEMORY;
}

HRESULT __stdcall filter_out_filter::AdvisePeriodic(REFERENCE_TIME startTime, REFERENCE_TIME periodTime, HSEMAPHORE hSemaphore, DWORD_PTR *pdwAdviseCookie)
{
#if write_level > 0
    print("filter_out_filter::AdvisePeriodic called...\n");
#endif
    return E_OUTOFMEMORY;
}

HRESULT __stdcall filter_out_filter::Unadvise(DWORD_PTR dwAdviseCookie)
{
#if write_level > 0
    print("filter_out_filter::Unadvise called...\n");
#endif
    return S_FALSE;
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

nat32 __stdcall filter_out_filter::worker_thread(void *param)
{
    filter_out_filter *pfilter = (filter_out_filter *)param;
    for(;;)
    {
        EnterCriticalSection(&pfilter->cs_filter);
        if(!pfilter->reference_count)
        {
            LeaveCriticalSection(&pfilter->cs_filter);
            return 0;
        }
        if(pfilter->state == State_Running)
        {
            if(pfilter->pin.amt.majortype == MEDIATYPE_Audio)
            {
                int64 t;
                HRESULT r = pfilter->pclock->GetTime(&t);
                if(r != S_OK && r != S_FALSE)
                {
#if write_level > 0
                    error("IReferenceClock::GetTime", r);
#endif
                }
                else
                {
                    if(t > pfilter->t_start + pfilter->t_last)
                    {
                        /*IAMClockAdjust *pamca;
                        r = pfilter->pclock->QueryInterface(IID_IAMClockAdjust, (void **)&pamca);
                        if(r != S_OK)
                        {
#if write_level > 0
                            error("IReferenceClock::QueryInterface(IID_IAMClockAdjust)", r);
#endif
                        }
                        else
                        {
                            pamca->SetClockDelta(-30000);
                            pamca->Release();
                        }*/

                        /*IMediaEventSink *pmes;
                        r = pfilter->pgraph->QueryInterface(IID_IMediaEventSink, (void **)&pmes);
                        if(r != S_OK)
                        {
#if write_level > 0
                            error("IGraphBuilder::QueryInterface(IID_IMediaEventSink)", r);
#endif
                        }
                        else
                        {
                            // pmes->Notify(EC_STARVATION, 0, 0);
                            pmes->Release();
                        }*/
                    }
                }
            }
        }
        LeaveCriticalSection(&pfilter->cs_filter);
        Sleep(1);
    }
}

bool create_filter_out(const AM_MEDIA_TYPE *pamt, player_callback *pcallback, IBaseFilter **ppfilter)
{
#if write_level > 2
    print("filter_out_filter::create called...\n");
#endif
    if(!pamt || !pcallback || !ppfilter) return false;

    filter_out_filter *pfilter = new filter_out_filter;
    if(!pfilter) return false;

    /* filter_out_allocator *p_allocator;
    if(!filter_out_allocator::create(&p_allocator))
    {
        delete pfilter;
        return false;
    }*/
    pfilter->pin.p_allocator = null; //p_allocator;

    if(!InitializeCriticalSectionAndSpinCount(&pfilter->cs_filter, 0x80000000))
    {
        pfilter->pin.p_allocator->Release();
        delete pfilter;
        return false;
    }

    pfilter->pin.event_flushing = CreateEvent(null, true, false, null);
    if(!pfilter->pin.event_flushing)
    {
        DeleteCriticalSection(&pfilter->cs_filter);
        pfilter->pin.p_allocator->Release();
        delete pfilter;
        return false;
    }

    if(!InitializeCriticalSectionAndSpinCount(&pfilter->pin.cs_receive, 0x80000000))
    {
        CloseHandle(pfilter->pin.event_flushing);
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

    QueryPerformanceFrequency((LARGE_INTEGER *)&pfilter->perf_freq);
// #if write_level > 0
//     print("Performance frequency=%I64i.\n", pfilter->perf_freq);
// #endif
    QueryPerformanceCounter((LARGE_INTEGER *)&pfilter->t_perf_prev);
    pfilter->t_prev = 0;

    pfilter->thread_worker = (HANDLE)_beginthreadex(null, 0, filter_out_filter::worker_thread, pfilter, 0, null);

    *ppfilter =  pfilter;
    return true;
}
