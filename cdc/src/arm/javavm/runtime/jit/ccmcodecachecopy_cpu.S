/*
 * @(#)ccmcodecachecopy_cpu.S	1.10 06/10/10
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
 * Compile together all the code we want to copy into the codecache 
 * so it is easier to copy and guarantee ordering of symbols.
 */

#include "javavm/include/asmmacros_cpu.h"
#include "javavm/include/jit/jitasmconstants.h"

#ifdef __RVCT__
	IMPORT CVMJITfixupFrames
	IMPORT java_lang_NullPointerException_Classblock
	IMPORT java_lang_ArrayIndexOutOfBoundsException_Classblock
	IMPORT java_lang_ArithmeticException_Classblock
	IMPORT java_lang_Object_Classblock
	IMPORT CVMCCMruntimeResolveNewClassBlockAndClinit
	IMPORT CVMCCMruntimeResolveGetstaticFieldBlockAndClinit
	IMPORT CVMCCMruntimeResolvePutstaticFieldBlockAndClinit
	IMPORT CVMCCMruntimeResolveStaticMethodBlockAndClinit
	IMPORT CVMCCMruntimeResolveClassBlock
	IMPORT CVMCCMruntimeResolveArrayClassBlock
	IMPORT CVMCCMruntimeResolveGetfieldFieldOffset
	IMPORT CVMCCMruntimeResolvePutfieldFieldOffset
	IMPORT CVMCCMruntimeResolveSpecialMethodBlock
	IMPORT CVMCCMruntimeResolveMethodBlock
	IMPORT CVMCCMruntimeResolveMethodTableOffset
	IMPORT CVMobjGlobalMicroLockPtr
	IMPORT CVMCCMruntimeMonitorEnter
	IMPORT CVMobjGlobalMicroLockPtr
	IMPORT CVMCCMruntimeMonitorExit
	IMPORT CVMCCMruntimeRunClassInitializer
	IMPORT CVMJITflushCache
#ifndef CVM_JIT_CCM_USE_C_SYNC_HELPER
        IMPORT CVMmicrolockLockImpl
#endif
	IMPORT CVMCCMruntimeGCRendezvous
	IMPORT CVMCCMruntimeThrowClass
	IMPORT CVMCCMruntimeThrowObject
	IMPORT CVMCCMruntimeCheckCast
	IMPORT CVMCCMruntimeInstanceOf
	IMPORT CVMCCMruntimeCheckArrayAssignable
	IMPORT CVMCCMruntimeLookupInterfaceMB
	IMPORT CVMgcAllocNewInstance
	IMPORT CVMthrowOutOfMemoryError
	IMPORT CVMgcAllocNewArrayWithInstanceSize
	IMPORT CVMthrowOutOfMemoryError
	IMPORT CVMthrowNegativeArraySizeException
	IMPORT CVMgcAllocNewArrayWithInstanceSize
	IMPORT CVMthrowOutOfMemoryError
	IMPORT CVMthrowNegativeArraySizeException
	IMPORT CVMglobals
	IMPORT CVMJITexitNative
	IMPORT CVMCCMruntimeMultiANewArray
#ifdef CVM_TRACE
	IMPORT CVMtraceMethodCall
	IMPORT CVMtraceMethodReturn
	IMPORT CVMtraceFramelessMethodCall
	IMPORT CVMtraceFramelessMethodReturn
#endif
#endif /* __RVCT__ */

/*
 * NOTE: Some linker such as the ARM RVCT (v2.2) linker sorts 
 *	 sections by attributes and section name. To make sure
 *	 the CCM copied code in the same order as they are included
 *	 in ccmcodecachecopy_cpu.S, we need to name the sections
 *	 in alphabetical order.
 */
        PRESERVE8
	SET_SECTION_EXEC(s0_ccmcodecachecopy_cpu)


/*************************************************************************
 * This is the start of the code we copy into the start of the code cache.
 *************************************************************************/
	ENTRY(CVMCCMcodeCacheCopyStart)
ENTRY1 ( CVMCCMcodeCacheCopyStart )
	
#include "ccmglue_cpu.S"
#include "ccmallocators_cpu.S"
#include "ccminvokers_cpu.S"
#include "ccmintrinsics_asm_cpu.S"
#include "ccmmath_cpu.S"
#include "../memory_asm_cpu.S"

/*************************************************************************
 * This is the end of the code we copy into the start of the code cache.
 * The .pool is important. It must appear just before this symbol.
 *************************************************************************/
	POOL
	ENTRY(CVMCCMcodeCacheCopyEnd)
ENTRY1( CVMCCMcodeCacheCopyEnd )
