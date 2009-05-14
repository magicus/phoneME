/*
 * @(#)classlink.c	1.111 06/10/25
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

#ifdef CVM_CLASSLOADING

#include "javavm/include/interpreter.h"
#include "javavm/include/directmem.h"
#ifdef CVM_DUAL_STACK
#include "javavm/include/dualstack_impl.h"
#endif
#include "javavm/include/indirectmem.h"
#include "javavm/include/common_exceptions.h"
#include "javavm/include/preloader.h"
#include "javavm/include/utils.h"
#include "javavm/include/typeid.h"
#include "javavm/include/gc_common.h"
#include "javavm/include/stackmaps.h"
#include "javavm/include/limits.h"
#include "javavm/include/opcodes.h"
#ifdef CVM_JVMTI
#include "javavm/include/jvmtiExport.h"
#endif
#ifdef CVM_JIT
#include "javavm/include/ccm_runtime.h"
#endif
#ifdef CVM_SPLIT_VERIFY
#include "javavm/include/split_verify.h"
#endif
#ifdef CVM_HW
#include "include/hw.h"
#endif

static CVMBool
CVMclassPrepare(CVMExecEnv* ee, CVMClassBlock* cb, CVMBool isRedefine);

static CVMBool
CVMclassPrepareFields(CVMExecEnv* ee, CVMClassBlock* cb, CVMBool isRedefine);

static CVMBool
CVMclassPrepareMethods(CVMExecEnv* ee, CVMClassBlock* cb);

static CVMBool
CVMclassPrepareInterfaces(CVMExecEnv* ee, CVMClassBlock* cb);


/* Initialize a static final field with a constant value. */
static void 
CVMinitializeStaticField(CVMExecEnv* ee, CVMFieldBlock* fb,
			 CVMConstantPool* cp, CVMUint16 cpIdx);

/* 
 * Perform the class linking as described in the VM spec,
 * 2nd Edition (2.17.3).
 */
CVMBool
CVMclassLink(CVMExecEnv* ee, CVMClassBlock* cb, CVMBool isRedefine)
{
    CVMBool success = CVM_TRUE;

    /* C stack redzone check */
    if (!CVMCstackCheckSize(ee, CVM_REDZONE_CVMclassLink, 
        "CVMclassLink", CVM_TRUE)) {
	return CVM_FALSE;
    }

    if (CVMcbCheckRuntimeFlag(cb, LINKED)) {
        return CVM_TRUE;
    }

    CVMtraceClassLink(("LK: Linking class: %C\n", cb));

    CVMassert(CVMcbCheckRuntimeFlag(cb, SUPERCLASS_LOADED));

    {
        /*
	 * Make sure the current class links with the system Throwable
	 * class. See bug 4157312.
	 */
        CVMClassBlock* myThrowable = 
	    CVMclassLookupByNameFromClass(ee, "java/lang/Throwable",
					  CVM_FALSE, cb);
	if (myThrowable == NULL) {
	    return CVM_FALSE; /* exception already thrown */

	}
	if (CVMsystemClass(java_lang_Throwable) != myThrowable) {
	    CVMthrowLinkageError(ee, "wrong Throwable class linked with %C",
				 cb);
	    return CVM_FALSE;
	}
    }

    /* link superclass */
    {
	CVMClassBlock* superCb = CVMcbSuperclass(cb);
	if (superCb != NULL) {
	    if (CVMcbIs(superCb, INTERFACE)) {
		CVMthrowIncompatibleClassChangeError(
		    ee, "Class %C has interface %C as super class",
		    cb, superCb);
		return CVM_FALSE;
	    }
	    if (!CVMcbCheckRuntimeFlag(superCb, LINKED)) {
		/* %comment c */
		if (!CVMclassLink(ee, superCb, isRedefine)) {
		    return CVM_FALSE; /* exception already thrown */
		}
	    }
	}
	/* Propagate the <clinit> flag from the super class so 
	 * that the flag of this class gets set if any of the 
	 * super classes has <clinit>. */
	if (CVMcbHasStaticsOrClinit(superCb)) {
	    CVMcbSetHasStaticsOrClinit(cb, ee);
	}
    }
    
    /*
     * It is OK to enter the class monitor here because no methods
     * in the class will be executing yet at this moment.
     *
     * Unlike JDK, we need to grab the lock before linking the interfaces.
     * This is because CVMcbInterfaces() changes during linking and
     * CVMcbImplementsCount() and CVMcbInterfacecb() both use
     * CVMcbInterfaces(). If another thread were to interrupt us and link
     * this class after CVMcbInterfacecb() was cached in a register, then
     * we'd be hosed because it would point to freed up memory.
     */
    if (!CVMgcSafeObjectLock(ee, CVMcbJavaInstance(cb))) {
	CVMthrowOutOfMemoryError(ee, NULL);
	return CVM_FALSE;
    }

    /* In case the class was linked while waiting for the lock, or
     * possibly even before we attempted to grab the lock.
     */
    if (CVMcbCheckRuntimeFlag(cb, LINKED)) {
	/* Trace to help SQE verify that race conditions are being tested. */
	CVMtraceClassLink(("LK: Race detected linking class: %C\n", cb));
	goto unlock;
    }

    /* link interfaces */
    {
	CVMUint16 i;
	CVMBool hasStaticsOrClinit = CVM_FALSE;
	for (i = 0; i < CVMcbImplementsCount(cb); i++) {
	    if (!CVMcbCheckRuntimeFlag(CVMcbInterfacecb(cb, i), LINKED)) {
		/* %comment c */
		if (!CVMclassLink(ee, CVMcbInterfacecb(cb, i), isRedefine)) {
		    success = CVM_FALSE; /* exception already thrown */
		    goto unlock;
		}
	    }
	    if (CVMcbHasStaticsOrClinit(CVMcbInterfacecb(cb, i))) {
		hasStaticsOrClinit = CVM_TRUE;
	    }
	}
	/* If any of interfaces implemented in this class has <clinit>,
	 * set the HAS_STATICS_OR_CLINIT flag of this class too. */
	if (hasStaticsOrClinit) {
	    CVMcbSetHasStaticsOrClinit(cb, ee);
	}
    }

    /* Verify the class if necessary. */
    if (CVMloaderNeedsVerify(ee, CVMcbClassLoader(cb), CVM_TRUE)) {
	CVMBool verified = CVM_FALSE;
        CVMBool isMidletClass = CVM_FALSE;
#ifdef CVM_DUAL_STACK
        CVMD_gcUnsafeExec(ee, {
            isMidletClass = CVMclassloaderIsMIDPClassLoader(
                ee, CVMcbClassLoader(cb), CVM_FALSE);
        });
#endif
#ifdef CVM_SPLIT_VERIFY
	/*
	 * JVM spec 3rd edition says there is an implicit empty
	 * StackMapTable attribute if none is specified.
         *
         * Midlet classes should be verified with the split verifier just
         * in case there is a missing StackMap resource.
         * Redefined classes use full verifier since Netbeans doesn't
         * copy stackmaps to new methods.
	 */
	if (CVMglobals.splitVerify && !isRedefine &&
            (CVMsplitVerifyClassHasMaps(ee, cb) ||
             isMidletClass ||
             cb->major_version >= 50))
        {
	    verified = (CVMsplitVerifyClass(ee, cb, isRedefine) == 0);
#ifndef CVM_50_0_FALL_BACK
	    if (!verified) {
		success = CVM_FALSE;
		goto unlock;
	    }
#else
	    /* Falling back to full verifier is allowed for 50.0 */
	    if (!verified) {
		if (cb->major_version == 50 && cb->minor_version == 0) {
		    CVMclearLocalException(ee);
		} else {
		    success = CVM_FALSE;
		    goto unlock;
		}
	    }
#endif
	}
#endif
	if (!verified) {
	    verified = CVMclassVerify(ee, cb, isRedefine);
	}
        if (!verified) {
	    success = CVM_FALSE;
	    goto unlock;
	}
    }

    /* The verification process may have (recursively) prepared cb. */
    if (CVMcbCheckRuntimeFlag(cb, LINKED)) {
	goto unlock;
    }

    /* Prepare the classes methods, fields, and interfaces. */
    if (!CVMclassPrepare(ee, cb, isRedefine)) {
	success = CVM_FALSE;
	goto unlock;
    }

    CVMcbSetRuntimeFlag(cb, ee, LINKED);

#ifdef CVM_JVMTI
    if (CVMjvmtiIsEnabled()) {
	CVMjvmtiPostClassPrepareEvent(ee, CVMcbJavaInstance(cb));
    }
#endif

 unlock:
    if (!CVMgcSafeObjectUnlock(ee, CVMcbJavaInstance(cb))) {
	CVMassert(CVM_FALSE); /* should never happen */
    }

    if (success) {
	CVMtraceClassLink(("LK: Class linked: %C\n", cb));
    } else {
	CVMtraceClassLink(("LK: Class NOT linked: %C\n", cb));
    }

    return success;
}


static CVMBool
CVMclassPrepare(CVMExecEnv* ee, CVMClassBlock* cb, CVMBool isRedefine) {
    CVMtraceClassLoading(("CL: Preparing class %C.\n", cb));

    if (!CVMclassPrepareFields(ee, cb, isRedefine)) {
	goto failed;
    }
    /* bug #4940678. make sure FIELDS_PREPARED flag isn't clobbered */
    CVMassert(CVMcbCheckRuntimeFlag(cb, FIELDS_PREPARED));
    if (!CVMclassPrepareMethods(ee, cb)) {
	goto failed;
    }
    /* bug #4940678. make sure FIELDS_PREPARED flag isn't clobbered */
    CVMassert(CVMcbCheckRuntimeFlag(cb, FIELDS_PREPARED));
    if (!CVMclassPrepareInterfaces(ee, cb)) {
	goto failed;
    }
    /* bug #4940678. make sure FIELDS_PREPARED flag isn't clobbered */
    CVMassert(CVMcbCheckRuntimeFlag(cb, FIELDS_PREPARED));
    
    CVMtraceClassLoading(("CL: Done preparing class %C.\n", cb));
    return CVM_TRUE;

 failed:
    CVMtraceClassLoading(("CL: Failed preparing class %C.\n", cb));
    return CVM_FALSE;
}


static CVMBool
CVMclassPrepareFields(CVMExecEnv* ee, CVMClassBlock* cb, CVMBool isRedefine)
{
    CVMUint16      i;
    CVMClassBlock* superCb = CVMcbSuperclass(cb);
    CVMGCBitMap    superGcMap = CVMcbGcMap(superCb);
    CVMGCBitMap    gcMap;
    CVMUint32      numFieldWords;
    CVMUint32      numSuperFieldWords;
    CVMUint32      instanceSize;
    CVMUint32      fieldOffset; /* word offset for next field */
    CVMUint32      currBigMapIndex = 0;
    CVMUint16      nextMapBit;
    CVMBool        hasShortMap;
    /* 
     * Make 'map' the same type as 'map' in 'CVMGCBitMap'.
     */
    CVMAddr        map;  /* current map we are writing into. */
    CVMConstantPool* cp = CVMcbConstantPool(cb);

    /* offset of next nonref static field */
    CVMUint16 nextStaticFieldOffset = CVMcbNumStaticRefs(cb);
    /* offset of next ref static field */
    CVMUint16 nextStaticRefFieldOffset = 0;

    /*
     * Check if fields have already been prepared.
     */
    if (CVMcbCheckRuntimeFlag(cb, FIELDS_PREPARED)) {
	return CVM_TRUE;
    }
    /* It's a bug if we've already called CVMclassPrepareMethods() */
    CVMassert(CVMcbMethodTableCount(cb) == 0);

    /*
     * When the class was first loaded, CVMcbInstanceSize() was 
     * initialized with the number words used by non-static
     * fields this class has (not counting the superclasses). This
     * save us from having to make 2 passes over the fb's here.
     */
    numFieldWords = CVMcbInstanceSize(cb);
    numSuperFieldWords = 
	(CVMcbInstanceSize(superCb) - sizeof(CVMObjectHeader)) /
	 sizeof(CVMObject*);
    numFieldWords += numSuperFieldWords;

    if (numFieldWords > CVM_LIMIT_OBJECT_NUMFIELDWORDS) {
#ifdef JAVASE
        CVMthrowOutOfMemoryError(
            ee, "Class %C exceeds the 64K byte object size limit", cb);
#else
        CVMthrowInternalError(
            ee, "Class %C exceeds the 64K byte object size limit", cb);
#endif
	return CVM_FALSE;
    }

    /* Set fieldOffset to just past the last field of the superclass. */
    fieldOffset =
	numSuperFieldWords + sizeof(CVMObjectHeader) / sizeof(CVMObject*);

    /*
     * Determine if this class needs a long or a short gc map and copy
     * the superclass' gc map into this class' gc map.
     */
    hasShortMap = numFieldWords + CVM_GCMAP_NUM_FLAGS <= 32;
    if (hasShortMap) {
	/* 
	 * Both the current class and the superclass have short versions
	 * of the gcmap. Just copy the superclass map and turn off the flags
	 * for now.
	 */
	gcMap.map = superGcMap.map >>= CVM_GCMAP_NUM_FLAGS;
    } else {
	/*
	 * The current class needs a long version of the gc map.
	 */
	/* 
	 * Make 'superGcMapFlags' the same type as 'map' in 'CVMGCBitMap'.
	 */
	CVMAddr   superGcMapFlags = superGcMap.map & CVM_GCMAP_FLAGS_MASK;
	CVMUint32 maplen = (numFieldWords + 31) / 32;
	
	/* Allocate space for the current class' long map. Allocate one
	 * extra entry for the extremely paranoid case of the map
	 * reader reading off the edge of "legal" memory, in gc_common.h.
	 */
	gcMap.bigmap = (CVMBigGCBitMap*)
	    calloc(1, CVMoffsetof(CVMBigGCBitMap, map) +
		       (maplen + 1) * sizeof(gcMap.bigmap->map[0]));
	if (gcMap.bigmap == NULL) {
	    CVMthrowOutOfMemoryError(ee, NULL);
	    return CVM_FALSE;
	}
	gcMap.bigmap->maplen = maplen;

	if (superGcMapFlags == CVM_GCMAP_SHORTGCMAP_FLAG) {
	    /*
	     * The superclass has a short gcmap and the current class
	     * needs a long one, so just copy the superclass's short
	     * map into the last word of the long map.
	     */
	    gcMap.bigmap->map[0] = (superGcMap.map >> CVM_GCMAP_NUM_FLAGS);
	} else {
	    /* 
	     * Both the superclass and the current class have long maps.
	     * Copy the superclass's long map into the current class'
	     * longmap.
	     */
	    CVMBigGCBitMap* superBigMap;
	    CVMUint16 i;
	    CVMassert(superGcMapFlags == CVM_GCMAP_LONGGCMAP_FLAG);
	    /* 
	     * superGcMap.bigmap is a native pointer
	     * (union CVMGCBitMap in classes.h)
	     * therefore the cast has to be CVMAddr which is 4 byte on
	     * 32 bit platforms and 8 byte on 64 bit platforms
	     */
	    superBigMap = (CVMBigGCBitMap*)
		((CVMAddr)superGcMap.bigmap & ~CVM_GCMAP_FLAGS_MASK);
	    for (i = 0; i < superBigMap->maplen; i++) {
		gcMap.bigmap->map[i] = superBigMap->map[i];
	    }
	}
    }

    /* 
     * Setup the "map" variable that we will be setting bits in. Also
     * calculate the next bit in "map" that we will be working on.
     */
    nextMapBit = numSuperFieldWords % 32;
    if (hasShortMap) {
	map = gcMap.map;
    } else {
	currBigMapIndex = numSuperFieldWords / 32;
	map = gcMap.bigmap->map[currBigMapIndex];
    } 

    /* 
     * Iteterate over each field in the class, doing the following bookkeeping
     * in the process:
     *   -Calculate the object offset of each non-static field
     *   -Calculate the offset into the class' statics area for 
     *    each static field.
     *   -Compute the gcmap for the class.
     *   -Initialize each static field that has a ConstantValue attribute.
     */
    for (i = 0; i < CVMcbFieldCount(cb) ; i++) {
	CVMFieldBlock* fb = CVMcbFieldSlot(cb, i);
	if (CVMfbIs(fb, STATIC)) {
	    /* 
	     * Code in classcreate.c stored the constant pool index of
	     * the initial value for static fields in the CVMfbOffset()
	     * field. Grab it before we clobber this field with the
	     * actual offset of the static field.
	     */
	    CVMUint16 constantValueIdx = CVMfbOffset(fb);

	    /* 
	     * Set the offset of the field in the statics area. We put
	     * all the ref types first, so gc can find them easily.
	     */
	    if (CVMtypeidFieldIsRef(CVMfbNameAndTypeID(fb))) {
		CVMfbOffset(fb) = nextStaticRefFieldOffset;
		/* Make room for the next ref static. */
		nextStaticRefFieldOffset++;
	    } else {
		CVMfbOffset(fb) = nextStaticFieldOffset;
		/* Make room for the next non-ref static. */
		nextStaticFieldOffset++;
		if (CVMtypeidFieldIsDoubleword(CVMfbNameAndTypeID(fb))) {
		    nextStaticFieldOffset++;
		}
	    }

	    /*
	     * If the field is a static with a ConstantValue attribute, then 
	     * initialize it.
	     */
	    if (constantValueIdx != 0 && !isRedefine) {
		CVMinitializeStaticField(ee, fb, cp, constantValueIdx);
	    }
	} else { /* Field is not static */	    
	    /* Set the object offset of this field */
	    CVMfbOffset(fb) = fieldOffset;

	    /* 
	     * Calculate the object offset of the next field and also set
	     * the gcmap bit if necessary.
	     */
	    if (CVMtypeidFieldIsDoubleword(CVMfbNameAndTypeID(fb))) {
		fieldOffset += 2;
		nextMapBit += 2;
	    } else {
		/* For ref types, we set the bit in the gc map. */
		if (CVMtypeidFieldIsRef(CVMfbNameAndTypeID(fb))) {
		    /* 
		     * The expression "1<<nextMapBit" is a signed integer
		     * expression. According to the C language data type
		     * promotion rules an integer expression gets sign-
		     * extended when assigned to an unsigned long l-value.
		     * Of course we don't want this behaviour here because
		     * CVMobjectWalkRefsAux() relies on the map becoming 0
		     * when all bits are used up. The solution is to cast
		     * the constant "1" to CVMAddr and thus turning the
		     * whole expression into a CVMAddr expression.
		     */
		    map |= (CVMAddr)1<<nextMapBit;
		}
		fieldOffset += 1;
		nextMapBit += 1;
	    }

	    /* 
	     * If necessary, flush the current gc map into the big gcmap.
	     */
	    if (!hasShortMap) {
		/* Note that a 2-word field could cause nextMapBit == 33 */
		if (nextMapBit >= 32) {
		    nextMapBit %= 32;
		    gcMap.bigmap->map[currBigMapIndex] = map;
		    currBigMapIndex++;
		    map = gcMap.bigmap->map[currBigMapIndex];
		}
	    } else {
		CVMassert(nextMapBit + CVM_GCMAP_NUM_FLAGS <= 32);
	    }
	}
    }

    /* After this point, there can be no failures. */

    /* Set instance size of the class. */
    instanceSize = 
	numFieldWords * sizeof(CVMObject*) + sizeof(CVMObjectHeader) ;
#ifdef CVM_DEBUG
    /* This is debugging code to help track
     * down bug 4940678, should it ever occur again.
     */
    if (fieldOffset * sizeof(CVMObject*) != instanceSize) {
	CVMconsolePrintf("fieldOffset * sizeof(CVMObject*) != instanceSize\n");
	CVMconsolePrintf("fieldOffset:   0x%x\n", fieldOffset);
	CVMconsolePrintf("instanceSize:  0x%x\n", instanceSize);
	CVMconsolePrintf("numFieldWords: 0x%x\n", numFieldWords);
    }
#endif
    CVMassert(instanceSize <= 65535); /* Must fit in 2 words */
    CVMassert(fieldOffset * sizeof(CVMObject*) == instanceSize);
    CVMcbInstanceSize(cb) = (CVMUint16)instanceSize;

    /*
     * Store the calculated gc map into gcMap and set gc map flags.
     */
    if (hasShortMap) {
	gcMap.map =
	    (map << CVM_GCMAP_NUM_FLAGS) | CVM_GCMAP_SHORTGCMAP_FLAG;
    } else {
	gcMap.bigmap->map[currBigMapIndex] = map;
	gcMap.map |= CVM_GCMAP_LONGGCMAP_FLAG;
    }
    CVMcbGcMap(cb) = gcMap;

    CVMcbSetRuntimeFlag(cb, ee, FIELDS_PREPARED);
    return CVM_TRUE;
}


/*
 * Does this method do anything interesting?
 */
static CVMBool
CVMmethodNonTrivial(CVMMethodBlock* mb)
{
    if (CVMmbIsJava(mb)) {
	CVMJavaMethodDescriptor* jmd = CVMmbJmd(mb);
	/*
	 * Are there any other good criteria here?
	 */
	if (CVMjmdCodeLength(jmd) == 1 &&
	    CVMjmdCode(jmd)[0] == opc_return) {
	    return CVM_FALSE;
	}
    }
    return CVM_TRUE;
}


static CVMBool
CVMclassPrepareMethods(CVMExecEnv* ee, CVMClassBlock* cb)
{
    CVMMethodBlock* mb;
    CVMClassBlock*  superCb;
    CVMUint32       methodIdx;
    CVMUint32       nextMethodTableIdx;
    CVMBool         isFinalizable;
    CVMMethodBlock** tmpMethodTable = NULL;

    /*
     * Check if methods have already been prepared.
     */
    if (CVMcbMethodTableCount(cb) != 0) {
	return CVM_TRUE;
    }

    /* 
     * Get some info from the superclass.
     */
    CVMassert(CVMcbSuperclass(cb) != NULL);
    superCb = CVMcbSuperclass(cb);
    if (CVMcbIs(superCb, FINAL)) {
	CVMthrowVerifyError(ee, "Class %C is subclass of final class %C",
			    cb, superCb);
	return CVM_FALSE;
    }

    /*
     * This might change soon, if we try to override the non-trivial
     * finalizer of the superclass with a trivial one. Otherwise
     * it stands.
     */
    isFinalizable = CVMcbIs(superCb, FINALIZABLE);

    /*
     * Scan the class' method array, looking for methods that will end
     * up in the method table. Calculate method table indices in the process
     * and figure out how big the method table needs to be.
     *
     * Also, for each method, generate a stackmap and set its invoker index.
     */
    if (CVMcbIs(cb, INTERFACE)) { 
        /* Interfaces don't have method tables. */ 
	nextMethodTableIdx = 0;
    } else {
        /* Allocate a methodtable that is large enough. */
	nextMethodTableIdx = CVMcbMethodTableCount(superCb);
        tmpMethodTable = (CVMMethodBlock**)
	    calloc(nextMethodTableIdx + CVMcbMethodCount(cb),
		   sizeof(CVMMethodBlock*));
        if (tmpMethodTable == NULL) {
            CVMthrowOutOfMemoryError(ee, NULL);
            return CVM_FALSE;
        }
        /* Copy the superclass' methodtable. */
        memcpy(tmpMethodTable, CVMcbMethodTablePtr(superCb),
                   nextMethodTableIdx * sizeof(CVMMethodBlock*));
    }
    for (methodIdx = 0; methodIdx < CVMcbMethodCount(cb); methodIdx++) {
	CVMBool isOverride;
	int superMethodIdx;

	mb = CVMcbMethodSlot(cb, methodIdx);
#ifdef CVM_JIT
	CVMassert(CVMmbJitInvoker(mb) == (void*)CVMCCMletInterpreterDoInvoke);
#endif
        /* Set the CVMmbInvokerIdx() for the method. */
	if (CVMmbIs(mb, ABSTRACT)) {
	    CVMmbSetInvokerIdx(mb, CVM_INVOKE_ABSTRACT_METHOD);
	} else if (CVMmbIs(mb, NATIVE)) {
	    CVMmbSetInvokerIdx(mb, CVM_INVOKE_LAZY_JNI_METHOD);
	} else if (CVMmbIs(mb, SYNCHRONIZED)) {
	    CVMmbSetInvokerIdx(mb, CVM_INVOKE_JAVA_SYNC_METHOD);
	} else {
	    CVMmbSetInvokerIdx(mb, CVM_INVOKE_JAVA_METHOD);
	}

	/*
	 * If the class is an interface then the method better be
	 * abstract or a static intializer.
	 */
	CVMassert(!CVMcbIs(cb, INTERFACE) || CVMmbIs(mb, ABSTRACT) ||
		  CVMtypeidIsStaticInitializer(CVMmbNameAndTypeID(mb)));

	/* 
	 * If it's a java method then fixup maxLocals and disambiguate it.
	 */
	if (CVMmbIsJava(mb)) {
	    /*
	     * Make sure maxLocals is big enough to hold a 2 word result so 
	     * there is room on the stack without corrupting the current frame.
	     */
	    CVMJavaMethodDescriptor* jmd = CVMmbJmd(mb);
	    if (CVMjmdMaxLocals(jmd) < 2) {
		CVMjmdCapacity(jmd) += (2 - CVMjmdMaxLocals(jmd));
		CVMjmdMaxLocals(jmd) = 2;
	    }
	    if (!CVMcbCheckRuntimeFlag(cb, VERIFIED) ||
		(CVMjmdFlags(jmd) & CVM_JMD_MAY_NEED_REWRITE))
	    {
		/* 
		 * Either this method has never been inspected by a
		 * verifier, or the verifier found a jsr. In either
		 * case, there may be ambiguities that require
		 * bytecode rewriting to avoid. 
		 */
		switch (CVMstackmapDisambiguate(ee, mb,
			(CVMjmdFlags(jmd) & CVM_JMD_MAY_NEED_REWRITE))) {
		    case CVM_MAP_SUCCESS:
			break;
		    case CVM_MAP_OUT_OF_MEMORY:
			CVMthrowOutOfMemoryError(ee, NULL);
			goto fail;
		    case CVM_MAP_EXCEEDED_LIMITS:
			CVMthrowInternalError(
			    ee, "Exceeded limits while disambiguating %C.%M",
			    cb, mb);
			goto fail;
		    case CVM_MAP_CANNOT_MAP:
			CVMthrowInternalError(
			    ee, "Cannot disambiguate %C.%M",
			    cb, mb);
			goto fail;
		    default:
			CVMassert(CVM_FALSE);
			goto fail;
		}
	    }

	    /*
	     * reclaim clinit memory that would otherwise be lost if
	     * rewriting was done here!  */
	    if (CVMjmdFlags(CVMmbJmd(mb)) & CVM_JMD_DID_REWRITE) {
		if (CVMtypeidIsStaticInitializer(CVMmbNameAndTypeID(mb))) {
		    free(jmd);
		}
	    }

#ifdef CVM_HW
	    CVMhwPrepareMethod(mb);
	    CVMhwFlushCache(CVMmbJavaCode(mb),
			    CVMmbJavaCode(mb) + CVMmbCodeLength(mb));
#endif
	}

	/*
	 * Now check if we declare a non-trivial finalize() method or not.
	 */
	if (CVMtypeidIsFinalizer(CVMmbNameAndTypeID(mb))) {
	    isFinalizable = CVMmethodNonTrivial(mb);
	}

	if (CVMcbIs(cb, INTERFACE)) { 
	    continue; /* interfaces don't have method tables */
	}
	
	/* These types of methods never go in the method table. */
	if (CVMmbIs(mb, STATIC) || CVMmbIs(mb, PRIVATE)) {
	    continue;
	}
	if (CVMtypeidIsConstructor(CVMmbNameAndTypeID(mb))) {
	    continue;
	}

	/*
	 * Look for a virtual method in the superclass' methodtable
	 * that the current method can override.
	 */
        CVMassert(tmpMethodTable != NULL);

	isOverride = CVM_FALSE; /* Does this method override a superclass
				   method? */
	for (superMethodIdx = 0; 
	     superMethodIdx < CVMcbMethodTableCount(superCb);
	     superMethodIdx++) 
	{
	    CVMMethodBlock* smb = tmpMethodTable[superMethodIdx];
	    if (smb == NULL) {
		continue;
	    }

#ifdef CVM_DUAL_STACK
            {
	        /*
                 * Check against dual-stack filter to see if
                 * the super MB exists. If the super MB doesn't
                 * exist, then it's not an override and just
                 * continue.
                 */
	        if (!CVMdualStackFindSuperMB(
                            ee, cb, superCb, smb)) {
		    continue;
	        }
	    }
#endif

            /* Private methods never go in the method table. */
            CVMassert(!CVMmbIs(smb, PRIVATE));

	    /*
	     * See if the superclass method typeID matches this method typeID.
	     * Note if the superclass method is a miranda method then we need
	     * to look at its CVMmbMirandaInterfaceMb() so we don't get the
	     * typeID that is prefixed with a '+'. JDK doesn't do this.
	     */
	    if (CVMmbIsMiranda(smb)) {
		if (CVMmbNameAndTypeID(mb) != 
		    CVMmbNameAndTypeID(CVMmbMirandaInterfaceMb(smb))) {
		    continue;
		}
	    } else {
		if (CVMmbNameAndTypeID(mb) != CVMmbNameAndTypeID(smb)) {
		    continue;
		}
	    }

	    /* 
	     * If the method is Package-private (no access flags set),
	     * then this class must be in the same package as the
	     * superclass in order to inherit the method.
	     */
	    if (!CVMmbIs(smb, PUBLIC) && !CVMmbIs(smb, PROTECTED) &&
		!CVMisSameClassPackage(ee, CVMmbClassBlock(smb), cb))
	    {
		continue;
	    }

	    if (CVMmbIs(smb, FINAL))
	    {
		CVMthrowVerifyError(ee, "Class %C overrides final method %M",
				    cb,  mb);
		goto fail;
	    }

	    /* 
	     * Verify that the signature doesn't break existing loader
	     * constraints.
	     */
	    if (!CVMloaderConstraintsCheckMethodSignatureLoaders(
		     ee, CVMmbNameAndTypeID(smb),
	    	     CVMcbClassLoader(cb),
		     CVMcbClassLoader(CVMmbClassBlock(smb)))) {
		goto fail; /* exception already thrown */
	    }
	    /*
	     * The method is a match and meets all requirements for
	     * overriding, so override it.
	     */
	    CVMmbMethodTableIndex(mb) = CVMmbMethodTableIndex(smb);
	    isOverride = CVM_TRUE;
	    tmpMethodTable[superMethodIdx] = mb;

#ifdef CVM_JIT_PATCHED_METHOD_INVOCATIONS
	    /* Grab the jitLock before patching. */
	    CVMsysMutexLock(ee, &CVMglobals.jitLock);

	    /* This must be done after grabbing the jitLock because
	       there is a special check in CVMJITPMIpatchCall() that
	       will prevent the patch from being made if the
	       OVERRIDDEN flag is set when patching for transitions
	       between compiled and decompiled. If we did this outside
	       of the jitLock and then another thread ran before the
	       patch below was applied, and that thread attempts to
	       patch from compiled to decompiled, then the patch would
	       not be applied and there would temporarily be direct
	       method calls to a decompiled method, meaning it would
	       branch to an invalid area of the code cache..
	    */
	    CVMmbCompileFlags(smb) |= CVMJIT_IS_OVERRIDDEN;

	    /* Since this method was not previously overridden, there may
	       be direct method calls to them. Patch them so they instead
	       do a true virtual invoke. */
	    CVMJITPMIpatchCallsToMethod(smb, CVMJITPMI_PATCHSTATE_OVERRIDDEN);

	    /* Release the jitLock after patching. */
	    CVMsysMutexUnlock(ee, &CVMglobals.jitLock);
#endif
	} 

	if (!isOverride) { 
	    /*
	     * It looks this method isn't an override so allocate a new
	     * method table slot for it.
	     */
	    CVMmbMethodTableIndex(mb) = nextMethodTableIdx;
	    tmpMethodTable[nextMethodTableIdx++] = mb;
	}
    }

    /* By the time we get here, we have already built a methodtable
     * for non-interface classes. And remember, interfaces don't have
     * methodtables.
     */

    if (nextMethodTableIdx > CVM_LIMIT_NUM_METHODTABLE_ENTRIES) {
#ifdef JAVASE
        CVMthrowOutOfMemoryError(ee, 
	    "Class %C exceeds 64K method table size limit", cb);
#else
        CVMthrowInternalError(ee, 
	    "Class %C exceeds 64K method table size limit", cb);
#endif
	goto fail;
    }

    if (CVMcbIs(cb, INTERFACE)) { 
	CVMassert(nextMethodTableIdx == 0);
        CVMassert(tmpMethodTable == NULL);
    } else {
        CVMassert(tmpMethodTable != NULL);
	/* Allocate the methodtable with the actual length. */
	/* %comment c */
	CVMcbMethodTablePtr(cb) = (CVMMethodBlock**)
	    calloc(nextMethodTableIdx, sizeof(CVMMethodBlock*));
	if (CVMcbMethodTablePtr(cb) == NULL) {
	    CVMthrowOutOfMemoryError(ee, NULL);
	    goto fail;
	}
	memcpy(CVMcbMethodTablePtr(cb), tmpMethodTable,
	           nextMethodTableIdx * sizeof(CVMMethodBlock*));	
    }

    CVMcbMethodTableCount(cb) = (CVMUint16)nextMethodTableIdx;
    if (isFinalizable) {
	CVMcbSetAccessFlag(cb, FINALIZABLE);
    }

    if (tmpMethodTable != NULL) {
        free(tmpMethodTable);
    }
    return CVM_TRUE;

fail:
    if (tmpMethodTable != NULL) {
        free(tmpMethodTable);
    }
    return CVM_FALSE;
}

#undef  ILLEGAL_ACCESS
#define ILLEGAL_ACCESS 0xFFFF

static CVMBool
CVMclassPrepareInterfaces(CVMExecEnv* ee, CVMClassBlock* cb)
{
    CVMBool          isInterface = CVMcbIs(cb, INTERFACE);
    CVMClassBlock*   superCb = CVMcbSuperclass(cb);
    CVMClassBlock*   icb;
    CVMInterfaces*   oldCbInterfaces = CVMcbInterfaces(cb);
    CVMUint16*       methodTableIndices;
    CVMUint16        superInterfaceCount = CVMcbInterfaceCount(superCb);
    CVMUint16        implementsCount = CVMcbImplementsCount(cb);
    int              n_miranda_methods = 0;
    CVMUint16 i, j, icount, mcount;
    CVMInt32 k;
    CVMMethodRange*  miranda_method_range;

    /*
     * There is no need for a CVM_CLASS_INTERFACES_PREPARED flag here because
     * CVM_CLASS_LINKED implies that the interfaces have been prepared
     * and vice-versa.
     */

    /* 
     * Classes that don't explicity declare any interfaces can just 
     * inherit their parent's CVMInterfaces. Note that interfaces that
     * don't inherit any interfaces will have an implementsCount
     * of 0. We still need to allocate a CVMInterfaces structure for them
     * that contains an entry for the interface itself, so we don't inherit
     * the parent's (NULL) CVMInterfaces in this case.
     */
    if (implementsCount == 0 && !isInterface) {
	/*
	 * This flag indicates that the CVMcbInterfaces table in inherited
	 * from the parent cb. This will cause the CVMcbInterfaces() to not
	 * be freed when class is unloaded.
	 */
	CVMcbSetRuntimeFlag(cb, ee, INHERITED_INTERFACES);

	CVMassert(CVMcbInterfaces(cb) == NULL);
	CVMcbInterfaces(cb) = CVMcbInterfaces(superCb);
	return CVM_TRUE;
    }

    /*
     * This assert is just another way of saying that all interfaces
     * inherit from Object, which has no interfaces.
     */
    CVMassert(!isInterface || superInterfaceCount == 0);

    /* 
     * icount: number of interfaces that this class implements, including
     *         interfaces implemented by our superclass, and by interfaces 
     *         implemented by interfaces that we implement.
     * mcount: number of interface methods in interfaces this class 
     *         declares it implements.
     */
    /* %comment c */
    icount = superInterfaceCount + (isInterface ? 1 : 0);
    mcount = 0;
    for (i = 0; i < implementsCount; i++) {
	icb = CVMcbInterfacecb(cb, i);
	icount += CVMcbInterfaceCount(icb);
	
	if (!isInterface) {
	    /*
	     * Since we implement this interfaces, we need to add the number
	     * of methods this interface implements, which is the number
	     * of methods this interface declares plus the number of
	     * methods declared in each interface this interface inherits
	     * from. Note that each interface has itself in it's
	     * CVMInterfaces array.
	     */
	    for (j = 0; j < CVMcbInterfaceCount(icb); j++ ) {
		mcount += CVMcbMethodCount(CVMcbInterfacecb(icb, j));
	    }
	}
    }

    /*
     * Allocate the new CVMcbInterfaces() table. We need room for an
     * CVMInterfaces struct with icount itable entries, plus room for
     * the methodTableIndices array.
     */
    CVMcbInterfaces(cb) = (CVMInterfaces *)
	calloc(1, CVMoffsetof(CVMInterfaces, itable) +
		   sizeof(CVMInterfaceTable) * icount +
		   sizeof(CVMUint16) * mcount);
    if (CVMcbInterfaces(cb) == NULL) {
	/* 
	 * Restore the original CVMInterfaces pointer so it will get freed
	 * up properly when the class is gc'd.
	 */
	CVMcbInterfaces(cb) = oldCbInterfaces;
	CVMthrowOutOfMemoryError(ee, NULL);
	return CVM_FALSE;
    }
    CVMcbSetImplementsCount(cb, implementsCount);
    
    /************************************
     * Fill in the new CVMInterfaceTable.
     ************************************/
    
    /* 
     * Add the interfaces that we declare we implement. These must always
     * be first in the CVMInterfaces array.
     */
    icount = 0;
    for (j = 0; j < implementsCount; j++) {
	icb = oldCbInterfaces->itable[j].interfaceCb;
	CVMcbInterfacecb(cb, icount) = icb;
	icount++;
    }
    
    /* If this is an interface class, then put itself in the next slot. */
    if (isInterface) {
	CVMcbInterfacecb(cb, icount) = cb;
	icount++;
    }

    /* 
     * Add the interfaces that are implemented by the interfaces we declare
     * we implement.
     */
    for (j = 0; j < implementsCount; j++) {
	icb = oldCbInterfaces->itable[j].interfaceCb;
	for (i = 0; i < CVMcbInterfaceCount(icb); i++) {
	    /* "icb" and was already added above, so skip it. */
	    if (CVMcbInterfacecb(icb, i) == icb) {
		continue;
	    }
	    CVMcbInterfacecb(cb, icount) = CVMcbInterfacecb(icb, i);
	    icount++;
	}
    }
    
    /* 
     * We can copy our superclass' CVMInterfaceTable. 
     * The methodTableIndices will stay the same.  
     */
    for (i = 0; i < superInterfaceCount; i++) {
	CVMcbInterfacecb(cb, icount) = CVMcbInterfacecb(superCb, i);
	CVMcbInterfaceMethodTableIndices(cb, icount) =
	    CVMcbInterfaceMethodTableIndices(superCb, i);
	icount++;
    }

    /*
     * We no longer need the old CVMInterfaces struct that was created
     * when the class was loaded. It will be NULL for interfaces that
     * don't declare that they inherit any other interfaces.
     */
    if (oldCbInterfaces != NULL) {
	free(oldCbInterfaces);
    }

    /*
     * Delete duplicates from the table.  This is very rare, so it can
     * be quite inefficient. We always delete from the end of the table
     * because this is where inherited interfaces are.
     */
    for (i = 0; i < icount - (isInterface ? 1 : superInterfaceCount); i++) {
	icb = CVMcbInterfacecb(cb, i);
	for (j = i+1; j < icount; j++) { 
	    if (icb == CVMcbInterfacecb(cb, j)) { 
		/* We have an overlap.  Item j is a duplicate.  Delete from
		 * the table */
		for (k = j + 1; k < icount; k++) { 
		    CVMcbInterfacecb(cb, k - 1) = CVMcbInterfacecb(cb, k);
		    CVMcbInterfaceMethodTableIndices(cb, k - 1) =
			CVMcbInterfaceMethodTableIndices(cb, k);
		}

		/*
		 * We have to fill up the freed space with 0, because
		 * we will put the method table indices directly after
		 * the last surviving CVMInterfaceTable and expect
		 * that region to be filled with 0 (normally done by the
		 * calloc), because a method table index of 0 means that
		 * no implementation of that method was found.
		 */
		memset(&(CVMcbInterfaces(cb)->itable[icount - 1]), 0,
		       sizeof(CVMInterfaceTable));
		
		/* 
		 * If the interface we deleted came from the superclass,
		 * then we also need to decrement superInterfaceCount because
		 * we now inherit one less interface from the superclass.
		 */
		if (!isInterface && (j >= icount - superInterfaceCount)) {
		    superInterfaceCount--;
		}
		icount--; /* we now implement one less interface */
		i--;   /* Reconsider entry in case duplicated more than once */
		break;
	    }
	}
    }

    /* We finally know how many interfaces this class implements. */
    CVMcbSetInterfaceCount(cb, icount);

    /* Nothing more to do for interfaces */
    if (isInterface) { 
	return CVM_TRUE;
    }

    /*
     * Setup the methodTableIndices array for each implemented interface.
     * Inherited interfaces are already setup. There's space allocated
     * for the methodTableIndices arrays right after the last
     * CVMInterfaceTable.
     */
    methodTableIndices = (CVMUint16*)&CVMcbInterfaces(cb)->itable[icount];
    for (i = 0; i < icount - superInterfaceCount; i++) { 
	int numIntfMethods;

	icb = CVMcbInterfacecb(cb, i);
	numIntfMethods = CVMcbMethodCount(icb);
	CVMcbInterfaceMethodTableIndices(cb, i) = methodTableIndices;
	
	/* Look at each interface method */
	for (j = 0; j < numIntfMethods; j++) { 
	    CVMMethodBlock* imb = CVMcbMethodSlot(icb, j);

	    /*
	     * Verify that the signature does not break any existing
	     * loader constraints.
	     */
	    if (!CVMloaderConstraintsCheckMethodSignatureLoaders(
		    ee, CVMmbNameAndTypeID(imb),
		    CVMcbClassLoader(cb), 
		    CVMcbClassLoader(CVMmbClassBlock(imb)))) {
		return CVM_FALSE; /* exception already thrown */
	    }

	    /* If the method is static then it must be <clinit>. */
	    if (CVMmbIs(imb, STATIC)) { 
		CVMassert(
                    CVMtypeidIsStaticInitializer(CVMmbNameAndTypeID(imb)));
		continue;
	    } 

	    /* Find the virtual method that implements the interface method. */
	    for (k = CVMcbMethodTableCount(cb) - 1; k >= 0; k--) { 
		CVMMethodBlock* mb = CVMcbMethodTableSlot(cb, k);
		if (mb != NULL && CVMtypeidIsSame(CVMmbNameAndTypeID(mb),
						  CVMmbNameAndTypeID(imb))) {
		    if (CVMmbIs(mb, PUBLIC)) {
			methodTableIndices[j] = CVMmbMethodTableIndex(mb);
			break;
		    } else {
			methodTableIndices[j] = ILLEGAL_ACCESS;
			n_miranda_methods++;
			break;
		    }
		}
	    }

	    /* 
	     * If we didn't find a virtual method that implements the
	     * interface method, then make sure there is no private 
	     * method trying to implement the interface method.
	     */
	    if (methodTableIndices[j] == 0) {
		for (k = 0; k < CVMcbMethodCount(cb); k++) {
		    CVMMethodBlock* mb = CVMcbMethodSlot(cb, k);
		    if (!CVMmbIs(mb, STATIC)) {
			if (CVMtypeidIsSame(CVMmbNameAndTypeID(mb),
					    CVMmbNameAndTypeID(imb))) {
			    methodTableIndices[j] = ILLEGAL_ACCESS;
			    break;
			}
		    }
		}
		n_miranda_methods++;
	    }
	}

	/* 
	 * Setup pointer to next table. The length of the current table
	 * is the number of interface methods.
	 */
	methodTableIndices += numIntfMethods;
    }
    
    if (n_miranda_methods == 0) {
	return CVM_TRUE;
    }

    /* 
     * MIRANDA METHODS - you've come to the right place for all the details
     * on miranda methods. Read on...
     *
     * Miranda methods are created whenever a class does not provide a proper
     * implementation of an interface method. This could mean it doesn't
     * implement the method at all, or that it implements it with a non-public
     * method. Normally javac will catch such an error, but if the interface
     * is modified and recompiled without also recompiling the class that
     * implements the interface, then you can end up methods in the 
     * interface that either don't exists in the class or are private
     * in the class. The JLS does not allow you to reject such classes.
     * Instead, an exception must be thrown if an attempt to call one
     * of the missing methods is made.
     *
     * One reason the miranda method is needed is because there must
     * be a methodtable slot set aside for each interface method, whether or
     * not the class provides an implementation. It's also necessary for the
     * method table slot to point to a valid method block for a number of
     * reasons. Filling in subclass method tables, properly throwing
     * an exception when dispatching through the slot, and reflection are
     * just a few.
     *
     * CASE 1 - Interface method is missing. Usually javac is kind enough
     * to provide a miranda method for you if your class does not implement
     * an interface method. Of course you can only do this with an abstract
     * class. If the java compiler didn't create a miranda method then
     * the vm needs to. Also, if the interface added a method and the
     * class was not modified and recompiled, then the vm nees to create
     * a miranda method in this case too. In fact, this can happen even
     * if the class is not abstract. Thus, it's possible to call an interface
     * method and get an AbstractMethodError because the class did not 
     * implement the method.
     *
     * CASE 2 - Class implements interface method with a non-public method.
     * javac won't allow you to do this. However, this can happen if the
     * interface adds a new method and just by chance the class already
     * contains a non-public method with the same name. If the class is
     * not recompiled (which would catch the error), then the vm needs to
     * do something about it. The vm can't allow the non-public method to
     * actually implement the interface method. Instead, a miranda method 
     * is created with the same name as interface method, except it is
     * prefixed with a '+' to avoid a name conflict. This miranda methods
     * ends up in the method table and when called, results in an
     * IncompatibleClassChangeError.
     *
     * IMPLEMENTATION DETAILS
     *
     * The miranda methods are allocated in a CVMMethodArray. For this
     * reason we don't allow more than 256 miranda methods. There are two
     * advantages to using the CVMMethodArray. The first is that it allows
     * CVMmbClassBlock() to work. The 2nd is that you can easily find the
     * memory allocated for the miranda methods by using CVMmbMethodRange().
     * For this reason we don't need to store a pointer to the miranda methods
     * in the cb.
     *
     * Pointers to the miranda methods are always stored at the end of the
     * method table. When a class is freed, we check the last method table 
     * slot to see if it is a miranda method. If it is and its cb is the
     * same as the cb we are freeing, then we simply call
     * free(CVMmbMethodRange()).
     *
     * Each miranda method contains a pointer to the interface method that
     * it was created for. This is stored in CVMmbMirandaInterfaceMb().
     * There are two cases where we need acces to the interface method.
     * In reflection code, we want to obtain the interface method block
     * if the miranda method was created to deal with a missing interface
     * method. In executejava.c, when we invoke a miranda method that
     * was created to deal with a non-public interace method, we want to
     * use the name of the inteface method in the exception, not the miranda
     * method name which is prefixed with a '+'. Thus we need the interface
     * method in both CASE 1 and CASE 2.
     */

    if (n_miranda_methods > 256) {
	CVMthrowInternalError(ee, "Class %C requires more than 256 "
			      "miranda methods.", cb);
	return CVM_FALSE;
    }

    mcount = CVMcbMethodTableCount(cb);

    /* Make sure there is room for the miranda methods. */
    if (mcount + n_miranda_methods > CVM_LIMIT_NUM_METHODTABLE_ENTRIES) {
#ifdef JAVASE
        CVMthrowOutOfMemoryError(ee, 
            "Class %C exceeds 64K method table size limit", cb);
#else
        CVMthrowInternalError(ee, 
            "Class %C exceeds 64K method table size limit", cb);
#endif
	return CVM_FALSE;
    }

    /*
     * Allocate memory for the miranda methods.
     */
    miranda_method_range = (CVMMethodRange *)
	calloc(CVMoffsetof(CVMMethodRange, mb) + 
		   n_miranda_methods * sizeof(CVMMethodBlock),
		   1);
    if (miranda_method_range == NULL) {
	CVMthrowOutOfMemoryError(ee, NULL);
	return CVM_FALSE;
    }
    miranda_method_range->cb = cb;
    
    /*
     * Expand the method table.
     */ 
    {
	/* Allocate new method table big enough for the miranda methods. */
	CVMMethodBlock** newMethodTablePtr = (CVMMethodBlock**)
	    malloc((CVMcbMethodTableCount(cb) + n_miranda_methods)
		   * sizeof(CVMMethodBlock*));
	if (newMethodTablePtr == NULL) {
	    free(miranda_method_range);
	    CVMthrowOutOfMemoryError(ee, NULL);
	    return CVM_FALSE;
	}

	/* Copy contents of the old methodtable into the new one. */
	memcpy(newMethodTablePtr, CVMcbMethodTablePtr(cb),
	       CVMcbMethodTableCount(cb) * sizeof(CVMMethodBlock*));

	/* Free the old one and swap in the new one. */
	free(CVMcbMethodTablePtr(cb));
	CVMcbMethodTablePtr(cb) = newMethodTablePtr;
    }
    
    /*
     * Look at every CVMInterfaceTable entry, looking for slots determined
     * above to need miranda methods. We don't have to bother looking at
     * the slots at the end of the table that we got from the superclass.
     */
    for (i = 0; i < icount - superInterfaceCount; i++) { 
	/* The table length is the number of interface methods */
	CVMClassBlock* icb = CVMcbInterfacecb(cb, i);
	
	/* Look at each interface method */
	for (j = 0; j < CVMcbMethodCount(icb); j++) { 
	    CVMMethodBlock* imb = CVMcbMethodSlot(icb, j);
	    if (!CVMmbIs(imb, STATIC)) {
		CVMUint16 methodIndex = 
		    CVMcbInterfaceMethodTableIndex(cb, i, j);
		if (methodIndex == 0 || methodIndex == ILLEGAL_ACCESS) {
		    CVMMethodBlock* mb;
		    n_miranda_methods--;
		    mb = &miranda_method_range->mb[n_miranda_methods];
		    CVMmbClassBlock(mb) = cb;
		    CVMmbMethodTableIndex(mb) = mcount;
		    CVMmbArgsSize(mb)         = CVMmbArgsSize(imb);
		    CVMmbSetAccessFlags(mb, CVMmbAccessFlags(imb));
		    CVMmbMethodIndex(mb)      = n_miranda_methods;
		    CVMmbClassBlock(mb)       = cb;
		    CVMcbInterfaceMethodTableIndex(cb, i, j) = mcount;
		    CVMcbMethodTableSlot(cb, mcount) = mb;
#ifdef CVM_JIT
		    CVMmbJitInvoker(mb) = (void*)CVMCCMletInterpreterDoInvoke;
#endif
		    mcount++;
		    if (methodIndex != ILLEGAL_ACCESS) {
			CVMmbNameAndTypeID(mb) = CVMtypeidCloneMethodID(
                            ee, CVMmbNameAndTypeID(imb));
			CVMmbSetInvokerIdx(mb, 
			    CVM_INVOKE_MISSINGINTERFACE_MIRANDA_METHOD);
		    } else {
			/* 
			 * create a *fake* name that begins with '+', not
			 * to be in conflict with other methods.
			 */
			CVMBool success;
			char* sig = CVMtypeidMethodTypeToAllocatedCString(
                            CVMmbNameAndTypeID(imb));
			char* iname = CVMtypeidMethodNameToAllocatedCString(
                            CVMmbNameAndTypeID(imb));
			char* mname = NULL;
			if (iname != NULL) {
			    mname = (char *)malloc(strlen(iname) + 2);
			}
			if (sig == NULL || iname == NULL || mname == NULL) {
			    success = CVM_FALSE;
			    CVMthrowOutOfMemoryError(ee, NULL);
			    goto doneWithName;
			}
			strcpy(&mname[1], iname);
			mname[0] = '+';
			CVMmbNameAndTypeID(mb) =
			    CVMtypeidNewMethodIDFromNameAndSig(ee, mname, sig);
			if (CVMmbNameAndTypeID(mb) == CVM_TYPEID_ERROR) {
			    success = CVM_FALSE; /* exception already thrown */
			} else {
			    success = CVM_TRUE;
			}

		    doneWithName:
			if (sig != NULL)   free(sig);
			if (iname != NULL) free(iname);
			if (mname != NULL) free(mname);
			
			if (!success) {
			    int i;
			    /*
			     * Free all the MethodTypeID's that were 
			     * allocated for miranda methods.
			     */
			    for (i = CVMcbMethodTableCount(cb); 
				 i < mcount - 1;
				 i++) {
				CVMMethodBlock* mb = 
				    CVMcbMethodTableSlot(cb, i);
				CVMtypeidDisposeMethodID(
				    ee, CVMmbNameAndTypeID(mb));
			    }
			    /* free memory allocated for miranda methods */
			    free(miranda_method_range);
			    return CVM_FALSE;
			}
			CVMmbSetInvokerIdx(mb,
			    CVM_INVOKE_NONPUBLIC_MIRANDA_METHOD);
		    }
		    /* 
		     * WARNING: this must be done after setting up 
		     * CVMmbInvokerIdx() or you'll get an assertion.
		     */
		    CVMmbMirandaInterfaceMb(mb) = imb;
		}
	    }
	}
    }

    /*
     * Setup the new methodTablecount. mcount has been incremented
     * by the number of miranda methods.
     */
    CVMcbMethodTableCount(cb) = mcount;
    /* n_miranda_methods was decremetned each time one was setup */
    CVMassert(n_miranda_methods == 0);

    return CVM_TRUE;
}

/*
 * Initialize a static field with a constant value.
 */
static void 
CVMinitializeStaticField(CVMExecEnv* ee, CVMFieldBlock* fb,
			 CVMConstantPool* cp, CVMUint16 cpIdx)
{    
    switch (CVMtypeidGetType(CVMfbNameAndTypeID(fb))) { 
    case CVM_TYPEID_DOUBLE: 
	CVMassert(CVMcpTypeIs(cp, cpIdx, Double));
	goto get64bitConstant;
	
    case CVM_TYPEID_LONG: 
	CVMassert(CVMcpTypeIs(cp, cpIdx, Long));
	goto get64bitConstant;
	
    get64bitConstant: {
	    CVMJavaVal64 val64;
	    CVMcpGetVal64(cp, cpIdx, val64);
	    CVMmemCopy64(&CVMfbStaticField(ee, fb).raw, val64.v);
	    break;
	}
	
    case CVM_TYPEID_BYTE:
    case CVM_TYPEID_CHAR:
    case CVM_TYPEID_SHORT:
    case CVM_TYPEID_BOOLEAN:
    case CVM_TYPEID_INT:  
	CVMassert(CVMcpTypeIs(cp, cpIdx, Integer));
	goto get32bitConstant;
    case CVM_TYPEID_FLOAT:
	CVMassert(CVMcpTypeIs(cp, cpIdx, Float));
	goto get32bitConstant;
	
    get32bitConstant:
	CVMfbStaticField(ee, fb) = CVMcpGetVal32(cp, cpIdx);
	break;
	
    default: 
	/* Must be a java/lang/String. */
	CVMassert(!CVMtypeidIsPrimitive(CVMfbNameAndTypeID(fb)));
	CVMassert(CVMcpTypeIs(cp, cpIdx, StringICell));
	CVMD_gcUnsafeExec(ee, {
	    CVMObject* stringObj = 
		CVMID_icellDirect(ee, CVMcpGetStringICell(cp, cpIdx));
	    CVMID_icellSetDirect(ee, &CVMfbStaticField(ee, fb).r, stringObj);
	});
	break;
    }
}

#endif /* CVM_CLASSLOADING */
