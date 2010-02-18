/*
 * @(#)globalroots.h	1.19 06/10/10
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

#ifndef _INCLUDED_GLOBALROOTS_H
#define _INCLUDED_GLOBALROOTS_H

#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/interpreter.h" /* needed before stacks.h */
#include "javavm/include/stacks.h"

typedef CVMFreelistFrame CVMGlobalRootsFrame;

extern CVMObjectICell*
CVMID_getGlobalRoot(CVMExecEnv* ee);

extern CVMObjectICell*
CVMID_getClassGlobalRoot(CVMExecEnv* ee);

extern CVMObjectICell*
CVMID_getClassLoaderGlobalRoot(CVMExecEnv* ee);

extern CVMObjectICell*
CVMID_getProtectionDomainGlobalRoot(CVMExecEnv* ee);

extern CVMObjectICell*
CVMID_getWeakGlobalRoot(CVMExecEnv* ee);

extern void
CVMID_freeGlobalRoot(CVMExecEnv* ee, CVMObjectICell* glRoot);

extern void
CVMID_freeClassGlobalRoot(CVMExecEnv* ee, CVMObjectICell* glRoot);

extern void
CVMID_freeClassLoaderGlobalRoot(CVMExecEnv* ee, CVMObjectICell* glRoot);

extern void
CVMID_freeProtectionDomainGlobalRoot(CVMExecEnv* ee, CVMObjectICell* glRoot);

extern void
CVMID_freeWeakGlobalRoot(CVMExecEnv* ee, CVMObjectICell* glRoot);

/*
 * Scan global roots for GC, calling 'callback' on each
 */
extern CVMFrameGCScannerFunc CVMglobalrootFrameScanner;

#endif /* _INCLUDED_GLOBALROOTS_H */

