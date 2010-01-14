/*
 * @(#)preloader_impl.h	1.41 06/10/10
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
 * This file defines the contract between the ROMizer and
 * the code in preloader.c, which is its interface to the
 * rest of the VM. This file is essentially a private
 * communications between romclasses.c and preloader.c
 * Any other use indicates a problem!
 */

#ifndef _INCLUDED_PRELOADER_IMPL_H
#define _INCLUDED_PRELOADER_IMPL_H

#include "javavm/include/objects.h"

/* These must exactly match settings in objects.h */

/* In the following, we must use + instead of | or compiler thinks expression is
   not constant.  Will this work with all compilers? */
/*
 * make CVM ready to run on 64 bit platforms
 *
 * cb is a native pointer (CVMClassBlock*)
 * therefore the cast has to be CVMAddr which is 4 byte on
 * 32 bit platforms and 8 byte on 64 bit platforms
 */
#define CVM_OBJHDR_CLASS_INIT(cb) \
    ((CVMClassBlock*)(((char*)(cb)) + CVM_OBJECT_IN_ROM_MASK + CVM_DEFAULT_MARKED_TYPE))
#define CVM_OBJHDR_VARIOUS_INIT(hashCode) \
    (((hashCode) << 2) | CVM_LOCKSTATE_UNLOCKED)

#define CVM_ROM_OBJECT_HDR_INIT0(cbName,hashCode)		\
    {CVM_OBJHDR_CLASS_INIT(&(cbName)),				\
     CVM_OBJHDR_VARIOUS_INIT((hashCode))}

#define CVM_ROM_OBJECT_HDR_INIT(cb,hashCode)			\
    CVM_ROM_OBJECT_HDR_INIT0(cb##_Classblock, hashCode)

/*
 * Struct declaration for the java.lang.Class proxy objects
 * that the ROMizer instantiates, along with the initialization macro
 * NOTE: The field, classBlockPointer, appears in the Java class
 * as an "int", even though it is actually a C pointer. IT IS NOT
 * A JAVA REF!
 */
struct java_lang_Class {
    CVMObjectHeader    hdr;
    CVMClassBlock*     classBlockPointer;
    CVMObject*         classLoader;
#if JAVASE >= 16
    CVMObject* name;
#endif
#if JAVASE == 15
    CVMJavaInt classRedefinedCount;
    CVMJavaInt lastRedefinedCount;
#endif
#if JAVASE >= 15
    CVMObject* genericInfo;
    CVMObject* enumConstants;
    CVMObject* enumConstantDirectory;
    CVMObject* annotations;
    CVMObject* declaredAnnotations;
    CVMObject* annotationType;
#endif
};

#define CVM_CLASS_INIT(cbname,hashCode) \
    {CVM_ROM_OBJECT_HDR_INIT(java_lang_Class,hashCode),	\
     (CVMClassBlock*)(cbname), \
     NULL  \
    }

/*
 * Used by the preloader initialization code
 * to copy initialized data into writable space
 */
struct CVM_preloaderInitTriple {
    const void *	from;
    void *		to;
    int			count;
};

/*
 * The data structure for compressed immutable method tables
 */
/*
 * The immutable part of a method block. Once initialized (by JCC or
 * by the class loader), these fields are never written into.  
 */
struct CVMMethodBlockImmutableCompressed {
#ifdef CVM_METHODBLOCK_HAS_CB
    CVMClassBlock*   cbX; /* An extra pointer to a methodblock's class */
#endif
    CVMMethodTypeID  nameAndTypeIDX;/* name and type */
    CVMUint8  methodTableIndexX;/* index of this method into cb.methodTable */
    CVMUint8   entryIdxX;     /* A summary of argsSize, invokerIdx, accessFlags */
    CVMUint8   methodIndexX;  /* index into CB.methods[].mb[] of this method */
    CVMUint8  checkedExceptionsOffsetX; /* index into CB.checkedExceptions */
    union {   /* code specific method information */
	CVMJavaMethodDescriptor* jmd;        /* more info for java methods */
	CVMUint8*  nativeCode;       /* code for native methods */
	CVMAddr    methodSlotIndex;  /* for interface methods, the real 16-bit
				      * index of the method. This could be a
				      * CVMUint16, but it's easier for JCC if
				      * it's an CVMUint32.
				      */
	CVMMethodBlock* interfaceMb; /* for miranda methods, the interface mb
				      * the miranda methods was created for.
				      * Sometimes NULL. 
				      * See CVMclassPrepareInterfaces.
				      */
    } codeX;
};

typedef struct CVMMethodBlockImmutableCompressed CVMMethodBlockImmutableCompressed;

/*
 * A table of (argsSize, invokerIdx, accessFlags) triples. An index to
 * this table is stored in the compressed mb (the entryIdxX field above)
 *
 * This table is generated by JavaCodeCompact.
 */
extern const CVMUint16 argsInvokerAccess[];

/*
 * The classblock pointer at the start of the ROMized method array contains
 * a flag in the low-order bit. If this bit is 1, it means that all the mb's
 * in this method array are compressed. If this bit is 0, the mb's are the
 * uncompressed immutable portions of CVMMethodBlock.
 */
#define CVM_METHODARRAY_IS_COMPRESSED_MASK  0x1

#define CVM_INIT_METHODARRAY_CB_WITH_UNCOMPRESSED_MBS(cb)      (cb)
#define CVM_INIT_METHODARRAY_CB_WITH_COMPRESSED_MBS(cb)    	\
    ((CVMClassBlock*)(((CVMAddr)(cb)) + CVM_METHODARRAY_IS_COMPRESSED_MASK))

#define CVM_INIT_METHODBLOCK_IMMUTABLE_COMPRESSED(cb,			\
                             nameAndTypeID, methodTableIndex,		\
			     entryIdx,					\
			     methodIndex, checkedExceptionsOffset,	\
			     code)					\
    {									\
        CVM_INIT_METHODBLOCK_CB_FIELD(cb)     				\
	nameAndTypeID, methodTableIndex,				\
	entryIdx, methodIndex,						\
	checkedExceptionsOffset, {(CVMJavaMethodDescriptor*)code}	\
    }




/*
 * Finally, the data structures exported from romjava and used
 * by the preloader interface code.
 * The CVM_ROMClasses is now sorted by typeid.
 * The first part, non-array classes, can be used for direct
 * access with just a little computing. The rest should be
 * binsearched. Since there are far more 1-dimensional than
 * 2-dimensional arrays, they can be handled separately.
 * The constants CVM_{first,last}ROMVectorClass mark the
 * divisions.
 * Primitive classes, defined by the VM, are sorted before
 * any other classes. They precede CVM_firstROMNonPrimitiveClass;
 */

extern const CVMClassBlock * const CVM_ROMClassblocks[];
extern struct java_lang_Class * const CVM_ROMClasses;
extern const int CVM_firstROMNonPrimitiveClass;
extern const int CVM_firstROMSingleDimensionArrayClass;
extern const int CVM_lastROMSingleDimensionArrayClass;
extern const int CVM_nTotalROMClasses;

extern const int CVM_nROMStrings;
extern const int CVM_nROMStringsCompressedEntries;

extern struct java_lang_String * const CVM_ROMStrings;
/* The compressed versions of the strings instantiated by JCC */
extern const CVMUint16 CVM_ROMStringsMaster[];
/* The character array that is shared amongst all ROMized strings */
extern CVMArrayOfChar* const CVM_ROMStringsMasterDataPtr;

extern const struct CVM_preloaderInitTriple CVM_preloaderInitMap[];
extern const void* const CVM_preloaderInitMapForMbs[];

#endif /* _INCLUDED_PRELOADER_IMPL_H */
