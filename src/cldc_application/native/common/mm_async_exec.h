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

#ifndef __mm_async_exec_H__
#define __mm_async_exec_H__

#include <sni.h>
#include <midpServices.h>
#define JAVACALL_MM_ASYNC_GET_RESULT_returns_data(ret_args_)  \
    do { \
        void *args__[] = {ret_args_}; \
        javacall_media_get_event_data(handle__, javacall_event__, ctx__->pResult, sizeof args__ / sizeof args__[0], args__); \
    } while (0) \

#define JAVACALL_MM_ASYNC_GET_RESULT_returns_no_data  (void)ctx__ /* empty */

#define JAVACALL_MM_ASYNC_EXEC(status_,code_,handle_,app_id_,player_id_,javacall_event_,args_) \
do { \
    MidpReentryData* ctx__ = (MidpReentryData *)SNI_GetReentryData(NULL); \
    javacall_result result__ = JAVACALL_FAIL; \
    javacall_handle handle__ = (handle_); \
    int javacall_event__ = (int)(javacall_event_); \
    if (ctx__ == NULL) { \
        result__ = (code_); \
    } else { \
        (result__ = ctx__->status); \
        if (result__ == JAVACALL_OK) { \
            JAVACALL_MM_ASYNC_GET_RESULT_##args_; \
        } \
    } \
    if (result__ == JAVACALL_WOULD_BLOCK) { \
        if (ctx__ == NULL) { \
            if ((ctx__ = (MidpReentryData *)(SNI_AllocateReentryData(sizeof (MidpReentryData)))) == NULL) { \
                (status_) = JAVACALL_OUT_OF_MEMORY; \
                break; \
            } \
        } \
        ctx__->descriptor = MAKE_PLAYER_DESCRIPTOR(app_id_, player_id_, javacall_event__); \
        ctx__->waitingFor = MEDIA_EVENT_SIGNAL; \
        SNI_BlockThread(); \
    } \
    (status_) = result__; \
} while(0)

/**
 * Constructs the descriptor from appId (10 bit), playerId (16 bit) and
 * event code (6 bit)
 */
#define MAKE_PLAYER_DESCRIPTOR(appId_, playerId_, event_) \
    (((((event_)-JAVACALL_EVENT_MEDIA_JAVA_EVENTS_MARKER) & 0x3F) << 26) | \
        (((appId_) & 0x3FF) << 16) | ((playerId_) & 0xFFFF))

#define PLAYER_DESCRIPTOR_EVENT_MASK    MAKE_PLAYER_DESCRIPTOR(-1, -1, JAVACALL_EVENT_MEDIA_JAVA_EVENTS_MARKER)
#endif __mm_async_exec_H__
