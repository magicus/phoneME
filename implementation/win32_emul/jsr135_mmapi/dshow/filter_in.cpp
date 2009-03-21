#include <wmsdkidl.h>
#include "filter_in.hpp"
#pragma comment (lib, "strmiids.lib")

#define DUMP_LEVEL 1

#if DUMP_LEVEL > 0
#include "writer.hpp"
#endif

static void PRINTF( const char* fmt, ... ) {
    char           str8[ 256 ];
	va_list        args;

	va_start(args, fmt);
    vsprintf( str8, fmt, args );
	va_end(args);

    OutputDebugString( str8 );
}

class filter_in_filter;
class filter_in_pin;

class filter_in_enum_media_types : public IEnumMediaTypes
{
    friend filter_in_pin;

    ULONG reference_count;
    ULONG index;

    filter_in_enum_media_types();
    ~filter_in_enum_media_types();
public:
    virtual HRESULT __stdcall QueryInterface(REFIID riid, void **ppvObject);
    virtual ULONG __stdcall AddRef();
    virtual ULONG __stdcall Release();
    virtual HRESULT __stdcall Next(ULONG cMediaTypes, AM_MEDIA_TYPE **ppMediaTypes, ULONG *pcFetched);
    virtual HRESULT __stdcall Skip(ULONG cMediaTypes);
    virtual HRESULT __stdcall Reset();
    virtual HRESULT __stdcall Clone(IEnumMediaTypes **ppEnum);
};

class filter_in_pin : public IPin
{
    friend filter_in_filter;

    filter_in_filter *pfilter;
    IPin *pconnected;
    IMemInputPin *pinput;
    IMemAllocator *pallocator;

    filter_in_pin();
    ~filter_in_pin();
public:
    virtual HRESULT __stdcall QueryInterface(REFIID riid, void **ppvObject);
    virtual ULONG __stdcall AddRef();
    virtual ULONG __stdcall Release();
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
};

class filter_in_enum_pins : public IEnumPins
{
    friend filter_in_filter;

    ULONG reference_count;
    filter_in_pin *ppin;
    ULONG index;

    filter_in_enum_pins();
    ~filter_in_enum_pins();
public:
    virtual HRESULT __stdcall QueryInterface(REFIID riid, void **ppvObject);
    virtual ULONG __stdcall AddRef();
    virtual ULONG __stdcall Release();
    virtual HRESULT __stdcall Next(ULONG cPins, IPin **ppPins, ULONG *pcFetched);
    virtual HRESULT __stdcall Skip(ULONG cPins);
    virtual HRESULT __stdcall Reset();
    virtual HRESULT __stdcall Clone(IEnumPins **ppEnum);
};

class filter_in_filter : public filter_in
{
    friend filter_in_pin;

    ULONG reference_count;
    filter_in_pin *ppin;
    IFilterGraph *pgraph;
    IReferenceClock *pclock;
    WCHAR name[MAX_FILTER_NAME];
    FILTER_STATE state;

    filter_in_filter();
    ~filter_in_filter();
public:
    virtual HRESULT __stdcall QueryInterface(REFIID riid, void **ppvObject);
    virtual ULONG __stdcall AddRef();
    virtual ULONG __stdcall Release();
    virtual HRESULT __stdcall GetClassID(CLSID *pClassID);
    virtual HRESULT __stdcall Stop();
    virtual HRESULT __stdcall Pause();
    virtual HRESULT __stdcall Run(REFERENCE_TIME tStart);
    virtual HRESULT __stdcall GetState(DWORD dwMilliSecsTimeout, FILTER_STATE *State);
    virtual HRESULT __stdcall SetSyncSource(IReferenceClock *pClock);
    virtual HRESULT __stdcall GetSyncSource(IReferenceClock **pClock);
    virtual HRESULT __stdcall EnumPins(IEnumPins **ppEnum);
    virtual HRESULT __stdcall FindPin(LPCWSTR Id, IPin **ppPin);
    virtual HRESULT __stdcall QueryFilterInfo(FILTER_INFO *pInfo);
    virtual HRESULT __stdcall JoinFilterGraph(IFilterGraph *pGraph, LPCWSTR pName);
    virtual HRESULT __stdcall QueryVendorInfo(LPWSTR *pVendorInfo);
    virtual bool data(SIZE_T size, BYTE *p);
    static bool create(wchar_t const *format, filter_in_filter **ppfilter);
};

//----------------------------------------------------------------------------
// filter_in_enum_media_types
//----------------------------------------------------------------------------

filter_in_enum_media_types::filter_in_enum_media_types()
{
#if DUMP_LEVEL > 1
    PRINTF("filter_in_enum_media_types::filter_in_enum_media_types called...\n");
#endif
}

filter_in_enum_media_types::~filter_in_enum_media_types()
{
#if DUMP_LEVEL > 1
    PRINTF("filter_in_enum_media_types::~filter_in_enum_media_types called...\n");
#endif
}

HRESULT __stdcall filter_in_enum_media_types::QueryInterface(REFIID riid, void **ppvObject)
{
#if DUMP_LEVEL > 0
    PRINTF("filter_in_enum_media_types::QueryInterface called...\n");
#endif
    if(!ppvObject) return E_POINTER;
    if(riid == IID_IUnknown)
    {
        *(IUnknown **) ppvObject = this;
        ((IUnknown *) *ppvObject)->AddRef();
        return S_OK;
    }
    if(riid == IID_IEnumMediaTypes)
    {
        *(IEnumMediaTypes **) ppvObject = this;
        ((IEnumMediaTypes *) *ppvObject)->AddRef();
        return S_OK;
    }
    *ppvObject = NULL;
    return E_NOINTERFACE;
}

ULONG __stdcall filter_in_enum_media_types::AddRef()
{
#if DUMP_LEVEL > 1
    PRINTF("filter_in_enum_media_types::AddRef called...\n");
#endif
    return ++reference_count;
}

ULONG __stdcall filter_in_enum_media_types::Release()
{
#if DUMP_LEVEL > 1
    PRINTF("filter_in_enum_media_types::Release called...\n");
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
#if DUMP_LEVEL > 0
    PRINTF("filter_in_enum_media_types::Next called...\n");
#endif
    if(!ppMediaTypes) return E_POINTER;
    if(cMediaTypes != 1 && !pcFetched) return E_INVALIDARG;
    ULONG i;
    for(i = 0; i < cMediaTypes; i++)
    {
        if(index >= 1)
        {
            if(pcFetched) *pcFetched = i;
            return S_FALSE;
        }
        AM_MEDIA_TYPE *pamt = (AM_MEDIA_TYPE*) CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
        if(!pamt)
        {
            if(pcFetched) *pcFetched = i;
            return E_OUTOFMEMORY;
        }

        MPEGLAYER3WAVEFORMAT *pml3wf = (MPEGLAYER3WAVEFORMAT*) CoTaskMemAlloc(sizeof(MPEGLAYER3WAVEFORMAT));
        if(!pml3wf)
        {
            CoTaskMemFree(pamt);
            if(pcFetched) *pcFetched = i;
            return E_OUTOFMEMORY;
        }
        pamt->majortype = MEDIATYPE_Audio;
        pamt->subtype = WMMEDIASUBTYPE_MP3;
        pamt->bFixedSizeSamples = TRUE;
        pamt->bTemporalCompression = FALSE;
        pamt->lSampleSize = 1;
        pamt->formattype = FORMAT_WaveFormatEx;
        pamt->pUnk = NULL;
        pamt->cbFormat = sizeof(MPEGLAYER3WAVEFORMAT);
        pamt->pbFormat = (BYTE*) pml3wf;
        pml3wf->wfx.wFormatTag = WAVE_FORMAT_MPEGLAYER3;
        pml3wf->wfx.nChannels = 2;
        pml3wf->wfx.nSamplesPerSec = 44100;
        pml3wf->wfx.nAvgBytesPerSec = 16000;
        pml3wf->wfx.nBlockAlign = 1;
        pml3wf->wfx.wBitsPerSample = 0;
        pml3wf->wfx.cbSize = MPEGLAYER3_WFX_EXTRA_BYTES;
        pml3wf->wID = 1;
        pml3wf->fdwFlags = 0;
        pml3wf->nBlockSize = 1;
        pml3wf->nFramesPerBlock = 1;
        pml3wf->nCodecDelay = 0;

        /*MPEG1WAVEFORMAT *pm1wf = (MPEG1WAVEFORMAT*) CoTaskMemAlloc(sizeof(MPEG1WAVEFORMAT));
        if(!pm1wf)
        {
            CoTaskMemFree(pamt);
            if(pcFetched) *pcFetched = i;
            return E_OUTOFMEMORY;
        }
        pamt->majortype = MEDIATYPE_Audio;
        pamt->subtype = MEDIASUBTYPE_MPEG1AudioPayload;
        pamt->bFixedSizeSamples = TRUE;
        pamt->bTemporalCompression = FALSE;
        pamt->lSampleSize = 1;
        pamt->formattype = FORMAT_WaveFormatEx;
        pamt->pUnk = NULL;
        pamt->cbFormat = sizeof(MPEG1WAVEFORMAT);
        pamt->pbFormat = (BYTE*) pm1wf;
        pm1wf->wfx.wFormatTag = WAVE_FORMAT_MPEG;
        pm1wf->wfx.nChannels = 2;
        pm1wf->wfx.nSamplesPerSec = 44100;
        pm1wf->wfx.nAvgBytesPerSec = 16000;
        pm1wf->wfx.nBlockAlign = 1;
        pm1wf->wfx.wBitsPerSample = 0;
        pm1wf->wfx.cbSize = sizeof(MPEG1WAVEFORMAT) - sizeof(WAVEFORMATEX);
        pm1wf->fwHeadLayer = 4;
        pm1wf->dwHeadBitrate = 128000;
        pm1wf->fwHeadMode = 2;
        pm1wf->fwHeadModeExt = 64;
        pm1wf->wHeadEmphasis = 1;
        pm1wf->fwHeadFlags = 24;
        pm1wf->dwPTSLow = 0;
        pm1wf->dwPTSHigh = 0;*/

        ppMediaTypes[i] = pamt;
        index++;
    }
    if(pcFetched) *pcFetched = i;
    return S_OK;
}

HRESULT __stdcall filter_in_enum_media_types::Skip(ULONG cMediaTypes)
{
#if DUMP_LEVEL > 0
    PRINTF("filter_in_enum_media_types::Skip called...\n");
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
#if DUMP_LEVEL > 0
    PRINTF("filter_in_enum_media_types::Reset called...\n");
#endif
    index = 0;
    return S_OK;
}

HRESULT __stdcall filter_in_enum_media_types::Clone(IEnumMediaTypes **ppEnum)
{
#if DUMP_LEVEL > 0
    PRINTF("filter_in_enum_media_types::Clone called...\n");
#endif
    if(!ppEnum) return E_POINTER;
    filter_in_enum_media_types *penum_media_types = new filter_in_enum_media_types;
    if(!penum_media_types) return E_OUTOFMEMORY;
    penum_media_types->reference_count = 1;
    penum_media_types->index = index;
    *ppEnum = penum_media_types;
    return S_OK;
}

//----------------------------------------------------------------------------
// filter_in_pin
//----------------------------------------------------------------------------

filter_in_pin::filter_in_pin()
{
#if DUMP_LEVEL > 1
    PRINTF("filter_in_pin::filter_in_pin called...\n");
#endif
}

filter_in_pin::~filter_in_pin()
{
#if DUMP_LEVEL > 1
    PRINTF("filter_in_pin::~filter_in_pin called...\n");
#endif
}

HRESULT __stdcall filter_in_pin::QueryInterface(REFIID riid, void **ppvObject)
{
#if DUMP_LEVEL > 0
    PRINTF("filter_in_pin::QueryInterface(");
    print(riid);
    PRINTF(", %p) called...\n", ppvObject);
#endif
    if(!ppvObject) return E_POINTER;
    if(riid == IID_IUnknown)
    {
        *(IUnknown **) ppvObject = this;
        ((IUnknown *) *ppvObject)->AddRef();
        return S_OK;
    }
    if(riid == IID_IPin)
    {
        *(IPin **) ppvObject = this;
        ((IPin *) *ppvObject)->AddRef();
        return S_OK;
    }
    *ppvObject = NULL;
    return E_NOINTERFACE;
}

ULONG __stdcall filter_in_pin::AddRef()
{
#if DUMP_LEVEL > 1
    PRINTF("filter_in_pin::AddRef called...\n");
#endif
    return pfilter->AddRef();
}

ULONG __stdcall filter_in_pin::Release()
{
#if DUMP_LEVEL > 1
    PRINTF("filter_in_pin::Release called...\n");
#endif
    return pfilter->Release();
}

HRESULT __stdcall filter_in_pin::Connect(IPin *pReceivePin, AM_MEDIA_TYPE const *pmt)
{
#if DUMP_LEVEL > 0
    PRINTF("filter_in_pin::Connect called...\n");
#endif
    if(!pReceivePin) return E_POINTER;
    if(pconnected) return VFW_E_ALREADY_CONNECTED;
    if(pfilter->state != State_Stopped) return VFW_E_NOT_STOPPED;
    if(pmt)
    {
        if(
            pmt->majortype != GUID_NULL &&
            pmt->majortype != MEDIATYPE_Audio ||
            pmt->subtype != GUID_NULL &&
            //pmt->subtype != WMMEDIASUBTYPE_MP3 ||
            pmt->subtype != MEDIASUBTYPE_MPEG1AudioPayload ||
            pmt->formattype != GUID_NULL &&
            pmt->formattype != FORMAT_WaveFormatEx)
            return VFW_E_TYPE_NOT_ACCEPTED;
    }
    AM_MEDIA_TYPE amt;

    MPEGLAYER3WAVEFORMAT ml3wf;
    amt.majortype = MEDIATYPE_Audio;
    amt.subtype = WMMEDIASUBTYPE_MP3;
    amt.bFixedSizeSamples = TRUE;
    amt.bTemporalCompression = FALSE;
    amt.lSampleSize = 1;
    amt.formattype = FORMAT_WaveFormatEx;
    amt.pUnk = NULL;
    amt.cbFormat = sizeof(MPEGLAYER3WAVEFORMAT);
    amt.pbFormat = (BYTE*) &ml3wf;
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

    /*MPEG1WAVEFORMAT m1wf;
    amt.majortype = MEDIATYPE_Audio;
    amt.subtype = MEDIASUBTYPE_MPEG1AudioPayload;
    amt.bFixedSizeSamples = TRUE;
    amt.bTemporalCompression = FALSE;
    amt.lSampleSize = 1;
    amt.formattype = FORMAT_WaveFormatEx;
    amt.pUnk = NULL;
    amt.cbFormat = sizeof(MPEG1WAVEFORMAT);
    amt.pbFormat = (BYTE*) &m1wf;
    m1wf.wfx.wFormatTag = WAVE_FORMAT_MPEG;
    m1wf.wfx.nChannels = 2;
    m1wf.wfx.nSamplesPerSec = 44100;
    m1wf.wfx.nAvgBytesPerSec = 16000;
    m1wf.wfx.nBlockAlign = 1;
    m1wf.wfx.wBitsPerSample = 0;
    m1wf.wfx.cbSize = sizeof(MPEG1WAVEFORMAT) - sizeof(WAVEFORMATEX);
    m1wf.fwHeadLayer = 4;
    m1wf.dwHeadBitrate = 128000;
    m1wf.fwHeadMode = 2;
    m1wf.fwHeadModeExt = 64;
    m1wf.wHeadEmphasis = 1;
    m1wf.fwHeadFlags = 24;
    m1wf.dwPTSLow = 0;
    m1wf.dwPTSHigh = 0;*/

    HRESULT hr = pReceivePin->ReceiveConnection(this, &amt);
    if(hr == VFW_E_TYPE_NOT_ACCEPTED) return VFW_E_NO_ACCEPTABLE_TYPES;
    if(hr != S_OK) return hr;
    hr = pReceivePin->QueryInterface(IID_IMemInputPin, (void**) &pinput);
    if(hr != S_OK) return VFW_E_NO_TRANSPORT;
    hr = pinput->GetAllocator(&pallocator);
    if(hr != S_OK)
    {
        pinput->Release();
        return VFW_E_NO_TRANSPORT;
    }
    ALLOCATOR_PROPERTIES request;
    ALLOCATOR_PROPERTIES actual;
    request.cBuffers = 1;
    request.cbBuffer = 1;
    request.cbAlign = 1;
    request.cbPrefix = 0;
    hr = pallocator->SetProperties(&request, &actual);
    if(hr != S_OK || actual.cBuffers != 1 || actual.cbBuffer != 1 || actual.cbAlign != 1 || actual.cbPrefix != 0)
    {
        pallocator->Release();
        pinput->Release();
        return VFW_E_NO_TRANSPORT;
    }
    hr = pinput->NotifyAllocator(pallocator, FALSE);
    if(hr != S_OK)
    {
        pallocator->Release();
        pinput->Release();
        return VFW_E_NO_TRANSPORT;
    }
    hr = pallocator->Commit();
    if(hr != S_OK)
    {
        pallocator->Release();
        pinput->Release();
        return VFW_E_NO_TRANSPORT;
    }
    pconnected = pReceivePin;
    pconnected->AddRef();
    return S_OK;
}

HRESULT __stdcall filter_in_pin::ReceiveConnection(IPin *pConnector, AM_MEDIA_TYPE const *pmt)
{
#if DUMP_LEVEL > 0
    PRINTF("filter_in_pin::ReceiveConnection called...\n");
#endif
    if(!pConnector || !pmt) return E_POINTER;
    return E_UNEXPECTED;
}

HRESULT __stdcall filter_in_pin::Disconnect()
{
#if DUMP_LEVEL > 0
    PRINTF("filter_in_pin::Disconnect called...\n");
#endif
    if(pfilter->state != State_Stopped) return VFW_E_NOT_STOPPED;
    if(!pconnected) return S_FALSE;
    pallocator->Release();
    pinput->Release();
    pconnected->Release();
    pconnected = NULL;
    return S_OK;
}

HRESULT __stdcall filter_in_pin::ConnectedTo(IPin **pPin)
{
#if DUMP_LEVEL > 0
    PRINTF("filter_in_pin::ConnectedTo called...\n");
#endif
    if(!pPin) return E_POINTER;
    if(!pconnected)
    {
        *pPin = NULL;
        return VFW_E_NOT_CONNECTED;
    }
    *pPin = pconnected;
    (*pPin)->AddRef();
    return S_OK;
}

HRESULT __stdcall filter_in_pin::ConnectionMediaType(AM_MEDIA_TYPE *pmt)
{
#if DUMP_LEVEL > 0
    PRINTF("filter_in_pin::ConnectionMediaType called...\n");
#endif
    if(!pmt) return E_POINTER;
    if(!pconnected)
    {
        memset(pmt, 0, sizeof(AM_MEDIA_TYPE));
        return VFW_E_NOT_CONNECTED;
    }

    MPEGLAYER3WAVEFORMAT *pml3wf = (MPEGLAYER3WAVEFORMAT*) CoTaskMemAlloc(sizeof(MPEGLAYER3WAVEFORMAT));
    if(!pml3wf) return E_OUTOFMEMORY;
    pmt->majortype = MEDIATYPE_Audio;
    pmt->subtype = WMMEDIASUBTYPE_MP3;
    pmt->bFixedSizeSamples = TRUE;
    pmt->bTemporalCompression = FALSE;
    pmt->lSampleSize = 1;
    pmt->formattype = FORMAT_WaveFormatEx;
    pmt->pUnk = NULL;
    pmt->cbFormat = sizeof(MPEGLAYER3WAVEFORMAT);
    pmt->pbFormat = (BYTE*) pml3wf;
    pml3wf->wfx.wFormatTag = WAVE_FORMAT_MPEGLAYER3;
    pml3wf->wfx.nChannels = 2;
    pml3wf->wfx.nSamplesPerSec = 44100;
    pml3wf->wfx.nAvgBytesPerSec = 16000;
    pml3wf->wfx.nBlockAlign = 1;
    pml3wf->wfx.wBitsPerSample = 0;
    pml3wf->wfx.cbSize = MPEGLAYER3_WFX_EXTRA_BYTES;
    pml3wf->wID = 1;
    pml3wf->fdwFlags = 0;
    pml3wf->nBlockSize = 1;
    pml3wf->nFramesPerBlock = 1;
    pml3wf->nCodecDelay = 0;

    /*MPEG1WAVEFORMAT *pm1wf = (MPEG1WAVEFORMAT*) CoTaskMemAlloc(sizeof(MPEG1WAVEFORMAT));
    if(!pm1wf) return E_OUTOFMEMORY;
    pmt->majortype = MEDIATYPE_Audio;
    pmt->subtype = MEDIASUBTYPE_MPEG1AudioPayload;
    pmt->bFixedSizeSamples = TRUE;
    pmt->bTemporalCompression = FALSE;
    pmt->lSampleSize = 1;
    pmt->formattype = FORMAT_WaveFormatEx;
    pmt->pUnk = NULL;
    pmt->cbFormat = sizeof(MPEG1WAVEFORMAT);
    pmt->pbFormat = (BYTE*) pm1wf;
    pm1wf->wfx.wFormatTag = WAVE_FORMAT_MPEG;
    pm1wf->wfx.nChannels = 2;
    pm1wf->wfx.nSamplesPerSec = 44100;
    pm1wf->wfx.nAvgBytesPerSec = 16000;
    pm1wf->wfx.nBlockAlign = 1;
    pm1wf->wfx.wBitsPerSample = 0;
    pm1wf->wfx.cbSize = sizeof(MPEG1WAVEFORMAT) - sizeof(WAVEFORMATEX);
    pm1wf->fwHeadLayer = 4;
    pm1wf->dwHeadBitrate = 128000;
    pm1wf->fwHeadMode = 2;
    pm1wf->fwHeadModeExt = 64;
    pm1wf->wHeadEmphasis = 1;
    pm1wf->fwHeadFlags = 24;
    pm1wf->dwPTSLow = 0;
    pm1wf->dwPTSHigh = 0;*/

    return S_OK;
}

HRESULT __stdcall filter_in_pin::QueryPinInfo(PIN_INFO *pInfo)
{
#if DUMP_LEVEL > 0
    PRINTF("filter_in_pin::QueryPinInfo called...\n");
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
#if DUMP_LEVEL > 0
    PRINTF("filter_in_pin::QueryDirection called...\n");
#endif
    if(!pPinDir) return E_POINTER;
    *pPinDir = PINDIR_OUTPUT;
    return S_OK;
}

HRESULT __stdcall filter_in_pin::QueryId(LPWSTR *Id)
{
#if DUMP_LEVEL > 0
    PRINTF("filter_in_pin::QueryId called...\n");
#endif
    if(!Id) return E_POINTER;
    *Id = (WCHAR *) CoTaskMemAlloc(sizeof(WCHAR) * 7);
    if(!*Id) return E_OUTOFMEMORY;
    memcpy(*Id, L"Output", sizeof(WCHAR) * 7);
    return S_OK;
}

HRESULT __stdcall filter_in_pin::QueryAccept(AM_MEDIA_TYPE const *pmt)
{
#if DUMP_LEVEL > 0
    PRINTF("filter_in_pin::QueryAccept called...\n");
#endif
    if(
        pmt->majortype != MEDIATYPE_Audio ||
        pmt->subtype != WMMEDIASUBTYPE_MP3 ||
        pmt->formattype != FORMAT_WaveFormatEx)
        return S_FALSE;
    return S_OK;
}

HRESULT __stdcall filter_in_pin::EnumMediaTypes(IEnumMediaTypes **ppEnum)
{
#if DUMP_LEVEL > 0
    PRINTF("filter_in_pin::EnumMediaTypes called...\n");
#endif
    if(!ppEnum) return E_POINTER;
    filter_in_enum_media_types *penum_media_types = new filter_in_enum_media_types;
    if(!penum_media_types) return E_OUTOFMEMORY;
    penum_media_types->reference_count = 1;
    penum_media_types->index = 0;
    *ppEnum = penum_media_types;
    return S_OK;
}

HRESULT __stdcall filter_in_pin::QueryInternalConnections(IPin **apPin, ULONG *nPin)
{
#if DUMP_LEVEL > 0
    PRINTF("filter_in_pin::QueryInternalConnections called...\n");
#endif
    if(!apPin || !nPin) return E_POINTER;
    return E_NOTIMPL;
}

HRESULT __stdcall filter_in_pin::EndOfStream()
{
#if DUMP_LEVEL > 0
    PRINTF("filter_in_pin::EndOfStream called...\n");
#endif
    return E_UNEXPECTED;
}

HRESULT __stdcall filter_in_pin::BeginFlush()
{
#if DUMP_LEVEL > 0
    PRINTF("filter_in_pin::BeginFlush called...\n");
#endif
    return E_UNEXPECTED;
}

HRESULT __stdcall filter_in_pin::EndFlush()
{
#if DUMP_LEVEL > 0
    PRINTF("filter_in_pin::EndFlush called...\n");
#endif
    return E_UNEXPECTED;
}

HRESULT __stdcall filter_in_pin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
#if DUMP_LEVEL > 0
    PRINTF("filter_in_pin::NewSegment called...\n");
#endif
    return E_UNEXPECTED;
}

//----------------------------------------------------------------------------
// filter_in_enum_pins
//----------------------------------------------------------------------------

filter_in_enum_pins::filter_in_enum_pins()
{
#if DUMP_LEVEL > 1
    PRINTF("filter_in_enum_pins::filter_in_enum_pins called...\n");
#endif
}

filter_in_enum_pins::~filter_in_enum_pins()
{
#if DUMP_LEVEL > 1
    PRINTF("filter_in_enum_pins::~filter_in_enum_pins called...\n");
#endif
}

HRESULT __stdcall filter_in_enum_pins::QueryInterface(REFIID riid, void **ppvObject)
{
#if DUMP_LEVEL > 0
    PRINTF("filter_in_enum_pins::QueryInterface called...\n");
#endif
    if(!ppvObject) return E_POINTER;
    if(riid == IID_IUnknown)
    {
        *(IUnknown **) ppvObject = this;
        ((IUnknown *) *ppvObject)->AddRef();
        return S_OK;
    }
    if(riid == IID_IEnumPins)
    {
        *(IEnumPins **) ppvObject = this;
        ((IEnumPins *) *ppvObject)->AddRef();
        return S_OK;
    }
    *ppvObject = NULL;
    return E_NOINTERFACE;
}

ULONG __stdcall filter_in_enum_pins::AddRef()
{
#if DUMP_LEVEL > 1
    PRINTF("filter_in_enum_pins::AddRef called...\n");
#endif
    return ++reference_count;
}

ULONG __stdcall filter_in_enum_pins::Release()
{
#if DUMP_LEVEL > 1
    PRINTF("filter_in_enum_pins::Release called...\n");
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
#if DUMP_LEVEL > 0
    PRINTF("filter_in_enum_pins::Next called...\n");
#endif
    if(!ppPins) return E_POINTER;
    if(cPins != 1 && !pcFetched) return E_INVALIDARG;
    ULONG i;
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
#if DUMP_LEVEL > 0
    PRINTF("filter_in_enum_pins::Skip called...\n");
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
#if DUMP_LEVEL > 0
    PRINTF("filter_in_enum_pins::Reset called...\n");
#endif
    index = 0;
    return S_OK;
}

HRESULT __stdcall filter_in_enum_pins::Clone(IEnumPins **ppEnum)
{
#if DUMP_LEVEL > 0
    PRINTF("filter_in_enum_pins::Clone called...\n");
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
#if DUMP_LEVEL > 1
    PRINTF("filter_in_filter::filter_in_filter called...\n");
#endif
}

filter_in_filter::~filter_in_filter()
{
#if DUMP_LEVEL > 1
    PRINTF("filter_in_filter::~filter_in_filter called...\n");
#endif
}

HRESULT __stdcall filter_in_filter::QueryInterface(REFIID riid, void **ppvObject)
{
#if DUMP_LEVEL > 0
    PRINTF("filter_in_filter::QueryInterface(");
    print(riid);
    PRINTF(", %p) called...\n", ppvObject);
#endif
    if(!ppvObject) return E_POINTER;
    if(riid == IID_IUnknown)
    {
        *(IUnknown **) ppvObject = this;
        ((IUnknown *) *ppvObject)->AddRef();
        return S_OK;
    }
    if(riid == IID_IPersist)
    {
        *(IPersist **) ppvObject = this;
        ((IPersist *) *ppvObject)->AddRef();
        return S_OK;
    }
    if(riid == IID_IMediaFilter)
    {
        *(IMediaFilter **) ppvObject = this;
        ((IMediaFilter *) *ppvObject)->AddRef();
        return S_OK;
    }
    if(riid == IID_IBaseFilter)
    {
        *(IBaseFilter **) ppvObject = this;
        ((IBaseFilter *) *ppvObject)->AddRef();
        return S_OK;
    }
    *ppvObject = NULL;
    return E_NOINTERFACE;
}

ULONG __stdcall filter_in_filter::AddRef()
{
#if DUMP_LEVEL > 1
    PRINTF("filter_in_filter::AddRef called...\n");
#endif
    return ++reference_count;
}

ULONG __stdcall filter_in_filter::Release()
{
#if DUMP_LEVEL > 1
    PRINTF("filter_in_filter::Release called...\n");
#endif
    if(reference_count == 1)
    {
        if(ppin->pconnected)
        {
            ppin->pallocator->Decommit();
            ppin->pallocator->Release();
            ppin->pinput->Release();
            ppin->pconnected->Release();
        }
        if(pclock) pclock->Release();
        delete ppin;
        delete this;
        return 0;
    }
    return --reference_count;
}

HRESULT __stdcall filter_in_filter::GetClassID(CLSID *pClassID)
{
#if DUMP_LEVEL > 0
    PRINTF("filter_in_filter::GetClassID called...\n");
#endif
    return E_FAIL;
}

HRESULT __stdcall filter_in_filter::Stop()
{
#if DUMP_LEVEL > 0
    PRINTF("filter_in_filter::Stop called...\n");
#endif
    state = State_Stopped;
    return S_OK;
}

HRESULT __stdcall filter_in_filter::Pause()
{
#if DUMP_LEVEL > 0
    PRINTF("filter_in_filter::Pause called...\n");
#endif
    state = State_Paused;
    return S_OK;
}

HRESULT __stdcall filter_in_filter::Run(REFERENCE_TIME tStart)
{
#if DUMP_LEVEL > 0
    PRINTF("filter_in_filter::Run called...\n");
#endif
    state = State_Running;
    return S_OK;
}

HRESULT __stdcall filter_in_filter::GetState(DWORD dwMilliSecsTimeout, FILTER_STATE *State)
{
#if DUMP_LEVEL > 0
    PRINTF("filter_in_filter::GetState called...\n");
#endif
    if(!State) return E_POINTER;
    if(state == State_Paused) return VFW_S_CANT_CUE;
    *State = state;
    return S_OK;
}

HRESULT __stdcall filter_in_filter::SetSyncSource(IReferenceClock *pClock)
{
#if DUMP_LEVEL > 0
    PRINTF("filter_in_filter::SetSyncSource called...\n");
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
#if DUMP_LEVEL > 0
    PRINTF("filter_in_filter::GetSyncSource called...\n");
#endif
    if(!pClock) return E_POINTER;
    *pClock = pclock;
    if(*pClock) (*pClock)->AddRef();
    return S_OK;
}

HRESULT __stdcall filter_in_filter::EnumPins(IEnumPins **ppEnum)
{
#if DUMP_LEVEL > 0
    PRINTF("filter_in_filter::EnumPins called...\n");
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
#if DUMP_LEVEL > 0
    PRINTF("filter_in_filter::FindPin called...\n");
#endif
    if(!ppPin) return E_POINTER;
    if(!wcscmp(Id, L"Output"))
    {
        *ppPin = ppin;
        (*ppPin)->AddRef();
        return S_OK;
    }
    *ppPin = NULL;
    return VFW_E_NOT_FOUND;
}

HRESULT __stdcall filter_in_filter::QueryFilterInfo(FILTER_INFO *pInfo)
{
#if DUMP_LEVEL > 0
    PRINTF("filter_in_filter::QueryFilterInfo called...\n");
#endif
    if(!pInfo) return E_POINTER;
    wcscpy_s(pInfo->achName, MAX_FILTER_NAME, name);
    pInfo->pGraph = pgraph;
    if(pInfo->pGraph) pInfo->pGraph->AddRef();
    return S_OK;
}

HRESULT __stdcall filter_in_filter::JoinFilterGraph(IFilterGraph *pGraph, LPCWSTR pName)
{
#if DUMP_LEVEL > 0
    PRINTF("filter_in_filter::JoinFilterGraph called...\n");
#endif
    pgraph = pGraph;
    if(pName)
        wcscpy_s(name, MAX_FILTER_NAME, pName);
    else
        wcscpy_s(name, MAX_FILTER_NAME, L"");
    return S_OK;
}

HRESULT __stdcall filter_in_filter::QueryVendorInfo(LPWSTR *pVendorInfo)
{
#if DUMP_LEVEL > 0
    PRINTF("filter_in_filter::QueryVendorInfo called...\n");
#endif
    if(!pVendorInfo) return E_POINTER;
    return E_NOTIMPL;
}

bool filter_in_filter::data(SIZE_T size, BYTE *p)
{
    IMediaSample *psample;
    HRESULT hr = ppin->pallocator->GetBuffer(&psample, NULL, NULL, 0);
    if(hr != S_OK) return false;
    BYTE *p2;
    psample->GetPointer(&p2);
    *p2 = *p;
    psample->SetActualDataLength(1);
    hr = ppin->pinput->Receive(psample);
    if(hr != S_OK)
    {
        ppin->pallocator->ReleaseBuffer(psample);
        psample->Release();
        return false;
    }
    psample->Release();
    return true;
}

bool filter_in_filter::create(wchar_t const *format, filter_in_filter **ppfilter)
{
#if DUMP_LEVEL > 1
    PRINTF("filter_in_filter::create called...\n");
#endif
    if(!ppfilter) return false;
    filter_in_filter *pfilter = new filter_in_filter;
    if(!pfilter) return false;
    pfilter->ppin = new filter_in_pin;
    if(!pfilter->ppin)
    {
        delete pfilter;
        return false;
    }
    pfilter->reference_count = 1;
    pfilter->pgraph = NULL;
    pfilter->pclock = NULL;
    wcscpy_s(pfilter->name, MAX_FILTER_NAME, L"");
    pfilter->state = State_Stopped;
    pfilter->ppin->pfilter = pfilter;
    pfilter->ppin->pconnected = NULL;
    *ppfilter =  pfilter;
    return true;
}

//----------------------------------------------------------------------------
// filter_in
//----------------------------------------------------------------------------

bool filter_in::create(wchar_t const *format, filter_in **ppfilter)
{
    if(!ppfilter) return false;
    filter_in_filter *pfilter;
    if(!filter_in_filter::create(format, &pfilter)) return false;
    *ppfilter = pfilter;
    return true;
}
