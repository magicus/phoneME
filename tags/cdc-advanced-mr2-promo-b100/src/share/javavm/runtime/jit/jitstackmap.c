/*
 * @(#)jitstackmap.c	1.29 06/10/10
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
 * Stack map writer for compiled code.
 */
#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/classes.h"
#include "javavm/include/utils.h"
#include "javavm/include/bcutils.h"
#include "javavm/include/jit/jit_defs.h"
#include "javavm/include/jit/jitcontext.h"
#include "javavm/include/jit/jit.h"
#include "javavm/include/jit/jitcodebuffer.h"
#include "javavm/include/jit/jitcomments.h"
#include "javavm/include/jit/jitmemory.h"
#include "javavm/include/jit/jitstats.h"
#include "javavm/include/jit/jitstackmap.h"

#include "javavm/include/clib.h"

#define CVM_COMPILED_MAP_MAX_SMALL_SET_SIZE	16

#ifdef CVM_DEBUG

void
CVMJITdumpStackmapItem(CVMJITCompilationContext* con, CVMJITStackmapItem* smap)
{
    CVMconsolePrintf("-----\nStackmap info at lpc %d, pcOffset %d\n",
		     CVMJITcbufGetLogicalPC(con), smap->pcOffset);
    CVMconsolePrintf("	at physical 0x%x\n", 
	CVMJITcbufLogicalToPhysical(con,CVMJITcbufGetLogicalPC(con)));
    CVMconsolePrintf("%d locals:\n", con->numberLocalWords);
    CVMJITsetDumpElements(con, &smap->localMap);
    CVMconsolePrintf("\n%d spill temps:\n", smap->nTemps);
    if (smap->nTemps > 0){
	CVMJITsetDumpElements(con, &smap->tempMap);
    }
    CVMconsolePrintf("\n%d Java stack words (%d parameters):\n",
	smap->stackDepth, smap->nParams);
    if (smap->stackDepth > 0)
	CVMJITsetDumpElements(con, &smap->stackMap);
    CVMconsolePrintf("\n-----\n");
}

#endif /* CVM_DEBUG */

void
CVMJITcaptureStackmap(CVMJITCompilationContext* con, int pwords )
{
    /*
     * There are three components to the stackmap:
     * locals, spill, and outgoing Java parameters.
     */

    CVMJITStackmapItem*	smap;
    CVMJITprintCodegenComment(("Captured a stackmap here."));
    smap = CVMJITmemNew(con, JIT_ALLOC_CGEN_OTHER, sizeof(CVMJITStackmapItem));
    CVMJITstatsRecordInc(con, CVMJIT_STATS_STACKMAPS_CAPTURED);
    smap->pcOffset = CVMJITcbufGetLogicalPC(con);

    CVMJITsetInit(con, &smap->localMap);
    CVMJITsetCopy(con, &smap->localMap, &con->localRefSet);

    smap->nTemps = con->RMcommonContext.maxSpillNumber+1;
    smap->nParams = pwords;
    CVMJITsetInit(con, &smap->tempMap);
    CVMJITsetInit(con, &smap->stackMap);
    if (smap->nTemps > 0){
	CVMJITsetIntersection(con, &con->RMcommonContext.spillRefSet, &(con->RMcommonContext.spillBusySet));
	CVMJITsetCopy(con, &smap->tempMap, &con->RMcommonContext.spillRefSet);
    }
    smap->stackDepth = con->SMdepth;
    if (smap->stackDepth > 0){
	CVMJITsetCopy(con, &smap->stackMap, &con->SMstackRefSet);
    }
    if (con->stackmapListSize == 0){
	con->stackmapList = smap;
	con->stackmapListTail = smap;
    } else {
        /* 
	 * Add this map at the end of the list.
	 * Since entries are (usually) created in ascending address
	 * order, not much would be gained by an insertion sort.
	 */
	con->stackmapListTail->next = smap;
	con->stackmapListTail = smap;
    }
    con->stackmapListSize += 1;

    /*
    CVMJITdumpStackmapItem(con, smap);
    */
}

static CVMUint16*
setToMap(
    CVMJITCompilationContext*	con,
    CVMJITSet*			set,
    CVMUint16*			map,
    int*       			bitNo,
    int				nSetEntries)
{
    int i;
    int localBitNo = *bitNo;
    CVMBool doesContain;
    for (i = 0; i < nSetEntries; i++){
	CVMJITsetContains(set, i, doesContain);
	if (doesContain){
	    *map |= 1 << localBitNo;
	}
	if (++localBitNo >= 16){
	    localBitNo = 0;
	    map++;
	}
    }
    *bitNo = localBitNo;
    return map;
}

void
CVMJITwriteStackmaps(CVMJITCompilationContext* con)
{
    CVMInt32			nbytes;
    CVMInt32			nlocals;
    CVMInt32			ntemps;
    CVMInt32			nfixed;
    CVMJITStackmapItem*		smap;
    char*			mem;
    CVMCompiledStackMaps*	maps;
    CVMCompiledStackMapEntry*	mapEntry;
    CVMUint16*			endMaps;
    CVMUint16*			largeEntry;
    int				nStateWords;

    if (con->stackmapListSize == 0){
	/* no maps required, so none will be provided */
	return;
    }
    /*
     * First pass to estimate space requirements
     */
    nbytes = sizeof (maps->noGCPoints) + 
	(con->stackmapListSize * sizeof (maps->smEntries[0]));

    nlocals = con->numberLocalWords;
    ntemps  =  con->maxTempWords;
    nfixed = nlocals + ntemps;
    nStateWords = 0;
    for (smap = con->stackmapList; smap != NULL; smap = smap->next){
	if (nfixed + smap->stackDepth > CVM_COMPILED_MAP_MAX_SMALL_SET_SIZE){
	    nStateWords += 2; /* fixed part of CVMCompiledStackMapLargeEntry */
	    nStateWords += (nfixed + smap->stackDepth + 15) / 16;
	}
    }
    nbytes += nStateWords * sizeof (CVMUint16);
    /*
     * Can now allocate nbytes of memory.
     */
    /* On platform with variably sized instruction, such as
     * x86, need to pad the codebuffer before the start of stackmap. */
    mem = (char *)((CVMAddr)(CVMJITcbufGetPhysicalPC(con) + 
                              sizeof(CVMUint32) - 1) &
                   ~(sizeof(CVMUint32) - 1));
    CVMJITcbufGetPhysicalPC(con) = (CVMUint8*)mem;

    /* Make room in the code buffer for the stackmaps just after the
     * generated code */

    CVMJITcbufGetPhysicalPC(con) += nbytes;

    if (CVMJITcbufGetPhysicalPC(con) >= con->codeBufEnd) {
	con->extraStackmapSpace = nbytes;
        /* Fail to compile. We will retry with a larger buffer */
        CVMJITerror((con), CODEBUFFER_TOO_SMALL,
                    "Estimated code buffer too small");
    }

    memset(mem, 0, nbytes);

    endMaps = (CVMUint16*)(mem + sizeof (maps->noGCPoints) + 
	(con->stackmapListSize * sizeof (maps->smEntries[0])));
    maps = (CVMCompiledStackMaps*)mem;
    con->stackmaps = maps;
    largeEntry = endMaps;
    /*
     * Start filling in entries.
     */
    maps->noGCPoints = con->stackmapListSize;
    mapEntry = maps->smEntries;
    for (smap = con->stackmapList; smap != NULL; smap = smap->next){
	int nBits;
	CVMUint16* datap;
	int bitInWord;

	mapEntry->pc = smap->pcOffset;
	nBits = nfixed + smap->stackDepth;
	/*
	CVMconsolePrintf("Compiled stackmap for %C.%M @ pc %d has %d bits\n",
		CVMmbClassBlock(con->mb), con->mb, mapEntry->pc, nBits );
	CVMconsolePrintf("	%d locals, %d temps, %d stack (%d param)\n",
		nlocals, ntemps, smap->stackDepth, smap->nParams);
	*/
	if (nBits <= CVM_COMPILED_MAP_MAX_SMALL_SET_SIZE){
	    /* a normal, small entry */
	    datap = & mapEntry->state;
	    mapEntry->totalSize = nBits;
	    mapEntry->paramSize = smap->nParams;
	} else {
	    /* a large entry */
	    nStateWords = 2 + (nBits + 15) / 16;
	    datap = largeEntry;
	    mapEntry->totalSize = 0xff;
	    mapEntry->state = largeEntry - endMaps;
	    CVMassert(mapEntry->state == largeEntry - endMaps);
	    largeEntry += nStateWords;
	    *datap++ = nBits;
	    *datap++ = smap->nParams;
	}
	/*
	 * datap points at the CMVUint16 array we want to fill in with
	 * nBits bits of data.
	 */
	*datap = 0;
	bitInWord = 0;

	datap = setToMap(con, &smap->localMap, datap, &bitInWord, nlocals);
	datap = setToMap(con, &(smap->tempMap), datap, &bitInWord, ntemps);
	datap = setToMap(con, &(smap->stackMap), datap, &bitInWord,
			 smap->stackDepth);
	mapEntry += 1;
    }
    CVMassert( (char*)maps + nbytes == (char*)largeEntry);
}

/*
 * Returns the logicalPC of the previous stackmap emitted.
 */
CVMInt32
CVMJITgetPreviousStackmapLogicalPC(CVMJITCompilationContext* con)
{
    if (con->stackmapListTail == NULL) {
	return -1;
    } else {
	return con->stackmapListTail->pcOffset;
    }
}
