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

#include<stdio.h>
#include<dshow.h>
#include<initguid.h>
#include<wmsdkidl.h>

#include"audioplayer.hpp"
#include"sourcefilter.hpp"

//#pragma comment(lib,"strmiids.lib")
//#pragma comment(lib,"winmm.lib")

// {AD533349-CBC3-482c-87F0-25F9CB360257}
DEFINE_GUID(CLSID_sourcefilter,
0xad533349, 0xcbc3, 0x482c, 0x87, 0xf0, 0x25, 0xf9, 0xcb, 0x36, 0x2, 0x57);

sourcefilterpin::sourcefilterpin(sourcefilter* pms, HRESULT* phr, ap_callback* cb)
: CSourceStream(TEXT("Output"), phr, pms, L"Output")
, psf( pms )
, pcb( cb )
{
    len  = 0;
    offs = 0;
    end  = false;

    rate     = 44100;
    channels = 1;
}

sourcefilterpin::~sourcefilterpin()
{
    if(len) free(data);
}

HRESULT sourcefilterpin::DecideBufferSize(IMemAllocator*pAlloc,ALLOCATOR_PROPERTIES*ppropInputRequest)
{
    if(!pAlloc||!ppropInputRequest)return E_POINTER;

    CAutoLock al(m_pFilter->pStateLock());

    if(!data)return E_FAIL;
    if(ppropInputRequest->cBuffers==0)
    {
        ppropInputRequest->cBuffers=1;
    }
    ppropInputRequest->cbBuffer=1;
    ALLOCATOR_PROPERTIES ap;
    HRESULT hr=pAlloc->SetProperties(ppropInputRequest,&ap);
    if(hr!=S_OK)return hr;
    if(ap.cbBuffer<1)return E_FAIL;
    return S_OK;
}

HRESULT sourcefilterpin::FillBuffer(IMediaSample*pSample)
{
    if(!pSample)return E_POINTER;

    for(;;)
    {
        {
            CAutoLock al(m_pFilter->pStateLock());
            if(end)return S_FALSE;
            if(offs<len)
            {
                BYTE*p;
                HRESULT hr=pSample->GetPointer(&p);
                if(hr != S_OK)return hr;

                memcpy(p,data+offs,1);

                REFERENCE_TIME rt1=offs;
                REFERENCE_TIME rt2=offs+1;
                offs++;

                pcb->call( len - offs );
                //memcpy(data,data+1,len-1);
                //len--;
                hr=pSample->SetTime(&rt1,&rt2);
                if(hr!=S_OK)return hr;
                hr=pSample->SetSyncPoint(TRUE);
                if(hr!=S_OK)return hr;
                return S_OK;
            }
        }
        Sleep(1);
    }
}

HRESULT sourcefilterpin::GetMediaType(CMediaType*pMediaType)
{
    if(!pMediaType)return E_POINTER;

    CAutoLock al(m_pFilter->pStateLock());

    if(!data)return E_FAIL;

    pMediaType->SetType(&MEDIATYPE_Audio);
    pMediaType->SetSubtype(&WMMEDIASUBTYPE_MP3);
    pMediaType->SetSampleSize(1);
    pMediaType->SetTemporalCompression(FALSE);
    pMediaType->SetFormatType(&FORMAT_WaveFormatEx);

    MPEGLAYER3WAVEFORMAT ml3wf;
    ml3wf.wfx.wFormatTag      = WAVE_FORMAT_MPEGLAYER3;
    ml3wf.wfx.nChannels       = channels;
    ml3wf.wfx.nSamplesPerSec  = rate;
    ml3wf.wfx.nAvgBytesPerSec = 16000;
    ml3wf.wfx.nBlockAlign     = 1;
    ml3wf.wfx.wBitsPerSample  = 0;
    ml3wf.wfx.cbSize          = 12;
    ml3wf.wID                 = 1;
    ml3wf.fdwFlags            = 0;
    ml3wf.nBlockSize          = 1;
    ml3wf.nFramesPerBlock     = 1;
    ml3wf.nCodecDelay         = 0;

    pMediaType->SetFormat((BYTE*)&ml3wf,sizeof(MPEGLAYER3WAVEFORMAT));

    return S_OK;
}

sourcefilter::sourcefilter(LPUNKNOWN lpunk,HRESULT*phr, ap_callback* cb)
: CSource(TEXT("Source Filter"),lpunk,CLSID_sourcefilter)
{
    HRESULT hr=S_OK;
    psfp=new sourcefilterpin(this, &hr, cb);
    if(!psfp)
    {
        if(phr)*phr=E_OUTOFMEMORY;
        return;
    }
    if(hr!=S_OK)
    {
        delete psfp;
        if(phr)*phr=hr;
        return;
    }
}

bool sourcefilter::data(UINT32 len,const UINT8*data)
{
    CAutoLock al(psfp->m_pFilter->pStateLock());

    if(psfp->len)
        psfp->data=(UINT8*)realloc(psfp->data,psfp->len+len);
    else
        psfp->data=(UINT8*)malloc(psfp->len+len);
    memcpy(psfp->data+psfp->len,data,len);

    psfp->len+=len;
    return true;
}

bool sourcefilter::end()
{
    psfp->end=true;
    return true;
}
