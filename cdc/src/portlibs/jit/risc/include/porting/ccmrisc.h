/*
 * @(#)ccmrisc.h	1.27 06/10/10
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
 * Porting layer for JIT functions.
 */

#ifndef _INCLUDED_CCMRISC_H
#define _INCLUDED_CCMRISC_H

#include "javavm/include/porting/defs.h"

/*************************************************
 * CCM functions: called directly by compiled code,
 * supplied by the platform.
 * Routines with 'Glue' names often save state and
 * marshal parameters as required, then
 * call C routines to do the work, though sometimes they
 * can do some of the work themselves in the simpler cases.
 * It depends on how much effort you want to make retargeting.
 *
 * Parameters are passed in the CVMCPU_ARGn_REG registers as defined
 * in jitrisc.h
 *
 * In some cases, for the convenience of the glue functions,
 * the compiled code does not pass parameters starting in
 * the usual position, but further down the list. This allows
 * a glue function to easily fill in the missing pieces
 * (such as a CVMExecEnv* parameter )
 * and call into the VM service
 * without having to copy things around. PAY ATTENTION!
 * I will use "void" parameters to indicated the gaps.
 */

/*
 * Invoke a static synchronized method from compiled code.
 */
extern void
CVMCCMinvokeStaticSyncMethodHelper(CVMMethodBlock *mb);

/*
 * Invoke a nonstatic synchronized method from compiled code.
 */
extern void
CVMCCMinvokeNonstaticSyncMethodHelper(CVMMethodBlock *mb,
				   CVMObjectICell* syncObject);

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

/* glue routine that calls CVMCCMruntimeGCRendezvous
 *
 * Although not really part of the ccm risc porting layer, all the risc ports
 * need it so we declare it here for convenience.
 */
extern void
CVMCCMruntimeGCRendezvousGlue();

/* lock and unlock and object */
extern void
CVMCCMruntimeMonitorExitGlue(/*void, void, CVMObject *obj*/);
extern void
CVMCCMruntimeMonitorEnterGlue(/*void, void, CVMObject *obj*/);

/* Throw various well known exceptions */
extern void
CVMCCMruntimeThrowNullPointerExceptionGlue();
extern void
CVMCCMruntimeThrowArrayIndexOutOfBoundsExceptionGlue();
extern void
CVMCCMruntimeThrowDivideByZeroGlue();

/* Throw an object */
extern void
CVMCCMruntimeThrowObjectGlue(/* void, void, CVMObject* obj */);

/*
 * The general scheme for calling CVMCCMruntimeCheckCastGlue 
 * and CVMCCMruntimeInstanceOfGlue is:
 *	load-constant CVMCPU_ARG3_REG,&targetClassBlock
 *	move	      CVMCPU_ARG1_REG,objPtr ! and set CC if available
 *	call	      CVMCCMruntime<helper>Glue
 *	<call delay slot if required by the platform>
 *	.word	      0	! GuessCB here
 *	! helper returns here.
 * Note:
 * a) #ifdef CVMCPU_HAS_ALU_SETCC, then the move instruction used will set 
 *    the CC and the helper glue can use the CC to determine immediately 
 *    whether objPtr is null.
 * b) The inline guess cell allows the the glue code to make a quick
 *    test before calling into the system. It  keeps the actual 
 *    classBlock* of the last successful call
 *    from this location.
 * The Glue routines must test the objPtr for null and its classblock
 * for equality with the targetClassBlock before calling the rest of the
 * system.
 * The Glue routines will call one of CVMCCMruntimeInstanceOf or
 * CVMCCMruntimeCheckCast if the quick tests (for null and for 
 * classblock equality with targetClassBlock or the guess)
 * don't yield definitive results.
 * Those routines take the address of the guessCB cell as a parameter,
 * and will re-set it if the test returns success.
 */		

/* checkcast */
/* Returns null or CVMClassBlock*, or throws an exception. */
extern CVMObject*
CVMCCMruntimeCheckCastGlue(/* CVMObject* obj, void, CVMClassBlock* target*/);

/* instanceof */
/* Returns TRUE(1) or FALSE(0) */
extern CVMBool
CVMCCMruntimeInstanceOfGlue(/* CVMObject* obj, void, CVMClassBlock* target*/);

/* CVMCCMruntimeCheckArrayAssignableGlue is called to implement the bytecode
 * aastore.  It is called after an inline
 * test for null object has already been done.
 * It can test for objectClass == arrayClass,
 * and can also tests for arrayClass == array of Object
 * before calling CVMCCMruntimeCheckArrayAssignable.
 * The CVMClassBlock* passed to this routine are as fetched from the
 * respective objects, and may have low-order flag bits set.
 * If the quick checks fail, this routine calls
 * CVMCCMruntimeCheckArrayAssignable.
 */
extern void
CVMCCMruntimeCheckArrayAssignableGlue(/* void, void,
					 CVMClassBlock* arrayClass,
					 CVMClassBlock* objectClass */);
 /* Just returns, or throws an exception. */

/* lookup an interface mb */
/* At a fixed offset from the call to this routine is
 * an inline 'guess' cell (as in the example above).
 * This contains a word-size integer index 'i' such that
 *   CVMcbInterfacecb(CVMobjectGetClass(obj),i) ==  CVMmbClassBlock(intf)
 * at the last call from this location.
 * The guess is maintained by CVMCCMruntimeLookupInterfaceMB. The
 * glue code may opt to use this for a faster lookup before calling system
 * code.
 */
extern CVMMethodBlock *
CVMCCMruntimeLookupInterfaceMBGlue(/* void, CVMObject *obj,
					    CVMMethodBlock *intf */);

/* Run a clinit if necessary.
 * Needs to call CVMCCMruntimeRunClassInitializer.
 * This can return in a variety of ways:
 * -- if the class gets initialized in this thread, then
 *    the system returns directly to the calling compiled code. This 
 *    is because of the way we avoid C recursion at this point by making
 *    it into Java stack recursion. In order to do this, make sure to
 *    update the return PC before flushing it to the Java stack frame.
 * -- if the class was already initialized, then
 *    CVMCCMruntimeRunClassInitializer will return TRUE to the Glue code.
 *    The Glue code can rewrite the call instruction into a nop
 *    (but not the instruction before it, which may be at the beginning
 *    of a block and thus liable for rewriting for a GC rendezvous).
 * -- if the class is being initialized by the current thread (and yes,
 *    recursion does happen in initialization), then
 *    CVMCCMruntimeRunClassInitializer will return TRUE to the Glue code.
 *    In this case the Glue code just returns without rewriting.
 * -- an exception occurs and the stack is unwound using the usual exception
 *    delivery mechanism.
 */
extern void
CVMCCMruntimeRunClassInitializerGlue(/* void, void, CVMClassBlock* cb */);

/* resolve a cp entry
 *
 * Although not really part of the ccm risc porting layer, all the risc ports
 * need it so we declare it here for convenience.
 */
extern void
CVMCCMruntimeResolveGlue();

/*
 * The following all call different underlying systems functions,
 * but are otherwise identical.
 * The CVMInt32 parameter is the index into the containing
 * class' constant pool of the entity in need of resolution.
 *
 * The calling paradigm for all of them is:
 *
 *      load          Rx,cachedConstant ! use PC-relative address if possible
 * patchHere:
 *	load-constant CVMCPU_ARG3_REG,cpIndex
 *      call    CVMCCMruntimeResolve<entryType><AndClinit>Glue
 *	<call delay slot if required by the platform>
 * cachedConstant:
 *      .word   -1
 * resolveReturn1:
 *      load          Rx,cachedConstant ! use PC-relative address if possible
 * resolveReturn2:
 *
 * The Glue code needs to call CVMCCMruntimeResolve<entryType><AndClinit>,
 * after having adjusted the return address
 * to return to resolveReturn1.
 * It will also form the address of cachedConstant and pass it to
 * the system to be filled in. 
 *
 * As in the case with CVMCCMruntimeRunClassInitializer, the system
 * will behave in one of a number of ways:
 * -- if the class gets initialized in this thread, then
 *    the system returns directly to the calling compiled code. This 
 *    is because of the way we avoid C recursion at this point by making
 *    it into Java stack recursion. In order to do this, make sure to
 *    update the return PC before flushing it to the Java stack frame,
 *    so that execution of the compiled code is resumed at resolveReturn1.
 * -- if the class was already initialized, then
 *    the system code will return TRUE to the Glue code.
 *    The Glue code can rewrite the instruction at 'patchHere'
 *    (but not the instruction before it, which may be at the beginning
 *    of a block and thus liable for rewriting for a GC rendezvous)
 *    into a branch to 'resolveReturn2.' (It also works to patch the
 *    call if that's easier because of needing to atomically provide
 *    a branch and its delay slot.)
 * -- if the class is being initialized by the current thread (and yes,
 *    recursion does happen in initialization), then
 *    CVMCCMruntimeRunClassInitializer will return TRUE to the Glue code.
 *    In this case the Glue code should return to 'resolveReturn1' 
 *    without rewriting.
 * -- an exception occurs and the stack is unwound using the usual exception
 *    delivery mechanism.
 * In the non-exceptional cases, the cache word is filled in with 
 * the required value.
 *
 */
extern void
CVMCCMruntimeResolveNewClassBlockAndClinitGlue(/* void, void, CVMInt32 */);
extern void
CVMCCMruntimeResolveGetstaticFieldBlockAndClinitGlue(/* void, void, CVMInt32 */);
extern void
CVMCCMruntimeResolvePutstaticFieldBlockAndClinitGlue(/* void, void, CVMInt32 */);
extern void
CVMCCMruntimeResolveStaticMethodBlockAndClinitGlue(/* void, void, CVMInt32 */);
extern void
CVMCCMruntimeResolveClassBlockGlue(/* void, void, CVMInt32 */);
extern void
CVMCCMruntimeResolveArrayClassBlockGlue(/* void, void, CVMInt32 */);
extern void
CVMCCMruntimeResolveGetfieldFieldOffsetGlue(/* void, void, CVMInt32 */);
extern void
CVMCCMruntimeResolvePutfieldFieldOffsetGlue(/* void, void, CVMInt32 */);
extern void
CVMCCMruntimeResolveSpecialMethodBlockGlue(/* void, void, CVMInt32 */);
extern void
CVMCCMruntimeResolveMethodBlockGlue(/* void, void, CVMInt32 */);
extern void
CVMCCMruntimeResolveMethodTableOffsetGlue(/* void, void, CVMInt32 */);

/*
 * Memory allocators.
 * These can all cause a GC.
 */

/* Allocate a plain object.
 * Can just call CVMCCMruntimeNew.
 * It is also possible for it to try to do the allocation in assembler
 * in most cases, as long as you have a quick way to acquire an uncontested
 * lock.
 */
extern CVMObject*
CVMCCMruntimeNewGlue(/*void, CVMClassBlock *objClass*/);

/* Allocate a single-dimension array of non-reference 
 * (e.g. byte[], char[], float[] )
 * We know the basic array CB's already exist because they are ROMized.
 *
 * Can just call CVMCCMruntimeNewArray.
 * It is also possible for it to try to do the allocation in assembler
 * in most cases, as long as you have a quick way to acquire an uncontested
 * lock.
 */
extern CVMObject*
CVMCCMruntimeNewArrayGlue(/*CVMInt32 elementSize, CVMInt32 dimension,
			    CVMClassBlock* arrayCB*/);

/* Allocate a single-dimension array of reference.
 * As with the other allocaters, this can just call CVMCCMruntimeANewArray,
 * or can attempt to do the allocation itself.
 */
extern CVMObject*
CVMCCMruntimeANewArrayGlue(/*void, CVMInt32 dimension,
			     CVMClassBlock* arrayCB*/);

/* Allocate a multi-dimension array of reference.
 * (The array of dimensions is actually on the Java stack,
 * as it was evaluated as a Java parameter list.)
 */
extern CVMObject*
CVMCCMruntimeMultiANewArrayGlue(/* void, CVMInt32 nDimensions,
				CVMClassBlock* arrayCB, CVMIn32* dimArray*/);

#endif /* _INCLUDED_CCMRISC_H */

