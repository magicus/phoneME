/*
 * @(#)classes.c	1.100 06/10/25
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

#include "javavm/include/typeid.h"
#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/classes.h"
#include "javavm/include/typeid.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/preloader.h"
#include "javavm/include/globalroots.h"
#include "javavm/include/globals.h"
#include "javavm/include/utils.h"
#include "javavm/include/clib.h"
 
/*
 * Get an array class with elements of type elemCb, but only if it
 * is already created.
 */
CVMClassBlock*
CVMclassGetArrayOfWithNoClassCreation(CVMExecEnv* ee, CVMClassBlock* elemCb)
{
    CVMassert(CVMD_isgcSafe(ee));

    if (CVMcbIs(elemCb, PRIMITIVE)) {
	return (CVMClassBlock*)
	    CVMbasicTypeArrayClassblocks[CVMcbBasicTypeCode(elemCb)];
    }
 
    /*
     * Look it up in the loaderCache.
     */
    {
	CVMClassTypeID arrayID = 
	    CVMtypeidIncrementArrayDepth(ee, CVMcbClassName(elemCb), 1);
	CVMClassLoaderICell* loader = CVMcbClassLoader(elemCb);
	CVMClassBlock* arrayCb = NULL;

	if (arrayCb == NULL) {
	    CVM_LOADERCACHE_LOCK(ee);
	    arrayCb = CVMloaderCacheLookup(ee, arrayID, loader);
	    CVM_LOADERCACHE_UNLOCK(ee);
	}

	CVMtypeidDisposeClassID(ee, arrayID);
	return arrayCb;
    }
}

/*
 * Get an array class with elements of type elemCb.
 */
CVMClassBlock*
CVMclassGetArrayOf(CVMExecEnv* ee, CVMClassBlock* elemCb)
{
    CVMassert(CVMD_isgcSafe(ee));

    if (CVMcbIs(elemCb, PRIMITIVE)) {
	return (CVMClassBlock*)
	    CVMbasicTypeArrayClassblocks[CVMcbBasicTypeCode(elemCb)];
    }

    /*
     * Look it up.
     */
    {
	CVMClassTypeID arrayID = 
	    CVMtypeidIncrementArrayDepth(ee, CVMcbClassName(elemCb), 1);
	CVMClassBlock * arrayClass =
	    CVMclassLookupByTypeFromClass(ee, arrayID, CVM_FALSE, elemCb);
	 CVMtypeidDisposeClassID( ee, arrayID );
	 return arrayClass;
    }
}

static CVMMethodBlock*
CVMclassprivateReturnDeclaredMethodInClassMethods(const CVMClassBlock* cb,
						  CVMMethodTypeID tid)
{
    int i;
    for (i = 0; i < CVMcbMethodCount(cb); i++) {
        CVMMethodBlock* mb = CVMcbMethodSlot(cb, i);
	if (CVMtypeidIsSame(CVMmbNameAndTypeID(mb), tid)) {
            return mb;
	}
    }
    return NULL;
}

static CVMMethodBlock*
CVMclassprivateReturnDeclaredMethodInClassMethodTable(
    const CVMClassBlock* cb,
    const CVMMethodTypeID tid)
{
    int i;
    for (i = 0; i < CVMcbMethodTableCount(cb); i++) {
        CVMMethodBlock* mb = CVMcbMethodTableSlot(cb, i);
	if (CVMtypeidIsSame(CVMmbNameAndTypeID(mb), tid)) {
            return mb;
	}
    }
    return NULL;
}

/*
 * Look for public or declared method of type 'tid'
 */
CVMMethodBlock*
CVMclassGetMethodBlock(const CVMClassBlock* cb,
		       const CVMMethodTypeID tid, CVMBool isStatic)
{
    CVMMethodBlock* mb;

    if (!isStatic) {
	/* Look in methodTable of the given class */
	mb = CVMclassprivateReturnDeclaredMethodInClassMethodTable(cb, tid);
	if (mb != NULL) {
	    goto found;
	}

	/* Look in declared methods of the given class */
	mb = CVMclassprivateReturnDeclaredMethodInClassMethods(cb, tid);
	if (mb != NULL) {
	    goto found;
	}

	/*
	 * Now look in all implemented interfaces for the method.
	 */
	{
	    int i;
	    for (i = 0; i < CVMcbInterfaceCount(cb); i++) {
		mb = CVMclassprivateReturnDeclaredMethodInClassMethods(
		        CVMcbInterfacecb(cb, i), tid);
		if (mb != NULL) {
		    goto found;
		}
	    }
	}
    } else {
	const CVMClassBlock *cb0 = cb;

	while (cb0 != NULL) {
	    /* Look in declared methods of the current class */
	    mb = CVMclassprivateReturnDeclaredMethodInClassMethods(cb0, tid);
	    if (mb != NULL) {
		if (CVMmbIs(mb, PRIVATE) && cb0 != cb) {
		    return NULL;
		} else {
		    goto found;
		}
	    }

	    /* <clinit> from current class only, not superclasses */
	    if (CVMtypeidIsStaticInitializer(tid)) {
		return NULL;
	    }

	    /* We're still here! Keep searching. */
	    cb0 = CVMcbSuperclass(cb0);
	}
    }

    return NULL;

found:

    return CVMmbIs(mb, STATIC) == isStatic ? mb : NULL;
}

/*
 * Look for declared method of type 'tid'
 */
CVMMethodBlock*
CVMclassGetDeclaredMethodBlockFromTID(const CVMClassBlock* cb,
				      CVMMethodTypeID tid)
{
    /* Look in methodTable of the given class */
    return CVMclassprivateReturnDeclaredMethodInClassMethods(cb, tid);
}

/*
 * Look for declared method of given name and signature
 */
CVMMethodBlock*
CVMclassGetDeclaredMethodBlock(CVMExecEnv *ee,
			       const CVMClassBlock* cb,
			       const char *name,
			       const char *sig)
{
    /*
     * We need to use the "new" function, because the "lookup" function
     * is only valid when we know that the type already exists.
     */
    CVMMethodTypeID tid = CVMtypeidNewMethodIDFromNameAndSig(ee, name, sig);

    if (tid != CVM_TYPEID_ERROR) {
	CVMMethodBlock* resultMb =
	    CVMclassGetDeclaredMethodBlockFromTID(cb, tid);
	CVMtypeidDisposeMethodID(ee, tid);
	return resultMb;
    } else {
	CVMclearLocalException(ee);
	return NULL;
    }
}

static CVMFieldBlock* 
CVMclassprivateReturnDeclaredFieldInClass(const CVMClassBlock* cb, 
					  const CVMFieldTypeID tid)
{
    int i;
    for (i = 0; i < CVMcbFieldCount(cb); i++) {
        CVMFieldBlock* fb = CVMcbFieldSlot(cb, i);
	if (CVMtypeidIsSame(CVMfbNameAndTypeID(fb), tid)) {
            return fb;
	}
    }
    return 0;
}

/*
 * Look for declared field of type 'tid'
 */
CVMFieldBlock*
CVMclassGetFieldBlock(const CVMClassBlock* cb, const CVMFieldTypeID tid,
		      CVMBool isStatic)
{
    const CVMClassBlock *cb0 = cb;
    CVMFieldBlock* fb;

    while (cb0 != NULL) {
	fb = CVMclassprivateReturnDeclaredFieldInClass(cb0, tid);
	if (fb != NULL) {
	    /* JavaSE allows JNI access to private fields */
#ifndef JAVASE
	    if (CVMfbIs(fb, PRIVATE) && cb0 != cb) {
		return NULL;
	    } else {
		return fb;
	    }
#else
            return fb;
#endif
	}

	/* We're still here! Keep searching. */
	cb0 = CVMcbSuperclass(cb0);
    }

    /*
     * OK, now let's look at implemented interfaces if we are looking
     * for a static 
     */
    if (isStatic) {
	int i;
	for (i = 0; i < CVMcbInterfaceCount(cb); i++) {
	    fb = CVMclassprivateReturnDeclaredFieldInClass(
                    CVMcbInterfacecb(cb, i), tid);
	    if (fb != NULL) {
		return fb;
	    }
	}
    }

    return NULL;
}

/*
 * Iterate over all classes, both romized and dynamically loaded,
 * and call 'callback' on each class.
 */
void
CVMclassIterateAllClasses(CVMExecEnv* ee, 
			  CVMClassCallbackFunc callback,
			  void* data)
{
    /* Iterate over all the romized classes first. */
    CVMpreloaderIterateAllClasses( ee, callback, data);

    /* Iterate over all dynamically loaded classes. */
    CVMclassIterateDynamicallyLoadedClasses(ee, callback, data);
}

/*
 * Iterate over the loaded classes, and call 'callback'
 * on each class.
 */
void
CVMclassIterateDynamicallyLoadedClasses(CVMExecEnv* ee, 
					CVMClassCallbackFunc callback,
					void* data)
{
    CVMclassTableIterate(ee, callback, data);
}

/*
 * Traverse the scannable state of one class, and call 'callback' on
 * each reference in it. The scannable state of the class is its
 * statics, and also the ClassLoader and javaInstance fields.  
 */
void
CVMclassScan(CVMExecEnv* ee, CVMClassBlock* cb,
	     CVMRefCallbackFunc callback, void* data)
{
    CVMUint32     numRefStatics = CVMcbNumStaticRefs(cb);
    if (CVMcbClassName(cb) != 0) {
	CVMtraceGcScan(("Scanning class %C (0x%x), "
			"with %d ref-type statics\n",
			cb, cb, numRefStatics));
    } else {
	CVMtraceGcScan(("Scanning class 0x%x, with %d ref-type statics\n",
			cb, numRefStatics));
    }
    if (numRefStatics > 0) {
	CVMJavaVal32* refStatics = CVMcbStatics(cb);
	CVMwalkContiguousRefs(refStatics, numRefStatics, callback, data);
    }
    /*
     * The instance can be NULL if the class is being loaded and
     * linked.  
     */
    if (CVMcbJavaInstance(cb) != NULL) {
	CVMwalkOneICell(CVMcbJavaInstance(cb), callback, data);
    }
    /*
     * There may or may not be a Protection Domain.
     */
    if (CVMcbProtectionDomain(cb) != NULL) {
	CVMwalkOneICell(CVMcbProtectionDomain(cb), callback, data);
    }
}

/*
 * Do the initialization of the classes module
 */
CVMBool
CVMclassModuleInit(CVMExecEnv* ee)
{
    /* assert some invariants */
#undef  assertValidSize
#define assertValidSize(type) \
    CVMassert((sizeof(type) & (sizeof(CVMUint32) - 1)) == 0);
    assertValidSize(CVMInterfaces);
    assertValidSize(CVMInterfaceTable);
    assertValidSize(CVMClassBlock);
    assertValidSize(CVMMethodBlock);
    assertValidSize(CVMFieldBlock);
    assertValidSize(CVMInnerClassesInfo);
    assertValidSize(CVMJavaMethodDescriptor);
    assertValidSize(CVMExceptionHandler);
#ifdef CVM_DEBUG_CLASSINFO
    assertValidSize(CVMLineNumberEntry);
    /*
     * We have split up the typeid into to parts, so this assertion
     * is no longer valid. But because the local variable entries are
     * the last ones in the jmd, this check is not really needed anyway.
     */
/*     assertValidSize(CVMLocalVariableEntry); */
#endif
    /*
     * CVMConstantPoolEntry is a union containing native pointers
     * therefore the check has to be adjusted for 64 bit platforms
     */
#ifdef CVM_64
    CVMassert(sizeof(CVMConstantPoolEntry) == 8);
#else
    /*
     * The size of a constantpool entry in the 32 bit case is
     * determined by the size of a typeid.
     */
    CVMassert(sizeof(CVMConstantPoolEntry) == sizeof(CVMTypeID));
#endif


    if (!CVMloaderCacheInit()) {
	return CVM_FALSE;
    }

    return CVM_TRUE;
}

/*
 * Destroy the classes module
 * Start by freeing all dynamically loaded classes.
 * We don't need to free any heap objects, of course.
 */

void
CVMclassModuleDestroy(CVMExecEnv* ee)
{
    CVMclassTableFreeAllClasses(ee);
    CVMloaderCacheDestroy(ee);
}

/*
 * This function is for debugging purpose, so we
 * can get the ClassBlock from a MethodBlock
 * easily in a debugger.
 */
CVMClassBlock* 
CVMgetClassBlock(CVMMethodBlock *mb)
{
    return CVMmbClassBlock(mb);
}

#ifdef CVM_INSPECTOR

/*============================================= ClassBlockValidityVerifier ==*/

typedef struct CVMClassIsValidClassData CVMClassIsValidClassData;
struct CVMClassIsValidClassData
{
    CVMClassBlock *cb;
    CVMBool found;
};

/* Purpose: Call back function for CVMgcIsValidClassBlock(). */
/* Returns: CVM_FALSE if the specified classblock is found (i.e. abort scan).
            Else returns CVM_TRUE. */
static void
CVMclassIsValidHeapClassBlockCallBack(CVMExecEnv *ee, CVMClassBlock *cb,
                                      void *data)
{
    CVMClassIsValidClassData *ivcdata = (CVMClassIsValidClassData *) data;
    if (ivcdata->cb == cb) {
        ivcdata->found = CVM_TRUE;
    }
}

/* Purpose: Checks to see if the specified pointer is a valid classblock. */
CVMBool CVMclassIsValidClassBlock(CVMExecEnv *ee, CVMClassBlock *cb)
{
    CVMClassIsValidClassData data;
    data.cb = cb;
    data.found = CVM_FALSE;

    /* Iterate over all the romized classes first: */
    CVMpreloaderIterateAllClasses(ee, CVMclassIsValidHeapClassBlockCallBack,
                                  (void *) &data);

    /* Iterate over all dynamically loaded classes if necessary: */
    if (!data.found) {
        CVMclassIterateDynamicallyLoadedClasses(ee,
                    CVMclassIsValidHeapClassBlockCallBack, (void *) &data);
    }
    return data.found;
}

/*===========================================================================*/

#endif
