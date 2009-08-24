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

#ifndef __JSR135_MULTIMEDIA_AUDIO_H__
#define __JSR135_MULTIMEDIA_AUDIO_H__

#include "multimedia.h"
#include "pcm_out.h"
#include "mm_qsound_audio_conf.h"

#define _MQ234IMP  /**/   
#include "mQ_JSR-234.h"

#define GLOBMAN_INDEX_MAX  20

/* Native porting layer context */
typedef enum
{
    CON135_METADATA,
    CON135_MIDI,
    CON135_PITCH,
    CON135_RATE,
    CON135_RECORD,
    CON135_STOPTIME,
    CON135_TEMPO,
    CON135_VOLUME,

    CONT_MAX
} controls_enum;

/* Native porting layer context */
typedef enum
{
    PL135_UNREALIZED,
    PL135_REALIZED,
    PL135_PREFETCHED,
    PL135_STARTED,
    PL135_CLOSED
} player_state_enum;

typedef struct {
    jc_fmt                  mediaType;
    int                     appId;
    int                     playerId;
    int                     gmIdx;
    int                     playing;
    int                     eom;
    
    /* Buffering variables */
    unsigned char           *dataBuffer;
    int                     dataBufferLen;
    int                     dataPos;
    javacall_int64          streamLen;
    int                     portionLen;
    
    IControl*               controls[CONT_MAX];
    javacall_bool           needProcessHeader;
    player_state_enum       state;

    IPlayControl            *synth;
    IEventTrigger           *doneCallback;
    MQ234_HostBlock         *midiStream;
    IHostStorage            *storage;
    long                    mtime; /* stores media time when state < PREFETCHED */

    HANDLE                  hRealizedEvent;
} ah;

typedef struct
{
    IGlobalManager          *gm;
    IPlayControl            *toneSynth;
    ISynthPerformance       *sp;

    int                     isolateId;      // id of isolate to which this GM belongs
    int                     isolateRefs;
    pcm_handle_t            pcm_handle;

    IEffectModule           *EM135;
    javacall_int64          players135[20];
    int                     numplayers135;
    volatile BOOL           bDelayedMIDI;
    HANDLE                  hMutexREAD;
} globalMan;

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

javacall_result appIDtoGM(int appID, /*OUT*/ int *gmIdx );
void            gmDetach(int gmIdx);

#ifdef __cplusplus
} // extern "C"
#endif //__cplusplus

#endif /* __JSR135_MULTIMEDIA_AUDIO_H__ */
