/*
 * @(#)typeid.h	1.52 06/10/10
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.  
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


/* TypeIDs: Logical Abstraction / What are they?
   ============================================
   TypeIDs are generally cookies that are used in the VM to quickly represent
   strings.  There are 3 types of strings that are represented in the system.
   These strings and their typeIDs are as follows:

   1. CVMClassTypeID  - represents names of classes.
   2. CVMFieldTypeID  - represents the pair of field name and field type.
   3. CVMMethodTypeID - represents the pair of method name and method signature.

   In the typeID code, we will use the some abbreviate terms to refer to the
   above types.  These abbreviated terms are used in the comments as well as
   the variable names.  These terms are:

   1. classID    - instances of CVMClassTypeID.
   2. fieldID    - instances of CVMFieldTypeID
   3. methodID   - instances of CVMMethodTypeID

   In addition, here are a few additional terms that have useful context:

   4. memberID   - represents fieldID and methodID.  Conceptually,
                   fieldIDs and methodIDs are all memberIDs (like fieldIDs
                   and methodIDs are subclasses of memberIDs).

                   memberIDs contains a name and type component.

                   For fieldIDs, the name component is the field name.  The
                   type component is the field type.

                   For methodIDs, the name component is the method name.
                   The type component is the method signature i.e. parameter
                   and return types.

   5. nameID     - represents the name part of the memberID.
   6. sigID      - represents the type part of the memberID.

   Hence, all fieldIDs and methodIDs have a nameID and sigID component.  One
   can get the nameID of any memberID by calling CVMtypeidGetMemberName() on
   the memberID.  Similarly, the sigID can be be attained by calling
   CVMtypeidGetMemberType() on the memberID.

   NOTE: the sigID for fieldIDs are tracked in the same type database as
   classIDs.  Hence, it is valid to compare them against each other.


   What about CVMTypeID itself?
   ===========================
   CVMTypeID is a union of most of the above types.  This is necessary only for
   the use of constant pool code.  This is because the CP APIs take only a CP
   index and is expected to possibly produce a classID, fieldID, or memberID
   depending on the type of the CP entry.  The CVMTypeID is used as a vehicle
   for passing back any of these cookies as the result of the constant pool
   code.

   That said, note that there is actually no ambiguity in the usage of the
   CVMTypeID.  The caller of the CP code knows whether to expect a classID,
   fieldID, or methodID.

   WARNING: Just because a classID, fieldID, or methodID may be returned in
   the same CVMTypeID var, it doesn't mean that there is any meaning in
   accessing the result as a fieldID if it is expected to be a classID.
   Similarly, don't expect any meaning in access the result as a classID or
   methodID when it's not meant to be these.

   OK, now back to discussion about typeIDs in general ...


   How are typeIDs used?
   ====================
   TypeIDs are usually used as a quick way to compare strings.  For example,
   to compare class names, just compare their typeIDs.  However, this comparison
   is only done for testing equality.

   To compare 2 classIDs for equality, use CVMtypeidIsSameClass().
   To compare 2 fieldIDs for equality, use CVMtypeidIsSameField().
   To compare 2 methodIDs for equality, use CVMtypeidIsSameMethod().
   To compare 2 nameIDs for equality, use CVMtypeidIsSameName().
   To compare 2 sigIDs for equality, use CVMtypeidIsSameSig().

   There is no other comparisons that can be done.  Well, strictly speaking,
   that's not always true.  Here are some exceptional cases:

   1. Using classIDs as integer values for hashing or sorting
      =======================================================
      Hashing or sorting relies on integer ALU operations (e.g. modulo, or
      less/greater than comparisons) being done on the classID.

      To do these operations, use CVMtypeidGetToken() to get a unique integer
      token value that represents the classID.  This token value is of type
      CVMTypeIDToken which is an integer value.  Normally, CVMTypeIDToken is
      a CVMUint32.  When CVM_16BIT_TYPEID is defined, CVMTypeIDToken is a
      CVMUint16.

      NOTE: CVM_16BIT_TYPEID is currently an untested and unsupported mode.

      C switch statements
      ===================
      This the same case as the hashing / sorting case above where an integer
      value of the classID is needed to do the switch on.
      
   2. Using fieldIDs or methodIDs as integer values for hashing or sorting
      ====================================================================
      Similarly, we can use CVMtypeidGetMemberToken() on fieldIDs or methodIDs
      to get a unique token


   NOTE: these are exceptions rather than the norm.  These exceptions are there
   to serve special purposes.  In general, VM code should not use the techniques
   of these special cases unless when absolutely needed.  And when needed, you
   (the programmer) better really know what you are doing, or you will risk
   compromising the typeid system.  Think of this as an unsafe operation that
   must be used with care.

   NOTE: classID tokens are only unique with respect to other classIDs.
   fieldID tokens are only unique with respect to other fieldIDs.  And,
   methodID tokens are only unique with respect to other methodIDs.

   Do NOT mix their usage or you will get unpredictable results / failures.
   For example, it is meaningless to compare classID tokens with fieldID tokens
   or fieldID tokens with methodID tokens.

   3. Comparing memberID names only
      =============================
      One example of when you might want to do this is in identifying class
      constructors i.e. method name is <init> independent of the method
      signature.

      To compare member names only, you can use CVMtypeidIsSameMemberName().

      Comparing memberID types only
      =============================
      To compare member types only, you can use CVMtypeidIsSameMemberType().

   These name and type specific comparators are safe to use on memberIDs unlike
   the approached mentioned in (1) and (2) above which uses tokens.


   Physical Representation / How do they work?
   ==========================================
   There are 2 concrete types:

   1. CVMSimpleTypeID     - for classIDs, nameIDs, and sigIDs.
   2. CVMCompositeTypeID  - for memberIDs, fieldIDs, and methodIDs.

   Let's refer to these simply as simpleID and compositeID.

   A compositeID is made up of 2 simpleIDs: one for the nameID component, and
   one for the sigID component.

   NOTE: the code is set up to prevent simpleIDs from being casted into
   compositeIDs, and vice versa.  The code is set up to enforce this type
   safety for the purpose of avoiding unintended mix ups between classIDs and
   field/methodIDs.

   These mix ups can occur due to ambiguity that may have been introduced in
   the VM code due to the old preceeding typeid system which uses integer
   values for both classIDs and field/methodIDs.


   TypeID Type Safety
   ==================
   However, the type safety does not protect against deliberate action taken
   by the programmer to defeat it.  Such action would include using explicit
   casts to resolve C compiler warnings/errors that may arise due to erroneous
   use of the various typeIDs.  Hence, casting is generally frowned on.

   The rule of thumb (best practice) is that VM code should not need to do
   any casting for typeIDs.  The typeID system should provide all the macros /
   functions needed to convert between typeIDs correctly.

   If the typeID system does not have a needed API, chances are you might be
   trying to do something in a wrong way.  One example of this is the old way
   of creating a fieldID or methodID on the fly by simply concatenating a
   nameID and a sigID.  In this present typeID system, fieldIDs and methodIDs
   are abstract types that are constructed via some factory mechanism and
   is then passed around for use.  Think of them as actually being associated
   with some VM object or a piece of memory, and not just some integer token
   as in the old system.

   In the occasion that there really is a need to add a missing typeID API,
   please handle with care.  You will be working in an unsafe area.  Please
   know what you are doing to make sure that you don't accidentally defeat
   all the type safe protection that has been designed into the system.

   See also CVM_TYPEID_IS_STRONGLY_TYPED below.

   Internal Mechanics / What's under the covers?
   ============================================
   For implementation details, see comment block at the top of typeid.c.


   CVM_16BIT_TYPEID
   ================
   CVM_16BIT_TYPEID is provided to support the old legacy approach of typeIDs
   which are 16bit in size.

   In that scheme, CVMClassTypeID is a 32 bit value but actually only contains
   16bit of data in the low 16 bits.

   CVMFieldTypeID is a 32 bit value composed of a 16 bit nameID in the high
   16 bits, and a 16 bit fieldTypeID in the low 16.  The fieldTypeID is from
   the same range and type database as classIDs.

   CVMMethodTypeID is a 32 bit value composed of a 16 bit nameID in the high
   16 bits, and a 16 bit methodSigID in the low16.  The nameID is from the
   same range and type database as the field's nameIDs.  The methodSigID has
   its own range and type database.

   This option is disabled by default.  To enable this option, define the
   CVM_16BIT_TYPEID symbol at the compiler command line.  This will
   ensure that all files that depend on the symbol will be configured
   correctly.

   NOTE: enabling this option is not currently tested and supported.


   CVM_TYPEID_IS_STRONGLY_TYPED
   ============================
   CVM_TYPEID_IS_STRONGLY_TYPED specified whether to represent CVMSimpleTypeID
   in a struct for strong typing, or not.  The strong typing adds to the type
   safety protection that is designed into the present typeID system.

   NOTE: this option does not guarantee that all programmer errors will be
   detected.  A malicious or uninformed Programmers can still introduce
   errors by casting the types when it isn't appropriate to do so.  This
   option only provides a mechanism for helping detect errors.

   See also "TypeID Type Safety" above.

   This option is provided so that, when disabled, CVMSimpleTypeID can be
   defined as a simple integer type.  This may make a difference in terms of
   performance.  However, currently disabling is CVM_TYPEID_IS_STRONGLY_TYPED
   is not tested and supported.  Once this has been properly tested and
   supported, we should change the default below to disable it for non-debug
   builds.

   This option is enabled by default.  To disable it, #define the symbol to
   CVM_FALSE or 0.  This can be done at the compiler command line or here.
 */
#ifndef CVM_TYPEID_IS_STRONGLY_TYPED
#define CVM_TYPEID_IS_STRONGLY_TYPED    CVM_TRUE
#endif

#ifdef CVM_DEBUG_ASSERTS
#undef CVM_TYPEID_IS_STRONGLY_TYPED
#define CVM_TYPEID_IS_STRONGLY_TYPED    CVM_TRUE
#endif


/* NOTE: One of the main differences between the old 16 bit typeIDs and the
   new 32 bit typeIDs is the size of the basic typeID token.

   The other difference is the fact that 16 bit typeIDs require the use of
   Big Arrays typeIDs to represent array types with very deep aray dimensions.

   With 32 bit typeIDs, there is enough room for 8 bits of array depth in the
   the typeID.  This allows for a max depth of 255 array dimensions which is
   the maximum allowed the VM specification for classfiles.  Hence, there is
   no longer a need for Big Array support when 32 bit typeIDs are used.
*/
#ifdef CVM_16BIT_TYPEID
typedef CVMUint16 CVMTypeIDToken;
#else
typedef CVMUint32 CVMTypeIDToken;
#endif

#define CVM_INIT_TYPEID_TOKEN(_x)  ((CVMTypeIDToken)(_x))

/*============================================================================
  Definition of simple typeIDs:
*/

#ifdef CVM_TYPEID_IS_STRONGLY_TYPED
typedef struct CVMSimpleTypeID CVMSimpleTypeID;
struct CVMSimpleTypeID {
    union {
        CVMTypeIDToken token;

        /* This struct is for strong type static type verification only.
           It serves no other purpose in the typeID system.
         */
        struct {
            CVMUint8 v0;
            CVMUint8 v1;
            CVMUint8 v2;
            CVMUint8 v3;
        } verify;
    } u;
};

#define CVMtypeidVerifyIsSimpleTypeID(_simpleID) \
    ((void)(_simpleID).u.verify.v0, (void)(_simpleID).u.verify.v1, \
     (void)(_simpleID).u.verify.v2, (void)(_simpleID).u.verify.v3)

/* For declaring the initial value of a CVMSimpleTypeID in romizer generated
   const data: */
#define CVM_INIT_SIMPLE_TYPEID(_tokenX)  {{ ((CVMTypeIDToken)(_tokenX)) }}


#else /* !CVM_TYPEID_IS_STRONGLY_TYPED */

/* Regardless of whether we're using a 16bit or 32 bit typeID system, the
   CVMSimpleTypeID should always be the size of the native int of the system
   which is expected to be at least 32 bits:
*/
typedef CVMAddr CVMSimpleTypeID;

/* For declaring the initial value of a CVMSimpleTypeID in romizer generated
   const data: */
#define CVM_INIT_SIMPLE_TYPEID(_tokenX)  ((CVMTypeIDToken)(_tokenX))

/* Never complain when not strongly typed: */
#define CVMtypeidVerifyIsSimpleTypeID(_simpleID) ((void)(_simpleID))

#endif /* CVM_TYPEID_IS_STRONGLY_TYPED */

typedef CVMSimpleTypeID  CVMClassTypeID;
typedef CVMSimpleTypeID  CVMNameTypeID;
typedef CVMSimpleTypeID  CVMSigTypeID;


/*============================================================================
  Definition of composite typeIDs:
*/

#ifdef CVM_16BIT_TYPEID
typedef struct CVMCompositeTypeID CVMCompositeTypeID;
struct CVMCompositeTypeID {
    CVMTypeIDToken nameX;
    CVMTypeIDToken typeX;
};

#define CVMtypeidVerifyIsCompositeTypeID(_typeID) \
    ((void)(_typeID).nameX, (void)(_typeID).typeX)


typedef CVMCompositeTypeID  CVMMemberTypeID;

#else /* !CVM_16BIT_TYPEID */

typedef struct CVMCompositeTypeID CVMCompositeTypeID;
struct CVMCompositeTypeID {
    CVMSimpleTypeID nameX;
    CVMSimpleTypeID typeX;
};

#define CVMtypeidVerifyIsCompositeTypeID(_typeID) \
    ((void)(_typeID)->nameX, (void)(_typeID)->typeX)

typedef CVMCompositeTypeID  *CVMMemberTypeID;

#endif /* CVM_16BIT_TYPEID */

typedef CVMMemberTypeID  CVMMethodTypeID;
typedef CVMMemberTypeID  CVMFieldTypeID;

/* For declaring the initial value of a CVMCompositeTypeID in romizer generated
   const data: */
#define CVM_INIT_COMPOSITE_TYPEID(_nameX, _typeX) \
    { CVM_INIT_SIMPLE_TYPEID(_nameX), CVM_INIT_SIMPLE_TYPEID(_typeX) }


/*============================================================================
  A union typeID record for use by the constant pool resolution code which
  can return different typeID value types dependng on the content of the
  constant pool entry being resolved:

  NOTE: There should be no ambiguity still because the caller of the CP
  resolution function is still expecting a specific type of typeID back.
*/

typedef union CVMTypeID CVMTypeID;
union CVMTypeID {
    CVMMethodTypeID  methodID;
    CVMFieldTypeID   fieldID;
    CVMClassTypeID   classID;
};


/*============================================================================
  Macros for defining initialization values for the typeID values.  These are
  used in the romizer output which needs to specify constant typeID values.
*/

#define CVM_INIT_CLASSID(_x)  CVM_INIT_SIMPLE_TYPEID(_x)
#define CVM_INIT_MEMBERID(_x) ((CVMMemberTypeID)(_x))

/*============================================================================
  TypeID error codes: 
*/

/*
 * This value is returned by any of the lookup routines
 * when the entry cannot be found, or for allocation
 * errors for the underlying tables, or for a syntax
 * error on input (especially a malformed signature string)
 */

#define CVM_TYPEID_ERROR	((CVMTypeIDToken)-1)

#ifdef CVM_TYPEID_IS_STRONGLY_TYPED
extern const CVMSimpleTypeID CVM_SIMPLE_TYPEID_ERROR;
#else
#define CVM_SIMPLE_TYPEID_ERROR  CVM_INIT_SIMPLE_TYPEID(CVM_TYPEID_ERROR)
#endif

#define CVM_CLASS_TYPEID_ERROR  CVM_SIMPLE_TYPEID_ERROR
#define CVM_NAME_TYPEID_ERROR   CVM_SIMPLE_TYPEID_ERROR
#define CVM_SIG_TYPEID_ERROR    CVM_SIMPLE_TYPEID_ERROR

#define CVM_MEMBER_TYPEID_ERROR CVM_INIT_MEMBERID(CVM_TYPEID_ERROR)
#define CVM_FIELD_TYPEID_ERROR  CVM_MEMBER_TYPEID_ERROR
#define CVM_METHOD_TYPEID_ERROR CVM_MEMBER_TYPEID_ERROR

/*============================================================================
  The VM array depth limit: 
*/

/*
 * This value is a limitation of the VM definition.
 * It is the deepest array you can have on any Java VM.
 * This is enforced by the verifier.
 */
#define CVM_MAX_ARRAY_DIMENSIONS 255

/* CVM_TYPEID_NEED_BIG_ARRAY_SUPPORT is only true if we are using 16 bit
   typeIDs.  This is because with 32 bit typeIDs, we allot 8 bits in the
   CVMClassTypeID for the array depth.  This means that there will always
   be enough room to encode the array depth, and Big Array encodings are
   no longer needed:
*/
#ifdef CVM_16BIT_TYPEID
#define CVM_TYPEID_NEED_BIG_ARRAY_SUPPORT
#else
#undef CVM_TYPEID_NEED_BIG_ARRAY_SUPPORT
#endif

/*============================================================================
  How classID tokens are encoded:
*/

/*
   When CVM_16BIT_TYPEID is enabled, a field typeID or classID is a 16-bit
   cookie/token and is encoded as follows:

   +--2--+----14-------+  <== number of bits.
   | ary | basetype    |
   +-----+-------------+

   When using 32 bit typeIDs (i.e. CVM_16BIT_TYPEID is disabled), a classID is
   a 32 bit cookie/token and is encoded as follows:

   +--8--+----24-------+  <== number of bits.
   | ary | basetype    |
   +-----+-------------+

   where:
   ary -- if value is 3 in the 16 bit classID case, then the basetype field
          is an index into a table giving array depth & basetype.  Otherwise,
          this is the array depth, for 0 <= depth <= 2. This should cover
  	  the majority of arrays.

          For the 32 bit classID case, this is always the array depth where
          0 (i.e. not an array) <= depth <= 255.

   basetype -- if it is a little number (in below list), then the base type
  	  is self-evident. Otherwise it is an index into the
          CVMtypeidClassEntries table where more information is kept.

   All this (the bit encoding of typeIDs) should be opaque to the client i.e.
   other VM code outside of the typeID system/implementation.
*/

/*============================================================================
  Predefined CVMClassTypeID values:
*/

/* NOTE: once a typeID value is initialized, it should never be 0.  If you
   see one, it is a bug.
*/
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


/*============================================================================
  CVMClassTypeID getter/setter and conversion to/from tokens:

   NOTE: This token value is used for hashing / sorting or switch purposes
   only.  Do not pass it around in place of the classID.  Consider this an
   unsafe operation and handle with care (better know what you are doing)!

   NOTE: classID tokens are only unique with respect to other classIDs.  Do
   not mix their usage with memberID tokens.  Doing so will yield
   unpredictable results / failures.
*/
#ifdef CVM_TYPEID_IS_STRONGLY_TYPED
#define CVMtypeidGetToken(_tid)     ((_tid).u.token)
#define CVMtypeidSetToken(_tid, _token) {  \
        CVMtypeidVerifyIsSimpleTypeID(_tid); \
        (_tid).u.token = (_token);           \
    }

/* Purpose: Converts a token into a CVMClassTypeID.
   Ideally, this should be an inlined function.  It is expressed here as a
   static so that it can be inlined into the caller.  It cannot be expressed
   as a macro because it needs to define a classID local var for use in the
   conversion.
 */
extern CVMClassTypeID
CVMtypeidToken2ClassID(CVMTypeIDToken token);

#else /* !CVM_TYPEID_IS_STRONGLY_TYPED */
#define CVMtypeidGetToken(_tid)          (_tid)
#define CVMtypeidSetToken(_tid, _token)  ((_tid) = (_token))
#define CVMtypeidToken2ClassID(_token)   ((CVMClassTypeID)(_token))

#endif /* CVM_TYPEID_IS_STRONGLY_TYPED */


/*============================================================================
  CVMClassTypeID query macros:
*/

/* Purpose: Checks to see if the specified typeidToken contains the encoding of
   a primitive type.

   NOTE: This macro should only be called from typeid code (i.e. typeid.c and
         other typeid macros).  It should NEVER be called directly from other
         VM code in general.  Other VM code should use CVMtypeidIsPrimitive()
         instead.
*/
#define CVMtypeidTokenIsPrimitive(_token) \
     (((_token) >= CVM_TYPEID_VOID) && ((_token) <= CVM_TYPEID_BOOLEAN))

/* Purpose: Checks to see if the specified classID is a primitive type. */
#define CVMtypeidIsPrimitive(_classID) \
    (CVMtypeidVerifyIsSimpleTypeID(_classID), \
     CVMtypeidTokenIsPrimitive(CVMtypeidGetToken(_classID)))


/* Purpose: Indicates if the current configuration of the typeid system makes
   use of Big Arrays at all.  NOTE: This is a compile time macro which is
   expected to evaluate to a constant boolean at compile time.  It is used to
   compile out Big Array support code that will not be necessary if the system
   does not need Big Arrays.

   NOTE: Normally, if Big Arrays are needed, then the CVMtypeidMaxSmallArray
   array depth will be one less than the depth in CVMtypeidArrayMask.  This
   ensures that the max depth value encoding (which equals CVMtypeidArrayMask)
   will be used to encode Big Arrays.  But if the CVMtypeidMaxSmallArray array
   depth is the same as the depth value in CVMtypeidArrayMask, then that means
   the entire range of value in the depth field of CVMtypeidArrayMask can
   be used for small arrays, and there is no need for Big Arrays.

   However, for this macro, we determine whether Big Array support is needed
   simply by checking if the max small array depth is the max array dimensions
   allowed by the VM spec.  If so, then small arrays are adequate to encode
   all possible array dimensions, and there is no need to support Big Arrays.

   Another way is to conditionally define this macro depending on whether
   CVM_TYPEID_NEED_BIG_ARRAY_SUPPORT is #defined or not.
*/
#define CVMtypeidHasBigArrays() \
    (CVMtypeidMaxSmallArray != CVM_MAX_ARRAY_DIMENSIONS)

/* Purpose: Checks if the specified CVMTypeIDToken contains the encoding of a
   Big Array type.  This macro should only be called by typeid code (i.e.
   typeid.c or other typeid macros).  It should not be called directly by other
   code in the VM because they should avoid working directly with typeidTokens
   in general.
*/
#define CVMtypeidTokenIsBigArray(_token) \
    (CVMtypeidHasBigArrays() ? \
        (((_token) & CVMtypeidArrayMask) == CVMtypeidBigArray) : \
        CVM_FALSE)

/* Purpose: Checks if the specified CVMClassTypeID is a Big Array type. */
#define CVMtypeidIsBigArray(_classID) \
    (CVMtypeidVerifyIsSimpleTypeID(_classID), \
     CVMtypeidTokenIsBigArray(CVMtypeidGetToken(_classID)))

/* Purpose: Checks if the specified CVMClassTypeID is an Array type. */
#define CVMtypeidIsArray(_classID) \
    (CVMtypeidVerifyIsSimpleTypeID(_classID), \
     ((CVMtypeidGetToken(_classID) & CVMtypeidArrayMask) != 0))

/* Purpose: Gets the array depth of the specified array CVMClassTypeID. */
#define CVMtypeidGetArrayDepth(_arrayID) \
    (CVMtypeidVerifyIsSimpleTypeID(_arrayID), \
     (CVMtypeidIsBigArray(_arrayID)? CVMtypeidGetArrayDepthX(_arrayID) : \
      ((CVMUint32)(CVMtypeidGetToken(_arrayID) & CVMtypeidArrayMask) >> \
           CVMtypeidArrayShift)))

/* Purpose: Gets the array base type CVMClassTypeID of the specified array
   CVMClassTypeID. 
   NOTE: For example, the base type of [[[[I is I, not [[[I.
*/
#define CVMtypeidGetArrayBaseType(_arrayID) \
    (CVMtypeidVerifyIsSimpleTypeID(_arrayID), \
     (CVMtypeidIsBigArray(_arrayID) ? CVMtypeidGetArrayBaseTypeX(_arrayID) : \
          CVMtypeidToken2ClassID(CVMtypeidGetToken(_arrayID) &               \
                                 CVMtypeidBaseTypeMask)))


#ifdef CVM_TYPEID_NEED_BIG_ARRAY_SUPPORT
extern int 		CVMtypeidGetArrayDepthX(CVMClassTypeID arrayID);
extern CVMClassTypeID	CVMtypeidGetArrayBaseTypeX(CVMClassTypeID arrayID);
#else
/* These are just stubs to keep the C compiler parser happy.  These will never
   be called. */
#define CVMtypeidGetArrayDepthX(_arrayID)    0
#define CVMtypeidGetArrayBaseTypeX(_arrayID) CVM_CLASS_TYPEID_ERROR
#endif


#define CVMtypeidEncodeBasicPrimitiveArrayToken(primitiveBaseTypeToken) \
    (CVMassert(CVMtypeidTokenIsPrimitive(primitiveBaseTypeToken)), \
     ((1 << CVMtypeidArrayShift) | (primitiveBaseTypeToken)))


/*============================================================================
  CVMMemberTypeID query macros:
*/

/* Purpose: Extracts the nameID from a CVMMemberTypeID.  Expects an argument
   of type CVMMemberTypeID (or equivalent) and returns a CVMNameTypeID.
 */
#ifdef CVM_16BIT_TYPEID
#define CVMtypeidGetMemberName(_memberID) \
    (CVMtypeidVerifyIsCompositeTypeID(_memberID), (_memberID).nameX)

#else /* !CVM_16BIT_TYPEID */
#define CVMtypeidGetMemberName(_memberID) \
    (CVMtypeidVerifyIsCompositeTypeID(_memberID), \
     /* If the memberID is a small memberID encoding ...  */     \
     (((CVMAddr)(_memberID) & 0x1) != 0) ?                       \
     /* Then, compute the name from the encoded memberID: */     \
     CVMtypeidToken2ClassID((CVMTypeIDToken)(_memberID) >> 16) : \
     /* Else, fetch from the record: */                          \
     (_memberID)->nameX)

#endif /* CVM_16BIT_TYPEID */

/* Purpose: Extracts the sigID from a CVMMemberTypeID. Expects an argument
   of type CVMMemberTypeID (or equivalent) and returns a CVMSigTypeID.
*/
#ifdef CVM_16BIT_TYPEID
#define CVMtypeidGetMemberType(_memberID) \
    (CVMtypeidVerifyIsCompositeTypeID(_memberID), (_memberID).typeX)

#else /* !CVM_16BIT_TYPEID */

/* This should only be called by CVMtypeidGetMemberType(): */
#define CVMtypeidGetMemberTypeX(_memberID) \
    CVMtypeidToken2ClassID((CVMTypeIDToken) \
        ((((CVMTypeIDToken)(_memberID) >> 1) & 0x1fff) | \
         (((CVMTypeIDToken)(_memberID) << 10) & 0x03000000)))

#define CVMtypeidGetMemberType(_memberID) \
    (CVMtypeidVerifyIsCompositeTypeID(_memberID), \
     /* If the memberID is a small memberID encoding ...  */   \
     (((CVMAddr)(_memberID) & 0x1) != 0) ?                     \
     /* Then, compute the type from the encoded memberID: */   \
     CVMtypeidGetMemberTypeX(_memberID) :                      \
     /* Else, fetch from the record: */                        \
     (_memberID)->typeX)

#endif /* CVM_16BIT_TYPEID */


/* Purpose: Get a unique integer token value for the specified memberID.

   NOTE: This token value is used for hashing / sorting purposes only.
   Do not pass it around in place of the memberID.  Consider this an
   unsafe operation and handle with care (better know what you are doing)!

   NOTE: fieldID tokens are only unique with respect to other fieldIDs.
   And methodID tokens are only unique with respect to other methodIDs.
   Do not mix their usage with each other or with classID tokens.  Doing
   so will yield unpredictable results / failures.
*/
#define CVMtypeidGetMemberToken(_memberID) \
    ((CVMUint32)(_memberID))


/*============================================================================
  Miscellaneous:
*/

/*
 * A limitation of the implementation
 */
#ifdef CVM_16BIT_TYPE
#define CVM_TYPEID_MAX_BASETYPE 0x3fff
#else
#define CVM_TYPEID_MAX_BASETYPE 0xffffff
#endif

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
CVMtypeidLookupMethodIDFromNameAndSig(CVMExecEnv *ee,
		    const CVMUtf8* memberName, const CVMUtf8* memberSig);

extern CVMMethodTypeID
CVMtypeidNewMethodIDFromNameAndSig(CVMExecEnv *ee,
		    const CVMUtf8* memberName, const CVMUtf8* memberSig);

/*
 * Manipulate the reference counts on existing method type IDs.
 * Use one when copying one. Use the other when unloading or otherwise
 * deleting the reference.
 */
extern CVMMethodTypeID
CVMtypeidCloneMethodID(CVMExecEnv *ee, CVMMethodTypeID methodID);

extern void
CVMtypeidDisposeMethodID(CVMExecEnv *ee, CVMMethodTypeID methodID);


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
CVMtypeidLookupFieldIDFromNameAndSig(CVMExecEnv *ee,
			const CVMUtf8* memberName, const CVMUtf8* memberSig);

extern CVMFieldTypeID
CVMtypeidNewFieldIDFromNameAndSig(CVMExecEnv *ee,
			const CVMUtf8* memberName, const CVMUtf8* memberSig);
/*
 * Manipulate the reference counts on existing field type IDs.
 * Use one when copying one. Use the other when unloading or otherwise
 * deleting the reference.
 */
extern CVMFieldTypeID
CVMtypeidCloneFieldID(CVMExecEnv *ee, CVMFieldTypeID fieldID);

extern void
CVMtypeidDisposeFieldID(CVMExecEnv *ee, CVMFieldTypeID fieldID);

/*
 * Make a class type ID out of a UTF8 class name. (This is equivalent to a
 * field's sigID.)
 * CVMtypeidLookupClassID will find already-existing
 *	entries, and return the values. Use this when querying something
 *	that may exist, but which you do not plan to instantiate.
 * CVMtypeidNewClassID will find entries, inserting if
 *	necessary, and will increment reference counts. Use this when
 *	instantiating a new class.
 */
extern CVMClassTypeID
CVMtypeidLookupClassID(CVMExecEnv *ee, const char *name, int nameLength);

extern CVMClassTypeID
CVMtypeidNewClassID(CVMExecEnv *ee, const char *name, int nameLength);

/*
 * Manipulate the reference counts on existing class type IDs.
 * Use one when copying one. Use the other when unloading or otherwise
 * deleting the reference.
 */
extern CVMClassTypeID
CVMtypeidCloneClassID(CVMExecEnv *ee, CVMClassTypeID classID);

extern void
CVMtypeidDisposeClassID(CVMExecEnv *ee, CVMClassTypeID classID);

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
#if 0 /* NOT used */
extern CVMNameTypeID
CVMtypeidLookupMemberName(CVMExecEnv *ee, const char *name);
#endif /* NOT used */

extern CVMNameTypeID
CVMtypeidNewMemberName(CVMExecEnv *ee, const char *name);

/*
 * Manipulate the reference counts on existing member name IDs.
 * Use one when copying one. Use the other when unloading or otherwise
 * deleting the reference.
 */
#if 0 /* NOT used */
extern CVMNameTypeID
CVMtypeidCloneMemberName(CVMExecEnv *ee, CVMNameTypeID nameID);
#endif /* NOT used */

extern void
CVMtypeidDisposeMemberName(CVMExecEnv *ee, CVMNameTypeID nameID);

/*
 * A limitation of the implementation
 */
#ifdef CVM_16BIT_TYPEID
#define CVM_TYPEID_MAX_MEMBERNAME 0xfffe
#else
#define CVM_TYPEID_MAX_MEMBERNAME 0xfffffffe
#endif

/*============================================================================
  Misc comparators:
*/

/*
 * Are two TypeIDs' type components equal?
 * If they are proper entries in our system, the answer
 * is easily derived.
 */
#define CVMtypeidIsSameMemberType(member1, member2) \
    (CVMtypeidVerifyIsCompositeTypeID(member1), \
     CVMtypeidVerifyIsCompositeTypeID(member2), \
     CVMtypeidIsSameSig(CVMtypeidGetMemberType(member1), \
                        CVMtypeidGetMemberType(member2)))

/*
 * Are two names equal?
 * If they are proper entries in our system, the answer
 * is easily derived.
 */
#define CVMtypeidIsSameMemberName(member1, member2) \
    (CVMtypeidVerifyIsCompositeTypeID(member1), \
     CVMtypeidVerifyIsCompositeTypeID(member2), \
     CVMtypeidIsSameName(CVMtypeidGetMemberName(member1), \
                         CVMtypeidGetMemberName(member2)))

/*
 * Are two TypeIDs' name -and- type components equal?
 * If they are proper entries in our system, the answer
 * is easily derived.
 */
#define CVMtypeidIsSameSimple(type1, type2) \
    (CVMtypeidVerifyIsSimpleTypeID(type1), \
     CVMtypeidVerifyIsSimpleTypeID(type2), \
     CVMtypeidGetToken(type1) == CVMtypeidGetToken(type2))


#define CVMtypeidIsSameClass(type1, type2) CVMtypeidIsSameSimple(type1, type2)
#define CVMtypeidIsSameName(type1, type2)  CVMtypeidIsSameSimple(type1, type2)
#define CVMtypeidIsSameSig(type1, type2)   CVMtypeidIsSameSimple(type1, type2)


#define CVMtypeidIsSameMember(type1, type2) \
    (CVMtypeidVerifyIsCompositeTypeID(type1), \
     CVMtypeidVerifyIsCompositeTypeID(type2), \
     ((type1) == (type2)))

#define CVMtypeidIsSameField(type1, type2)  CVMtypeidIsSameMember(type1, type2)
#define CVMtypeidIsSameMethod(type1, type2) CVMtypeidIsSameMember(type1, type2)


/*============================================================================
  Misc methodID tests:
*/

/* Purpose: Checks if the specified method is a finalizer. */
#define CVMtypeidIsFinalizer(_methodID) \
    (CVMtypeidVerifyIsCompositeTypeID(_methodID), \
     (CVMtypeidIsSameMethod(_methodID, CVMglobals.finalizeTid)))

/* Purpose: Checks if the specified method is a constructor i.e. has name
   "<init>".  NOTE: we need to compare the name and not the whole methodID
   because any method that has that name (independent of the methodSig) is
   a constructor.
 */
#define CVMtypeidIsConstructor(_methodID) \
    (CVMtypeidVerifyIsCompositeTypeID(_methodID), \
     (CVMtypeidIsSameMemberName(_methodID, CVMglobals.initTid)))

/* Purpose: Checks if the specified method is a static initializer (i.e.
   "<clinit>" "()V") or not.
 */
#define CVMtypeidIsClinit(_methodID) \
    (CVMtypeidVerifyIsCompositeTypeID(_methodID), \
     (CVMtypeidIsSameMethod(_methodID, CVMglobals.clinitTid)))

/* Purpose: Checks if the specified method has a <clinit> name or not.
   NOTE: This is used specifically by verifier code that needs to ensure that
   the <clinit> name is treated specially i.e. a class is not allowed to
   define another method with the name <clinit> even if the methodSig is
   different.
 */
#define CVMtypeidHasClinitName(_methodID) \
    (CVMtypeidVerifyIsCompositeTypeID(_methodID), \
     (CVMtypeidIsSameMemberName(_methodID, CVMglobals.clinitTid)))


/* Purpose: Return the predefined type token of the specified classID.
   Since only CVM_TYPEID_OBJ is in the predefined list of classID tokens,
   all object types will be returned as CVM_TYPEID_OBJ.
*/
#define CVMtypeidGetPredefinedTypeToken(_classID) \
    (CVMtypeidVerifyIsSimpleTypeID(_classID), \
     CVMtypeidClassIsRef(_classID) ? \
         CVM_TYPEID_OBJ : CVMtypeidGetToken(_classID))

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
CVMtypeidGetArgsSize(CVMMethodTypeID methodTypeID);

#ifdef CVM_JIT
/*
 * Returns the total number of arguments that the method has.
 *
 * WARNING: does not account for the "this" argument.
 */
extern CVMUint16
CVMtypeidGetArgsCount(CVMMethodTypeID methodTypeID);
#endif

/*
 * Returns true if the ID is a double-word (long or double). 
 */
#define CVMtypeidFieldIsDoubleword(t) \
    (CVMtypeidVerifyIsCompositeTypeID(t), \
     ((CVMtypeidGetToken(CVMtypeidGetMemberType(t)) == CVM_TYPEID_LONG) || \
      (CVMtypeidGetToken(CVMtypeidGetMemberType(t)) == CVM_TYPEID_DOUBLE)))

/*
 * Returns true if the ID is a ref. The first works for reference-typed
 * data types, and the second for method return types.
 * The field version is pretty trivial. The method version requires
 * more grubbing around.
 */

/* Purpose: Checks if the specified token encodes a type which is not a
   primitive.

   NOTE: This macro should only be called by typeid code (i.e. typeid.c and
   typeid macros).  Other VM code should call one of CVMtypeidClassIsRef(),
   CVMtypeidFieldIsRef(), or CVMtypeidMethodIsRef() instead.
*/
#define CVMtypeidTokenIsRef(_token) \
    ((_token) > CVMtypeidLastPrimitive)

/* Purpose: Checks if the specified classID is for a reference type i.e. is
   not a primitive.
*/
#define CVMtypeidClassIsRef(_classID) \
    (CVMtypeidVerifyIsSimpleTypeID(_classID), \
     CVMtypeidTokenIsRef(CVMtypeidGetToken(_classID)))

#define CVMtypeidFieldIsRef(_fieldID) \
    (CVMtypeidVerifyIsCompositeTypeID(_fieldID), \
     CVMtypeidClassIsRef(CVMtypeidGetMemberType(_fieldID)))

extern CVMBool
CVMtypeidMethodIsRef(CVMMethodTypeID type);

extern size_t
CVMtypeidFieldTypeLengthX(CVMTypeIDToken classToken, CVMBool isField);
extern size_t
CVMtypeidMemberNameLength(CVMNameTypeID nameID);

/* extern size_t CVMtypeidClassNameLength(CVMClassTypeID type); */
#define CVMtypeidClassNameLength(classID) \
    CVMtypeidFieldTypeLengthX(CVMtypeidGetToken(classID), CVM_FALSE)

/* extern size_t CVMtypeidFieldNameLength(CVMFieldTypeID type); */
#define CVMtypeidFieldNameLength(fieldID) \
    CVMtypeidMemberNameLength(CVMtypeidGetMemberName(fieldID))

/* extern size_t CVMtypeidFieldTypeLength(CVMFieldTypeID type); */
#define CVMtypeidFieldTypeLength(fieldID) \
    CVMtypeidFieldTypeLengthX( \
        CVMtypeidGetToken(CVMtypeidGetMemberType(fieldID)), CVM_TRUE)

/* extern size_t CVMtypeidMethodNameLength(CVMMethodTypeID type); */
#define CVMtypeidMethodNameLength(methodID) \
    CVMtypeidMemberNameLength(CVMtypeidGetMemberName(methodID))

extern size_t
CVMtypeidMethodTypeLength(CVMMethodTypeID type);

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
CVMtypeidMethodTypeToAllocatedCString(CVMMethodTypeID type);

extern char *
CVMtypeidFieldTypeToAllocatedCString(CVMFieldTypeID type);

extern char *
CVMtypeidMethodNameToAllocatedCString(CVMMethodTypeID type);

extern char *
CVMtypeidFieldNameToAllocatedCString(CVMFieldTypeID type);

extern char *
CVMtypeidClassNameToAllocatedCString(CVMClassTypeID type);


/*
 * CVMtypeidIncrementArrayDepth will find an entry, inserting if
 *	necessary, and will increment reference counts. 
 *	The resulting ID must be disposed of when finished. See
 *	CVMtypeidDisposeClassID. !!
 *
 */
extern CVMClassTypeID
CVMtypeidIncrementArrayDepth(CVMExecEnv *ee, CVMClassTypeID base, 
                             int depthIncrement);

/*
 * Compare the containing packages of a pair of class types.
 * In the case of array types, the package of the base type is used.
 */
extern CVMBool
CVMtypeidIsSameClassPackage(CVMClassTypeID classname1, 
                            CVMClassTypeID classname2);

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
CVMtypeidGetTerseSignature(CVMMethodTypeID tid, CVMterseSig* result);

void
CVMtypeidGetTerseSignatureIterator(CVMMethodTypeID methodIDd,
                                   CVMterseSigIterator* result);

/*
 * The C terse signature iterator paradigm.
 */
#define CVM_TERSE_ITER_NEXT(tsi) \
    ((((tsi).syllableInWord>=8)? ((tsi).syllableInWord=0, (tsi).word++) : 0), \
      ((tsi).thisSig.datap[(tsi).word] >> (4*((tsi).syllableInWord++))) & 0xf)

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
    CVMTypeIDToken     *parameterDetails;
    CVMTypeIDToken      returnType;
    CVMTypeIDToken      temp;
} CVMSigIterator;

void
CVMtypeidGetSignatureIterator(CVMMethodTypeID tid, CVMSigIterator* result);

#define CVM_SIGNATURE_ITER_NEXT(sigiter) \
    ((((sigiter).temp = CVM_TERSE_ITER_NEXT((sigiter).terseSig)) == \
      CVM_TYPEID_OBJ) ?                                             \
          *((sigiter).parameterDetails++) : (sigiter).temp)

#define CVM_SIGNATURE_ITER_RETURNTYPE(sigiter) ((sigiter).returnType)

#define CVM_SIGNATURE_ITER_PARAMCOUNT(sigiter) \
    CVM_TERSE_ITER_PARAMCOUNT((sigiter).terseSig)


/*
 * Private to the implementation, exposed to make macros work.
 */

#ifdef CVM_16BIT_TYPEID

/* This is only used in the computation of values below but need not actually
   be defined as it isn't being used by any code:
#define CVMtypeidTokenSize      16 
*/
#define CVMtypeidArrayShift	14
    /*
     * This is how these masks are derived:
     * CVMtypeidBaseTypeMask = (1<<CVMtypeidArrayShift)-1
     * CVMtypeidArrayMask = ((1<<(CVMtypeidTokenSize-CVMtypeidArrayShift))-1)
     *				<< CVMtypeidArrayShift;
     * CVMtypeidBigArray = CVMtypeidArrayMask
     * CVMtypeidMaxSmallArray = (CVMtypeidBigArray>>CVMtypeidArrayShift)-1
     */
#define CVMtypeidArrayMask	0xc000
#define CVMtypeidBaseTypeMask	0x3fff
#define CVMtypeidBigArray	0xc000
#define CVMtypeidLastPrimitive  CVM_TYPEID_BOOLEAN
#define CVMtypeidMaxSmallArray	2

#else /* !CVM_16BIT_TYPEID */

/* With 32 bit typeIDs, the CVMClassTypeID is encoded with 8 bits array depth
   in the high bits, and 24 bit base type in the low bits.  This encoding is
   where the following values are derived from:
*/
#define CVMtypeidArrayShift	24
#define CVMtypeidArrayMask	0xff000000
#define CVMtypeidBaseTypeMask	0x00ffffff
#define CVMtypeidBigArray	CVMtypeidArrayMask
#define CVMtypeidLastPrimitive  CVM_TYPEID_BOOLEAN
#define CVMtypeidMaxSmallArray	0xff

#endif /* CVM_16BIT_TYPEID */


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
extern void CVMtypeidPrintDiffs(CVMBool verbose);

/*
 * Check integrety of (some) type tables:
 * follow hash chains and detect duplicates/merging/loops
 * make sure all reachable entries have non-zero ref count, and
 * that unreachable entries have zero ref count.
 */
extern void CVMtypeidCheckTables();

#endif

#endif /* _INCLUDED_TYPEID_H */
