/*
 * @(#)jitinlineinfo.h	1.11 06/10/10
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

#ifndef _INCLUDED_JITINLINEINFO_H
#define _INCLUDED_JITINLINEINFO_H

#include "javavm/include/defs.h"
#include "javavm/include/jit/jit_defs.h"

/*
 * For each inlining performed, mark the end points of the PC's, and the MB
 * inlined.
 */
struct CVMCompiledInliningInfoEntry {
    CVMUint16 pcOffset1;
    CVMUint16 pcOffset2;
    CVMMethodBlock* mb;

    CVMUint8 flags;	/* CVMFrameFlags */

    /* offset in frame of local 0 */
    CVMUint16 firstLocal;

    /* Java PC offset of inlined invoke (parent context) */
    CVMUint16 invokePC;

    /* local for "this" or java.lang.Class of synchronized method */
    CVMUint16 syncObject;

#ifdef EXCEPTION_HANDLERS_IN_INLINED_METHODS
    CVMCompiledPcMapTable *pcMapTable;
#endif
};

struct CVMJITInliningInfoStackEntry {
    CVMUint16 pcOffset1;
    CVMJITMethodContext *mc;
};

struct CVMCompiledInliningInfo {
    /* The maximum inlining depth produced when compiling this method */
    CVMUint32 maxDepth;
    /* Number of entries to search */
    CVMUint32 numEntries;
    CVMCompiledInliningInfoEntry entries[1];
};

#endif /* _INCLUDED_JITINLINEINFO_H */
