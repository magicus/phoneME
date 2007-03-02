/*
 *
 *
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

#include <stdio.h>
#include <stdlib.h> /* malloc() */
#include <pthread.h>

struct _THREAD_WAIT_BLOCK {
    long handle;    // an abstract handle
    int waitFor;    // a type of a signal, "0" for threadId
    int blockCount; // number of opened sync blocks
    int state;  // (0 - wait, 1 - shell go, -1 - interrupted)
    pthread_mutex_t mutex;  // a pthread mutex
    pthread_cond_t cond;    // a pthread condition variable
    struct _THREAD_WAIT_BLOCK *next; // link to the next block
};

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static struct _THREAD_WAIT_BLOCK *waiting_list = NULL;

/* Starts a sync block */
int jumpThreadSyncBegin(long handle, int waitFor) {
}

int jumpThreadSyncEnd(long handle, int waitFor) {
}
int jumpThreadSyncNotify(long handle, int waitFor) {
}
int jumpThreadSyncNotifyAll(long handle, int waitFor) {
}

/* return 0 if being notified, -1 when interrupted */
int jumpThreadSyncWait(long handle, int waitFor) {
}


