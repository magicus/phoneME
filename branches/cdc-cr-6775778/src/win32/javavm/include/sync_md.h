/*
 * @(#)sync_md.h	1.18 06/10/10
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
 * Machine-dependent synchronization definitions.
 */

#ifndef _WIN32_SYNC_MD_H
#define _WIN32_SYNC_MD_H

#include "javavm/include/porting/vm-defs.h"
#include "javavm/include/sync_arch.h"


#ifndef _ASM
#include <windows.h>

struct CVMMutex {
#if 1
    CRITICAL_SECTION crit;
#else
    HANDLE h;
#endif
};

typedef struct {
    CVMThreadID *waiters;
    CVMThreadID **last_p;
} CVMDQueue;

struct CVMCondVar {
    CVMDQueue q;
};

#ifdef CVM_ADV_THREAD_BOOST
struct CVMThreadBoostQueue {
    int nboosters;
    CVMDQueue boosters;
    CVMMutex lock;
    HANDLE handles[2];
#ifdef CVM_DEBUG_ASSERTS
    CVMBool boosting;
#endif
};

struct CVMThreadBoostRecord {
    CVMThreadBoostQueue *currentRecord;
};
#endif

#endif /* !_ASM */
#endif /* _WIN32_SYNC_MD_H */
