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

//=============================================================================

/***    Constants                                                         ***/

#define LAYER          4
#define BITRATE_INDEX 16
#define SAMPFREQ_INDEX 4
#define MPEG_INDEX     2
#define SUBBAND       32

/*** definition of LAYER-IDs ***/

#define MPA_LAYER1  3
#define MPA_LAYER2  2
#define MPA_LAYER3  1

/*** definition of MPEG-Standard ***/

#define MPA_MPEG1   1
#define MPA_MPEG2   0

/*** definition of modes ***/

#define STEREO         0
#define JOINT_STEREO   1
#define DUAL_CHANNEL   2
#define SINGLE_CHANNEL 3

#define NORMAL_STEREO  0
#define INTENSITY_ONLY 1
#define MS_ONLY        2
#define INTENSITY_MS   3

static const short mpa_t_bitrates[MPEG_INDEX][LAYER][BITRATE_INDEX]=
{
 {
  {-1, -1, -1, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, -1 },
  { 0,  8, 16, 24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160, -1 },
  { 0,  8, 16, 24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160, -1 },
  { 0, 32, 48, 56,  64,  80,  96, 112, 128, 144, 160, 176, 192, 224, 256, -1 }
 },
 {
  {-1, -1, -1, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1, -1 }, /* MPEG 1     */
  { 0, 32, 40, 48,  56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, -1 }, /* Layer  III */
  { 0, 32, 48, 56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 384, -1 }, /* Layer  II  */
  { 0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, -1 } /* Layer  I   */
 }
};

static const int mpa_t_samp_freq[MPEG_INDEX][SAMPFREQ_INDEX]=
  { { 22050, 24000, 16000, 0 }, { 44100, 48000, 32000, 0 } };

static const int mpa_t_samples_per_frame[MPEG_INDEX][LAYER]=
  { { 0,  576, 1152, 384 }, { 0, 1152, 1152, 384 } };

//=============================================================================

sourcefilterpin::sourcefilterpin(sourcefilter* pms, HRESULT* phr, ap_callback* cb)
: CSourceStream(TEXT("Output"), phr, pms, L"Output")
, psf( pms )
, pcb( cb )
{
    len  = 0;
    offs = 0;
    end  = false;

    first_packet = true;

    ml3wf.wfx.wFormatTag      = WAVE_FORMAT_MPEGLAYER3;
    ml3wf.wfx.nChannels       = 1;
    ml3wf.wfx.nSamplesPerSec  = 44100;
    ml3wf.wfx.nAvgBytesPerSec = 16000;
    ml3wf.wfx.nBlockAlign     = 1;
    ml3wf.wfx.wBitsPerSample  = 0;
    ml3wf.wfx.cbSize          = sizeof(MPEGLAYER3WAVEFORMAT) - sizeof(WAVEFORMATEX);
    ml3wf.wID                 = 1;
    ml3wf.fdwFlags            = 0;
    ml3wf.nBlockSize          = 1;
    ml3wf.nFramesPerBlock     = 1;
    ml3wf.nCodecDelay         = 0;
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

bool sourcefilter::data(UINT32 len, const BYTE* data)
{
    CAutoLock al(psfp->m_pFilter->pStateLock());

    if( psfp->first_packet )
    {
        const BYTE* frm = data;

        if( ( frm[ 0 ]        ) == 0xFF && // check first 8 bits of syncword
            ( frm[ 1 ] & 0xF6 ) >  0xF0 && // check last 4 and layer != '00'
            ( frm[ 2 ] & 0xF0 ) != 0xF0 && // check bitrate != '1111'
            ( frm[ 2 ] & 0x0C ) != 0x0C && // check sampling != '11'
            ( frm[ 3 ] & 0x03 ) != 0x02 )  // check emphasis != '10'
        {
            int id                 = ( frm[ 1 ] >> 3 ) & 0x01;
            int layer              = ( frm[ 1 ] >> 1 ) & 0x03;
            int protection_bit     = ( frm[ 1 ]      ) & 0x01;
            int bitrate_index      = ( frm[ 2 ] >> 4 ) ;
            int sampling_frequency = ( frm[ 2 ] >> 2 ) & 0x03;
            int padding_bit        = ( frm[ 2 ] >> 1 ) & 0x01;        
            int private_bit        = ( frm[ 2 ]      ) & 0x01;
            int mode               = ( frm[ 3 ] >> 6 ) ;
            int mode_extension     = ( frm[ 3 ] >> 4 ) & 0x03;
            int copyright          = ( frm[ 3 ] >> 3 ) & 0x01;
            int original_home      = ( frm[ 3 ] >> 2 ) & 0x01;
            int emphasis           = ( frm[ 3 ]      ) & 0x03;

            int channels = (mode != SINGLE_CHANNEL) ? 2 : 1 ;

            int bit_rate      = mpa_t_bitrates[ id ][ layer ][ bitrate_index ];
            int sampling_rate = mpa_t_samp_freq[ id ][ sampling_frequency ];
            int nsamples      = mpa_t_samples_per_frame[ id ][ layer ];

            psfp->ml3wf.wfx.nChannels       = channels;
            psfp->ml3wf.wfx.nSamplesPerSec  = sampling_rate;
            psfp->ml3wf.wfx.nAvgBytesPerSec = bit_rate * 1000 / 8;
            psfp->ml3wf.wID                 = id;
            psfp->ml3wf.fdwFlags            = 0;
        }

        psfp->first_packet = false;
    }

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
