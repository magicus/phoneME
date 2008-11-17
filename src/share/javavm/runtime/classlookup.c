/*
 * @(#)classlookup.c	1.91 06/10/22
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
 * Contains functions used for looking up a class.
 */

#include "javavm/include/interpreter.h"
#include "javavm/include/classes.h"
#include "javavm/include/utils.h"
#include "javavm/include/common_exceptions.h"
#include "javavm/include/globals.h"
#include "javavm/include/preloader.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/directmem.h"
#include "javavm/include/stacks.h"
#include "javavm/include/stackwalk.h"
#include "javavm/include/globalroots.h"
#include "javavm/include/localroots.h"

#ifdef CVM_CLASSLOADING
#include "generated/offsets/java_lang_ClassLoader.h"
#endif

/* static functions used by this compilation unit. */

static CVMClassBlock*
CVMclassLookupFromClassLoader(CVMExecEnv* ee, 
			      CVMClassTypeID typeID, const char* name,
			      CVMBool init,
			      CVMClassLoaderICell* loader,
			      CVMObjectICell* pd,
			      CVMBool throwError);

/*
 * Do a lookup without class loading. Checks if class is preloaded
 * (only if loader == NULL) and checks if the class is in the
 * loaderCache.
 *
 * NOTE: It is possible for this function to return NULL even if the
 * class is loaded. This can happen if the loader specified is neither
 * the intiating nor the defining ClassLoader.
 */
CVMClassBlock*
CVMclassLookupClassWithoutLoading(CVMExecEnv* ee, CVMClassTypeID typeID,
				  CVMClassLoaderICell* loader)
{
    CVMClassBlock* cb;

    /*
     * First, see if it is a preloaded class, since they may not be in
     * the loader cache yet.
     */
    cb = CVMpreloaderLookupFromType(ee, typeID, loader);
    if (cb != NULL) {
	return cb;
    }

    /* See if it is in the loader cache */
    CVM_LOADERCACHE_LOCK(ee);
    cb = CVMloaderCacheLookup(ee, typeID, loader);
    CVM_LOADERCACHE_UNLOCK(ee);

    return cb;
}

/*
 * Class lookup by name or typeID. 
 *   -The "init" flag indicates if static initializers should be run.
 *   -The "throwError" flag will cause NoClassDefFoundError to be thrown
 *    instead of ClassNotFoundException.
 *   -An exception os thrown if a NULL result is returned.
 *
 * All of these functions will cause an attempt to load the class if it
 * is not already loaded.
 */

CVMClassBlock*
CVMclassLookupByNameFromClass(CVMExecEnv* ee, const char* name,
			      CVMBool init, CVMClassBlock* fromClass)
{
    CVMClassLoaderICell* loader;
    CVMObjectICell*      pd;
    CVMClassBlock*       cb;
    CVMClassTypeID       typeID = 
	CVMtypeidNewClassID(ee, name, (int)strlen(name));

    if (typeID == CVM_TYPEID_ERROR) {
	return NULL; /* exception already thrown */
    }

    if (fromClass != NULL) {
	loader = CVMcbClassLoader(fromClass);
	pd = CVMcbProtectionDomain(fromClass);
    } else {
	loader = NULL;
	pd = NULL;
    }
    cb = CVMclassLookupFromClassLoader(ee, typeID, name, init,
				       loader, pd, CVM_TRUE);
    CVMtypeidDisposeClassID(ee, typeID);
    return cb;
}
					       
CVMClassBlock*
CVMclassLookupByTypeFromClass(CVMExecEnv* ee, CVMClassTypeID typeID, 
			      CVMBool init, CVMClassBlock* fromClass)
{
    CVMClassLoaderICell* loader;
    CVMObjectICell*      pd;
    if (fromClass != NULL) {
	loader = CVMcbClassLoader(fromClass);
	pd = CVMcbProtectionDomain(fromClass);
    } else {
	loader = NULL;
	pd = NULL;
    }
    return CVMclassLookupFromClassLoader(ee, typeID, NULL, init, 
					 loader, pd, CVM_TRUE);
}

CVMClassBlock*
CVMclassLookupByNameFromClassLoader(CVMExecEnv* ee, const char* name, 
				    CVMBool init,
				    CVMClassLoaderICell* loader, 
				    CVMObjectICell* pd,
				    CVMBool throwError)
{
    CVMClassBlock* cb;
    CVMClassTypeID typeID = 
	CVMtypeidNewClassID(ee, name, (int)strlen(name));

    if (typeID == CVM_TYPEID_ERROR) {
	return NULL; /* exception already thrown */
    }

    cb = CVMclassLookupFromClassLoader(ee, typeID, name, init,
				       loader, pd, throwError);
    CVMtypeidDisposeClassID(ee, typeID);
    return cb;
}

CVMClassBlock*
CVMclassLookupByTypeFromClassLoader(CVMExecEnv* ee, CVMClassTypeID typeID, 
				    CVMBool init,
				    CVMClassLoaderICell* loader, 
				    CVMObjectICell* pd,
				    CVMBool throwError)
{
    return CVMclassLookupFromClassLoader(ee, typeID, NULL, init,
					 loader, pd, throwError);
}


/*
 * CVMclassLookupFromClassLoader - The main lookup function. The class
 * typeID is passed in, but the name may be NULL. If both are passed in
 * then it can save some time by avoiding memory allocations for the name.
 */

static CVMClassBlock*
CVMclassLookupFromClassLoader(CVMExecEnv* ee, 
			      CVMClassTypeID typeID, const char* name,
			      CVMBool init,
			      CVMClassLoaderICell* loader,
			      CVMObjectICell* pd,
			      CVMBool throwError)
{
    CVMClassBlock* cb = NULL;
    char* namebuf = NULL;

    if (!CVMCstackCheckSize(ee, CVM_REDZONE_CVMclassLookupFromClassLoader,
			    "CVMclassLookupFromClassLoader", CVM_TRUE)) {
	return NULL;
    }

    CVMassert(!CVMlocalExceptionOccurred(ee));

    if (typeID == CVM_TYPEID_ERROR) {
	/*
	 * This should only happen if the typeid system ran out of memory.
	 * Note that passing CVM_TYPEID_ERROR is not allowed unless you also
	 * pass in the class name.
	 */
	CVMassert(name != NULL);
	CVMthrowNoClassDefFoundError(ee, "%s", name);
	return NULL;
    }

    /* No CVMFieldTypeIDs allowed. Upper bits must never be set. */
    CVMassert(CVMtypeidGetType(typeID) == typeID);

#ifndef CVM_CLASSLOADING
    CVMassert(loader == NULL);
    loader = NULL; /* should cause compiler to optimize away some code */
#endif

    /* 
     * See if the class is a romized one. If it is, then just return
     * it. Otherwise we'll need to do further lookups.
     */
    if (loader == NULL || CVMtypeidIsPrimitive(typeID) || 
	(CVMtypeidIsArray(typeID) &&
	 CVMtypeidIsPrimitive(CVMtypeidGetArrayBasetype(typeID))))
    {
	cb = CVMpreloaderLookupFromType(ee, typeID, NULL);
	if (cb != NULL) {
	    /* 
	     * If the class was preloaded then make sure it
	     * has been initialized if requested.
	     */
	    if (init) {
		if (!CVMclassInit(ee, cb)) {
		    cb = NULL;
		}
	    }
	    return cb;
	}
    }

    if (loader == NULL) {
	CVM_NULL_CLASSLOADER_LOCK(ee);
    } else {
         /* 
	  * Although class loaders are supposed to define loadClass as
	  * a synchronized method, the VM does not trust the class loader
	  * to do so. It enters the loader monitor just in case.
	  */
	if (!CVMgcSafeObjectLock(ee, loader)) {
	    CVMthrowOutOfMemoryError(ee, NULL);
	    return NULL;
	}
   }

    /* 
     * See if the class is already in the loader cache.
     */
    CVM_LOADERCACHE_LOCK(ee);
    cb = CVMloaderCacheLookup(ee, typeID, loader);
    CVM_LOADERCACHE_UNLOCK(ee);

    /*
     * If the class isn't already in the loader cache then load it if
     * necessary. If may already be loaded with a different initiating
     * class loader. If so, then we'll eventually recurse back into this
     * function with a parent class loader as the initiating class loader.
     */
    if (cb == NULL) {
	/*
	 * We need to disable remote exceptions, because we don't want
	 * to allow them to interrupt any java code we may end up
	 * executing or cause us to abort any C code while class loading.
	 */
	CVMdisableRemoteExceptions(ee);
	CVMassert(!CVMlocalExceptionOccurred(ee));
	if (CVMtypeidIsArray(typeID)) {
	    cb = CVMclassCreateMultiArrayClass(ee, typeID, loader, pd);
	} else {
	    /*
	     * The API supports passing in a valid typeID and a NULL class
	     * name. This way we only need to convert the typeID to a name
	     * string if necessary. At this point the conversion is necessary.
	     */
	    if (name == NULL) {
		namebuf = CVMtypeidClassNameToAllocatedCString(typeID);
		name = namebuf;
	    }
	    if (name == NULL) {
		CVMthrowOutOfMemoryError(ee, NULL);
	    } else {
#ifdef CVM_CLASSLOADING
		CVMassert(!CVMexceptionOccurred(ee));
		cb = CVMclassLoadClass(ee, loader, name, typeID);
#else
		CVMdebugPrintf(("CVMclassLoadClass: class loading"
				" not supported\n"));
		/*
		 * Class loading not supported. Throw a ClassNotFoundException.
		 * If throwError is set, this will be converted to
		 * a NoClassDefFoundError below.
		 */
		CVMthrowClassNotFoundException(ee, "%s", name);
#endif
	    }
	}
	CVMenableRemoteExceptions(ee);
	if (CVMexceptionOccurred(ee)) {
	    cb = NULL;
	}

	/* Update the loader cache */
	if (cb != NULL) {
	    if (!CVMloaderCacheAdd(ee, cb, loader)) {
		cb = NULL; /* exception already thrown */
	    }
	}
    }

#ifdef CVM_CLASSLOADING
    /* Fix for 6708366, don't check array class access */
    if (cb != NULL && !CVMtypeidIsArray(typeID)) {
	if (!CVMloaderCacheCheckPackageAccess(ee, loader, cb, pd)) {
	    cb = NULL;
	}
    }
#endif

    /* unlock the class loader we used. */
    if (loader == NULL) {
	CVM_NULL_CLASSLOADER_UNLOCK(ee);
    } else {
	CVMBool success = CVMgcSafeObjectUnlock(ee, loader);
	CVMassert(success); (void) success;
    }

    if (cb != NULL) {
	/* 
	 * If there were no errors then make sure the class
	 * has been initialized if requested.
	 */
	if (init) {
	    if (!CVMclassInit(ee, cb)) {
		cb = NULL;
	    }
	}
    } else {
	/*
	 * Handle errors: Convert ClassNotFoundException to 
	 * NoClassDefFoundError if requested.
	 */
	CVMassert(CVMexceptionOccurred(ee));
	if (throwError) {
	    CVMClassBlock* classNotFoundCb =
		CVMsystemClass(java_lang_ClassNotFoundException);
	    CVMClassBlock* exceptionCb;
	    CVMID_objectGetClass(ee, CVMlocalExceptionICell(ee), exceptionCb);

	    if (CVMisSubclassOf(ee, exceptionCb, classNotFoundCb)) {
		CVMclearLocalException(ee);
		CVMthrowNoClassDefFoundError(ee, "%!C", typeID);
	    }
	}
    }
    
    if (namebuf != NULL) {
	free(namebuf);
    }
    return cb;	
}
