/*
 * @(#)proc_md.h	1.10 06/10/26
 *
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
 *
 */

/* Windows process id's and threads */

#ifdef WINCE
#include <Winbase.h>
#include <Kfuncs.h>
#else
#include <process.h>
#endif
#include <time.h>

static CRITICAL_SECTION __my_mutex__;

#define MUTEX_T         CRITICAL_SECTION *
#define MUTEX_INIT    (InitializeCriticalSection(&__my_mutex__), &__my_mutex__)

#define MUTEX_LOCK(x)  EnterCriticalSection(x)
#define MUTEX_UNLOCK(x) LeaveCriticalSection(x)
#define GET_THREAD_ID() GetCurrentThreadId()
#define THREAD_T        unsigned long
#define PID_T           int
#ifdef WINCE
#define GETPID()        GetCurrentProcessId()
#else
#define GETPID()        getpid()
#endif
#define GETMILLSECS(millisecs) (millisecs=0)

#define popen   _popen
#define pclose  _pclose
#define sleep   _sleep

