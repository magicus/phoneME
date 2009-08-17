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

#include <limits.h>
#include "mmrecord.h"

#ifdef RECORD_BY_DSOUND

#include <dsound.h>

DWORD WINAPI DirectSoundCaptureThread(void* parms);

static HANDLE                      DSThread;
static int                         captureInProgress = 0;
static LPDIRECTSOUNDCAPTURE        dsCap = NULL;
static LPDIRECTSOUNDCAPTUREBUFFER  captureBuffer = NULL;
static DWORD                       captureSize;
static DWORD                       captureOffset;

BOOL initAudioCapture(recorder *c)
{
    DSCBUFFERDESC cap_bdesc;
    WAVEFORMATEX  wfmt;
    HRESULT r;
    int blockSize;
    int rate, bits, channels;

    if(captureBuffer != NULL) return FALSE;

    rate     = c->rate;
    channels = c->channels;
    bits     = c->bits;

    ZeroMemory(&wfmt, sizeof(wfmt));
    wfmt.nSamplesPerSec = rate;
    wfmt.nChannels = (WORD)channels;
    wfmt.wBitsPerSample = (WORD)bits;
    wfmt.wFormatTag = WAVE_FORMAT_PCM;
    wfmt.cbSize = 0;
    wfmt.nBlockAlign = (WORD)((wfmt.wBitsPerSample * wfmt.nChannels)/8);
    wfmt.nAvgBytesPerSec = wfmt.nSamplesPerSec * wfmt.nBlockAlign;

    blockSize = ENV_REC_SAMPLETIME * (rate * channels * (bits/8)) / 1000;

    ZeroMemory(&cap_bdesc, sizeof(cap_bdesc));
    cap_bdesc.dwSize = sizeof(cap_bdesc);
    cap_bdesc.dwFlags = 0;
    // 200 milli Second buffer
    captureSize = cap_bdesc.dwBufferBytes = blockSize;
    cap_bdesc.lpwfxFormat = (WAVEFORMATEX *)&wfmt;
    captureOffset = 0;

    r = DirectSoundCaptureCreate(NULL, &dsCap, NULL);
    if(r != DS_OK)
    {
        printf("No audio capture device found.\n");
        return FALSE;
    }

    r = dsCap->CreateCaptureBuffer(&cap_bdesc, &captureBuffer, NULL);
    assert(r == DS_OK);

    DSThread = CreateThread(NULL, 0, DirectSoundCaptureThread, c, 0, NULL);
    assert(SetThreadPriority(DSThread, THREAD_PRIORITY_HIGHEST));

    return TRUE;
}

BOOL toggleAudioCapture(BOOL on)
{
    DWORD status = 0;

    OutputDebugString("toggle capture\n");

    assert(captureBuffer->GetStatus(&status) == DS_OK);

    BOOL now_on = ( 0 != (status & DSCBSTATUS_CAPTURING) );

    if( !( on ^ now_on ) ) return on;

    if( on )
    {
        DWORD dwReadCursor;

        assert(captureBuffer->GetCurrentPosition (NULL, &dwReadCursor) == DS_OK);
        captureOffset = dwReadCursor;
        captureBuffer->Start(DSCBSTART_LOOPING);
    }
    else
    {
        captureBuffer->Stop();
    }

    return on;
}


DWORD WINAPI DirectSoundCaptureThread(void *parms)
{
    HRESULT hr;

    LPBYTE lpb1;
    LPBYTE lpb2;
    DWORD dws1;
    DWORD dws2;

    DWORD dwReadCursor;
    DWORD capLen;

    int r = 0;

    recorder* h = (recorder*)parms;

    OutputDebugString("Starting Capture\n");
    captureInProgress = 1;

    while(captureInProgress)
    {
        assert (captureBuffer->GetCurrentPosition (NULL, &dwReadCursor) == DS_OK);

        capLen = (captureOffset > dwReadCursor) 
            ? (captureSize - captureOffset) + dwReadCursor 
            : dwReadCursor - captureOffset;

        lpb1 = lpb2 = NULL;
        dws1 = dws2 = 0;

        hr = captureBuffer->Lock(captureOffset, capLen, 
            (void **)&lpb1, &dws1, (void **)&lpb2, &dws2, 0);

        if (hr == DS_OK)
        {
            // NEED REVISIT: need to check recorded length, 
            // if enough post message?
            int wlen = dws1 + dws2;

            if(h->rsl) break;

            if((INT_MAX != h->lengthLimit) && 
                (h->recordLen >= h->lengthLimit))
            {
                int bytesPerMilliSec = (h->rate * 
                    h->channels * (h->bits >> 3)) / 1000;

                int ms = bytesPerMilliSec != 0 
                    ? h->recordLen / bytesPerMilliSec 
                    : 0;

                sendRSL(h->isolateId, h->playerId, ms);
                h->rsl = TRUE;
            }
            else if((INT_MAX != h->lengthLimit) && 
                ((wlen + h->recordLen) > h->lengthLimit))
            {
                if((int)(h->recordLen + dws1) > 
                    h->lengthLimit)
                {
                    dws2 = 0;
                    dws1 = h->lengthLimit - h->recordLen;
                }
                else
                {
                    dws2 = h->lengthLimit - h->recordLen;
                }
            }
        
            wlen = fwrite(lpb1, 1, dws1, h->recordData);
            if(dws2>0)
                wlen += fwrite(lpb2, 1, dws2, h->recordData);

            h->recordLen += wlen;

            captureBuffer->Unlock(lpb1, dws1, lpb2, dws2);

            captureOffset += dws1+dws2;
            if(captureOffset >= captureSize)
                captureOffset = captureOffset - captureSize;
        }

        Sleep(ENV_REC_SAMPLETIME/2);
    }

    OutputDebugString("Capture Thread - exit\n");

    return 0;
}  


void closeAudioCapture()
{
    DWORD es = 0;

    OutputDebugString("Closing DirectCapture Interface\n");

    captureInProgress = 0;

    while(GetExitCodeThread(DSThread, &es) && (es == STILL_ACTIVE))
    {
        OutputDebugString("Capture Thread not down\n");
        Sleep(ENV_REC_SAMPLETIME);
        captureInProgress = 0;    
        // If the close gets called before the thread is created...
    }

    assert(es == 0);

    OutputDebugString("Capture Thread down\n");

    captureBuffer->Release();
    dsCap->Release();
    captureBuffer = NULL;
    dsCap = NULL;
}

#endif // RECORD_BY_DSOUND

