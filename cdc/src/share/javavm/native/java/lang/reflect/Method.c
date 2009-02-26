/*
 * @(#)Method.c	1.30 06/10/10
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

#include "javavm/include/interpreter.h"
#include "javavm/include/directmem.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/utils.h"
#include "javavm/include/common_exceptions.h"
#include "javavm/include/reflect.h"
#include "javavm/export/jvm.h"

#ifdef CVM_REFLECT

#include "generated/offsets/java_lang_Class.h"
#include "generated/offsets/java_lang_reflect_AccessibleObject.h"
#include "generated/offsets/java_lang_reflect_Method.h"

#endif /* CVM_REFLECT */

#ifdef CVM_REFLECT

/*
 * This is where the invocation of reflected methods actually
 * occurs.
 *
 * A transition frame is pushed with enough room to store the
 * arguments of the method to be invoked, and its methodblock set up
 * to point to the methodblock of that method. Then we step down the
 * second argument to Method.invoke(), an array of Objects, verifying
 * that each is of the appropriate type as specified in the
 * parameterTypes array, unboxing primitive types, and pushing the
 * data onto the transition frame's operand stack. At this point we
 * are prepared to return to the interpreter loop with an
 * CNI_NEW_TRANSITION_FRAME result code, which will indicate to the
 * interpreter to set up the transition code and program counter and
 * resume execution in the transition frame, which will ultimately
 * cause the invokee to get called.
 *
 * The invokee returns its value as it
 * ordinarily would through whatever mechanism it is used to (Java's
 * [iad...]return, JNI's return mechanism in CVMjniInvokeNative, or
 * CVM's "top of stack" return mechanism). In all cases the return
 * value is pushed on the top of the transition frame's stack. The
 * opc_exittransition opcode, which, except in the case of an
 * exception, is executed immediately after the invokee returns,
 * checks the return type of the method to find the size of the return
 * value in words (0, 1, or 2) and copies that many words from the
 * transition frame's stack to the caller's stack.
 *
 * This function should only be called from GC-unsafe code. It may,
 * however, become GC-safe internally (due to a necessary call to
 * CVMpushTransitionFrame).
 */
static CNIResultCode
CVMinvokeMethod(CVMExecEnv* ee, CVMStackVal32 *arguments,
		CVMMethodBlock **p_mb)
{
    /* NOTE that we are gc-UNSAFE when we enter and exit this
       method. This is very important. If the GC-safety policy for CVM
       native methods changes, much of this code will need to be run
       in an CVMD_gcUnsafeExec block. */

    CVMFrame* frame;
    CVMObject* methodObject;
    CVMObject* obj;
    CVMMethodBlock* declaredMb;
    CVMObject* declaredClass;
    /* 
     * declaredCbAddr holds a native pointer as an int value
     * therefore change the type to CVMAddr which is 4 byte on
     * 32 bit platforms and 8 byte on 64 bit platforms
     */
    CVMAddr declaredCbAddr;
    CVMClassBlock* declaredCb;
    CVMArrayOfRef* args;
    CVMObject* arg;
    CVMObject* argTypes;
    CVMObject* argTypeClass;
    /* 
     * cbAddr holds a native pointer as an int value
     * therefore change the type to CVMAddr which is 4 byte on
     * 32 bit platforms and 8 byte on 64 bit platforms
     */
    CVMAddr cbAddr;
    CVMBool isStatic;
    int i, numArgs;
    CVMClassBlock* icb;  /* instance class block */
    CVMMethodBlock* imb; /* instance method block */
    CVMInt32 override;

    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

    /* Get the methodblock pointer out of the Method object, which is
       the first argument to Method.invoke(). NOTE use of the direct
       memory interface here. */

    methodObject = CVMID_icellDirect(ee, &arguments[0].j.r);
    CVMassert(methodObject != NULL); /* Java code checks for
					NullPointerException */

    declaredMb = CVMreflectGCUnsafeGetMethodBlock(
        ee, methodObject, 
	CVMsystemClass(java_lang_reflect_Method_ArgumentException));
    CVMassert(declaredMb != NULL);

    CVMD_fieldReadRef(methodObject,
		      CVMoffsetOfjava_lang_reflect_Method_clazz,
		      declaredClass);
    /* 
     * Access member variable of type 'int'
     * and cast it to a native pointer. The java type 'int' only garanties 
     * 32 bit, but because one slot is used as storage space and
     * a slot is 64 bit on 64 bit platforms, it is possible 
     * to store a native pointer without modification of
     * java source code. This assumes that all places in the C-code
     * are catched which set/get this member.
     */
    CVMD_fieldReadAddr(declaredClass,
		       CVMoffsetOfjava_lang_Class_classBlockPointer,
		       declaredCbAddr);
    declaredCb = (CVMClassBlock*) declaredCbAddr;

    /*
     * We need to make sure the class is initialized.
     */
    if (!CVMcbInitializationDoneFlag(ee, declaredCb)) {
	if (!CVMreflectEnsureInitialized(ee, declaredCb)) {
	    return CNI_EXCEPTION;
	}
	/* recache methodObject since we become gcsafe. */
	methodObject = CVMID_icellDirect(ee, &arguments[0].j.r);
    }

    isStatic = CVMmbIs(declaredMb, STATIC);
    obj = CVMID_icellDirect(ee, &arguments[1].j.r);

    if (isStatic) {
	icb = declaredCb;
	imb = declaredMb;
    } else {
	/* Check class of object against class declaring method; from
           jvm.c */
	icb = CVMobjectGetClass(obj);
	if (!(icb == declaredCb ||
	      CVMgcUnsafeIsInstanceOf(ee, obj, declaredCb))) {
	    CVMsignalError(
                ee,
		CVMsystemClass(java_lang_reflect_Method_ArgumentException),
		"object is not an instance of declaring class");
	    return CNI_EXCEPTION;
	}
	/* Resolve to instance method */
	if ((imb = CVMreflectGetObjectMethodblock(ee, icb,
						  declaredMb)) == NULL)
	    return CNI_EXCEPTION;		/* exception */
    }

    /*
     * Access checking (unless overridden by Method)
     */
    CVMD_fieldReadInt(methodObject,
		      CVMoffsetOfjava_lang_reflect_AccessibleObject_override,
		      override);
    if (!override) {
	if (!(CVMcbIs(declaredCb, PUBLIC) && CVMmbIs(declaredMb, PUBLIC))) {
	    CVMClassBlock* exceptionCb =
		CVMsystemClass(java_lang_reflect_Method_AccessException);
	    if (!CVMreflectCheckAccess(ee, declaredCb,
				       CVMmbAccessFlags(declaredMb),
				       icb, CVM_TRUE, exceptionCb)) {
		return CNI_EXCEPTION;		/* exception */
	    }
	}
    }

    /* Allocate the transition frame. Note that this might become
       GC-safe, so we need to make sure we haven't screwed up our
       frame's top of stack. If we'd moved it down at this point, it's
       possible our arguments wouldn't get scanned and would therefore
       be GC'd. Also at this point note that the direct methodObject
       reference above is invalid. */

    frame = (CVMFrame*)CVMpushTransitionFrame(ee, imb);
    if (frame == NULL) {
	return CNI_EXCEPTION;   /* exception already thrown */
    }

    /* If the methodblock is non-static, we need to push the object
       which was the second argument to Method.invoke() on the operand
       stack of the transition frame */

    if (!isStatic) {
	frame->topOfStack->ref = arguments[1].j.r;
	frame->topOfStack++;
    }

    /* Obtain the arguments to Method.invoke() from the local variable
       section. Note that arguments, the "top of the operand stack",
       actually points to the bottom of the set of arguments; that is,
       arguments[0] corresponds to the first argument to this method,
       arguments[1] to the second, etc. */

    /* Recache methodObject in case pushTransitionFrame caused GC */
    methodObject = CVMID_icellDirect(ee, &arguments[0].j.r);
    CVMassert(methodObject != NULL);
    args = (CVMArrayOfRef*) CVMID_icellDirect(ee, &arguments[2].j.r);
    CVMD_fieldReadRef(methodObject,
		      CVMoffsetOfjava_lang_reflect_Method_parameterTypes,
		      argTypes);
    /* NOTE: number of arguments checked above in Java */
    if (CVMD_arrayGetLength((CVMArrayOfRef*) argTypes) > 0)
	CVMassert(CVMD_arrayGetLength(args) ==
		  CVMD_arrayGetLength((CVMArrayOfRef*) argTypes));

    numArgs = CVMD_arrayGetLength((CVMArrayOfRef*) argTypes);

    /* NOTE: should probably offer GC checkpoints every few arguments.
       However, will have to recache the direct pointers we hold if GC
       actually occurred. Also, stackmap won't be present for
       transition frame, but that probably doesn't matter since object
       arguments will get scanned from the previous frame's stack. */
    for (i = 0; i < numArgs; i++) {
	CVMClassBlock* exceptionCb =
	    CVMsystemClass(java_lang_reflect_Method_ArgumentException);
	CVMD_arrayReadRef(args, i, arg);
	CVMD_arrayReadRef((CVMArrayOfRef*) argTypes, i, argTypeClass);
	/* 
	 * Access member variable of type 'int'
	 * and cast it to a native pointer. The java type 'int' only garanties 
	 * 32 bit, but because one slot is used as storage space and
	 * a slot is 64 bit on 64 bit platforms, it is possible 
	 * to store a native pointer without modification of
	 * java source code. This assumes that all places in the C-code
	 * are catched which set/get this member.
	 */
	CVMD_fieldReadAddr(argTypeClass,
			   CVMoffsetOfjava_lang_Class_classBlockPointer,
			   cbAddr);
	if (!CVMreflectGCUnsafePushArg(ee, frame, arg,
				       (CVMClassBlock*) cbAddr, exceptionCb)) {
	    CVMpopFrame(&ee->interpreterStack, frame);
	    return CNI_EXCEPTION; /* exception occurred */
	}
    }

    /* Note we leave myFrame->topOfStack untouched, because it is
       decached by the interpreter when we return, but it is very
       important that we don't offer a GC-safe point after
       mutating the stack slot(s) corresponding to the return
       value because of possible changes of refness, which would
       make the stackmap incorrect. */

    *p_mb = imb;
    return CNI_NEW_TRANSITION_FRAME;
}

#endif /* CVM_REFLECT */

/* The CVM methods implementing the native code for
   java.lang.reflect.Method can all share the same C code. */

CNIEXPORT CNIResultCode
CNIjava_lang_reflect_Method_invokeV(CVMExecEnv* ee, CVMStackVal32 *arguments,
				    CVMMethodBlock **p_mb)
{
#ifdef CVM_REFLECT
    return CVMinvokeMethod(ee, arguments, p_mb);
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(ee, NULL);
    return CNI_EXCEPTION;
#endif /* CVM_REFLECT */
}

CNIEXPORT CNIResultCode
CNIjava_lang_reflect_Method_invokeZ(CVMExecEnv* ee, CVMStackVal32 *arguments,
				    CVMMethodBlock **p_mb)
{
#ifdef CVM_REFLECT
    return CVMinvokeMethod(ee, arguments, p_mb);
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(ee, NULL);
    return CNI_EXCEPTION;
#endif /* CVM_REFLECT */
}

CNIEXPORT CNIResultCode
CNIjava_lang_reflect_Method_invokeC(CVMExecEnv* ee, CVMStackVal32 *arguments,
				    CVMMethodBlock **p_mb)
{
#ifdef CVM_REFLECT
    return CVMinvokeMethod(ee, arguments, p_mb);
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(ee, NULL);
    return CNI_EXCEPTION;
#endif /* CVM_REFLECT */
}

CNIEXPORT CNIResultCode
CNIjava_lang_reflect_Method_invokeF(CVMExecEnv* ee, CVMStackVal32 *arguments,
				    CVMMethodBlock **p_mb)
{
#ifdef CVM_REFLECT
    return CVMinvokeMethod(ee, arguments, p_mb);
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(ee, NULL);
    return CNI_EXCEPTION;
#endif /* CVM_REFLECT */
}

CNIEXPORT CNIResultCode
CNIjava_lang_reflect_Method_invokeD(CVMExecEnv* ee, CVMStackVal32 *arguments,
				    CVMMethodBlock **p_mb)
{
#ifdef CVM_REFLECT
    return CVMinvokeMethod(ee, arguments, p_mb);
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(ee, NULL);
    return CNI_EXCEPTION;
#endif /* CVM_REFLECT */
}

CNIEXPORT CNIResultCode
CNIjava_lang_reflect_Method_invokeB(CVMExecEnv* ee, CVMStackVal32 *arguments,
				    CVMMethodBlock **p_mb)
{
#ifdef CVM_REFLECT
    return CVMinvokeMethod(ee, arguments, p_mb);
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(ee, NULL);
    return CNI_EXCEPTION;
#endif /* CVM_REFLECT */
}

CNIEXPORT CNIResultCode
CNIjava_lang_reflect_Method_invokeS(CVMExecEnv* ee, CVMStackVal32 *arguments,
				    CVMMethodBlock **p_mb)
{
#ifdef CVM_REFLECT
    return CVMinvokeMethod(ee, arguments, p_mb);
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(ee, NULL);
    return CNI_EXCEPTION;
#endif /* CVM_REFLECT */
}

CNIEXPORT CNIResultCode
CNIjava_lang_reflect_Method_invokeI(CVMExecEnv* ee, CVMStackVal32 *arguments,
				    CVMMethodBlock **p_mb)
{
#ifdef CVM_REFLECT
    return CVMinvokeMethod(ee, arguments, p_mb);
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(ee, NULL);
    return CNI_EXCEPTION;
#endif /* CVM_REFLECT */
}

CNIEXPORT CNIResultCode
CNIjava_lang_reflect_Method_invokeL(CVMExecEnv* ee, CVMStackVal32 *arguments,
				    CVMMethodBlock **p_mb)
{
#ifdef CVM_REFLECT
    return CVMinvokeMethod(ee, arguments, p_mb);
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(ee, NULL);
    return CNI_EXCEPTION;
#endif /* CVM_REFLECT */
}

CNIEXPORT CNIResultCode
CNIjava_lang_reflect_Method_invokeA(CVMExecEnv* ee, CVMStackVal32 *arguments,
				    CVMMethodBlock **p_mb)
{
#ifdef CVM_REFLECT
    return CVMinvokeMethod(ee, arguments, p_mb);
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(ee, NULL);
    return CNI_EXCEPTION;
#endif /* CVM_REFLECT */
}
