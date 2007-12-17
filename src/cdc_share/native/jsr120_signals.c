/*
 *
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

#include <kni.h>
#include <sni.h>

static CVMCondVar _condvar;
static CVMMutex   _mutex;
static int _handle;
static int _signal;

static int is_mutex_inited = 0;

void jsr120_init_signal() {
    is_mutex_inited = 1;
    CVMmutexInit(&_mutex);
    CVMcondvarInit(&_condvar, &_mutex);
}

void jsr120_finalize_signal() {
    if (!is_mutex_inited) {
       is_mutex_inited = 0;
       CVMmutexDestroy(&_mutex);
       CVMcondvarDestroy(&_condvar);
    }
}

void jsr120_wait_for_signal(int handle, int signal) {
    if (!is_mutex_inited) { 
        jsr120_init_signal(); 
    }
    CVMmutexLock(&_mutex);
    do {
        CVMcondvarWait(&_condvar, &_mutex, 0);
    } while ( ((_handle != 0) && (handle != _handle)) || 
              ((_signal != 0) && (signal != _signal)) ) ;
    CVMmutexUnlock(&_mutex);
}

void jsr120_throw_signal(int handle, int signal) {
    if (is_mutex_inited) {
        CVMmutexLock(&_mutex);
        _handle = handle;
        _signal = signal;
        CVMcondvarNotifyAll(&_condvar);
        CVMmutexUnlock(&_mutex);
    }
}
