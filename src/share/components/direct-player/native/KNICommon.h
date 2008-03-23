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

#ifndef __KNICommon_H__
#define __KNICommon_H__

#include <kni.h>
#include <jsrop_memory.h>
#include <jsrop_logging.h>

#include "javacall_defs.h"
#include "javacall_multimedia.h"

#define MMP_MAX_TEMPBUF_SIZE 256

#define MMP_MALLOC(_size_)      JAVAME_MALLOC((_size_))
#define MMP_FREE(_p_)           JAVAME_FREE((_p_))
#define MMP_GET_FREE_SPACE()    (0)

#define MMP_DEBUG_STR(_x_) \
    REPORT_INFO(LC_MMAPI, (_x_))
#define MMP_DEBUG_STR1(_x_, _p1_) \
    REPORT_INFO1(LC_MMAPI, (_x_), (_p1_))
#define MMP_DEBUG_STR2(_x_, _p1_, _p2_) \
    REPORT_INFO2(LC_MMAPI, (_x_), (_p1_), (_p2_))
#define MMP_DEBUG_STR3(_x_, _p1_, _p2_, _p3_) \
    REPORT_INFO3(LC_MMAPI, (_x_), (_p1_), (_p2_), (_p3_))
#define MMP_DEBUG_STR4(_x_, _p1_, _p2_, _p3_, _p4_) \
    REPORT_INFO4(LC_MMAPI, (_x_), (_p1_), (_p2_), (_p3_), (_p4_))

/* Java MMAPI Player Status */
#define UNREALIZED  100
#define REALIZED    200
#define PREFETCHED  300
#define STARTED     400
#define CLOSED      0

/* Java MMAPI recording state */
#define RECORD_COMMIT   5
#define RECORD_RESET    4
#define RECORD_STOP     3   /* Pause & Stop has same state context during finalize */
#define RECORD_PAUSE    3
#define RECORD_START    2
#define RECORD_INIT     1
#define RECORD_CLOSE    0

/* Mime type strings */
#define VIDEO_MPEG4_MIME    "video/mp4v-es"
#define VIDEO_3GPP_MIME     "video/3gpp"
#define AUDIO_MIDI_MIME     "audio/midi"
#define AUDIO_WAV_MIME      "audio/x-wav"
#define AUDIO_MP3_MIME      "audio/mpeg"
#define AUDIO_AMR_MIME      "audio/amr"
#define AUDIO_MPEG4_MIME    "audio/mp4a-latm"
#define AUDIO_TONE_MIME     "audio/x-tone-seq"

typedef struct _KNIPlayerInfo {
    int appId;              /* Unique Application ID */
    int playerId;           /* Unique player Id inside application */
    long contentLength;     /* Content length */
    long offset;            /* Current offset of buffer */
    long firstPacketSize;   /* First packet Size */
    jboolean needMoreData;  /* need more media data immediatelly */
    int isAcquire;          /* Is this player acquire devices? */
    int isDirectFile;       /* Is from direct file? */
    int isForeground;           /* Is in foreground? */
    int recordState;            /* State of recording */
    void* hBuffer;          /* Handle of buffer */
    void* pNativeHandle;    /* OEM can use this field to extend handle */
} KNIPlayerInfo;

javacall_result javacall_media_get_event_data(javacall_handle handle, int eventType, void *pResult, int numArgs, void *args[]);

#ifdef ENABLE_CDC

#define JAVACALL_ASYNC_EXEC(status_,code_,handle_,descr_,midp_event_,javacall_event_,args_) \
    status_ = code_
#else

#define JAVACALL_ASYNC_GET_RESULT_returns_data(ret_args_)  \
    do { \
        void *args__[] = ret_args_; \
        javacall_media_get_event_data(handle__, javacall_event__, ctx__->pResult, sizeof args__ / sizeof args__[0], args__); \
    } while (0) \

#define JAVACALL_ASYNC_GET_RESULT_returns_no_data  (void)ctx__ /* empty */

#define JAVACALL_ASYNC_EXEC(status_,code_,handle_,descr_,midp_event_,javacall_event_,args_) \
do { \
    MidpReentryData* ctx__ = (MidpReentryData *)SNI_GetReentryData(NULL); \
    javacall_result result__ = JAVACALL_FAIL; \
    javacall_handle handle__ = (handle_); \
    int javacall_event__ = (int)(javacall_event_); \
    if (ctx__ == NULL) { \
        result__ = (code_); \
    } else { \
        ((status_) = ctx__->status); \
        JAVACALL_ASYNC_GET_RESULT_##args_; \
    } \
    if (result__ == JAVACALL_WOULD_BLOCK) { \
        if (ctx__ == NULL) { \
            if ((ctx__ = (MidpReentryData *)(SNI_AllocateReentryData(sizeof (MidpReentryData)))) == NULL) { \
                (status_) = JAVACALL_NO_MEMORY; \
                break; \
            } \
        } \
        ctx__->descriptor = (int)(descr_); \
        ctx__->waitingFor = (int)(midp_event_); \
        SNI_BlockThread(); \
    } \
    (status_) = result__; \
} while(0)

#endif /* ENABLE_CDC */

#endif
