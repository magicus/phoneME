/*
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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
#ifndef _JUMP_THREAD_SYNC_H
#define _JUMP_THREAD_SYNC_H

typedef struct _JUMPThreadMutex *JUMPThreadMutex;
typedef struct _JUMPThreadCond *JUMPThreadCond;

#ifdef __cplusplus
extern "C" {
#endif

enum {
    JUMP_SYNC_WOULD_BLOCK = 1,
    JUMP_SYNC_OK = 0,
    JUMP_SYNC_FAILURE = -1,
    JUMP_SYNC_OUT_OF_MEMORY = -2,
    JUMP_SYNC_INTERRUPTED = -3,
    JUMP_SYNC_TIMEOUT = -4,

};


JUMPThreadMutex jumpThreadMutexCreate();
void jumpThreadMutexDestroy(JUMPThreadMutex mutex);
int jumpThreadMutexLock(JUMPThreadMutex mutex);
int jumpThreadMutexTrylock(JUMPThreadMutex mutex);
int jumpThreadMutexUnlock(JUMPThreadMutex mutex);

JUMPThreadCond jumpThreadCondCreate(JUMPThreadMutex mutex);
JUMPThreadMutex jumpThreadCondGetMutex(JUMPThreadCond cond);
void jumpThreadCondSetMutex(JUMPThreadCond cond, JUMPThreadMutex mutex);
void jumpThreadCondDestroy(JUMPThreadCond cond);
int jumpThreadCondWait(JUMPThreadCond cond, long millis);
int jumpThreadCondSignal(JUMPThreadCond cond);
int jumpThreadCondBroadcast(JUMPThreadCond cond);
#ifdef __cplusplus
}
#endif

#endif /* #ifdef _JUMP_THREAD_SYNC_H */

