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

static HWAVEIN g_hWI        = NULL;
static BOOL    g_bCapturing = FALSE;

static void CALLBACK callback_function( HWAVEIN hwi,
                                        UINT uMsg,
                                        DWORD dwInstance,
                                        DWORD dwParam1,
                                        DWORD dwParam2 )
{
}

int initAudioCapture(recorder *c)
{
    MMRESULT      mmr;
    WAVEFORMATEX  wfmt;
    int rate, bits, channels;

    if( NULL != g_hwi ) return 0;

    rate     = c->rate;
    channels = c->channels;
    bits     = c->bits;

    memset(&wfmt, 0, sizeof(wfmt));
    wfmt.nSamplesPerSec  = rate;
    wfmt.nChannels       = (WORD)channels;
    wfmt.wBitsPerSample  = (WORD)bits;
    wfmt.wFormatTag      = WAVE_FORMAT_PCM;
    wfmt.cbSize          = 0;
    wfmt.nBlockAlign     = (WORD)((wfmt.wBitsPerSample * wfmt.nChannels)/8);
    wfmt.nAvgBytesPerSec = wfmt.nSamplesPerSec * wfmt.nBlockAlign;

    mmr = waveInOpen( &g_hWI, WAVE_MAPPER, &wfmt, 
                      (DWORD)callback_function, 0, CALLBACK_FUNCTION );

    if( MMSYSERR_NOERROR != mmr )
    {
        return 0;
    }

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

int closeAudioCapture()
{
}

#endif /* RECORD_BY_DSOUND */
