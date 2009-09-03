/*
 * @(#)sync_md.h	1.13 06/10/10
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

/*
 * Machine-dependent synchronization definitions.
 */

#ifndef _VXWORKS_SYNC_MD_H
#define _VXWORKS_SYNC_MD_H

#include "javavm/include/porting/vm-defs.h"

/*
 * Setup the fastlock type.
 */
#ifdef CVM_ADV_ATOMIC_CMPANDSWAP
#define CVM_FASTLOCK_TYPE  CVM_FASTLOCK_ATOMICOPS
#else
#define CVM_FASTLOCK_TYPE  CVM_FASTLOCK_MICROLOCK
#endif

/*
 * Setup the microlock type.
 */
#define CVM_MICROLOCK_TYPE CVM_MICROLOCK_SCHEDLOCK

/* allow override of default locking setup above */
#include "javavm/include/sync_arch.h"


#ifndef _ASM

#include <vxWorks.h>
#include <semLib.h>

struct CVMMutex {
    SEM_ID m;
};

struct CVMCondVar {
    SEM_ID sem;
};

#ifdef VXWORKS_PRIVATE
extern CVMBool vxworksSyncInit(CVMThreadID *);
extern void vxworksSyncDestroy(CVMThreadID *);
extern void vxworksSyncInterruptWait(CVMThreadID *);
#endif

#endif /* _ASM */
#endif /* _VXWORKS_SYNC_MD_H */
