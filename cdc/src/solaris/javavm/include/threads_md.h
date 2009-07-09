/*
 * @(#)threads_md.h	1.41 06/10/10
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
 */

/*
 * Machine-dependent thread definitions.
 */

#ifndef _SOLARIS_THREADS_MD_H
#define _SOLARIS_THREADS_MD_H

#include "portlibs/posix/threads.h"
#include <pthread.h>
#include <thread.h>
#include <synch.h>
#include <setjmp.h>
#include "javavm/include/porting/sync.h"
#ifdef CVM_JVMTI
#include <sys/lwp.h>
#endif

struct CVMThreadID {
    union {
	thread_t threadCookie;
	pthread_t pthreadCookie;
    } cookie;
    void *stackTop;
    mutex_t lock;
    /* support for CVMmutexSetOwner */
    int suspended;
    POSIXMutex locked;
    /* support for Thread.interrupt */
    CVMBool interrupted;
    CVMBool in_wait;
#ifdef SOLARIS_BUG_4377403_FIXED
    sigjmp_buf env;
#endif
#ifdef CVM_ADV_THREAD_BOOST
    CVMThreadBoostQueue *boost;
    CVMThreadID *next;
    CVMThreadID **prev_p;
#ifdef SOLARIS_BOOST_MUTEX
    SOLARISBoostLock *thread_lock;
    SOLARISBoostLock *next_thread_lock;
#endif
#endif
#ifdef CVM_JVMTI
    lwpid_t lwp_id;
#endif
};

#define POSIX_COOKIE(t)		((t)->cookie.pthreadCookie)
#define SOLARIS_COOKIE(t)	((t)->cookie.threadCookie)

#define CVMthreadCreate		POSIXthreadCreate
#define CVMthreadSelf		POSIXthreadGetSelf

#define CVM_THREAD_MIN_C_STACK_SIZE				(32 * 1024)

/*
 * The static stack analysis tool doesn't know additional stack requirement 
 * for the execution of library functions.
 * Use a reasonable value for the library function overhead. 
 */
#define CVM_REDZONE_Lib_Overhead				(3 * 1024)

#define CVM_REDZONE_ILOOP					\
    (5976 + CVM_REDZONE_Lib_Overhead) /*  8.84KB */
#define CVM_REDZONE_CVMimplementsInterface			\
    (1680 + CVM_REDZONE_Lib_Overhead) /*  4.64KB */
#define CVM_REDZONE_CVMCstackCVMpc2string			\
    (1680 + CVM_REDZONE_Lib_Overhead) /*  4.64KB */
#define CVM_REDZONE_CVMCstackCVMID_objectGetClassAndHashSafe	\
    (1944 + CVM_REDZONE_Lib_Overhead) /*  4.90KB */
#define CVM_REDZONE_CVMclassLookupFromClassLoader		\
    (6704 + CVM_REDZONE_Lib_Overhead) /*  9.55KB */
#define CVM_REDZONE_CVMclassLink				\
    (3592 + CVM_REDZONE_Lib_Overhead) /*  6.51KB */
#define CVM_REDZONE_CVMCstackmerge_fullinfo_to_types		\
    (1936 + CVM_REDZONE_Lib_Overhead) /*  4.89KB */
#define CVM_REDZONE_CVMsignalErrorVaList			\
    (6040 + CVM_REDZONE_Lib_Overhead) /* 8.90KB */
#define CVM_REDZONE_CVMJITirdumpIRNode				\
    (1024 + CVM_REDZONE_Lib_Overhead) /* 4KB */
#define CVM_REDZONE_disassembleRangeWithIndentation		\
    (1024 + CVM_REDZONE_Lib_Overhead) /* 4KB */
#define CVM_REDZONE_CVMcpFindFieldInSuperInterfaces             \
    (1024 + CVM_REDZONE_Lib_Overhead)

#endif /* _SOLARIS_THREADS_MD_H */
