/*
 * @(#)ccmglue_cpu.S	1.41 06/10/29
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
 * Copyright 2005 Intel Corporation. All rights reserved.  
 */

/*
 * The file includes glue code that aids in the calling of some the
 * CCM C helpers, and also do shortcuts to try to avoid having
 * to call the helpers.
 */
	
#include "javavm/include/asmmacros_cpu.h"
#include "javavm/include/jit/jitasmmacros_cpu.h"
#include "javavm/include/jit/jitasmconstants.h"
#include "javavm/include/porting/jit/jit.h"
#include "javavm/include/sync.h"

        /* Monitor enter/exit code may post an event which will
         * push a JNI local frame on the stack.  Need to have frame->TOS
         * consistent with JSP
         */
#ifdef CVM_JVMTI
#define FLUSH_TOS 	str     JSP, [JFP, #OFFSET_CVMFrame_topOfStack]
#else
#define FLUSH_TOS
#endif

/*
 * NOTE: Some linker such as the ARM RVCT (v2.2) linker sorts 
 *	 sections by attributes and section name. To make sure
 *	 the CCM copied code in the same order as they are included
 *	 in ccmcodecachecopy_cpu.S, we need to name the sections
 *	 in alphabetical order.
 */
	SET_SECTION_EXEC(s1_ccmglue_cpu)

/*
 * Make a pc-relative branch instruction. Used for patching and instruction
 * to jump around generated code.
 */	
#ifdef __RVCT__
#define MAKE_BRANCH(offset)					\
    ((14 << 28) :OR:			 /* COND_AL */		\
     (5 << 25) :OR:			 /* branch opcode */	\
     (((offset - 8) >> 2) :AND: 0x00ffffff)) /* branch offset */ 
#else
#define MAKE_BRANCH(offset)					\
    ((14 << 28) |			 /* COND_AL */		\
     (5 << 25) |			 /* branch opcode */	\
     (((offset - 8) >> 2) & 0x00ffffff)) /* branch offset */ 
#endif

/*
 * Macro for calling a helper. Sets up the ccee and ee as first two
 * arguments and flushes the pc to the frame and the frame to the 
 * interpreter stack.
 */
#ifndef __RVCT__
#define CALL_HELPER_AND_PASS_EE(HELPER)					\
	mov	a1,  sp ;						\
	ldr	a2,  [sp,  #OFFSET_CVMCCExecEnv_ee] ;			\
        str	lr,  [JFP, #OFFSET_CVMCompiledFrame_PC] ;		\
        str     JFP, [a2, #OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame] ; \
	BRANCH_TO_VM_FUNCTION(HELPER)
#else
	MACRO
	CALL_HELPER_AND_PASS_EE0 $HELPER
	IMPORT $HELPER
	mov	a1,  sp
	ldr	a2,  [sp,  #OFFSET_CVMCCExecEnv_ee]
        str	lr,  [JFP, #OFFSET_CVMCompiledFrame_PC]
        str     JFP, [a2, #OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame]
	BRANCH_TO_VM_FUNCTION($HELPER)
	MEND

#define CALL_HELPER_AND_PASS_EE(HELPER)		\
	CALL_HELPER_AND_PASS_EE0 HELPER
#endif

/*
 * Macro for calling a helper. Sets up the ccee as the first
 * argument and flushes the pc to the frame and the frame to the 
 * interpreter stack. It also passes the address of the cache or
 * guess value in a4.
 */
#ifndef __RVCT__
#define CALL_HELPER_AND_PASS_CACHE_ADDR(HELPER)				\
	ldr	a1,  [sp,  #OFFSET_CVMCCExecEnv_ee] ;			\
        str	lr,  [JFP, #OFFSET_CVMCompiledFrame_PC] ;		\
        str     JFP, [a1, #OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame] ; \
	mov	a1,  sp ;						\
	BRANCH_TO_VM_FUNCTION(HELPER)
#else
	MACRO
	CALL_HELPER_AND_PASS_CACHE_ADDR0 $HELPER
	IMPORT $HELPER
	ldr	a1,  [sp,  #OFFSET_CVMCCExecEnv_ee]
        str	lr,  [JFP, #OFFSET_CVMCompiledFrame_PC]
        str     JFP, [a1, #OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame]
	mov	a1,  sp
	BRANCH_TO_VM_FUNCTION($HELPER)
	MEND

#define CALL_HELPER_AND_PASS_CACHE_ADDR(HELPER)				\
	CALL_HELPER_AND_PASS_CACHE_ADDR0 HELPER
#endif

/* do a gc rendezvous */
	ENTRY(CVMCCMruntimeGCRendezvousGlue)
ENTRY1 ( CVMCCMruntimeGCRendezvousGlue )
	CALL_HELPER_AND_PASS_EE(CVMCCMruntimeGCRendezvous)
	SET_SIZE(CVMCCMruntimeGCRendezvousGlue)


#if defined(CVM_JIT_COPY_CCMCODE_TO_CODECACHE) && defined(CVMJIT_PATCH_BASED_GC_CHECKS)	
	ENTRY(CVMARMruntimeGCRendezvousAdjustRetAddrGlue)
ENTRY1 ( CVMARMruntimeGCRendezvousAdjustRetAddrGlue )
	/* If we are copying to the code cache, then only one instruction
         * was needed to call us, so we want to return to it. Otherwise
         * it may have taken multiple instructions, so the last one is
         * patched with a nop so we can skip it.
         */
	sub	lr, lr, #4    /* we want to return to the patch point */
	CALL_HELPER_AND_PASS_EE(CVMCCMruntimeGCRendezvous)
	SET_SIZE(CVMARMruntimeGCRendezvousAdjustRetAddrGlue)
#endif

/* Throw various well known exceptions */

	ENTRY(CVMCCMruntimeThrowNullPointerExceptionGlue)
ENTRY1 ( CVMCCMruntimeThrowNullPointerExceptionGlue )
LABEL(CVMCCMruntimeThrowNullPointerExceptionGlueLocal)
	IMPORT(java_lang_NullPointerException_Classblock)
	FIXUP_FRAMES(JFP, {lr}, 1)
	ldr	a3, SYMBOL(java_lang_NullPointerException_Classblock)
	mov	a4, #0  /* exception message */
	CALL_HELPER_AND_PASS_EE(CVMCCMruntimeThrowClass)
	SET_SIZE(CVMCCMruntimeThrowNullPointerExceptionGlue)

	ENTRY(CVMCCMruntimeThrowArrayIndexOutOfBoundsExceptionGlue)
ENTRY1 ( CVMCCMruntimeThrowArrayIndexOutOfBoundsExceptionGlue )
	IMPORT(java_lang_ArrayIndexOutOfBoundsException_Classblock)
	FIXUP_FRAMES(JFP, {lr}, 1)
#ifdef ARMASM_LONG_SYMBOL_NAME_BUG
        ldr	a3, L1
#else
	ldr	a3, SYMBOL(java_lang_ArrayIndexOutOfBoundsException_Classblock)
#endif
	mov	a4, #0  /* exception message */
	CALL_HELPER_AND_PASS_EE(CVMCCMruntimeThrowClass)
	SET_SIZE(CVMCCMruntimeThrowArrayIndexOutOfBoundsExceptionGlue)
#ifdef ARMASM_LONG_SYMBOL_NAME_BUG
L1	DCD	java_lang_ArrayIndexOutOfBoundsException_Classblock
#endif

	ENTRY(CVMCCMruntimeThrowDivideByZeroGlue)
ENTRY1 ( CVMCCMruntimeThrowDivideByZeroGlue )
LABEL(CVMCCMruntimeThrowDivideByZeroGlueLocal)
	IMPORT(java_lang_ArithmeticException_Classblock)
	FIXUP_FRAMES(JFP, {lr}, 1)
	adr	a4, divideByZeroString /*exception message */
	ldr	a3, SYMBOL(java_lang_ArithmeticException_Classblock)
	CALL_HELPER_AND_PASS_EE(CVMCCMruntimeThrowClass)
LABEL(divideByZeroString)
	STRING("/ by zero")
       	SET_SIZE(CVMCCMruntimeThrowDivideByZeroGlue)

/* Throw an object */
	ENTRY(CVMCCMruntimeThrowObjectGlue)
ENTRY1 ( CVMCCMruntimeThrowObjectGlue )
#define SAVESET {a3, lr}
	FIXUP_FRAMES(JFP, SAVESET, 2)
#undef SAVESET
	CALL_HELPER_AND_PASS_EE(CVMCCMruntimeThrowObject)

/* do a checkcast */
	ENTRY(CVMCCMruntimeCheckCastGlue)
ENTRY1 ( CVMCCMruntimeCheckCastGlue )
        /* a1 = object to cast 
	 * a2 = objectCb #ifdef IAI_CACHEDCONSTANT_INLINING 
         * a3 = castCb 
	 * a4 = guess cb address #ifdef IAI_CACHEDCONSTANT 
	 * lr = guess cb address #ifndef IAI_CACHEDCONSTANT   
	 * cc = eq if a1 is null #ifndef IAI_CACHEDCONSTANT_INLINING 
	 */
#ifndef IAI_CACHEDCONSTANT_INLINING
#ifdef IAI_CACHEDCONSTANT
	ldrne	a2, [a1]	/* a2 = object.cb */
	ldrne	a1, [a4]	/* load the guess cb */
	bicne	a2, a2, #3	/* mask off low bits of object cb */
	cmpne	a1, a2		/* see if guess is correct */
	moveq   pc, lr		/* return, null or same as last successful cast*/
#else	
	ldrne	a2, [a1]	/* a2 = object.cb */
	ldrne	a1, [lr]	/* load the guess cb */
	add	lr, lr, #4	/* skip passed the guess addr to the return addr*/
	bicne	a2, a2, #3	/* mask off low bits of object cb */
	cmpne	a1, a2		/* see if guess is correct */
	moveq   pc, lr		/* return, null or same as last successful cast*/
#endif /* !IAI_CACHEDCONSTANT*/
#endif /* IAI_CACHEDCONSTANT_INLINING */

#ifndef IAI_CACHEDCONSTANT 
	sub	a4,  lr, #4	/* load guess cb address */
#endif

/* IAI - 21 */
#if defined(IAI_FAST_ASSIGNABLE_CHECKING) && !defined(CVM_AOT)
	/* Fast runtime check cast. 
	 * Compare the cast cb with current object's class, super class, 
         * super'super class ...  till to the super class is NULL.
         */
 	mov 	a1, a2   /* a1 = object.cb */

FASTCHECKING1:
	cmp   	a3, a1      /* cast cb ?= object.cb */
	streq  	a2, [a4]   /* save the matched cb into cache */
	moveq  	pc, lr    /* match, so return */
       	ldr   	a1, [a1, #OFFSET_CVMClassBlock_superClassCb]   /* load super class*/
       	cmp   	a1,  #0      /* Object class? no super class */
       	bne   	FASTCHECKING1
#endif /* IAI_FAST_ASSIGNABLE_CHECKING */

	/* need to call the helper
	 * a2 = objectCb 
 	 * a3 = instanceCb 
	 * a4 = guess cb address 
	 */
#define SAVESET {a2, a3, lr}
	FIXUP_FRAMES(JFP, SAVESET, 3)
#undef SAVESET
	CALL_HELPER_AND_PASS_CACHE_ADDR(CVMCCMruntimeCheckCast)
/* do an instanceof check */
	ENTRY(CVMCCMruntimeInstanceOfGlue)
ENTRY1 ( CVMCCMruntimeInstanceOfGlue )
        /* a1 = object to cast 
	 * a2 = objectCb #ifdef IAI_CACHEDCONSTANT_INLINING 
         * a3 = instanceofCb 
	 * a4 = guess cb address #ifdef IAI_CACHEDCONSTANT 
	 * lr = guess cb address #ifndef IAI_CACHEDCONSTANT   
	 * cc = eq if a1 is null #ifndef IAI_CACHEDCONSTANT_INLINING 
	 */
#ifndef IAI_CACHEDCONSTANT_INLINING
#ifdef IAI_CACHEDCONSTANT
	moveq	pc, lr		/* return FALSE if object is NULL */
	ldrne	a2, [a1]	/* a2 = object.cb */
	ldrne	a1, [a4]	/* load the guess cb */
	bicne	a2, a2, #3	/* mask off low bits of object cb */
	cmpne	a1, a2		/* see if guess is correct */
	moveq   a1, #1		/* return TRUE if equal */
	moveq   pc, lr		/* return, same as last successful cast */
#else 
	addeq	pc, lr, #4	/* return FALSE if object is NULL */
	ldrne	a2, [a1]	/* a2 = object.cb */
	ldrne	a1, [lr]	/* load the guess cb */
	add	lr, lr, #4	/* skip passed the guess addr to the return addr*/
	bicne	a2, a2, #3	/* mask off low bits of object cb */
	cmpne	a1, a2		/* see if guess is correct */
	moveq   a1, #1		/* return TRUE if equal */
	moveq   pc, lr		/* return, same as last successful cast */
#endif /* !IAI_CACHEDCONSTANT*/
#endif /* IAI_CACHEDCONSTANT_INLINING */

#ifndef IAI_CACHEDCONSTANT 
	sub	a4,  lr, #4	/* load guess cb address */
#endif

/* IAI - 21 */
#if defined(IAI_FAST_ASSIGNABLE_CHECKING) && !defined(CVM_AOT)
        /* Fast runtime instanceOf Test. 
         *  Compare the cast cb with current object's class, super class, 
         *  super'super class ...  till to the super class is NULL.
         */

        mov 	a1, a2      /* a1 = object.cb */

FASTCHECKING2:
        cmp    	a3, a1       /* cast cb ?= object.cb */
        streq  	a2, [a4]    /* save the matched cb into cache */
        moveq  	a1, #1
        moveq  	pc, lr     /* match, so return */
        ldr    	a1, [a1, #OFFSET_CVMClassBlock_superClassCb]   /* load super class*/
       	cmp    	a1,  #0     /* Object class? no super class */
       	bne    	FASTCHECKING2
#endif  /* IAI_FAST_ASSIGNABLE_CHECKING */

	/* need to call the helper 
	 * a2 = objectCb 
	 * a3 = instanceCb 
	 * a4 = guess cb address 
	 */
	CALL_HELPER_AND_PASS_CACHE_ADDR(CVMCCMruntimeInstanceOf)

/* check reference array assignment capatibility */
	ENTRY(CVMCCMruntimeCheckArrayAssignableGlue)
ENTRY1 ( CVMCCMruntimeCheckArrayAssignableGlue )
        /* a3 = cb of array object (with low bits still set) */
        /* a4 = cb of rhs object (with low bits still set)  */
	bic	a3, a3, #3	/* clear low bits of arraycb */
	ldr	a3, [a3, #OFFSET_CVMClassBlock_arrayInfoX] /*arraycb->arrayInfo*/
	bic	a4, a4, #3	/* clear low bits of rhs cb */
	ldr	a3, [a3, #OFFSET_CVMArrayInfo_elementCb]   /* arrayInfo->elemCb*/
	cmp	a3, a4		/* check if elemClass(arr) == class(rhs) */
	moveq	pc, lr
	IMPORT(java_lang_Object_Classblock)
	ldr	a1, SYMBOL(java_lang_Object_Classblock)
	cmp	a3, a1		/* check if elemClass(arr) == Object */
	moveq	pc, lr
	/* a3 = element cb of array object (with low bits clear) */
	/* a4 = cb of rhs object (with low bits clear) */
#define SAVESET {a3, a4, lr}
	FIXUP_FRAMES(JFP, SAVESET, 3)
#undef SAVESET
	CALL_HELPER_AND_PASS_EE(CVMCCMruntimeCheckArrayAssignable)

/* run the clinit of a class */
	ENTRY(CVMCCMruntimeRunClassInitializerGlue)
ENTRY1 ( CVMCCMruntimeRunClassInitializerGlue )
        /* a3 = target cb 
	 * lr = return address 
	 * setup ccee and ee arguments 
	 */
	mov	a1,  sp
	ldr	a2,  [sp,  #OFFSET_CVMCCExecEnv_ee]
	/* flush state */
	str     JSP, [JFP, #OFFSET_CVMFrame_topOfStack]
        str	lr,  [JFP, #OFFSET_CVMCompiledFrame_PC]
        str     JFP, [a2, #OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame]
	/* call the helper. If the clinit has to be run, it will return
	 * directly to the method that called us. */
	CALL_VM_FUNCTION(CVMCCMruntimeRunClassInitializer)
	/* reload the return address */
        ldr	lr,  [JFP, #OFFSET_CVMCompiledFrame_PC]
	/* At this point we know that the either class initialization is
	 * not needed (result is TRUE), or is being done by the
	 * current thread. For the later we just return. */
	cmp	a1,  #0	/* FALSE means current thread is running clinit */
	moveq	pc, lr
	/* At this point we know the class has been intialized. Patch the 
	 * the call to this helper to be a nop.
	 * WARNING: Don't attempt to patch over the ldr of the cb with
	 * a branch around the call to the helper. The ldr may be the
	 * first instruction of a block, which means we may also try to
	 * patch it when doing a gc-rendezvous. */
	/* We don't want to patch pre-compiled code through. */
#ifdef CVM_MTASK
	IMPORT(CVMJITcodeCacheDecompileStart)
	ldr	a2, SYMBOL(CVMJITcodeCacheDecompileStart)
	ldr	a2, [a2]
	ldr	a2, [a2]
	cmp	lr, a2
	movlo	pc, lr
#endif
#ifdef CVM_AOT
	IMPORT(CVMJITcodeCacheAOTEnd)
	IMPORT(CVMJITcodeCacheAOTStart)
	ldr	a2, SYMBOL(CVMJITcodeCacheAOTEnd)
	ldr	a2, [a2]
	ldr	a2, [a2]
	cmp	lr, a2
	bhs	doPatch
	ldr	a2,  SYMBOL(CVMJITcodeCacheAOTStart)
	ldr	a2, [a2]
	ldr	a2, [a2]
	cmp	lr, a2
	movhs	pc, lr
#endif

LABEL(doPatch)	
	ldr	a2,  CONSTANT(0xe1a00000)/* get the patch, a nop instruction */
	str	a2,  [lr, #-4]     /* apply the patch */
	/* flush the patched instruction */
	sub	a1,  lr, #4	  /* begin */
	add	a2,  a1, #4       /* end */
	CALL_VM_FUNCTION(CVMflushCache)
	/* return to the helper */
        ldr	pc,  [JFP, #OFFSET_CVMCompiledFrame_PC]
	
/*
 * Resolve a cp entry and run the clinit if necessary
 */
	ENTRY(CVMCCMruntimeResolveGlue)
ENTRY1 ( CVMCCMruntimeResolveGlue )
LABEL(CVMCCMruntimeResolveGlueLocal)
        /* a3 = cpIndex 
	 * lr = address of cachedConstant, 4 bytes before returnAddress 
	 * ip = address of ccm helper to call 
	 * setup remaining arguments and call the helper 
	 */
	mov	a1, sp		/* a1 = ccee */
	ldr	a2, [sp, #OFFSET_CVMCCExecEnv_ee]	/* a2 = ee */
	mov	a4, lr		/* a4 = address of cachedConstant */
	/* flush state */
	str     JSP, [JFP, #OFFSET_CVMFrame_topOfStack]
	add	lr,  lr, #4	/* lr = address we want to return to */
        str	lr,  [JFP, #OFFSET_CVMCompiledFrame_PC]
        str     JFP, [a2, #OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame]
	/* call the helper */
	mov	lr, pc
	mov	pc, ip
	/* reload the return address */
        ldr	lr,  [JFP, #OFFSET_CVMCompiledFrame_PC]
	/* At this point we know that the either class initialization is
	 * not needed (result is TRUE), or is being done by the
	 * current thread. For the later we just return. */
	cmp	a1,  #0	/* FALSE means current thread is running clinit */
	moveq	pc, lr
	/* At this point we know that any needed class initialization has
	 * been done. Patch the first instruction of the generated code
	 * to branch around the call to this helper, and also around
	 * the cachedConstant word. */
#ifdef CVM_JIT_COPY_CCMCODE_TO_CODECACHE
#define RESOLVE_PATCH_OFFSET 16
#else
#define RESOLVE_PATCH_OFFSET 20
#endif
	ldr	a2,  L_MAKEBRANCH_1 /* get the patch */
	add	lr, lr, #4
	str	a2,  [lr, #-RESOLVE_PATCH_OFFSET]       /* apply the patch */
	/* flush the patched instruction */
	sub	a1,  lr, #RESOLVE_PATCH_OFFSET  /* begin */
	add	a2,  a1, #4		        /* end */
 	CALL_VM_FUNCTION(CVMflushCache)
	/* return to compiled code */
        ldr	pc,  [JFP, #OFFSET_CVMCompiledFrame_PC]
LABEL(L_MAKEBRANCH_1)
        WORD(MAKE_BRANCH(RESOLVE_PATCH_OFFSET))

        ENTRY(CVMCCMruntimeResolveNewClassBlockAndClinitGlue)
ENTRY1(CVMCCMruntimeResolveNewClassBlockAndClinitGlue)
	IMPORT(CVMCCMruntimeResolveNewClassBlockAndClinit)
	ldr     ip, SYMBOL(CVMCCMruntimeResolveNewClassBlockAndClinit)
	b	CVMCCMruntimeResolveGlueLocal
        SET_SIZE(CVMCCMruntimeResolveNewClassBlockAndClinitGlue)

        ENTRY(CVMCCMruntimeResolveGetstaticFieldBlockAndClinitGlue)
ENTRY1(CVMCCMruntimeResolveGetstaticFieldBlockAndClinitGlue)
	IMPORT(CVMCCMruntimeResolveGetstaticFieldBlockAndClinit)
	ldr     ip, SYMBOL(CVMCCMruntimeResolveGetstaticFieldBlockAndClinit)
	b	CVMCCMruntimeResolveGlueLocal
        SET_SIZE(CVMCCMruntimeResolveGetstaticFieldBlockAndClinitGlue)

        ENTRY(CVMCCMruntimeResolvePutstaticFieldBlockAndClinitGlue)
ENTRY1(CVMCCMruntimeResolvePutstaticFieldBlockAndClinitGlue)
	IMPORT(CVMCCMruntimeResolvePutstaticFieldBlockAndClinit)
	ldr     ip, SYMBOL(CVMCCMruntimeResolvePutstaticFieldBlockAndClinit)
	b	CVMCCMruntimeResolveGlueLocal
        SET_SIZE(CVMCCMruntimeResolvePutstaticFieldBlockAndClinitGlue)

        ENTRY(CVMCCMruntimeResolveStaticMethodBlockAndClinitGlue)
ENTRY1(CVMCCMruntimeResolveStaticMethodBlockAndClinitGlue)
	IMPORT(CVMCCMruntimeResolveStaticMethodBlockAndClinit)
	ldr     ip, SYMBOL(CVMCCMruntimeResolveStaticMethodBlockAndClinit)
	b	CVMCCMruntimeResolveGlueLocal
        SET_SIZE(CVMCCMruntimeResolveStaticMethodBlockAndClinitGlue)

        ENTRY(CVMCCMruntimeResolveClassBlockGlue)
ENTRY1(CVMCCMruntimeResolveClassBlockGlue)
	IMPORT(CVMCCMruntimeResolveClassBlock)
	ldr     ip, SYMBOL(CVMCCMruntimeResolveClassBlock)
	b	CVMCCMruntimeResolveGlueLocal
        SET_SIZE(CVMCCMruntimeResolveClassBlockGlue)

        ENTRY(CVMCCMruntimeResolveArrayClassBlockGlue)
ENTRY1(CVMCCMruntimeResolveArrayClassBlockGlue)
	IMPORT(CVMCCMruntimeResolveArrayClassBlock)
	ldr     ip, SYMBOL(CVMCCMruntimeResolveArrayClassBlock)
	b	CVMCCMruntimeResolveGlueLocal
        SET_SIZE(CVMCCMruntimeResolveArrayClassBlockGlue)

        ENTRY(CVMCCMruntimeResolveGetfieldFieldOffsetGlue)
ENTRY1(CVMCCMruntimeResolveGetfieldFieldOffsetGlue)
	IMPORT(CVMCCMruntimeResolveGetfieldFieldOffset)
	ldr     ip, SYMBOL(CVMCCMruntimeResolveGetfieldFieldOffset)
	b	CVMCCMruntimeResolveGlueLocal
        SET_SIZE(CVMCCMruntimeResolveGetfieldFieldOffsetGlue)

        ENTRY(CVMCCMruntimeResolvePutfieldFieldOffsetGlue)
ENTRY1(CVMCCMruntimeResolvePutfieldFieldOffsetGlue)
	IMPORT(CVMCCMruntimeResolvePutfieldFieldOffset)
	ldr     ip, SYMBOL(CVMCCMruntimeResolvePutfieldFieldOffset)
	b	CVMCCMruntimeResolveGlueLocal
        SET_SIZE(CVMCCMruntimeResolvePutfieldFieldOffsetGlue)

        ENTRY(CVMCCMruntimeResolveSpecialMethodBlockGlue)
ENTRY1(CVMCCMruntimeResolveSpecialMethodBlockGlue)
	IMPORT(CVMCCMruntimeResolveSpecialMethodBlock)
	ldr     ip, SYMBOL(CVMCCMruntimeResolveSpecialMethodBlock)
	b	CVMCCMruntimeResolveGlueLocal
        SET_SIZE(CVMCCMruntimeResolveSpecialMethodBlockGlue)

        ENTRY(CVMCCMruntimeResolveMethodBlockGlue)
ENTRY1(CVMCCMruntimeResolveMethodBlockGlue)
	IMPORT(CVMCCMruntimeResolveMethodBlock)
	ldr     ip, SYMBOL(CVMCCMruntimeResolveMethodBlock)
	b	CVMCCMruntimeResolveGlueLocal
        SET_SIZE(CVMCCMruntimeResolveMethodBlockGlue)



/*
 * Resolve a vtable offset. The protocol here is slight
 * different because the result could possibly be an MB pointer
 * rather than just an offset. Calling code looks like this:
 *	ld	destreg,cache
 *	mov	ARG1, cpIndex
 *	call	CVMCCMruntimeResolveMethodTableOffsetGlue
 *	mov	mbDestreg,RETURN1
 *	b	haveMB
 *  cache:
 *	.word	-1
 *  return1:
 *	ld	destreg, cache
 *
 * If the helper returns 0, then the cache will have been filled with
 * the vtable offset and we go let the general resolver glue code
 * do the rewrite and return.
 * Otherwise, the helper returned the MB pointer and we return that
 * directly.
 */
	ENTRY(CVMCCMruntimeResolveMethodTableOffsetGlue)
ENTRY1 ( CVMCCMruntimeResolveMethodTableOffsetGlue )
	IMPORT(CVMCCMruntimeResolveMethodTableOffset)
        /* a3 = cpIndex */
	/* lr = address of instruction following call */
	ldr	ip, SYMBOL(CVMCCMruntimeResolveMethodTableOffset)
	/* setup remaining arguments and call the helper */
	mov	a1, sp		/* a1 = ccee */
	ldr	a2, [sp, #OFFSET_CVMCCExecEnv_ee]	/* a2 = ee */
	add	a4, lr, #8	/* a4 = address of cachedConstant */
	/* flush state */
	str     JSP, [JFP, #OFFSET_CVMFrame_topOfStack]
	add	lr,  a4, #4	/* lr = address we want to return to normally */
        str     JFP, [a2, #OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame]
        str	lr,  [JFP, #OFFSET_CVMCompiledFrame_PC]
	/* call the helper */
	mov	lr, pc
	mov	pc, ip
	/* reload the return address */
        ldr	lr,  [JFP, #OFFSET_CVMCompiledFrame_PC]
	cmp	a1,  #0		/* 0 means cache contains vtbl address */
	/* will return value already in a1 at point just after call */
	subne	pc, lr, #12

	/* At this point we know that the method table offset has been
	 * written to the cache word.
	 * Patch the first instruction of the generated code
	 * to branch around the call to this helper, around the special
	 * case return code, and around the cachedConstant word.
	 */
#define RESOLVE_METHOD_TABLE_PATCH_OFFSET (RESOLVE_PATCH_OFFSET+8)
	ldr	a2,  L_MAKEBRANCH_2
	add	lr, lr, #4
	str	a2,  [lr, #-RESOLVE_METHOD_TABLE_PATCH_OFFSET]
	/* flush the patched instruction */
	sub	a1,  lr, #RESOLVE_METHOD_TABLE_PATCH_OFFSET  /* begin */
	add	a2,  a1, #4		        /* end */
	CALL_VM_FUNCTION(CVMflushCache)
	/* return to compiled code */
        ldr	pc,  [JFP, #OFFSET_CVMCompiledFrame_PC]
LABEL(L_MAKEBRANCH_2)
        WORD(MAKE_BRANCH(RESOLVE_METHOD_TABLE_PATCH_OFFSET))

	/* lookup an interface mb */
	ENTRY(CVMCCMruntimeLookupInterfaceMBGlue)
ENTRY1 ( CVMCCMruntimeLookupInterfaceMBGlue )
        /* r1 = object to invoke with 
         * r2 = interface mb 
	 * lr = address of guess from last interface mb lookup 
	 */

	add	lr, lr, #4	/* skip pass the guess addr to the return addr */

#define OCB           r1
#define INTERFACE_MB  r2
#define OINTERFACES   ip

        /* ocb = CVMobjectGetClass(obj); */
        ldr     OCB, [r1, #OFFSET_CVMObjectHeader_clas] /* get ocb from obj */
        bic     OCB, OCB, #3            /* ocb &= ~0x3; prepare ocb */

        /* interfaces = ocb->interfacesX */
        ldr     OINTERFACES, [r1, #OFFSET_CVMClassBlock_interfacesX]
        cmp     OINTERFACES, #0         /* Check if ocb->interfacesX == NULL. */
        beq     call_lookup_helper
       
        /* ointerfaceCount = ocb->interfacesX->interfaceCountX; */ 
        ldrh    r3, [OINTERFACES, #OFFSET_CVMInterfaces_interfaceCountX]

#define GUESS   r0 
        ldr     GUESS, [lr, #-4]        /* load guess value */
        cmp     GUESS, r3               /* cmp guess and ointerfaceCount */
        bcs     call_lookup_helper   /* if guess >= ointerfaceCount, invalid */

        /* If we get here than the guess is within the valid range: */
        add     r3, OINTERFACES, #OFFSET_CVMInterfaces_itable
        mov     GUESS, GUESS, asl #CONSTANT_LOG2_CVMInterfaceTable_SIZE
        ldr     r3, [r3, GUESS]   /* target ICB = &ointerfaces.itable[guess] */

        /* Free up the GUESS reg (r0) for use below. Save its content first: */
        str     GUESS, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+0]
#undef GUESS

#ifdef CVM_METHODBLOCK_HAS_CB
        ldr     r0, [INTERFACE_MB, #OFFSET_CVMMethodBlock_cbX] /* get icb */
#else
#if CONSTANT_CVMMethodBlock_size != 28
#error Wrong CVMMethodBlock size
#endif
#ifdef OFFSET_CVMMethodBlock_cbX
#error OFFSET_CVMMethodBlock_cbX defined but not CVM_METHODBLOCK_HAS_CB
#endif
        ldrb    r0, [INTERFACE_MB, #OFFSET_CVMMethodBlock_methodIndexX]

	/* Here is a way to do r0 = r0 * 28 
         * r0 = r0 * 32 
	 * r0 = r0 - r0 / 8 
	 */
	mov	r0, r0, asl #5
	sub	r0, r0, r0, asr #3
	sub	r0, INTERFACE_MB, r0

        ldr     r0, [r0, #-OFFSET_CVMMethodRange_mb]
#endif

        /* Check if the guess' interface CB is the one we want: */
        cmp     r3, r0                  /* test if target ICB == source ICB */
        bne     call_lookup_helper      /* go call helper if guess failed. */

        /* If we get here, then the guess is correct. Go fetch the method
           block from the interface CB: */

#define SOURCE_MB_IDX   r3
        /* get source mb MethodSlotIndex */
        ldr     SOURCE_MB_IDX, [INTERFACE_MB, #OFFSET_CVMMethodBlock_codeX]

#define TARGET_METHODTABLE_INDICES ip
#define GUESS r0
        /* Reload the GUESS reg (r0) content from earlier: */
        ldr     GUESS, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+0]

        /* Target methodTableIndices = 
                ocb->interfacesX.itable[guess].methodTableIndicesX;
        */ 
        add     ip, OINTERFACES, #OFFSET_CVMInterfaces_itable0_intfInfoX
        ldr     TARGET_METHODTABLE_INDICES, [ip, GUESS]
#undef GUESS
#undef OINTERFACES

        /* Get the interface mb from the ocb's vtbl: */
        ldr     r0, [OCB, #OFFSET_CVMClassBlock_methodTablePtrX]
        mov     r3, SOURCE_MB_IDX, \
	            asl #CONSTANT_LOG2_CVMInterfaceTable_methodTableIndex_SIZE
        ldrh	ip, [TARGET_METHODTABLE_INDICES, r3]

        /* r0 = ocb->methodTablePtrX[ip]: */
        ldr     r0, [r0, ip, asl #2]
        mov     pc, lr

LABEL(call_lookup_helper)
	sub	a4,  lr, #4	/* load guessCb adderess */
        /* r1 = a2 = ocb 
         * r2 = a3 = imb 
         * r3 = a4 = guess address 
	 */
#define SAVESET {a2, a3, lr}
	FIXUP_FRAMES(JFP, SAVESET, 3)
#undef SAVESET
        CALL_HELPER_AND_PASS_CACHE_ADDR(CVMCCMruntimeLookupInterfaceMB)

#undef OCB
#undef TARGET_METHODTABLE_INDICES

/* support for fastlocking with C sync helpers */
#if defined(CVM_JIT_CCM_USE_C_HELPER) || defined(CVM_JIT_CCM_USE_C_SYNC_HELPER)

/*
 * Entry point for monitorenter.
 */
	ENTRY(CVMCCMruntimeMonitorEnterGlue)
ENTRY1 ( CVMCCMruntimeMonitorEnterGlue )
        /* Arguments: 
         *       r2 = a3 = 'obj' 
	 *
         * Also incoming: 
         *       r4 = v1 = jfp 
         *       r5 = v2 = jsp 
         *       sp = ccee 
	 */
        FLUSH_TOS
        CALL_HELPER_AND_PASS_EE(CVMCCMruntimeMonitorEnter)

/*
 * Entry point for monitorexit.
 */
	ENTRY(CVMCCMruntimeMonitorExitGlue)
ENTRY1 ( CVMCCMruntimeMonitorExitGlue )
        /* Arguments: 
         *       r2 = a3 = 'obj' 
	 *
         * Also incoming: 
         *       r4 = v1 = jfp 
         *       r5 = v2 = jsp 
         *       sp = ccee 
	 */
        FLUSH_TOS
	CALL_HELPER_AND_PASS_EE(CVMCCMruntimeMonitorExit)

/* Support for fastlocking with swap/spinlock based microlocks */
#elif CVM_FASTLOCK_TYPE == CVM_FASTLOCK_MICROLOCK && \
    CVM_MICROLOCK_TYPE == CVM_MICROLOCK_SWAP_SPINLOCK

/*
 * Entry point for monitorenter.
 */
	ENTRY(CVMCCMruntimeMonitorEnterGlue)
ENTRY1 ( CVMCCMruntimeMonitorEnterGlue )

        /* Arguments: 
         *       r2 = a3 = 'obj' 
	 *
         * Also incoming: 
         *       r4 = v1 = jfp 
         *       r5 = v2 = jsp 
         *       sp = ccee 
	 */

        /* Do fastTryLock(): */

        /*
           r0 = &microlock
           r1 = ee
           r2 = obj, scratch
           r3 = obits, scratch
           r4 = v1 = JFP
           r5 = v2 = JSP
           r12 = lockrec
        */

#define EE          r1
#define OBJ         r2
#define OBITS       r3
#define MICROLOCK   r0
#define LOCKREC     r12

/* IAI-04 */
        /* W_MICROLOCK = address of microlock */
#ifdef IAI_CACHE_GLOBAL_VARIABLES_IN_WMMX
        /* Get address of microlock from wmmx register */
        textrmuw MICROLOCK, W_MICROLOCK, #0
        cmp     OBJ, #0
#else
        ldr     MICROLOCK, SYMBOL(CVMobjGlobalMicroLockPtr)
        /* Make sure that the object is not NULL: */
        cmp     OBJ, #0
	/* Get address of object microlock */
        ldr     MICROLOCK, [MICROLOCK]        
#endif
        beq     SYM_NAME(CVMCCMruntimeThrowNullPointerExceptionGlueLocal)
        ldr     r3, [MICROLOCK]

        /* Acquire the microlock: */
        mov     r3, #CVM_MICROLOCK_LOCKED   /* Swap CVM_MICROLOCK_LOCKED into */
#ifndef CVM_MP_SAFE
        swp     r3, r3, [MICROLOCK]         /*    the lockWord. */
        cmp     r3, #CVM_MICROLOCK_UNLOCKED /* See if we succeeded. */
        bne     _monenterSlowAcquireMicrolock    /* Branch if failed. */
#else
LABEL(_monenterAcquireMicrolockMPSafe)
	ldrex	r12, [MICROLOCK]
	cmp	r12, #CVM_MICROLOCK_UNLOCKED  /* is the lock free? */
	bne	_monenterSlowAcquireMicrolock /* No. Go slow */
	strex	r12, r3, [MICROLOCK]          /* try acquire the lock */
	cmp	r12, #0                       /* strex succeed? */
	bne	_monenterAcquireMicrolockMPSafe /* if strex failed, try again */
	mcr	p15, 0, r12, c7, c10, 5       /* memory barrier. r12 has 0. */
#endif

        /* The microlock has been acquired: */
LABEL(_monenterLockObj)
        ldr     OBITS, [OBJ, #OFFSET_CVMObjectHeader_various32] /* Get obits. */
        ldr     EE, [sp, #OFFSET_CVMCCExecEnv_ee]
        and     r12, OBITS, #0x3
        cmp     r12, #CONSTANT_CVM_LOCKSTATE_UNLOCKED
        bne     _monenterObjAlreadyLocked

        /* If we get here, then the object has not been locked yet. */
        /* lockrec = ee->objLocksFreeOwned: */
        ldr     LOCKREC, [EE, #OFFSET_CVMExecEnv_objLocksFreeOwned]
        cmp     LOCKREC, #0
        beq     _monenterRecordNotAvailable

        /* lockrec->u.fast.bits = obits: */
        str     OBITS, [LOCKREC, #OFFSET_CVMOwnedMonitor_u_fast_bits]

        /* Now, r3 (i.e. OBITS) is free. */

#ifdef CVM_DEBUG
        /* lockrec->state = CONSTANT_CVM_OWNEDMON_OWNED: */
        mov     r3, #CONSTANT_CVM_OWNEDMON_OWNED
        str     r3, [LOCKREC, #OFFSET_CVMOwnedMonitor_state]
#endif
        /* obj->hdr.various32 = lockrec: */
        str     LOCKREC, [OBJ, #OFFSET_CVMObjectHeader_various32]

        /* lockrec->count = 1: */
        mov     r3, #1                  /* Initial lock re-entry count. */
        str     r3, [LOCKREC, #OFFSET_CVMOwnedMonitor_count]

        /* lockrec->object = obj: */
        str     OBJ, [LOCKREC, #OFFSET_CVMOwnedMonitor_object]

        /* Now, r2 (i.e OBJ) is free. */
	
#ifdef CVM_MP_SAFE
	mov	r3, #0
	mcr	p15, 0, r3, c7, c10, 5       /* memory barrier. */
#endif
        /* Release the microlock: */
        mov     r3, #CVM_MICROLOCK_UNLOCKED
        str     r3, [MICROLOCK]         /* microlock->lockWord = UNLOCKED. */

        /* Remove lockrec from the ee's free list: */
        /* nextRec = lockrec->next: */
        ldr     r2, [LOCKREC, #OFFSET_CVMOwnedMonitor_next]
        /* ee->objLocksFreeOwned = nextRec: */
        str     r2, [EE, #OFFSET_CVMExecEnv_objLocksFreeOwned]

        /* Add the lockrec to the ee's owned list: */
        /* nextRec = ee->objLocksOwned: */
        ldr     r2, [EE, #OFFSET_CVMExecEnv_objLocksOwned]
        /* lockrec->next = nextRec: */
        str     r2, [LOCKREC, #OFFSET_CVMOwnedMonitor_next]
        /* ee->objLocksOwned = lockrec: */
        str     LOCKREC, [EE, #OFFSET_CVMExecEnv_objLocksOwned]

        mov     pc, lr                  /* Return to the caller. */

LABEL(_monenterSlowAcquireMicrolock)
        /* Call a C function to acquire the microlock:
           NOTE: We have to save OBJ below because it is in a volatile reg.
                 However, it is safe to simply save it in a ccmStorage field
                 without worrying about GC scans because we are currently
                 GC unsafe and won't be becoming GC safe while acquiring the
                 microlock.
        */
        str     MICROLOCK, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+0]
        str     OBJ, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+4]
        str     lr, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+8]
        CALL_VM_FUNCTION(CVMmicrolockLockImpl)
        ldr     MICROLOCK, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+0]
        ldr     OBJ, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+4]
        ldr     lr, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+8]
        b       _monenterLockObj

LABEL(_monenterObjAlreadyLocked)
        cmp     r12, #CONSTANT_CVM_LOCKSTATE_LOCKED
        bne     _monenterFastReentryFailed

        /* Make sure the current thread owns this lock: */
        ldr     r12, [OBITS, #OFFSET_CVMOwnedMonitor_owner]
        cmp     r12, EE
        bne     _monenterFastReentryFailed

        ldr     r12, [OBITS, #OFFSET_CVMOwnedMonitor_count]
        add     r12, r12, #1
        str     r12, [OBITS, #OFFSET_CVMOwnedMonitor_count]

        /* Release the microlock: */
        mov     r3, #CVM_MICROLOCK_UNLOCKED
        str     r3, [MICROLOCK]         /* microlock->lockWord = UNLOCKED. */

        mov     pc, lr                  /* Return to the caller. */

LABEL(_monenterFastReentryFailed)
LABEL(_monenterRecordNotAvailable)
        /* Release the microlock: */
        mov     r3, #CVM_MICROLOCK_UNLOCKED
        str     r3, [MICROLOCK]

        /* Load the ccee.  The ee, and obj are already loaded. */
        mov     r0,  sp
        str     lr,  [JFP, #OFFSET_CVMCompiledFrame_PC]
        FLUSH_TOS
        str     JFP, [a2, #OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame]
        BRANCH_TO_VM_FUNCTION(CVMCCMruntimeMonitorEnter)

#undef EE
#undef OBJ
#undef OBITS
#undef MICROLOCK
#undef LOCKREC

/*
 * Entry point for monitorexit.
 */
	ENTRY(CVMCCMruntimeMonitorExitGlue)
ENTRY1 ( CVMCCMruntimeMonitorExitGlue )

        /* Arguments: 
         *       r2 = a3 = 'obj' 
	 *
         * Also incoming: 
         *       r4 = v1 = jfp 
         *       r5 = v2 = jsp 
         *       sp = ccee 
	 */

        /* Do fastTryUnlock(): */

        /*
           r0 = a1 = &microlock
           r1 = a2 = ee
           r2 = a3 = obj, scratch
           r3 = a4 = scratch
           r4 = v1 = JFP
           r5 = v2 = JSP
           r12 = lockrec
        */

#define EE          r1
#define OBJ         r2
#define MICROLOCK   r0
#define LOCKREC     r12

/* IAI-04 */
        /* W_MICROLOCK = address of microlock */
#ifdef IAI_CACHE_GLOBAL_VARIABLES_IN_WMMX
        textrmuw MICROLOCK, W_MICROLOCK, #0
        cmp     OBJ, #0
#else
        ldr     MICROLOCK, SYMBOL(CVMobjGlobalMicroLockPtr)
        /* Make sure that the object is not NULL: */
        cmp     OBJ, #0
	/* Get address of object microlock */
        ldr     MICROLOCK, [MICROLOCK]
#endif
        beq     SYM_NAME(CVMCCMruntimeThrowNullPointerExceptionGlueLocal)
        ldr     r3, [MICROLOCK]

LABEL(_monexitAcquireMicrolock)

        mov     r3, #CVM_MICROLOCK_LOCKED   /* Swap CVM_MICROLOCK_LOCKED into */
#ifndef CVM_MP_SAFE	
        swp     r3, r3, [MICROLOCK]         /*    the lockWord. */
        cmp     r3, #CVM_MICROLOCK_UNLOCKED /* See if we succeeded. */
        bne     _monexitSlowAcquireMicrolock    /* Branch if failed. */
#else
LABEL(_monexitAcquireMicrolockMPSafe)
	ldrex	r12, [MICROLOCK]
	cmp	r12, #CVM_MICROLOCK_UNLOCKED /* is the lock free? */
	bne	_monexitSlowAcquireMicrolock /* No. Go slow */
	strex	r12, r3, [MICROLOCK]         /* acquire the lock */
	cmp	r12, #0		             /* strex succeed? */
	bne	_monexitAcquireMicrolockMPSafe /* if strex failed, try again */
	mcr	p15, 0, r12, c7, c10, 5      /* memory barrier. r12 has 0 */
#endif

        /* The microlock has been acquired: */
LABEL(_monexitUnlockObj)
        /* Check to see if the object is locked with a fastlock: */
        ldr     LOCKREC, [OBJ, #OFFSET_CVMObjectHeader_various32] /* Get obits.*/
        ldr     EE, [sp, #OFFSET_CVMCCExecEnv_ee]
        tst     LOCKREC, #0x3        /* (obits & 0x3) == CVM_LOCKSTATE_LOCKED? */
        bne     _monexitFastTryUnlockFailed /* If not, we failed. */

        /* If we get here, then the object is locked with a fastlock: */

        /* Make sure that the current thread owns the monitor: */
        ldr     r3, [LOCKREC, #OFFSET_CVMOwnedMonitor_owner]
        cmp     r3, EE
        bne     _monexitFastTryUnlockFailed /* If not, we failed. */

        /* If we get here, then the current thread does own the monitor,
           and all is well.  Proceed with unlocking: */
        ldr     r3, [LOCKREC, #OFFSET_CVMOwnedMonitor_count]
        subs    r3, r3, #1
        bne     _monexitFastTryUnlockSuccess    /* If not zero, we are done. */

        /* If we get here, then the re-entry count has reached 0. */
        /* Restore the obits to the object header: */
        ldr     r3, [LOCKREC, #OFFSET_CVMOwnedMonitor_u_fast_bits]
        str     r3, [OBJ, #OFFSET_CVMObjectHeader_various32]

#ifdef CVM_DEBUG
        /* Make the lockrec play nice with the debug assertions: */
        mov     r3, #CONSTANT_CVM_OWNEDMON_FREE
        str     r3, [LOCKREC, #OFFSET_CVMOwnedMonitor_state]
        mov     r3, #0
        str     r3, [LOCKREC, #OFFSET_CVMOwnedMonitor_u_fast_bits]
        str     r3, [LOCKREC, #OFFSET_CVMOwnedMonitor_object]
	str	r3, [LOCKREC, #OFFSET_CVMOwnedMonitor_count]
#endif

#ifdef CVM_MP_SAFE
	mov	r3, #0
	mcr	p15, 0, r3, c7, c10, 5       /* memory barrier. */
#endif
        /* Release the microlock: */
        mov     r3, #CVM_MICROLOCK_UNLOCKED
        str     r3, [MICROLOCK]

        /* Now, r2 (i.e. OBJ) is free. */

        /* Check if the lockrec is the first one on the thread's owned list: */
        ldr     r3, [EE, #OFFSET_CVMExecEnv_objLocksOwned]
        cmp     r3, LOCKREC
        bne     _monexitFastTryUnlockFindPrevLockRecord

        /* Remove the lockrec from the ee's owned list: */
        ldr     r3, [LOCKREC, #OFFSET_CVMOwnedMonitor_next]
        str     r3, [EE, #OFFSET_CVMExecEnv_objLocksOwned]

LABEL(_monexitFastTryUnlockAddLockRecordToFreeList)
        /* Add the lockrec to the ee's free list: */
        ldr     r3, [EE, #OFFSET_CVMExecEnv_objLocksFreeOwned]
        str     r3, [LOCKREC, #OFFSET_CVMOwnedMonitor_next]
        str     LOCKREC, [EE, #OFFSET_CVMExecEnv_objLocksFreeOwned]

        mov     pc, lr          /* Return to the caller. */

#define PREV_REC r3
LABEL(_monexitFastTryUnlockFindPrevLockRecord)
        ldr     r2, [PREV_REC, #OFFSET_CVMOwnedMonitor_next]
        cmp     r2, LOCKREC
        beq     _monexitFastTryUnlockFoundPrevLockRecord
        mov     PREV_REC, r2
        b       _monexitFastTryUnlockFindPrevLockRecord

LABEL(_monexitFastTryUnlockFoundPrevLockRecord)
        /* Remove the lockrec from the ee's owned list: */
        ldr     r2, [LOCKREC, #OFFSET_CVMOwnedMonitor_next]
        str     r2, [PREV_REC, #OFFSET_CVMOwnedMonitor_next]
        b       _monexitFastTryUnlockAddLockRecordToFreeList
#undef PREV_REC

LABEL(_monexitFastTryUnlockSuccess)
        /* Set the new re-entry count: */
        str     r3, [LOCKREC, #OFFSET_CVMOwnedMonitor_count]
        /* Release the microlock: */
        mov     r3, #CVM_MICROLOCK_UNLOCKED
        str     r3, [MICROLOCK]

        mov     pc, lr          /* Return to the caller. */

LABEL(_monexitSlowAcquireMicrolock)
        /* Call a C function to acquire the microlock:
           NOTE: We have to save OBJ below because it is in a volatile reg.
                 However, it is safe to simply save it in a non-volatile reg
                 without worrying about GC scans because we are currently
                 GC unsafe and won't be becoming GC safe while acquiring the
                 microlock.
        */
        str     MICROLOCK, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+0]
        str     OBJ, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+4]
        str     lr, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+8]
        CALL_VM_FUNCTION(CVMmicrolockLockImpl)
        ldr     MICROLOCK, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+0]
        ldr     OBJ, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+4]
        ldr     lr, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+8]
        b       _monexitUnlockObj       /* Go unlock the object if possible. */

LABEL(_monexitFastTryUnlockFailed)
        /* Release the microlock: */
        mov     r3, #CVM_MICROLOCK_UNLOCKED
        str     r3, [MICROLOCK]

        /* Let C helper do the hard stuff: */
        /* Load the ccee.  The ee, and obj are already loaded. */
        mov     r0, sp
        str     lr,  [JFP, #OFFSET_CVMCompiledFrame_PC]
        FLUSH_TOS
        str     JFP, [a2, #OFFSET_CVMExecEnv_interpreterStack+OFFSET_CVMStack_currentFrame]
        BRANCH_TO_VM_FUNCTION(CVMCCMruntimeMonitorExit)

#undef EE
#undef OBJ
#undef MICROLOCK
#undef LOCKREC

/* We only support fastlocking with atomic ops */
#elif CVM_FASTLOCK_TYPE == CVM_FASTLOCK_ATOMICOPS

/*
 * Entry point for monitorenter.
 */
        LEAF_ENTRY CVMCCMruntimeMonitorEnterGlue

        ;
        ; Arguments:
        ;       r2 = a3 = 'obj'
        ;
        ; Also incoming:
        ;       r4 = v1 = jfp
        ;       r5 = v2 = jsp
        ;       sp = ccee
        ;

/* TODO: This monitorenter and monitorexit code has not undergone thorough
   multi-threaded stress testing yet.  It needs to undergo stress testing
   before release. */

#if 0
        ;
        ; If you just want to call the C helper and write very little
        ; assembler code, then just the following line is needed.
        ;
        FLUSH_TOS
        CALL_HELPER_AND_PASS_EE(CVMCCMruntimeMonitorEnter)

#endif

        /* Do fastTryLock(): */

#define TEMP            r0
#define EXPECTED_BITS   r1
#define OBJ             r2
#define EE              r3
#define OWNEDREC        r12

        ; Make sure that the object is not NULL:
        ldr     EE, [sp, #OFFSET_CVMCCExecEnv_ee]
        cmp     OBJ, #0
        beq     SYM_NAME(CVMCCMruntimeThrowNullPointerExceptionGlueLocal)

        /* Setup a lock record and assume the object has not been locked
           yet: */
        ; ownedRec = ee->objLocksFreeOwned:
        ldr     OWNEDREC, [EE, #OFFSET_CVMExecEnv_objLocksFreeOwned]
        cmp     OWNEDREC, #0
        beq     _monenterRecordNotAvailable

        ; ownedRec->object = obj:
        str     OBJ, [OWNEDREC, #OFFSET_CVMOwnedMonitor_object]

        ; ownedRec->count = 1:
        mov     TEMP, #1        /* Initial lock re-entry count */
        str     TEMP, [OWNEDREC, #OFFSET_CVMOwnedMonitor_count]

#ifdef CVM_DEBUG
        ; ownedRec->state = CONSTANT_CVM_OWNEDMON_OWNED:
        mov     TEMP, #CONSTANT_CVM_OWNEDMON_OWNED
        str     TEMP, [OWNEDREC, #OFFSET_CVMOwnedMonitor_state]
#endif

        ; NEW_BITS = (OWNEDREC) | CVM_LOCKSTATE_LOCKED:
        ; Since the value of CVM_LOCKSTATE_LOCKED is 0 by design, this means
        ; that NEW_BITS is essentially ownedRec.  Nothing need to be done to
        ; initialize NEW_BITS.

#define ACTUAL_BITS     r0
#define NEW_BITS        OWNEDREC

        ; obits = CVMhdrBitsPtr(obj->hdr.various32) | CVM_LOCKSTATE_UNLOCKED:
        ldr     EXPECTED_BITS, [OBJ, #OFFSET_CVMObjectHeader_various32]
        bic     EXPECTED_BITS, EXPECTED_BITS, #3 /* clear rightmost 2 bits */
        orr     EXPECTED_BITS, EXPECTED_BITS, #CONSTANT_CVM_LOCKSTATE_UNLOCKED

        ; ownedRec->u.fast.bits = obits:
        str     EXPECTED_BITS, [OWNEDREC, #OFFSET_CVMOwnedMonitor_u_fast_bits]

        ; Do atomicCompareAndSwap:
        ; ACTUAL_BITS = [OBJ, #OFFSET_CVMObjectHeader_various32]
        ; if ([OBJ, #OFFSET_CVMObjectHeader_various32] == EXPECTED_BITS) {
        ;     [OBJ, #OFFSET_CVMObjectHeader_various32] = NEW_BITS
        ; }

        /* Save some values that we need later:
           NOTE: We don't save EE and EXPECTED_BITS, because EE can always be
           fetched from the CCEE, and EXPECTED_BITS was already saved in the
           OWNEDREC.
        */
        str     OBJ, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+0]
        str     OWNEDREC, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+4]
        str     lr, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+8]

        add     r0, OBJ, #OFFSET_CVMObjectHeader_various32
        mov     r1, EXPECTED_BITS
        mov     r2, NEW_BITS
        CALL_VM_FUNCTION(InterlockedTestExchange)

        /* Restore the needed values: */
        ldr     OBJ, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+0]
        ldr     OWNEDREC, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+4]
        ldr     lr, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+8]
        ldr     EXPECTED_BITS, [OWNEDREC, #OFFSET_CVMOwnedMonitor_u_fast_bits]
        ldr     EE, [sp, #OFFSET_CVMCCExecEnv_ee] /* Restore EE. */

        cmp     ACTUAL_BITS, EXPECTED_BITS      /* ACTUAL_BITS is in r0. */
        bne     _monenterFastFailed             /* already locked */

#undef NEW_BITS

        /* Remove lockrec from the ee's free list: */
        ; nextRec = ownedRec->next:
        ldr     TEMP, [OWNEDREC, #OFFSET_CVMOwnedMonitor_next]
        ; ee->objLocksFreeOwned = nextRec:
        str     TEMP, [EE, #OFFSET_CVMExecEnv_objLocksFreeOwned]

        /* Add the lockrec to the ee's owned list: */
        ; nextRec = ee->objLocksOwned:
        ldr     TEMP, [EE, #OFFSET_CVMExecEnv_objLocksOwned]
        ; ownedRec->next = nextRec:
        str     TEMP, [OWNEDREC, #OFFSET_CVMOwnedMonitor_next]
        ; ee->objLocksOwned = lockrec:
        str     OWNEDREC, [EE, #OFFSET_CVMExecEnv_objLocksOwned]

        mov     pc, lr          /* Return to the caller */

_monenterFastFailed
#ifdef CVM_DEBUG
        /* NOTE: EXPECTED_BITS is no longer needed here.  We can't use TEMP
                 because TEMP is r0 which is currently needed to retain the
                 value of ACTUAL_BITS. */
#define TEMP2   EXPECTED_BITS
        ; ownedRec->state = CONSTANT_CVM_OWNEDMON_FREE:
        mov     TEMP2, #0
        str     TEMP2, [OWNEDREC, #OFFSET_CVMOwnedMonitor_count]
        mov     TEMP2, #CONSTANT_CVM_OWNEDMON_FREE
        str     TEMP2, [OWNEDREC, #OFFSET_CVMOwnedMonitor_state]
#undef TEMP2
#endif

        ; If object is not in LOCKED state, then fail:
        tst     ACTUAL_BITS, #0x3 /* check for CVM_LOCKSTATE_LOCKED */
        bne     _monenterFastRetryFailed

        ; If not associated with an ownedRec, then fail:
        bics    OWNEDREC, ACTUAL_BITS, #3
        beq     _monenterFastReentryFailed

#undef ACTUAL_BITS

        ; If (ownedRec->owner != ee), then fail:
        ldr     TEMP, [OWNEDREC, #OFFSET_CVMOwnedMonitor_owner]
        cmp     TEMP, EE
        bne     _monenterFastReentryFailed

#define EXPECTED_CNT    EXPECTED_BITS
#define ACTUAL_CNT      r0
#define NEW_COUNT       r2

        ; If we get here, then we are re-entering the lock:
        ldr     EXPECTED_CNT, [OWNEDREC, #OFFSET_CVMOwnedMonitor_count]
        cmp     EXPECTED_CNT, #CONSTANT_CVM_INVALID_REENTRY_COUNT
        beq     _monenterFastReentryFailed

        add     NEW_COUNT, EXPECTED_CNT, #1

        /* Save some values that we need later:
           NOTE: We only need OBJ, EXPECTED_CNT, and lr after this call.
           OBJ and lr was already saved before the first call to
           InterlockedTestExchange() and is invariant during that call.
           Hence, we only need to save the EXPECTED_CNT.
        */
        str     EXPECTED_CNT, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+4]

        add     r0, OWNEDREC, #OFFSET_CVMOwnedMonitor_count
        mov     r1, EXPECTED_CNT
        ; r2 already contains NEW_CNT.

        CALL_VM_FUNCTION(InterlockedTestExchange)

        /* Restore the needed values: */
        ldr     OBJ, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+0]
        ldr     EXPECTED_CNT, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+4]
        ldr     lr, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+8]

        cmp     ACTUAL_CNT, EXPECTED_CNT        /* ACTUAL_CNT is in r0. */
        bne     _monenterFastReentryFailed      /* Fail if not as expected. */

#undef EXPECTED_CNT
#undef ACTUAL_CNT
#undef NEW_COUNT

        ; If the old count is as expected, then we're successful:
        mov     pc, lr  /* Return to the caller */

_monenterFastRetryFailed
_monenterFastReentryFailed
_monenterRecordNotAvailable
        /* Let C helper do the hard stuff: */
        /* NOTE: It is expected that the OBJ pointer is already in r2. */
        FLUSH_TOS
        CALL_HELPER_AND_PASS_EE(CVMCCMruntimeMonitorEnter)

#undef TEMP
#undef EXPECTED_BITS
#undef OBJ
#undef EE
#undef OWNEDREC

	ENTRY_END

/*
 * Entry point for monitorexit.
 */
        ;
        ; Arguments:
        ;       r2 = a3 = 'obj'
        ;
        ; Also incoming:
        ;       r4 = v1 = jfp
        ;       r5 = v2 = jsp
        ;       sp = ccee
        ;

        LEAF_ENTRY CVMCCMruntimeMonitorExitGlue

#if 0
        ;
        ; If you just want to call the C helper and write very little
        ; assembler code, then just the following line is needed.
        ;
        FLUSH_TOS
        CALL_HELPER_AND_PASS_EE(CVMCCMruntimeMonitorExit)

#endif

        /* Do fastTryUnlock(): */

#define TEMP            r0
#define EXPECTED_CNT    r1
#define OBJ             r2
#define EE              r3
#define OWNEDREC        r12

        ; Make sure that the object is not NULL:
        ldr     EE, [sp, #OFFSET_CVMCCExecEnv_ee]
        cmp     OBJ, #0
        beq     SYM_NAME(CVMCCMruntimeThrowNullPointerExceptionGlueLocal)

        /* Check to see if the object is locked with a fastlock: */
        ldr     OWNEDREC, [OBJ, #OFFSET_CVMObjectHeader_various32] /* obits */
        tst     OWNEDREC, #0x3 /* (obits & 0x3) == CVM_LOCKSTATE_LOCKED? */
        bne     _monexitFastTryUnlockFailed /* If not, we failed. */

        /* If the LOCKREC is NULL, is is currently being inflated */
        cmp     OWNEDREC, #0
        beq     _monexitFastTryUnlockFailed

        /* If we get here, then the object is locked with a fastlock: */

        /* Make sure that the current thread owns the monitor: */
        ldr     TEMP, [OWNEDREC, #OFFSET_CVMOwnedMonitor_owner]
        cmp     TEMP, EE
        bne     _monexitFastTryUnlockFailed	/* If not owner, we failed. */

        /* If we get here, then the current thread does own the monitor,
           and all is well.  Proceed with unlocking: */
        ldr     EXPECTED_CNT, [OWNEDREC, #OFFSET_CVMOwnedMonitor_count]
        cmp     EXPECTED_CNT, #CONSTANT_CVM_INVALID_REENTRY_COUNT
        beq     _monexitFastTryUnlockFailed

#define ACTUAL_CNT      r0
#define NEW_COUNT       OBJ     /* r2 */

        str     OBJ, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+0]
        subs    NEW_COUNT, EXPECTED_CNT, #1
        beq     _monexitDoUnlock	/* If zero, then unlock */

        ; new monitor count > 0, so just update it and we're done:

        str     lr, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+8]
        add     r0, OWNEDREC, #OFFSET_CVMOwnedMonitor_count
        str     EXPECTED_CNT, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+4]
        ; r1 already has EXPECTED_CNT.
        ; r2 already has NEW_COUNT.

        CALL_VM_FUNCTION(InterlockedTestExchange)

        ldr     OBJ, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+0]
        ldr     EXPECTED_CNT, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+4]
        ldr     lr, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+8]

        cmp     ACTUAL_CNT, EXPECTED_CNT        /* ACTUAL_CNT is in r0. */
        bne     _monexitFastTryUnlockFailed     /* Go slow way. */

#undef ACTUAL_CNT
#undef NEW_COUNT

        ; we're done! monitor count was successfully decrement:
        mov     pc, lr  /* return to caller */

_monexitDoUnlock
#define ACTUAL_BITS     r0
#define EXPECTED_BITS   OWNEDREC
#define NEW_BITS        r2
        /* If we get here, then the re-entry count has reached 0. */
        ; Restore the obits to the object header:

        ldr     OBJ, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+0]
        add     r0, OBJ, #OFFSET_CVMObjectHeader_various32
        ; NEW_BITS is r2.
        ldr     NEW_BITS, [OWNEDREC, #OFFSET_CVMOwnedMonitor_u_fast_bits]
        mov     r1, EXPECTED_BITS

        str     EXPECTED_BITS, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+4]
        str     lr, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+8]

        CALL_VM_FUNCTION(InterlockedTestExchange)

        /* NOTE: EXPECTED_BITS is defined to be OWNEDREC.  By restoring
                 EXPECTED_BITS, we're also restoring OWNEDREC.
        */
        ldr     OBJ, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+0]
        ldr     EXPECTED_BITS, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+4]
        ldr     lr, [sp, #OFFSET_CVMCCExecEnv_ccmStorage+8]

        cmp     ACTUAL_BITS, EXPECTED_BITS      /* ACTUAL_BITS is in r0. */
        bne     _monexitFastTryUnlockFailed     /* Go slow way. */

#undef ACTUAL_BITS
#undef EXPECTED_BITS
#undef NEW_BITS

#ifdef CVM_DEBUG
        ; Make the lockrec play nice with the debug assertions:
        mov     TEMP, #CONSTANT_CVM_OWNEDMON_FREE
        str     TEMP, [OWNEDREC, #OFFSET_CVMOwnedMonitor_state]
        mov     TEMP, #0
        str     TEMP, [OWNEDREC, #OFFSET_CVMOwnedMonitor_u_fast_bits]
        str     TEMP, [OWNEDREC, #OFFSET_CVMOwnedMonitor_object]
        str     TEMP, [OWNEDREC, #OFFSET_CVMOwnedMonitor_count]
#endif

        ldr     EE, [sp, #OFFSET_CVMCCExecEnv_ee]

        ; Check if the lockrec is the first one on the thread's owned list:
        ldr     TEMP, [EE, #OFFSET_CVMExecEnv_objLocksOwned]
        cmp     TEMP, OWNEDREC
        bne     _monexitFastTryUnlockFindPrevLockRecord

        ; Remove the lockrec from the ee's owned list:
        ldr     TEMP, [OWNEDREC, #OFFSET_CVMOwnedMonitor_next]
        str     TEMP, [EE, #OFFSET_CVMExecEnv_objLocksOwned]

_monexitFastTryUnlockAddLockRecordToFreeList
        ; Add the lockrec to the ee's free list:
        ldr     TEMP, [EE, #OFFSET_CVMExecEnv_objLocksFreeOwned]
        str     TEMP, [OWNEDREC, #OFFSET_CVMOwnedMonitor_next]
        str     OWNEDREC, [EE, #OFFSET_CVMExecEnv_objLocksFreeOwned]

        mov     pc, lr  /* Return to the caller. */

/* It is safe to reuse NBITS at this point */
/* NOTE: PREV_REC must be defined to be TEMP because it depends on the value
         that TEMP is set to above (as its initial value) before coming here.
*/
#define PREV_REC        TEMP
#define TEMP2           EXPECTED_CNT
_monexitFastTryUnlockFindPrevLockRecord
        ldr     TEMP2, [PREV_REC, #OFFSET_CVMOwnedMonitor_next]
        cmp     TEMP2, OWNEDREC
        beq     _monexitFastTryUnlockFoundPrevLockRecord
        mov     PREV_REC, TEMP2
        b       _monexitFastTryUnlockFindPrevLockRecord

_monexitFastTryUnlockFoundPrevLockRecord
        ; Remove the lockrec from the ee's owned list:
        ldr     TEMP2, [OWNEDREC, #OFFSET_CVMOwnedMonitor_next]
        str     TEMP2, [PREV_REC, #OFFSET_CVMOwnedMonitor_next]
        b       _monexitFastTryUnlockAddLockRecordToFreeList
#undef PREV_REC
#undef TEMP2

_monexitFastTryUnlockFailed
        /* Let C helper do the hard stuff: */
        /* NOTE: It is expected that the OBJ pointer is already in r2. */
        FLUSH_TOS
        CALL_HELPER_AND_PASS_EE(CVMCCMruntimeMonitorExit)

#undef TEMP
#undef EXPECTED_CNT
#undef OBJ
#undef EE
#undef OWNEDREC

        ENTRY_END
#else
#error unsupported CVM_FASTLOCK_TYPE or CVM_MICROLOCK_TYPE
#endif 

	POOL
