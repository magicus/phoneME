/*
 * @(#)preloader.c	1.113 06/10/25
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
#include "javavm/include/typeid.h"
#include "javavm/include/typeid_impl.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/preloader.h"
#include "javavm/include/preloader_impl.h"
#include "javavm/include/stackmaps.h"
#include "javavm/include/utils.h"
#include "javavm/include/clib.h"
#include "javavm/include/string_impl.h"
#include "generated/offsets/java_lang_String.h"

#ifdef CVM_JIT
#include "javavm/include/porting/jit/ccm.h"
#endif

#ifdef CVM_HW
#include "include/hw.h"
#endif
#ifdef CVM_DEBUG_ASSERTS

static void
CVMpreloaderVerifyGCMap(const CVMClassBlock* cb);

static void
CVMpreloaderVerifyGCMaps();
#endif

extern const char *CVMROMClassLoaderNames;
extern CVMAddr * const CVMROMClassLoaderRefs;
extern const int CVMROMNumClassLoaders;

const char *
CVMpreloaderGetClassLoaderNames(CVMExecEnv *ee)
{
    return CVMROMClassLoaderNames;
}

void
CVMpreloaderRegisterClassLoaderUnsafe(CVMExecEnv *ee, CVMInt32 index,
    CVMClassLoaderICell *loader)
{
    CVMInt32 clIndex = index * 2;
    CVMInt32 pdIndex = clIndex + 1;
    CVMObjectICell *refStatics = (CVMObjectICell *)CVMROMClassLoaderRefs;
    CVMassert(index < CVMROMNumClassLoaders);
    CVMID_icellAssignDirect(ee, &refStatics[clIndex], loader);
    (void)pdIndex;
}

/*
 * Look up class by name. Only used from debuggers.
 */
#ifdef CVM_DEBUG
const CVMClassBlock* 
CVMpreloaderLookup(const char* className)
{
    CVMClassTypeID typeID = 
	CVMtypeidLookupClassID(NULL, className, (int)strlen(className) );
    if (typeID == CVM_TYPEID_ERROR) {
	return NULL; /* not in type table, so don't bother looking. */
    }
    return CVMpreloaderLookupFromType(CVMgetEE(), typeID, NULL);
}
#endif

static CVMClassBlock *
CVMpreloaderLookupFromType0(CVMClassTypeID typeID)
{
    CVMClassTypeID i, lowest, highbound;

    /* No CVMFieldTypeIDs allowed. Upper bits must never be set. */
    CVMassert(CVMtypeidGetType(typeID) == typeID);

    if (CVMtypeidIsPrimitive(typeID)) {
	/* NOTE: casting away const is safe here because we know that
           run-time flags are separated from ROMized classblocks */
	return (CVMClassBlock*) CVMterseTypeClassblocks[typeID];
    }

    /* NOTE: The Class instances of the Class and Array types in the
       CVM_ROMClasses[] are sorted in incremental order of typeid values.  The
       only exception is that deep array types (i.e. array types with depth >=
       3) will be sorted based only on the basetype field of their typeids
       (i.e. the depth field is ignored ... hence the masking operation in the
       computation of the index for the deep array case below).  This is what
       makes it possible to access Class instances for Class and Deep array
       types in CVM_ROMClasses[] by indexing. 

       The sorted order looks like this:

             .---------------------------------------.
             |  Primtive classes                     |
             |---------------------------------------|
             |  Regular Loaded classes               | <= e1
             |  e.g. java.lang.Object                |
             |                                       |
             |  Big Array classes                    |
             |  i.e. depth exceeds or equals the     |
             |  the value of CVMtypeArrayMask.       |
             |
             |---------------------------------------|
             |  Single Dimension Array classes       | <= e2
             |  e.g. [I or [Ljava.lang.Object;       |
             |                                       | <= e3
             |---------------------------------------|
             |  Multi Dimension Array classes        |
             |  e.g. [[I or [[Ljava.lang.Object; or  |
             |       [[[I, etc.                      |
             |                                       |
             '---------------------------------------'
         
       In the above illustration, entry e1 is the first entry in the
       Regular Loaded classes section.  Entry e2 is the first entry in the
       Single Dimension Array classes section, and entry e3 is the
       last entry in this same section.

       The Regular Loaded classes section currectly also holds the Big
       Array classes whose dimension depth exceeds or equals the bits
       in CVMtypeArrayMask (currently 3).  In the current implemetation
       the Big Array classes always come at the end of this section after
       the Regular Loaded classes.

       CVM_firstNonPrimitiveClass to be the index of entry e1,
       CVM_firstSingleDimensionArrayClass to be the index of entry e2, and
       CVM_lastSingleDimensionArrayClass to be the index of entry e3. 
    */
    if ( !CVMtypeidIsArray(typeID)) {
	const CVMClassBlock * cb;
	
        /* If we get here, then we're looking for a normal class i.e. not
           a primitive class and not an array class.
           The index should be within the range of regular loaded (or
           preloaded in this case) classes if its there: */
	i = typeID-CVMtypeidLastScalar-1;
        if ((i < CVM_firstROMNonPrimitiveClass) ||
            (i >= CVM_firstROMSingleDimensionArrayClass)) {
	    return NULL; /* not here! */
	}
	cb = CVM_ROMClassblocks[i];
        CVMassert(cb == NULL || CVMcbClassName(cb) == typeID);
	return (CVMClassBlock*)cb;
    }

    if (CVMtypeidIsBigArray(typeID)){
        const CVMClassBlock * cb;

        /* If we get here, then we're looking for a Big Array class.
           The index should come before the first single dimension array
           class if it's there: */
        i = (typeID & CVMtypeidBasetypeMask)-CVMtypeidLastScalar-1;
        if (i >= CVM_firstROMSingleDimensionArrayClass) {
            return NULL; /* not here! */
        }
        cb = CVM_ROMClassblocks[i];
        CVMassert(CVMcbClassName(cb) == typeID);
        return (CVMClassBlock*)cb;
    }

    /*
     * It is either a one-dimensional array or a higher-dimensional
     * array. Choose the appropriate partition of the ROMClasses array
     * and binary search for it.
     */
    if (CVMtypeidGetArrayDepth(typeID) == 1) {
        /* If we get here, we are looking for a single dimension array class.
           The index should be between the first and last single dimension
           array class if its there: */
        lowest = CVM_firstROMSingleDimensionArrayClass;
        highbound= CVM_lastROMSingleDimensionArrayClass+1;
    } else {
        /* Else, we're looking for a multi-dimension (2 or more but less
           than the BigArray depth which is currently 4) array class.
           The index should be between the last single dimension
           array class and the end of the list if its there: */
        lowest = CVM_lastROMSingleDimensionArrayClass+1;
	highbound= CVM_nROMClasses;
    }

    /* Do a binary search to see if the sought array class is there: */
    while (lowest < highbound) {
	const CVMClassBlock * cb;
	int candidateID;
	i = lowest + (highbound-lowest)/2;
	cb = CVM_ROMClassblocks[i];
	candidateID = CVMcbClassName(cb);
        if (candidateID == typeID) return (CVMClassBlock*)cb;
        if (candidateID < typeID) {
            lowest = i+1;
        } else {
            highbound = i;
        }
    }

    /*
     * Fell out of while loop.
     * not in table.
     */
    return NULL;
}

CVMClassBlock*
CVMpreloaderLookupFromType(CVMExecEnv *ee,
    CVMClassTypeID typeID, CVMClassLoaderICell *loader)
{
    CVMBool match;
    CVMClassBlock *cb = CVMpreloaderLookupFromType0(typeID);
    if (cb == NULL) {
	return cb;
    }
    CVMID_icellSameObjectNullCheck(ee, loader, CVMcbClassLoader(cb), match);
    if (match) {
	return cb;
    }
    return NULL;
}

/*
 * Only for lookup of primitive VM-defined types, by type name.
 */
CVMClassBlock*
CVMpreloaderLookupPrimitiveClassFromType(CVMClassTypeID classType){
    int i;
    const CVMClassBlock* cb;

    i = classType-CVMtypeidLastScalar-1;
    if ( (i < 0) || ( i >= CVM_firstROMNonPrimitiveClass) ){
	return NULL; /* not here! */
    }
    cb = CVM_ROMClassblocks[i];
    CVMassert( CVMcbClassName(cb) == classType );
    return (CVMClassBlock*)cb;
}

#ifdef CVM_DEBUG_ASSERTS
void
CVMpreloaderCheckROMClassInitState(CVMExecEnv* ee)
{
    int i;
    for (i = 0; i < CVM_nROMClasses; ++i) {
	const CVMClassBlock* cb = CVM_ROMClassblocks[i];
	if (cb == NULL) {
	    continue;
	}
	CVMassert(CVMcbIsInROM(cb));
	CVMassert(CVMcbCheckRuntimeFlag(cb, LINKED));
	CVMassert(CVMcbGetClinitEE(ee, cb) == NULL);
	CVMassert(CVMcbHasStaticsOrClinit(cb)
	    || CVMcbInitializationDoneFlag(ee, cb));
	CVMassert(!CVMcbCheckErrorFlag(ee, cb));
    }
}
#endif /* CVM_DEBUG_ASSERTS */

#ifdef CVM_JIT
static void
setJitInvoker(CVMMethodBlock* mb)
{
    /* First check for CNI or JNI */
    if (CVMmbIs(mb, NATIVE)) {
	if (CVMmbInvokerIdx(mb) == CVM_INVOKE_CNI_METHOD) {
	    CVMmbJitInvoker(mb) = (void*)CVMCCMinvokeCNIMethod;
	} else {
	    /* JNI method */
	    CVMassert(CVMmbInvokerIdx(mb) == CVM_INVOKE_JNI_METHOD ||
		      CVMmbInvokerIdx(mb) == CVM_INVOKE_JNI_SYNC_METHOD);
	    CVMmbJitInvoker(mb) = (void*)CVMCCMinvokeJNIMethod;
	}
    } else {
	CVMmbJitInvoker(mb) = (void*)CVMCCMletInterpreterDoInvoke;
    }
}
#endif

static void
unpack(CVMMethodBlockImmutable* mbImm,
       CVMMethodBlockImmutableCompressed* mbComp)
{
#define CVM_DO_MB_UNPACK_FROM_COMPRESSED(fieldName) \
    mbImm->fieldName = mbComp->fieldName;
#ifdef CVM_METHODBLOCK_HAS_CB
    CVM_DO_MB_UNPACK_FROM_COMPRESSED(cbX);
#endif
    CVM_DO_MB_UNPACK_FROM_COMPRESSED(nameAndTypeIDX);
    CVM_DO_MB_UNPACK_FROM_COMPRESSED(methodTableIndexX);
    CVM_DO_MB_UNPACK_FROM_COMPRESSED(methodIndexX);
    CVM_DO_MB_UNPACK_FROM_COMPRESSED(checkedExceptionsOffsetX);
    CVM_DO_MB_UNPACK_FROM_COMPRESSED(codeX.jmd);
    mbImm->argsSizeX = argsInvokerAccess[mbComp->entryIdxX * 3];

    CVMprivateMbImmSetInvokerAndAccessFlags(mbImm,
        argsInvokerAccess[mbComp->entryIdxX * 3 + 1],
        argsInvokerAccess[mbComp->entryIdxX * 3 + 2]);
}

#ifdef CVM_DEBUG_ASSERTS
CVMBool
CVMpreloaderReallyInROM(CVMObject* ref)
{
    extern char * const CVM_CharacterX_data_start;
    extern const  CVMUint32 CVM_CharacterX_data_size;
    extern char * const CVM_CharacterY_data_start;
    extern const  CVMUint32 CVM_CharacterY_data_size;
    extern char * const CVM_CharacterA_data_start;
    extern const  CVMUint32 CVM_CharacterA_data_size;
#ifndef CDC_10
    extern char * const CVM_CharacterLatinA_data_start;
    extern const  CVMUint32 CVM_CharacterLatinA_data_size;
    extern const char * const CVM_CharacterCharMap_data_start[];
    extern const CVMUint32 CVM_CharacterCharMap_data_size[];
    extern const CVMUint32 CVM_CharacterCharMap_number_of_elements;
    int i;
#endif

    if (CVMpreloaderIsPreloadedObject(ref)) {
	return CVM_TRUE;
    }
    
    if (((char*)ref >= CVM_CharacterX_data_start) &&
	((char*)ref <  CVM_CharacterX_data_start + CVM_CharacterX_data_size))
	return CVM_TRUE;
    
    if (((char*)ref >= CVM_CharacterY_data_start) &&
	((char*)ref <  CVM_CharacterY_data_start + CVM_CharacterY_data_size))
	return CVM_TRUE;
    
    if (((char*)ref >= CVM_CharacterA_data_start) &&
	((char*)ref <  CVM_CharacterA_data_start + CVM_CharacterA_data_size))
	return CVM_TRUE;

#ifndef CDC_10
    if (((char*)ref >= CVM_CharacterLatinA_data_start) &&
	((char*)ref <  CVM_CharacterLatinA_data_start + CVM_CharacterLatinA_data_size))
	return CVM_TRUE;

    for (i = 0; i < CVM_CharacterCharMap_number_of_elements; i++) {
       if (((char*)ref >= CVM_CharacterCharMap_data_start[i]) &&
           ((char*)ref <  CVM_CharacterCharMap_data_start[i] + CVM_CharacterCharMap_data_size[i]))
	   return CVM_TRUE;
    }
#endif

    return CVM_FALSE;
}
#endif

void
CVMpreloaderInit()
{
    /*
     * First, do a series of block initializations from read-only
     * master copies to BSS areas.
     */
    struct CVM_preloaderInitTriple* initTriples =
	    (struct CVM_preloaderInitTriple*)CVM_preloaderInitMap;
    void** t;

    /* 
     * Check that member various32 in struct CVMObjectHeader (which is used in
     * the "atomic compare and swap" routines) has platform specific alignment.
     */
    CVMassert(CVMalignAddrUp(CVMoffsetof(struct CVMObjectHeader, various32)) == 
	      CVMoffsetof(struct CVMObjectHeader, various32));
    CVMassert(CVMalignAddrUp(CVMoffsetof(CVMObject, hdr.various32)) == 
	      CVMoffsetof(CVMObject, hdr.various32));
    
    /*
     * Check that member elems in all CVMArrayOf<type> structs 
     * has the same offset. This is the case on 32 bit platforms 
     * but not automatically on 64 bit platforms.
     * Further check that the JCC generates coding for 
     * CVM_ROMStringsMaster_data that fits for struct CVMArrayOfChar.
     * The magic values may vary and has to be get from romjavaAux.c, 
     * initialization of CVM_ROMStringsMaster_data.
     */
    CVMassert(CVMoffsetof(CVMArrayOfAnyType, elems) == CVMoffsetof(CVMArrayOfByte, elems));
    CVMassert(CVMoffsetof(CVMArrayOfAnyType, elems) == CVMoffsetof(CVMArrayOfShort, elems));
    CVMassert(CVMoffsetof(CVMArrayOfAnyType, elems) == CVMoffsetof(CVMArrayOfChar, elems));
    CVMassert(CVMoffsetof(CVMArrayOfAnyType, elems) == CVMoffsetof(CVMArrayOfBoolean, elems));
    CVMassert(CVMoffsetof(CVMArrayOfAnyType, elems) == CVMoffsetof(CVMArrayOfInt, elems));
    CVMassert(CVMoffsetof(CVMArrayOfAnyType, elems) == CVMoffsetof(CVMArrayOfRef, elems));
    CVMassert(CVMoffsetof(CVMArrayOfAnyType, elems) == CVMoffsetof(CVMArrayOfFloat, elems));
    CVMassert(CVMoffsetof(CVMArrayOfAnyType, elems) == CVMoffsetof(CVMArrayOfLong, elems));
    CVMassert(CVMoffsetof(CVMArrayOfAnyType, elems) == CVMoffsetof(CVMArrayOfDouble, elems));

    /*
     * check CVM_FIELD_OFFSET macro:
     * CVMObjectHeader is a multiple of the size of CVMJavaVal32 
     */
    CVMassert( sizeof(CVMObjectHeader) % sizeof(CVMJavaVal32) == 0);
    
    while (initTriples->to != NULL){
	if (initTriples->from == NULL){
	    memset(initTriples->to, 0, initTriples->count);
	} else {
	    memcpy(initTriples->to, initTriples->from, initTriples->count);
	}
	initTriples++;
    }

    /*
     * Initialize class instances. Just iterate over all classblocks,
     * find the java instances, and fill them in.
     */
    {
	int i;
	CVMInt32 randomHash = CVMrandomNext();
	CVMClassBlock* classcb =
	    CVM_OBJHDR_CLASS_INIT(CVMsystemClass(java_lang_Class));

	/* Make sure that the random number generator (RNG) has been
	   initialized.  We test this my fetching the next random number.
	   It is not probable that the next random number is identical to the
	   current one.  If it happens to be, then we call the RNG again for
	   yet another number.  If we get 5 same numbers consecutively, then
	   either the RNG is not initialized, or it is not very random at all.
	   Either case, we will fail the assertion in that case.  We need to
	   assert this because the initialization of ROMized object hashcodes
	   below depends on this functionality.

	   NOTE: This assertion is meant to be a warning to the VM engineer
	         in case some changes are made to the RNG or its initialization
		 that causes it to be useless for the purpose of initializing
		 the romized object hashcodes.
	*/
	CVMassert((randomHash != CVMrandomNext()) ||
		  (randomHash != CVMrandomNext()) ||
		  (randomHash != CVMrandomNext()) ||
		  (randomHash != CVMrandomNext()));

	for (i = 0; i < CVM_nROMClasses; ++i) {
	    const CVMClassBlock *cb = CVM_ROMClassblocks[i];
	    struct java_lang_Class **cbInstanceICell;
	    struct java_lang_Class *cbInstance;
	    CVMInt32 hash;
	    int shift = i & 0x1f;

	    if (cb == NULL) {
		continue;
	    }

	    cbInstanceICell = (struct java_lang_Class **)CVMcbJavaInstance(cb);
	    cbInstance = *cbInstanceICell;

	    hash = ((CVMAddr)cbInstance >> 4) ^ ((CVMAddr)cb >> 5);
	    hash = (((CVMUint32)hash >> shift) |
		    ((CVMUint32)hash << (32 - shift)));
	    hash = hash ^ randomHash;

	    cbInstance->hdr.clas = classcb;
	    cbInstance->hdr.various32 = CVM_OBJHDR_VARIOUS_INIT(hash);
	    cbInstance->classBlockPointer = (CVMClassBlock*)cb;
	    cbInstance->classLoader = NULL;
	}
    }

    /*
     * Initialize String instances. Iterate over the compressed strings data,
     * and construct the matching Java instances
     */
    {
	CVMInt32 randomHash = CVMrandomNext();
	struct java_lang_String* strPtr = CVM_ROMStrings;
	CVMUint32 i, j;
	CVMUint32 offset = 0;
	CVMUint32 strLen;
	CVMUint32 numInstances;
	CVMClassBlock *cb =
	    CVM_OBJHDR_CLASS_INIT(CVMsystemClass(java_lang_String));
	
	for (i = 0; i < CVM_nROMStringsCompressedEntries * 2; i += 2) {
	    numInstances = CVM_ROMStringsMaster[i];
	    strLen       = CVM_ROMStringsMaster[i + 1];
	    
	    for (j = 0; j < numInstances; j++, offset += strLen, strPtr++) {
	        CVMInt32 hash;
		int shift = i & 0x1f;
		strPtr->hdr.clas = 
		    CVM_OBJHDR_CLASS_INIT(CVMsystemClass(java_lang_String));
		hash = ((CVMAddr)strPtr >> 2) ^
		       ((CVMAddr)cb << ((offset + strLen) & 0x1f));
		hash = ((CVMUint32)hash << shift) |
		       ((CVMUint32)hash >> (32 - shift));
		hash = hash ^ randomHash;

		strPtr->hdr.clas = cb;
		strPtr->hdr.various32 = CVM_OBJHDR_VARIOUS_INIT(hash);
		strPtr->value = (CVMArrayOfChar*)CVM_ROMStringsMasterDataPtr;
		strPtr->offset = offset;
		strPtr->count  = strLen;
	    }
	}
	CVMassert(strPtr == CVM_ROMStrings + CVM_nROMStrings);
    }
    

    /*
     * Now initialize method data structures.
     * For each method, copy the master immutable part, and fill in
     * the rest.
     */
    for (t = (void**)CVM_preloaderInitMapForMbs; *t != NULL; t++) {
	CVMUint8* masterData = (CVMUint8*)*t;
	CVMMethodBlock* destMethods;
	int i, count;
	CVMUint32 sizeofMasterDataElement;
	CVMBool compressed;

	/* 
	 * use CVMAddr for native pointer to be 64 bit clean
	 */
	CVMAddr theCbAsInt = *((CVMAddr*)masterData);
	CVMClassBlock* theCb;

	if ((theCbAsInt & CVM_METHODARRAY_IS_COMPRESSED_MASK) == 0) {
	    theCb = (CVMClassBlock*)theCbAsInt;
	    compressed = CVM_FALSE;
	    sizeofMasterDataElement = 
		sizeof(CVMMethodBlockImmutable);
	} else {
	    /* Get rid of flag in lowest bits */
	    theCbAsInt &= ~CVM_METHODARRAY_IS_COMPRESSED_MASK; 
	    theCb = (CVMClassBlock*)theCbAsInt;
	    compressed = CVM_TRUE;
	    sizeofMasterDataElement = 
		sizeof(CVMMethodBlockImmutableCompressed);
	}

	/*
	 * Get the list of destination methods from the class
	 */
	destMethods = (CVMMethodBlock*)CVMcbMethods(theCb);
	count = CVMcbMethodCount(theCb);
	
	/*
	 * Skip the class 
	 */
	masterData += sizeof(CVMClassBlock*);
	
	/* Now iterate over master copies of methods */
	for (i = 0; i < count; i++) {
	    if (i % 256 == 0) {
		CVMUint8* destData;
		/* Put in a cb pointer and move on */
		destData = (CVMUint8*)destMethods;
		*((CVMClassBlock**)destData) = theCb;
		destData += sizeof(CVMClassBlock*);
		destMethods = (CVMMethodBlock*)destData;
	    }
	    
	    /* Initialize mutable part */
	    memset(destMethods, 0, sizeof(CVMMethodBlock) -
		   sizeof(CVMMethodBlockImmutable));
	    
	    if (compressed) {
		unpack(&destMethods->immutX,
		       (CVMMethodBlockImmutableCompressed*)masterData);
	    } else {
		/* Copy immutable part */
		memcpy(&destMethods->immutX, masterData,
		       sizeof(CVMMethodBlockImmutable));
	    }
	    

#ifdef CVM_JIT
	    /* Set jitInvoker for all ROMized methods */
	    setJitInvoker(destMethods);
#endif
	    
	    /* Sanity check */
	    CVMassert(CVMmbClassBlock(destMethods) == theCb);
	    
	    /* Move on */
	    destMethods++;
	    masterData += sizeofMasterDataElement;
	}
    }

#if defined(CVM_DEBUG_ASSERTS) && !defined(CVM_LVM)
    /*
     * Sanity check for class state flags.
     * %begin lvm
     * In non-LVM mode, we can call this function with NULL argument 
     * since it will not depend on 'ee' during sanity checking.
     * %end lvm
     */
    CVMpreloaderCheckROMClassInitState(NULL);
#endif

#ifdef CVM_DEBUG_ASSERTS
    /*
     * Sanity check mb indexes
     */
    {
	int i, j;
	for (i = 0; i < CVM_nROMClasses; ++i) {
	    const CVMClassBlock* cb = CVM_ROMClassblocks[i];
	    if (cb == NULL || CVMcbIs(cb, PRIMITIVE) ||
		CVMcbMethodTablePtr(cb) == NULL)
	    {
		continue;
	    }
	    for (j = 0; j < CVMcbMethodTableCount(cb); ++j) {
		CVMMethodBlock *mb = CVMcbMethodTableSlot(cb, j);
		const CVMClassBlock* mcb = CVMmbClassBlock(mb);
		CVMassert(CVMmbMethodTableIndex(mb) == j);
		/* The following assert is only valid for the first 256
		 * methods of the class, since the methodIndex is always in
		 * the range of 0..255, no matter which slot the method is in.
		 */
		if (CVMcbMethodCount(mcb) < 256) {
		    CVMassert(mb == 
			      CVMcbMethodSlot(mcb,(short)CVMmbMethodIndex(mb)));
		}
	    }
	}
    }
#endif

#ifdef CVM_DEBUG_ASSERTS
    CVMpreloaderVerifyGCMaps();
#endif

#ifdef CVM_JIT_PATCHED_METHOD_INVOCATIONS
    /*
     * Iterate over all methods and detect override state. This is needed
     * so we can treat invokervirtual as invokenonvirtual if a method
     * is not overridden. If it is later overridden then the override flag
     * is set and we patch the invokenonvirtual locations to do an
     * invokevirtual.
     */
    {
    	int i, j;
	CVMExecEnv *ee = CVMgetEE();

    	for (i = 0; i < CVM_nROMClasses; ++i) {
	    const CVMClassBlock* cb 	= CVM_ROMClassblocks[i];
	    CVMClassBlock* scb;

	    if (cb == NULL || CVMcbIs(cb, INTERFACE)) {
		continue;
	    }

	    scb = CVMcbSuperclass(cb);

	    if (scb == NULL || CVMcbMethodTablePtr(scb) == NULL) {
		continue;
	    }

	    for (j = 0; j < CVMcbMethodTableCount(scb); ++j) {
	    	CVMMethodBlock *mb = NULL;
	    	CVMMethodBlock *smb = NULL;

	    	smb = CVMcbMethodTableSlot(scb, j);
	    	if (smb == NULL) {
		    continue;
		}

	    	/* If method is already overridden, continue. */
            	if (CVMmbCompileFlags(smb) & CVMJIT_IS_OVERRIDDEN) {
	    	    continue;
	    	}

	    	if (CVMmbIs(smb, ABSTRACT)) {
		    continue;
		}

	    	if (CVMcbMethodTablePtr(cb) == NULL) {
		    continue;
		}

		mb = CVMcbMethodTableSlot(cb, j);
	    	if (mb == NULL) {
		    continue;
		}

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

	    	if (!CVMmbIs(smb, PUBLIC) && !CVMmbIs(smb, PROTECTED) &&
		    !CVMisSameClassPackage(ee, CVMmbClassBlock(smb),
					   (CVMClassBlock*)cb)) {
		    continue;
	    	}

	    	if (smb != mb) {
	            CVMmbCompileFlags(smb) |= CVMJIT_IS_OVERRIDDEN;
	    	} 
	    }
    	}
    }
#endif /* CVM_JIT_PATCHED_METHOD_INVOCATIONS */
}

CVMBool
CVMpreloaderDisambiguateAllMethods(CVMExecEnv* ee)
{
    int i;

    for (i = 0; i < CVM_nROMClasses; ++i) {
	const CVMClassBlock* cb = CVM_ROMClassblocks[i];
	int nmethods;
	int j;

	if (cb == NULL) {
	    continue;
	}
	nmethods = CVMcbMethodCount(cb);
	for (j = 0; j < nmethods; j++) {
	    CVMMethodBlock* mb = CVMcbMethodSlot(cb, j);
	    if (CVMmbInvokerIdx(mb) < CVM_INVOKE_CNI_METHOD) {
		CVMJavaMethodDescriptor* jmd = CVMmbJmd(mb);
		/* Java method. If JIT is on, do a pre-pass to figure out
		   any special cases. */
		if ( CVMjmdFlags(jmd) & CVM_JMD_MAY_NEED_REWRITE ){
		    switch (CVMstackmapDisambiguate(ee, mb, CVM_TRUE)) {
			case CVM_MAP_SUCCESS:
#ifdef CVM_HW
			    CVMhwFlushCache(CVMmbJavaCode(mb),
					    CVMmbJavaCode(mb)
					    + CVMmbCodeLength(mb));
#endif
			    break;
			case CVM_MAP_OUT_OF_MEMORY:
			    CVMconsolePrintf(
			        "Ran out of memory while disambiguating %C.%M",
				cb, mb);
			    return CVM_FALSE;
			case CVM_MAP_EXCEEDED_LIMITS:
			    CVMconsolePrintf(
			        "Exceeded limits while disambiguating %C.%M",
				cb, mb);
			    return CVM_FALSE;
			case CVM_MAP_CANNOT_MAP:
			    CVMconsolePrintf(
				"Cannot disambiguate %C.%M",
				cb, mb);
			    return CVM_FALSE;
			default:
			    CVMassert(CVM_FALSE);
			    return CVM_FALSE;
		    }
		}
	    }
	}
    }
    return CVM_TRUE;
}

#ifdef CVM_JIT
void
CVMpreloaderInitInvokeCost()
{
    int i, j;
#ifdef CVM_MTASK
    CVMUint16 k = 0;
    /* We use an array to store the invokeCost values for
     * romized methods. The romized mb->invokeCostX is the
     * index to the array. See classes.h for details. The
     * invokeCostX (array index) is 16-bit, so make sure
     * the number of romized methods can fit into the array. */
    CVMassert(CVM_nROMMethods < (0x1 << 16));
#endif
    
    for (i = 0; i < CVM_nROMClasses; ++i) {
	const CVMClassBlock* cb = CVM_ROMClassblocks[i];
	if (cb == NULL) {
	    continue;
	}
	for (j = 0; j < CVMcbMethodCount(cb); ++j) {
	    CVMMethodBlock *mb = CVMcbMethodSlot(cb, j);
#ifndef CVM_MTASK
	    CVMmbInvokeCostSet(mb, CVMglobals.jit.compileThreshold);
#else
            /* Initialize mb->invokeCostX. It's the index to the
             * invokeCost array allocated for romized methods. */
            mb->invokeCostX = k;
            /* Set the initial invokeCost value. The value is stored
             * in the array. */
            CVMROMMethodInvokeCosts[k++] = CVMglobals.jit.compileThreshold;
#endif
	}
    }
#ifdef CVM_MTASK
    CVMassert(k == CVM_nROMMethods);
#endif
}
#endif

#ifdef CVM_DEBUG_ASSERTS

/*
 * For each romized class, verify that the romized gcmaps are the same
 * as the gcmap that the classloader would have generated if it had
 * dynamically loaded the class instead.
 */
static void
CVMpreloaderVerifyGCMaps()
{
    int i;
    for (i = 0; i < CVM_nROMClasses; ++i) {
	const CVMClassBlock* cb = CVM_ROMClassblocks[i];
	/* 
	 * We don't check java_lang_Object, array classes or
	 * primitive clases.
	 */
	if (cb != NULL && cb != CVMsystemClass(java_lang_Object) &&
	    !CVMisArrayClass(cb) && !CVMcbIs(cb, PRIMITIVE)) {
	    CVMpreloaderVerifyGCMap(cb);
	}
   }
}

/*
 * This code is basically the same is CVMclassPrepareFields(), minus
 * a few things like setting up static field offsets. Also, instead
 * of setting cb fields like the CVMcbInstanceSize() and CVMcbGcMap(),
 * it instead verifies that the computed values are the same as the
 * romized values.
 */
static void
CVMpreloaderVerifyGCMap(const CVMClassBlock* cb)
{
    CVMUint16      i;
    CVMClassBlock* superCb = CVMcbSuperclass(cb);
    CVMGCBitMap    superGcMap = CVMcbGcMap(superCb);
    CVMGCBitMap    gcMap;
    CVMUint16      numFieldWords;
    CVMUint16      numSuperFieldWords;
    CVMUint32      instanceSize;
    CVMUint32      fieldOffset; /* word offset for next field */
    CVMUint32      currBigMapIndex = 0;
    CVMUint16      nextMapBit;
    CVMBool        hasShortMap;
    /* 
     * Make 'map' the same type as 'map' in 'CVMGCBitMap'.
     */
    CVMAddr        map;  /* current map we are writing into. */

    numFieldWords = 
	(CVMcbInstanceSize(cb) - sizeof(CVMObjectHeader)) /
	 sizeof(CVMObject*);
    numSuperFieldWords = 
	(CVMcbInstanceSize(superCb) - sizeof(CVMObjectHeader)) /
	 sizeof(CVMObject*);

    /* Set instance size of the class. */
    instanceSize = 
	numFieldWords * sizeof(CVMObject*) + sizeof(CVMObjectHeader) ;

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
	
	/* Allocate space for the current class' long map. */
	gcMap.bigmap = (CVMBigGCBitMap*)
	    calloc(1, CVMoffsetof(CVMBigGCBitMap, map) +
		       maplen * sizeof(gcMap.bigmap->map[0]));
	if (gcMap.bigmap == NULL) {
	    /* just return. The caller will soon fail on another malloc,
	     * causing the VM to exit. */
	    return;
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
     * Iteterate over each field in the class and compute the gcmap for
     * the class.
     */
    for (i = 0; i < CVMcbFieldCount(cb) ; i++) {
	CVMFieldBlock* fb = CVMcbFieldSlot(cb, i);
	if (!CVMfbIs(fb, STATIC)) {
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
		    /* Is there any more work to be done? */
		    if (fieldOffset * sizeof(CVMObject*) < 
			CVMcbInstanceSize(cb)) {
			currBigMapIndex++;
			map = gcMap.bigmap->map[currBigMapIndex];
		    }
		}
	    } else {
		CVMassert(nextMapBit + CVM_GCMAP_NUM_FLAGS <= 32);
	    }
	}
    }

    CVMassert(fieldOffset * sizeof(CVMObject*) == CVMcbInstanceSize(cb));

    /*
     * Store the calculated gc map into gcMap and set gc map flags.
     */
    if (hasShortMap) {
	gcMap.map =
	    (map << CVM_GCMAP_NUM_FLAGS) | CVM_GCMAP_SHORTGCMAP_FLAG;
	CVMassert(gcMap.map == CVMcbGcMap(cb).map);
    } else {
	int i;
	gcMap.bigmap->map[currBigMapIndex] = map;
	gcMap.map |= CVM_GCMAP_LONGGCMAP_FLAG;
	CVMassert((CVMcbGcMap(cb).map & CVM_GCMAP_LONGGCMAP_FLAG) != 0);
	CVMassert(CVMcbGcMap(cb).bigmap->maplen == gcMap.bigmap->maplen);
	for (i = 0; i < gcMap.bigmap->maplen; i++) {
	    CVMassert(CVMcbGcMap(cb).bigmap->map[i] = gcMap.bigmap->map[i]);
	}   
	/*
	 * CVMGCBitMap is a union of the scalar 'map' and the
	 * pointer 'bigmap'. So it's best to 
	 * use the pointer if we need the pointer
	 */
	free((char*)((CVMAddr)gcMap.bigmap & ~CVM_GCMAP_LONGGCMAP_FLAG));
    }
}

#endif

/*
 * Iterate over all preloaded classes,
 * and call 'callback' on each class.
 */
void
CVMpreloaderIterateAllClasses(CVMExecEnv* ee, 
			      CVMClassCallbackFunc callback,
			      void* data)
{
    int i;
    for (i = 0; i < CVM_nROMClasses; ++i) {
        if (CVM_ROMClassblocks[i] != NULL) {
            callback(ee, (CVMClassBlock*)CVM_ROMClassblocks[i], data);
        }
    }

}

#ifdef CVM_JAVASE_CLASS_HAS_REF_FIELD
void
CVMpreloaderScanPreloadedClassObjects(CVMExecEnv* ee,
                                      CVMGCOptions* gcOpts,
                                      CVMRefCallbackFunc callback,
                                      void* data)
{
    int i;
    for (i = 0; i < CVM_nROMClasses; ++i) {
        const CVMClassBlock *cb = CVM_ROMClassblocks[i];
        if (cb != NULL) {
	    CVMObject *obj = *(CVMObject**)CVMcbJavaInstance(cb);
            CVMobjectWalkRefsWithSpecialHandling(ee, gcOpts, obj,
                    CVMsystemClass(java_lang_Class), {
	        if (*refPtr != 0) {
	            (*callback)(refPtr, data);
	        }                            
	    }, callback, data);
        }
    }
}
#endif

#if defined(CVM_INSPECTOR) || defined(CVM_DEBUG_ASSERTS) || defined(CVM_JVMPI)

/* Purpose: Checks to see if the specified object is a preloaded object. */
/* Returns: CVM_TRUE is the specified object is a preloaded object, else
            returns CVM_FALSE. */
CVMBool CVMpreloaderIsPreloadedObject(CVMObject *obj)
{
    CVMBool result = CVM_FALSE;
    struct java_lang_Class *firstClass = &CVM_ROMClasses[0];
    struct java_lang_Class *afterLastClass =
					&CVM_ROMClasses[CVM_nROMClasses];
    struct java_lang_String *firstString = &CVM_ROMStrings[0];
    struct java_lang_String *afterLastString = &CVM_ROMStrings[CVM_nROMStrings];

    /* Check to see if the object is in the range of a preloaded class: */
    if (((struct java_lang_Class *)obj >= firstClass) &&
        ((struct java_lang_Class *)obj < afterLastClass)) {
        /* Check to see if the purported obj is aligned properly to be a
           ROMized class: */
	/* 
	 * obj and firstClass are native pointer
	 * therefore the cast has to be CVMAddr which is 4 byte on
	 * 32 bit platforms and 8 byte on 64 bit platforms
	 */
        CVMAddr objAddr = (CVMAddr)obj;
        CVMAddr firstClassAddr = (CVMAddr)firstClass;
        CVMUint32 sizeOfJavaLangClass = sizeof(struct java_lang_Class);
        if (((objAddr - firstClassAddr) % sizeOfJavaLangClass) == 0) {
            result = CVM_TRUE;
        }

    /* Check to see if the object is in the range of a preloaded string: */
    } else if (((struct java_lang_String *)obj >= firstString) &&
               ((struct java_lang_String *)obj < afterLastString)) {
        /* Check to see if the purported obj is aligned properly to be a
           ROMized string: */
	/* 
	 * obj and firstClass are native pointer
	 * therefore the cast has to be CVMAddr which is 4 byte on
	 * 32 bit platforms and 8 byte on 64 bit platforms
	 */
        CVMAddr objAddr = (CVMAddr)obj;
        CVMAddr firstStringAddr = (CVMAddr)firstString;
        CVMUint32 sizeOfJavaLangString = sizeof(struct java_lang_String);
        if (((objAddr - firstStringAddr) % sizeOfJavaLangString) == 0) {
            result = CVM_TRUE;
        }

    /* Check to see if the object is in char array of the ROMized string: */
    } else {
        CVMObject *charsArrayObj;
        CVMD_fieldReadRef((CVMObject *)firstString,
                          CVMoffsetOfjava_lang_String_value,
                          charsArrayObj);
	CVMassert(charsArrayObj != NULL);
        if (obj == charsArrayObj) {
            result = CVM_TRUE;
        }
    }

    return result;
}
#endif

#if defined(CVM_INSPECTOR) || defined(CVM_JVMPI) || defined(CVM_JVMTI)
/* Purpose: Iterate over all preloaded objects and calls Call callback() on
            each preloaded object. */
/* Returns: CVM_FALSE if exiting due to an abortion (i.e. the callback
            requested an abortion by returning CVM_FALSE).
            CVM_TRUE if exiting without an abortion. */
CVMBool
CVMpreloaderIteratePreloadedObjects(CVMExecEnv* ee, 
                                    CVMObjectCallbackFunc callback,
                                    void* callbackData)
{
    int i;
    /* Iterate over preloaded classes: */
    for (i = 0; i < CVM_nROMClasses; ++i) {
        const CVMClassBlock *cb = CVM_ROMClassblocks[i];
        if (cb != NULL) {
            CVMObject *currObj = *(CVMObject **)CVMcbJavaInstance(cb);
            CVMClassBlock *currCb = CVMobjectGetClass(currObj);
            CVMUint32  objSize = CVMobjectSizeGivenClass(currObj, currCb);
            if (!callback(currObj, currCb, objSize, callbackData)) {
                return CVM_FALSE;
            }
        }
    }

    /* Iterate over preloaded strings and their char arrays:
       NOTE: We're only interested in the first segment because that is the
             only one that is generated by the preloader: */
    {
        CVMInternSegment *curSeg;
        CVMSize capacity;
        CVMUint8 *refArray;
        CVMStringICell *stringICell;
        CVMBool charArrayReported = CVM_FALSE;

        curSeg = (CVMInternSegment*)&CVMInternTable;
        capacity = curSeg->capacity;
        refArray = CVMInternRefCount(curSeg);
        for (i = 0; i < capacity; i++) {
            if (refArray[i] != CVMInternUnused) {
                CVMObject *stringObj;
                CVMObject *charsArrayObj;
                CVMClassBlock *currCb;
                CVMUint32 objSize;

                /* Report the preloaded String object: */
                stringICell = &(curSeg->data[i]);
                stringObj = *(CVMObject **)stringICell;
                currCb = CVMobjectGetClass(stringObj);
                objSize = CVMobjectSizeGivenClass(stringObj, currCb);
                if (!callback(stringObj, currCb, objSize, callbackData)) {
                    return CVM_FALSE;
                }

                /* Report the preloaded Char array object:
                   NOTE: We only need to do this once because all the preloaded
                      strings refer to the same char array: */
                if (!charArrayReported) {
                    CVMD_fieldReadRef(stringObj, 
                                      CVMoffsetOfjava_lang_String_value,
                                      charsArrayObj);
                    currCb = CVMobjectGetClass(charsArrayObj);
                    objSize = CVMobjectSizeGivenClass(charsArrayObj, currCb);
                    if (!callback(charsArrayObj, currCb, objSize, callbackData)) {
                            return CVM_FALSE;
                    }
                    charArrayReported = CVM_TRUE;
                }
            }
        }
    }

    return CVM_TRUE;
}

#endif /* (defined(CVM_INSPECTOR) || defined(CVM_JVMPI)) */

/*
 * Free allocated data structures associated with preloaded classes.
 * At the moment, this consists of stackmaps and any java method descriptors
 * re-allocated by the stackmap disambiguator, and also compiled code.
 */
void
CVMpreloaderDestroy()
{
    int i;

    for (i = 0; i < CVM_nROMClasses; ++i) {
	const CVMClassBlock * cb = CVM_ROMClassblocks[i];
        if (cb != NULL) {
            CVMtraceClassLoading(("CL: Destroying methods for preloaded class %C (cb=0x%x)\n", cb, cb));
#ifdef CVM_JIT
            CVMclassFreeCompiledMethods(NULL, (CVMClassBlock*)cb);
#endif
            CVMclassFreeJavaMethods(NULL, (CVMClassBlock*)cb, CVM_TRUE);
        }
    }
}

/* Monitor and report "write" to the .bss region. */
#ifdef CVM_USE_MEM_MGR
#undef ALIGNED
#undef ALIGNEDNEXT
#define ALIGNED(addr) \
    (CVMAddr)((CVMUint32)addr & ~(CVMgetPagesize() - 1))
#define ALIGNEDNEXT(addr) \
    (CVMAddr)(((CVMUint32)addr + CVMgetPagesize() - 1) & \
                  ~(CVMgetPagesize() - 1))

extern struct java_lang_Class * const CVM_ROMClassBlocks;
extern struct pkg * const CVM_ROMpackages;
extern const int CVM_nROMpackages;
extern const int CVMROMGlobalsSize;

void
CVMmemRegisterBSS()
{
    CVMMemHandle *h;
    CVMconsolePrintf("Register BSS(0x%x ~ 0x%x) With Memory Manager ...\n",
                     (CVMAddr)CVM_ROMStrings,
                     (CVMAddr)CVM_ROMStrings + CVMROMGlobalsSize);
    h = CVMmemRegisterWithMetaData(
                   (CVMAddr)CVM_ROMStrings,
                   (CVMAddr)CVM_ROMStrings + CVMROMGlobalsSize,
                   CVM_MEM_EXE_BSS,
                   0);
    CVMmemSetMonitorMode(h, CVM_MEM_MON_ALL_WRITES);
}

/* Start and end of sections in the .bss region. */
CVMUint32 romStringStart = 0;
CVMUint32 romStringEnd = 0;
CVMUint32 staticDataStart = 0;
CVMUint32 staticDataEnd = 0;
CVMUint32 romClassStart = 0;
CVMUint32 romClassEnd = 0;
CVMUint32 romCBStart = 0;
CVMUint32 romCBEnd = 0;
CVMUint32 romPkgStart = 0;
CVMUint32 romPkgEnd = 0;
CVMUint32 romPkgHashStart = 0;
CVMUint32 romPkgHashEnd = 0;
CVMUint32 methodTypeHashStart = 0;
CVMUint32 methodTypeHashEnd = 0;
CVMUint32 memberNameHashStart = 0;
CVMUint32 memberNameHashEnd = 0;
CVMUint32 methodsStart = 0;
/* Maps for individual sections. */
CVMUint8 *mamap = NULL;
CVMUint8 *strmap = NULL;
CVMUint8 *sdmap = NULL;
CVMUint8 *cbmap = NULL;
CVMUint8 *clmap = NULL;
CVMUint8 *pkgmap = NULL;
CVMUint8 *pkghmap = NULL;
CVMUint8 *mthmap = NULL;
CVMUint8 *mnhmap = NULL;
/* Dirty page number for individual sections. */
int mdp = 0;
int strdp = 0;
int sddp = 0;
int cbdp = 0;
int cldp = 0;
int pkgdp = 0;
int pkghdp = 0;
int mthdp = 0;
int mnhdp = 0;

void
CVMmemBssWriteNotify(int pid, void *addr, void *pc, CVMMemHandle *h)
{
    CVMUint32 waddr = (CVMUint32)addr;
    char *sectionName = NULL;
    int p;

    /* NOTE: This is based on current layout of CVMROMGlobalState:
     *       CVM_ROMStrings array
     *       ...
     *       CVM_staticData array
     *       Method arrays
     *       CVM_ROMClassBlocks array
     *       CVM_ROMClasses array
     *       ...
     */

    if (mamap == NULL) {
        romStringStart = (CVMUint32)CVM_ROMStrings;
        romStringEnd = (CVMUint32)&CVM_ROMStrings[CVM_nROMStrings];
        staticDataStart = (CVMUint32)CVMpreloaderGetRefStatics();
        staticDataEnd =
            (CVMUint32)CVMcbMethods(CVMsystemClass(java_lang_Class));
        romClassStart = (CVMUint32)CVM_ROMClasses;
        romClassEnd = (CVMUint32)&CVM_ROMClasses[CVM_nROMClasses];
        romCBStart = (CVMUint32)CVM_ROMClassBlocks;
        romCBEnd = (CVMUint32)&CVM_ROMClassBlocks[CVM_nROMClasses];
        romPkgStart = (CVMUint32)CVM_ROMpackages;
        romPkgEnd = (CVMUint32)(CVM_ROMpackages + CVM_nROMpackages);
        romPkgHashStart = (CVMUint32)CVM_pkgHashtable;
        romPkgHashEnd = (CVMUint32)(CVM_pkgHashtable + NPACKAGEHASH);
        methodTypeHashStart = (CVMUint32)CVMMethodTypeHash;
        methodTypeHashEnd = (CVMUint32)(CVMMethodTypeHash + NMETHODTYPEHASH);
        memberNameHashStart = (CVMUint32)CVMMemberNameHash;
        memberNameHashEnd = (CVMUint32)(CVMMemberNameHash + NMEMBERNAMEHASH);
        methodsStart =
            (CVMUint32)CVMcbMethods(CVMsystemClass(java_lang_Class));
        mamap = (CVMUint8*)calloc(sizeof(CVMUint8),
                                  (ALIGNEDNEXT(romCBStart) -
                                   ALIGNED(methodsStart)) / 4096);
        strmap = (CVMUint8*)calloc(sizeof(CVMUint8),
                                   (ALIGNEDNEXT(romStringEnd) -
                                    ALIGNED(romStringStart)) / 4096);
        sdmap = (CVMUint8*)calloc(sizeof(CVMUint8),
                                  (ALIGNEDNEXT(staticDataEnd) -
                                   ALIGNED(staticDataStart)) / 4096);
        cbmap = (CVMUint8*)calloc(sizeof(CVMUint8),
                                  (ALIGNEDNEXT(romCBEnd) -
                                   ALIGNED(romCBStart)) / 4096);
        clmap = (CVMUint8*)calloc(sizeof(CVMUint8),
                                  (ALIGNEDNEXT(romClassEnd) -
                                   ALIGNED(romClassStart)) / 4096);
        pkgmap = (CVMUint8*)calloc(sizeof(CVMUint8),
                                   (ALIGNEDNEXT(romPkgEnd) -
                                    ALIGNED(romPkgStart)) / 4096);
        pkghmap = (CVMUint8*)calloc(sizeof(CVMUint8),
                                    (ALIGNEDNEXT(romPkgHashEnd) -
                                     ALIGNED(romPkgHashStart)) / 4096);
        mthmap = (CVMUint8*)calloc(sizeof(CVMUint8),
                                   (ALIGNEDNEXT(methodTypeHashEnd) -
                                    ALIGNED(methodTypeHashStart)) / 4096);
        mnhmap = (CVMUint8*)calloc(sizeof(CVMUint8),
                                   (ALIGNEDNEXT(memberNameHashEnd) -
                                    ALIGNED(memberNameHashStart)) / 4096);
    }

    if (waddr < romStringStart ||
        waddr > romStringStart + CVMROMGlobalsSize) {
        CVMassert(CVM_FALSE);
    } else if (waddr >= romStringStart && waddr < romStringEnd) {
        sectionName = "Rom String";
        p = (waddr - ALIGNED(romStringStart)) / 4096 - 1;
        if (strmap[p] == 0) {
            strmap[p] = 0x1;
            strdp ++;
        }
    } else if (waddr >= romClassStart && waddr < romClassEnd) {
        sectionName = "Rom Class";
        p  = (waddr - ALIGNED(romClassStart)) / 4096 -1;
        if (clmap[p] == 0) {
            clmap[p] = 0x1;
            cldp ++;
        }
    } else if (waddr >= romCBStart && waddr < romCBEnd) {
        sectionName = "Rom CB";
        p = (waddr -  ALIGNED(romCBStart)) / 4096 - 1;
        if (cbmap[p] == 0) {
            cbmap[p] = 0x1;
            cbdp ++;
        }
    } else if (waddr > staticDataStart && waddr < staticDataEnd) {
        sectionName = "staticData";
        p = (waddr - ALIGNED(staticDataStart)) / 4096 -1;
        if (sdmap[p] == 0) {
            sdmap[p] = 0x1;
            sddp ++;
        }
    } else if (waddr > methodsStart && waddr < romCBStart) {
        sectionName = "Rom methods";
        p = (waddr - ALIGNED(methodsStart)) / 4096 - 1;
        if (mamap[p] == 0) {
            mamap[p] = 0x1;
            mdp ++;
        }
    } else if (waddr >= romPkgStart && waddr < romPkgEnd) {
        sectionName = "Rom Package"; 
        p = (waddr - ALIGNED(romPkgStart)) / 4096 -1;
        if (pkgmap[p] == 0) {
            pkgmap[p] = 0x1;
            pkgdp ++;
        }
    } else if (waddr >= romPkgHashStart && waddr < romPkgHashEnd) {
        sectionName = "Package Hash";
        p = (waddr - ALIGNED(romPkgHashStart)) / 4096 - 1;
        if (pkghmap[p] == 0) {
            pkghmap[p] = 0x1;
            pkghdp ++;
        }
    } else if (waddr >= methodTypeHashStart && waddr < methodTypeHashEnd) {
        sectionName = "Method Type Hash";
        p = (waddr - ALIGNED(methodTypeHashStart)) / 4096 -1;
        if (mthmap[p] == 0) {
            mthmap[p] = 0x1;
            mthdp++;
        }
    } else if (waddr >= memberNameHashStart && waddr < memberNameHashEnd) {
        sectionName = "Member Name Hash";
        p = (waddr - ALIGNED(memberNameHashStart)) / 4096 -1;
        if (mnhmap[p] == 0) {
            mnhmap[p] = 0x1;
            mnhdp++;
        }
    }

    CVMconsolePrintf(
            "Process #%d (at 0x%x) is write into BSS 0x%x (%s).\n",
            pid, pc, addr, sectionName);

    return;
}

void
CVMmemBssReportWrites()
{
    int totalmbP = (ALIGNEDNEXT(romCBStart) -
                    ALIGNED(methodsStart)) / 4096;
    int totalstrP = (ALIGNEDNEXT(romStringEnd) -
                     ALIGNED(romStringStart)) / 4096;
    int totalsdP = (ALIGNEDNEXT(staticDataEnd) -
                    ALIGNED(staticDataStart)) / 4096;
    int totalcbP = (ALIGNEDNEXT(romCBEnd) -
                    ALIGNED(romCBStart)) / 4096;
    int totalclP = (ALIGNEDNEXT(romClassEnd) -
                    ALIGNED(romClassStart)) / 4096;
    int totalpkgP = (ALIGNEDNEXT(romPkgEnd) -
                     ALIGNED(romPkgStart)) / 4096;
    int totalpkghP = (ALIGNEDNEXT(romPkgHashEnd) -
                      ALIGNED(romPkgHashStart)) / 4096;
    int totalmthP = (ALIGNEDNEXT(methodTypeHashEnd) -
                     ALIGNED(methodTypeHashStart)) / 4096;
    int totalmnhP = (ALIGNEDNEXT(memberNameHashEnd) -
                     ALIGNED(memberNameHashStart)) / 4096;
    CVMconsolePrintf("  BSS Region\n");
    CVMconsolePrintf("	Rom String: Total page = %d, Dirty page = %d\n",
                     totalstrP, strdp);
    CVMconsolePrintf("	Rom Class: Total page = %d, Dirty page = %d\n",
                     totalcbP, cldp);
    CVMconsolePrintf("	Rom CB: Total page = %d, Dirty page = %d\n",
                     totalclP, cbdp);
    CVMconsolePrintf("	StaticData: Total page = %d, Dirty page = %d\n",
                     totalsdP, sddp);
    CVMconsolePrintf("	Rom Methods: Total page = %d, Dirty page = %d\n",
                     totalmbP, mdp);
    CVMconsolePrintf("	Rom Package: Total page = %d, Dirty page = %d\n",
                     totalpkgP, pkgdp);
    CVMconsolePrintf("	Package Hash: Total page = %d, Dirty page = %d\n",
                     totalpkghP, pkghdp);
    CVMconsolePrintf("	MethodType Hash: Total page = %d, Dirty page = %d\n",
                     totalmthP, mthdp);
    CVMconsolePrintf("	MemberName Hash: Total page = %d, Dirty page = %d\n",
                     totalmnhP, mnhdp);
}
#endif
