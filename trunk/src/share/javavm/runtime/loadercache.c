/*
 * @(#)loadercache.c	1.46 06/10/25
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
 * Routines for maintaining the loader cache and loader constraints.
 */

#include "javavm/include/defs.h"
#include "javavm/include/objects.h"
#include "javavm/include/classes.h"
#include "javavm/include/globals.h"
#include "javavm/include/utils.h"
#include "javavm/include/directmem.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/globalroots.h"
#include "javavm/include/common_exceptions.h"

#ifdef CVM_CLASSLOADING
#include "generated/offsets/java_lang_ClassLoader.h"
#endif

/*
 * For a complete description of class loader constraints and loader
 * cache, see the following paper:
 *
 *     "Dynamic Class Loading in the Java Virtual Machine"
 *
 * by Sheng Liang and Gilad Bracha. The paper will be published in the
 * proceedings of the OOPSLA'98 conference.
 */


/* 
 * The Loader cache keeps a "(classID, loader) => cb" mapping. All the
 * classes in the cache have been verified to meet the class loader
 * constraints. The cache also speeds up class loading.
 *
 * Note that in the "(classID, loader) => cb" maping, "loader" is the
 * initiating loader, not necessary the loader that ultimately defines 
 * the class. A class loader may delegate class loading to another 
 * class loader. For example, when "java/lang/String" is loaded with 
 * an applet class loader, the resulting class in fact has NULL as its 
 * class loader. However, a loaded class will also always get an
 * entry with its actual class loader.
 *
 * When a new class is about to be added to the cache, we make sure 
 * it doesn't break any of the existing constraints.
 *
 * During the course of execution, new constraints may be added as
 * the result of class linking. Before each new constraint is added,
 * we make sure classes loaded in the cache will satisfy the new 
 * constraint.
 */

struct CVMLoaderCacheEntry {
    CVMLoaderCacheEntry* next;      /* next entry in hash bucket */
    CVMClassBlock*       cb;
    CVMClassLoaderICell* loader;    /* initiating loader */
    int num_pds;
    CVMObjectICell**     pds;       /* protection domains */
};

/* %comment c015 */
#define CVM_LOADER_CACHE_TABLE_SIZE 1009

#undef HASH_INDEX
/* 
 * loader is casted to a native pointer (CVMClassLoaderICell*)
 * therefore the cast has to be CVMAddr which is 4 byte on
 * 32 bit platforms and 8 byte on 64 bit platforms
 *
 * ...but on the other hand we finally need an integer between 0
 * and CVM_LOADER_CACHE_TABLE_SIZE. So it's quite safe to do only
 * an intermediate cast to CVMAddr to make the 64 bit compiler happy.
 */
#define HASH_INDEX(classID, loader) \
    (((CVMUint32)classID + (CVMUint32)(CVMAddr)loader) % CVM_LOADER_CACHE_TABLE_SIZE)


/*
 * Loader constraints form a "(ClassTypeID, loader set) => cb"
 * mapping.
 *
 * The following structure stores an ClassTypeID and a set of loaders.
 * The loaders are required to map the ClassTypeID to the same cb.
 *
 * The cb field exists to speed up checking. It is initially set to 
 * NULL, and later set to the first class loaded by one of the 
 * loaders.
 */

/*
 * Loader constraints aren't needed if we don't support class loading
 * or if class loaders are trusted.
 */
#if defined(CVM_CLASSLOADING) && !defined(CVM_TRUSTED_CLASSLOADERS)

struct CVMLoaderConstraint {
    CVMLoaderConstraint* next;
    CVMClassBlock*       cb;	/* initially NULL */
    CVMClassTypeID       classID;
    int num_loaders;            /* number of loaders[] entries in use */
    int max_loaders;		/* capacity loaders[] */
    CVMClassLoaderICell* loaders[2];	/* initiating loaders */
};

#define CVM_LOADER_CONSTRAINT_TABLE_SIZE 107


/*
 * Return the loader constraint with the specified classID and loader.
 */
static CVMLoaderConstraint**
CVMloaderConstraintLookup(CVMClassTypeID classID, CVMClassLoaderICell* loader)
{
    int index = (CVMUint32)classID % CVM_LOADER_CONSTRAINT_TABLE_SIZE;
    CVMLoaderConstraint** pp = &CVMglobals.loaderConstraints[index];
    while (*pp != NULL) {
        CVMLoaderConstraint* p = *pp;
	if (CVMtypeidIsSameType(p->classID, classID)) {
	    int i;
	    for (i = p->num_loaders - 1; i >= 0; i--) {
	        if (p->loaders[i] == loader) {
		    return pp;
		}
	    }
	}
	pp = &(p->next);
    }
    return pp;
}

/*
 * Make sure that the specified loader constraint has the specified 
 * number of free slots in the loaders[] array.
 */
static CVMBool
CVMloaderConstraintEnsureCapacity(CVMExecEnv* ee,
				  CVMLoaderConstraint** pp, int nfree)
{
    CVMLoaderConstraint* p = *pp;
    CVMLoaderConstraint* oldp;

    if (p->max_loaders - p->num_loaders < nfree) {
        int nh = offsetof(CVMLoaderConstraint, loaders);
        int n = nfree + p->num_loaders;
	if (n < 2 * p->max_loaders) {
	    n = 2 * p->max_loaders;
	}
        p = (CVMLoaderConstraint *)malloc(nh + sizeof(void*) * n);
	if (p == NULL) {
	    return CVM_FALSE;
	}
	memcpy(p, *pp, nh + sizeof(void*) * (*pp)->num_loaders);
	p->max_loaders = n;

	/*
	 * CVMloaderConstraintsMarkUnscannedClassesAndLoaders() may be
	 * running and it doesn't grab the loaderCache lock, so we make
	 * sure we block until it is done before we modify the entry.
	 */
	CVMD_gcUnsafeExec(ee, {
	    oldp = *pp;
	    *pp = p;
	});
	free(oldp);
    }
    return CVM_TRUE;
}

/*
 * Add the specified loader and cb to the specified loader constraint.
 */
static CVMBool
CVMloaderConstraintExtend(CVMExecEnv* ee,
			  CVMLoaderConstraint** pp, 
			  CVMClassLoaderICell* loader,
			  CVMClassBlock* cb)
{
    if (!CVMloaderConstraintEnsureCapacity(ee, pp, 1)) {
        return CVM_FALSE;
    }
    (*pp)->loaders[(*pp)->num_loaders++] = loader;
    (*pp)->cb = cb;
    return CVM_TRUE;
}

/*
 * Merge the two specified loader constraints into one.
 */
static CVMBool
CVMloaderConstraintMerge(CVMExecEnv* ee,
			 CVMLoaderConstraint **pp1,
			 CVMLoaderConstraint **pp2,
			 CVMClassBlock* cb)
{
    CVMLoaderConstraint* p1;
    CVMLoaderConstraint* p2;
    int i;

    /* make sure *pp1 has higher capacity */
    if ((*pp1)->max_loaders < (*pp2)->max_loaders) {
        CVMLoaderConstraint** tmp = pp2;
	pp2 = pp1;
	pp1 = tmp;
    }

    p1 = *pp1;
    p2 = *pp2;
    
    if (!CVMloaderConstraintEnsureCapacity(ee, pp1, p2->num_loaders)) {
        return CVM_FALSE;
    }

    /*
     * CVMloaderConstraintsMarkUnscannedClassesAndLoaders() may be
     * running and it doesn't grab the loaderCache lock, so we make
     * sure we block until it is done, so it doesn't look at this
     * entry while it is being modified.
     */
    CVMD_gcUnsafeExec(ee, {
	if (*pp1 != p1) {
	    /* p1 has been moved */
	    if (pp2 == &p1->next) {
		/* need to update pp2 because p1 has moved */
		pp2 = &(*pp1)->next;
		p2 = *pp2;
	    }
	    p1 = *pp1;
	}
 
	for (i = 0; i < p2->num_loaders; i++) {
	    p1->loaders[p1->num_loaders++] = p2->loaders[i];
	}
	p1->cb = cb;

	/* release pp2 */
	*pp2 = p2->next;
    });

    free(p2);

    return CVM_TRUE;
}

/* 
 * Impose a new loader constraint: "classID" must be loaded the same
 * way in loader1 and loader2.
 */
static CVMBool 
CVMloaderConstraintAdd(CVMExecEnv* ee,
		       CVMClassTypeID classID,
		       CVMClassLoaderICell* loader1,
		       CVMClassLoaderICell* loader2)
{
    CVMLoaderConstraint** pp1;
    CVMLoaderConstraint** pp2;
    CVMClassBlock* cb;
    CVMClassBlock* cb1;
    CVMClassBlock* cb2;
    CVMBool result;
    
    CVM_LOADERCACHE_LOCK(ee);
    
    /*
     * See if either loader put the class in the loader cache. If they
     * both did, then they both better have added the same class.
     */
    cb1 = CVMloaderCacheLookup(ee, classID, loader1);
    cb2 = CVMloaderCacheLookup(ee, classID, loader2);
    if (cb1 != NULL && cb2 != NULL && cb1 != cb2) {
        goto constraint_violation;
    } else {
        cb = (cb1 != NULL ? cb1 : cb2);
    }

    /* Verify that we meet any constraint setup for <classID, loader1> */
    pp1 = CVMloaderConstraintLookup(classID, loader1);
    if (*pp1 != NULL && (*pp1)->cb != NULL) {
        if (cb != NULL) {
	    if (cb != (*pp1)->cb) {
	        goto constraint_violation;
	    }
        } else {
	    cb = (*pp1)->cb;
        }
    }

    /* Verify that we meet any constraint setup for <classID, loader2> */
    pp2 = CVMloaderConstraintLookup(classID, loader2);
    if (*pp2 != NULL && (*pp2)->cb != NULL) {
        if (cb != NULL) {
	    if (cb != (*pp2)->cb) {
	        goto constraint_violation;
	    }
        } else {
	    cb = (*pp2)->cb;
        }
    }
    
    /* If there is are not existing constraints, then make one. */
    if (*pp1 == NULL && *pp2 == NULL) {
        int index;
        CVMLoaderConstraint* p =
	    (CVMLoaderConstraint *)malloc(sizeof(CVMLoaderConstraint));
	if (p == NULL) {
	    result = CVM_FALSE;
	} else {
	    CVMtraceClassLoading(("LC: creating new constraint: "
				  "0x%x => (0x%x, 0x%x, %!C)\n", 
				  p, loader1, loader2, classID));
	
	    p->classID = classID;
	    p->loaders[0] = loader1;
	    p->loaders[1] = loader2;
	    p->num_loaders = 2;
	    p->max_loaders = 2;
	    p->cb = cb;
	    
	    index = (CVMUint32)classID % CVM_LOADER_CONSTRAINT_TABLE_SIZE;
	    p->next = CVMglobals.loaderConstraints[index];
	    CVMglobals.loaderConstraints[index] = p;
	    result = CVM_TRUE;
	}
    } else if (*pp1 == *pp2) {
        /* constraint already imposed */
        (*pp1)->cb = cb;
	result = CVM_TRUE;
    } else if (*pp1 == NULL) {
	/* failure implies OutOfMemoryError */
        result = CVMloaderConstraintExtend(ee, pp2, loader1, cb);
    } else if (*pp2 == NULL) {
 	/* failure implies OutOfMemoryError */
	result = CVMloaderConstraintExtend(ee, pp1, loader2, cb);
    } else {
	/* failure implies OutOfMemoryError */
	result = CVMloaderConstraintMerge(ee, pp1, pp2, cb);
    }

    CVM_LOADERCACHE_UNLOCK(ee);
    if (!result) {
	CVMthrowOutOfMemoryError(ee, NULL);
    }
    return result;
    
 constraint_violation:
    CVM_LOADERCACHE_UNLOCK(ee);
    CVMthrowLinkageError(ee, "Class %!C violates loader constraints", classID);
    return CVM_FALSE;
}

/* 
 * Make sure all class components (including arrays) in the given
 * signature will be resolved to the same class in both loaders.
 * loader1 and loader2 must both be ClassLoader global roots. In other
 * words, they should orginate from the CVMcbClassLoader() field.
 */
CVMBool 
CVMloaderConstraintsCheckMethodSignatureLoaders(CVMExecEnv* ee,
						CVMMethodTypeID methodID, 
						CVMClassLoaderICell* loader1,
						CVMClassLoaderICell* loader2)
{
    CVMClassTypeID argClassID;
    CVMSigIterator sigIterator;

    /* Nothing to do if loaders are the same. */
    if (loader1 == loader2) {
       return CVM_TRUE;
    }

    /* Handle the signature result type first */
    CVMtypeidGetSignatureIterator(methodID, &sigIterator);
    argClassID = CVM_SIGNATURE_ITER_RETURNTYPE(sigIterator);

    /*
     * Iteratate over all method argument types. The result type will
     * be handled first.
     */
    do {
	if (!CVMtypeidIsPrimitive(argClassID)) {
	    if (!CVMloaderConstraintAdd(ee, argClassID, loader1, loader2)) {
		return CVM_FALSE; /* exception already thrown */
	    }
	}
	    
	/* Get the next signature argument */
	argClassID = CVM_SIGNATURE_ITER_NEXT(sigIterator);
    } while (argClassID != CVM_TYPEID_ENDFUNC);

    return CVM_TRUE;
}

/* 
 * Make sure all class components (including arrays) in the given
 * signature will be resolved to the same class in both loaders.
 * loader1 and loader2 must both be ClassLoader global roots. In other
 * words, they should orginate from the CVMcbClassLoader() field.
 */
CVMBool 
CVMloaderConstraintsCheckFieldSignatureLoaders(CVMExecEnv* ee,
					       CVMFieldTypeID fieldID, 
					       CVMClassLoaderICell* loader1,
					       CVMClassLoaderICell* loader2)
{
    /* Nothing to do for primitive classes. */
    if (CVMtypeidIsPrimitive(fieldID)) {
	return CVM_TRUE;
    }

    /* Nothing to do if loaders are the same. */
    if (loader1 == loader2) {
       return CVM_TRUE;
    }

    return CVMloaderConstraintAdd(ee, CVMtypeidGetType(fieldID),
				  loader1, loader2);
}

/* 
 * Called during pass #1 of class unloadering. After a gc scan, we need to
 * mark all entries for loaders that are going away, and NULL out the cb
 * field of all entries whose class is going away. We will defer actually
 * deleting the entries for unused loaders until pass #2 when we can
 * safely grab the loaderCacheLock.
 */
void 
CVMloaderConstraintsMarkUnscannedClassesAndLoaders(
    CVMExecEnv* ee,
    CVMRefLivenessQueryFunc isLive,
    void* isLiveData)
{
    int i;
    for (i = 0; i < CVM_LOADER_CONSTRAINT_TABLE_SIZE; i++) {
        CVMLoaderConstraint** cp = &(CVMglobals.loaderConstraints[i]);
	while (*cp != NULL) {
	    CVMLoaderConstraint* p = *cp;
	    int j = 0;
	    while (j < p->num_loaders) {
		CVMClassLoaderICell* loader = p->loaders[j];
		/*
		 * If the low bit on the loader is set, then this entry was
		 * marked for removal by the previous gc, and we never got
		 * a chance to remove it before gc was entered again.
		 */
		/* 
		 * loader is cast to a native pointer (CVMClassLoaderICell*)
		 * therefore the cast has to be CVMAddr which is 4 byte on
		 * 32 bit platforms and 8 byte on 64 bit platforms
		 */
		if ((((CVMAddr)loader) & 1) != 0) {
		    ++j;
		    continue;
		}

		/* If the loader isn't live, then mark it for removal. */
	        if (loader != NULL 
		    && !isLive((CVMObject**)loader, isLiveData)) 
		{
		    CVMtraceClassLoading(("LC: deleting loader constraint "
					  "#%i (%!C, 0x%x)\n",
					  i, p->classID, loader));
		    /* mark as unused, free it later */
		    /* 
		     * loader is cast to a native pointer (CVMClassLoaderICell*)
		     * therefore the cast has to be CVMAddr which is 4 byte on
		     * 32 bit platforms and 8 byte on 64 bit platforms
		     */
		    p->loaders[j] = 
			(CVMClassLoaderICell*)(((CVMAddr)loader) | 1);
		}
		++j;
	    }
	    
	    /* If the class isn't live, then NULL out the cb field. */
	    if (p->cb != NULL &&
		!isLive((CVMObject**)CVMcbJavaInstance(p->cb), isLiveData)) {
	        /* This happens if the initiating loader outlives the class */
	        p->cb = NULL;
	    }
	    cp = &(p->next);
	}
    }
}

/* 
 * Called during pass #2 of class unloading. Delete all the loader entries
 * that got marked during CVMloaderConstraintsMarkUnscannedClassesAndLoaders.
 */
void 
CVMloaderConstraintsPurgeUnscannedClassesAndLoaders(CVMExecEnv* ee)
{
    int i;
    CVM_LOADERCACHE_LOCK(ee);
    /*
     * We need to become gc unsafe or we could end up having
     * CVMloaderConstraintsMarkUnscannedClassesAndLoaders running at the 
     * same time.
     */
    CVMD_gcUnsafeExec(ee, {
	for (i = 0; i < CVM_LOADER_CONSTRAINT_TABLE_SIZE; i++) {
	    CVMLoaderConstraint** cp = &(CVMglobals.loaderConstraints[i]);
	    while (*cp != NULL) {
		CVMLoaderConstraint* p = *cp;
		int j = 0;
		while (j < p->num_loaders) {
		    CVMClassLoaderICell* loader = p->loaders[j];
		    /* 
		     * loader is cast to a native pointer (CVMClassLoaderICell*)
		     * therefore the cast has to be CVMAddr which is 4 byte on
		     * 32 bit platforms and 8 byte on 64 bit platforms
		     */
		    if ((((CVMAddr)loader) & 1) != 0) {
			CVMtraceClassLoading(("LC: deleting loader constraint "
					      "#%i\n", i));
			p->loaders[j] = p->loaders[p->num_loaders - 1];
			p->num_loaders--;
		    } else {
			++j;
		    }
		}
		if (p->num_loaders < 2) {
		    *cp = p->next;
		    free(p);
		    continue;
		}
		cp = &(p->next);
	    }
	}
    });
    CVM_LOADERCACHE_UNLOCK(ee);
}

/*
 * Dump the loader cache.
 */
#ifdef CVM_DEBUG
void
CVMloaderConstraintsDump(CVMExecEnv* ee)
{
    int i;
    for (i = 0; i < CVM_LOADER_CONSTRAINT_TABLE_SIZE; i++) {
	CVMLoaderConstraint* entry = CVMglobals.loaderConstraints[i];
	while (entry != NULL) {
	    CVMtraceClassLookup(("LC: #%i 0x%x: <0x%x,%!C>\n", 
				 i, entry->cb, entry->loaders[0], 
				 entry->classID));
	    entry = entry->next;
	}
    }
}
#endif /* CVM_DEBUG */

#endif /* CVM_CLASSLOADING */

/* End of loader constraints functions */


/* Start of loader cache functions */

/* 
 * The loader CVMClassLoaderICell may not be a ClassLoader global root. It
 * could be java stack based or jni local ref. We need to get the real
 * ClassLoader global root for the loader instance. Fortunately, the
 * ClassLoader instance has a ClassLoader global root that refers to
 * itself in the ClassLoader.loaderGlobalRoot field.
 */
CVMClassLoaderICell*
CVMloaderCacheGetGlobalRootFromLoader(CVMExecEnv* ee,
				      CVMClassLoaderICell* loader)
{
#ifdef CVM_CLASSLOADING
    if (loader != NULL) {
	/* Get the current ClassLoader.loaderGlobalRoot field */
	/* 
	 * Access member variable of type 'int'
	 * and cast it to a native pointer. The java type 'int' only garanties 
	 * 32 bit, but because one slot is used as storage space and
	 * a slot is 64 bit on 64 bit platforms, it is possible 
	 * to store a native pointer without modification of
	 * java source code. This assumes that all places in the C-code
	 * are catched which set/get this member.
	 */
	CVMAddr temp;
	CVMID_fieldReadAddr(ee, loader,
			    CVMoffsetOfjava_lang_ClassLoader_loaderGlobalRoot,
			    temp);
	return (CVMClassLoaderICell*)temp;
    }
#endif
    return NULL;
}

static void
CVMloaderCacheFreeEntry(CVMExecEnv* ee, CVMLoaderCacheEntry* currEntry)
{
    if (currEntry->pds != NULL) {
	int j;
	for (j = 0; j < currEntry->num_pds; j++) {
	    CVMID_freeWeakGlobalRoot(ee, currEntry->pds[j]);
	}
	free(currEntry->pds);
    }
    free(currEntry);
}

/*
 * Added the specified protection domain to the loader cache entry.
 */

#ifdef CVM_CLASSLOADING
static CVMBool
CVMloaderCacheAddProtectionDomain(CVMExecEnv* ee, CVMClassTypeID classID, 
				  CVMClassLoaderICell* loader,
				  CVMObjectICell* pd)
{
    CVMLoaderCacheEntry* entry;

    loader = CVMloaderCacheGetGlobalRootFromLoader(ee, loader);
    entry  = CVMglobals.loaderCache[HASH_INDEX(classID, loader)];

    CVM_LOADERCACHE_ASSERT_LOCKED(ee);

    while (entry) {
	CVMClassBlock* cb = entry->cb;
	CVMBool isMarked;
	
	/*
	 * If the entry's cb has it's low bit set, then the entry has been
	 * marked for purging and should be ignored.
	 */
	/* 
	 * cb is a native pointer (CVMClassBlock*)
	 * therefore the cast has to be CVMAddr which is 4 byte on
	 * 32 bit platforms and 8 byte on 64 bit platforms
	 */
	isMarked = (((CVMAddr)cb) & 1) != 0;
	if (!isMarked && CVMcbClassName(cb) == classID
	    && entry->loader == loader) {
	    /* Allocate new pds array that can hold new pd */
	    CVMObjectICell** pds = (CVMObjectICell**)
		malloc((entry->num_pds + 1) * sizeof(CVMObjectICell*));
	    if (pds == NULL) {
		return CVM_FALSE;/* can't throw exception while holding lock */
	    }

	    /* copy the old pd's into the new array */
	    {
		int i;
		for(i=0; i < entry->num_pds; i++) {
		    pds[i] = entry->pds[i];
		}
	    }

	    /* Get a new global root for the new pd entry */
	    pds[entry->num_pds] = CVMID_getWeakGlobalRoot(ee);
	    if (pds[entry->num_pds] == NULL) {
		free(pds);
		return CVM_FALSE;/* can't throw exception while holding lock */
	    }
	    CVMID_icellAssign(ee, pds[entry->num_pds], pd);

	    entry->num_pds++; /* one more entry added */
	    free(entry->pds); /* no need to keep the old array around */
	    entry->pds = pds; /* start using the new pd array */
	    return CVM_TRUE;
	}
	entry = entry->next;
    }
    CVMassert(CVM_FALSE); /* we should never get here */
    return CVM_FALSE;
}
#endif

/*
 * CVMloaderCacheCheckPackageAccess - verify that the given class/loader
 * pair are accessible in the given protection domain.
 */

#ifdef CVM_CLASSLOADING
CVMBool
CVMloaderCacheCheckPackageAccess(CVMExecEnv *ee, CVMClassLoaderICell* loader, 
				 CVMClassBlock* cb, CVMObjectICell* pd)
{
    JNIEnv* env = CVMexecEnv2JniEnv(ee);
    CVMClassTypeID classID = CVMcbClassName(cb);

    if (pd == NULL || loader == NULL) {
	return CVM_TRUE;
    }

    /*
     * Has this package already been loaded by this loader with the
     * given domain?
     */
    CVM_LOADERCACHE_LOCK(ee);
    if (CVMloaderCacheLookupWithProtectionDomain(ee, classID, loader, pd)) {
        CVM_LOADERCACHE_UNLOCK(ee);
	return CVM_TRUE;
    }
    CVM_LOADERCACHE_UNLOCK(ee);

    if ((*env)->PushLocalFrame(env, 4) < 0) {
	return CVM_FALSE; /* exception already thrown */
    }

    /*
     * Does this domain have permission to access the package?
     */
    (*env)->CallVoidMethod(env, loader,
			   CVMglobals.java_lang_ClassLoader_checkPackageAccess,
			   CVMcbJavaInstance(cb), pd);

    (*env)->PopLocalFrame(env, 0);

    if (CVMlocalExceptionOccurred(ee)) {
	return CVM_FALSE;
    }

    CVM_LOADERCACHE_LOCK(ee);
    if (!CVMloaderCacheLookupWithProtectionDomain(ee, classID, loader, pd) &&
	!CVMloaderCacheAddProtectionDomain(ee, classID, loader, pd))
    {
	/*
	 * All errors are assumed to be OutOfMemoryError. We need to throw
	 * one here since we were not allowed to throw one while holding
	 * the lock.
	 */
        CVM_LOADERCACHE_UNLOCK(ee);
	CVMassert(!CVMexceptionOccurred(ee));
	CVMthrowOutOfMemoryError(ee, NULL);
	return CVM_FALSE;
    }
    CVM_LOADERCACHE_UNLOCK(ee);

    return CVM_TRUE;
}
#endif

/*
 * Look for an entry with the specified typeID, class loader
 * and protection domain.
 */
CVMClassBlock*
CVMloaderCacheLookupWithProtectionDomain(CVMExecEnv* ee, 
					 CVMClassTypeID classID,
					 CVMClassLoaderICell* loader,
					 CVMObjectICell* pd)
{
    CVMLoaderCacheEntry* entry;

    loader = CVMloaderCacheGetGlobalRootFromLoader(ee, loader);

    CVMtraceClassLookup(("LC: loader cache lookup <0x%x,%!C>\n",
			 loader, classID));

    {
	CVMClassBlock *cb = CVMpreloaderLookupFromType(ee, classID, loader);
	if (cb != NULL) {
	    return cb;
	}
    }

    CVM_LOADERCACHE_ASSERT_LOCKED(ee);

    entry  = CVMglobals.loaderCache[HASH_INDEX(classID, loader)];

    while (entry) {
	CVMClassBlock* cb = entry->cb;
	CVMBool isMarked;
	
	/*
	 * If the entry's cb has it's low bit set, then the entry has been
	 * marked for purging and should be ignored.
	 */
	/* 
	 * cb is a native pointer (CVMClassBlock*)
	 * therefore the cast has to be CVMAddr which is 4 byte on
	 * 32 bit platforms and 8 byte on 64 bit platforms
	 */
	isMarked = (((CVMAddr)cb) & 1) != 0;
	if (!isMarked && CVMcbClassName(cb) == classID
	    && entry->loader == loader) {
	    /*
	     * If the ERROR flag is set and the SUPERCLASS_LOADED flag
	     * is not set, then ignore this entry. This can only happen
	     * if the Class.loadSuperClasses() call failed when the
	     * class was created. Nothing keeps the class live if that
	     * happens, so we let the class hang around in the loaderCache
	     * and classTable until it gets gc'd. It's not safe to return
	     * a reference to this class, because this breaks the invariant
	     * that the loadercache will never return a reference to a class
	     * that isn't being kept live. Otherwise a gc could delete the
	     * class returned by the lookup. Note there may be another instance
	     * of this same class in the loadercache, because we do allow
	     * subsequent attempts to create the same class. Therefore,
	     * we continue the search if this class is in error.
	     *
	     * If both ERROR and SUPERCLASS_LOADED are set, then this means
	     * that superclasses were successfully loaded for this class,
	     * but initialization of the class failed. In this case we do
	     * return the class, because it is a valid class.
	     *
	     * See c09/11/2000 in classcreate.c for more details.
	     */
	    if (CVMcbCheckErrorFlag(ee, cb) && 
		!CVMcbCheckRuntimeFlag(cb, SUPERCLASS_LOADED)) {
		goto next_entry;
	    }

	    /*
	     * If necessary, make sure protection domain matches.
	     */
	    if (pd != NULL) {
		CVMBool found = CVM_FALSE;
		int i;
		for (i = 0; i < entry->num_pds; i++) {
		    CVMBool isSamePD;
		    CVMID_icellSameObject(ee, entry->pds[i], pd, isSamePD);
		    if (isSamePD) {
			found = CVM_TRUE;
			break;
		    }
		}
		if (!found) {
		    break; /* don't search anymore */
		}
	    }
	    
	    CVMtraceClassLookup(("LC: Found <0x%x,%C> cb=0x%x\n",
				 loader, cb, cb));
	    return cb;
	}
    next_entry:
	entry = entry->next;
    }
    
    CVMtraceClassLookup(("LC: not found <0x%x,%!C>\n",
			 loader, classID));
    return NULL;
}

/*
 * Add a class to the loader cache. The loading of "cb" has been initiated
 * by "loader". This function ensures that cb meets the constraints already.
 * established for this (class,loader) pair.
 */
CVMBool
CVMloaderCacheAdd(CVMExecEnv* ee, CVMClassBlock* cb,
		  CVMClassLoaderICell* loader)
{
#if defined(CVM_CLASSLOADING) && !defined(CVM_TRUSTED_CLASSLOADERS)
    CVMLoaderConstraint* p;
#endif
    CVMBool result;

    loader = CVMloaderCacheGetGlobalRootFromLoader(ee, loader);

    CVM_LOADERCACHE_ASSERT_UNLOCKED(ee);
    CVM_LOADERCACHE_LOCK(ee);

#ifdef CVM_JVMTI
    /*
     * NOTE: check to see if this
     * class is in the process of being redefined.  If so
     * then we just ignore this attempt at adding the new
     * redefined class to the cache.
     */
    /* Note: make the following test a macro when we remove ThreadNode */
    if (CVMjvmtiIsEnabled() && CVMjvmtiClassBeingRedefined(ee, cb)) {
	CVM_LOADERCACHE_UNLOCK(ee);
	return CVM_TRUE;
    }
#endif
    /*
     * Make sure we aren't trying to replace an existing entry.
     */
    {
        CVMClassBlock* cb0 = 
	    CVMloaderCacheLookup(ee, CVMcbClassName(cb), loader);
	if (cb0 != NULL) {
	    CVM_LOADERCACHE_UNLOCK(ee);
#if defined(CVM_CLASSLOADING) && !defined(CVM_TRUSTED_CLASSLOADERS)
	    /*
	     * Not allowed to update an existing entry. This probably can
	     * only happen if a class loader does something bad like
	     * call defineClass() twice with the same class name.
	     */
	    if (cb0 != cb) {
		CVMthrowLinkageError(ee, "trying to redefine class %C "
				     "(bad class loader?)", cb);
		result = CVM_FALSE;
		goto finish;
	    }
#endif
	    result = CVM_TRUE;
	    goto finish;
	}
    }

#if defined(CVM_CLASSLOADING) && !defined(CVM_TRUSTED_CLASSLOADERS)
    /* Make sure we don't violate any existing loader constraints. */
    p = *(CVMloaderConstraintLookup(CVMcbClassName(cb), loader));
    if (p != NULL && p->cb != NULL && p->cb != cb) {
	CVM_LOADERCACHE_UNLOCK(ee);
        CVMthrowLinkageError(ee, "Class %C violates loader constraints", cb);
	result = CVM_FALSE;
	goto finish;
    }
#endif

    /* We can now add (cb, loader) into the loader cache */
    {
	CVMLoaderCacheEntry* entry;
	int index = HASH_INDEX(CVMcbClassName(cb), loader);
	
	/* %comment c016 */
	entry = (CVMLoaderCacheEntry *)malloc(sizeof(CVMLoaderCacheEntry));
	if (entry == NULL) {
	    CVM_LOADERCACHE_UNLOCK(ee);
	    CVMthrowOutOfMemoryError(ee, NULL);
	    result = CVM_FALSE;
	    goto finish;
	} 
		
	entry->next = CVMglobals.loaderCache[index];
	entry->cb = cb;
	entry->loader = loader;
	entry->num_pds = 0;
	entry->pds = NULL;
	CVMglobals.loaderCache[index] = entry;
    }

#if defined(CVM_CLASSLOADING) && !defined(CVM_TRUSTED_CLASSLOADERS)
    if (p != NULL && p->cb == NULL) {
        p->cb = cb;
    }
#endif

    CVMtraceClassLoading(("LC: added to loader cache: (0x%x,%C) => 0x%x\n",
			  loader, cb, cb));

    CVM_LOADERCACHE_UNLOCK(ee);
    result = CVM_TRUE;

 finish:

    CVM_LOADERCACHE_ASSERT_UNLOCKED(ee);
    return result;
}

#ifdef CVM_CLASSLOADING

/* 
 * Called during pass #1 of class unloader. Mark all loader cache
 * entries that contain an unreachable Class or ClassLoader. This is
 * done after a full gc. We will defer actually deleting the entries
 * until pass #2 when we can safely grab the loaderCacheLock.
 */
extern void
CVMloaderCacheMarkUnscannedClassesAndLoaders(CVMExecEnv* ee,
					     CVMRefLivenessQueryFunc isLive,
					     void* isLiveData)
{
    int i;
    for (i = 0; i < CVM_LOADER_CACHE_TABLE_SIZE; i++) {
	CVMLoaderCacheEntry* currEntry = CVMglobals.loaderCache[i];
	while (currEntry != NULL) {
	    CVMBool removeEntry;
	    CVMClassLoaderICell* loader = currEntry->loader;

	    /*
	     * If the low bit on the cb is set, then this entry was marked
	     * for removal by the previous gc, and we never got a chance
	     * to remove it before gc was entered again.
	     */
	    /* 
	     * cb is a native pointer (CVMClassBlock*)
	     * therefore the cast has to be CVMAddr which is 4 byte on
	     * 32 bit platforms and 8 byte on 64 bit platforms
	     */
	    if ((((CVMAddr)currEntry->cb) & 1) != 0) {
		currEntry = currEntry->next;
		continue;
	    }
	    
	    /*
	     * Remove the entry if the ClassLoader is not live or if
	     * the Class is not live.
	     */
	    if (loader != NULL && !isLive((CVMObject**)loader, isLiveData)) {
		removeEntry = CVM_TRUE;
	    } else if (!CVMcbIsInROM(currEntry->cb)) {
		removeEntry = 
		    !isLive((CVMObject**)CVMcbJavaInstance(currEntry->cb),
			    isLiveData);
	    } else {
		removeEntry = CVM_FALSE;
	    }
	    
	    if (removeEntry) {
		CVMtraceClassLookup(("LC: Loader cache removed "
				     "#%i <0x%x,%C>\n", 
				     i, currEntry->loader, currEntry->cb));
		/* mark as unused, free it later */
		/* 
		 * cb is a native pointer (CVMClassBlock*)
		 * therefore the cast has to be CVMAddr which is 4 byte on
		 * 32 bit platforms and 8 byte on 64 bit platforms
		 */
		currEntry->cb = 
		    (CVMClassBlock*)(((CVMAddr)currEntry->cb) | 1);
	    }
	    currEntry = currEntry->next;
	}
    }
}

/*
 * Called during pass #2 of class unloading. Free all the entries marked
 * during pass #1.
 */
extern void
CVMloaderCachePurgeUnscannedClassesAndLoaders(CVMExecEnv* ee)
{
    int i;
    CVMLoaderCacheEntry* entriesToFree = NULL;

    CVM_LOADERCACHE_LOCK(ee);
    /*
     * We need to become gc unsafe or we could end up having
     * CVMloaderCacheMarkUnscannedClassesAndLoaders running at the 
     * same time.
     */
    CVMD_gcUnsafeExec(ee, {
	for (i = 0; i < CVM_LOADER_CACHE_TABLE_SIZE; i++) {
	    CVMLoaderCacheEntry* currEntry = CVMglobals.loaderCache[i];
	    CVMLoaderCacheEntry* prevEntry = NULL;
	    while (currEntry != NULL) {
		/* 
		 * cb is a native pointer (CVMClassBlock*)
		 * therefore the cast has to be CVMAddr which is 4 byte on
		 * 32 bit platforms and 8 byte on 64 bit platforms
		 */
		if ((((CVMAddr)currEntry->cb) & 1) != 0) {
		    CVMLoaderCacheEntry* entryToFree = currEntry;
		    if (prevEntry == NULL) {
			CVMglobals.loaderCache[i] = currEntry->next;
		    } else {
			prevEntry->next = currEntry->next;
		    }
		    currEntry = currEntry->next;
                    /* Put this entry on the entriesToFree list to be freed
                       later below: */
		    entryToFree->next = entriesToFree;
		    entriesToFree = entryToFree;
		    CVMtraceClassLookup(("LC: Loader cache removed #%i\n", i));
		} else {
		    prevEntry = currEntry;
		    currEntry = currEntry->next;
		}
	    }
	}
    });
    CVM_LOADERCACHE_UNLOCK(ee);
    
    /*
     * Free the memory while gcSafe. The main reason for doing this is
     * because we need to free up the global roots used for protection
     * domains, and we have to be gcSafe to do this.
     */
    while (entriesToFree != NULL) {
	CVMLoaderCacheEntry* currEntry = entriesToFree;
	entriesToFree = currEntry->next;
	CVMloaderCacheFreeEntry(ee, currEntry);
    }
}

#endif

/*
 * Dump the loader cache.
 */
#ifdef CVM_DEBUG
void
CVMloaderCacheDump(CVMExecEnv* ee)
{
    int i;
    for (i = 0; i < CVM_LOADER_CACHE_TABLE_SIZE; i++) {
	CVMLoaderCacheEntry* entry = CVMglobals.loaderCache[i];
	while (entry != NULL) {
	    CVMtraceClassLookup(("LC: #%i 0x%x: <0x%x,%C>\n", 
				 i, entry->cb, entry->loader, entry->cb));
	    entry = entry->next;
	}
    }
}
#endif /* CVM_DEBUG */

#ifdef CVM_JVMTI
/*
 * Enumerate the loader cache.
 */
void
CVMloaderCacheIterate(CVMExecEnv* ee, CVMLoaderCacheIterator *iter)
{
    iter->entry = NULL;
    iter->index = -1;
}

CVMBool
CVMloaderCacheIterateNext(CVMExecEnv* ee, CVMLoaderCacheIterator *iter)
{
    CVMLoaderCacheEntry* entry = (CVMLoaderCacheEntry *)iter->entry;
    if (entry != NULL) {
	entry = entry->next;
    }
    if (entry == NULL) {
	int index = iter->index;
	while (++index < CVM_LOADER_CACHE_TABLE_SIZE) {
	    entry = CVMglobals.loaderCache[index];
	    if (entry != NULL) {
		break;
	    }
	}
	iter->index = index;
    }
    iter->entry = entry;
    return entry != NULL;
}

CVMObjectICell *
CVMloaderCacheIterateGetLoader(CVMExecEnv* ee, CVMLoaderCacheIterator *iter)
{
    if (iter->entry != NULL) {
	return ((CVMLoaderCacheEntry *)iter->entry)->loader;
    } else {
	return NULL;
    }
}

CVMClassBlock *
CVMloaderCacheIterateGetCB(CVMExecEnv* ee, CVMLoaderCacheIterator *iter)
{
    if (iter->entry != NULL) {
	return ((CVMLoaderCacheEntry *)iter->entry)->cb;
    } else {
	return NULL;
    }
}
#endif

/*
 * Do the initialization of the loader cache and loader constraints.
 */
CVMBool
CVMloaderCacheInit()
{
    CVMglobals.loaderCache = (CVMLoaderCacheEntry **)
	calloc(CVM_LOADER_CACHE_TABLE_SIZE, sizeof(CVMLoaderCacheEntry*));
    if (CVMglobals.loaderCache == NULL) {
	return CVM_FALSE;
    }
#if defined(CVM_CLASSLOADING) && !defined(CVM_TRUSTED_CLASSLOADERS)
    CVMglobals.loaderConstraints = (CVMLoaderConstraint **)
	calloc(CVM_LOADER_CONSTRAINT_TABLE_SIZE, sizeof(CVMLoaderConstraint*));
    if (CVMglobals.loaderConstraints == NULL) {
	return CVM_FALSE;
    }
#endif
    return CVM_TRUE;
}

/*
 * Free up all loader cache entries
 */
void
CVMloaderCacheDestroy(CVMExecEnv* ee)
{
    /* 
     * Free all loader cache and loader constraints entries
     * and the tables themselves.
     */
    int i;

    for (i = 0; i < CVM_LOADER_CACHE_TABLE_SIZE; i++) {
	CVMLoaderCacheEntry* entry = CVMglobals.loaderCache[i];
	while (entry != NULL) {
	    CVMLoaderCacheEntry* nextEntry = entry->next;
	    CVMloaderCacheFreeEntry(ee, entry);
	    entry = nextEntry;
	}
    }
    free(CVMglobals.loaderCache);

#if defined(CVM_CLASSLOADING) && !defined(CVM_TRUSTED_CLASSLOADERS)
    for (i = 0; i < CVM_LOADER_CONSTRAINT_TABLE_SIZE; i++) {
	CVMLoaderConstraint* entry = CVMglobals.loaderConstraints[i];
	while (entry != NULL) {
	    CVMLoaderConstraint* nextEntry = entry->next;
	    free(entry);
	    entry = nextEntry;
	}
    }
    free(CVMglobals.loaderConstraints);
#endif
}
