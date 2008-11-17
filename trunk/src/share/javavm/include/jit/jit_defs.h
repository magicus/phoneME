/*
 * @(#)jit_defs.h	1.37 06/10/10
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

#ifndef _INCLUDED_JIT_IMPL_DEFS_H
#define _INCLUDED_JIT_IMPL_DEFS_H

#include "javavm/include/porting/defs.h"
#include "javavm/include/porting/vm-defs.h"

/*
 * Some basic definitions to prevent header file circularities.
 */
CVM_STRUCT_TYPEDEF(CVMJITCompilationContext);
CVM_STRUCT_TYPEDEF(CVMJITTargetCompilationContext);
CVM_STRUCT_TYPEDEF(CVMJITLoop);
CVM_STRUCT_TYPEDEF(CVMJITNestedLoop);
CVM_STRUCT_TYPEDEF(CVMJITIRNode);
CVM_STRUCT_TYPEDEF(CVMJITIRRange);
CVM_STRUCT_TYPEDEF(CVMJITIRBlock);
CVM_STRUCT_TYPEDEF(CVMJITIRRoot);
CVM_STRUCT_TYPEDEF(CVMJITIRList);
CVM_STRUCT_TYPEDEF(CVMJITIRListItem);
CVM_STRUCT_TYPEDEF(CVMJITFixupElement);
CVM_STRUCT_TYPEDEF(CVMRMResource);
CVM_STRUCT_TYPEDEF(CVMJITMemChunk);
CVM_STRUCT_TYPEDEF(CVMJITMethodContext);
CVM_STRUCT_TYPEDEF(CVMJITStackmapItem);
CVM_STRUCT_TYPEDEF(CVMJITConstantEntry);
CVM_STRUCT_TYPEDEF(CVMJITFreeBuf);
CVM_STRUCT_TYPEDEF(CVMCodegenComment);
CVM_STRUCT_TYPEDEF(CVMJITStack);
CVM_STRUCT_TYPEDEF(CVMJITIntrinsic);
CVM_STRUCT_TYPEDEF(CVMJITIntrinsicConfigList);
CVM_STRUCT_TYPEDEF(CVMJITRMContext);
CVM_STRUCT_TYPEDEF(CVMJITRMCommonContext);
CVM_STRUCT_TYPEDEF(CVMJITInliningInfoStackEntry);
#ifdef CVM_JIT_COLLECT_STATS
CVM_STRUCT_TYPEDEF(CVMJITStats);
CVM_STRUCT_TYPEDEF(CVMJITGlobalStats);
#endif

#ifdef IAI_CODE_SCHEDULER_SCORE_BOARD
CVM_STRUCT_TYPEDEF(CVMJITCSContext);
#endif

typedef CVMJITIRNode*	CVMJITIRNodePtr;
typedef CVMUint32       CVMRMregset;

/* The address mode types.
 * These are used when computing reachability for constant-reference
 * and branch-destination-reference fixups.
 *
 * CVMJIT_BRANCH_ADDRESS_MODE:      a branch instruction
 * CVMJIT_COND_BRANCH_ADDRESS_MODE: a conditional branch instruction
 * CVMJIT_MEMSPEC_ADDRESS_MODE:     a load/store instruction
 */
enum CVMJITAddressMode {
	CVMJIT_BRANCH_ADDRESS_MODE = 0,
	CVMJIT_COND_BRANCH_ADDRESS_MODE,
	CVMJIT_MEMSPEC_ADDRESS_MODE,
	CVMJIT_FPMEMSPEC_ADDRESS_MODE
};
typedef enum CVMJITAddressMode CVMJITAddressMode;

/* for passing "setcc" argument so it is easier to read */
#define CVMJIT_NOSETCC CVM_FALSE
#define CVMJIT_SETCC   CVM_TRUE

/* for passing "okToDumpCp" argument so it is easier to read */
#define CVMJIT_NOCPDUMP CVM_FALSE
#define CVMJIT_CPDUMPOK CVM_TRUE

/* for passing "okToBranchAroundCpDump" argument so it is easier to read */
#define CVMJIT_NOCPBRANCH CVM_FALSE
#define CVMJIT_CPBRANCHOK CVM_TRUE


#endif /* _INCLUDED_JIT_IMPL_DEFS_H */
