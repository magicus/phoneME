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

#ifndef RECORD_BY_DSOUND

#define HDRS_COUNT      8

static HWAVEIN g_hWI         = NULL;
static BOOL    g_bCapturing  = FALSE;
static HANDLE  g_hReadyEvent = NULL;
static HANDLE  g_hRecThread  = NULL;
static BOOL    g_bTerminate  = FALSE;

static WAVEHDR g_Hdrs[ HDRS_COUNT ];

static DWORD WINAPI rec_thread( void* arg )
{
    recorder* h = (recorder*)arg;
    DWORD     r;
    WAVEHDR*  hdr;
    int       len;
    MMRESULT  mmr;
    int       n;

    do
    {
        r = WaitForSingleObject( g_hReadyEvent, 50 );

        if( WAIT_OBJECT_0 == r )
        {
            for( n = 0; n < HDRS_COUNT; n++ )
            {
                if( 0 != ( g_Hdrs[ n ].dwFlags & WHDR_DONE ) ) break;
            }

            if( n < HDRS_COUNT )
            {
                hdr = &( g_Hdrs[ n ] );
                mmr = waveInUnprepareHeader( g_hWI, hdr, sizeof( WAVEHDR ) );
                len = hdr->dwBytesRecorded;

                if( !h->rsl && !g_bTerminate )
                {
                    if( INT_MAX != h->lengthLimit && h->recordLen + len >= h->lengthLimit )
                    {
                        len = h->lengthLimit - h->recordLen;
                    }

                    len = fwrite(hdr->lpData, 1, len, h->recordData);
                    h->recordLen += len;

                    if( INT_MAX != h->lengthLimit && h->recordLen >= h->lengthLimit )
                    {
                        int bytesPerMilliSec = (h->rate * 
                            h->channels * (h->bits >> 3)) / 1000;

                        int ms = bytesPerMilliSec != 0 
                            ? h->recordLen / bytesPerMilliSec 
                            : 0;

                        sendRSL(h->isolateId, h->playerId, ms);
                        h->rsl = TRUE;
                    }
                    else
                    {
                        hdr->dwFlags         = 0;
                        hdr->dwBytesRecorded = 0;

                        mmr = waveInPrepareHeader( g_hWI, hdr, sizeof( WAVEHDR ) );
                        mmr = waveInAddBuffer( g_hWI, hdr, sizeof( WAVEHDR ) );
                    }
                }
            }
        }

    } while( !g_bTerminate && !h->rsl );

    return 0;
}

BOOL initAudioCapture(recorder *h)
{
    MMRESULT      mmr;
    WAVEFORMATEX  wfmt;
    int           i;
    int           rate, bits, channels;
    int           block_size; // = 8192;

    if( NULL != g_hWI ) return FALSE;

    if( NULL == g_hReadyEvent )
        g_hReadyEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

    rate     = h->rate;
    channels = h->channels;
    bits     = h->bits;

    block_size = ENV_REC_SAMPLETIME * (rate * channels * (bits >> 3)) / 1000;

    memset(&wfmt, 0, sizeof(wfmt));
    wfmt.nSamplesPerSec  = rate;
    wfmt.nChannels       = (WORD)channels;
    wfmt.wBitsPerSample  = (WORD)bits;
    wfmt.wFormatTag      = WAVE_FORMAT_PCM;
    wfmt.cbSize          = 0;
    wfmt.nBlockAlign     = (WORD)((wfmt.wBitsPerSample * wfmt.nChannels)/8);
    wfmt.nAvgBytesPerSec = wfmt.nSamplesPerSec * wfmt.nBlockAlign;

    mmr = waveInOpen( &g_hWI, WAVE_MAPPER, &wfmt, 
                      (DWORD_PTR)g_hReadyEvent, 0, CALLBACK_EVENT );

    if( MMSYSERR_NOERROR != mmr )
    {
        return FALSE;
    }

    memset( g_Hdrs, 0, sizeof( g_Hdrs ) );

    g_bTerminate = FALSE;
    g_hRecThread = CreateThread( NULL, 0, rec_thread, h, 0, NULL);

    for( i = 0; i < HDRS_COUNT; i++ )
    {
        g_Hdrs[ i ].lpData = MALLOC( block_size );
        g_Hdrs[ i ].dwBufferLength = block_size;
        mmr = waveInPrepareHeader( g_hWI, &( g_Hdrs[i] ), sizeof( WAVEHDR ) );
        mmr = waveInAddBuffer( g_hWI, &( g_Hdrs[i] ), sizeof( WAVEHDR ) );
    }

    return TRUE;
}

BOOL toggleAudioCapture(BOOL on)
{
    MMRESULT mmr;

    if( !( on ^ g_bCapturing ) ) return on;

    if( on )
    {
        mmr = waveInStart( g_hWI );
    }
    else
    {
        mmr = waveInStop( g_hWI );
    }

    return on;
}

void closeAudioCapture()
{
    int      i;
    MMRESULT mmr;

    g_bTerminate = TRUE;

    WaitForSingleObject( g_hRecThread, INFINITE );
    g_hRecThread = NULL;

    mmr = waveInReset( g_hWI );
    mmr = waveInClose( g_hWI );
    g_hWI = NULL;

    for( i = 0; i < HDRS_COUNT; i++ )
    {
        FREE( g_Hdrs[ i ].lpData );
        g_Hdrs[ i ].dwBufferLength = 0;
    }

    CloseHandle( g_hReadyEvent );
    g_hReadyEvent = NULL;
}

#endif /* RECORD_BY_DSOUND */
