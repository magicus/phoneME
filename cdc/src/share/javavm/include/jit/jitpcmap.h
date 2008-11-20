/*
 * @(#)jitpcmap.h	1.11 06/10/10
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

#ifndef _INCLUDED_JITPCMAPS_H
#define _INCLUDED_JITPCMAPS_H

#include "javavm/include/defs.h"

/*
 * Tables used to do javaPc <-> compiledPc mappings. One entry exists
 * for each pc mentioned in a CVMExceptionHandler or CVMLineNumberEntry.
 */
struct CVMCompiledPcMap {
    CVMUint16 javaPc;     /* offset from start of bytecodes */
    CVMUint16 compiledPc; /* offset from start of compiled code */
};
struct CVMCompiledPcMapTable {
    CVMUint16         numEntries;
    CVMCompiledPcMap  maps[1];
};

/*
 * Converts a java pc to a compiled pc. The assumption is that this function
 * will only be called with a java pc that we can map. It will assert
 * otherwise.
 */
/* NOTE: CVMpcmapJavaPcToCompiledPc() is not symmetrical with
 * CVMpcmapCompiledPcToJavaPc().  See CVMpcmapCompiledPcToJavaPc() for details.
 */
extern CVMUint8*
CVMpcmapJavaPcToCompiledPc(CVMMethodBlock* mb, CVMUint8* javaPc);

/*
 * Converts a java pc to a compiled pc. The assumption is that this function
 * will only be called with a java pc that we can map. It will assert
 * otherwise.  The difference between CVMpcmapJavaPcToCompiledPcStrict()
 * and CVMpcmapJavaPcToCompiledPc() is that the strict version will map
 * javaPc 0 to the location after the compiled method prologue.  This is
 * needed for the JIT On-Stack Replacement feature to work.
 */
/* NOTE: CVMpcmapJavaPcToCompiledPcStrict() is not symmetrical with
 * CVMpcmapCompiledPcToJavaPc().  See CVMpcmapCompiledPcToJavaPc() for details.
 */
extern CVMUint8*
CVMpcmapJavaPcToCompiledPcStrict(CVMMethodBlock* mb, CVMUint8* javaPc);

/*
 * Converts a compiled pc to a java pc. You can pass in a pc that doesn't
 * have a mapping. In this case the last mapping that is <= the pc
 * is returned. If none exists, then NULL is returned.
 */
/* NOTE: The compiled PC in a compiled frame points to the return instruction
 * rather than the calling instruction as is the case for other frame types.
 * CVMpcmapCompiledPcToJavaPc() is ONLY used in cases where the compiledPC is
 * fetched from a compiled frame and is expected to make the appropriate
 * adjustments so that the computed JavaPc is that of the calling instruction.
 * This means that CVMpcmapCompiledPcToJavaPc() is not symmetrical with
 * CVMpcmapJavaPcToCompiledPc() which does a direct map without any adjustment.
*/
extern CVMUint8*
CVMpcmapCompiledPcToJavaPc(CVMMethodBlock* mb, CVMUint8* compiledPc);

#endif /* _INCLUDED_JITPCMAPS_H */
