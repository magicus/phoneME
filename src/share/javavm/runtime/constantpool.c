/*
 * @(#)constantpool.c	1.53 06/10/22
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
#include "javavm/include/classes.h"
#include "javavm/include/utils.h"
#include "javavm/include/common_exceptions.h"
#include "javavm/include/globals.h"
#include "javavm/include/indirectmem.h"

/*
 * Resolve a NameAndType constant pool entry into an fb or mb.
 */
static CVMFieldBlock*
CVMcpResolveFieldref(CVMExecEnv* ee,
		     CVMClassBlock* currentCb, 
		     CVMConstantPool* cp, 
		     CVMClassBlock* fieldCb,
		     CVMUint16 typeIDIdx);

static CVMMethodBlock*
CVMcpResolveMethodref(CVMExecEnv* ee,
		      CVMClassBlock* currentCb, 
		      CVMConstantPool* cp, 
		      CVMClassBlock* methodCb,
		      CVMUint16 typeIDIdx);

static CVMMethodBlock*
CVMcpResolveInterfaceMethodref(CVMExecEnv* ee,
			       CVMClassBlock* currentCb, 
			       CVMConstantPool* cp, 
			       CVMClassBlock* methodCb,
			       CVMUint16 typeIDIdx);

/* Iterate the class' constant pool and resolve unresolved entries. */
void
CVMcpResolveCbEntriesWithoutClassLoading(CVMExecEnv* ee, 
                                         CVMClassBlock* cb)
{
    CVMConstantPool* cp = CVMcbConstantPool(cb);
    CVMUint16 i;

    if ((cp != NULL) && (CVMcpTypes(cp) != NULL)) {
        for (i = 1; i < CVMcbConstantPoolCount(cb); i++) {
            CVMConstantPoolEntryType cpType = CVMcpEntryType(cp, i);
            switch(cpType) {
                case CVM_CONSTANT_ClassTypeID:
                case CVM_CONSTANT_Fieldref:
                case CVM_CONSTANT_Methodref:
                case CVM_CONSTANT_InterfaceMethodref: {
                    if (!CVMcpIsResolved(cp, i)) {
                        CVMprivate_cpResolveEntryWithoutClassLoading(
                            ee, cb, cp, i, NULL);
                    }
                    break;
                }
	        default:
                    break;
            }
        }
    }
}

/*
 * CVMcpResolveEntryWithoutClassLoading - resolves the specified cp entry,
 * but only if it can be done without causing any class loading. Returns
 * TRUE on success. If false is returned, then "p_typeid" will contain the
 * CVMMethodTypeID or CVMFieldTypeID of the entry if the entry is a MemberRef.
 * This is done because the typeID must be fetched while holding a lock.
 */
CVMBool
CVMprivate_cpResolveEntryWithoutClassLoading(CVMExecEnv* ee,
					     CVMClassBlock* currentCb, 
					     CVMConstantPool* cp, 
					     CVMUint16 cpIndex,
					     CVMTypeID* p_typeid)
{
    CVMClassTypeID  classID;/* id of the class referred to by the cp entry */
    CVMConstantPoolEntryType cpType;

    /*
     * We need to make sure the cp entry is not resolved and get its
     * contents while holding the lock. Otherwise it could get resolved
     * after we do the check, but before we grab the entry, in which
     * case the data we get is not what we expect.
     */
    CVMcpLock(ee);

    cpType = CVMcpEntryType(cp, cpIndex);
    if (cpType != CVM_CONSTANT_ClassTypeID && p_typeid != NULL) {
	*p_typeid = CVM_TYPEID_ERROR;
    }

    if (CVMcpIsResolved(cp, cpIndex)) {
	CVMcpUnlock(ee);
	return CVM_TRUE; /* it's already resolved */
#ifdef CVM_DUAL_STACK
    } else if (CVMcpIsIllegal(cp, cpIndex)) {
	CVMcpUnlock(ee);
	return CVM_FALSE; /* it cannot be resolved */
#endif
    }

    /* 
     * Get the classID of the class the entry refers to. This is key to
     * how this function works. If the class that is referred to is 
     * already loaded, then we can resolve this entry without causing
     * any class loading.
     */
    if (cpType == CVM_CONSTANT_ClassTypeID) {
	classID = CVMcpGetClassTypeID(cp, cpIndex);
	CVMassert(p_typeid == NULL);
    } else {
	CVMUint16 typeIDIdx;
	CVMUint16 classCpIndex;

	/* Get the classID of the class this entry refers to indirectly */
	classCpIndex = CVMcpGetMemberRefClassIdx(cp, cpIndex);
	if (!CVMcpIsResolved(cp, classCpIndex)) {
	    classID = CVMcpGetClassTypeID(cp, classCpIndex);
	} else {
	    classID = CVM_TYPEID_ERROR;
	}
	
	/* also set p_typeid so we can return it to the caller if needed */
	typeIDIdx = CVMcpGetMemberRefTypeIDIdx(cp, cpIndex);
	if (p_typeid != NULL) {
	    if (cpType == CVM_CONSTANT_Fieldref) {
		*p_typeid = CVMcpGetFieldTypeID(cp, typeIDIdx);
	    } else {
		*p_typeid = CVMcpGetMethodTypeID(cp, typeIDIdx);
	    }
	}
    }
    
    /* We are all done accessing the constant pool, so we can unlock now */
    CVMcpUnlock(ee);
	
    /*
     * If we got a classid, then we need to try to lookup the class without
     * doing any class loading. If we can't do this, then we cannot
     * resolve the entry without class loading.
     *
     * If compilingCausesClassLoading is true, then there is no need to
     * check if the class is already loaded.
     *
     * NOTE: This check for a loaded class isn't 100% accurate. It's possible
     * that the class is loaded, but loading has not yet been initiated by
     * the ClassLoader of currentCb.
     */
    if (classID != CVM_TYPEID_ERROR
#ifdef CVM_JIT
	&& !CVMglobals.jit.compilingCausesClassLoading
#endif
	)
    {
	CVMClassBlock* cb = NULL;
	CVMClassLoaderICell* currLoader = CVMcbClassLoader(currentCb);
	CVMBool isSystemClassLoader = CVM_FALSE;

	/*
	 * If the current class loader is the systemClassLoader, then that
	 * allows us to do preloader lookups and do loadcache lookups
	 * using the NULL class loader. This is because we know that the
	 * systemClassLoader always defers to the NULL class loader first.
	 * This helps us to resolve java.* and sun.* classes that are
	 * loaded, but haven't been referenced by the application yet.
	 */
	if (currLoader != NULL) {
	    CVMID_icellSameObject(ee, currLoader, CVMsystemClassLoader(ee),
				  isSystemClassLoader);
	}

	/*
	 * If the the class is primitive or the class
	 * is a primitive array, then we can do a preloader lookup first.
	 */
	if (CVMtypeidIsPrimitive(classID) || 
	    (CVMtypeidIsArray(classID) &&
	     CVMtypeidIsPrimitive(CVMtypeidGetArrayBasetype(classID))))
	{
	    cb = CVMpreloaderLookupFromType(ee, classID, NULL);
	} 

	if (cb == NULL) {
	    CVMBool needLoaderCacheUpdate = CVM_FALSE;
	    CVM_LOADERCACHE_LOCK(ee);
	    cb = CVMloaderCacheLookup(ee, classID, currLoader);
	    if (cb == NULL && isSystemClassLoader) {
		cb = CVMloaderCacheLookup(ee, classID, NULL);
		needLoaderCacheUpdate = (cb != NULL);
	    }
	    CVM_LOADERCACHE_UNLOCK(ee);
	    if (needLoaderCacheUpdate) {
		/* Add to the loadercache so when CVMcpResolveEntryFromClass()
		 * is called, we don't end up invoking currLoader.loadClass().
		 */
		if (!CVMloaderCacheAdd(ee, cb, currLoader)) {
		    /* clear exception and fetched cb */
		    CVMclearLocalException(ee);
		    cb = NULL;
		}
	    }
	}
	if ((cb == NULL) || !CVMcbCheckRuntimeFlag(cb, LINKED)) {
	    return CVM_FALSE; /* resolving would cause class loading */
	}
    }

    /*
     * If we get here it means that we can resolve this entry without
     * causing class loading, so just do it. However, it is possible for
     * an exception to occur due to access checks, so we need to check
     * for this and treat the entry as unresolved in this case.
     */
    if (!CVMcpResolveEntryFromClass(ee, currentCb, cp, cpIndex)) {
	CVMassert(CVMlocalExceptionOccurred(ee));
	CVMclearLocalException(ee);
	return CVM_FALSE;
    } else {
	return CVM_TRUE; /* entry is now resolved */
    }
}

/*
 * CVMprivate_cpExtractTypeIDFromUnresolvedEntry - extracts the typeID
 * from a yet-unresolved constant pool entry.
 *
 * Also handles the case where another thread beats us to resolving
 * this constant pool entry. Returns CVM_TRUE if the c.p. entry ended
 * up being resolved, and CVM_FALSE if it remained unresolved.  
 */
CVMBool
CVMprivate_cpExtractTypeIDFromUnresolvedEntry(CVMExecEnv* ee,
					      CVMClassBlock* currentCb, 
					      CVMConstantPool* cp, 
					      CVMUint16 cpIndex,
					      CVMTypeID* p_typeid)
{
    CVMConstantPoolEntryType cpType;

    /*
     * We need to make sure the cp entry is not resolved and get its
     * contents while holding the lock. Otherwise it could get resolved
     * after we do the check, but before we grab the entry, in which
     * case the data we get is not what we expect.
     */
    CVMcpLock(ee);

    cpType = CVMcpEntryType(cp, cpIndex);
    if (cpType != CVM_CONSTANT_ClassTypeID && p_typeid != NULL) {
	*p_typeid = CVM_TYPEID_ERROR;
    }

    if (CVMcpIsResolved(cp, cpIndex)) {
	/* it's already resolved by another thread. No need to assign
           to p_typeid */
	CVMcpUnlock(ee);
	return CVM_TRUE; 
    }

    /* 
     * Get the classID of the class the entry refers to. This is key to
     * how this function works. If the class that is referred to is 
     * already loaded, then we can resolve this entry without causing
     * any class loading.
     */
    if (cpType != CVM_CONSTANT_ClassTypeID) {
	CVMUint16 typeIDIdx;

	/* also set p_typeid so we can return it to the caller if needed */
	typeIDIdx = CVMcpGetMemberRefTypeIDIdx(cp, cpIndex);
	if (p_typeid != NULL) {
	    if (cpType == CVM_CONSTANT_Fieldref) {
		*p_typeid = CVMcpGetFieldTypeID(cp, typeIDIdx);
	    } else {
		*p_typeid = CVMcpGetMethodTypeID(cp, typeIDIdx);
	    }
	}
    } else {
        if (p_typeid != NULL) {
	    *p_typeid = CVMcpGetClassTypeID(cp, cpIndex);
        }
    }
    
    /* We are all done accessing the constant pool, so we can unlock now */
    CVMcpUnlock(ee);
	
    return CVMcpIsResolved(cp, cpIndex); /* final chance to find resolved
					    entry */
}

/*
 * CVMcpLookupClassAndVerifyAccess - Lookup the specified classID and
 * verify that the current class can access it.
 */
static CVMClassBlock*
CVMcpLookupClassAndVerifyAccess(CVMExecEnv* ee,
				CVMClassBlock* currentCb,
				CVMClassTypeID classID)
{
    CVMClassBlock* resolvedCb = 
	CVMclassLookupByTypeFromClass(ee, classID, CVM_FALSE, currentCb);
    if (resolvedCb == NULL) {
        /* Exception already thrown */
        return NULL;
    } else if (!CVMverifyClassAccess(ee, currentCb, resolvedCb, CVM_TRUE)){
	CVMthrowIllegalAccessError(ee, 
				   "try to access class %C from class %C",
				   resolvedCb, currentCb);
	return NULL;
    } else {
	return resolvedCb;
    }
}

/* 
 * CVMprivate_cpResolveEntryFromClass - resolve a constant pool entry.
 *   currentCb: the "current class" for security and class loading purposes.
 *   cp:        the constant pool
 *   cpIndex:   index of entry in cp to resolve
 * 
 * This function should only be called via the public CVMcpResolveEntry
 * and CVMcpResolveEntryFromClass macros.
 */

CVMBool
CVMprivate_cpResolveEntryFromClass(CVMExecEnv* ee,
				   CVMClassBlock* currentCb, 
				   CVMConstantPool* cp, 
				   CVMUint16 cpIndex)
{
    CVMConstantPoolEntryType cpType;
    CVMFieldBlock*  resolvedFb = NULL;
    CVMMethodBlock* resolvedMb = NULL;

    /* The cp index, classID, and cb for any class we may resolve, 
     * including the class referred to by a FieldRef or MethodRef.
     */
    CVMUint16       classCpIndex;
    CVMClassBlock*  resolvedCb;
    CVMClassTypeID  classID = CVM_TYPEID_ERROR;
    /*
     * We need to make sure the cp entry is not resolved and get its
     * contents while holding the lock. Otherwise it could get resolved
     * after we do the check, but before we grab the entry, in which
     * case the data we get is not what we expect.
     */
    CVMcpLock(ee);
    if (CVMcpIsResolved(cp, cpIndex)) {
	CVMcpUnlock(ee);
	return CVM_TRUE;
    }

    cpType = CVMcpEntryType(cp, cpIndex);
    switch(cpType) {
    case CVM_CONSTANT_ClassTypeID: {
	classID = CVMcpGetClassTypeID(cp, cpIndex);
	CVMcpUnlock(ee);

	classCpIndex = cpIndex;
	resolvedCb = 
	    CVMcpLookupClassAndVerifyAccess(ee, currentCb, classID);
	if (resolvedCb == NULL) {
	    /* Exception already thrown */
	    return CVM_FALSE;
	}
	
	break;
    }
    
    case CVM_CONSTANT_Fieldref:
    case CVM_CONSTANT_Methodref: 
    case CVM_CONSTANT_InterfaceMethodref: {
	CVMClassLoaderICell* currentLoader = 
	    (currentCb != NULL) ? CVMcbClassLoader(currentCb) : NULL;
	CVMUint16 typeIDIdx = CVMcpGetMemberRefTypeIDIdx(cp, cpIndex);
	classCpIndex  = CVMcpGetMemberRefClassIdx(cp, cpIndex);

	/* First, resolve reference to the class */
	if (!CVMcpIsResolved(cp, classCpIndex)) {
	    classID = CVMcpGetClassTypeID(cp, classCpIndex);
	    CVMcpUnlock(ee);
	
	    resolvedCb = 
		CVMcpLookupClassAndVerifyAccess(ee, currentCb, classID);
	    if (resolvedCb == NULL) {
		/* Exception already thrown */
		return CVM_FALSE;
	    }
	} else {
	    CVMcpUnlock(ee);
	    resolvedCb = CVMcpGetCb(cp, classCpIndex);
	}

	if (!CVMcbCheckRuntimeFlag(resolvedCb, LINKED)) {
	    if (!CVMclassLink(ee, resolvedCb, CVM_FALSE)) {
		return CVM_FALSE;
	    }
	}
	
	switch(cpType) {
	case CVM_CONSTANT_Fieldref:
#ifdef CVM_DUAL_STACK
            if (CVMcpIsIllegal(cp, cpIndex)) {
                CVMFieldTypeID fieldTypeID =
                    CVMcpGetFieldTypeID(cp, typeIDIdx);
                CVMthrowIllegalAccessError(
                    ee, "try to access field %C.%!F from class %C",
                    resolvedCb, fieldTypeID, currentCb);
                return CVM_FALSE;
	    }
#endif

	    resolvedFb = CVMcpResolveFieldref(ee, currentCb, cp, 
					      resolvedCb, typeIDIdx);
	    if (resolvedFb == NULL) {
		return CVM_FALSE;
	    }
	    /* Check loader constraints */
	    if (!CVMloaderConstraintsCheckFieldSignatureLoaders(
                    ee, CVMcpGetFieldTypeID(cp, typeIDIdx),
		    currentLoader,
		    CVMcbClassLoader(CVMfbClassBlock(resolvedFb)))) {
		return CVM_FALSE;
	    }
	    break;

	case CVM_CONSTANT_Methodref:
#ifdef CVM_DUAL_STACK
            if (CVMcpIsIllegal(cp, cpIndex)) {
                CVMMethodTypeID methodTypeID =
                    CVMcpGetMethodTypeID(cp, typeIDIdx);
                CVMthrowIllegalAccessError(
                    ee, "try to access field %C.%!M from class %C",
                    resolvedCb, methodTypeID, currentCb);
                return CVM_FALSE;
	    }
#endif

	    resolvedMb = CVMcpResolveMethodref(ee, currentCb, cp, 
					       resolvedCb, typeIDIdx);
	    if (resolvedMb == NULL) {
		return CVM_FALSE;
	    }
	    /* Check loader constraints */
	    if (!CVMloaderConstraintsCheckMethodSignatureLoaders(
                    ee, CVMcpGetMethodTypeID(cp, typeIDIdx),
	  	    currentLoader, 
		    CVMcbClassLoader(CVMmbClassBlock(resolvedMb)))) {
		return CVM_FALSE;
	    }
	    break;

	case CVM_CONSTANT_InterfaceMethodref: 
#ifdef CVM_DUAL_STACK
            if (CVMcpIsIllegal(cp, cpIndex)) {
                CVMMethodTypeID methodTypeID =
                    CVMcpGetMethodTypeID(cp, typeIDIdx);
                CVMthrowIllegalAccessError(
                    ee, "try to access field %C.%!M from class %C",
                    resolvedCb, methodTypeID, currentCb);
                return CVM_FALSE;
	    }
#endif

	    resolvedMb = CVMcpResolveInterfaceMethodref(ee, currentCb, cp,
							resolvedCb, typeIDIdx);
	    if (resolvedMb == NULL) {
		return CVM_FALSE;
	    }
	    /* Check loader constraints */
	    if (!CVMloaderConstraintsCheckMethodSignatureLoaders(
                    ee, CVMcpGetMethodTypeID(cp, typeIDIdx),
		    currentLoader,
		    CVMcbClassLoader(CVMmbClassBlock(resolvedMb)))) {
		return CVM_FALSE;
	    }
	    break;
	}

	break;
    }

    default: 
	CVMassert(CVM_FALSE);
	CVMcpUnlock(ee);
	return CVM_TRUE;
    }
	
    /*
     * We must set the new entry value and type atomically, so this
     * must be done inside the lock. Otherwise someone might think
     * this entry is not resolved when it really is. Checking if the
     * entry is already resolved just avoids asserts by CVMcpSetXXX.
     *
     * We always have to set a cb entry no matter what type of entry this
     * function was called to resolve, since they all either directly 
     * (for ClassTypeID entries) or indirectly (for FieldRef, Methodref,
     * and InterfaceMethodref entries) refer to a ClassTypeID entry.
     */
    CVMcpLock(ee);
    if (!CVMcpIsResolved(cp, classCpIndex)) {
	CVMcpSetCb(cp, classCpIndex, resolvedCb);
    } else {
	classID = CVM_TYPEID_ERROR; /* so we don't try to dispose it later */
    }
    if (!CVMcpIsResolved(cp, cpIndex)) {
	/*
	 * NOTE: since a CVM_CONSTANT_ClassTypeID would have just gotten
	 * resolved above, here we only need to check whether or not we
	 * are resolving a FieldRef.
	 */
	if (cpType == CVM_CONSTANT_Fieldref) {
	    /* Constant pool entries for static fields may be changed
	     * to point directly to the static data by the quickening code.
	     * Comment c08/26/99 in quicken.c explains why. For now it
	     * must be set to the fb.
	     */
	    CVMcpSetFb(cp, cpIndex, resolvedFb);
	} else {
	    CVMassert(cpType == CVM_CONSTANT_Methodref ||
		      cpType == CVM_CONSTANT_InterfaceMethodref);
	    CVMcpSetMb(cp, cpIndex, resolvedMb);
	}
    }
    CVMcpUnlock(ee);

    /*
     * Free the constantpool reference to the classID, but only if we
     * resolved a class entry. Note that we deferred doing this until
     * after releasing the cpLock. This probably isn't totally necessary,
     * but it means we hold the lock less and avoid any possible rank error.
     */
    if (classID != CVM_TYPEID_ERROR) {
	CVMtypeidDisposeClassID(ee, classID);
    }
    
    return CVM_TRUE;
}

static CVMFieldBlock*
CVMcpFindFieldInClass(CVMExecEnv* ee,
                      CVMClassBlock* targetCb,
                      CVMFieldTypeID fieldTypeID)
{
    CVMInt32 fieldIdx;
    for (fieldIdx = CVMcbFieldCount(targetCb) - 1; fieldIdx >=0; fieldIdx--) {
        CVMFieldBlock* fb = CVMcbFieldSlot(targetCb, fieldIdx);
        if (CVMtypeidIsSame(fieldTypeID, CVMfbNameAndTypeID(fb))) {
            return fb;
        }
    }
    return NULL;
}

/* Search fieldCb's direct superinterfaces for the field recursively */
static CVMFieldBlock*
CVMcpFindFieldInSuperInterfaces(CVMExecEnv* ee,
                                CVMClassBlock* fieldCb,
                                CVMFieldTypeID fieldTypeID)
{
    CVMFieldBlock* fb;
    CVMClassBlock* targetCb;
    int i;

    /* C stack redzone check */
    if (!CVMCstackCheckSize(CVMgetEE(),
            CVM_REDZONE_CVMcpFindFieldInSuperInterfaces,
            "CVMcpFindFieldInSuperInterfaces", CVM_FALSE)) {
        return NULL;
    }

    /* Search direct superinterfaces */
    for (i = 0; i < CVMcbImplementsCount(fieldCb); i++) {
        targetCb = CVMcbInterfacecb(fieldCb, i);
        fb = CVMcpFindFieldInClass(ee, targetCb, fieldTypeID);
        if (fb != NULL) {
            return fb;
        } else {
	    /* Search superinterface's superface */
            fb = CVMcpFindFieldInSuperInterfaces(
                            ee, targetCb, fieldTypeID);
            if (fb != NULL) {
                return fb;
            }
        }
    }
    return NULL;
}

/*
 * Returns the CVMFieldBlock specified by the nameAndType cp index
 * and fieldCb pair.
 */
static CVMFieldBlock*
CVMcpResolveFieldref(CVMExecEnv* ee,
		     CVMClassBlock* currentCb, 
		     CVMConstantPool* cp, 
		     CVMClassBlock* fieldCb,
		     CVMUint16 typeIDIdx)
{
    CVMFieldTypeID fieldTypeID = CVMcpGetFieldTypeID(cp, typeIDIdx);
    CVMClassBlock* targetCb = fieldCb;
    CVMFieldBlock* fb;

    /*
     * Field resolution order is according to the 2nd edition 
     * of VM spedification (5.4.3.2, p.167).
     */

resolveFieldref_loop: 
    if (targetCb != NULL) { 
        /* 1. Search the field's class or interface first. */
        fb = CVMcpFindFieldInClass(ee, targetCb, fieldTypeID);
        if (fb != NULL) {
            goto field_found;
        }

        /* 
         * 2. Otherwise, field lookup is applied recursively to the 
         * direct superinterfaces of the specified class or interface. 
         */
        fb = CVMcpFindFieldInSuperInterfaces(
                                ee, targetCb, fieldTypeID);
        if (fb != NULL) {
            goto field_found;
        }

        /* 
         * 3. Otherwise, field lookup is applied recursively to the
         * superclasses of the specified class. 
         */
        targetCb = CVMcbSuperclass(targetCb);
        goto resolveFieldref_loop;
    }

    /*
     * 4. Otherwise, field lookup fails. Throw an exception.
     */
    CVMthrowNoSuchFieldError(ee, "%C: field %!F not found", 
			     fieldCb, fieldTypeID);
    return NULL;

field_found:
    if (CVMverifyMemberAccess3(ee, currentCb, fieldCb, targetCb,
                              CVMfbAccessFlags(fb), CVM_TRUE, CVM_FALSE)) {
        return fb;
    } else {
        CVMthrowIllegalAccessError(
            ee, "try to access field %C.%F from class %C",
            targetCb, fb, currentCb);
        return NULL;
    }
}

/*
 * Returns the CVMMethodBlock specified by the nameAndType cp index
 * and methodCb pair.
 */
static CVMMethodBlock*
CVMcpResolveMethodref(CVMExecEnv* ee,
		      CVMClassBlock* currentCb, 
		      CVMConstantPool* cp, 
		      CVMClassBlock* methodCb,
		      CVMUint16 typeIDIdx)
{
    CVMMethodTypeID methodTypeID = CVMcpGetMethodTypeID(cp, typeIDIdx);
    CVMClassBlock*  cb = methodCb;
    CVMMethodBlock* mb;

    CVMInt32 access;

#undef IsProtected
#define IsProtected(x)	CVMmemberPPPAccessIs((x), METHOD, PROTECTED)

    /*
     * The method must not be a method of an interface class.
     */
    if (CVMcbIs(methodCb, INTERFACE)) {
	CVMthrowIncompatibleClassChangeError(ee, "%C", methodCb);
	return NULL;
    }

    /*
     * Search the method's class and all of its super classes
     * for a method with a matching typeID.
     */
    while (cb != NULL) {
	CVMInt32 methodIdx;
	for (methodIdx = CVMcbMethodCount(cb) - 1;
	     methodIdx >= 0; methodIdx--) {
	    mb = CVMcbMethodSlot(cb, methodIdx);
	    if (CVMtypeidIsSame(methodTypeID, CVMmbNameAndTypeID(mb))) {
		goto found;
	    }
	}
	cb = CVMcbSuperclass(cb);
    }

    /* search for miranda methods */
    {
	CVMUint16 n;	
	CVMUint16 objectMethods =
	    CVMcbMethodTableCount(CVMsystemClass(java_lang_Object));
	for (n = CVMcbMethodTableCount(methodCb) - 1; n >= objectMethods ;n--){
	    mb = CVMcbMethodTableSlot(methodCb, n);
	    if (CVMtypeidIsSame(methodTypeID, CVMmbNameAndTypeID(mb))) {
		/*
		 * If the mb is in the methodtable, but not declared,
		 * it must be a miranda method.
		 */
		CVMassert(CVMmbIsMiranda(mb));
		cb = CVMmbClassBlock(mb);
		goto found;
	    }
	}
    }
    
    /*
     * Method not found. Throw an exception.
     */
 fail:
    CVMthrowNoSuchMethodError(ee, "%C: method %!M not found", 
			      methodCb, methodTypeID);
    return NULL;

 found:
    /*
     * Check if erroneously "inheriting" a constructor.
     */
    if ((methodCb != cb) && CVMtypeidIsConstructor(CVMmbNameAndTypeID(mb))) {
	goto fail;
    }

    /* special addition for 4825010 (JDK 4750641) */
    /* Special case:  arrays always override "clone". JVMS 2.15.
     * If the resolved class is an array class, and the 
     * declaring class is java.lang.Object and the method is "clone", 
     * set the flags to public.
     * We'll check for the method name first, as that's most likely
     * to be false (so we'll short-circuit out of these tests).
     */
    access = CVMmbAccessFlags(mb);

    if (CVMtypeidIsSameName(CVMmbNameAndTypeID(mb), CVMglobals.cloneTid) &&
	(cb == CVMsystemClass(java_lang_Object)) &&
	CVMtypeidIsArray(CVMcbClassName(methodCb))) {
      CVMassert(IsProtected(access));
      access &= ~CVM_MEMBER_PPP_MASK;
      access |= CVM_METHOD_ACC_PUBLIC;
    }

    if (CVMverifyMemberAccess3(ee, currentCb, methodCb, cb, 
			      access, CVM_TRUE, CVM_FALSE)) {
	return mb;
    } else {
	CVMthrowIllegalAccessError(ee,
				   "try to access method %C.%M from class %C",
				   cb, mb, currentCb);
	return NULL;
    }
}

/*
 * Returns the CVMMethodBlock specified by the nameAndType cp index
 * and methodCb pair.
 */
static CVMMethodBlock*
CVMcpResolveInterfaceMethodref(CVMExecEnv* ee,
			       CVMClassBlock* currentCb, 
			       CVMConstantPool* cp, 
			       CVMClassBlock* methodCb,
			       CVMUint16 typeIDIdx)
{
    CVMUint16 intfIdx;
    CVMMethodTypeID methodTypeID = CVMcpGetMethodTypeID(cp, typeIDIdx);
    CVMInt32 methodIdx;

    /*
     * The method must a method of an interface class.
     */
    if (!CVMcbIs(methodCb, INTERFACE)) {
	CVMthrowIncompatibleClassChangeError(ee, "%C", methodCb);
	return NULL;
    }

    /* 
     * The CVMInterfaceTable of an interface contains this interface
     * in the first slot, and all of the superinterfaces in the
     * remaining slots. Check the interface and all of its superinterfaces
     * for a matching method.
     */
    for (intfIdx = 0; intfIdx < CVMcbInterfaceCount(methodCb); intfIdx++) {
	CVMClassBlock* intfCb = CVMcbInterfacecb(methodCb, intfIdx);	
	for (methodIdx = CVMcbMethodCount(intfCb) - 1; 
	     methodIdx >= 0; methodIdx--) {
	    CVMMethodBlock* mb = CVMcbMethodSlot(intfCb, methodIdx);
	    if (CVMtypeidIsSame(methodTypeID, CVMmbNameAndTypeID(mb))) {
		return mb;
	    }
	}
    }

    /* Now check java/lang/Object for the method per VM Spec 
       section 5.4.3.4. */
    {
        CVMClassBlock* ObjectCb = CVMsystemClass(java_lang_Object); 
        for (methodIdx = CVMcbMethodCount(ObjectCb) - 1;
             methodIdx >= 0; methodIdx--) {
             CVMMethodBlock* ObjectMb = CVMcbMethodSlot(ObjectCb, methodIdx);
             if (CVMtypeidIsSame(methodTypeID, CVMmbNameAndTypeID(ObjectMb))) {
                 return ObjectMb;
             }
        }
    }

    /*
     * Method not found. Throw an exception.
     */
    CVMthrowNoSuchMethodError(ee, "%C: method %!M not found", 
			      methodCb, methodTypeID);
    return NULL;
}

#endif /* CVM_CLASSLOADING */
