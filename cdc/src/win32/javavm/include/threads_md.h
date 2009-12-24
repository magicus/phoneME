/*
 * @(#)threads_md.h	1.18 06/10/10
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

#ifndef _WIN32_THREADS_MD_H
#define _WIN32_THREADS_MD_H

#include "javavm/include/porting/sync.h"

#ifdef WINCE
#include <winsock.h>
#endif

struct CVMThreadID {
#ifdef CVM_DEBUG
    int where;
#endif
    HANDLE handle;		    /* Win32 thread handle */
    void *stackTop;
    unsigned long id;   	    /* Win32 thread id */
    CVMMutex lock;

    CVMBool suspended;
    CVMBool suspended_in_wait;
    CVMBool suspended_in_mutex_blocked;
    volatile CVMBool is_mutex_blocked;

    HANDLE interrupt_event;	    /* Event signaled on thread interrupt */

    /* SEM */
    CVMMutex wait_mutex;
    HANDLE wait_cv;

    HANDLE suspend_cv;

    int    value;
    CVMBool interrupted;		    /* Shadow thread interruption */
    CVMBool in_wait;
    CVMBool notified;	

    /* IO or wait queue */
    CVMThreadID *next;
    CVMThreadID **prev_p;
	    
    void (*start_proc)(void *);     /* Thread start routine address */
    void *start_parm;		    /* Thread start routine parameter */

#ifdef CVM_ADV_THREAD_BOOST
    HANDLE threadMutex;
    CVMThreadBoostQueue *boost;
#endif

#ifdef WINCE
    int tcp_if;
    int tcp_ttl;
#endif
};

#if _MSC_VER >= 1300
#undef INTERFACE
#endif

/* NOTE: The current redzone and stack sizes below are taken straight from the
         the debug version of Solaris port.  These numbers will probably fine 
	 for the Linux port but may not be optimum.  To obtain the optimum numbers, 
	 one will have to run a static stack analysis written for analyzing the
         assembly codes of CVM on the Linux platform. 
*/
#define CVM_THREAD_MIN_C_STACK_SIZE				(32 * 1024)

/* 
 * The static stack analysis tool doesn't know additional stack requirement 
 * for the execution of library functions.
 * Use a reasonable value for the library function overhead. 
 */
#define CVM_REDZONE_Lib_Overhead                                 (3 * 1024)

#define CVM_REDZONE_ILOOP                                       \
    (5976 + CVM_REDZONE_Lib_Overhead) /*  8.84KB */
#define CVM_REDZONE_CVMimplementsInterface                      \
    (1680 + CVM_REDZONE_Lib_Overhead) /*  4.64KB */
#define CVM_REDZONE_CVMCstackCVMpc2string                       \
    (1680 + CVM_REDZONE_Lib_Overhead) /*  4.64KB */
#define CVM_REDZONE_CVMCstackCVMID_objectGetClassAndHashSafe    \
    (1944 + CVM_REDZONE_Lib_Overhead) /*  4.90KB */
#define CVM_REDZONE_CVMclassLookupFromClassLoader               \
    (6704 + CVM_REDZONE_Lib_Overhead) /*  9.55KB */
#define CVM_REDZONE_CVMclassLink                                \
    (3592 + CVM_REDZONE_Lib_Overhead) /*  6.51KB */
#define CVM_REDZONE_CVMCstackmerge_fullinfo_to_types            \
    (1936 + CVM_REDZONE_Lib_Overhead) /*  4.89KB */
#define CVM_REDZONE_CVMsignalErrorVaList                        \
    (6040 + CVM_REDZONE_Lib_Overhead) /* 8.90KB */
#define CVM_REDZONE_CVMJITirdumpIRNode                          \
    (1024 + CVM_REDZONE_Lib_Overhead) /* 4KB */
#define CVM_REDZONE_disassembleRangeWithIndentation             \
    (1024 + CVM_REDZONE_Lib_Overhead) /* 4KB */
#define CVM_REDZONE_CVMcpFindFieldInSuperInterfaces             \
    (1024 + CVM_REDZONE_Lib_Overhead)

extern void WIN32threadInitStaticState();
extern void WIN32threadDestroyStaticState();

extern CVMInt32 WIN32threadCreate(CVMSize stackSize, CVMInt16 priority,
    void (*func)(void *), void *arg);

extern CVMThreadID * WIN32threadGetSelf();

extern void WIN32threadAttach(CVMThreadID *self);
extern void WIN32threadDetach(CVMThreadID *self);

extern void WIN32threadSetPriority(CVMThreadID *t, CVMInt32 priority);
extern void WIN32threadGetPriority(CVMThreadID *t, CVMInt32 *priority);

/* implementation private */
CVMBool WIN32threadsInit();
void WIN32threadsDestroy();

#endif /* _WIN32_THREADS_MD_H */
