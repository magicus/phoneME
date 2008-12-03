/*
 * @(#)Constructor.c	1.28 06/10/10
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
#include "generated/offsets/java_lang_reflect_Constructor.h"

#endif /* CVM_REFLECT */

#ifdef CVM_REFLECT
/*
 * Implementation essentially copied from Method.c and modified so it
 * throws Constructor's internal exceptions instead of Method's. See
 * Method.c for more documentation.
 */
static CNIResultCode
CVMinvokeConstructor(CVMExecEnv* ee, CVMStackVal32 *arguments,
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
     * read the java lang Class member variable classBlockPointer
     * into declaredCbAddr
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
    int i, numArgs;
    CVMClassBlock* icb;  /* instance class block */
    CVMMethodBlock* imb; /* instance method block */
    CVMInt32 override;

    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

    /* Get the methodblock pointer out of the Constructor object,
       which is the first argument to Constructor.newInstance(). NOTE
       use of the direct memory interface here. */

    methodObject = CVMID_icellDirect(ee, &arguments[0].j.r);
    CVMassert(methodObject != NULL); /* Java code checks for
					NullPointerException */

    declaredMb = CVMreflectGCUnsafeGetMethodBlock(
        ee, methodObject,
	CVMsystemClass(java_lang_reflect_Constructor_ArgumentException));
    CVMassert(declaredMb != NULL);

    CVMD_fieldReadRef(methodObject,
		      CVMoffsetOfjava_lang_reflect_Constructor_clazz,
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
	CVMBool initialized;
#ifdef CVM_TRACE
	CVMFrame *f = CVMeeGetCurrentFrame(ee);
#endif
        TRACE_FRAMELESS_METHOD_RETURN(*p_mb, f);
	initialized = CVMreflectEnsureInitialized(ee, declaredCb);
	TRACE_FRAMELESS_METHOD_CALL(f, *p_mb, CVM_FALSE);
	if (!initialized) {
	    return CNI_EXCEPTION;
	}
	/* recache methodObject since we become gcsafe. */
	methodObject = CVMID_icellDirect(ee, &arguments[0].j.r);
    }

    obj = CVMID_icellDirect(ee, &arguments[1].j.r);
    CVMassert(obj != NULL); /* Constructor can't operate on null
			       object. If object is null, an exception
			       should have been thrown and we
			       shouldn't have gotten here. */

    /* Constructor methodblocks can't be static or virtual, and
       furthermore Jcc always sets constructors' method table slots 
       to slot 0 (same as finalize()) so it is actually incorrect to
       do what should be a no-op CVMreflectGetObjectMethodblock. Let's
       settle for some assertions. */
    CVMassert(!CVMmbIs(declaredMb, STATIC));
    CVMassert(declaredCb == CVMobjectGetClass(obj));
    icb = declaredCb;
    imb = declaredMb;

    /*
     * Access checking (unless overridden by Method)
     */
    CVMD_fieldReadInt(methodObject,
		      CVMoffsetOfjava_lang_reflect_AccessibleObject_override,
		      override);
    if (!override) {
	if (!(CVMcbIs(declaredCb, PUBLIC) && CVMmbIs(declaredMb, PUBLIC))) {
	    CVMClassBlock* exceptionCb = 
		CVMsystemClass(java_lang_reflect_Constructor_AccessException);
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

    frame = (CVMFrame*)CVMpushTransitionFrame(ee, declaredMb);
    if (frame == NULL) {
	return CNI_EXCEPTION;   /* exception already thrown */
    }

    /* Push the object which was the second argument to
       Constructor.invoke() on the operand stack of the transition
       frame */

    frame->topOfStack->ref = arguments[1].j.r;
    frame->topOfStack++;

    /* Obtain the arguments to Constructor.invoke() from the local
       variable section. Note that arguments, the "top of the operand
       stack", actually points to the bottom of the set of arguments;
       that is, arguments[0] corresponds to the first argument to this
       method, arguments[1] to the second, etc. */

    /* Recache methodObject in case pushTransitionFrame caused GC */
    methodObject = CVMID_icellDirect(ee, &arguments[0].j.r);
    CVMassert(methodObject != NULL);
    args = (CVMArrayOfRef*) CVMID_icellDirect(ee, &arguments[2].j.r);
    CVMD_fieldReadRef(methodObject,
		      CVMoffsetOfjava_lang_reflect_Constructor_parameterTypes,
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
	    CVMsystemClass(java_lang_reflect_Constructor_ArgumentException);
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
				       (CVMClassBlock*) cbAddr,
				       exceptionCb)) {
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

    *p_mb = declaredMb;
    return CNI_NEW_TRANSITION_FRAME;
}
#endif /* CVM_REFLECT */

CNIEXPORT CNIResultCode
CNIjava_lang_reflect_Constructor_allocateUninitializedObject(
	CVMExecEnv* ee,
	CVMStackVal32 *arguments,
	CVMMethodBlock **p_mb
)
{
#ifdef CVM_REFLECT
    CVMObject* clazz;
    /*
     * Access member variable of type 'int'
     * and cast it to a native pointer. The java type 'int' only garanties 
     * 32 bit, but because one slot is used as storage space and
     * a slot is 64 bit on 64 bit platforms, it is possible 
     * to store a native pointer without modification of
     * java source code. This assumes that all places in the C-code
     * are catched which set/get this member.
     */
    CVMAddr cbAddr;
    CVMClassBlock* cb;

    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

    /* This is a little scary, but should work. We reuse the Object
       stack slot reserved for the incoming java.lang.Class object for
       our return value. Note we don't change the "refness" of the
       stack slot so we should be okay even if we GC in unexpected
       places. We should also be okay in the case of GC'ing
       classes and classblocks, because the switch from the object
       being a Class to being an instance of that class is atomic with
       respect to GC, and in both cases we will be scanning the
       Class. */
    clazz = CVMID_icellDirect(ee, &arguments[0].j.r);
    CVMD_fieldReadAddr(clazz, CVMoffsetOfjava_lang_Class_classBlockPointer,
		       cbAddr);
    cb = (CVMClassBlock*) cbAddr;

    /* If we're are trying to instantiate an instance of java.lang.Class,
       then this should result in an IllegalAccessException.  Don't bother
       allocating the unused object.  Just throw the exception. */
    if (cb == CVMsystemClass(java_lang_Class)) {
        CVMthrowIllegalAccessException(ee, NULL);
        return CNI_EXCEPTION;
    }

    if (CVMcbIs(cb, INTERFACE) || CVMcbIs(cb, ABSTRACT)) {
	CVMthrowInstantiationException(ee,
				       "class type is interface or abstract");
	return CNI_EXCEPTION;
    }

    CVMD_gcSafeExec(ee, {
	CVMID_allocNewInstance(ee, cb, &arguments[0].j.r);
    });
    
    if (CVMID_icellIsNull(&arguments[0].j.r)) {
	CVMthrowOutOfMemoryError(ee, NULL);
	return CNI_EXCEPTION;
    }
	
    return CNI_SINGLE;
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(ee, NULL);
    return CNI_EXCEPTION;
#endif /* CVM_REFLECT */
}

CNIEXPORT CNIResultCode
CNIjava_lang_reflect_Constructor_invokeConstructor(CVMExecEnv* ee,
						   CVMStackVal32 *arguments,
						   CVMMethodBlock **p_mb)
{
    /* CNI policy: offer a gc-safe checkpoint */
    CVMD_gcSafeCheckPoint(ee, {}, {});

#ifdef CVM_REFLECT
    return CVMinvokeConstructor(ee, arguments, p_mb);
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(ee, NULL);
    return CNI_EXCEPTION;
#endif /* CVM_REFLECT */
}
