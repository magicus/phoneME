/*
 * @(#)stackmaps.h	1.22 06/10/10
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

#ifndef _INCLUDED_STACKMAPS_H
#define _INCLUDED_STACKMAPS_H

/*
 * The types of errors that may occur during stackmap generation 
 */
typedef enum CVMMapError {
    CVM_MAP_SUCCESS,
    CVM_MAP_OUT_OF_MEMORY,
    CVM_MAP_EXCEEDED_LIMITS, /* branch too long or too many vars
				during rewrite */
    CVM_MAP_CANNOT_MAP       /* stackmap computation failed for
				some other reason (e.g. could not
				understand control flow) */
} CVMMapError;

/*
 * Rewrite method as required prior to stackmap generation.
 * This allows us to defer generation until need, assured that
 * there will be no conflicts, and no need for rewriting, when
 * we do. CVM_TRUE on success, CVM_FALSE otherwise
 */
extern CVMMapError
CVMstackmapDisambiguate(CVMExecEnv* ee, CVMMethodBlock* mb,
			CVMBool didJSRscan);

/*
 * Compute stackmaps and store.
 */
extern CVMStackMaps*
CVMstackmapCompute(CVMExecEnv* ee, CVMMethodBlock* mb,
		   CVMBool doConditionalGcPoints);

/*
 * Find the stackmaps for the mb. Return NULL if there is no stackmaps.
 */
extern CVMStackMaps*
CVMstackmapFind(CVMExecEnv* ee, CVMMethodBlock* mb);

/* Promoted the specified stackmaps to the front of the global list. */
extern void
CVMstackmapPromoteToFrontOfGlobalList(CVMExecEnv *ee, CVMStackMaps *maps);

/* Destroys the stackmaps. */
extern void
CVMstackmapDestroy(CVMExecEnv *ee, CVMStackMaps *maps);

/*
 * Get the stackmap data for a given PC. This is a 16-bit PC value,
 * followed by a bunch of 16-bit chunks.
 */
extern CVMStackMapEntry*
CVMstackmapGetEntryForPC(CVMStackMaps* smaps, CVMUint16 pc);

#ifdef CVM_JVMPI_TRACE_INSTRUCTION
/* Purpose: Maps a Java method's PC offset back to its original PC offset
 *          prior to stackmap disambiguation. */
/* Returns: A negative value if the specified PC offset is for code that
 *          is prepended to the original and hence does not have an original
 *          PC offset equivalent. */
extern CVMInt32
CVMstackmapReverseMapPC(CVMJavaMethodDescriptor *jmd, int newOffset);
#endif

/*
 * Initialization and destruction routines
 */
extern void
CVMstackmapComputerInit();

extern void
CVMstackmapComputerDestroy();

#endif /* _INCLUDED_STACKMAPS_H */
