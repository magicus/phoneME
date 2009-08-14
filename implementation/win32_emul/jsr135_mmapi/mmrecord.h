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

#include <windows.h>
#include <assert.h>

#include "multimedia.h"

#define ENV_REC_SAMPLETIME 200

//#define     RECORD_BY_DSOUND // uncomment to use DirectSound-based
                               // implementation instead of winapi-based


typedef struct {
    int     isolateId;
    int     playerId;

    int     bits;
    int     rate;
    int     channels;

    BOOL    recInitDone;

    int     lengthLimit;
    BOOL    rsl;

    FILE*   recordData;
    char*   fname;
    int     recordLen;
} recorder;

#ifdef __cplusplus
extern "C" {
#endif

BOOL initAudioCapture(recorder* cap);
BOOL toggleAudioCapture(BOOL on);
void closeAudioCapture();

void sendRSL(int appId, int playerId, long duration);
int  create_wavhead(recorder* h, char *buffer, int buflen);

#ifdef __cplusplus
} // extern "C"
#endif
