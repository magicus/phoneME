/*
 * @(#)classcreate.c	1.195 06/10/25
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

#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/classes.h"
#ifdef CVM_DUAL_STACK
#include "javavm/include/dualstack_impl.h"
#endif
#include "javavm/include/limits.h"
#include "javavm/include/common_exceptions.h"
#include "javavm/include/typeid.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/localroots.h"
#include "javavm/include/preloader.h"
#include "javavm/include/globalroots.h"
#include "javavm/include/globals.h"
#include "javavm/include/utils.h"
#include "javavm/include/stackmaps.h"
#include "javavm/include/string.h"
#include "javavm/include/packages.h"
#include "javavm/include/verify.h"
#include "javavm/export/jvm.h"
#ifdef CVM_SPLIT_VERIFY
#include "javavm/include/split_verify.h"
#endif
#ifdef CVM_JVMPI
#include "javavm/include/jvmpi_impl.h"
#endif
#ifdef CVM_JVMTI
#include "javavm/include/jvmtiExport.h"
#endif
#ifdef CVM_JIT
#include "javavm/include/jit/jitmemory.h"
#include "javavm/include/porting/jit/ccm.h"
#endif

#include "generated/offsets/java_lang_Class.h"
#ifdef CVM_CLASSLOADING
#include "generated/offsets/java_lang_ClassLoader.h"
#include "generated/offsets/java_lang_Throwable.h"
#endif

#include "javavm/include/clib.h"
#include "javavm/include/porting/ansi/setjmp.h"
#include "javavm/include/porting/endianness.h"

/*
 * align byte code *
 */
#undef ALIGN_UP
#undef ALIGN_NATURAL
/* 
 * Make sure that the whole expression is casted to CVMUint32.
 * Original code:
 *  ((CVMUint32)((n) + ((align_grain) - 1)) & ~((align_grain)-1))
 */
#define ALIGN_UP(n,align_grain) \
    (CVMUint32)(((n) + ((align_grain) - 1)) & ~((align_grain)-1))
/* 
 * A natural alignment is an alinment to the type CVMAddr which
 * is 4 byte on 32 bit platforms and 8 byte on 64 bit platforms.
 */
#define ALIGN_NATURAL(p)        \
    (ALIGN_UP(p, sizeof(CVMAddr)))

CVMClassICell*
CVMdefineClass(CVMExecEnv* ee, const char *name, CVMClassLoaderICell* loader,
	       const CVMUint8* buf, CVMUint32 bufLen, CVMObjectICell* pd,
	       CVMBool isRedefine)

{
#ifndef CVM_CLASSLOADING
    CVMthrowUnsupportedOperationException(ee, "Classloading not supported");
    return NULL;
#else
    CVMClassICell*  classGlobalRoot;
    CVMObjectICell* resultCell;
    CVMObjectICell* pdCell;

    resultCell = CVMjniCreateLocalRef(ee);
    if (resultCell == NULL) {
	return NULL; /* exception already thrown */
    }

    if (pd != NULL) {
	pdCell = CVMID_getProtectionDomainGlobalRoot(ee);
	if (pdCell == NULL) {
	    CVMjniDeleteLocalRef(CVMexecEnv2JniEnv(ee), resultCell);
	    CVMthrowOutOfMemoryError(ee, NULL);
	    return NULL;
	}
    } else {
	pdCell = NULL;
    }

    classGlobalRoot = 
	CVMclassCreateInternalClass(ee, buf, bufLen, loader, name, NULL,
				    isRedefine);
    if (classGlobalRoot == NULL) {
	CVMjniDeleteLocalRef(CVMexecEnv2JniEnv(ee), resultCell);
	if (pdCell != NULL) {
	    CVMID_freeProtectionDomainGlobalRoot(ee, pdCell);
	}
	return NULL; /* exception already thrown */
    }

    {
	CVMClassBlock* cb = CVMgcSafeClassRef2ClassBlock(ee, classGlobalRoot);
	CVMcbProtectionDomain(cb) = pdCell;
	if (pdCell != NULL){
	    /*
	     * Assign to this cell after assigning the cell to the cb
	     * so that it gets scanned. Any earlier and there could be
	     * a race.
	     */
	    CVMID_icellAssign(ee, pdCell, pd);
	}
    }

    /* We must free the global root returned, but not until after we've
     * assigned it to another root to keep the Class alive until
     * Class.loadSuperClasses() is called.
     */
    CVMID_icellAssign(ee, resultCell, classGlobalRoot);
    CVMID_freeGlobalRoot(ee, classGlobalRoot);

    return resultCell;
#endif
}

/*
 * Create a java.lang.Class instance that corresponds to the new class cb.
 */
static CVMClassICell*
CVMclassCreateJavaInstance(CVMExecEnv* ee, CVMClassBlock* cb,
			   CVMClassLoaderICell* loader)
{
    CVMClassICell* classCell = CVMID_getGlobalRoot(ee);
    if (classCell == NULL) {
	CVMthrowOutOfMemoryError(ee, NULL);
    } else {
	CVMBool success;
	CVMD_gcUnsafeExec(ee, {
	    CVMObject* newInstance = CVMgcAllocNewClassInstance(ee, cb);
	    if (newInstance != NULL) {
		CVMID_icellSetDirect(ee, classCell, newInstance);
		/* Make sure the back-pointer is filled */
		/*
		 * cb is a native pointer (CVMClassBlock*)
		 * therefore the cast has to be CVMAddr which is 4 byte on
		 * 32 bit platforms and 8 byte on 64 bit platforms
		 */
		CVMassert(((CVMAddr*)newInstance)[CVMoffsetOfjava_lang_Class_classBlockPointer] == (CVMAddr)cb);
		if (loader != NULL) {
		    CVMObject* directClassLoaderObj =
			CVMID_icellDirect(ee, loader);
		    /*
		     * Fill in the class loader field in the
		     * java.lang.Class, so that keeping a Class alive
		     * keeps its ClassLoader alive as well 
		     */
		    CVMD_fieldWriteRef(newInstance,
				       CVMoffsetOfjava_lang_Class_loader,
				       directClassLoaderObj);
		}
	    }
	    success = newInstance != NULL;
	});
	if (!success) {
	    CVMID_freeGlobalRoot(ee, classCell);
	    CVMthrowOutOfMemoryError(ee, NULL);
	    classCell = NULL;
	}
    }

    return classCell;
}

/*
 * CVMclassSetupClassLoaderRoot - silly function that is needed because
 * we can't put an #ifdef within an CVMD_gcUnsafeExec block.
 */
static void
CVMclassSetupClassLoaderRoot(CVMExecEnv* ee, CVMClassBlock* cb,
			     CVMClassLoaderICell* newClassLoaderRoot)
{
#ifdef CVM_CLASSLOADING
    if (newClassLoaderRoot != NULL) {
	CVMcbClassLoader(cb) = newClassLoaderRoot;
    }
#endif
}

/*
 * CVMclassAddToLoadedClasses - Add the class to our class databases. There
 * are 4 of them:
 *   1. classGlobalRoots - used by gc to keep all loaded classes live
 *      when doing partial gc.
 *   2. class table - used by gc and jvmti. Akin to JDK bin_classes.
 *   3. loader cache - cache of <Class,ClassLoader> pairs used to speed
 *      up class lookups.
 *   4. ClassLoader - each ClassLoader maintains a private database of
 *      Classes it has loaded. This is needed to keep a Class live when
 *      there is no instances of the Class and the ClassLoader is still
 *      live.
 * However, (3) and (4) are deferred until after superclasses have
 * been loaded. See Class.loadSuperClasses().
 */

static CVMBool
CVMclassAddToLoadedClasses(CVMExecEnv* ee, CVMClassBlock* cb,
			   CVMClassICell* oldClassRoot)
{
    CVMClassICell* newClassRoot;
    CVMClassLoaderICell* newClassLoaderRoot;
    CVMClassBlock** classTableSlot = NULL;
    CVMBool result;
    
    /* 
     * We make no attempt to keep gc from accessing the ClassLoader
     * global roots, Class global roots, or the class table while we
     * are allocating slots from them.  It turns out that it is still
     * safe to allocate and free these resources while gc is accessing
     * them. The reason is because the cells are always empty while
     * they are being allocated or freed, so if gc does see one that
     * is the in process of being allocated or freed, it's just going
     * to ignore it anyway.
     */

    /* 
     * Get the ClassLoader global root to replace the root
     * currently stored in CVMcbClassLoader(). The CVMcbClassLoader()
     * root is not one we explicitly allocated. It either came from a
     * java stack frame or from the CVMcbClassLoader() field of
     * another loaded class. Because of this we don't need to worry
     * about deleting the original CVMcbClassLoader() root.
     */
#ifndef CVM_CLASSLOADING
    newClassLoaderRoot = NULL;
#else
    if (CVMcbClassLoader(cb) == NULL) {
	newClassLoaderRoot = NULL;
    } else {
	/* Get the current ClassLoader.loaderGlobalRoot field */
	/* 
	 * Access member variable of type 'int'
	 * and cast it to a native pointer. The java type 'int' only guarantees
	 * 32 bit, but because one slot is used as storage space and
	 * a slot is 64 bit on 64 bit platforms, it is possible 
	 * to store a native pointer without modification of
	 * java source code. This assumes that all places in the C-code
	 * which set/get this member are caught.
	 */
	CVMAddr temp;
	CVMID_fieldReadAddr(ee, CVMcbClassLoader(cb),
			    CVMoffsetOfjava_lang_ClassLoader_loaderGlobalRoot,
			    temp);
	newClassLoaderRoot = (CVMClassLoaderICell*)temp;
    }
#endif
    
    /*
     * 1. Allocate a Class global root to replace the regular global
     * root currently keeping the Class instance live. Note that we must
     * hold onto the regular global root until after the new Class
     * global root has been assigned and the class loader has been
     * notified of the new class, or it may get gc'd.
     */
    newClassRoot = CVMID_getClassGlobalRoot(ee);
    if (newClassRoot == NULL) {
	CVMthrowOutOfMemoryError(ee, NULL);
	goto done; /* exception already thrown */
    }
    
    /*
     * 2. Allocate a class table slot.
     */
    classTableSlot = CVMclassTableAllocateSlot(ee);
    if (classTableSlot == NULL) {
	CVMID_freeClassGlobalRoot(ee, newClassRoot);
	CVMthrowOutOfMemoryError(ee, NULL);
	goto done; /* exception already thrown */
    }

    /*
     * We definitely need to do the following without fear that gc
     * will start up in the middle, because we will be in an inconsistent
     * state while doing the following assignments. This is why we got
     * all the allocations above out of the way first.
     */
    CVMD_gcUnsafeExec(ee, {
	*classTableSlot = cb;
	CVMcbJavaInstance(cb) = newClassRoot;
	CVMID_icellAssignDirect(ee, newClassRoot, oldClassRoot);
        /* This is the starting point from which the Class object gets
           registered with the VM and special handling is required to
           GC it later: */
        CVMglobals.gcCommon.classCreatedSinceLastGC = CVM_TRUE;
	CVMclassSetupClassLoaderRoot(ee, cb, newClassLoaderRoot);
    });

    /*
     * After this point, if there is a failure then the class will get freed
     * when it is gc'd. This will happen very shortly since there will be
     * no class global root or ClassLoader instance refering to the class.
     */

 done:
    if (CVMlocalExceptionOccurred(ee)) {
	/*
	 * We failed. If the Class global root was successfully allocated,
	 * then the class will be gc'd and CVMclassFree() will handle the 
	 * cleanup. Otherwise, upon return it's up to the caller to handle
	 * the cleanup.
	 */
	result = CVM_FALSE;
    } else {
	result = CVM_TRUE;
    }

    return result;
}

/*
 * CVMclassCreateArrayClass - creates a fake array class. Needed for any
 * array class type that wasn't reference by romized code and we make 
 * a reference to it at runtime.
 */
static CVMClassBlock*
CVMclassCreateArrayClass(CVMExecEnv* ee, CVMClassTypeID arrayTypeId,
			 CVMClassLoaderICell* loader, CVMObjectICell* pd)
{
    CVMClassBlock* elemCb = NULL;
    CVMClassBlock* arrayCb;
    CVMUint32      elemClassAccess;
    CVMClassTypeID elementTypeID = 
	CVMtypeidIncrementArrayDepth(ee, arrayTypeId, -1);
    CVMClassICell* arrayClass = NULL;
    CVMClassLoaderICell* arrayLoader;

    CVMtraceClassLoading(("CL: Creating array <%!C>\n", arrayTypeId));

    /* 
     * All primitive array types are part of the romized set so
     * we should never hit this assert.
     */
    CVMassert(!CVMtypeidIsPrimitive(elementTypeID));

    if (CVMtypeidIsArray(elementTypeID)) {
	/*
	 * The element class is an array class, so lookup the array element
	 * class. This may cause recursion back into CVMclassMakeArrayClass
	 * if the element array class doesn't exists yet.
	 */
	/* %comment: c022 */

	elemCb = CVMclassLookupByTypeFromClassLoader(ee, elementTypeID,
					       CVM_FALSE,
					       loader, pd, CVM_FALSE);
    } else {
	/* lookup the element class */
	elemCb = CVMclassLookupByTypeFromClassLoader(ee, elementTypeID,
					       CVM_FALSE,
					       loader, pd, CVM_FALSE);
    }
    CVMtypeidDisposeClassID(ee, elementTypeID);

    if (elemCb == NULL) {
	return NULL; /* exception already thrown */
    }

    /*
     * The "loader" passed in is the initiating ClassLoader. For here on
     * we want "loader" to be the ClassLoader of elemCb.
     */
    arrayLoader = CVMcbClassLoader(elemCb);

    /*
     * Fix for 4490166: Check if the array class is already defined in the
     * defining class loader of the element class before creating one.
     * We need this step since we don't perform CVMclassLoadClass() 
     * against an array class directly.
     */
    if (arrayLoader != loader) {
	/* Need to lock the arrayLoader too */
	if (arrayLoader == NULL) {
	    CVM_NULL_CLASSLOADER_LOCK(ee);
	} else {
	    if (!CVMgcSafeObjectLock(ee, arrayLoader)) {
		CVMthrowOutOfMemoryError(ee, NULL);
		return NULL;
	    }
	}
	arrayCb = CVMclassLookupByTypeFromClassLoader(ee, arrayTypeId,
						      CVM_FALSE, arrayLoader, 
						      pd, CVM_FALSE);
	if (arrayCb != NULL) {
	    /* Found the array class */
	    goto finish;
	} else {
	    CVMClassBlock* classNotFoundCb =
		CVMsystemClass(java_lang_ClassNotFoundException);
	    CVMClassBlock* noClassDefFoundCb =
		CVMsystemClass(java_lang_NoClassDefFoundError);
	    CVMClassBlock* exceptionCb;
	    CVMID_objectGetClass(ee, CVMlocalExceptionICell(ee), exceptionCb);

	    if (CVMisSubclassOf(ee, exceptionCb, classNotFoundCb)
		|| CVMisSubclassOf(ee, exceptionCb, noClassDefFoundCb)) {
		/* Not loaded yet. Clear the error and keep going */
		CVMclearLocalException(ee);
	    } else {
		/* Critical error must have occurred. Discontinue the work */
		goto finish;
	    }
	}
    }

    /*
     * Allocate memory for the array class.
     */
    arrayCb = (CVMClassBlock*)
	calloc(1, sizeof(CVMClassBlock) + sizeof(CVMArrayInfo));
    if (arrayCb == NULL) {
	CVMthrowOutOfMemoryError(ee, NULL);
	goto finish;
    }
    CVMcbArrayInfo(arrayCb) = (CVMArrayInfo*)(arrayCb + 1);

    /* Must be done before calling CVMarrayXXX macros to avoid asserts. */
    CVMcbClassName(arrayCb) = CVMtypeidCloneClassID(ee, arrayTypeId );

    /*
     * Two cases. The new CVMArrayInfo is formed differently in each case.
     *
     * 1) The element type is an array.
     * 2) The element type is a class.
     *
     * This function does not make arrays of basic types. Those are romized.
     */
    CVMassert(!CVMcbIs(elemCb, PRIMITIVE));
    if (CVMtypeidIsArray(CVMcbClassName(elemCb))) {
	/*
	 * And now, make up the details of the array info struct
	 * for the new array class.
	 */
	CVMarrayDepth(arrayCb)    = CVMarrayDepth(elemCb) + 1;
	CVMarrayBaseType(arrayCb) = CVMarrayBaseType(elemCb);
	CVMarrayBaseCb(arrayCb)   = CVMarrayBaseCb(elemCb);
    } else {
	/*
	 * elemCb must be a class (it can't be a basic type, since
	 * we already have the array classes of basic types stashed
	 * away in CVMbasicTypeArrayClassblocks[])
	 */
	/*
	 * And now, make up the details of the fake constant pool
	 * for the new array
	 */
	CVMarrayDepth(arrayCb)    = 1;
	CVMarrayBaseType(arrayCb) = CVM_T_CLASS;
	CVMarrayBaseCb(arrayCb)   = elemCb;
    }
    CVMarrayElementCb(arrayCb) = elemCb;

    /*
     * Fill in the other details
     */
    CVMcbGcMap(arrayCb).map   = 
	CVM_GCMAP_ALLREFS_FLAG; /* No basic type arrays! */
    CVMcbSuperclass(arrayCb)  = 
	(CVMClassBlock*)CVMsystemClass(java_lang_Object);
    CVMcbInterfaces(arrayCb)  =
	CVMcbInterfaces(CVMsystemClass(java_lang_Object));
    /* not in ROM, no <clinit> nor statics */
    CVMassert(!CVMcbIsInROM(arrayCb));
    CVMassert(!CVMcbHasStaticsOrClinit(arrayCb));

    /*
     * We are as public as our element class
     */
    elemClassAccess = CVMcbIs(elemCb, PUBLIC);
    CVMcbAccessFlags(arrayCb) =
	elemClassAccess | CVM_CLASS_ACC_ABSTRACT | CVM_CLASS_ACC_FINAL;

    CVMcbRuntimeFlags(arrayCb) =
	CVM_CLASS_SUPERCLASS_LOADED | CVM_CLASS_LINKED |
	CVM_CLASS_VERIFIED;
    CVMcbSetInitializationDoneFlag(ee, arrayCb);

    /*
     * Use the java/lang/Object method table
     */
    CVMcbMethodTablePtr(arrayCb) =
	CVMcbMethodTablePtr(CVMsystemClass(java_lang_Object));
    CVMcbMethodTableCount(arrayCb) =
        CVMcbMethodTableCount(CVMsystemClass(java_lang_Object));

    /* 
     * Create the Java side "mirror" to this class
     */
    arrayClass = CVMclassCreateJavaInstance(ee, arrayCb, arrayLoader);
    if (arrayClass == NULL) {
	/* free everything and return */
	free(arrayCb);
	arrayCb = NULL;
	goto finish;
    }

#ifdef CVM_CLASSLOADING
    CVMcbClassLoader(arrayCb) = arrayLoader;
#endif

    /*
     * Add the class to our class database and let the class loader know 
     * about the class.
     */
    if (!CVMclassAddToLoadedClasses(ee, arrayCb, arrayClass)) {
	/*
	 * If CVMcbJavaInstance() is still NULL, then the new Class 
	 * global root was never setup, which means we are responsible for 
	 * freeing the class.
	 */
	if (CVMcbJavaInstance(arrayCb) == NULL) {
	    free(arrayCb);
	}
	arrayCb = NULL; /* exception already thrown */
	goto finish;
    }

    /*
     * After this point, if there is a failure then the class will get
     * freed when it is gc'd. This will happen very shortly since
     * there will be no class global root or ClassLoader instance
     * refering to the class.
     */
    
    /* Add to the loader cache. */
    if (!CVMloaderCacheAdd(ee, arrayCb, CVMcbClassLoader(arrayCb))) {
	arrayCb = NULL; /* exception already thrown */
	goto finish;
    }
    
#ifdef CVM_CLASSLOADING
    /* 
     * Call ClassLoader.addClass() to add to the ClassLoader's
     * database of classes.
     */
    if (CVMcbClassLoader(arrayCb) != NULL) {
	JNIEnv* env = CVMexecEnv2JniEnv(ee);
	/* We need a JNI local frame so we can make a JNI method call. */
	if (CVMjniPushLocalFrame(env, 1) == JNI_OK) {
	    (*env)->CallVoidMethod(env, CVMcbClassLoader(arrayCb),
				   CVMglobals.java_lang_ClassLoader_addClass,
				   arrayClass);
	    CVMjniPopLocalFrame(env, NULL);
	}
	if (CVMlocalExceptionOccurred(ee)) {
	    /* make sure a loaderCache lookup will fail */
	    CVMcbSetErrorFlag(ee, arrayCb);
	    CVMcbClearRuntimeFlag(arrayCb, ee, SUPERCLASS_LOADED);
	    arrayCb = NULL; /* exception already thrown */
	    goto finish;
	}
    }
#endif
	
    CVMtraceClassLoading(("CL: Created array cb=0x%x <%!C>\n",
			  arrayCb, arrayTypeId));

#ifdef CVM_JVMPI
    /*
     * JVMPI is notified of other class load events in
     * java.lang.Class.notifyClassLoaded. But that never
     * gets called for arrays, so we do it here.
     */
    if (CVMjvmpiEventClassLoadIsEnabled()) {
	CVMjvmpiPostClassLoadEvent(ee, arrayCb);
    }
#endif /* CVM_JVMPI */

 finish:
    if (arrayLoader != loader) {
	/* Need to unlock the arrayLoader */
	if (arrayLoader == NULL) {
	    CVM_NULL_CLASSLOADER_UNLOCK(ee);
	} else {
	    CVMBool success = CVMgcSafeObjectUnlock(ee, arrayLoader);
	    CVMassert(success); (void) success;
	}
    }

    /* 
     * We can free the arrayClass root that was being used to keep the
     * Class instance alive. If there was a failure, then this will allow
     * the Class to be gc'd. If there wasn't a failure, then calling
     * ClassLoader.addClass() will keep the class alive.
     */
    if (arrayClass != NULL) {
	CVMID_freeGlobalRoot(ee, arrayClass);
	arrayClass = NULL;
    }
	
    return arrayCb;
}

/*
 * CVMclassCreateMultiArrayClass - creates the specified array class by
 * iteratively (i.e. non-recursively) creating all the layers of the array
 * class from the inner most to the outer most.  Needed for any array class
 * type that wasn't reference by romized code and we make a reference to it
 * at runtime.
 */
CVMClassBlock*
CVMclassCreateMultiArrayClass(CVMExecEnv* ee, CVMClassTypeID arrayTypeId,
			      CVMClassLoaderICell* loader, CVMObjectICell* pd)
{
    int outerDepth = 0;
    CVMClassBlock *arrayCb = NULL;
    CVMClassTypeID currentTypeID = arrayTypeId;
    CVMClassTypeID elemTypeID;
    CVMBool currentTypeIDWasAcquired = CVM_FALSE;
    int i;

    CVMassert(CVMtypeidIsArray(arrayTypeId));

    /* Count the array layers that haven't been loaded yet.  We start from the
       outer most until we get to an array element that has been loaded or an
       array element that is not an array type.  We can assume that at least
       one layer isn't loaded yet (regardless of whether it is or not).

       CVMclassCreateArrayClass() will do the real work of loading the array
       class later, and will be called once for each layer that we counted.
       In the case that we only counted one layer, we'll call
       CVMclassCreateArrayClass() only once, and it will take care of loading
       the array (if it hasn't already been loaded) where its element is
       guaranteed to either have already been loaded, or is a non-array type.

       The guarantee comes from our pre-counting the number of layers that need
       to be loaded, and doing the loading from the innermost layer to the
       outermost.
    */
    while (CVM_TRUE) {
	elemTypeID = CVMtypeidIncrementArrayDepth(ee, currentTypeID , -1);

	outerDepth++;

	/* If the element is also an array, then keep probing.  Otherwise,
	    we need to load it using the normal path: */
	if (CVMtypeidIsArray(elemTypeID)) {

	    CVMClassBlock *elemCb = NULL;

	    /* The element of the current type is also an array.  Look it up: */
	    if (loader == NULL ||
		CVMtypeidIsPrimitive(CVMtypeidGetArrayBasetype(elemTypeID))) {
    		elemCb = CVMpreloaderLookupFromType(ee, elemTypeID, NULL);
	    }
	    if (elemCb == NULL) {
		CVM_LOADERCACHE_LOCK(ee);
		elemCb = CVMloaderCacheLookup(ee, elemTypeID, loader);
		CVM_LOADERCACHE_UNLOCK(ee);
	    }

	    /* If we've found the element cb, then we're done because we can
	       start building the array layers from there: */
	    if (elemCb != NULL) {
		break;
	    }
	} else {
	    /* If we get here, then we've peeled the layers down to a single
	       dimensional array (we have reached the non-array element here).
	       We can start building the array layers from here: */
	    break;
	}

	/* If we get here, then the element isn't loaded yet and is not a
	   single dimension array.  Let's see if the element's element has
	   been loaded yet.  Set prepare to repeat this loop for element
	   layer. */

	/* If the previous currentTypeID is one that we acquired using
	   CVMtypeidIncrementArrayDepth() above, then we need to dispose
	   of it before losing track of it: */
	if (currentTypeIDWasAcquired) {
	    CVMtypeidDisposeClassID(ee, currentTypeID);
	}

	/* Prepare to inspect the next inner level of array type: */
	currentTypeID = elemTypeID;
	currentTypeIDWasAcquired = CVM_TRUE;
    }

    /* We have to dispose of the last elemTypeID that we acquired above using
       CVMtypeidIncrementArrayDepth(): */
    CVMtypeidDisposeClassID(ee, elemTypeID);

    for (i = 0; i < outerDepth; i++) {
	CVMClassTypeID newTypeID;
	
	arrayCb = CVMclassCreateArrayClass(ee, currentTypeID, loader, pd);
	/* If we fail to load the element, then something wrong must have
	   happened (e.g. an OutOfMemoryError): */
	if (arrayCb == NULL) {
	    break;
	}
	/* If we've just loaded the requested array type, then we're done.
	   Skip the typeid work below: */
	if (currentTypeID == arrayTypeId) {
	    CVMassert(i == outerDepth - 1);
	    break;
	}

	/* Prepare to load the next outer level of array type: */
	newTypeID = CVMtypeidIncrementArrayDepth(ee, currentTypeID, 1);
	if (newTypeID == CVM_TYPEID_ERROR) {
	    /* InternalError should have been thrown */
	    CVMassert(CVMlocalExceptionOccurred(ee));
	    arrayCb = NULL;
	    break;
	}
	if (currentTypeIDWasAcquired) {
	    CVMtypeidDisposeClassID(ee, currentTypeID);
	}
	currentTypeID = newTypeID;
	currentTypeIDWasAcquired = CVM_TRUE;
    }
    if (currentTypeIDWasAcquired) {
	CVMtypeidDisposeClassID(ee, currentTypeID);
    }

    return arrayCb;
}

/*
 * CVMclassCreateInternalClass - Creates a CVMClassBlock out of the
 * array of bytes passed to it.
 */

#ifdef CVM_CLASSLOADING

/*
 * CICcontext maintains our context while we parse the array of bytes.
 */
typedef struct CICcontext CICcontext;
struct CICcontext {
    const CVMUint8* ptr;
    const CVMUint8* end_ptr;
#ifdef CVM_DEBUG_CLASSINFO
    const CVMUint8* mark;
#endif
    jmp_buf jump_buffer;

    const char* classname;

    char*          exceptionMsg;
    CVMClassBlock* exceptionCb;

    CVMClassICell*   oldClassRoot;  /* global root for the Class instance */
    CVMClassBlock* cb;
    CVMConstantPool* cp;
    CVMConstantPool* utf8Cp; /* temporary cp for storing utf8 entries */

    CVMUint16 major_version;
    CVMUint16 minor_version;

    int in_clinit;	/* whether we are loading the <clinit> method */

    /*
     * We keep track of 4 regions of memory:
     */
    CVMUint8* intfSpace; /* CVMInterfaces */
    CVMUint8* mainSpace; /* constant pool, methods, fields, and statics area */
    CVMUint8* jmdSpace;  /* CVMJavaMethodDescriptors */
    CVMUint8* clinitJmdSpace; /* <clinit> CVMJavaMethodDescriptors */

    CVMUint8* nextJmdSpace;   /* next free byte in jmdSpace */
#ifdef CVM_DEBUG
    CVMUint8* jmdSpaceLimit;
    CVMUint8* clinitJmdSpaceLimit;
#endif
    CVMBool   needsVerify;
 
    /*
     * "size" keeps track of counts of certains items in the class file. 
     * "offset" keeps track of where the items end up being stored in the
     * various data structures which represent the class.
     */
    class_size_info size;
    class_size_info offset;
};

/* Read in class bytes */
static CVMUint8  get1byte(CICcontext*);
static CVMUint16 get2bytes(CICcontext*);
static CVMUint32 get4bytes(CICcontext*);
static CVMUtf8*  getUTF8String(CVMExecEnv* ee, CICcontext*);
static void getNbytes(CICcontext*, size_t count, CVMUint8* buffer);

#ifdef CVM_DEBUG_CLASSINFO
/* Mark a location in the class bytes so we can rewind to it.*/
static void mark(CICcontext*);
static void rewindToMark(CICcontext*);
#endif

/* Free up memory allocated while creating the class. */
static void CVMfreeAllocatedMemory(CVMExecEnv* ee, CICcontext*);

/* Free up the utf8 constant pool and all utf8 entries */
static void CVMfreeUtf8Memory(CICcontext*);

/* read in the constant pool */
static void 
CVMreadConstantPool(CVMExecEnv* ee, CICcontext *);

/* read in a method's "Code" attribute. */
static void
CVMreadCode(CVMExecEnv* ee, CICcontext* context, CVMMethodBlock* mb,
	    CVMBool isStrictFP);

/* read in a method's checked exceptions. */
static CVMCheckedExceptions*
CVMreadCheckedExceptions(CICcontext *context, CVMMethodBlock* mb, 
			 CVMCheckedExceptions* checkedExceptions);

#ifdef CVM_SPLIT_VERIFY
/* Read in JVMSv3-style StackMapTable attribute */
static CVMsplitVerifyStackMaps*
CVMreadStackMapTable(CVMExecEnv* ee, CICcontext* context, CVMMethodBlock *mb,
	     CVMUint32 length, CVMUint32 codeLength,
	     CVMUint32 maxLocals, CVMUint32 maxStack);
/* Read in CLDC-style StackMap attribute */
static CVMsplitVerifyStackMaps*
CVMreadStackMaps(CVMExecEnv* ee, CICcontext* context, CVMUint32 length,
	     CVMUint32 codeLength, CVMUint32 maxLocals, CVMUint32 maxStack);
#endif

/* used to compare LineNumberTable entries during sorting. */
#ifdef CVM_DEBUG_CLASSINFO
static int 
CVMlineNumberCompare(const void *p1, const void *p2);
#endif


/* A vm function threw an exception. */
static void 
CVMexceptionHandler(CVMExecEnv* ee, CICcontext* context);

/* We failed to allocate some memory. */
static void 
CVMoutOfMemoryHandler(CVMExecEnv* ee, CICcontext* context);

/* A VM-internal limit has been hit. */
static void 
CVMlimitHandler(CVMExecEnv* ee, CICcontext* context, char* comment);

static CVMClassBlock*
CVMallocClassSpace(CVMExecEnv* ee, CICcontext* context, CVMBool isRedefine);

/* Purpose: Creates an CVMClassBlock out of the array of bytes passed to it. */
/* Parameters: externalClass - the buffer.
 *             classSize - size of the buffer. */
/* NOTE: This should be called while the current thread is in a GC safe
 *       state. */
CVMClassICell*
CVMclassCreateInternalClass(CVMExecEnv* ee, 
			    const CVMUint8* externalClass, 
			    CVMUint32 classSize, 
			    CVMClassLoaderICell* loader, 
			    const char* classname,
			    const char* dirNameOrZipFileName,
			    CVMBool isRedefine)
{
    CVMClassBlock*   cb;
    CVMConstantPool* cp;
    CVMConstantPool* utf8Cp;
    int attributeCount;
    int i, j, cpIdx;
    CICcontext context_block;
    CICcontext *context = &context_block;
    CVMUint8 * volatile buffer = (CVMUint8 *) externalClass;
    volatile CVMInt32 bufferLength = classSize;
#ifdef CVM_JVMTI
    CVMUint8 *newBuffer = NULL;
    CVMInt32 newBufferLength = 0;
    volatile CVMBool jvmtiBufferWasReplaced = CVM_FALSE;
#endif
#ifdef CVM_DUAL_STACK
    CVMBool isMidletClass;
#endif
#ifdef CVM_JVMPI
    volatile CVMBool bufferWasReplaced = CVM_FALSE;
#endif
#ifdef CVM_JVMTI
    if (CVMjvmtiShouldPostClassFileLoadHook()) {
	jclass klass = NULL;
	CVMClassLoaderICell* loaderToUse = loader;
	/* At this point in time we may not have a classblock or
	 * a protection domain so we pass NULL
	 */
	if (isRedefine) {
	    CVMClassBlock*   oldCb = NULL;
	    /* ClassLoadHook event tests require klass to be the
	     * class that is being redefined.  Also the loader 
	     * should be the loader for the class being redefined
	     */
	    oldCb = CVMjvmtiGetCurrentRedefinedClass(ee);
	    CVMassert(oldCb != NULL);
	    klass = CVMcbJavaInstance(oldCb);
	    if (loader == NULL) {
		loaderToUse = CVMcbClassLoader(oldCb);
	    }
	}
	CVMjvmtiPostClassLoadHookEvent(klass, loaderToUse, classname,
				       NULL, bufferLength, buffer,
				       &newBufferLength, &newBuffer);
	if (newBuffer != NULL) {
	    /* class was replaced with new one */
            jvmtiBufferWasReplaced = CVM_TRUE;
	    buffer = newBuffer;
	    bufferLength = newBufferLength;
	}
    }
#endif
#ifdef CVM_JVMPI

    if (CVMjvmpiEventClassLoadHookIsEnabled()) {
        CVMjvmpiPostClassLoadHookEvent((CVMUint8**)&buffer,
                                       (CVMInt32*)&bufferLength,
                                       (void *(*)(unsigned int))&malloc);

        /* If buffer is NULL, then the profiler must have attempted to allocate
         * a buffer of its own for instrumentation and failed: */
        if (buffer == NULL) {
            CVMthrowOutOfMemoryError(ee, "instrumenting class \"%s\"",
                                     classname);
            return NULL;

        /* Else if buffer is not the same as the original, then the profiler
         * must have successfully attempted to allocate a buffer of its own for
         * instrumentation and succeeded: */
        } else if (buffer != externalClass) {
            bufferWasReplaced = CVM_TRUE;

        /* Else buffer must be the same as the original i.e. the profiler does
         * not wish to instrument this class.  This is in accordance with the
         * requirements laid out in the JVMPI specs: */
        } else {
            CVMassert(buffer == externalClass);
            CVMassert(bufferLength == classSize);
        }
    }
#endif  /* CVM_JVMPI */

    memset(context, 0, sizeof(CICcontext));
    context->classname = classname;

#ifdef CVM_DUAL_STACK
    CVMD_gcUnsafeExec(ee, {
        isMidletClass = CVMclassloaderIsMIDPClassLoader(
	    ee, loader, CVM_FALSE);
    });
#endif

    {
        char buf[200];
	jboolean measure_only =
	    CVMloaderNeedsVerify(ee, loader, CVM_TRUE) ? JNI_FALSE : JNI_TRUE;
	jboolean check_relaxed =
	    CVMloaderNeedsVerify(ee, loader, CVM_FALSE) ? JNI_FALSE 
	    				        	          : JNI_TRUE;
        jint res = CVMverifyClassFormat(classname, buffer, bufferLength,
					&(context->size), 
					buf, sizeof(buf),
					measure_only,
#ifdef CVM_DUAL_STACK
                                        isMidletClass,
#endif
					check_relaxed);
	if (res == -1) { /* nomem */
	    CVMthrowOutOfMemoryError(ee, "loading class \"%s\"", classname);
            goto doCleanup;
	}
	if (res == -2) { /* format error */
	    CVMthrowClassFormatError(ee, "%s", buf);
            goto doCleanup;
	}
	if (res == -3) { /* unsupported class version error */
	    CVMthrowUnsupportedClassVersionError(ee, "%s", buf);
            goto doCleanup;
	}
	if (res == -4) { /* bad name */
	    CVMthrowNoClassDefFoundError(ee, "%s", buf);
            goto doCleanup;
	}
#ifdef JAVASE
	if (res == -5) { /* verify error, failure in byte code verification */
	    CVMthrowVerifyError(ee, "%s", buf);
            goto doCleanup;
	}
#endif
	context->needsVerify = !(CVMBool)measure_only;
    }

    /* Setup for handling failures. */
    if (setjmp(context->jump_buffer)) {
	CVMBool freeExceptionMsg = CVM_FALSE;

	/* We got an error of some sort */
	CVMfreeAllocatedMemory(ee, context);
	
	/*
	 * Throw the appropriate exception. The needed information is
	 * stored in the context. We wait until after freeing any allocated
	 * memory so we don't run out of malloc space while 
	 * throwing the exception.
	 */
	
	if (CVMlocalExceptionOccurred(ee)) {
	    /*
	     * Someone has already thrown the exception for us (probably
	     * the typeid system), but we want to prefix the exception 
	     * message with "loading class...", so we rethrow the 
	     * exception with the proper message.
	     */
	    
	    /* Store the exception message into context->exceptionMsg. */
	    CVMID_localrootBegin(ee) {
		CVMID_localrootDeclare(CVMStringICell, exceptionString);
		CVMID_fieldReadRef(
                    ee, CVMlocalExceptionICell(ee), 
		    CVMoffsetOfjava_lang_Throwable_detailMessage,
		    exceptionString);
		
		/*
		 * We no longer need the old exception and we also need
		 * to clear it before calling CVMjniGetStringUTFChars().
		 */
		CVMclearLocalException(ee);
		
		/* get a copy of the current exception message */
		if (!CVMID_icellIsNull(exceptionString)) {
		    JNIEnv* env = CVMexecEnv2JniEnv(ee);
		    jboolean isCopy = JNI_FALSE;
		    const char* msg = 
			CVMjniGetStringUTFChars(env, exceptionString, &isCopy);
		    if (msg == NULL) {
			/* Looks like CVMjniGetStringUTFChars() failed. */
			CVMclearLocalException(ee);
		    } else {
			/* dup and free the orignal exception message. */
			context->exceptionMsg = strdup(msg);
			freeExceptionMsg = CVM_TRUE;
			if (isCopy) {
			    CVMjniReleaseStringUTFChars(
			        env, exceptionString, msg);
			}
		    }
		}
	    } CVMID_localrootEnd();
	}
	
	if (context->exceptionMsg == NULL) {
	    CVMsignalError(ee, context->exceptionCb,
			   "loading class \"%s\"", classname);
	} else {
	    CVMsignalError(ee, context->exceptionCb, 
			   "%s loading class \"%s\"",
			   context->exceptionMsg, classname);
	    if (freeExceptionMsg) {
		free(context->exceptionMsg);
	    }
	}
        goto doCleanup;
    }

    cb = CVMallocClassSpace(ee, context, isRedefine);

    /* not in ROM, no <clinit> */
    CVMassert(!CVMcbIsInROM(cb));
    CVMassert(!CVMcbHasStaticsOrClinit(cb));

    /* 
     * Create the Java side "mirror" to this class
     */
    context->oldClassRoot = CVMclassCreateJavaInstance(ee, cb, loader);
    if (context->oldClassRoot == NULL) {
	CVMexceptionHandler(ee, context);
    }
    /*
     * This assignment is just temporary until CVMclassAddToLoadedClasses()
     * creates a new ClassLoader global root for the class loader.
     */
    CVMcbClassLoader(cb) = loader;

    if (context->size.main.excs + context->size.clinit.excs == 0) {
	CVMcbCheckedExceptions(cb) = NULL;
    } else {
	CVMcbCheckedExceptions(cb) = (CVMUint16*)
	    (context->mainSpace + context->offset.main.excs);
    }
    if (!isRedefine) {
	CVMcbStatics(cb) = (CVMJavaVal32*) 
	    (context->mainSpace + context->offset.staticFields);
	CVMcbNumStaticRefs(cb) = context->size.staticRefFields;
    }

    /* Set up the context */
    context->ptr = buffer;
    context->end_ptr = buffer + bufferLength;
    context->cb = cb;

    get4bytes(context); /* magic number */

    context->minor_version = get2bytes(context);
    context->major_version = get2bytes(context);

    cb->major_version = context->major_version;
    cb->minor_version = context->minor_version;

    CVMreadConstantPool(ee, context);
    cp = context->cp;
    utf8Cp = context->utf8Cp;

    {
	CVMUint16 accessFlags = 
	    get2bytes(context) & JVM_RECOGNIZED_CLASS_MODIFIERS;

	CVMassert(JVM_ACC_PUBLIC     == CVM_CLASS_ACC_PUBLIC);
	CVMassert(JVM_ACC_FINAL      == CVM_CLASS_ACC_FINAL);
	CVMassert(JVM_ACC_SUPER      == CVM_CLASS_ACC_SUPER);
	CVMassert(JVM_ACC_INTERFACE  == CVM_CLASS_ACC_INTERFACE);
	CVMassert(JVM_ACC_ABSTRACT   == CVM_CLASS_ACC_ABSTRACT);
	CVMassert(JVM_ACC_SYNTHETIC  == CVM_CLASS_ACC_SYNTHETIC);
	CVMassert(JVM_ACC_ANNOTATION == CVM_CLASS_ACC_ANNOTATION);
	CVMassert(JVM_ACC_ENUM       == CVM_CLASS_ACC_ENUM);

	if (accessFlags & JVM_ACC_INTERFACE) {
#ifdef CVM_DUAL_STACK
            if (!isMidletClass) {
#endif
                /* This is a workaround for a javac bug. Interfaces are not
                 * always marked as ABSTRACT.
                 */
                accessFlags |= CVM_CLASS_ACC_ABSTRACT;
#ifdef CVM_DUAL_STACK
            }
#endif
	}
	CVMcbAccessFlags(cb) = accessFlags;
    }

    /* Get the name of the class */
    cpIdx = get2bytes(context);	/* idx in constant pool of class typeID */
    CVMcbClassName(cb) = 
	CVMtypeidCloneClassID(ee, CVMcpGetClassTypeID(cp, cpIdx));

#ifdef CVM_DEBUG
    /* Make a good effort at remembering the class name */
    if (classname != NULL) {
	CVMcbClassNameString(cb) = strdup(classname);
    }
#endif

    /* Get the super class name. */
    cpIdx = get2bytes(context);	/* idx in constant pool of superclass typeID */
    if (cpIdx > 0) {
	CVMcbSuperclassTypeID(cb) = CVMcpGetClassTypeID(cp, cpIdx);
    }

    /* Get the implemented interfaces.*/
    i = get2bytes(context);
    if (i == 0) {
	CVMcbInterfaces(cb) = NULL;
    } else {
	CVMcbInterfaces(cb) = (CVMInterfaces*)(context->intfSpace);
	CVMcbSetImplementsCount(cb, i);
	for (j = 0; j < i; j++) {
	    cpIdx = get2bytes(context);
	    CVMcbInterfaceTypeID(cb, j) = CVMcpGetClassTypeID(cp, cpIdx);
	}
    }

    /* Get the fields */
    CVMcbFieldCount(cb) = get2bytes(context);
    if (CVMcbFieldCount(cb)  > 0) {
        CVMcbFields(cb) = (CVMFieldArray*)
	    (context->mainSpace + context->offset.fields);
	i = 0;
	while (i < CVMcbFieldCount(cb)) {
	    CVMFieldBlock* fb = CVMcbFieldSlot(cb, i);
	    CVMUint16 accessFlags, newAccessFlags;
	    CVMUtf8* fieldname;
	    CVMUtf8* fieldsig;

	    CVMfbIndex(fb) = i % 256; /* must be done before setting cb */
	    CVMfbClassBlock(fb) = cb;

	    /*
	     * Our internal flags do not always map one-to-one with class
	     * file flags, although in the case of field access flags they
	     * do. Make sure the following invariants are met.
	     */
	    CVMassert(JVM_ACC_STATIC    == CVM_FIELD_ACC_STATIC);
	    CVMassert(JVM_ACC_FINAL     == CVM_FIELD_ACC_FINAL);
	    CVMassert(JVM_ACC_VOLATILE  == CVM_FIELD_ACC_VOLATILE);
	    CVMassert(JVM_ACC_TRANSIENT == CVM_FIELD_ACC_TRANSIENT);

	    accessFlags = get2bytes(context) & JVM_RECOGNIZED_FIELD_MODIFIERS;

	    newAccessFlags = accessFlags & 
		~(JVM_ACC_PUBLIC | JVM_ACC_PRIVATE | JVM_ACC_PROTECTED |
		  JVM_ACC_SYNTHETIC | JVM_ACC_ENUM);

	    /* In the event that more than one of the private, protected, and
	       public flags are set (which is not allowed by the VM spec), we
	       always set it to private to be conservative.  This can only
	       occur with unverified code that isn't conforming to the spec.
	    */
#undef JVM_PPP_MASK
#define JVM_PPP_MASK  (JVM_ACC_PUBLIC |JVM_ACC_PROTECTED | JVM_ACC_PRIVATE)
	    if ((accessFlags & JVM_PPP_MASK) == JVM_ACC_PUBLIC) {
		/* Is public. */
		newAccessFlags |= CVM_FIELD_ACC_PUBLIC;
	    } else if ((accessFlags & JVM_PPP_MASK) == JVM_ACC_PROTECTED) {
		/* Is protected. */
		newAccessFlags |= CVM_FIELD_ACC_PROTECTED;
	    } else if ((accessFlags & JVM_PPP_MASK) == 0) {
		/* Default access.  Don't set any bits. */
	    } else {
		/* Is private or illegal.  Either way, treat as private. */
		newAccessFlags |= CVM_FIELD_ACC_PRIVATE;
	    }

	    if (accessFlags & JVM_ACC_SYNTHETIC) {
		newAccessFlags |= CVM_FIELD_ACC_SYNTHETIC;
	    }
	    if (accessFlags & JVM_ACC_ENUM) {
		newAccessFlags |= CVM_FIELD_ACC_ENUM;
	    }
	    CVMassert(newAccessFlags <= 255); /* must fit in one byte */
	    CVMfbAccessFlags(fb) = newAccessFlags;

	    cpIdx = get2bytes(context);
	    fieldname = CVMcpGetUtf8(utf8Cp, cpIdx);
	    cpIdx = get2bytes(context);
	    fieldsig  = CVMcpGetUtf8(utf8Cp, cpIdx);
	    CVMfbNameAndTypeID(fb) = 
		CVMtypeidNewFieldIDFromNameAndSig(ee, fieldname, fieldsig);
	    if (CVMfbNameAndTypeID(fb) == CVM_TYPEID_ERROR) {
		CVMexceptionHandler(ee, context);
	    }

	    if (CVMfbIs(fb, STATIC)) {
		/* If the field is static then set its offset in the class'
		 * array of statics and make room for the next static.
		 * 
		 * In addition, set HAS_STATICS_OR_CLINIT flag which means
		 * "the class has <clinit> and/or at least one static field".
		 * We take care of detection of <clinit> later.
		 */
		CVMcbSetHasStaticsOrClinit(cb, ee);
	    } else {
		CVMUint32 fieldSize;
		/* The field is not static. We use CVMcbInstanceSize() to keep
		 * track of the number of non-static fields this class has.
		 * CVMclassLink() will use this information and then change
		 * CVMcbInstanceSize() to the proper value. This saves
		 * CVMclassLink() from having to traverse the fields twice.
		 */
		if (CVMtypeidFieldIsDoubleword(CVMfbNameAndTypeID(fb))) {
		    fieldSize = 2;
		} else {
		    fieldSize = 1;
		}
		if (CVMcbInstanceSize(cb) + fieldSize >
		    CVM_LIMIT_OBJECT_NUMFIELDWORDS) {
		    CVMlimitHandler(ee, context, 
				    "Exceeding the 64KB object size limit");
		}
		CVMcbInstanceSize(cb) += fieldSize;
	    }

	    /* Read in each field attribute. */
	    attributeCount = get2bytes(context);
	    for (j = 0; j < attributeCount; j++) {
		CVMUtf8*  attrName;
		CVMUint32 attrLength;
		cpIdx = get2bytes(context);
		attrName = CVMcpGetUtf8(utf8Cp, cpIdx);
		attrLength = get4bytes(context);
		if (CVMfbIs(fb, STATIC) 
		    && !strcmp(attrName, "ConstantValue")) {
		    /* We store the cpIdx of the ConstantValue attribute
		     * in the CVMfbOffset() so CVMclassLink() can grab and
		     * use it to initialize the static field.
		     * If we tried to initialize a static ref field here,
		     * then gc would not see it if a gc happens before
		     * the class creating was done because it's not 
		     * reachable yet.
		     */
		    CVMfbOffset(fb) = get2bytes(context);
		} else {
		    /* skip over any other attributes */
		    getNbytes(context, attrLength, NULL);
		}
	    }
		
	    i++; /* next fb */
	}
    }

    /* Get the methods */
    CVMcbMethodCount(cb) = get2bytes(context);
    if (CVMcbMethodCount(cb) > 0) {
	CVMCheckedExceptions* checkedExceptions;
	CVMUint16* tmpCheckedExceptions = CVMcbCheckedExceptions(cb);
	if (tmpCheckedExceptions != NULL) {
	    tmpCheckedExceptions[0] = 0; /* dummy first entry is always 0 */
	    tmpCheckedExceptions++; 
	}
	checkedExceptions = (CVMCheckedExceptions*)tmpCheckedExceptions;

	CVMcbMethods(cb) = (CVMMethodArray*)
	    (context->mainSpace + context->offset.methods);
	i = 0;
	while (i < CVMcbMethodCount(cb)) {
	    CVMMethodBlock* mb = CVMcbMethodSlot(cb, i);
	    CVMUint16 accessFlags, newAccessFlags;
	    CVMUtf8* methodName;
	    CVMUtf8* methodSig;
	    CVMBool  isStrict = CVM_FALSE;

	    /*
	     * Our internal flags do not map one-to-one with class file flags.
	     * We need to remap flags that have values greater than 0x80.
	     */
	    CVMassert(JVM_ACC_STATIC        == CVM_METHOD_ACC_STATIC);
	    CVMassert(JVM_ACC_FINAL         == CVM_METHOD_ACC_FINAL);
	    CVMassert(JVM_ACC_SYNCHRONIZED  == CVM_METHOD_ACC_SYNCHRONIZED);
	    CVMassert(JVM_ACC_BRIDGE        == CVM_METHOD_ACC_BRIDGE);
	    CVMassert(JVM_ACC_VARARGS       == CVM_METHOD_ACC_VARARGS);
	    CVMassert(JVM_ACC_NATIVE        == CVM_METHOD_ACC_NATIVE);
	    CVMassert(JVM_ACC_ABSTRACT      == CVM_METHOD_ACC_ABSTRACT);

	    accessFlags = get2bytes(context) & JVM_RECOGNIZED_METHOD_MODIFIERS;

	    newAccessFlags = accessFlags & 
		~(JVM_ACC_PUBLIC | JVM_ACC_PRIVATE | JVM_ACC_PROTECTED |
		  JVM_ACC_SYNTHETIC | JVM_ACC_STRICT);

	    /* In the event that more than one of the private, protected, and
	       public flags are set (which is not allowed by the VM spec), we
	       always set it to private to be conservative.  This can only
	       occur with unverified code that isn't conforming to the spec.
	    */
#undef JVM_PPP_MASK
#define JVM_PPP_MASK  (JVM_ACC_PUBLIC |JVM_ACC_PROTECTED | JVM_ACC_PRIVATE)
	    if ((accessFlags & JVM_PPP_MASK) == JVM_ACC_PUBLIC) {
		/* Is public. */
		newAccessFlags |= CVM_FIELD_ACC_PUBLIC;
	    } else if ((accessFlags & JVM_PPP_MASK) == JVM_ACC_PROTECTED) {
		/* Is protected. */
		newAccessFlags |= CVM_FIELD_ACC_PROTECTED;
	    } else if ((accessFlags & JVM_PPP_MASK) == 0) {
		/* Default access.  Don't set any bits. */
	    } else {
		/* Is private or illegal.  Either way, treat as private. */
		newAccessFlags |= CVM_FIELD_ACC_PRIVATE;
	    }

	    if (accessFlags & JVM_ACC_SYNTHETIC) {
		newAccessFlags |= CVM_METHOD_ACC_SYNTHETIC;
	    }
	    if (accessFlags & JVM_ACC_STRICT) {
		newAccessFlags |= CVM_METHOD_ACC_STRICT;
		isStrict = CVM_TRUE;
	    }
	    CVMmbSetAccessFlags(mb, newAccessFlags);

	    CVMmbMethodIndex(mb) = i % 256;/* must be done before setting cb */
	    CVMmbClassBlock(mb) = cb;
	    CVMmbClassBlockInRange(mb) = cb;

	    if (CVMmbIs(mb, ABSTRACT) && CVMcbIs(cb, INTERFACE)) {
		/* must be done after setting cb and accessFlags */
		CVMmbMethodSlotIndex(mb) = i;
	    }
	    CVMmbMethodTableIndex(mb) = CVM_LIMIT_NUM_METHODTABLE_ENTRIES;
#ifdef CVM_JIT
	    /* All methods get this at first */
	    CVMmbJitInvoker(mb) = (void*)CVMCCMletInterpreterDoInvoke;
#endif

	    cpIdx = get2bytes(context);
	    methodName = CVMcpGetUtf8(utf8Cp, cpIdx);
	    cpIdx = get2bytes(context);
	    methodSig  = CVMcpGetUtf8(utf8Cp, cpIdx);
	    CVMmbNameAndTypeID(mb) = 
		CVMtypeidNewMethodIDFromNameAndSig(ee, methodName, methodSig);
	    if (CVMmbNameAndTypeID(mb) == CVM_TYPEID_ERROR) {
		CVMexceptionHandler(ee, context);
	    }
	    
	    if (CVMtypeidIsStaticInitializer(CVMmbNameAndTypeID(mb))) {
		/* The VM ignores the access flags of <clinit>. We reset the
		 * access flags to avoid future errors in the VM */
		CVMmbSetAccessFlags(mb, CVM_METHOD_ACC_STATIC);
		context->in_clinit = CVM_TRUE;

		/* Here we just set the HAS_STATICS_OR_CLINIT flag 
		 * if this class has <clinit>.  
		 * Later on during class linking, we propagate
		 * the flag from the super classes so that the flag gets
		 * set if any of the super classes has <clinit>. */
		CVMcbSetHasStaticsOrClinit(cb, ee);
		/* not in ROM, has <clinit> */
		CVMassert(!CVMcbIsInROM(cb));
	    }
	    
	    CVMmbArgsSize(mb) = CVMtypeidGetArgsSize(CVMmbNameAndTypeID(mb)) 
		+ (CVMmbIs(mb, STATIC) ? 0 : 1);
	    attributeCount = get2bytes(context);
	    for (j = 0; j < attributeCount; j++) {
		CVMUtf8* attrName;
		CVMUint32 attrLength;
		cpIdx = get2bytes(context);
		attrLength = get4bytes(context);
		attrName = CVMcpGetUtf8(utf8Cp, cpIdx);
		if (!strcmp(attrName, "Code") && CVMmbIsJava(mb)) {
		    CVMreadCode(ee, context, mb, isStrict);
		} else if (!strcmp(attrName, "Exceptions")) {
		    checkedExceptions = 
			CVMreadCheckedExceptions(context, mb, 
						 checkedExceptions);
		} else {
		    /* skip over any other attributes */
		    getNbytes(context, attrLength, NULL);
		}
	    }
	    context->in_clinit = CVM_FALSE;
	    i++; /* next mb */
	}
    }

    /* See if there are class attributes */
    attributeCount = get2bytes(context); 
    for (j = 0; j < attributeCount; j++) {
	CVMUtf8*  attrName;
	CVMUint32 attrLength;
	cpIdx = get2bytes(context);
	attrName = CVMcpGetUtf8(utf8Cp, cpIdx);
	attrLength = get4bytes(context);
#ifdef CVM_DEBUG_CLASSINFO
	if (!strcmp(attrName, "SourceFile")) {
	    CVMUtf8* sourceFileName;
	    cpIdx = get2bytes(context);
	    sourceFileName = CVMcpGetUtf8(utf8Cp, cpIdx);
	    CVMcbSourceFileName(cb) = strdup(sourceFileName);
	} else 
#endif
	if (!strcmp(attrName, "InnerClasses")) {
	    int k;
	    CVMcbInnerClassesInfo(cb) = (CVMInnerClassesInfo *)
		(context->mainSpace + context->offset.innerclasses);
	    CVMcbInnerClassesInfo(cb)->count = get2bytes(context);

	    for (k = 0; k < CVMcbInnerClassesInfoCount(cb); k++) {
	        CVMUint16 accessFlags;
		CVMInnerClassInfo* info = CVMcbInnerClassInfo(cb, k);
		info->innerClassIndex = get2bytes(context);
		info->outerClassIndex = get2bytes(context);
		/* innerNameIndex not supported in CVM
		   info->innerNameIndex = get2bytes(context);
		*/
		get2bytes(context);
 		accessFlags = get2bytes(context) &
		    (JVM_RECOGNIZED_CLASS_MODIFIERS | 
		     JVM_ACC_PRIVATE | JVM_ACC_PROTECTED | JVM_ACC_STATIC);
#ifdef CVM_DUAL_STACK
                if (!isMidletClass) {
#endif
                    /* This is a workaround for a javac bug. Interfaces are
                     * not always marked as ABSTRACT.
                     */
                    if (accessFlags & JVM_ACC_INTERFACE) {
                        accessFlags |= CVM_CLASS_ACC_ABSTRACT;
                    }
#ifdef CVM_DUAL_STACK
                }
#endif
		info->innerClassAccessFlags = accessFlags;
	    }
	} else { /* unkown attribute */
	    getNbytes(context, attrLength, NULL);
	}
    }

    /*
     * Free up the utf8 constant pool and utf8 strings since we don't
     * need them anymore.
     */
    CVMfreeUtf8Memory(context);

    /* Add package information */
    if (dirNameOrZipFileName != NULL) {
	if (CVMpackagesGetEntry(classname) == NULL) {
	    if (!CVMpackagesAddEntry(classname, dirNameOrZipFileName)) {
		CVMoutOfMemoryHandler(ee, context);
	    }
	}
    }

#ifdef CVM_JVMTI
    /*
     * In JVMTI case, we check to see if we are redefining a class.
     * If so, we don't add it to the class lists.  Caller of
     * this function will do the housekeeping instead.
     */
    if (!isRedefine)
#endif
    {
	/*
	 * Add the class to our class database and let the class loader know 
	 * about the class.
	 */
	if (!CVMclassAddToLoadedClasses(ee, cb, context->oldClassRoot)) {
	    CVMID_freeGlobalRoot(ee, context->oldClassRoot);
	    context->oldClassRoot = NULL;
	    /*
	     * If the new Class global root was never setup then we are
	     * responsible for freeing the class.
	     */
	    if (CVMcbJavaInstance(cb) == NULL) {
		CVMexceptionHandler(ee, context);
	    } else {
		goto doCleanup; /* exception already thrown */
	    }
	}
    }
    /* 
     * The class is loaded and verified to be structually correct.
     * From this point on the responsibility of freeing malloc'ed
     * memory will be on CVMclassFree().
     *
     * If any class error occurred before this, the control
     * flow would have been caught by setjmp and the function would
     * have jumped to (via a goto) doCleanup below:
     */

    CVMtraceClassLoading(("CL: Created cb=0x%x for class %s\n",
			  cb, classname));
#ifdef CVM_JVMPI
    if (bufferWasReplaced) {
        free(buffer);
    }
#endif
#ifdef CVM_JVMTI
    if (jvmtiBufferWasReplaced) {
	CVMjvmtiDeallocate(newBuffer);
    }
#endif
    return context->oldClassRoot;

    /* This is where the above code jumps to clean-up before exiting: */
doCleanup:
#ifdef CVM_JVMPI
    if (bufferWasReplaced) {
        free(buffer);
    }
#endif
#ifdef CVM_JVMTI
    if (jvmtiBufferWasReplaced) {
	CVMjvmtiDeallocate(newBuffer);
    }
#endif
    return NULL;
}

static CVMUint8
get1byte(CICcontext *context)
{
    const CVMUint8* ptr = context->ptr;
    CVMassert(context->end_ptr - ptr >= 1);
    (context->ptr) += 1;
    return ptr[0];
}

static CVMUint16 
get2bytes(CICcontext *context)
{
    const CVMUint8* ptr = context->ptr;
    CVMassert(context->end_ptr - ptr >= 2);
    (context->ptr) += 2;
    return (ptr[0] << 8) + ptr[1];
}

static CVMUint32
get4bytes(CICcontext *context)
{
    const CVMUint8* ptr = context->ptr;
    CVMassert(context->end_ptr - ptr >= 4);
    (context->ptr) += 4;
    return (ptr[0] << 24) + (ptr[1] << 16) + (ptr[2] << 8) + ptr[3];
}

static void 
getNbytes(CICcontext* context, size_t count, CVMUint8* buffer) 
{
    const CVMUint8* ptr = context->ptr;
    CVMassert(context->end_ptr - ptr >= count);
    if (buffer != NULL)
	memcpy(buffer, ptr, count);
    (context->ptr) += count;
}

static CVMUtf8*
getUTF8String(CVMExecEnv* ee, CICcontext* context)
{
    CVMUtf8*  utf8;
    CVMUint16 length = get2bytes(context);
    utf8 = (CVMUtf8*)malloc(length + sizeof(CVMUtf8));
    if (utf8 == NULL) {
	CVMoutOfMemoryHandler(ee, context);
    }
    getNbytes(context, length, (CVMUint8*)utf8);
    utf8[length] = '\0';
    return utf8;
}

#ifdef CVM_DEBUG_CLASSINFO

static void mark(CICcontext* context)
{
    context->mark = context->ptr;
}

static void rewindToMark(CICcontext* context)
{
    context->ptr = context->mark;
}

#endif

static void
CVMexceptionHandler(CVMExecEnv* ee, CICcontext *context)
{
    CVMID_objectGetClass(ee, CVMlocalExceptionICell(ee), context->exceptionCb);
    context->exceptionMsg = NULL;
    longjmp(context->jump_buffer, 1);
}

static void
CVMoutOfMemoryHandler(CVMExecEnv* ee, CICcontext *context)
{
    context->exceptionCb = CVMsystemClass(java_lang_OutOfMemoryError);
    context->exceptionMsg = NULL;
    longjmp(context->jump_buffer, 1);
}

static void
CVMlimitHandler(CVMExecEnv* ee, CICcontext *context, char* comment)
{
#ifdef JAVASE
    context->exceptionCb = CVMsystemClass(java_lang_OutOfMemoryError);
#else
    context->exceptionCb = CVMsystemClass(java_lang_InternalError);
#endif
    context->exceptionMsg = comment;
    longjmp(context->jump_buffer, 1);
}

static CVMClassBlock*
CVMallocClassSpace(CVMExecEnv* ee, CICcontext *context, CVMBool isRedefine)
{
    unsigned int offset = 0;
    int numberOfJmd;
 
   /* 
     * CVMClassBlock.interfaces - this is a temporary allocation until
     * the class is linked and a new CVMInterfaces is allocated that is
     * big enough to hold inherited interfaces and the methodTableIndices
     * arrays.
     */
    context->offset.interfaces = offset;
    if (context->size.interfaces > 0) {
	offset += CVMoffsetof(CVMInterfaces, itable) + 
	          (sizeof(CVMInterfaceTable) * context->size.interfaces);

	/* allocate the intfSpace */
	context->intfSpace = (CVMUint8 *)calloc(offset, 1);
	if (context->intfSpace == NULL) {
	    CVMoutOfMemoryHandler(ee, context);
	}
    }

    offset = 0;

    /* 
     * Allocate the utf8 constant pool:
     *
     * Most, if not all, java compilers put all the utf8 entries at 
     * the end of the constant pool. We don't need any utf8 entries 
     * after the class has been loaded, since they are all converted
     * to typeid's, interned Strings, or duplicated in the case of the
     * source file name. For this reason we can reduce the size of the
     * constant pool by the number of utf8 entries at the end of the
     * constant pool.
     *
     * Our strategy is to maintain 2 constant pools. One is a temporary 
     * one that is the size of the full constant pool, but is only used to
     * track utf8 entries. It is freed (along with all utf8 strings) after
     * the class is loaded.
     *
     * The other constant pool is the one we will keep around. Its size
     * is reduced by the number of utf8 entries at the end of the constant
     * pool. Note that we don't do any work to remove utf8 entries that
     * may be in the middle of the constant pool, but the utf8 strings
     * the entries point to will still be freed. Removing these entries
     * would require rewriting the bytecodes.
     *
     * context->size.constants is the total size of the constant pool.
     * context->size.reducedConstants is the reduced size of the constant
     * pool. It is the index of the last non-utf8 entry in the constant pool
     * plus 1.
     */
    context->offset.constants = offset;
    offset += ALIGN_NATURAL((sizeof(CVMConstantPoolEntry) + sizeof(CVMUint8))
			    * context->size.constants);
    context->utf8Cp = (CVMConstantPool*)calloc(offset, 1);
    if (context->utf8Cp == NULL) {
        CVMoutOfMemoryHandler(ee, context);
    }
    CVMcpTypes(context->utf8Cp) = (CVMConstantPoolEntryType*)
	((CVMConstantPoolEntry*)context->utf8Cp + context->size.constants);

    offset = 0;

    /* CVMClassBlock */
    offset += sizeof(CVMClassBlock);


    /* inner classes */
    if (context->size.innerclasses != 0) {
	context->offset.innerclasses = offset;
	/*
	 * Make sure that the CVMConstantPoolEntries are aligned correctly
	 * irrespective of the size of CVMInnerClassesInfo.
	 */
	offset += ALIGN_NATURAL(sizeof(CVMInnerClassesInfo) +
	    (sizeof(CVMInnerClassInfo) * (context->size.innerclasses - 1)));
    }

    /* CVMClassBlock.constantpool */
    context->offset.reducedConstants = offset;
    offset += ALIGN_NATURAL((sizeof(CVMConstantPoolEntry) + sizeof(CVMUint8))
			    * context->size.reducedConstants);

    /* CVMClassBlock.methods - in addition to leaving room for CVMMethodBlocks,
     * we also need room for one CVMClassBlock* every 256 entries. */
    /* 
     * Take padding of CVMMethodRange structure into acccount.
     */
    context->offset.methods = ALIGN_UP(offset, CVM_CLASSBLOCK_OFFSET_IN_METHODRANGE);
    offset += sizeof(CVMClassBlock*) * ((255 + context->size.methods) / 256) +
	      sizeof(CVMMethodBlock) * context->size.methods;

    /* CVMClassBlock.fieldsX - in addition to leaving room for CVMFieldBlocks,
     * we also need room for one CVMClassBlock* every 256 entries. */
    /*
     * Take padding of CVMFieldRange into acccount.
     */
    context->offset.fields = ALIGN_UP(offset, CVM_CLASSBLOCK_OFFSET_IN_FIELDRANGE);
    offset += sizeof(CVMClassBlock*) * ((255 + context->size.fields) / 256) +
	      sizeof(CVMFieldBlock) * context->size.fields;
    
    if (!isRedefine) {
	/* CVMClassBlock.statics */
	context->offset.staticFields = offset;
	/*
	 * Just use the same type as multiplier here that is used
	 * for 'statics' in the union 'staticsX' in CVMClassBlock.
	 */
	offset += sizeof(CVMJavaVal32) * context->size.staticFields;
    }
    /* CVMClassBlock.checkedExceptions */
    context->offset.main.excs = offset;
    offset += (sizeof(CVMUint16) * 
	       (context->size.main.excs + context->size.clinit.excs) +
	       sizeof(CVMUint16) * context->size.methods);
    /* Make room for the dummy entry 0 at the beginning of the array */
    if (context->size.main.excs != 0 || context->size.clinit.excs != 0) {
	offset += sizeof(CVMUint16);
    }

    /* allocate the main space */
    context->mainSpace = (CVMUint8 *)calloc(offset, 1);
    if (context->mainSpace == NULL) {
        CVMoutOfMemoryHandler(ee, context);
    }

    offset = 0;

    /* CVMJavaMethodDescriptors */
    CVMassert((context->size.main.code & 3) == 0);
    numberOfJmd = context->size.javamethods;
    /*
     * We do allocation for <clinit> separately later if there is one.
     * context->size.javamethods is the total number of methods
     * within the class, including <clinit>.
     */
    if (context->size.clinit.code != 0) {
       numberOfJmd --;
    }
    if (numberOfJmd > 0) {
	offset += sizeof(CVMJavaMethodDescriptor) * numberOfJmd +
	          context->size.main.code +
	          sizeof(CVMExceptionHandler) * context->size.main.etab;
#ifdef CVM_DEBUG_CLASSINFO
	offset += sizeof(CVMLineNumberEntry) * context->size.main.lnum +
	          sizeof(CVMLocalVariableEntry) * context->size.main.lvar;
#endif

	/* 
	 * Since we don't know exactly how much space we'll need to
	 * align up the several CVMJMDMutableData (one for each Java
	 * method) we just accomodate to the worst case for now.
	 */
	offset += numberOfJmd * (sizeof(void *) - 1);

	/* allocate the jmd space */
	context->nextJmdSpace = context->jmdSpace = (CVMUint8 *)calloc(offset, 1);
	if (context->jmdSpace == NULL) {
	    CVMoutOfMemoryHandler(ee, context);
	}
#ifdef CVM_DEBUG
	context->jmdSpaceLimit = context->jmdSpace + offset;
#endif

	offset = 0;
    }

    /* <clinit> CVMJavaMethodDescriptor, only if needed */
    if (context->size.clinit.code == 0) {
        CVMassert(context->size.clinit.etab == 0);
	/* Nothing more to do. Return. */
	return (CVMClassBlock*)context->mainSpace;
    }

    /* <clinit> CVMJavaMethodDescriptor */
    CVMassert((context->size.clinit.code & 3) == 0);
    context->offset.clinit.code = offset;
    offset += sizeof(CVMJavaMethodDescriptor) + context->size.clinit.code +
	      sizeof(CVMExceptionHandler) * context->size.clinit.etab;
#ifdef CVM_DEBUG_CLASSINFO
    offset += (sizeof(CVMLineNumberEntry) * context->size.clinit.lnum +
	       sizeof(CVMLocalVariableEntry) * context->size.clinit.lvar);
#endif

    /*
     * Make sure that we have enough space to align up CVMJMDMutableData.
     */
    offset = ALIGN_NATURAL(offset);

    /* allocate the <clinit> jmd space */
    context->clinitJmdSpace = (CVMUint8 *)calloc(offset, 1);
    if (context->clinitJmdSpace == NULL) {
        CVMoutOfMemoryHandler(ee, context);
    }
#ifdef CVM_DEBUG
    context->clinitJmdSpaceLimit = context->clinitJmdSpace + offset;
#endif

    return (CVMClassBlock*)context->mainSpace;
}

static void 
CVMreadConstantPool(CVMExecEnv* ee, CICcontext *context) 
{
    CVMClassBlock*   cb = context->cb;
    CVMConstantPool* cp;
    CVMConstantPool* utf8Cp = context->utf8Cp;
    CVMUint16 nconstants = get2bytes(context);
    int i;

    CVMassert(nconstants == context->size.constants);

    /* setup the constant pool pointer */
    cp = (CVMConstantPool*)(context->mainSpace + 
			    context->offset.reducedConstants);
    CVMcpTypes(cp) = (CVMConstantPoolEntryType*)
	((CVMConstantPoolEntry*)cp + context->size.reducedConstants);
    context->cp = cp;
    CVMcbConstantPool(cb) = cp;
    CVMcbConstantPoolCount(cb) = context->size.reducedConstants;

#if 0
    CVMconsolePrintf("Utf8 constant pool saved %4.2f%%\n", 100 * (1 - 
		     (float)CVMcbConstantPoolCount(cb) / (float)nconstants));
#endif

    /* Read in the constant pool */
    for (i = 1; i < nconstants; i++) {
	CVMConstantPoolEntryType entryType = get1byte(context);
	switch (entryType) {
	case CVM_CONSTANT_Utf8:
	    CVMcpSetUtf8(utf8Cp, i, getUTF8String(ee, context));
	    break;
	    
	case CVM_CONSTANT_Class:
	    CVMcpSetClassUtf8Idx(cp, i, get2bytes(context));
	    break;
	case CVM_CONSTANT_String:
	    CVMcpSetStringUtf8Idx(cp, i, get2bytes(context));
	    break;
	    
	case CVM_CONSTANT_Fieldref:
	    CVMcpSetMemberRefClassIdx(cp, i, get2bytes(context), Fieldref);
	    CVMcpSetMemberRefTypeIDIdx(cp, i, get2bytes(context), Fieldref);
	    break;
	case CVM_CONSTANT_Methodref:
	    CVMcpSetMemberRefClassIdx(cp, i, get2bytes(context), Methodref);
	    CVMcpSetMemberRefTypeIDIdx(cp, i, get2bytes(context), Methodref);
	    break;
	case CVM_CONSTANT_InterfaceMethodref:
	    CVMcpSetMemberRefClassIdx(cp, i, get2bytes(context),
				      InterfaceMethodref);
	    CVMcpSetMemberRefTypeIDIdx(cp, i, get2bytes(context),
				       InterfaceMethodref);
	    break;
	    
	case CVM_CONSTANT_NameAndType:
	    CVMcpSetNameAndTypeNameUtf8Idx(cp, i, get2bytes(context));
	    CVMcpSetNameAndTypeTypeUtf8Idx(cp, i, get2bytes(context));
	    break;
	    
	case CVM_CONSTANT_Integer:
	    CVMcpSetInteger(cp, i, get4bytes(context));
	    break;
	case CVM_CONSTANT_Float: {
	    CVMJavaVal32 val32;
	    val32.i = get4bytes(context); 
	    CVMcpSetFloat(cp, i, val32.f);
	    break;
	}
	    
	case CVM_CONSTANT_Long: {
	    volatile CVMJavaVal64 value;/* The space for the 2 word constant */
#ifdef CVM_64
	    value.v[0] = (((CVMAddr)(get4bytes(context))<<32) | 
                          ((CVMAddr)(get4bytes(context))&0x0ffffffff));
	    value.v[1] = 0;
#else
#ifndef CVM_ENDIANNESS
#error CVM_ENDIANNESS undefined
#endif
#if (CVM_ENDIANNESS == CVM_BIG_ENDIAN)
	    value.v[0] = get4bytes(context);
	    value.v[1] = get4bytes(context);
#elif (CVM_ENDIANNESS == CVM_LITTLE_ENDIAN)
	    value.v[1] = get4bytes(context);
	    value.v[0] = get4bytes(context);
#else
#error unknown CVM_ENDIANNESS
#endif
#endif
	    CVMcpSetLong(cp, i, value.l);
	    i++;		/* increment i for loop, too */
	    /* Indicate that the next object in the constant pool cannot
	     * be accessed independently.    */
	    CVMcpSetResolvedEntryType(cp, i, Invalid);
	    break;
	}
	case CVM_CONSTANT_Double: {
	    volatile CVMJavaVal64 value;/* The space for the 2 word constant */
#ifdef CVM_64
	    value.v[0] = (((CVMAddr)(get4bytes(context))<<32) |
                          ((CVMAddr)(get4bytes(context))&0x0ffffffff));
	    value.v[1] = 0;
#else
#ifndef CVM_DOUBLE_ENDIANNESS
#error CVM_DOUBLE_ENDIANNESS undefined
#endif
#if (CVM_DOUBLE_ENDIANNESS == CVM_BIG_ENDIAN)
	    value.v[0] = get4bytes(context);
	    value.v[1] = get4bytes(context);
#elif (CVM_DOUBLE_ENDIANNESS == CVM_LITTLE_ENDIAN)
	    value.v[1] = get4bytes(context);
	    value.v[0] = get4bytes(context);
#else
#error unknown CVM_DOUBLE_ENDIANNESS
#endif
#endif
	    CVMcpSetDouble(cp, i, value.d);
	    i++;		/* increment i for loop, too */
	    /* Indicate that the next object in the constant pool cannot
	     * be accessed independently.    */
	    CVMcpSetResolvedEntryType(cp, i, Invalid);
	    break;
	}
	
	default:
	    CVMassert(CVM_FALSE);
	}
    }

    /* Do a second pass on the constant pool, converting all
     * CVM_CONSTANT_Strings to CVM_CONSTANT_StringICellss and
     * CVM_CONSTANT_NameAndTypes to CVM_CONSTANT_MethodTypeIDs
     * and CVM_CONSTANT_FieldTypeID. 
     */
    for (i = 1; i < CVMcbConstantPoolCount(cb); i++) {
	CVMConstantPoolEntryType entryType = CVMcpEntryType(cp, i);
	switch (entryType) {
	case CVM_CONSTANT_String: {
	    /* 
	     *Convert String entry to StringICell.
	     */
	    CVMStringICell* internedStringICell;
	    CVMUint16       stringUtf8Idx = CVMcpGetStringUtf8Idx(cp, i);
	    CVMUtf8*        stringUtf8 = CVMcpGetUtf8(utf8Cp, stringUtf8Idx);

	    /* Intern the new String object. */
	    internedStringICell = CVMinternUTF(ee, stringUtf8 );
	    if (internedStringICell == NULL) {
		CVMoutOfMemoryHandler(ee, context);
	    }

	    /* Change the StringEntry to StringICell */
	    CVMcpSetStringICell(cp, i, internedStringICell);
	    break;
	}

	case CVM_CONSTANT_Class: {
	    /*
	     * Convert Class entry to ClassTypeID.
	     */
	    CVMUint16      classUtf8Idx = CVMcpGetClassUtf8Idx(cp, i);
	    CVMUtf8*       classUtf8 = CVMcpGetUtf8(utf8Cp, classUtf8Idx);
	    CVMClassTypeID classTypeID =  
		CVMtypeidNewClassID(ee, classUtf8, (int)strlen(classUtf8) );
	    if (classTypeID == CVM_TYPEID_ERROR) {
		CVMexceptionHandler(ee, context);
	    }

	    /* Change the Class entry to a ClassTypeID entry */
	    CVMcpSetClassTypeID(cp, i, classTypeID);
	    break;
	}

	case CVM_CONSTANT_Fieldref:
	case CVM_CONSTANT_Methodref:
	case CVM_CONSTANT_InterfaceMethodref: {
	    /*
	     * Convert NameAndType entry to MethodTypeID or FieldTypeID.
	     * Note that the Memberref entry remains unchanged. Only
	     * the NameAndType entry it refers to is changed.
	     */
	    CVMUint16 nameUtf8Idx;
	    CVMUint16 typeUtf8Idx;
	    CVMUtf8*  nameUtf8;
	    CVMUtf8*  typeUtf8;
	    CVMUint16 nameAndTypeIdx = CVMcpGetMemberRefTypeIDIdx(cp, i);
	    
	    /* If the entry is no longer a NameAndType, then there's
	     * nothing to do.
	     */
	    if (!CVMcpTypeIs(cp, nameAndTypeIdx, NameAndType)) {
		break;
	    }
	    
	    nameUtf8Idx = CVMcpGetNameAndTypeNameUtf8Idx(cp, nameAndTypeIdx);
	    typeUtf8Idx = CVMcpGetNameAndTypeTypeUtf8Idx(cp, nameAndTypeIdx);
	    nameUtf8 = CVMcpGetUtf8(utf8Cp, nameUtf8Idx);
	    typeUtf8 = CVMcpGetUtf8(utf8Cp, typeUtf8Idx);

	    /* Change the NameAndType entry to a FieldTypeID or 
	     * MethodTypeID entry 
	     */
	    if (entryType == CVM_CONSTANT_Fieldref) {
		CVMFieldTypeID fieldTypeID = 
		    CVMtypeidNewFieldIDFromNameAndSig(ee, nameUtf8, typeUtf8);
		if (fieldTypeID == CVM_TYPEID_ERROR) {
		    CVMexceptionHandler(ee, context);
		}
		CVMcpSetFieldTypeID(cp, nameAndTypeIdx, fieldTypeID);
	    } else {
		CVMMethodTypeID methodTypeID = 
		    CVMtypeidNewMethodIDFromNameAndSig(ee, nameUtf8, typeUtf8);
		if (methodTypeID == CVM_TYPEID_ERROR) {
		    CVMexceptionHandler(ee, context);
		}
		CVMcpSetMethodTypeID(cp, nameAndTypeIdx, methodTypeID);
	    }
	    break;
	}

	default:
	    break;
	}
    }
}

static void
CVMreadCode(CVMExecEnv* ee, CICcontext* context, CVMMethodBlock* mb,
	    CVMBool isStrictFP)
{
    CVMJavaMethodDescriptor* jmd;
    int attributeCount;
    CVMUint32 codeLength;
    CVMUint32 maxStack;
    CVMUint32 maxLocals;
    CVMUint8* code;
    CVMExceptionHandler*   exceptionTable;
    int i;

    /* Get the maxstack, maxlocals, and codelength */
    if (context->major_version == 45 && context->minor_version <= 2) { 
	maxStack = get1byte(context);
	maxLocals = get1byte(context);
	codeLength = get2bytes(context);
    } else { 
	maxStack = get2bytes(context);
	maxLocals = get2bytes(context);
	codeLength = get4bytes(context);
    }

    /* get the pointer to the method's CVMJavaMethodDescriptor */
    if (context->in_clinit) {
	jmd = (CVMJavaMethodDescriptor*)context->clinitJmdSpace;
    } else {
	jmd = (CVMJavaMethodDescriptor*)context->nextJmdSpace;
#ifdef CVM_DEBUG
	CVMassert(context->nextJmdSpace < context->jmdSpaceLimit);
#endif	    
    }
    /* 
     * jmd has to start at an aligned address
     */
    CVMassert((CVMAddr)CVMalignAddrUp(jmd) == (CVMAddr)jmd);
    CVMmbJmd(mb) = jmd;

    /* set some fields of the jmd */
    CVMjmdMaxLocals(jmd) = maxLocals;
    CVMjmdCodeLength(jmd) = codeLength;
    CVMjmdCapacity(jmd) = maxLocals + maxStack +
	CVM_JAVAFRAME_SIZE / sizeof(CVMStackVal32);
        
    if (isStrictFP) {
	CVMjmdFlags(jmd) |= CVM_JMD_STRICT;
    }

    /* read in the code */
    code = (CVMUint8*)(jmd + 1);
    getNbytes(context, codeLength, code);

    /* read in the exception table */
    exceptionTable = CVMjmdExceptionTable(jmd);
    CVMjmdExceptionTableLength(jmd) = get2bytes(context);
    for (i = 0; i < CVMjmdExceptionTableLength(jmd) ; i++) {
	exceptionTable[i].startpc   = get2bytes(context);
	exceptionTable[i].endpc     = get2bytes(context);
	exceptionTable[i].handlerpc = get2bytes(context);
	exceptionTable[i].catchtype = get2bytes(context);
    }

    attributeCount = get2bytes(context);
#if !defined(CVM_DEBUG_CLASSINFO) && !defined(CVM_SPLIT_VERIFY)
    for (i = 0; i < attributeCount; i++) {
	get2bytes(context); /* skip the cp index */
	getNbytes(context, get4bytes(context), NULL);
    }
#else
    {
	CVMConstantPool* utf8Cp = context->utf8Cp;
#ifdef CVM_DEBUG_CLASSINFO
	CVMUint16 tableLength ;
	CVMLineNumberEntry*    ln;
	CVMLocalVariableEntry* lv;

	/*
	 * Figure out how many total line number and local variable entries
	 * there are. Unfortunately there can be more than one LineNumberTable
	 * and LocalVariableTable attribute, and we really need the totals
	 * before we start reading them in.
	 */
	mark(context);  /* so we can rewind back to this point. */
	for (i = 0; i < attributeCount; i++) {
	    CVMUint16 cpIdx = get2bytes(context);
	    CVMUtf8* attrName = CVMcpGetUtf8(utf8Cp, cpIdx);
	    CVMUint32 length = get4bytes(context);
	    if (!strcmp(attrName, "LineNumberTable")) {
		CVMjmdLineNumberTableLength(jmd) += get2bytes(context);
		length -= 2;
	    } else if (!strcmp(attrName, "LocalVariableTable")) {
		CVMjmdLocalVariableTableLength(jmd) += get2bytes(context);
		length -= 2;
	    }
	    getNbytes(context, length, NULL);
	}
	
	/*
	 * Read in each LineNumberTable and LocalVariableTable attribute. 
	 */
	ln = CVMjmdLineNumberTable(jmd);
	lv = CVMjmdLocalVariableTable(jmd);
	rewindToMark(context);
#endif

	/* 
	 * Read in split verifier tables, line number tables
	 * and local variable tables.
	 */
	for (i = 0; i < attributeCount; i++) {
	    CVMUint16 cpIdx = get2bytes(context);
	    CVMUtf8*  attrName = CVMcpGetUtf8(utf8Cp, cpIdx);
	    CVMUint32 attrLength = get4bytes(context);
#ifdef CVM_SPLIT_VERIFY
	    if (CVMglobals.splitVerify && context->needsVerify) {
		if (!strcmp(attrName, "StackMapTable")) {
		    CVMBool ok;
                    CVMtraceVerifier(("SV add StackMapTable: %C.%M\n",
                                      context->cb, mb));
		    ok = CVMsplitVerifyAddMaps(ee, context->cb, mb,
			CVMreadStackMapTable(ee, context,
			    mb, attrLength, codeLength,
			    maxLocals, maxStack));
		    if (!ok){
			CVMexceptionHandler(ee, context);
		    }
		    continue;
		}
		if (!strcmp(attrName, "StackMap")) {
		    CVMBool ok;
                    CVMtraceVerifier(("SV add StackMap: %C.%M\n",
                                      context->cb, mb));
		    ok = CVMsplitVerifyAddMaps(ee, context->cb, mb,
			CVMreadStackMaps(ee, context, attrLength, codeLength,
			    maxLocals, maxStack));
		    if (!ok){
			CVMexceptionHandler(ee, context);
		    }
		    continue;
		}
	    }
#endif
#ifdef CVM_DEBUG_CLASSINFO
	    if (!strcmp(attrName, "LineNumberTable")) {
		tableLength = get2bytes(context);
		if (tableLength > 0) {
		    CVMUint16 j;
		    for (j = 0; j < tableLength; j++, ln++) {
			ln->startpc    = get2bytes(context);
			ln->lineNumber = get2bytes(context);
		    }
		}
		continue;
	    } 
	    if (!strcmp(attrName, "LocalVariableTable")) {
		tableLength = get2bytes(context);
		if (tableLength > 0) {
		    CVMUint16 j;
		    for (j = 0; j < tableLength; j++, lv++) {
			CVMUint16 nameIdx;
			CVMUint16 typeIdx;
			CVMFieldTypeID varTypeID;
			lv->startpc = get2bytes(context);
			lv->length  = get2bytes(context);
			nameIdx     = get2bytes(context);
			typeIdx     = get2bytes(context);
			lv->index   = get2bytes(context);
			varTypeID = CVMtypeidNewFieldIDFromNameAndSig(
                            ee, CVMcpGetUtf8(utf8Cp, nameIdx),
			    CVMcpGetUtf8(utf8Cp, typeIdx));
			if (varTypeID == CVM_TYPEID_ERROR) {
			    CVMexceptionHandler(ee, context);
			}
			/* 
			 * Split typeid into name and type part.
			 */
			lv->nameID = CVMtypeidGetNamePart(varTypeID);
			lv->typeID = CVMtypeidGetTypePart(varTypeID);
		    }
		}
		continue;
	    }
#endif
	    /* none of the above */
	    getNbytes(context, attrLength, NULL);
	}
#ifdef CVM_DEBUG_CLASSINFO
	CVMassert((void*)ln <= (void*)CVMjmdLocalVariableTable(jmd));

	/* The VM Spec doesn't seem to require line number tables are 
	 * ordered. The CVMpc2lineno function in interpreter.c relies
	 * on the entries being ordered.
	 */
	qsort(CVMjmdLineNumberTable(jmd),
		  CVMjmdLineNumberTableLength(jmd),
		  sizeof(CVMLineNumberEntry),
		  CVMlineNumberCompare);
#endif /* CVM_DEBUG_CLASSINFO */
    }
#endif /* CVM_DEBUG_CLASSINFO || CVM_SPLIT_VERIFY */

    /*
     * Finish off initialization of the code for this mb
     */
    {
	CVMUint8* thisJmdEnd;

	CVMmbInvokeCostSet(mb, CVMglobals.jit.compileThreshold);

	thisJmdEnd = CVMjmdEndPtr(jmd);
#ifdef CVM_DEBUG
	/* Check before possibility of smashing memory */
	if (!context->in_clinit) {
	    CVMassert(thisJmdEnd <= context->jmdSpaceLimit);
	} else {
	    CVMassert(thisJmdEnd <= context->clinitJmdSpaceLimit);
	}
#endif	    

	if (!context->in_clinit) {
	    context->nextJmdSpace = thisJmdEnd;
	}
    }
}

/*
 * CVMlineNumberCompare - used for sorting the line number table.
 */ 
#ifdef CVM_DEBUG_CLASSINFO

static int 
CVMlineNumberCompare(const void *p1, const void *p2)
{
    CVMLineNumberEntry* ln1 = (CVMLineNumberEntry*)p1;
    CVMLineNumberEntry* ln2 = (CVMLineNumberEntry*)p2;
    return ln1->startpc - ln2->startpc;
}

#endif /* CVM_DEBUG_CLASSINFO */

/*
 * CVMreadCheckedExceptions - read in a method's checked exceptions.
 * For each class, all checked exceptions are grouped into one chunk
 * of memory pointed to by the CVMClassBlock. The pointer to the next
 * location to store checked exceptions in must be passed in the 
 * checkedExceptions argument. The location for the next method's
 * checked exceptions should be returned as the result.
 */
static CVMCheckedExceptions*
CVMreadCheckedExceptions(CICcontext *context, CVMMethodBlock* mb, 
			 CVMCheckedExceptions* checkedExceptions)
{
    CVMUint16 nexceptions = get2bytes(context);

    if (nexceptions == 0) {
	CVMmbCheckedExceptionsOffset(mb) = 0;
	return checkedExceptions;
    } else {
	int i;
	CVMmbCheckedExceptionsOffset(mb) = ((CVMUint32)
	    ((CVMUint8*)checkedExceptions - 
	     (CVMUint8*)CVMcbCheckedExceptions(CVMmbClassBlock(mb)))) / 2;
	checkedExceptions->numExceptions = nexceptions;
	for (i = 0; i < nexceptions;  i++) {
	    checkedExceptions->exceptions[i] = get2bytes(context);
	}
	return (CVMCheckedExceptions*)
	    &checkedExceptions->exceptions[nexceptions];
    }
}

#endif /* CVM_CLASSLOADING */

/*
 * Frees up the CVMFieldTypeID's in the LocalVariableTable. This function
 * is exported so it can be called for clinit methods after they are
 * excecuted (and before they get freed!).
 */
void
CVMclassFreeLocalVariableTableFieldIDs(CVMExecEnv* ee, CVMMethodBlock* mb)
{
#if defined(CVM_CLASSLOADING) && defined(CVM_DEBUG_CLASSINFO)
    if (CVMmbIsJava(mb)) {
	CVMJavaMethodDescriptor* jmd = CVMmbJmd(mb);
	if (jmd != NULL) {
	    CVMLocalVariableEntry* lv = CVMjmdLocalVariableTable(jmd);
	    int k;
	    for (k = 0; k < CVMjmdLocalVariableTableLength(jmd); k++) {
		/* 
		 * Reconstruct the typeid of the local variable from
		 * its name and type parts.  
		 */
		CVMtypeidDisposeFieldID(ee, CVMtypeidCreateTypeIDFromParts(lv->nameID, lv->typeID));
		lv++;
	    }
	}
    }
#endif
}

#ifdef CVM_CLASSLOADING

/*
 * Free some of the memory used by the class. This is code that is common
 * to both freeing a class that failed to load and freeing a class that
 * was gc'd.
 */
static void
CVMfreeCommon(CVMExecEnv*ee, CVMClassBlock* cb)
{
    CVMConstantPool* cp = CVMcbConstantPool(cb);
    int i;

    CVMassert(!CVMisArrayClass(cb));

#if CVM_SPLIT_VERIFY
    /*
     * Free up any stackmaps associated with the class.
     */
    if (CVMsplitVerifyClassHasMaps(ee, cb)) {
        CVMsplitVerifyClassDeleteMaps(cb);
    }
#endif
    
    /*
     * Free CVMcbClassName().
     */
    CVMtypeidDisposeClassID(ee, CVMcbClassName(cb));
    
    /*
     * Go through the constant pool and free up String objects and
     * typeID's we created.
     */
    for (i = 1; i < CVMcbConstantPoolCount(cb); i++) {
	CVMConstantPoolEntryType entryType = CVMcpEntryType(cp, i);
	switch (entryType) {
	case CVM_CONSTANT_StringICell: {
	    CVMStringICell* internedStringICell =
		CVMcpGetStringICell(cp, i);
	    CVMinternDecRef(ee, internedStringICell);
	    break;
	}
	case CVM_CONSTANT_ClassTypeID: {
	    CVMtypeidDisposeClassID(ee, CVMcpGetClassTypeID(cp, i));
	    break;
	}
	case CVM_CONSTANT_FieldTypeID: {
	    CVMtypeidDisposeFieldID(ee, CVMcpGetFieldTypeID(cp, i));
	}
	break;
	case CVM_CONSTANT_MethodTypeID: {
	    CVMtypeidDisposeMethodID(ee,  CVMcpGetMethodTypeID(cp, i));
	    break;
	}
	}
    }
    
    /*
     * Go through the fields and release the CVMFieldTypeIDs we created.
     */
    for (i = 0; i < CVMcbFieldCount(cb); i++) {
	CVMFieldBlock* fb = CVMcbFieldSlot(cb, i);
	if (CVMfbNameAndTypeID(fb) != 0 
	    && CVMfbNameAndTypeID(fb) != CVM_TYPEID_ERROR) {
	    CVMtypeidDisposeFieldID(ee, CVMfbNameAndTypeID(fb));
	}
    }
    
    /*
     * Go through the methods and release the CVMMethodTypeIDs
     * and CVMFieldTypeIDs we created.
     */
    for (i = 0; i < CVMcbMethodCount(cb); i++) {
	CVMMethodBlock* mb = CVMcbMethodSlot(cb, i);
	
	/* Release the methods CVMMethodTypeID. */
	if (CVMmbNameAndTypeID(mb) != 0 
	    && CVMmbNameAndTypeID(mb) != CVM_TYPEID_ERROR) {
	    CVMtypeidDisposeMethodID(ee, CVMmbNameAndTypeID(mb));
	}

	/* Release all CVMFieldTypeIDs created in the LocalVariableTable.*/
	CVMclassFreeLocalVariableTableFieldIDs(ee, mb);
    }
    
#ifdef CVM_DEBUG_CLASSINFO
    /*
     * Free the source file name.
     */
    if (CVMcbSourceFileName(cb) != NULL) {
	free(CVMcbSourceFileName(cb));
    }
#endif
}

/*
 * CVMfreeAllocatedMemory - called when an error occurs while loading
 * the class. Frees all allocated memory.
 */
static void 
CVMfreeAllocatedMemory(CVMExecEnv* ee, CICcontext* context)
{
    /*
     * Free the global root that was used to keep the Class instance live.
     */
    if (context->oldClassRoot != NULL) {
	CVMID_freeGlobalRoot(ee, context->oldClassRoot);
    }

    /*
     * Free the cb and related structures.
     */
    if (context->cb != NULL) {
	CVMfreeCommon(ee, context->cb);
    }

    /*
     * Free up utf8 related memory.
     */
    CVMfreeUtf8Memory(context);

    /*
     * Free up other malloc'd memory.
     */
    if (context->intfSpace != NULL) {
	free(context->intfSpace);
    }
    if (context->mainSpace != NULL) {
	free(context->mainSpace);
    }
    if (context->jmdSpace != NULL) {
	free(context->jmdSpace);
    }
    if (context->clinitJmdSpace != NULL) {
	free(context->clinitJmdSpace);
    }
}

/*
 * CVMfreeUtf8Memory - called when there is an error while reading the
 * class or when the class is done being loaded. Frees up all memory
 * associated with utf8 strings.
 */
static void
CVMfreeUtf8Memory(CICcontext* context) {
    CVMConstantPool* utf8Cp = context->utf8Cp;
    int i;

    if (utf8Cp == NULL) {
	return;
    }

    /* Free up all of the utf8 strings in the utf8 constant pool */
    for (i = 1; i < context->size.constants; i++) {
	if (CVMcpEntryType(utf8Cp, i) == CVM_CONSTANT_Utf8) {
	    free(CVMcpGetUtf8(utf8Cp, i));
	}
    }

    /* Free up the utf8 constant pool */
    free(context->utf8Cp);
    context->utf8Cp = NULL; /* necessary in case we get an error later */
}

#endif /* CVM_CLASSLOADING */

/*
 * Free a class that was gc'd. Called by the gc when it detects that there
 * are no references to the class.
 */
void
CVMclassFree(CVMExecEnv* ee, CVMClassBlock* cb)
{
    /* Free the gcMap if it is a big map (doesn't fit in 31 bits) */
    /*
     * CVMGCBitMap is a union of the scalar 'map' and the
     * pointer 'bigmap'. So it's best to
     * use the pointer if we need the pointer
     */
    CVMAddr gcMap = (CVMAddr)(CVMcbGcMap(cb).bigmap);
    if ((gcMap & CVM_GCMAP_FLAGS_MASK) == CVM_GCMAP_LONGGCMAP_FLAG) {
	free((char*)(gcMap & ~CVM_GCMAP_LONGGCMAP_FLAG));
    }

    /* Free the Class global root. */
    CVMassert(CVMID_icellIsNull(CVMcbJavaInstance(cb)));
    CVMID_freeClassGlobalRoot(ee, CVMcbJavaInstance(cb));

    /* Free the class table slot */
    CVMclassTableFreeSlot(ee, CVMcbClassTableSlotPtr(cb));

#ifdef CVM_DEBUG
    CVMassert(!CVMcbIsInROM(cb));
    if (CVMcbClassNameString(cb) != NULL) {
	free(CVMcbClassNameString(cb));
	CVMcbClassNameString(cb) = NULL;
    }
#endif

    /* Free the protectionDomain global root */
    if (CVMcbProtectionDomain(cb) != NULL) {
	CVMID_freeProtectionDomainGlobalRoot(ee, CVMcbProtectionDomain(cb));
    }

    /*
     * Array classes only need to have their typeid and cb freed.
     */
    if (CVMisArrayClass(cb)) {
	CVMtypeidDisposeClassID(ee, CVMcbClassName(cb));
	free(cb);
	return;
    }

#ifdef CVM_CLASSLOADING

    /* Call free code that we have in common with CVMfreeAllocatedMemory(). */
    CVMfreeCommon(ee, cb);

    /* 
     * Free the CVMcbInterfaces() memory. This used to be pointed to
     * by context->intfSpace, until it got reallocated in
     * CVMclassPrepareInterfaces(). We only free it if this class
     * declared that it implements any interfaces or if it is an
     * interface. Otherwise it belongs to the superclass.
     */
    if (!CVMcbCheckRuntimeFlag(cb, INHERITED_INTERFACES)) {
	/* 
	 * The only way CVMcbInterfaces() could be NULL is if a class with
	 * no interfaces failed to link or the class is actually an 
	 * interface and failed to link.
	 */
	if (CVMcbInterfaces(cb) != NULL) {
	    free(CVMcbInterfaces(cb)); /* reallocated context->intfSpace */
	}
    }
    
    /* 
     * Free up space occupied by the CVMJavaMethodDescriptors and
     * stackmaps.
     */
    CVMclassFreeJavaMethods(ee, cb, CVM_FALSE);

    /*
     * Free memory allocated for miranda methods. Miranda methods are
     * always last in the method table. If the last method is a
     * miranda method and belongs to this class, then it can be used
     * to find the memory we need to free. See the big "MIRANDA
     * METHODS" comment in CVMclassPrepareInterfaces() for details.
     */
    if (CVMcbMirandaMethodCount(cb) != 0) {
	/* We know the last method in the method table is a miranda method */
	CVMMethodBlock* mb = 
	    CVMcbMethodTableSlot(cb, CVMcbMethodTableCount(cb) - 1);
	
	/* Get the address of the memory allocated for the miranda methods. */
	CVMMethodRange* miranda_method_range = CVMmbMethodRange(mb);
	
	/*
	 * Free all the MethodTypeID's that were allocated for miranda methods.
	 */
	{
	    int i;
	    for (i = CVMcbMirandaMethodCount(cb); i > 0; i--) {
		CVMtypeidDisposeMethodID(ee, CVMmbNameAndTypeID(mb));
		mb--;
	    }
	}
	/*
	 * Free the memory allocated for miranda methods.
	 */
	free(miranda_method_range);
    }

    /*
     * Free the methodtable.
     */
    if (CVMcbMethodTablePtr(cb) != NULL) {
	/* Array classes use the java/lang/Object method table. Don't
	 * try to free it.
	 */
	if (CVMcbMethodTablePtr(cb) !=
	    CVMcbMethodTablePtr(CVMsystemClass(java_lang_Object))) {
	    free(CVMcbMethodTablePtr(cb));
	}
    }

    /*
     * Free the cb. This is the context->mainSpace memory.
     */
    free(cb);

#endif /* CVM_CLASSLOADING */
}

#ifdef CVM_CLASSLOADING

/*
 * Free all ClassLoader global roots for ClassLoaders that were found to be
 * unreferenced in the GC scan. There is no need to grab classTableLock
 * since anyone who has grabbed the lock must also be gcUnsafe, and if
 * another thread is gcUnsafe then we can't possibly be executing here.
 */

typedef struct CVMClassrootHandlingParams {
    CVMRefLivenessQueryFunc isLive;
    void*                   isLiveData;
    CVMExecEnv*             ee;
} CVMClassrootHandlingParams;

static void
CVMclassFreeClassLoaderRoot(CVMObject** refPtr, void* data)
{
    CVMClassrootHandlingParams* params = (CVMClassrootHandlingParams*)data;
    if (!params->isLive(refPtr, params->isLiveData)) {
	/*
	 * Put the ClassLoader root for the unreferenced ClassLoader
	 * on the list of ClassLoader roots to free since we can't
	 * safely free it here (the classTableLock is not held and we
	 * can't grab it now). We need to set the low bit of the link
	 * field so gc doesn't try to scan the root.
	 */
	*refPtr = (CVMObject*)CVMglobals.freeClassLoaderList;
	/*
	 * refPtr is casted to a native pointer (CVMClassLoaderICell*)
	 * therefore the cast has to be CVMAddr which is 4 byte on
	 * 32 bit platforms and 8 byte on 64 bit platforms
	 */
	CVMglobals.freeClassLoaderList = 
	    (CVMClassLoaderICell*)(((CVMAddr)refPtr) | 1);
    }
}

static void
CVMclassFreeUnusedClassLoaderRoots(CVMExecEnv*ee,
				   CVMRefLivenessQueryFunc isLive,
				   void* isLiveData)
{
    CVMClassrootHandlingParams params;
    params.isLive = isLive;
    params.isLiveData = isLiveData;
    params.ee = ee;

    /*
     * Scan all the roots, and add the unused ones to
     * CVMglobals.freeClassLoaderList.
     */
    CVMstackWalkAllFrames(&CVMglobals.classLoaderGlobalRoots, {
	CVMwalkRefsInGCRootFrame(frame, ((CVMFreelistFrame*)frame)->vals,
				 chunk, CVMclassFreeClassLoaderRoot, &params,
				 CVM_TRUE);
    });
}

/* 
 * CVMclassDoClassUnloading - gc calls us after marking all live
 * objects so we can start the class unloading process for any classes
 * that are no longer referenced. In this first pass, we mark all
 * loader cache, loader constraints, class table entries, class global
 * roots, and class loader global roots for unrefenced classes and
 * classloaders. We also put all cb's of unreferenced classes on a
 * free list so we can free them up in pass #2.  GC requires that the
 * cb remain intact until pass #2 because it may need some information
 * from it like the instance size.
 */
void
CVMclassDoClassUnloadingPass1(CVMExecEnv* ee,
			      CVMRefLivenessQueryFunc isLive,
			      void* isLiveData,
			      CVMRefCallbackFunc transitiveScanner,
			      void* transitiveScannerData,
			      CVMGCOptions* gcOpts)
{
    CVMloaderConstraintsMarkUnscannedClassesAndLoaders(
        ee, isLive, isLiveData);
    CVMloaderCacheMarkUnscannedClassesAndLoaders(ee, isLive, isLiveData);

    /*
     * We don't need to grab the classTableLock. Another thread may
     * already have the lock, but anyone grabbing the lock is also
     * required to become gc-unsafe and would be blocked on the
     * safe->unsafe tranistion. This means that all data protected
     * by the classTableLock is guaranteed to be in a consistent state
     * at this point. WARNING: if we try to grab the classTableLock
     * here, we may block, and that would be disastrous.
     */
    CVMclassTableMarkUnscannedClasses(ee, isLive, isLiveData); 
    CVMclassFreeUnusedClassLoaderRoots(ee, isLive, isLiveData);

    /*
     * Update all live class loader roots
     */
    if (gcOpts->isUpdatingObjectPointers) {
        CVMstackScanRoots(ee, &CVMglobals.classLoaderGlobalRoots,
		          transitiveScanner, transitiveScannerData, gcOpts);
    }
}

void
CVMclassDoClassUnloadingPass2(CVMExecEnv* ee) 
{
    CVMClassBlock* cb;
    CVMClassBlock* firstCb;
    CVMClassLoaderICell* loader;

    /* 
     * Purge all the loaderContraints marked by 
     * CVMloaderConstraintsMarkUnscannedClassesAndLoaders().
     */
    CVMloaderConstraintsPurgeUnscannedClassesAndLoaders(ee);

    /*
     * Purge all the loaderCache entries marked by
     * CVMloaderCacheMarkUnscannedClassesAndLoaders().
     */
    CVMloaderCachePurgeUnscannedClassesAndLoaders(ee);

    /* 
     * Become gcUnsafe so we can "detach" the freeClassList without fear
     * of the next round of class unloading starting up and adding
     * items to the front of the list while we try to detach it.
     * Do the same for the freeClassLoaderList.
     */
    CVMD_gcUnsafeExec(ee, {
        firstCb = CVMglobals.freeClassList;
	CVMglobals.freeClassList = NULL;
	loader = CVMglobals.freeClassLoaderList;
	CVMglobals.freeClassLoaderList = NULL;
    });

#ifdef CVM_JIT
    /*
     * Decompile methods. We can't do this in CVMclassFreeJavaMethods(),
     * because some tracing that decompiling causes requires that the
     * typeid's have not been freed yet. Also, we can't do this in 
     * CVMclassFree() because we need to decompile all the methods
     * in this group, before any memory for the classes is freed.
     * This is because the CVMcmdCallees() array for a class may
     * contain references to an mb in another class that is getting
     * unloaded, and it will be deferenced during decompilation after
     * the memory for the mb has been freed.
     */
    cb = firstCb;
    while (cb != NULL) {
	CVMtraceClassUnloading((
            "CLU: Freeing compiled methods for (cb=0x%x) %C\n", cb, cb));
	CVMclassFreeCompiledMethods(ee, cb);
	cb = CVMcbFreeClassLink(cb);
    }
#endif

    /*
     * Free all the classes that were put on the freeClassList by
     * CVMclassTableMarkUnscannedClasses(). Do the same for the 
     * ClassLoader global roots that were put on the freeClassLoaderList
     * by CVMclassFreeUnusedClassLoaderRoots(). Note that gc will release
     * all locks before calling us for pass #2.
     */
    cb = firstCb;
    while (cb != NULL) {
	CVMClassBlock* currCb = cb;
	CVMtraceClassUnloading(("CLU: Freeing class (cb=0x%x) %C\n", cb, cb));
	/* must get next cb before freeing this cb */
	cb = CVMcbFreeClassLink(cb);
	CVMclassFree(ee, currCb);
    }
    while (loader != NULL) {
	 /* 
	  * The loader root has its low bit set to 1 so gc will ignore
	  * it. However, CVMframeFree() will think that the root is already
	  * free unless we clear the bit first.
	  */ 
	/*
	 * currLoader is casted to a native pointer (CVMClassLoaderICell*)
	 * therefore the cast has to be CVMAddr which is 4 byte on
	 * 32 bit platforms and 8 byte on 64 bit platforms
	 */
	CVMClassLoaderICell* currLoader = 
	    (CVMClassLoaderICell*)(((CVMAddr)loader) &~ 1);

	loader = *(CVMClassLoaderICell**)currLoader;
	CVMID_icellSetNull(currLoader);
	CVMID_freeClassLoaderGlobalRoot(ee, currLoader);
    }
}

#ifdef CVM_SPLIT_VERIFY
/*
 * Array to map external, classfile-defined representation
 * into the internal one used by the verifier.
 */
static const fullinfo_type tagMap[] = {
    ITEM_Bogus, 	/* TOP is Really Bogus */
    ITEM_Integer,
    ITEM_Float,
    (fullinfo_type)(-1), /* DOUBLE is not translated by this table */
    (fullinfo_type)(-1), /* LONG is not translated by this table */
    NULL_FULLINFO,
    ITEM_InitObject,
    (fullinfo_type)(-1), /* OBJECT is not translated by this table */
    (fullinfo_type)(-1)  /* UNINIT_OBJECT is not translated by this table */
};

/* These are no longer used, but may be useful for debugging */
#if 0
static const char* tagNames[] = {
    "ITEM_Bogus",
    "ITEM_Integer",
    "ITEM_Float",
    "ITEM_DoubleXX",
    "ITEM_LongXX",
    "NULL_FULLINFO",
    "ITEM_InitObject",
    "ITEM_ObjectXX",
    "ITEM_UninitObjectXX"
};
#endif

/*
 * We know it is safe to do all this, as the format verifier has
 * already been run. Thus all checks are in the form of assertions.
 */
static CVMsplitVerifyMapElement*
CVMreadMapElements(
    CVMExecEnv* ee,
    CICcontext* context,
    int nElements,
    int codeLength,
    CVMsplitVerifyMapElement* elementp)
{
    CVMClassTypeID objtype;
    CVMsplitVerifyMapElement v = ITEM_Bogus;
#ifdef CVM_DEBUG_ASSERTS
    int cpLength = CVMcbConstantPoolCount(context->cb);
#endif
    while (nElements-- > 0 ){
	CVMUint32 tag = get1byte(context);
	CVMUint32 offset;
	switch (tag){
	case JVM_STACKMAP_LONG:
	    *elementp++ = ITEM_Long;
	    v = ITEM_Long_2;
	    break; /* code below will write second word */
	case JVM_STACKMAP_DOUBLE:
	    *elementp++ = ITEM_Double;
	    v = ITEM_Double_2;
	    break; /* code below will write second word */
	case JVM_STACKMAP_OBJECT:
	    offset = get2bytes(context);
	    CVMassert(offset < cpLength &&
		CVMcpTypeIs(context->cp, offset, ClassTypeID));
	    objtype = CVMcpGetClassTypeID(context->cp, offset);
	    v = VERIFY_MAKE_OBJECT(objtype);
	    break;
	case JVM_STACKMAP_UNINIT_OBJECT:
	    offset = get2bytes(context);
	    CVMassert(offset < codeLength);
	    v = VERIFY_MAKE_UNINIT(offset);
	    break;
	default:
	    CVMassert(tag < sizeof(tagMap)/sizeof(tagMap[0]));
	    v = tagMap[tag];
	    break;
	}
	CVMassert(v != (fullinfo_type)(-1));
	*elementp++ = v;
    }
    return elementp;
}

/* Read in JVMSv3-style StackMapTable attribute */
/*
 * The code in verifyformat has already ensured that the offsets and
 * constant pool references we're reading in make sense.
 */
static CVMsplitVerifyStackMaps*
CVMreadStackMapTable(CVMExecEnv* ee, CICcontext* context,
    CVMMethodBlock *mb, CVMUint32 length, CVMUint32 codeLength,
    CVMUint32 maxLocals, CVMUint32 maxStack)
{
    CVMsplitVerifyStackMaps*	maps = NULL;
    CVMsplitVerifyMap*		entry;
    CVMUint32		  	nEntries;
    CVMUint32		  	bytesPerMap;
    int				lastPC = -1;
    int				nLocalSlots = 0;
    int				nStackSlots = 0;
    CVMsplitVerifyMapElement	*locals;
    CVMsplitVerifyMapElement	*stack;
#ifdef CVM_DEBUG_ASSERTS
    const CVMUint8*	  	startOffset = context->ptr;
#endif
    /*
     * These asserts are here to remind us that the format of these
     * things changes for big methods.
     */
    CVMassert(codeLength <= 65535);
    CVMassert(maxLocals  <= 65535);
    CVMassert(maxStack   <= 65535);

    nEntries = get2bytes(context);
    bytesPerMap = sizeof(CVMsplitVerifyMap) + 
	      (sizeof(CVMsplitVerifyMapElement) * 
	      (maxLocals + maxStack - 1));
    /* Note: the below is slightly overgenerous, since it doesn't account
     * for the trivial CVMsplitVerifyMap built into the 
     * CVMsplitVerifyStackMaps
     */
    maps = (CVMsplitVerifyStackMaps*)calloc(1, 
	    sizeof(struct CVMsplitVerifyStackMaps) + 
	    bytesPerMap * nEntries);
    if (maps == NULL){
	CVMoutOfMemoryHandler(ee, context);
    }
    locals = (CVMsplitVerifyMapElement *)calloc(maxLocals, sizeof locals[0]);
    if (locals == NULL){
	free(maps);
	CVMoutOfMemoryHandler(ee, context);
    }
    stack = (CVMsplitVerifyMapElement *)calloc(maxStack, sizeof stack[0]);
    if (stack == NULL){
	free(locals);
	free(maps);
	CVMoutOfMemoryHandler(ee, context);
    }

    /* Initialize locals from args */
{
    CVMSigIterator sig;
    CVMClassTypeID cid;
    CVMtypeidGetSignatureIterator(CVMmbNameAndTypeID(mb), &sig);

    if (!CVMmbIs(mb, STATIC)) {
	nLocalSlots = 1;
	locals[0] = VERIFY_MAKE_OBJECT(CVMcbClassName(context->cb));
    }
    while ((cid = CVM_SIGNATURE_ITER_NEXT(sig))
	!= CVM_TYPEID_ENDFUNC)
    {
	CVMClassTypeID t = cid;
	if (!CVMtypeidIsPrimitive(cid)) {
	    t = CVM_TYPEID_OBJ;
	}
	switch (t) {
	case CVM_TYPEID_OBJ:
	    locals[nLocalSlots++] =
		VERIFY_MAKE_OBJECT(cid);
	    break;
	case CVM_TYPEID_BOOLEAN:
	case CVM_TYPEID_BYTE:
	case CVM_TYPEID_SHORT:
	case CVM_TYPEID_CHAR:
	case CVM_TYPEID_INT:
	    locals[nLocalSlots++] = ITEM_Integer;
	    break;
	case CVM_TYPEID_FLOAT:
	    locals[nLocalSlots++] = ITEM_Float;
	    break;
	case CVM_TYPEID_LONG:
	    locals[nLocalSlots++] = ITEM_Long;
	    locals[nLocalSlots++] = ITEM_Long_2;
	    break;
	case CVM_TYPEID_DOUBLE:
	    locals[nLocalSlots++] = ITEM_Double;
	    locals[nLocalSlots++] = ITEM_Double_2;
	    break;
	default:
	    CVMassert(CVM_FALSE);
	}
    }
}

    CVMassert(nLocalSlots <= maxLocals);
    CVMassert(nStackSlots <= maxStack);

    maps->nEntries = nEntries;
    maps->entrySize = bytesPerMap;
    entry = &(maps->maps[0]);
    while (nEntries-- > 0){
	int tag, frame_type;
	int offset_delta;
	int pc;

	tag = frame_type = get1byte(context);
	if (tag < 64) {
	    /* 0 .. 63 SAME */
	    nStackSlots = 0;
	    offset_delta = frame_type;
	    pc = lastPC + offset_delta + 1;
	} else if (tag < 128) {
	    /* 64 .. 127 SAME_LOCALS_1_STACK_ITEM */
	    offset_delta = frame_type - 64;
	    pc = lastPC + offset_delta + 1;
	    {
		CVMsplitVerifyMapElement *e =
		    CVMreadMapElements(ee, context, 1, codeLength, stack);
		nStackSlots = e - stack;
	    }
	} else if (tag == 255) {
	    /* 255 FULL_FRAME */
	    int l, s;
	    CVMsplitVerifyMapElement *e;

	    offset_delta = get2bytes(context);
	    pc = lastPC + offset_delta + 1;
	    l = get2bytes(context);
	    e = CVMreadMapElements(ee, context, l, codeLength, locals);
	    nLocalSlots = e - locals;
	    s = get2bytes(context);
	    e = CVMreadMapElements(ee, context, s, codeLength, stack);
	    nStackSlots = e - stack;
	} else if (tag >= 252) {
	    /* 252 .. 254 APPEND */
	    CVMsplitVerifyMapElement *e;
	    int k = frame_type - 251;
	    offset_delta = get2bytes(context);
	    nStackSlots = 0;
	    pc = lastPC + offset_delta + 1;
	    e = CVMreadMapElements(ee, context, k,
		codeLength, locals + nLocalSlots);
	    nLocalSlots = e - locals;
	} else if (tag == 251) {
	    /* 251 SAME_FRAME_EXTENDED */
	    offset_delta = get2bytes(context);
	    nStackSlots = 0;
	    pc = lastPC + offset_delta + 1;
	} else if (tag >= 248) {
	    /* 248 .. 250 CHOP */
	    int k = 251 - frame_type;
	    while (k-- > 0) {
		int t = locals[nLocalSlots - 1];
		if (t == ITEM_Double_2 || t == ITEM_Long_2) {
		    nLocalSlots -= 2;
		} else {
		    nLocalSlots -= 1;
		}
	    }
	    offset_delta = get2bytes(context);
	    nStackSlots = 0;
	    pc = lastPC + offset_delta + 1;
	} else if (tag == 247) {
	    /* 247 SAME_LOCALS_1_STACK_ITEM_EXTENDED */
	    CVMsplitVerifyMapElement *e;
	    offset_delta = get2bytes(context);
	    pc = lastPC + offset_delta + 1;
	    e = CVMreadMapElements(ee, context, 1, codeLength, stack);
	    nStackSlots = e - stack;
	} else {
	    pc = -1;
	    CVMassert(CVM_FALSE);
	}

	entry->pc = pc;
	CVMassert(nLocalSlots <= maxLocals);
	memcpy(entry->mapElements, locals, nLocalSlots * sizeof locals[0]);
	entry->nLocals = nLocalSlots;
	CVMassert(entry->nLocals <= maxLocals);
	entry->sp = nStackSlots;
	CVMassert(nStackSlots <= maxStack);
	memcpy(entry->mapElements + nLocalSlots, stack,
	    nStackSlots * sizeof stack[0]);
	entry = (CVMsplitVerifyMap*)((char*)entry + bytesPerMap);
	CVMassert(context->ptr <= startOffset + length);
	lastPC = pc;
    }
    CVMassert(context->ptr == startOffset + length);
    free(locals);
    free(stack);
    return maps;
}

/* Read in CLDC-style StackMap attribute */
/*
 * The code in verifyformat has already ensured that the offsets and
 * constant pool references we're reading in make sense.
 */
static CVMsplitVerifyStackMaps*
CVMreadStackMaps(CVMExecEnv* ee, CICcontext* context, CVMUint32 length,
		 CVMUint32 codeLength, CVMUint32 maxLocals, CVMUint32 maxStack)
{
    CVMsplitVerifyStackMaps*	maps = NULL;
    CVMsplitVerifyMap*		entry;
    CVMUint32		  	nEntries;
    CVMUint32		  	bytesPerMap;
#ifdef CVM_DEBUG_ASSERTS
    const CVMUint8*	  	startOffset = context->ptr;
#endif
    /*
     * These asserts are here to remind us that the format of these
     * things changes for big methods.
     */
    CVMassert(codeLength <= 65535);
    CVMassert(maxLocals  <= 65535);
    CVMassert(maxStack   <= 65535);

    nEntries = get2bytes(context);
    bytesPerMap = sizeof(CVMsplitVerifyMap) + 
	      (sizeof(CVMsplitVerifyMapElement) * 
	      (maxLocals + maxStack - 1));
    /* Note: the below is slightly overgenerous, since it doesn't account
     * for the trivial CVMsplitVerifyMap built into the 
     * CVMsplitVerifyStackMaps
     */
    maps = (CVMsplitVerifyStackMaps*)calloc(1, 
	    sizeof(struct CVMsplitVerifyStackMaps) + 
	    bytesPerMap * nEntries);
    if (maps == NULL){
	CVMoutOfMemoryHandler(ee, context);
    }
    maps->nEntries = nEntries;
    maps->entrySize = bytesPerMap;
    entry = &(maps->maps[0]);
    while (nEntries-- > 0){
	int nLocalElements;
	int nStackElements;
	CVMsplitVerifyMapElement *t1, *t2;
	entry->pc = get2bytes(context);
	nLocalElements = get2bytes(context);
	CVMassert(nLocalElements <= maxLocals);
	t1 = CVMreadMapElements(ee, context, nLocalElements, codeLength,
			    &(entry->mapElements[0]));
	entry->nLocals = t1 - &(entry->mapElements[0]);
	CVMassert(entry->nLocals <= maxLocals);
	nStackElements = get2bytes(context);
	CVMassert(nStackElements <= maxStack);
	t2 = CVMreadMapElements(ee, context, nStackElements, codeLength, 
			    t1);
	entry->sp = t2 - t1;
	CVMassert(entry->sp <= maxStack);
	entry = (CVMsplitVerifyMap*)((char*)entry + bytesPerMap);
	CVMassert(context->ptr <= startOffset + length);
    }
    CVMassert(context->ptr == startOffset + length);
    return maps;
}

#endif /* CVM_SPLIT_VERIFY */
#endif /* CVM_CLASSLOADING */

/*
 * Free up a Class' JavaMethodDescriptors and associated stack maps.
 * For dynamic classes this is called by CVMclassFree, above,
 * but the preloader needs it to deallocate dynamic memory associated
 * with preloaded classes, such as the stackmaps.
 * 
 * The jmd for
 * the <clinit> method has its own separate allocation. The rest of
 * the jmd's are in one allocation (except those re-allocated for
 * stackmap disambiguation). The first non-clinit jmd points to this
 * memory.  The jmd's separately allocated during stackmap
 * disambiguation are flagged with CVM_JMD_DID_REWRITE.
 */

void
CVMclassFreeJavaMethods(CVMExecEnv* ee, CVMClassBlock* cb, CVMBool isPreloaded)
{
    CVMJavaMethodDescriptor* firstContiguousJmd = NULL;
    int i;
    for (i = 0; i < CVMcbMethodCount(cb); i++) {
	CVMMethodBlock* mb = CVMcbMethodSlot(cb, i);
	if (CVMmbIsJava(mb)) {
	    CVMJavaMethodDescriptor* jmd = CVMmbJmd(mb);
            CVMStackMaps *smaps;
	    if (jmd == NULL) {
		/*
		 * There are two ways that the jmd can end up being NULL for
		 * a non-native and non-abstract method. The first is if
		 * the method is a <clinit> and has already been executed
		 * and freed up. Then 2nd if an illegal class file contains a 
		 * non-native and non-abstract method that does not have
		 * a Code attribute. The class format checker does not 
		 * catch this, but the code verifier does. Unfortunately
		 * the class is already created and registered by this
		 * time.
		 */
		continue;
	    }
	    if ((smaps = CVMstackmapFind(ee, mb)) != NULL) {
                CVMstackmapDestroy(ee, smaps);
                CVMassert(CVMstackmapFind(ee, mb) == NULL);
	    }
	    if (CVMtypeidIsStaticInitializer(CVMmbNameAndTypeID(mb))) {
		/* This is an un-free'd clinit.
		 * It is either unexecuted or is preloaded.
		 * And even if it was preloaded, it might have been
		 * re-written for disambiguation. Sigh.
		 */
		if (!isPreloaded || (CVMjmdFlags(jmd) & CVM_JMD_DID_REWRITE)){
		    free(jmd);
		}
	    } else {
		if (CVMjmdFlags(jmd) & CVM_JMD_DID_REWRITE) {
		    /* this is separately allocated.
		     * but it may still be the first!
		     */
		    CVMJavaMethodExtension* extension; 
		    CVMJavaMethodDescriptor* originalJmd;
		    extension = 
			(CVMJavaMethodExtension *)CVMjmdEndPtr(jmd);
		    originalJmd = extension->originalJmd;
		    free(jmd); /* always free these */
		    jmd = originalJmd; /* consider freeing this */
				       /* upon falling through ... */
		}
		if (firstContiguousJmd == NULL) {
		    firstContiguousJmd = jmd;
		}
	    }
	}
    }
    if (!isPreloaded && (firstContiguousJmd != NULL)) {
	free(firstContiguousJmd);   /* context->jmdSpace */
    }
}

#ifdef CVM_JIT
void
CVMclassFreeCompiledMethods(CVMExecEnv* ee, CVMClassBlock* cb)
{
    int i;
    CVMJITGlobalState* jgs = &CVMglobals.jit;
    for (i = 0; i < CVMcbMethodCount(cb); i++) {
	CVMMethodBlock* mb = CVMcbMethodSlot(cb, i);
	if (CVMmbIsJava(mb)) {
	    CVMJavaMethodDescriptor* jmd = CVMmbJmd(mb);
	    if (jmd != NULL && CVMmbIsCompiled(mb)) {
		/*
		 * Decompile the method. If the ee is NULL then we are
		 * shutting down and there is no need to lock the jitLock.
		 */
		if (ee != NULL) {
		    CVMsysMutexLock(ee, &CVMglobals.jitLock);
		}
		/* Now check for IsCompiled again just in case someone
		   beat us to it */
		if (CVMmbIsCompiled(mb)) {
                    CVMUint8* pc = CVMmbStartPC(mb);
                    /* don't decompile if it is an AOT method */
                    if (pc >= jgs->codeCacheStart && pc < jgs->codeCacheEnd) {
                        CVMJITdecompileMethod(ee, mb);
                    }
		}
		if (ee != NULL) {
		    CVMsysMutexUnlock(ee, &CVMglobals.jitLock);
		}
	    }
	}
    }
}
#endif
