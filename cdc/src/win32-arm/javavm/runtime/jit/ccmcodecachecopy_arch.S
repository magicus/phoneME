/*
 * @(#)ccmcodecachecopy_arch.S	1.8 06/10/10
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

/*************************************************************************
 * This is the start of the code we copy into the start of the code cache.
 *************************************************************************/

	TEXTAREA

	START_REGION CVMCCMcodeCacheCopyStart

        ; WCE212, WCE300 ARMASM have problems with long symbol names
        ; WCE211, WCE210 are OK
#define ARMASM_LONG_SYMBOL_NAME_BUG
	
#include "javavm/runtime/jit/ccmglue_cpu.S"
#include "javavm/runtime/jit/ccmallocators_cpu.S"
#include "javavm/runtime/jit/ccminvokers_cpu.S"
#include "javavm/runtime/jit/ccmintrinsics_asm_cpu.S"
#include "javavm/runtime/jit/ccmmath_cpu.S"
#include "javavm/runtime/memory_asm_cpu.S"

/*************************************************************************
 * This is the end of the code we copy into the start of the code cache.
 * The LTORG is important. It must appear just before this symbol.
 *************************************************************************/
	LTORG

	END_REGION CVMCCMcodeCacheCopyEnd

	END
