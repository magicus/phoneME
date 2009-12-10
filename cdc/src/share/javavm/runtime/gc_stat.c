/*
 * @(#)gc_stat.c	1.10 06/10/10
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

#include "javavm/include/gc_stat.h"
#include "javavm/include/globals.h"
#include "javavm/include/porting/int.h"
#include "javavm/include/porting/time.h"
#include "javavm/include/porting/doubleword.h"

/*
 * Print the statistics for the last GC.
 */
static void 
CVMgcstatPrintGCStat(CVMInt64 curGCTime) 
{
    CVMExecEnv* ee = CVMgetEE();

    CVMconsolePrintf("\nGC Statistics\n");
    CVMconsolePrintf("Current GC pause time: %d ms\n", 
		     CVMlong2Int(curGCTime));
    CVMconsolePrintf("Total GC pause time: %d ms\n", 
		     CVMlong2Int(CVMglobals.totalGCTime));
    CVMconsolePrintf("Free memory before GC: %d bytes\n", 
		     CVMlong2Int(CVMglobals.initFreeMemory));
    CVMconsolePrintf("Free memory after GC: %d bytes\n", 
		     CVMlong2Int(CVMgcFreeMemory(ee)));
    CVMconsolePrintf("Total memory: %d bytes\n", 
		     CVMlong2Int(CVMgcTotalMemory(ee)));
    CVMconsolePrintf("\n");
    
}

void 
CVMgcstatStartGCMeasurement(void) 
{
    if (CVMglobals.measureGC) {
	CVMglobals.startGCTime = CVMtimeMillis();
	CVMglobals.initFreeMemory = CVMgcFreeMemory(CVMgetEE());
    }    
}

void 
CVMgcstatEndGCMeasurement(void) 
{
    if (CVMglobals.measureGC) {	
	CVMInt64 gcTime;

	gcTime = CVMlongSub(CVMtimeMillis(), CVMglobals.startGCTime);
	CVMglobals.totalGCTime = CVMlongAdd(CVMglobals.totalGCTime, gcTime);
	
	CVMgcstatPrintGCStat(gcTime);
    }
}

void 
CVMgcstatDoGCMeasurement(CVMBool doGCMeasurement) 
{
    CVMglobals.measureGC = doGCMeasurement;
}
