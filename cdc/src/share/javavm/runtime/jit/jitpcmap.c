/*
 * @(#)jitpcmap.c	1.13 06/10/10
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

#include "javavm/include/defs.h"
#include "javavm/include/porting/jit/jit.h"
#include "javavm/include/jit/jitpcmap.h"
#include "javavm/include/assert.h"
#include "javavm/include/porting/ansi/stddef.h"
#include "javavm/include/classes.h"

static CVMInt32
CVMpcmapCompiledPcOffsetToJavaPcOffset(CVMCompiledPcMapTable* table,
				       CVMUint16 pc);
static CVMUint16
CVMpcmapJavaPcOffsetToCompiledPcOffset(CVMCompiledPcMapTable* table,
				       CVMUint16 pc);

/*
 * Converts a java pc to a compiled pc. The assumption is that this function
 * will only be called with a java pc that we can map. It will assert.
 * otherwise.
 */
/* NOTE: CVMpcmapJavaPcToCompiledPc() is not symmetrical with
 * CVMpcmapCompiledPcToJavaPc().  See CVMpcmapCompiledPcToJavaPc() for details.
 */
CVMUint8*
CVMpcmapJavaPcToCompiledPc(CVMMethodBlock* mb, CVMUint8* javaPc)
{
    CVMCompiledMethodDescriptor* cmd = CVMmbCmd(mb);
    CVMCompiledPcMapTable* table = CVMcmdCompiledPcMapTable(cmd);
    CVMUint16 javaPcOffset = javaPc - CVMmbJavaCode(mb);
    CVMUint16 compiledPcOffset;
    compiledPcOffset = (javaPcOffset == 0) ? 0 :
        CVMpcmapJavaPcOffsetToCompiledPcOffset(table, javaPcOffset);
    return CVMcmdStartPC(cmd) + compiledPcOffset;
}

/*
 * Converts a java pc to a compiled pc. The assumption is that this function
 * will only be called with a java pc that we can map. It will assert.
 * otherwise.
 */
/* NOTE: CVMpcmapJavaPcToCompiledPc() is not symmetrical with
 * CVMpcmapCompiledPcToJavaPc().  See CVMpcmapCompiledPcToJavaPc() for details.
 */
CVMUint8*
CVMpcmapJavaPcToCompiledPcStrict(CVMMethodBlock* mb, CVMUint8* javaPc)
{
    CVMCompiledMethodDescriptor* cmd = CVMmbCmd(mb);
    CVMCompiledPcMapTable* table = CVMcmdCompiledPcMapTable(cmd);
    CVMUint16 javaPcOffset = javaPc - CVMmbJavaCode(mb);
    CVMUint16 compiledPcOffset =
	CVMpcmapJavaPcOffsetToCompiledPcOffset(table, javaPcOffset);
    return CVMcmdStartPC(cmd) + compiledPcOffset;
}

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
CVMUint8*
CVMpcmapCompiledPcToJavaPc(CVMMethodBlock* mb, CVMUint8* compiledPc)
{
    CVMCompiledMethodDescriptor* cmd = CVMmbCmd(mb);
    CVMCompiledPcMapTable* table = CVMcmdCompiledPcMapTable(cmd);
    CVMUint16 compiledPcOffset;
    CVMUint32 javaPcOffset;
    CVMUint8 *startPC = CVMcmdStartPC(cmd);

    /* Do any neccesary target specific massaging of Compiled PC: */
    compiledPc = CVMJITmassageCompiledPC(compiledPc, startPC);
    /* Compute the Java Bytecode PC: */
    compiledPcOffset = compiledPc - startPC;
    if (compiledPcOffset < CVMpcmapJavaPcOffsetToCompiledPcOffset(table, 0)) {
        /* Must be prologue code.  Map it to the first java byte code PC. */
        return CVMmbJavaCode(mb);
    }
    javaPcOffset =
	CVMpcmapCompiledPcOffsetToJavaPcOffset(table, compiledPcOffset);
    if (javaPcOffset == -1) {
	return NULL;
    } else {
	return CVMmbJavaCode(mb) + javaPcOffset;
    }
}

/*
 * Convert a java pc offset to a compiled pc offset. Offsets are always
 * from the start of the interpreted or java method. The assumption is
 * that this function will only be called with java pc we know we have
 * an entry for.
 */
static CVMUint16
CVMpcmapJavaPcOffsetToCompiledPcOffset(CVMCompiledPcMapTable* table,
				       CVMUint16 pc)
{
    CVMCompiledPcMap* map;
    int i;

    CVMassert(table != NULL);

    map = table->maps;
    for (i = table->numEntries; i > 0; i--, map++) {
	if (pc == map->javaPc) {
	    return map->compiledPc;
	}
    }

    CVMassert(CVM_FALSE); /* lookup are required to succeed */
    return 0;
}

/*
 * Convert a compiled pc offset to a java pc offset. Offsets are always
 * from the start of the interpreted or java method. Unlike
 * CVMpcmapJavaToCompiled(), you can pass in a pc that doesn't have
 * a matching entry. In this case the last entry that is <= the compiled pc
 * is returned. If non exists, then -1 is returned.
 */
static CVMInt32
CVMpcmapCompiledPcOffsetToJavaPcOffset(CVMCompiledPcMapTable* table,
				       CVMUint16 pc)
{
    CVMCompiledPcMap* map;
    CVMUint16 compiledPc;
    int i;
    
    if (table == NULL) {
	return -1;
    }
    
    map = table->maps;
    compiledPc = map->compiledPc;

    /* If there's not going to be a match, we can tell right now */
    if (pc < compiledPc) {
	return -1;
    }

    for (i = table->numEntries; i > 0; i--) {
	CVMUint16 javaPc     = map->javaPc;
        CVMCompiledPcMap *nextMap = map + 1;

	/* If the pc's match, or this is the last entry, then return it */
        /* NOTE: Because of optimizations, it is possible to have consecutive
           MAP PC nodes which map multiple javaPc to the same compiledPc.
           When converting compiledPc to javaPc, we want to find the inner
           most (i.e. largest) javaPc because this is required for exception
           handling purposes.  This unlike the javaPc to compiledPc mapping
           which requires an exact javaPc match.  The exactness of javaPc to
           compiledPc mapping is the reason why we don't have unique compiled
           PCs in the map entries.

           Hence, when (pc == compiledPc), we have to make sure that the next
           pc mapping is not for the same compiledPc.
        */
        if (((pc == compiledPc) && (pc != nextMap->compiledPc)) || i == 1) {
	    return javaPc;
	}
	/*
	 * Peek at the next map. If we are less than the next compiledPC,
	 * then return this map,
	 */
        map = nextMap;
	compiledPc = map->compiledPc;
	if ((pc < compiledPc)) {
	    return javaPc;
	}
    }
    CVMassert(CVM_FALSE); /* we should never get here */
    return -1;
}
