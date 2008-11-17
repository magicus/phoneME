/*
 * @(#)typeid.h	1.52 06/10/10
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
 * This file gives the interface to the type system.
 */

#ifndef _INCLUDED_TYPEID_H
#define _INCLUDED_TYPEID_H

#include "javavm/include/defs.h"
#include "javavm/include/porting/ansi/stddef.h"	/* size_t */

/*
 * An CVMxxxTypeID has two parts: the name part and the type part.
 * The high-order 16 bits is the name part. It is an index into the
 * member-name table.
 * The low-order 16 bits is the type part. It is either a method-type,
 * which is an index into the method-type table, or it is a field-type,
 * in which case it can be decoded, as described below.
 * Because of this context-dependence, there are two type ID types.
 * Needless to say, when only type information is to be represented, the
 * high-order bits are ignored.
 */
/* 
 * A CVMTypeID type is used as the basis for  CVMMethodTypeID, CVMFieldTypeID,
 * or CVMClassTypeID as appropriate. Additionally two typedefs for the name
 * and type part of a typeid were added and a type which is used when both
 * types of part are acceptable. The use of these typedefs are enforced
 * throughout the code.
 */
typedef CVMUint32 CVMTypeID;
typedef CVMUint16 CVMTypeIDPart;

typedef CVMTypeID CVMMethodTypeID;
typedef CVMTypeID CVMFieldTypeID;
typedef CVMTypeID CVMClassTypeID;
typedef CVMTypeIDPart CVMTypeIDNamePart;
typedef CVMTypeIDPart CVMTypeIDTypePart;

/*
 * This value is returned by any of the lookup routines
 * when the entry cannot be found, or for allocation
 * errors for the underlying tables, or for a syntax
 * error on input (especially a malformed signature string)
 */

/* 
 * Use CVMTypeID instead of CVUint32.
 */
#define CVM_TYPEID_ERROR	((CVMTypeID)-1)

/*
 * This value is a limitation of the VM definition.
 * It is the deepest array you can have on any Java VM.
 * This is enforced by the verifier.
 */
#define CVM_MAX_ARRAY_DIMENSIONS 255

/*
 * A field type is a 16-bit cookie.
 * Many can be interpreted directly.
 * Others are in the form of table index or key,
 * that indicate where more data is buried.
 * These are (currently) in no special order.
 *
 * +--2--+----14-------+
 * | ary | basetype    |
 * +-----+-------------+
 * where:
 * ary -- if value is 3, then the basetype field is an
 *	  index into a table giving array depth & basetype. Otherwise,
 *	  this is the array depth, for 0 <= depth <= 2. This should cover
 *	  the majority of arrays.
 * basetype -- if it is a little number (in below list), then the base type
 *	  is self-evident. Otherwise it is an index into a table where
 *	  more information is kept.
 *
 * All this should be opaque to the client.
 */

/* there is no 0 type. if you see one, it is a bug */
#define CVM_TYPEID_NONE         0
#define CVM_TYPEID_ENDFUNC	1
#define CVM_TYPEID_VOID		2
#define CVM_TYPEID_INT		3
#define CVM_TYPEID_SHORT	4
#define CVM_TYPEID_CHAR		5
#define CVM_TYPEID_LONG		6
#define CVM_TYPEID_BYTE		7
#define CVM_TYPEID_FLOAT	8
#define CVM_TYPEID_DOUBLE	9
#define CVM_TYPEID_BOOLEAN	10
#define CVM_TYPEID_OBJ		11
#define CVM_TYPEID_LAST_PREDEFINED_TYPE CVM_TYPEID_OBJ

#define CVMtypeidIsPrimitive(t) \
	((((t) & CVMtypeidTypeMask) >= CVM_TYPEID_VOID) && \
	 (((t) & CVMtypeidTypeMask) <= CVM_TYPEID_BOOLEAN))
#define CVMtypeidIsBigArray(t)	\
	(((t)&CVMtypeidArrayMask)==CVMtypeidBigArray)

#define CVMtypeidIsArray( t ) (((t)&CVMtypeidArrayMask) != 0 )
#define CVMtypeidGetArrayDepth( t ) \
    (CVMtypeidIsBigArray(t)? CVMtypeidGetArrayDepthX(t) \
		    : (((t)&CVMtypeidArrayMask)>>CVMtypeidArrayShift))
#define CVMtypeidGetArrayBasetype( t )  \
    (CVMtypeidIsBigArray(t)? CVMtypeidGetArrayBasetypeX(t) \
		    : ((t)&CVMtypeidBasetypeMask))

extern int 		CVMtypeidGetArrayDepthX( CVMClassTypeID );
extern CVMClassTypeID	CVMtypeidGetArrayBasetypeX( CVMClassTypeID );

#define CVMtypeidEncodeBasicPrimitiveArrayType(primitiveBaseType) \
    (CVMassert(CVMtypeidIsPrimitive(primitiveBaseType)), \
     ((1 << CVMtypeidArrayShift) | (primitiveBaseType)))

/* 
 * Extracts the name part of a typeid. Expects an argument of type CVMTypeID
 * (or equivalent) and returns a CVMTypeIDNamePart.
 */
#define CVMtypeidGetNamePart(typeID) \
     ((CVMTypeIDNamePart) ((typeID) >> CVMtypeidNameShift))

/* 
 * Extracts the type part of a typeid. Expects an argument of type
 * CVMTypeID (or equivalent) and returns a CVMTypeIDTypePart. This
 * should not be confused with CVMtypeidGetType(), which returns a
 * CVMTypeID with the name part nulled out.  
 */
#define CVMtypeidGetTypePart(typeID) \
     ((CVMTypeIDTypePart) ((typeID) & CVMtypeidTypeMask))

/* 
 * Constructs a CVMTypeID (or equivalent) from a CVMTypeIDTypePart and a
 * CVMTypeIDNamePart.
 */
#define CVMtypeidCreateTypeIDFromParts(namePart, typePart) \
     ((((CVMTypeID) ((CVMTypeIDNamePart) (namePart))) << CVMtypeidNameShift) | ((CVMTypeIDTypePart) (typePart)))

/*
 * A limitation of the implementation
 */
#define CVM_TYPEID_MAX_BASETYPE 0x3fff


/*
 * Initialize the type Id system
 * Register some well-known typeID's 
 */
extern CVMBool
CVMtypeidInit(CVMExecEnv *ee);

/*
 * A second stage of type ID initialization.
 * Register all pre-loaded packages using
 * CVMpackagesAddEntry( pkgName, "<preloaded>" )
 */
extern void
CVMtypeidRegisterPreloadedPackages();

/*
 * Delete all allocated data
 * at VM shutdown.
 */
extern void
CVMtypeidDestroy();

/*
 * Make a method type ID out of a UTF8 method name and signature
 * CVMtypeidLookupMethodIDFromNameAndSig will find already-existing
 *	entries, and return the values. Use this when querying something
 *	that may exist, but which you do not plan to instantiate.
 * CVMtypeidNewMethodIDFromNameAndSig will find entries, inserting if
 *	necessary, and will increment reference counts. Use this when
 *	instantiating a new method.
 */
extern CVMMethodTypeID
CVMtypeidLookupMethodIDFromNameAndSig( CVMExecEnv *ee,
		    const CVMUtf8* memberName, const CVMUtf8* memberSig);
extern CVMMethodTypeID
CVMtypeidNewMethodIDFromNameAndSig( CVMExecEnv *ee,
		    const CVMUtf8* memberName, const CVMUtf8* memberSig);
/*
 * Manipulate the reference counts on existing method type IDs.
 * Use one when copying one. Use the other when unloading or otherwise
 * deleting the reference.
 */
extern CVMMethodTypeID
CVMtypeidCloneMethodID( CVMExecEnv *ee, CVMMethodTypeID cookie );

extern void
CVMtypeidDisposeMethodID( CVMExecEnv *ee, CVMMethodTypeID cookie );


/*
 * Make a field type ID out of a UTF8 field name and signature
 * CVMtypeidLookupFieldIDFromNameAndSig will find already-existing
 *	entries, and return the values. Use this when querying something
 *	that may exist, but which you do not plan to instantiate.
 * CVMtypeidNewFieldIDFromNameAndSig will find entries, inserting if
 *	necessary, and will increment reference counts. Use this when
 *	instantiating a new field.
 */
extern CVMFieldTypeID
CVMtypeidLookupFieldIDFromNameAndSig( CVMExecEnv *ee,
			const CVMUtf8* memberName, const CVMUtf8* memberSig);
extern CVMFieldTypeID
CVMtypeidNewFieldIDFromNameAndSig( CVMExecEnv *ee,
			const CVMUtf8* memberName, const CVMUtf8* memberSig);
/*
 * Manipulate the reference counts on existing field type IDs.
 * Use one when copying one. Use the other when unloading or otherwise
 * deleting the reference.
 */
extern CVMFieldTypeID
CVMtypeidCloneFieldID( CVMExecEnv *ee, CVMFieldTypeID cookie );

extern void
CVMtypeidDisposeFieldID( CVMExecEnv *ee, CVMFieldTypeID cookie );

/*
 * Make a class type ID out of a UTF8 class name. (This is equivalent to a field
 * type ID.)
 * CVMtypeidLookupClassID will find already-existing
 *	entries, and return the values. Use this when querying something
 *	that may exist, but which you do not plan to instantiate.
 * CVMtypeidNewClassID will find entries, inserting if
 *	necessary, and will increment reference counts. Use this when
 *	instantiating a new class.
 */
extern CVMClassTypeID
CVMtypeidLookupClassID( CVMExecEnv *ee, const char * name, int nameLength );

extern CVMClassTypeID
CVMtypeidNewClassID( CVMExecEnv *ee, const char * name, int nameLength );

/*
 * Manipulate the reference counts on existing class type IDs.
 * Use one when copying one. Use the other when unloading or otherwise
 * deleting the reference.
 */
extern CVMClassTypeID
CVMtypeidCloneClassID( CVMExecEnv *ee, CVMClassTypeID cookie );

extern void
CVMtypeidDisposeClassID( CVMExecEnv *ee, CVMClassTypeID cookie );

/*
 * Make a member name ID out of a UTF8 string. This can be either a method
 * name or a field name. It is >not< a class name, which is dealt with above.
 * CVMtypeidLookupMembername will find an already-existing
 *	entry, and return the value. Use this when querying something
 *	that may exist, but which you do not plan to instantiate.
 * CVMtypeidNewMembername will find an entry, inserting if
 *	necessary, and will increment the reference count. Use this when
 *	instantiating a new member.
 */
extern CVMTypeID
CVMtypeidLookupMembername( CVMExecEnv *ee, const char * name );

extern CVMTypeID
CVMtypeidNewMembername( CVMExecEnv *ee, const char * name );

/*
 * Manipulate the reference counts on existing member name IDs.
 * Use one when copying one. Use the other when unloading or otherwise
 * deleting the reference.
 */
extern CVMTypeID
CVMtypeidCloneMembername( CVMExecEnv *ee, CVMTypeID cookie );

extern void
CVMtypeidDisposeMembername( CVMExecEnv *ee, CVMTypeID cookie );

/*
 * A limitation of the implementation
 */
#define CVM_TYPEID_MAX_MEMBERNAME 0xfffe

/*
 * Are two TypeIDs' type components equal?
 * If they are proper entries in our system, the answer
 * is easily derived.
 */
#define CVMtypeidIsSameType(type1, type2) \
	(((type1) & CVMtypeidTypeMask)==((type2) & CVMtypeidTypeMask))

/*
 * Are two names equal?
 * If they are proper entries in our system, the answer
 * is easily derived.
 */
#define CVMtypeidIsSameName(type1, type2) \
	(((type1)&CVMtypeidNameMask) == ((type2)&CVMtypeidNameMask))

/*
 * Are two TypeIDs' name -and- type components equal?
 * If they are proper entries in our system, the answer
 * is easily derived.
 */
#define CVMtypeidIsSame(type1, type2) ((type1) == (type2))

/*
 * Returns true if the ID is for a finalizer method
 */
#define CVMtypeidIsFinalizer(type) \
     (CVMtypeidIsSameName(type, CVMglobals.finalizeTid))

/*
 * Returns true if the ID is for a constructor method
 */
#define CVMtypeidIsConstructor(type) \
     (CVMtypeidIsSameName(type, CVMglobals.initTid))

/*
 * Returns true if the ID is for a static initializer ("<clinit>" "()V")
 */
#define CVMtypeidIsStaticInitializer(type) \
     (CVMtypeidIsSame(type, CVMglobals.clinitTid))

/*
 * Like above, but ignores signature
 */
#define CVMtypeidIsClinit(type) \
     (CVMtypeidIsSameName(type, CVMglobals.clinitTid))

/*
 * Return only the type component of a typeid.
 * Useful if you want to go from field typeid -> class typeid.
 */
#define CVMtypeidGetType(type1) ((type1) & CVMtypeidTypeMask)

/*
 * Return only the primitive type component of a fieldID.
 */
#define CVMtypeidGetPrimitiveType(fid) \
    (CVMtypeidFieldIsRef(fid) ? CVM_TYPEID_OBJ : CVMtypeidGetType(fid))
/*
 * Returns the return type of a method type. 
 * This returns one of the CVM_TYPEID_ type syllables.
 */
extern char
CVMtypeidGetReturnType(CVMMethodTypeID type);

/*
 * Returns the total number of words that the method arguments occupy.
 *
 * WARNING: does not account for the "this" argument.
 */
extern CVMUint16
CVMtypeidGetArgsSize( CVMMethodTypeID methodTypeID );

#ifdef CVM_JIT
/*
 * Returns the total number of arguments that the method has.
 *
 * WARNING: does not account for the "this" argument.
 */
extern CVMUint16
CVMtypeidGetArgsCount( CVMMethodTypeID methodTypeID );
#endif

/*
 * Returns true if the ID is a double-word (long or double). 
 */
#define CVMtypeidFieldIsDoubleword( t ) \
    (( ((t)&CVMtypeidTypeMask) == CVM_TYPEID_LONG) || \
     ( ((t)&CVMtypeidTypeMask) == CVM_TYPEID_DOUBLE))

/*
 * Returns true if the ID is a ref. The first works for reference-typed
 * data types, and the second for method return types.
 * The field version is pretty trivial. The method version requires
 * more grubbing around.
 */

#define CVMtypeidFieldIsRef( t ) \
	( ((t)&CVMtypeidTypeMask) > CVMtypeidLastScalar)

extern CVMBool
CVMtypeidMethodIsRef(CVMMethodTypeID type);

extern size_t
CVMtypeidFieldTypeLength0(CVMFieldTypeID type, CVMBool isField);
extern size_t
CVMtypeidMethodTypeLength(CVMMethodTypeID type);
extern size_t
CVMtypeidMemberNameLength(CVMMethodTypeID type);
extern size_t
CVMtypeidClassNameLength(CVMClassTypeID type);

#define CVMtypeidFieldTypeLength(tid) \
    CVMtypeidFieldTypeLength0((tid), CVM_TRUE)
#define CVMtypeidClassNameLength(tid) \
    CVMtypeidFieldTypeLength0((tid), CVM_FALSE)
#define CVMtypeidFieldNameLength(tid) \
    CVMtypeidMemberNameLength((tid))
#define CVMtypeidMethodNameLength(tid) \
    CVMtypeidMemberNameLength((tid))

/*
 * Convert type ID to string for printouts
 */
extern CVMBool
CVMtypeidMethodTypeToCString(CVMMethodTypeID type, char* buf, int bufLength);

extern CVMBool
CVMtypeidFieldTypeToCString(CVMFieldTypeID type, char* buf, int bufLength);

extern CVMBool
CVMtypeidMethodNameToCString(CVMMethodTypeID type, char* buf, int bufLength);

extern CVMBool
CVMtypeidFieldNameToCString(CVMFieldTypeID type, char* buf, int bufLength);

extern CVMBool
CVMtypeidClassNameToCString(CVMClassTypeID type, char* buf, int bufLength);

/*
 * Variants of the above that
 * calculate the size of the necessary buffer and allocate it for you
 * using malloc(). 
 * Warning!
 * You are responsible for de-allocating the resulting object yourself!
 */

extern char *
CVMtypeidMethodTypeToAllocatedCString( CVMMethodTypeID type );

extern char *
CVMtypeidFieldTypeToAllocatedCString( CVMFieldTypeID type );

extern char *
CVMtypeidMethodNameToAllocatedCString( CVMMethodTypeID type );

extern char *
CVMtypeidFieldNameToAllocatedCString( CVMFieldTypeID type );

extern char *
CVMtypeidClassNameToAllocatedCString( CVMClassTypeID type );

/*
 * Reference counting functions, for use by class loader and class-garbage
 * collector.
 */
#define CVMtypeidFieldIncRef( t ) \
    ( ((t)&CVMtypeidBasetypeMask>CVMtypeidLastScalar) ? \
	    CVMtypeidIncrementFieldRefcount( t ) : 0 )
#define CVMtypeidFieldDecRef( t ) \
    ( ((t)&CVMtypeidBasetypeMask>CVMtypeidLastScalar) ? \
	    CVMtypeidDecrementFieldRefcount( t ) : 0 )

int	CVMtypeidIncrementFieldRefcount( CVMFieldTypeID );
int	CVMtypeidDecrementFieldRefcount( CVMFieldTypeID );

/*
 * CVMtypeidIncrementArrayDepth will find an entry, inserting if
 *	necessary, and will increment reference counts. 
 *	The resulting ID must be disposed of when finished. See
 *	CVMtypeidDisposeClassID. !!
 *
 */
extern CVMClassTypeID
CVMtypeidIncrementArrayDepth( CVMExecEnv *ee, CVMClassTypeID base, 
			      int depthIncrement );

/*
 * Compare the containing packages of a pair of class types.
 * In the case of array types, the package of the base type is used.
 */
extern CVMBool
CVMtypeidIsSameClassPackage( CVMClassTypeID classname1, 
			     CVMClassTypeID classname2 );

/*******************************************************************
 * TERSE SIGNATURES.
 *
 * A terse signature is a way of compactly representing enough of
 * the parameter-passing and value-returning type information of a method
 * to allow the passing of information between the Java stack and the C
 * stack. (Internally we use a terse signature, which we sometimes call a
 * Form, to represent part of type information.) A terse signature can
 * be retrieved from an CVMtypeidMethodTypeID, and information can be
 * extracted from it. In particular, it should be easy to iterate over
 * the parameter types, and to extract the return type. Here are
 * the interfaces (and macros) you need to do this. In all cases, they
 * type syllables returned are from the CVM_TYPEID_ set. All references,
 * including arrays, are represented as CVM_TYPEID_OBJ.
 */

typedef struct CVMterseSig {
    CVMUint32 *	datap;
    int		nParameters;
} CVMterseSig;

typedef struct CVMterseSigIterator {
    CVMterseSig thisSig;
    int		word;
    int		syllableInWord;
} CVMterseSigIterator;

void
CVMtypeidGetTerseSignature( CVMMethodTypeID tid, CVMterseSig* result );

void
CVMtypeidGetTerseSignatureIterator( CVMMethodTypeID tid, CVMterseSigIterator* result );

/*
 * The C terse signature iterator paradigm.
 */
#define CVM_TERSE_ITER_NEXT( tsi ) \
    ( ( ((tsi).syllableInWord>=8)? ((tsi).syllableInWord=0, (tsi).word++) : 0 ) , \
      ((tsi).thisSig.datap[(tsi).word]>>(4*((tsi).syllableInWord++)) )&0xf )
#define CVM_TERSE_ITER_RETURNTYPE( tsi ) ((tsi).thisSig.datap[0]&0xf )

/* Since syllable count always includes the return and
 * end-of-parameter-list marker, the parameter count is two less.
 */
#define CVM_TERSE_ITER_PARAMCOUNT( tsi ) (CVM_TERSE_PARAMCOUNT((tsi).thisSig))

/*
 * This macro operates on a terse signature, not a terse signature iterator.
 */
#define CVM_TERSE_PARAMCOUNT( ts ) ((ts).nParameters )

/*
 * FULL SIGNATURE ITERATION.
 * A full signature iterator is, very simply, a terse signature
 * iterator plus a list of object-types we keep on the side.
 * We run the terse iterator, and if it would return CVM_TYPEID_OBJ,
 * we replace that return value with the next value from the object-type
 * array.
 */

typedef struct CVMSigIterator {
    CVMterseSigIterator terseSig;
    CVMTypeIDTypePart*	parameterDetails;
    CVMClassTypeID      returnType;
    CVMClassTypeID      temp;
} CVMSigIterator;

void
CVMtypeidGetSignatureIterator( CVMMethodTypeID tid, CVMSigIterator* result );

#define CVM_SIGNATURE_ITER_NEXT( sigiter ) \
    (( ((sigiter).temp=CVM_TERSE_ITER_NEXT((sigiter).terseSig)) == CVM_TYPEID_OBJ )\
	? *((sigiter).parameterDetails++) \
	: (sigiter).temp )

#define CVM_SIGNATURE_ITER_RETURNTYPE( sigiter ) ((sigiter).returnType)

#define CVM_SIGNATURE_ITER_PARAMCOUNT( sigiter ) CVM_TERSE_ITER_PARAMCOUNT((sigiter).terseSig)


/*
 * Private to the implementation, exposed to make macros work.
 */

#define CVMtypeidNameShift	16
#define CVMtypeidArrayShift	14
    /*
     * This is how these masks are derived:
     * CVMtypeidTypeMask = (1<<CVMtypeidNameShift)-1
     * CVMtypeidNameMask = ~CVMtypeidTypeMask
     * CVMtypeidBasetypeMask = (1<<CVMtypeidArrayShift)-1
     * CVMtypeidArrayMask = ((1<<(CVMtypeidNameShift-CVMtypeidArrayShift))-1)
     *				<< CVMtypeidArrayShift;
     * CVMtypeidBigArray = CVYtypeidArrayMask
     * CVMtypeidMaxSmallArray = (CVMtypeidBigArray>>CVMtypeidArrayShift)-1
     */
#define CVMtypeidTypeMask	0xffff
#define CVMtypeidNameMask	0xffff0000
#define CVMtypeidArrayMask	0xc000
#define CVMtypeidBasetypeMask	0x3fff
#define CVMtypeidBigArray	0xc000
#define CVMtypeidLastScalar	CVM_TYPEID_BOOLEAN
#define CVMtypeidMaxSmallArray	2

#ifdef CVM_DEBUG

/*
 * print a little report of type table insertions and deletions
 * using CVMconsolePrintf. Resets the counters so that the next
 * call reports incremental numbers.
 */
extern void CVMtypeidPrintStats();

/*
 * print a more verbose report of type table insertions and
 * deletions. If verbose==0, only report the net changes
 * deletions cancel insertions. If verbose!=0, report all
 * changes.
 */
extern void CVMtypeidPrintDiffs( CVMBool verbose );

/*
 * Check integrety of (some) type tables:
 * follow hash chains and detect duplicates/merging/loops
 * make sure all reachable entries have non-zero ref count, and
 * that unreachable entries have zero ref count.
 */
extern void CVMtypeidCheckTables();

#endif


#endif /* _INCLUDED_TYPEID_H */
