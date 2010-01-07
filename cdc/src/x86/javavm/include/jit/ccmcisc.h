/*
 * @(#)ccmcisc.h	1.6 06/10/23
 * 
 * Portions Copyright  2000-2008 Sun Microsystems, Inc. All Rights
 * Reserved.  Use is subject to license terms.
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
 */

/*
 * Porting layer for JIT functions.
 */

#ifndef _INCLUDED_CCMCISC_H
#define _INCLUDED_CCMCISC_H

#include "javavm/include/porting/defs.h"

/*
 * Invoke a static synchronized method from compiled code.
 */
extern void
CVMCCMinvokeStaticSyncMethodHelper(CVMMethodBlock *mb,
				   CVMObjectICell* syncObject);

/*
 * Invoke a nonstatic synchronized method from compiled code.
 */
extern void
CVMCCMinvokeNonstaticSyncMethodHelper(CVMMethodBlock *mb);

/*
 * return from a compiled method.
 */
extern void
CVMCCMreturnFromMethod();

/*
 * return from a sync compiled method.
 */
extern void
CVMCCMreturnFromSyncMethod();

/* glue routine that calls CVMtraceMethodCall */
#ifdef CVM_TRACE
extern void
CVMCCMtraceMethodCallGlue();
#endif

#if defined(CVMJIT_SIMPLE_SYNC_METHODS) && \
    (CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS)
/* Glue routine that calls CVMCCMruntimeSimpleSyncUnlock() */
extern void
CVMCCMruntimeSimpleSyncUnlockGlue(CVMExecEnv *ee, CVMObject* obj);
#endif /* CVMJIT_SIMPLE_SYNC_METHODS */

/* glue routine that calls CVMCCMruntimeGCRendezvous */
extern void
CVMCCMruntimeGCRendezvousGlue();

/* lock and unlock and object */
extern void
CVMCCMruntimeMonitorExitGlue(CVMObject *obj);
extern void
CVMCCMruntimeMonitorEnterGlue(CVMObject *obj);

/* Throw various well known exceptions */
extern void
CVMCCMruntimeThrowNullPointerExceptionGlue();
extern void
CVMCCMruntimeThrowArrayIndexOutOfBoundsExceptionGlue();
extern void
CVMCCMruntimeThrowDivideByZeroGlue();

/* Throw an object */
extern void
CVMCCMruntimeThrowObjectGlue();

/* checkcast */
extern void
CVMCCMruntimeCheckCastGlue();

/* instanceof */
extern void
CVMCCMruntimeInstanceOfGlue();

/* instanceof */
extern void
CVMCCMruntimeCheckArrayAssignableGlue();

/* lookup an interface mb */
extern void
CVMCCMruntimeLookupInterfaceMBGlue();

/* run a clinit */
extern void
CVMCCMruntimeRunClassInitializerGlue();

/* resolve a cp entry */
extern void
CVMCCMruntimeResolveGlue();
extern void
CVMCCMruntimeResolveNewClassBlockAndClinitGlue();
extern void
CVMCCMruntimeResolveGetstaticFieldBlockAndClinitGlue();
extern void
CVMCCMruntimeResolvePutstaticFieldBlockAndClinitGlue();
extern void
CVMCCMruntimeResolveStaticMethodBlockAndClinitGlue();
extern void
CVMCCMruntimeResolveClassBlockGlue();
extern void
CVMCCMruntimeResolveArrayClassBlockGlue();
extern void
CVMCCMruntimeResolveGetfieldFieldOffsetGlue();
extern void
CVMCCMruntimeResolvePutfieldFieldOffsetGlue();
extern void
CVMCCMruntimeResolveSpecialMethodBlockGlue();
extern void
CVMCCMruntimeResolveMethodBlockGlue();
extern void
CVMCCMruntimeResolveMethodTableOffsetGlue();

extern void
CVMCCMruntimeNewGlue();
extern void
CVMCCMruntimeNewArrayGlue();
/* SVMC_JIT c5041613 (dk) 12.02.2004 */
#ifdef CVM_JIT_INLINE_NEWARRAY
extern void
CVMCCMruntimeNewArrayGounlockandslowGlue();
extern void
CVMCCMruntimeNewArrayGoslowGlue();
extern void
CVMCCMruntimeNewArrayBadindexGlue();
#endif /* CVM_JIT_INLINE_NEWARRAY */
extern void
CVMCCMruntimeANewArrayGlue();
extern void
CVMCCMruntimeMultiANewArrayGlue();

#ifdef CVM_JIT_DEBUG
/* Purpose: Poll to see a breakpoint should occur at the current PC. */
extern void
CVMJITdebugPollBreakpointGlue();
#endif

#endif /* _INCLUDED_CCMCISC_H */
