/*
 * @(#)split_verify.c	1.7 06/10/25
 */
/*
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
 * CVM's split verifier implementation. Derived from KVM's verifierUtil.h...
 */

#include "javavm/include/clib.h"
#include "javavm/include/porting/ansi/setjmp.h"
#include "javavm/include/porting/ansi/limits.h"
#include "javavm/include/defs.h"
#include "javavm/include/classes.h"
#include "javavm/include/typeid.h"
#include "javavm/include/globals.h"
#include "javavm/include/preloader.h"
#include "javavm/include/split_verify.h"

#include "javavm/include/opcodes.h"
#include "javavm/include/constantpool.h"
#include "javavm/include/basictypes.h"
#include "javavm/include/bcattr.h"

#include <stddef.h>
#include "javavm/include/common_exceptions.h"
#include "javavm/include/interpreter.h" /* for CVMexceptionOccurred */
#include "javavm/include/directmem.h" /* for CVMD_gcUnsafeExec */

/*
 * This file is the concatenation of 3 files from KVM.
 * This was done to allow for better coupling between functions
 * by the compiler (e.g. inlining) for better speed.
 * Within the context of CVM, it is entirely shared.
 */
/* See opcode mappings at the end of this section */

/*
 * ------------------------------------------------------------------------ *
 *                               General types                              *
 * ------------------------------------------------------------------------ *
 */

/*
 * In order to split the verifier into a VM-specific part and a VM
 * independent part the verifier uses a variable typing 
 * system that is slightly different from the surrounding VM.
 *
 * The following types are use and have the same meaning as they do in KVM
 *
 *      CLASS           A reference to a class data structure
 *      METHOD          A reference to a method data structure
 */

typedef CVMClassBlock*   CLASS;
typedef CVMMethodBlock*  METHOD;
typedef CVMFieldBlock*   FIELD;
typedef CVMTypeID	 NameTypeKey;

/*
 * The following are type definitions that are only used by the
 * verifier core.
 *
 * In KVM a VERIFIERTYPE and a CVMClassTypeID are fairly similar.
 * The difference lies in the encoding of primitive data types.
 * For example, a CVMClassTypeID for an Integer is an ASCII 'I', while
 * a VERIFIERTYPE for an Integer is ITEM_Integer. In both
 * cases values less than 256 are primitive data types and
 * those above 256 are reference types and array types.
 * In KVM the encoding above 256 are the same for CVMClassTypeID
 * and VERIFIERTYPE. The are two conversion functions
 * Vfy_toVerifierType() and Vfy_toClassKey() that will
 * convert between these types. Currently verifierCore
 * never needs to convert primitive data types this way,
 * so these functions simply pass the input to the output
 *
 * Another small complexity exists with VERIFIERTYPE.
 * This is that the value pushed by the NEW bytecode
 * is an encoding of the IP address of the NEW instruction
 * that will create the object when executed (this encoding
 * is the same as that done in the stack maps). This is done in
 * order to distinguish initialized from uninitialized
 * references of the same type. For instance:
 *
 *      new Foo
 *      new Foo
 *      invokespecial <init>
 *
 * This results in the stack containing one initialized Foo
 * and one uninitialized Foo. The verifier needs to keep track
 * of these so after the invokespecial <init> the verifier
 * changes all occurences of the second Foo from being type
 * "New from bytecode-n" to being type "Foo".
 */

/*
 * This represents a class type in the verifier. Again it could
 * the same as a CLASS and/or CVMClassTypeID, but in KVM it is another
 * 16 bit data structure that matches the format the loader produces
 * for the stack map tables.
 * In CVM, we use a 32-bit data structure, so we can encode larger
 * PC values as well as our different encoding of class types.
 */
#define VERIFIERTYPE CVMsplitVerifyMapElement

/*
 * Index in to a bytecode array
 */
#define IPINDEX CVMUint32

/*
 * Index into the local variable array maintained by the verifier
 */
#define SLOTINDEX int

/*
 * Index into the constant pool
 */
#define POOLINDEX int

/*
 * Tag from the constant pool
 */
#define POOLTAG CVMConstantPoolEntryType

typedef struct VfyContext{
    CVMExecEnv*	ee;

    /*
     * This is always the method being verified.
     */
    METHOD methodBeingVerified;
    CLASS  classBeingVerified;

    /*
     * Pointer to the bytecodes for the above method
     */
    unsigned char *bytecodesBeingVerified;

    /*
     * Private to the implementation
     */
    VERIFIERTYPE* vStack;
    VERIFIERTYPE* vLocals;
    unsigned short  vMaxStack;
    unsigned short  vFrameSize;
    unsigned short  vSP;
    unsigned short  vSP_bak;
    VERIFIERTYPE    vStack0_bak;

    /*
     * Holds the return signature
     */
    VERIFIERTYPE  returnSig;
    /*
     * Signature & return type of an invocation we're processing.
     */
    CVMMethodTypeID   calleeContext;
    VERIFIERTYPE      sigResult;

    unsigned long *   NEWInstructions;
    /* must call a this.<init> or super.<init> */
    CVMBool 	      vNeedInitialization;
    /* is the current method a constructor? */
    CVMBool	      vIsConstructor;
    /* does the current method contain a tableswitch or lookupswitch? */
    CVMBool	      vContainsSwitch;

    /*
     * Error Handling jump buffer
     */
    jmp_buf 	vJumpBuffer;

#ifdef CVM_DEBUG
    /*
     * For printing debug messages
     */
    int		vErrorIp;
#endif
    int		constantpoolLength;
    CVMsplitVerifyStackMaps* maps;
    int		noGcPoints;

    /*
     * Stack map cache. Set in Mth_getStackMapEntryIP, read in getStackMap
     */
    CVMsplitVerifyMap*  cachedMap;
    IPINDEX 		cachedMapIP;

} VfyContext;

/*
 * All these functions produce a VERIFIERTYPE corresponding to an array type.
 */
/* The "extra info" associated with well-known system classes */
#define Vfy_getSystemClassName(x)  CVMcbClassName(CVMsystemClass(x))

/* These two are private helpers for contructing the others */
#define Vfy_getPrimArrayVfyType(x) MAKE_FULLINFO(ITEM_##x, 1, 0)
#define Vfy_getSystemVfyType(x)    MAKE_FULLINFO(ITEM_Object, 0, \
					Vfy_getSystemClassName(x))

#define Vfy_getBooleanArrayVerifierType() Vfy_getPrimArrayVfyType(Byte)
#define Vfy_getByteArrayVerifierType()    Vfy_getPrimArrayVfyType(Byte)
#define Vfy_getCharArrayVerifierType()    Vfy_getPrimArrayVfyType(Char)
#define Vfy_getShortArrayVerifierType()   Vfy_getPrimArrayVfyType(Short)
#define Vfy_getIntArrayVerifierType()     Vfy_getPrimArrayVfyType(Integer)
#define Vfy_getLongArrayVerifierType()    Vfy_getPrimArrayVfyType(Long)
#define Vfy_getFloatArrayVerifierType()   Vfy_getPrimArrayVfyType(Float)
#define Vfy_getDoubleArrayVerifierType()  Vfy_getPrimArrayVfyType(Double)
#define Vfy_getObjectArrayVerifierType()  MAKE_FULLINFO(ITEM_Object, 1, \
			    Vfy_getSystemClassName(java_lang_Object))

#define Vfy_getObjectVerifierType()    Vfy_getSystemVfyType(java_lang_Object)
#define Vfy_getStringVerifierType()    Vfy_getSystemVfyType(java_lang_String)
#define Vfy_getThrowableVerifierType() Vfy_getSystemVfyType(java_lang_Throwable)

/*
 * Even though in CVM the verifier type is based on TypeID,
 * and even though the latter are reference counted, we will do no reference
 * counting here.
 * We get the TypeIds from class blocks only, and we use our own system
 * to count array depths, so there should be no danger of either having
 * them disappear out from under us, or of leaving unused entries behind.
 */

/*
 * Get a class array from a CVMClassTypeID data type that some kind of object
 * reference. It is used for ANEWARRAY
 */
#define Vfy_getClassArrayVerifierType(typeKey) \
	(CVMassert(CVMtypeidFieldIsRef(typeKey)), \
	(Vfy_toVerifierType(typeKey) + MAKE_FULLINFO(0, 1, 0)))

/*
 * Gets the VERIFIERTYPE that represents the element type of an array
 */
#define Vfy_getArrayElementType(typeKey) \
	(CVMassert(GET_INDIRECTION(typeKey) != 0), \
	((typeKey) - MAKE_FULLINFO(0, 1, 0)))
/*
 * Gets the VERIFIERTYPE that represents the element type of an array
 * ... which must itself be a reference type -- either another array
 * or any object type.
 */
#define Vfy_getReferenceArrayElementType(typeKey) \
	(((typeKey) == ITEM_Null) ? ITEM_Null : \
	    (CVMassert((GET_INDIRECTION(typeKey) > 0 && \
		       GET_ITEM_TYPE(typeKey) == ITEM_Object) || \
		       GET_INDIRECTION(typeKey) > 1), \
	    ((typeKey) - MAKE_FULLINFO(0, 1, 0)))\
	)
/*
 * Discover if the given classkey is an array of the given dimensions.
 */
#define 	Vfy_isArrayClassKey(typeKey, dim) \
			(CVMtypeidGetArrayDepth(typeKey) >= (dim))
/*
 * This will encode an IP address into a special kind of VERIFIERTYPE such
 * that it will not be confused with other types.
 */
#define Vfy_createVerifierTypeForNewAt(ip)  VERIFY_MAKE_UNINIT(ip)

/*
 * Tests a VERIFIERTYPE to see if it was created using
 * Vfy_createVerifierTypeForNewAt()
 */
#define Vfy_isVerifierTypeForNew(verifierType)  \
		(GET_ITEM_TYPE(verifierType) == ITEM_NewObject)

/*
 * Translates a VERIFIERTYPE encoded using Vfy_createVerifierTypeForNewAt()
 * back into the IP address that was used to create it.
 */
#define Vfy_retrieveIpForNew(verifierType)      \
	    (CVMassert(Vfy_isVerifierTypeForNew(verifierType)), \
	    GET_EXTRA_INFO(verifierType))


/*
 * ------------------------------------------------------------------------
 *                               Error codes                              
 * ------------------------------------------------------------------------
 */

#define VE_STACK_OVERFLOW               1
#define VE_STACK_UNDERFLOW              2
#define VE_STACK_EXPECT_CAT1            3
#define VE_STACK_BAD_TYPE               4
#define VE_LOCALS_OVERFLOW              5
#define VE_LOCALS_BAD_TYPE              6
#define VE_LOCALS_UNDERFLOW             7
#define VE_TARGET_BAD_TYPE              8
#define VE_BACK_BRANCH_UNINIT           9
#define VE_SEQ_BAD_TYPE                10
#define VE_EXPECT_CLASS                11
#define VE_EXPECT_THROWABLE            12
#define VE_BAD_LOOKUPSWITCH            13
#define VE_BAD_LDC                     14
#define VE_BALOAD_BAD_TYPE             15
#define VE_AALOAD_BAD_TYPE             16
#define VE_BASTORE_BAD_TYPE            17
#define VE_AASTORE_BAD_TYPE            18
#define VE_FIELD_BAD_TYPE              19
#define VE_EXPECT_METHODREF            20
#define VE_ARGS_NOT_ENOUGH             21
#define VE_ARGS_BAD_TYPE               22
#define VE_EXPECT_INVOKESPECIAL        23
#define VE_EXPECT_NEW                  24
#define VE_EXPECT_UNINIT               25
#define VE_BAD_INSTR                   26
#define VE_EXPECT_ARRAY                27
#define VE_MULTIANEWARRAY              28
#define VE_EXPECT_NO_RETVAL            29
#define VE_RETVAL_BAD_TYPE             30
#define VE_EXPECT_RETVAL               31
#define VE_RETURN_UNINIT_THIS          32
#define VE_BAD_STACKMAP                33
#define VE_FALL_THROUGH                34
#define VE_EXPECT_ZERO                 35
#define VE_NARGS_MISMATCH              36
#define VE_INVOKESPECIAL               37
#define VE_BAD_INIT_CALL               38
#define VE_EXPECT_FIELDREF             39
#define VE_FINAL_METHOD_OVERRIDE       40
#define VE_MIDDLE_OF_BYTE_CODE         41
#define VE_BAD_NEW_OFFSET              42
#define VE_BAD_EXCEPTION_HANDLER_RANGE 43
#define VE_EXPECTING_OBJ_OR_ARR_ON_STK 44

#define VE_CONSTPOOL_OVERINDEX	       45
#define VE_CALLS_CLINIT		       46
#define VE_OUT_OF_MEMORY	       47
#define VE_SWITCH_NONZERO_PADDING      48

/*
 * Error messages. Keyed by VE_* error codes
 */
static const char *
verifierStatusInfo[] = {
    "<no error>",
    "push would overflow stack",
    "pop would underflow stack",
    "expected 1-word stack entity",
    "unexpected entity type on stack",
    "local overindexing",
    "unexpected entity type in locals",
    "local underindexing (2nd word of long/double at local[0])",
    "bad typematch at branch/handler target",
    "back branch with uninit object",
    "bad typematch at flow join",
    "class type constantpool entry expected",
    "catching non-throwable",
    "unsorted lookupswitch entries",
    "ldc references unexpected constant pool entry type",
    "baload indexes unexpected type",
    "aaload indexes unexpected type",
    "bastore indexes unexpected type",
    "aastore indexes unexpected type",
    "FIELD_BAD_TYPE",
    "method constantpool entry type expected",
    "not enough args on stack",
    "bad args to called method",
    "invokespecial must be used to invoke <init>",
    "'new' instruction expected",
    "uninit object expected in call to <init>",
    "bad opcode found",
    "arraylength expects array",
    "multianewarray dimension or base type wrong",
    "void method returns value",
    "method returns wrong type",
    "<init> doesn't initialize",
    "non-void method fails to return value",
    "stackmap format error",
    "method flow falls off end",
    "invokeinterface byte 5 must be zero",
    "invokeinterface argument count wrong",
    "invokespecial references method not in a superclass",
    "<init> called with wrong object type",
    "fieldref constantpool entry expected",
    "override of final method",
    "method ends in mid-instruction",
    "'new' object doesn't reference new op",
    "malformed exception handler entry",
    "field access requires correct object type",
    "constant pool overindex",
    "direct call to <clinit>",
    "could not allocate working memory",
    "Non zero padding bytes in switch"
};

static const char *
getMsg(int result){
    int  count = sizeof(verifierStatusInfo)/sizeof(verifierStatusInfo[0]);
    const char* info = (result > 0 && result < count)
                                 ? verifierStatusInfo[result]
                                 : "Unknown problem";
    return info;
}

/*
 * If you add new errors to this list, be sure to also add
 * an explanation to KVM_MSG_VERIFIER_STATUS_INFO_INITIALIZER in
 * messages.h.
 */

/*
 * ------------------------------------------------------------------------
 *                              Stack map flags                           
 * ------------------------------------------------------------------------
 */

/*
 * Flags used to control how to match two stack maps.
 * One of the stack maps is derived as part of the type checking process.
 * The other stack map is recorded in the class file.
 */
#define SM_CHECK 1           /* Check if derived types match recorded types. */
#define SM_MERGE 2           /* Merge recorded types into derived types. */
#define SM_EXIST 4           /* A recorded type attribute must exist. */

/*
 * ------------------------------------------------------------------------
 *                             General functions                          
 * ------------------------------------------------------------------------
 */


#define getCell(addr) \
                  ((((long)(((unsigned char *)(addr))[0])) << 24) |  \
                   (((long)(((unsigned char *)(addr))[1])) << 16) |  \
                   (((long)(((unsigned char *)(addr))[2])) << 8)  |  \
                   (((long)(((unsigned char *)(addr))[3])) << 0))


/* Get an unsigned 16-bit value from the given memory location */
#define getUShort(addr) \
                  ((((unsigned short)(((unsigned char *)(addr))[0])) << 8)  | \
                   (((unsigned short)(((unsigned char *)(addr))[1])) << 0))


/* Get a 16-bit value from the given memory location */
#define getShort(addr) ((short)(getUShort(addr)))


/*
 * Get an signed byte from the bytecode array
 */
#define Vfy_getByte(cntxt,ip)     \
    (((signed char *)((cntxt)->bytecodesBeingVerified))[ip])

/*
 * Get an unsigned byte from the bytecode array
 */
#define Vfy_getUByte(cntxt, ip)    (cntxt->bytecodesBeingVerified[ip])

/*
 * Get an signed short from the bytecode array
 */
#define Vfy_getShort(cntxt,ip)    getShort(cntxt->bytecodesBeingVerified+(ip))

/*
 * Get an unsigned short from the bytecode array
 */
#define Vfy_getUShort(cntxt,ip)   getUShort(cntxt->bytecodesBeingVerified+(ip))

/*
 * Get an signed word from the bytecode array
 */
#define Vfy_getCell(cntxt,ip)     getCell(cntxt->bytecodesBeingVerified+(ip))

/*
 * Get an opcode from the bytecode array.
 */
static int Vfy_getOpcode(VfyContext* cntxt, IPINDEX ip);

#if 0
VERIFIERTYPE CVMvfy_makeVerifierType(CVMClassTypeID key);
#endif

/*
 * Convert a VERIFIERTYPE to a CVMClassTypeID
 *
 * In KVM the types are the same when the value is above 256 and the
 * verifier core never needs to convert values lower then this so it
 * is simply enough to check that this assumption is true.
 *
 * @param classKey  A CVMClassTypeID
 * @return          A VERIFIERTYPE
 */
/*
#define Vfy_toVerifierType(classKey)  \
    (CVMassert(CVMtypeidFieldIsRef(classKey)), \
    (CVMtypeidGetArrayDepth(classKey)==0) ? VERIFY_MAKE_OBJECT(classKey) : \
    CVMvfy_makeVerifierType(classKey))
*/

#define Vfy_toVerifierType(classKey)  VERIFY_MAKE_OBJECT(classKey)

/*
 * Convert a CVMClassTypeID to a VERIFIERTYPE
 *
 * In KVM the types are the same when the value is above 256 and the
 * verifier core never needs to convert values lower then this so it
 * is simply enough to check that this assumption is true.
 *
 * @param verifierType  A VERIFIERTYPE
 * @return              A CVMClassTypeID
 */
/* Assert that this is really a class type, not an array or e.g. int */
#define Vfy_toClassKey(verifierType) \
    (CVMassert(WITH_ZERO_EXTRA_INFO(verifierType)==NULL_FULLINFO), \
    GET_EXTRA_INFO(verifierType))
/* Same, but with a more lenient assertion */
#define Vfy_baseClassKey(verifierType) \
    (CVMassert(GET_ITEM_TYPE(verifierType) == ITEM_Object),        \
    GET_EXTRA_INFO(verifierType))

/*
 * Test to see if a type is a primitive type
 *
 * @param  type     A VERIFIERTYPE
 * @return CVMBool   CVM_TRUE is type is a primitive
#define Vfy_isPrimitive(type)  ((type != ITEM_Null) && (type < 256))
 */

/*
 * Test to see if a type is an array
 *
 * @param  type     A VERIFIERTYPE
 * @return CVMBool   CVM_TRUE is type is an array
 */
#define Vfy_isArray(type)  (GET_INDIRECTION(type) > 0)

/*
 * Test to see if a type is an array or null
 *
 * @param  type     A VERIFIERTYPE
 * @return CVMBool   CVM_TRUE is type is an array or null
 */
#define Vfy_isArrayOrNull(type)  (Vfy_isArray(type) || (type == ITEM_Null))


/*
 * Vfy_vIsAssignable: factor out the most common case from vIsAssignable.
 * NOTE THAT this will not write the mergedCaseP result if the
 * shortcut is taken.
 */
#define Vfy_vIsAssignable(cntxt, fromKey, toKey, mergedKeyP) \
    ((((toKey) == ITEM_Bogus) || ((toKey) == (fromKey))) ? CVM_TRUE : \
	vIsAssignable((cntxt), (fromKey), (toKey), (mergedKeyP)))
/*
 * Test to see if one type can be assigned to another type
 *
 * @param fromKey   A VERIFIERTYPE to assign from
 * @param toKey     A VERIFIERTYPE to assign to
 * @return CVMBool   CVM_TRUE if the assignment is legal
 */
#define Vfy_isAssignable(fromKey, toKey)    \
	Vfy_vIsAssignable(cntxt, fromKey, toKey, NULL)

/*
 * Test to see if a field of a class is "protected"
 *
 * @param clazz     A CLASS
 * @param index     The field index (offset in words)
 * @return          CVM_TRUE if field is protected
 */
#define Vfy_isProtectedField(clazz, index) vIsProtectedAccess((CLASS)clazz, index, CVM_FALSE)

/*
 * Test to see if a method of a class is "protected"
 *
 * @param clazz     A CLASS
 * @param index     The method index (offset in words)
 * @return          CVM_TRUE if method is protected
 */
#define Vfy_isProtectedMethod(clazz, index) vIsProtectedAccess((CLASS)clazz, index, CVM_TRUE)

/*
 * Check if the current state of the local variable and stack contents match
 * that of a stack map at a specific IP address
 *
 * @param cntxt     Pointer to the current verification context
 * @param targetIP  The IP address for the stack map
 * @param flags     One or more of SM_CHECK, SM_MERGE, and SM_EXIST
 * @param throwCode A verification error code
 * @throw           If the match is not correct
 */
/* KILL ME
    #define Vfy_matchStackMap(cntxt, targetIP, flags, throwCode) {                \
	if (!matchStackMap(cntxt, targetIP, flags)){ \
	    Vfy_throw(cntxt, throwCode);                                          \
	}                                                                         \
    }
*/

/*
 * Check stack maps can be merged at the current location
 *
 * @param cntxt         Pointer to the current verification context
 * @param targetIP      The IP address for the stack map
 * @param noControlFlow CVM_TRUE if the location cannot be reached from the previous instruction
 * @throw               If the match is not correct
 */
#define Vfy_checkCurrentTarget(cntxt, current_ip, noControlFlow) {            \
        Vfy_matchStackMap( cntxt,                                             \
                           current_ip,                                        \
                           SM_MERGE | (noControlFlow ? SM_EXIST : SM_CHECK),  \
                           VE_SEQ_BAD_TYPE                                    \
                         );                                                   \
}

/*
 * Check stack maps match at an exception handler entry point
 *
 * @param cntxt         Pointer to the current verification context
 * @param target_ip     The IP address of the exception handler
 * @throw               If the match is not correct
 */
#define Vfy_checkHandlerTarget(cntxt, target_ip) {                            \
    Vfy_trace1("Check exception handler at %ld:\n", (long)target_ip);  \
    Vfy_matchStackMap(cntxt, target_ip, SM_CHECK | SM_EXIST,                  \
    		      VE_TARGET_BAD_TYPE);                                    \
}

/*
 * Check stack maps match at a jump target
 *
 * @param cntxt         Pointer to the current verification context
 * @param current_ip    The current IP address
 * @param target_ip     The IP address of the jump target
 * @throw               If the match is not correct
 */
#define Vfy_checkJumpTarget(cntxt, this_ip, target_ip) {                      \
    Vfy_trace1("Check jump target at %ld\n", (long)target_ip);         \
    Vfy_matchStackMap(cntxt, target_ip, SM_CHECK | SM_EXIST,                  \
    		      VE_TARGET_BAD_TYPE);                                    \
    if (!checkNewObject(cntxt, (IPINDEX)this_ip, target_ip)) {                \
        Vfy_throw(cntxt, VE_BACK_BRANCH_UNINIT);                              \
    }                                                                         \
}

/*
 * Check stack maps match at a jump target
 *
 * @param cntxt         Pointer to the current verification context
 * @param current_ip    The current IP address
 * @param target_ip     The IP address of the jump target
 * @throw               If the match is not correct
 */
#define Vfy_markNewInstruction(cntxt, ip, length) {                           \
    if (cntxt->NEWInstructions == NULL){                                      \
        cntxt->NEWInstructions = callocNewBitmap(length);                     \
    }                                                                         \
    SET_NTH_BIT(cntxt->NEWInstructions, ip);                                  \
}
#undef Vfy_markNewInstruction
#define Vfy_markNewInstruction(cntxt, ip, length) {                           \
    SET_NTH_BIT(cntxt->NEWInstructions, ip);                                  \
}

/*
 * Get the type of a local variable
 *
 * @param cntxt         Pointer to the current verification context
 * @param i             The slot index of the local variable
 * @param t             The VERIFIERTYPE that must match
 * @return              The actual VERIFIERTYPE found
 * @throw               If the match is not correct or index is bad
 */
static VERIFIERTYPE 
Vfy_getLocal(VfyContext* cntxt, SLOTINDEX  i, VERIFIERTYPE t);

/*
 * Set the type of a local variable
 *
 * @param cntxt         Pointer to the current verification context
 * @param i             The slot index of the local variable
 * @param t             The new VERIFIERTYPE of the slot
 * @throw               If index is bad
 */
static void 
Vfy_setLocal(VfyContext* cntxt, SLOTINDEX  i, VERIFIERTYPE t);

/*
 * Push a type
 *
 * @param cntxt         Pointer to the current verification context
 * @param t             The new VERIFIERTYPE to be pushed
 * @throw               If stack overflows
 */
static void Vfy_push(VfyContext* cntxt, VERIFIERTYPE t);

/*
 * Pop a type
 *
 * @param cntxt         Pointer to the current verification context
 * @param t             The VERIFIERTYPE that must match
 * @throw               If the match is not correct or stack underflow
 */
static VERIFIERTYPE Vfy_pop(VfyContext* cntxt, VERIFIERTYPE t);

/*
 * Pop category1 type (ITEM_Null, ITEM_Integer, ITEM_Float, <init> object,
 *                      reference type, or array type
 *
 * @param cntxt         Pointer to the current verification context
 * @throw               If the match is not correct or stack underflow
 */
static VERIFIERTYPE Vfy_popCategory1(VfyContext* cntxt);

/*
 * Pop the first word of a category2 type
 *
 * @param cntxt         Pointer to the current verification context
 * @throw               If the match is not correct or stack underflow
 */
static VERIFIERTYPE Vfy_popCategory2_firstWord(VfyContext* cntxt);

/*
 * Pop the second word of a category2 type
 *
 * @param cntxt         Pointer to the current verification context
 * @throw               If the match is not correct or stack underflow
 */
static VERIFIERTYPE Vfy_popCategory2_secondWord(VfyContext* cntxt);

/*
 * Pop the first word of a DoubleWord type
 *
 * @param cntxt         Pointer to the current verification context
 * @throw               If the match is not correct or stack underflow
 */
#define Vfy_popDoubleWord_firstWord(cntxt)  Vfy_popCategory2_firstWord(cntxt)

/*
 * Pop the second word of a DoubleWord type
 *
 * @param cntxt         Pointer to the current verification context
 * @throw               If the match is not correct or stack underflow
 */
#define Vfy_popDoubleWord_secondWord(cntxt) Vfy_popCategory2_secondWord(cntxt)

/*
 * Verify a void return
 *
 * @param cntxt         Pointer to the current verification context
 * @throw               If the method is not if type void, if the method
 *                      is <init> and super() was not called.
 */
static void Vfy_returnVoid(VfyContext* cntxt);

/*
 * Pop a type and verify a return
 *
 * @param cntxt         Pointer to the current verification context
 * @param returnType    The VERIFIERTYPE to be returned
 * @throw               If the return type is incorrect
 */
static void Vfy_popReturn(VfyContext* cntxt, VERIFIERTYPE returnType);

/*
 * Push a CVMClassTypeID
 *
 * @param cntxt         Pointer to the current verification context
 * @param returnType    The CVMClassTypeID to be pushed
 * @throw               If stack overflow, bad type.
 */
static void Vfy_pushClassKey(VfyContext* cntxt, CVMClassTypeID fieldType);

/*
 * Pop a CVMClassTypeID
 *
 * @param cntxt         Pointer to the current verification context
 * @param returnType    The CVMClassTypeID to be popped
 * @throw               If stack underflow, bad type.
 */
static void Vfy_popClassKey(VfyContext* cntxt, CVMClassTypeID fieldType);

/*
 * Setup the callee context (to process an invoke)
 *
 * @param cntxt         Pointer to the current verification context
 * @param methodTypeKey The CVMMethodTypeID the method type
 */
static void Vfy_setupCalleeContext(VfyContext* cntxt, CVMMethodTypeID methodTypeKey);

/*
 * Pop the callee arguments from the stack
 *
 * @param cntxt         Pointer to the current verification context
 * @throw               If argument are not correct
 */
static int Vfy_popInvokeArguments(VfyContext* cntxt);

/*
 * Push the callee result
 *
 * @param cntxt         Pointer to the current verification context
 * @throw               If result is not correct
 */
static void Vfy_pushInvokeResult(VfyContext* cntxt);

/*
 * Change all local and stack instances of one type to another
 *
 * @param cntxt         Pointer to the current verification context
 * @param fromType      The VERIFIERTYPE to convert from
 * @param fromType      The VERIFIERTYPE to convert to
 */
static void Vfy_ReplaceTypeWithType(VfyContext* cntxt, VERIFIERTYPE fromType, VERIFIERTYPE toType);

/*
 * ------------------------------------------------------------------------
 *                             Stack state management                     
 * ------------------------------------------------------------------------
 */

/*
 * Save the state of the type stack
 * @param cntxt         Pointer to the current verification context
 */
static void Vfy_saveStackState(VfyContext* cntxt);

/*
 * Restore the state of the type stack
 * @param cntxt         Pointer to the current verification context
 */
static void Vfy_restoreStackState(VfyContext* cntxt);

/*
 * Initialize the local variable types
 * @param cntxt         Pointer to the current verification context
 */
static void Vfy_initializeLocals(VfyContext* cntxt);

/*
 * Abort the verification
 *
 * @param cntxt         Pointer to the current verification context
 * @param code          One of the VE_xxx codes
 */
static void Vfy_throw(VfyContext* cntxt, int code);

#ifdef CVM_DEBUG

/*
 * Write a trace line with one parameter
 *
 * @param cntxt         Pointer to the current verification context
 * @param msg           printf format string
 * @param p1            32 bit parameter
 */
static void Vfy_trace1(char *msg, long p1);

/*
 * Save the ip somewhere the error printing routine can find it
 *
 * @param cntxt         Pointer to the current verification context
 * @param ip            The IP address
 */
#define Vfy_setErrorIp(cntxt, ip) cntxt->vErrorIp = ip

/*
 * Output a trace statement at the head of the dispatch loop
 *
 * @param cntxt         Pointer to the current verification context
 * @param vMethod       The METHOD being verified
 * @param ip            The IP address
 */
static void Vfy_printVerifyLoopMessage(VfyContext* cntxt, METHOD vMethod, IPINDEX ip);

#else

#define Vfy_trace1(x, y)                       /**/
#define Vfy_setErrorIp(c, x)                   /**/
#define Vfy_printVerifyLoopMessage(c, x, y)    /**/

#endif /* CVM_DEBUG */

/*
 * ------------------------------------------------------------------------
 *                           Class Accessor Methods                       
 * ------------------------------------------------------------------------
 */

/*
 * Return CVM_TRUE is the class is java.lang.Object
 *
 * @param vClass        The CLASS to be accessed
 * @return              CVM_TRUE if the the class is java.lang.Object
 */
#define Cls_isJavaLangObject(vClass)    ((vClass) == CVMsystemClass(java_lang_Object))

/*
 * Return the superclass of the class
 *
 * @param vClass        The CLASS to be accessed
 * @return              The CLASS of its superclass
 */
#define Cls_getSuper(vClass)            CVMcbSuperclass(vClass)

/*
 * Return the constant pool for the class
 *
 * @param vClass        The CLASS to be accessed
 * @return              Pointer to its constant pool
 */
#define Cls_getPool(vClass)		    CVMcbConstantPool(vClass)

/*
 * Lookup a method
 *
 * @param vClassContext The CLASS of the lookup context (e.g. calling class)
 * @param vClassTarget  The CLASS to be accessed (e.g. called class)
 * @param vMethod       The METHOD who's name should be used to do the lookup
 * @return              The METHOD if it exists else NULL
 */
static METHOD 
Cls_lookupMethodByName(CLASS context, CLASS target, CVMMethodTypeID method);

#define Cls_lookupMethod(vClassContext, vClassTarget, vMethod) \
    Cls_lookupMethodByName(vClassContext, vClassTarget, Mth_getName(vMethod))

/*
 * Lookup a field 
 *
 * @param vClass        The CLASS of the lookup context (e.g. calling class)
 * @param vNameTypeKey  The nameTypeKey which is being searched in field table
 * @return              The FIELD if it exists else NULL
 */
static FIELD 
Cls_lookupFieldByName(CLASS context, CLASS target, CVMFieldTypeID field);

#define Cls_lookupField(vClass, vNameTypeKey) \
    Cls_lookupFieldByName(NULL, vClass, vNameTypeKey)
/*
 * Get a class's key
 *
 * @param vClass        The CLASS to be accessed
 * @return              Its CVMClassTypeID
 */
#define Cls_getKey(vClass)  CVMcbClassName(vClass)

/*
 * ------------------------------------------------------------------------
 *                            Pool Accessor Methods                        
 * ------------------------------------------------------------------------
 */

/*
 * Get the tag of the pool entry at index.
 *
 * @param cntxt		VfyContext pointer
 * @param vPool         The constant pool to be accessed
 * @param index         The index to be accessed
 * @return              Its POOLTAG
 * @throw               If index out of bounds
 *
 */
#define Pol_getTag(cntxt, vPool, index)			\
	((index >= cntxt->constantpoolLength) ? 	\
        (Vfy_throw(cntxt, VE_CONSTPOOL_OVERINDEX), 0) : \
        CVMcpEntryType(vPool, index))     

/*
 * Check the tag at index is the same as "tag".
 *
 * @param cntxt		VfyContext pointer
 * @param vPool         The constant pool to be accessed
 * @param index         The index to be accessed
 * @param tag           The POOLTAG that should be there
 * @param errorcode     The error to throw if it is not
 * @throw               If tag is wrong
 */
#define Pol_checkTagIs(cntxt, vPool, index, tag, errorcode) { \
    if (Pol_getTag(cntxt, vPool, index) != tag) {             \
        Vfy_throw(cntxt, errorcode);                          \
    }                                                         \
}

/*
 * Check the tag at index is the same as "tag"or "tag2".
 *
 * @param cntxt		VfyContext pointer
 * @param vPool         The constant pool to be accessed
 * @param index         The index to be accessed
 * @param tag           The POOLTAG that should be there
 * @param tag2          The POOLTAG that should be there
 * @param errorcode     The error to throw if it is not
 * @throw               If both tags are wrong
 */
#define Pol_checkTag2Is(cntxt, vPool, index, tag, tag2, errorcode){ \
    POOLTAG t = Pol_getTag(cntxt, vPool, index);                    \
    if (t != tag && t != tag2) {                                    \
        Vfy_throw(cntxt, errorcode);                                \
    }                                                               \
}

/*
 * Check the tag at index is CONSTANT_Class.
 *
 * @param vPool         The constant pool to be accessed
 * @param index         The index to be accessed
 * @throw               If the pool tag is not CONSTANT_Class
 */
#define Pol_checkTagIsClass(cntxt, vPool, index)                   \
     Pol_checkTagIs(cntxt, vPool, index, CONSTANT_Class, VE_EXPECT_CLASS)

/*
 * Get the CVMClassTypeID for the pool entry at index
 *
 * @param cntxt		VfyContext pointer
 * @param vPool         The constant pool to be accessed
 * @param index         The index to be accessed
 * @return              The CLASS of the CONSTANT_Class at that index
 */
#define Pol_getClassKey(vPool, index)		\
    (CVMcpIsResolved(vPool, index)		\
     ? CVMcbClassName(CVMcpGetCb(vPool, index))	\
     : CVMcpGetClassTypeID(vPool, index))
    
/* For CONSTANT_FieldRef_info, CONSTANT_MethodRef_info, 
 * CONSTANT_InterfaceMethodRef_info 
 */

/*
 * Get the class index for the pool entry at index
 *
 * @param vPool         The constant pool to be accessed
 * @param index         The index to be accessed
 * @return              The POOLINDEX in the class_index field
 */
/* This only works if the entry is unresolved */
#define Pol_getClassIndex(vPool, index)				\
	CVMcpGetMemberRefClassIdx(vPool,index)

/*
 * Get the class for the pool entry at index 
 *
 * @param vPool         The constant pool to be accessed
 * @param index         The index to be accessed
 * @return              The POOLINDEX in the fieldClassIndex field
 */
/* This only works if this entry is resolved */
#define Pol_getClass(vPool, fieldClassIndex)               \
	CVMcpGetCb(vPool, fieldClassIndex)

/*
 * Get the Name-and-type index for the pool entry at index
 *
 * @param vPool         The constant pool to be accessed
 * @param index         The index to be accessed
 * @return              The POOLINDEX in the name_and_type_index field
 */
#define Pol_getNameAndTypeIndex(vPool, index)               \
	CVMcpGetMemberRefTypeIDIdx(vPool, index)

/*
 * Get the Name-and-type key for the pool entry at index
 *
 * @param vPool         The constant pool to be accessed
 * @param index         The index to be accessed
 * @return              The nameTypeKey 
 */
#define Pol_getFieldNameTypeKey(vPool, index)               \
	CVMcpGetFieldTypeID(vPool, index)
#define Pol_getMethodNameTypeKey(vPool, index)              \
	CVMcpGetMethodTypeID(vPool, index)

/* For CONSTANT_NameAndType_info, */

/*
 * Get the type key of a Name-and-type for the pool entry at index
 *
 * @param vPool             The constant pool to be accessed
 * @param nameAndTypeIndex The index to be accessed
 * @return                  The CVMMethodTypeID of that entry
 */
#define Pol_getFieldTypeKey(vPool, nameAndTypeIndex)             \
    CVMtypeidGetTypePart(Pol_getFieldNameTypeKey(vPool, nameAndTypeIndex))

#define Pol_getMethodTypeKey(vPool, nameAndTypeIndex)             \
    CVMtypeidGetTypePart(Pol_getMethodNameTypeKey(vPool, nameAndTypeIndex))

/*
 * Get the descriptor key of a Name-and-type for the pool entry at index
 *
 * @param vPool             The constant pool to be accessed
 * @param nameAndTypeIndex The index to be accessed
 * @return                  The CVMMethodTypeID of that entry
 */
#define Pol_getMethodDescriptorKey(_vPool, _nameAndTypeIndex)       \
	CVMcpGetMethodTypeID(_vPool, _nameAndTypeIndex)

/*
 * ------------------------------------------------------------------------ * 
 *                      Method/Field Accessor Methods                       *
 * ------------------------------------------------------------------------ * 
 */

/*
 * Get the CLASS of a METHOD
 *
 * @param vMethod       The METHOD to be accessed
 * @return              The CLASS of the method
 */
#define Mth_getClass(vMethod)  CVMmbClassBlock(vMethod)

/*
 * Get the CLASS of a FIELD
 *
 * @param vField        The FIELD to be accessed
 * @return              The CLASS of the field
 */
#define Fld_getClass(vField)  CVMfbClassBlock(vField)

/*
 * Get the address of the bytecode array
 *
 * @param vMethod       The METHOD to be accessed
 * @return              The address of the bytecodes
 */
#define Mth_getBytecodes(vMethod)    CVMmbJavaCode(vMethod)

/*
 * Get the length of the bytecode array
 *
 * @param vMethod       The METHOD to be accessed
 * @return              The length of the bytecodes
 */
#define Mth_getBytecodeLength(vMethod) CVMmbCodeLength(vMethod)

/*
 * Return CVM_TRUE if it is a static method
 *
 * @param vMethod       The METHOD to be accessed
 * @return              CVM_TRUE if it is a static method
 */
#define Mth_isStatic(vMethod) CVMmbIs(vMethod, STATIC)

/*
 * Return CVM_TRUE if it is a final method
 *
 * @param vMethod       The METHOD to be accessed
 * @return              CVM_TRUE if it is a final method
 */
#define Mth_isFinal(vMethod) CVMmbIs(vMethod, FINAL)

/*
 * Return a method's name.
 *
 * @param vMethod       The METHOD to be accessed
 * @return              the CVMMethodTypeID
 */
#define Mth_getName(vMethod)  CVMmbNameAndTypeID(vMethod)

/*
 * Get the start ip address for the ith exception table entry
 *
 * @param vMethod       The METHOD to be accessed
 * @param i             The exception table index
 * @return              The startPC
 */
#define Mth_getExceptionTableStartPC(vMethod, i)	  \
	(CVMjmdExceptionTable(CVMmbJmd(vMethod))[i].startpc)

/*
 * Get the end ip address for the ith exception table entry
 *
 * @param vMethod       The METHOD to be accessed
 * @param i             The exception table index
 * @return              The endPC
 */
#define Mth_getExceptionTableEndPC(vMethod, i)          \
	(CVMjmdExceptionTable(CVMmbJmd(vMethod))[i].endpc)

/*
 * Get the handler ip address for the ith exception table entry
 *
 * @param vMethod       The METHOD to be accessed
 * @param i             The exception table index
 * @return              The handlerPC
 */
#define Mth_getExceptionTableHandlerPC(vMethod, i)      \
	(CVMjmdExceptionTable(CVMmbJmd(vMethod))[i].handlerpc)

/*
 * Get the exception class for the ith exception table entry
 *
 * @param vMethod       The METHOD to be accessed
 * @param i             The exception table index
 * @return              The constant pool index of the
 *				CVMClassTypeID of the exception, or zero.
 */
#define Mth_getExceptionTableCatchType(vMethod,i)       \
	(CVMjmdExceptionTable(CVMmbJmd(vMethod))[i].catchtype)

/*
 * Get the ip address for the ith entry in the stack map table
 *
 * @param vMethod       The METHOD to be accessed
 * @param i             The index into the stack map table
 * @return              The IP address for that entry
 */
static int Mth_getStackMapEntryIP(VfyContext* cntxt, METHOD vMethod, int i);

/*
 * Check the stackmap offset for the ith entry in the stack map table
 *
 * @param vMethod       The METHOD to be accessed
 * @param stackmapIndex The index into the stack map table
 * @return              true if ok, false if not valid.
 */
static CVMBool Mth_checkStackMapOffset(VfyContext* cntxt, int stackmapIndex);

/*
 * Get the length of the stack map table
 *
 * @param vMethod       The METHOD to be accessed
 * @return              The number of entries in the  stack map table
 */
#define Mth_getExceptionTableLength(vMethod)        \
    CVMmbExceptionTableLength(vMethod)

/*
 * The following are not a part of this interface, but defined the
 * other variables and routines used from verifier.c
 */

#define IS_NTH_BIT(var, bit)    (var[(bit) >> 5] & (1 <<  ((bit) & 0x1F)))
#define SET_NTH_BIT(var, bit)   (var[(bit) >> 5] |= (1 << ((bit) & 0x1F)))

#define callocNewBitmap(size) \
        ((unsigned long *) calloc((size + 31) >> 5, sizeof(unsigned long)))


static CVMBool   vIsAssignable(VfyContext*, VERIFIERTYPE fromKey, 
			VERIFIERTYPE toKey, VERIFIERTYPE *mergedKeyP);
static void	 Vfy_matchStackMap(VfyContext*, IPINDEX target_ip, int flags, 
			int throwCode);
static CVMBool   checkNewObject(VfyContext*, IPINDEX this_ip, 
			IPINDEX target_ip);

static CVMBool   vIsProtectedAccess(CLASS thisClass, POOLINDEX index, 
			CVMBool isMethod);
static int      change_Field_to_StackType(CVMClassTypeID fieldType, 
			VERIFIERTYPE* stackTypeP);

#if CACHE_VERIFICATION_RESULT
CVMBool checkVerifiedClassList(CLASS);
void appendVerifiedClassList(CLASS);
#else 
#define checkVerifiedClassList(class) CVM_FALSE
#define appendVerifiedClassList(class)
#endif


/* CVM OPCODE NAME REMAPPING */
/* Try to avoid changing splitVerify.c by defining opcodes */
/* (if we give up on sharing that file, then just change all the names) */
#define NOP		opc_nop
#define ACONST_NULL 	opc_aconst_null
#define ICONST_M1	opc_iconst_m1
#define ICONST_0	opc_iconst_0
#define ICONST_1	opc_iconst_1
#define ICONST_2	opc_iconst_2
#define ICONST_3	opc_iconst_3
#define ICONST_4	opc_iconst_4
#define ICONST_5	opc_iconst_5
#define LCONST_0	opc_lconst_0
#define LCONST_1	opc_lconst_1
#define FCONST_0	opc_fconst_0
#define FCONST_1	opc_fconst_1
#define FCONST_2	opc_fconst_2
#define DCONST_0	opc_dconst_0
#define DCONST_1	opc_dconst_1
#define BIPUSH		opc_bipush
#define SIPUSH		opc_sipush
#define LDC		opc_ldc
#define LDC_W		opc_ldc_w
#define LDC2_W		opc_ldc2_w
#define ILOAD_0		opc_iload_0
#define ILOAD_1		opc_iload_1
#define ILOAD_2		opc_iload_2
#define ILOAD_3		opc_iload_3
#define ILOAD		opc_iload
#define LLOAD_0		opc_lload_0
#define LLOAD_1		opc_lload_1
#define LLOAD_2		opc_lload_2
#define LLOAD_3		opc_lload_3
#define LLOAD		opc_lload
#define FLOAD_0		opc_fload_0
#define FLOAD_1		opc_fload_1
#define FLOAD_2		opc_fload_2
#define FLOAD_3		opc_fload_3
#define FLOAD		opc_fload
#define DLOAD_0		opc_dload_0
#define DLOAD_1		opc_dload_1
#define DLOAD_2		opc_dload_2
#define DLOAD_3		opc_dload_3
#define DLOAD		opc_dload
#define ALOAD_0		opc_aload_0
#define ALOAD_1		opc_aload_1
#define ALOAD_2		opc_aload_2
#define ALOAD_3		opc_aload_3
#define ALOAD		opc_aload
#define BALOAD		opc_baload
#define IALOAD		opc_iaload
#define SALOAD		opc_saload
#define CALOAD		opc_caload
#define FALOAD		opc_faload
#define LALOAD		opc_laload
#define DALOAD		opc_daload
#define AALOAD		opc_aaload
#define ISTORE_0	opc_istore_0
#define ISTORE_1	opc_istore_1
#define ISTORE_2	opc_istore_2
#define ISTORE_3	opc_istore_3
#define ISTORE		opc_istore
#define LSTORE_0	opc_lstore_0
#define LSTORE_1	opc_lstore_1
#define LSTORE_2	opc_lstore_2
#define LSTORE_3	opc_lstore_3
#define LSTORE		opc_lstore
#define FSTORE_0	opc_fstore_0
#define FSTORE_1	opc_fstore_1
#define FSTORE_2	opc_fstore_2
#define FSTORE_3	opc_fstore_3
#define FSTORE		opc_fstore
#define DSTORE_0	opc_dstore_0
#define DSTORE_1	opc_dstore_1
#define DSTORE_2	opc_dstore_2
#define DSTORE_3	opc_dstore_3
#define DSTORE		opc_dstore
#define ASTORE_0	opc_astore_0
#define ASTORE_1	opc_astore_1
#define ASTORE_2	opc_astore_2
#define ASTORE_3	opc_astore_3
#define ASTORE		opc_astore
#define BASTORE		opc_bastore
#define IASTORE		opc_iastore
#define SASTORE		opc_sastore
#define CASTORE		opc_castore
#define FASTORE		opc_fastore
#define LASTORE		opc_lastore
#define DASTORE		opc_dastore
#define AASTORE		opc_aastore
#define POP		opc_pop
#define POP2		opc_pop2
#define DUP		opc_dup
#define DUP_X1		opc_dup_x1
#define DUP_X2		opc_dup_x2
#define DUP2		opc_dup2
#define DUP2_X1		opc_dup2_x1
#define DUP2_X2		opc_dup2_x2
#define SWAP		opc_swap
#define IADD		opc_iadd
#define ISUB		opc_isub
#define IMUL		opc_imul
#define IDIV		opc_idiv
#define IREM		opc_irem
#define ISHL		opc_ishl
#define ISHR		opc_ishr
#define IUSHR		opc_iushr
#define IOR		opc_ior
#define IXOR		opc_ixor
#define IAND		opc_iand
#define INEG		opc_ineg
#define LADD		opc_ladd
#define LSUB		opc_lsub
#define LMUL		opc_lmul
#define LDIV		opc_ldiv
#define LREM		opc_lrem
#define LSHL		opc_lshl
#define LSHR		opc_lshr
#define LUSHR		opc_lushr
#define LOR		opc_lor
#define LXOR		opc_lxor
#define LAND		opc_land
#define LNEG		opc_lneg
#define FADD		opc_fadd
#define FSUB		opc_fsub
#define FMUL		opc_fmul
#define FDIV		opc_fdiv
#define FREM		opc_frem
#define FNEG		opc_fneg
#define DADD		opc_dadd
#define DSUB		opc_dsub
#define DMUL		opc_dmul
#define DDIV		opc_ddiv
#define DREM		opc_drem
#define DNEG		opc_dneg
#define IINC		opc_iinc
#define I2L		opc_i2l
#define I2F		opc_i2f
#define I2D		opc_i2d
#define I2B		opc_i2b
#define I2C		opc_i2c
#define I2S		opc_i2s
#define L2I		opc_l2i
#define L2F		opc_l2f
#define L2D		opc_l2d
#define F2I		opc_f2i
#define F2L		opc_f2l
#define F2D		opc_f2d
#define D2I		opc_d2i
#define D2L		opc_d2l
#define D2F		opc_d2f
#define LCMP		opc_lcmp
#define FCMPL		opc_fcmpl
#define FCMPG		opc_fcmpg
#define DCMPL		opc_dcmpl
#define DCMPG		opc_dcmpg
#define IF_ICMPEQ	opc_if_icmpeq
#define IF_ICMPNE	opc_if_icmpne
#define IF_ICMPLT	opc_if_icmplt
#define IF_ICMPGE	opc_if_icmpge
#define IF_ICMPGT	opc_if_icmpgt
#define IF_ICMPLE	opc_if_icmple
#define IFEQ		opc_ifeq
#define IFNE		opc_ifne
#define IFLT		opc_iflt
#define IFGE		opc_ifge
#define IFGT		opc_ifgt
#define IFLE		opc_ifle
#define IF_ACMPEQ	opc_if_acmpeq
#define IF_ACMPNE	opc_if_acmpne
#define IFNULL		opc_ifnull
#define IFNONNULL	opc_ifnonnull
#define GOTO		opc_goto
#define GOTO_W		opc_goto_w
#define TABLESWITCH	opc_tableswitch
#define LOOKUPSWITCH	opc_lookupswitch
#define ARETURN		opc_areturn
#define IRETURN		opc_ireturn
#define FRETURN		opc_freturn
#define DRETURN		opc_dreturn
#define LRETURN		opc_lreturn
#define RETURN		opc_return
#define GETSTATIC	opc_getstatic
#define PUTSTATIC	opc_putstatic
#define GETFIELD	opc_getfield
#define PUTFIELD	opc_putfield
#define INVOKEVIRTUAL	opc_invokevirtual
#define INVOKESPECIAL	opc_invokespecial
#define INVOKESTATIC	opc_invokestatic
#define INVOKEINTERFACE	opc_invokeinterface
#define NEW		opc_new
#define NEWARRAY	opc_newarray
#define ANEWARRAY	opc_anewarray
#define MULTIANEWARRAY	opc_multianewarray
#define ARRAYLENGTH	opc_arraylength
#define CHECKCAST	opc_checkcast
#define INSTANCEOF	opc_instanceof
#define MONITORENTER	opc_monitorenter
#define MONITOREXIT	opc_monitorexit
#define ATHROW		opc_athrow
#define WIDE		opc_wide

/* 
 * We remap the constant-pool tags expected by
 * the split verifier into the tags reflecting the
 * state that CVM should have them in at the moment
 * of verification.
 */
#define CONSTANT_Integer    CVM_CONSTANT_Integer
#define CONSTANT_Float      CVM_CONSTANT_Float
#define CONSTANT_Long       CVM_CONSTANT_Long
#define CONSTANT_Double     CVM_CONSTANT_Double
#define CONSTANT_String     CVM_CONSTANT_StringICell
#define CONSTANT_Fieldref   CVM_CONSTANT_Fieldref
#define CONSTANT_Methodref  CVM_CONSTANT_Methodref
#define CONSTANT_InterfaceMethodref  CVM_CONSTANT_InterfaceMethodref
#define CONSTANT_Class      CVM_CONSTANT_ClassTypeID

/*
 * Remap the basic type tags, too.
 */
#define T_BOOLEAN    CVM_T_BOOLEAN
#define T_CHAR       CVM_T_CHAR
#define T_FLOAT      CVM_T_FLOAT
#define T_DOUBLE     CVM_T_DOUBLE
#define T_BYTE       CVM_T_BYTE
#define T_SHORT      CVM_T_SHORT
#define T_INT        CVM_T_INT
#define T_LONG       CVM_T_LONG



/*=========================================================================
 * SUBSYSTEM: Class file verifier (runtime part)
 * FILE:      verifierUtil.c
 * OVERVIEW:  Implementation-specific parts of the KVM verifier.
 *=======================================================================*/

#if 0
-- these used for converting verifier maps to GC maps.
extern void
CVMincrementalSignatureToStackMapEntry(
    CVMMethodBlock* mb, CVMStackMapEntry* rp);

extern CVMBool
CVMincrementalInterpret(
    CVMMethodBlock* mb, CVMPrecomputedStackMapEntry* ep, CVMUint32 pc);

struct CVMPrecomputedStackMaps*
CVMsplitVerifyRewriteStackMapsAsPointerMaps(VfyContext* cntxt);

#endif

static void Vfy_verifyMethodOrAbort(VfyContext* cntxt, const METHOD vMethod);
static CVMBool Vfy_checkNewInstructions(VfyContext* cntxt, METHOD vMethod);
static int Vfy_verifyMethod(VfyContext* cntxt, CLASS vClass, METHOD vMethod);

/*
 * Find a set of maps in the cache of maps created by the classloader.
 */
static CVMsplitVerifyStackMaps*
CVMsplitVerifyGetMaps(CVMClassBlock* cb, CVMMethodBlock* mb);

#ifdef CVM_DEBUG
static void 
printStackMap(VfyContext* cntxt, unsigned short ip);
#endif

#ifdef CVM_TRACE
#define TRACEVERIFIER (CVMcheckDebugFlags(CVM_DEBUGFLAG(TRACE_VERIFIER)) != 0)
#else
#define TRACEVERIFIER CVM_FALSE
#endif


/*=========================================================================
 * FUNCTION:      CVMvfyLookupClass
 * TYPE:          private operation on class type keys
 * OVERVIEW:      Lookup the class with the indicated type name.
 *		  Return pointer to its class block.
 *		  Should never cause class loading.
 *
 * INTERFACE:
 *   parameters:  cntxt:       VfyContext*
 *		  targetClass: The class in question
 *		  thisClass:   class from which the reference is made
 *   returns:     classblock pointer.
 *=======================================================================*/
static CVMClassBlock*
CVMvfyLookupClass(
    VfyContext* cntxt,
    CVMClassTypeID targetClass)
{
    /* TODO: CVMvfyLookupClass isn't suppose to cause any class loading,
       but calling CVMclassLookupByTypeFromClass will if necessary.
       I don't believe causing class loading will result in incorrect
       behavior, but we may want to consider instead using
       CVMclassLookupClassWithoutLoading.
    */
    CVMClassBlock* cb =
	CVMclassLookupByTypeFromClass(cntxt->ee, targetClass, CVM_FALSE, 
					cntxt->classBeingVerified);
    return cb;
}

/*=========================================================================
 * FUNCTION:      vIsAssignable
 * TYPE:          private operation on type keys
 * OVERVIEW:      Check if a value of one type key can be converted to a
 *                value of another type key.
 *
 * INTERFACE:
 *   parameters:  fromKey: from type
 *                toKey: to type
 *                mergedKeyP: pointer to merged type
 *   returns:     CVM_TRUE: can convert.
 *                CVM_FALSE: cannot convert.
 *=======================================================================*/

static CVMBool
vIsAssignable(
    VfyContext*  cntxt,
    VERIFIERTYPE fromKey,
    VERIFIERTYPE toKey,
    VERIFIERTYPE *mergedKeyP)
{
    ITEM_Tag fromTag;
    ITEM_Tag toTag;
    int fromDepth;
    int toDepth;
    CLASS fromClass;
    CLASS toClass;
    CVMBool inRecursion = CVM_FALSE;

    if (mergedKeyP != NULL) {
        /* Most of the time merged type is toKey */
        *mergedKeyP = toKey;
    }
    if (toKey == ITEM_Bogus){
	/* This would seem like a good place to set *mergedKeyP = fromKey */
	/* in which case this clause belongs above the first one */
        return CVM_TRUE;
    }
    if (fromKey == toKey){    /* trivial case */
        return CVM_TRUE;
    }

    fromTag = GET_ITEM_TYPE(fromKey);
    toTag   = GET_ITEM_TYPE(toKey);
    fromDepth = GET_INDIRECTION(fromKey);
    toDepth   = GET_INDIRECTION(toKey);
    /*
     * You can assign a null, an object, an init object,
     * an array, or a new object to a reference type
     */
    if (toKey == ITEM_Reference) {
        return (fromTag == ITEM_Object || /* Object tag includes NULL */
            fromTag == ITEM_InitObject ||
            fromTag == ITEM_NewObject ||
	    fromDepth > 0); /* arrays are objects, too */
    }
    /* 
     * you can't assign anything to a new object, and
     * you can't assign a new object to anything not a reference.
     */
    if ((toTag == ITEM_NewObject) || (fromTag == ITEM_NewObject)) {
        return CVM_FALSE;
    }
    /* You can't assign anything non-null to a null */
    if (toKey == ITEM_Null){
	return CVM_FALSE;
    }
    /* from here on, we only want to look at references */
    if (fromTag != ITEM_Object && fromDepth == 0)
    	return CVM_FALSE;
    if (toTag != ITEM_Object && toDepth == 0)
    	return CVM_FALSE;
    /* you can assign null to any specific reference or array type */
    if (fromKey == ITEM_Null){
        return CVM_TRUE;
    }
    /* if target is Object, then of course it can be assigned */
    if ((fromDepth > toDepth) && toTag == ITEM_Object 
	&& Vfy_baseClassKey(toKey) == Vfy_getSystemClassName(java_lang_Object)){
	    return CVM_TRUE;
    }

    /* when assigning non-identical references, take a closer look */
lookMoreDeeply:

    if (fromDepth > 0){
	/*
	 * source is an array. It can be assigned if:
	 * a) destination is Object (already handled by above clause)
	 * b) destination is interface Serializable or Cloneable
	 * c) destination is array, and base types are primitive 
	 *    and identical
	 * d) destination is array, and base types are 
	 *    object (includes array-of-array) and assignable.
	 */
	if (toKey == Vfy_getSystemVfyType(java_lang_Cloneable) ||
	    toKey == Vfy_getSystemVfyType(java_io_Serializable)){
	    	/* Interfaces are treated like java.lang.Object 
		   in the verifier. */
            	if (!inRecursion && mergedKeyP != NULL) {
                    *mergedKeyP = Vfy_getObjectVerifierType();
            	}
		return CVM_TRUE;
	}
	if (toDepth > 0){
	    /* handle cases c & d by (tail) recursion */
 	    fromDepth -= 1;
	    toDepth -= 1;
	    toKey   = Vfy_getArrayElementType(toKey);
	    fromKey = Vfy_getArrayElementType(fromKey);
	    inRecursion = CVM_TRUE;
	    goto lookMoreDeeply;
	}
	return CVM_FALSE;
    }
    /* If destination is an array, we're out of luck at this point */
    if (toDepth > 0){
	return CVM_FALSE;
    }
    /* 
     * Can only be looking at class types here
     */
    if (fromTag != ITEM_Object || toTag != ITEM_Object){
	return CVM_FALSE;
    }
    CVMassert(CVMtypeidFieldIsRef(Vfy_toClassKey(fromKey)));
    CVMassert(CVMtypeidFieldIsRef(Vfy_toClassKey(toKey)));

    fromClass = CVMvfyLookupClass(cntxt, Vfy_toClassKey(fromKey));
    if (fromClass == NULL)
	return CVM_FALSE; /* can't be -- a class not loaded? */

    toClass = CVMvfyLookupClass(cntxt, Vfy_toClassKey(toKey));    
    if (toClass == NULL)
	return CVM_FALSE; /* can't be -- a class not loaded? */

    /* Interfaces are treated like java.lang.Object in the verifier. */
    if (CVMcbIs(toClass,INTERFACE)) {
 	if (!inRecursion && mergedKeyP != NULL) {
            *mergedKeyP = Vfy_getObjectVerifierType();
        }
        return CVM_TRUE;
    }
    /* crawl up the superclass hierearchy of fromClass until we
     * find toClass or not.
     */
    fromClass = CVMcbSuperclass(fromClass);
    while (fromClass != NULL){
	if (fromClass == toClass)
	    return CVM_TRUE;
	fromClass = CVMcbSuperclass(fromClass);
    }
    /* toClass not in the hierarchy of fromClass. not assignable. */
    return CVM_FALSE;
}


/*
 * Helper functions for looking up fields and methods up the hierarchy.
 * They both take similar parameters:
 *	memberClass:	where to start looking
 *	nameTypeKey:	field or method signature we're looking for.
 * They return:
 *	CVMMethodBlock* or CVMFieldBlock* of the field or method found, or NULL
 */

static CVMFieldBlock*
CVMvfyLookupField(
    CLASS  memberClass,
    CVMFieldTypeID  nameTypeKey)
{
    int i, n;
    while (memberClass != NULL) {
	n = CVMcbFieldCount(memberClass);
	for (i = 0; i < n; i++){
	    CVMFieldBlock*  fp = CVMcbFieldSlot(memberClass, i);
	    if (CVMtypeidIsSame(CVMfbNameAndTypeID(fp), nameTypeKey)){
		return fp;
	    }
	}
	memberClass = CVMcbSuperclass(memberClass);
    }
    return NULL;
}

static CVMMethodBlock*
CVMvfyLookupMethod(
    CLASS  memberClass,
    CVMMethodTypeID nameTypeKey)
{
    int i, n;
    while (memberClass != NULL) {
	n = CVMcbMethodCount(memberClass);
	for (i = 0; i < n; i++){
	    CVMMethodBlock*  mp = CVMcbMethodSlot(memberClass, i);
	    if (CVMtypeidIsSame(CVMmbNameAndTypeID(mp), nameTypeKey)){
		return mp;
	    }
	}
	memberClass = CVMcbSuperclass(memberClass);
    }
    return NULL;
}

/*=========================================================================
 * FUNCTION:      vIsProtectedAccess
 * TYPE:          private operation on type keys
 * OVERVIEW:      Check if the access to a method or field is to
 *                a protected field/method of a superclass.
 *
 * INTERFACE:
 *   parameters:  thisClass:   The class in question
 *                index:       Constant pool entry of field or method
 *                isMethod:    true for method, false for field.
 *=======================================================================*/

static CVMBool 
vIsProtectedAccess(
    CLASS thisClass,
    POOLINDEX index,
    CVMBool isMethod) 
{
    CVMConstantPool* constPool = Cls_getPool(thisClass);
    int memberClassIndex =
        CVMcpGetMemberRefClassIdx(constPool, index);
    CVMClassTypeID tClass = Pol_getClassKey(constPool, memberClassIndex);
    CLASS memberClass;
    int nameTypeIndex;

    /* if memberClass isn't a superclass of this class, then we don't worry
     * about this case */
    for (memberClass = thisClass; ; memberClass = CVMcbSuperclass(memberClass))
    {
        if (memberClass == NULL) {
            return CVM_FALSE;
        } else if (CVMcbClassName(memberClass) == tClass) {
            break;
        }
    }

    nameTypeIndex = CVMcpGetMemberRefTypeIDIdx(constPool, index);
    if (isMethod){
	CVMMethodTypeID nameTypeKey = CVMcpGetMethodTypeID(constPool, nameTypeIndex);
	CVMMethodBlock*  mp = CVMvfyLookupMethod(memberClass, nameTypeKey);
	if (mp == NULL)
	    return CVM_FALSE;
	return (CVMmbIs(mp, PROTECTED) && 
		!CVMisSameClassPackage(NULL, thisClass, CVMmbClassBlock(mp)));
    }else{
	CVMFieldTypeID nameTypeKey = CVMcpGetFieldTypeID(constPool, nameTypeIndex);
	CVMFieldBlock*  fp = CVMvfyLookupField(memberClass, nameTypeKey);
	if (fp == NULL)
	    return CVM_FALSE;
	return (CVMfbIs(fp, PROTECTED) && 
		!CVMisSameClassPackage(NULL, thisClass, CVMfbClassBlock(fp)));
    }
    return CVM_FALSE;
}

/*
 * Lookup a method in the named class.
 */
 static METHOD 
 Cls_lookupMethodByName(
    CLASS thisClass,
    CLASS targetClass,
    CVMMethodTypeID nameAndType)
{
    /* do simple lookups until we find one that is visible to us */
    CVMClassBlock* containingClass = targetClass;
    CVMMethodBlock* mp;
    while (targetClass != NULL){
	mp = CVMvfyLookupMethod(targetClass, nameAndType);
	if (mp == NULL)
	    return NULL;
	/* test visibility. If not visible, keep looking */
	/* for purposes of this test, PROTECTED is visible enough */
	if (CVMmbIs(mp, PUBLIC) || CVMmbIs(mp, PROTECTED)){
		return mp;
	}
	containingClass = CVMmbClassBlock(mp);
	if ((thisClass == NULL) || (thisClass == containingClass)){
		return mp;
	}
	if (!CVMmbIs(mp, PRIVATE) &&
	    CVMisSameClassPackage(NULL, thisClass, containingClass)){
		return mp;
	}
	/* not good enough. Keep looking */
	targetClass = CVMcbSuperclass(containingClass);
    }
    return NULL;
}

/*
 * Lookup a field in the named class.
 */
static FIELD
Cls_lookupFieldByName(
    CLASS thisClass,
    CLASS targetClass,
    CVMFieldTypeID nameAndType)
{
    /* do simple lookups until we find one that is visible to us */
    CVMClassBlock* containingClass = targetClass;
    CVMFieldBlock* fp;
    while (targetClass != NULL){
	fp = CVMvfyLookupField(targetClass, nameAndType);
	if (fp == NULL)
	    return NULL;
	/* test visibility. If not visible, keep looking */
	/* for purposes of this test, PROTECTED is visible enough */
	if (CVMfbIs(fp, PUBLIC) || CVMfbIs(fp, PROTECTED)){
		return fp;
	}
	containingClass = CVMfbClassBlock(fp);
	if ((thisClass == NULL) || (thisClass == containingClass)){
		return fp;
	}
	if (!CVMfbIs(fp, PRIVATE) &&
	    CVMisSameClassPackage(NULL, thisClass, containingClass)){
		return fp;
	}
	/* not good enough. Keep looking */
	targetClass = CVMcbSuperclass(containingClass);
    }
    return NULL;
}

/*=========================================================================
 * FUNCTION:      CVMvfyMakeVerifierType
 * TYPE:          operation on type keys producing VERIFIERTYPE
 * OVERVIEW:      Change an individual CVMClassTypeID to a VERIFIERTYPE.
 *		  This operation is generally done inline, but needs more
 *		  help when an array-class-type is encountered.
 *		  See Vfy_toVerifierType
 *
 * INTERFACE:
 *   parameters:  key: array CVMClassTypeID to convert.
 *          
 *   returns:     canonical VERIFIERTYPE representation.
 *=======================================================================*/
VERIFIERTYPE
CVMsplitVerifyMakeVerifierType(CVMClassTypeID key){
    int tag;
    int depth = CVMtypeidGetArrayDepth(key);
    CVMFieldTypeID basetype = CVMtypeidGetArrayBasetype(key);
    CVMassert(depth > 0); /* else we are here in error */
    switch (basetype) {
        case CVM_TYPEID_INT:	tag = ITEM_Integer; break;
        case CVM_TYPEID_SHORT:	tag = ITEM_Short;   break;
        case CVM_TYPEID_CHAR:	tag = ITEM_Char;    break;
        case CVM_TYPEID_BYTE:	tag = ITEM_Byte;    break;
        case CVM_TYPEID_BOOLEAN:tag = ITEM_Byte;    break;
        case CVM_TYPEID_FLOAT:	tag = ITEM_Float;   break;
        case CVM_TYPEID_DOUBLE:	tag = ITEM_Double;  break;
        case CVM_TYPEID_LONG:	tag = ITEM_Long;    break;
        default:
            CVMassert(CVMtypeidFieldIsRef(basetype));
	    return MAKE_FULLINFO(ITEM_Object, depth, basetype);
    }
    return MAKE_FULLINFO(tag, depth, 0);
}

/*=========================================================================
 * FUNCTION:      change_Field_to_StackType
 * TYPE:          private operation on type keys
 * OVERVIEW:      Change an individual field type to a stack type
 *
 * INTERFACE:
 *   parameters:  fieldType: field type
 *                stackTypeP: pointer for placing the corresponding stack type(s)
 *   returns:     number of words occupied by the resulting type.
 *=======================================================================*/

static int 
change_Field_to_StackType(CVMClassTypeID fieldType, VERIFIERTYPE* stackTypeP) 
{
    int depth = CVMtypeidGetArrayDepth(fieldType);
    CVMFieldTypeID basetype = CVMtypeidGetArrayBasetype(fieldType);
    int tag;
    switch (basetype) {
        case CVM_TYPEID_SHORT:
	    tag = ITEM_Short;
	    goto smalltype;
        case CVM_TYPEID_CHAR:
	    tag = ITEM_Char;
	    goto smalltype;
        case CVM_TYPEID_BYTE:
        case CVM_TYPEID_BOOLEAN:
	    tag = ITEM_Byte;
	smalltype:
	    if (depth > 0){
		*stackTypeP = MAKE_FULLINFO(tag, depth, 0);
		return 1;
	    }
	    /* else depth is 0 so fall thru and make it an int */
        case CVM_TYPEID_INT:
            *stackTypeP = MAKE_FULLINFO(ITEM_Integer, depth, 0);
            return 1;
        case CVM_TYPEID_FLOAT:
            *stackTypeP = MAKE_FULLINFO(ITEM_Float, depth, 0);
            return 1;
        case CVM_TYPEID_DOUBLE:
	    if (depth == 0){
		*stackTypeP++ = MAKE_FULLINFO(ITEM_Double, 0, 0);
		*stackTypeP   = MAKE_FULLINFO(ITEM_Double_2, 0, 0);
		return 2;
	    } else {
		*stackTypeP = MAKE_FULLINFO(ITEM_Double, depth, 0);
		return 1;
	    }
        case CVM_TYPEID_LONG:
	    if (depth == 0){
		*stackTypeP++ = MAKE_FULLINFO(ITEM_Long, 0, 0);
		*stackTypeP   = MAKE_FULLINFO(ITEM_Long_2, 0, 0);
		return 2;
	    } else {
		*stackTypeP = MAKE_FULLINFO(ITEM_Long, depth, 0);
		return 1;
	    }
	case CVM_TYPEID_VOID:
	    *stackTypeP = MAKE_FULLINFO(ITEM_Void, 0, 0);
	    return 0; /* For return types only!! */
        default:
            *stackTypeP = MAKE_FULLINFO(ITEM_Object, depth, basetype);
            return 1;
    }
}

/*=========================================================================
 * FUNCTION:      getStackMap
 * TYPE:          private operation on type keys
 * OVERVIEW:      Get the recorded stack map of a given target ip.
 *
 * INTERFACE:
 *   parameters:  thisMethod: method being verified.
 *                this_ip: current ip (unused for now).
 *                target_ip: target ip (to look for a recored stack map).
 *   returns:     a stack map
 *=======================================================================*/

static CVMsplitVerifyMap* 
getStackMap(VfyContext* cntxt, IPINDEX target_ip) 
{
    CVMsplitVerifyStackMaps* stackMaps;
    if (target_ip == cntxt->cachedMapIP){
	return cntxt->cachedMap;
    }
    stackMaps = cntxt->maps;
    if (stackMaps == NULL) {
	  /* in our context this may be an error, but let it pass */
        return NULL;
    } else {
        int range   = stackMaps->nEntries;
	int mapSize = stackMaps->entrySize;
	CVMsplitVerifyMap*  lowMap = stackMaps->maps;
	CVMsplitVerifyMap*  highMap = (CVMsplitVerifyMap*)
		((char*)lowMap + ((range - 1) * mapSize));
	/*
	 * do binary search until we narrow it down to a small set
	 * of entries which we will search linearly.
	 * Since they are sorted by PC we can do this.
	 * Note how most methods are small so the binary-search part
	 * never gets executed.
	 */
	while (range > 5){
	    int midRange = range / 2;
	    CVMsplitVerifyMap*  midMap = (CVMsplitVerifyMap*)
		    ((char*)lowMap+(midRange * mapSize));
	    if (target_ip == midMap->pc){
		return midMap; /* found it */
	    } else if (target_ip < midMap->pc){
		highMap = (CVMsplitVerifyMap*)((char*)midMap - mapSize);
		range = midRange;
	    } else {
		lowMap = (CVMsplitVerifyMap*)((char*)midMap + mapSize);
		range -= midRange + 1;
	    }
	}

	/* small set of maps. Just do a linear search */
	while (lowMap <= highMap){
	    if (target_ip == lowMap->pc) {
		return lowMap;
	    }
	    lowMap = (CVMsplitVerifyMap*)((char*)lowMap + mapSize);
	}
    }
    return NULL;
}

/*=========================================================================
 * FUNCTION:      matchStackMap
 * TYPE:          private operation on type keys
 * OVERVIEW:      Match two stack maps.
 *
 * INTERFACE:
 *   parameters:  thisMethod: method being verified.
 *                target_ip: target ip (to look for a recored stack map).
 *                flags: bit-wise or of the SM_* flags.
 *   returns:     CVM_TRUE if match, CVM_FALSE otherwise.
 *=======================================================================*/

static void 
Vfy_matchStackMap(
    VfyContext* cntxt,
    IPINDEX target_ip,
    int flags,
    int throw_code)
{
    CVMBool target_needs_initialization;
    unsigned short nstack;
    unsigned short nlocals;
    unsigned short i;
    /* NOTE: this may go faster if we make local copies of cntxt->vLocals & vStack */
    VERIFIERTYPE* vStack;
    VERIFIERTYPE* vLocals;
    
    CVMsplitVerifyMap* stackMap = getStackMap(cntxt, target_ip);
    CVMsplitVerifyMapElement* elep;

#if CVM_DEBUG
    if (TRACEVERIFIER) {
        CVMconsolePrintf("Match stack maps (this=%ld, target=%ld)%s%s%s\n",
                (long)(cntxt->vErrorIp), (long)target_ip,
            (flags & SM_CHECK) ? " CHECK" : "",
            (flags & SM_MERGE) ? " MERGE" : "",
            (flags & SM_EXIST) ? " EXIST" : "");
    }
#endif

    if (stackMap == NULL) {
#if CVM_DEBUG
        if (TRACEVERIFIER) {
            CVMconsolePrintf("No recorded stack map at %ld\n", (long)target_ip);
        }
#endif
        if (flags & SM_EXIST) goto doThrow;
	return;
    }

#if CVM_DEBUG
    if (TRACEVERIFIER) {
        printStackMap(cntxt, target_ip);
    }
#endif

    vStack  = cntxt->vStack;
    vLocals = cntxt->vLocals;
    nlocals = stackMap->nLocals;
    elep = stackMap->mapElements;
    /* We implicitly need to still perform initialization if at least
     * one local or stack variable contains ITEM_InitObject */
    target_needs_initialization = CVM_FALSE;

    for (i = 0; i < nlocals; i++) {
        CVMsplitVerifyMapElement ty = *elep++;
	/* ty is the most likely value  for merged type */
        CVMsplitVerifyMapElement mergedType = ty;
        if (ty == ITEM_InitObject) { 
            target_needs_initialization = CVM_TRUE;
        }
        if ((SM_CHECK & flags) && 
	    !Vfy_vIsAssignable(cntxt, vLocals[i], ty, &mergedType)) {
		goto doThrow;
        }
        if (SM_MERGE & flags) {
            vLocals[i] = mergedType;
        }
    }
    if (SM_MERGE & flags) {
        for (i = nlocals; i < cntxt->vFrameSize; i++) {
            vLocals[i] = ITEM_Bogus;
        }
    }

    nstack = stackMap->sp;
    if ((SM_CHECK & flags) && nstack != cntxt->vSP) {
        goto doThrow;
    }
    if (SM_MERGE & flags) {
        cntxt->vSP = nstack;
    }
    for (i = 0; i < nstack; i++) {
        CVMsplitVerifyMapElement ty = *elep++;
        CVMsplitVerifyMapElement mergedType = ty; /* <= most likely value */
        if (ty == ITEM_InitObject) { 
            target_needs_initialization = CVM_TRUE;
        }
        if ((SM_CHECK & flags) && 
	    !Vfy_vIsAssignable(cntxt, vStack[i], ty, &mergedType)) {
		goto doThrow;
        }
        if (SM_MERGE & flags) {
            vStack[i] = mergedType;
        }
    }

    if (cntxt->vIsConstructor) {
        if (SM_CHECK & flags) { 
            if (cntxt->vNeedInitialization && !target_needs_initialization) { 
                /* We still need to perform initialization, but we are
                 * merging into a location that doesn't.
                 */
                goto doThrow;
            }
        }

        if (SM_MERGE & flags) { 
            cntxt->vNeedInitialization = target_needs_initialization;
        }
    }

    return;
    /*
     * a mismatch occured. Throw the indicated error type.
     */
doThrow:
    Vfy_throw(cntxt, throw_code);
}

/*=========================================================================
 * FUNCTION:      checkNewObject
 * TYPE:          private operation on type keys
 * OVERVIEW:      Check if uninitialized objects exist on backward branches.
 *
 * INTERFACE:
 *   parameters:  this_ip: current ip
 *                target_ip: branch target ip
 *   returns:     CVM_TRUE if no uninitialized objects exist, CVM_FALSE otherwise.
 *=======================================================================*/

static CVMBool
checkNewObject(VfyContext* cntxt, IPINDEX this_ip, IPINDEX target_ip) 
{
#if 0
    /* This check is now disabled. See bug 6819090 and related bugs
       like 4817320 and 6293528. */
    if (target_ip < this_ip) {
        int i, n;
	VERIFIERTYPE* v;
        n = cntxt->vFrameSize;
	v = cntxt->vLocals;
        for (i = 0; i < n; i++) {
            if (Vfy_isVerifierTypeForNew(v[i])) {
                return CVM_FALSE;
            }
        }
        n = cntxt->vSP;
	v = cntxt->vStack;
        for (i = 0; i < n; i++) {
            if (Vfy_isVerifierTypeForNew(v[i])) {
                return CVM_FALSE;
            }
        }
    }
#endif
    return CVM_TRUE;
}

/*=========================================================================
 * FUNCTION:	  allocateContext
 * OVERVIEW:	  allocate the VfyContext and all working storage. It will
 *		  be sized large enough to verify any method in this class.
 *		  It will consist of a single allocation, so a single call 
 *		  to free() will deallocate it.
 *=======================================================================*/
static VfyContext*
allocateContext(CVMExecEnv* ee, CLASS thisClass){
    /*
     * look at all methods in this class.
     * size working memory to the maximum required for any.
     */
    size_t maxLocals = 0;
    size_t maxStack = 0;
    size_t maxCodeLength = 0;
    int    i, nMethods;
    METHOD thisMethod;
    size_t totalAllocation;
    VfyContext* cntxt;

    nMethods = CVMcbMethodCount(thisClass);
    for (i = 0; i < nMethods; i++) {
	size_t nLocals;
	size_t nStack;
	size_t codeLength;

	thisMethod = CVMcbMethodSlot(thisClass, i);

	/* Skip special synthesized methods. */
	if (CVMmbIsMiranda(thisMethod)) {
	    continue;
	}
	/* Skip abstract and native methods. */
	if (!CVMmbIsJava(thisMethod)) {
	    continue;
	}

	/* Measure the size */
	nLocals = CVMmbMaxLocals(thisMethod);
	nStack = CVMjmdMaxStack(CVMmbJmd(thisMethod));
	codeLength = Mth_getBytecodeLength(thisMethod);
#define SET_MAX(a, b) if (a < b){a = b;}
	SET_MAX(maxLocals, nLocals);
	SET_MAX(maxStack, nStack);
	SET_MAX(maxCodeLength, codeLength);
    }
    /*
     * We are going to waste some space here: the maxLocals & maxStack might
     * be in different methods. Furthermore, we might not need the NEWInstruction
     * array at all -- though it is very likely for a method of any complexity.
     * All this is being traded off for the time savings of only doing one
     * malloc call for the whole class.
     */
    totalAllocation = sizeof(VfyContext) + sizeof(VERIFIERTYPE)*(maxLocals+maxStack)
	+ ((maxCodeLength + 31) / 8);

    cntxt = (VfyContext*)malloc(totalAllocation);
    if (cntxt == NULL)
	return NULL;

    /* some invariants of this execution */
    cntxt->ee = ee;
    cntxt->classBeingVerified = thisClass;

    /* stack comes directly after the VfyContext structure */
    cntxt->vStack = (VERIFIERTYPE*)(cntxt+1);
    /* locals follow stack */
    cntxt->vLocals = cntxt->vStack + maxStack;
    /* NEWInstructions follows locals */
    cntxt->NEWInstructions = (unsigned long*)(cntxt->vLocals + maxLocals);
    return cntxt;
}

/*=========================================================================
 * FUNCTION:      verifyClass
 * TYPE:          public operation on classes.
 * OVERVIEW:      Perform byte-code verification of a given class. Iterate
 *                through all methods.
 *
 * INTERFACE:
 *   parameters:  thisClass: class to be verified.
 *   returns:     0 if verification succeeds, error code if verification
 *                fails.
 *=======================================================================*/

int
CVMsplitVerifyClass(CVMExecEnv* ee, CLASS thisClass, CVMBool isRedefine)
{
    int i;
    int nMethods;
    int result = 0;
    VfyContext* cntxt = NULL;
    METHOD thisMethod;
    nMethods = CVMcbMethodCount(thisClass);
    if (nMethods > 0) {
        if (!CVMcbCheckRuntimeFlag(thisClass, VERIFIED)) {
            /* Verify all methods */
	    cntxt = allocateContext(ee, thisClass);
	    if (cntxt == NULL){
		if (!CVMexceptionOccurred(ee)){
		    CVMthrowVerifyError(ee, "%C: Out of memory", thisClass);
		}
		result = VE_OUT_OF_MEMORY;
		goto deallocateAndReturn;
	    }
            for (i = 0; i < nMethods; i++) {
                thisMethod = CVMcbMethodSlot(thisClass, i);

                /* Skip special synthesized methods. */
                if (CVMmbIsMiranda(thisMethod)) {
                    continue;
                }
                /* Skip abstract and native methods. */
                if (!CVMmbIsJava(thisMethod)) {
                    continue;
                }

               /*
                * Call the core routine
                */
#if 0
                CVMconsolePrintf("*** SPLIT: %C.%M\n", thisClass, thisMethod);
#endif
#ifdef CVM_JVMTI
                if (isRedefine) {
                    /* The class being verified is the new class. However,
                     * that class will not be in the list of classes as we
                     * just use it to hold the methods during the redefine
                     * phase. The oldcb is the class of record for this class
                     * so we want to make sure we use that as the target of
                     * any verifications.
                     */
                    CVMClassBlock *oldcb = CVMjvmtiGetCurrentRedefinedClass(ee);
                    CVMassert(oldcb != NULL);
                    result = Vfy_verifyMethod(cntxt, oldcb, thisMethod);
                } else
#endif
                {
                    result = Vfy_verifyMethod(cntxt, thisClass, thisMethod);
                }
                if (result != 0) {
		    if (!CVMexceptionOccurred(ee)){
			CVMthrowVerifyError(ee, "%C.%M: %s", thisClass,
			    thisMethod, getMsg(result));
		    }
                    break;
                }
            }

        }
    }
    if (result == 0) {
        CVMcbSetRuntimeFlag(thisClass, ee, VERIFIED);
    }
deallocateAndReturn:
    CVMsplitVerifyClassDeleteMaps(thisClass);
    if (cntxt != NULL){
	free(cntxt);
    }
    return result;
}

/*=========================================================================
 * Debugging and printing operations
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      printStackType, printStackMap
 * TYPE:          debugging operations
 * OVERVIEW:      Print verifier-related information
 *
 * INTERFACE:
 *   parameters:  pretty obvious
 *   returns:
 *=======================================================================*/

#ifdef CVM_DEBUG

#define EMPTY_FLAG 0x1F

/* Print a string, and add padding. */
static void
print_str(char *str, int depth)
{
    int max = 30;
    int i;
    for (i=0; i<depth; i++){
	CVMconsolePrintf("[");
	max -= 1;
    }
    CVMconsolePrintf("%s", str);
    i = max - strlen(str);
    if (i > 0) {
	/* HOPE */
        CVMconsolePrintf("%*s", i, " ");
    }
}

/* Print a type key */
static void
printStackType(VERIFIERTYPE key)
{
    char buf[128];
    CVMClassTypeID objectType;
    int depth;
    depth = GET_INDIRECTION(key);
    switch (GET_ITEM_TYPE(key)){
    case EMPTY_FLAG:
        print_str("-",0);
        return;
    case ITEM_Bogus:
        print_str("*",0);
        return;
    /* These aren't on the stack, but arrays of them are */
    case ITEM_Byte:
        print_str("B", depth);
	return;
    case ITEM_Short:
        print_str("S", depth);
	return;
    case ITEM_Char:
        print_str("C", depth);
	return;
    case ITEM_Integer:
        print_str("I", depth);
        return;
    case ITEM_Float:
        print_str("F", depth);
        return;
    case ITEM_Double:
        print_str("D", depth);
        return;
    case ITEM_Long:
        print_str("J", depth);
        return;
    case ITEM_InitObject:
        print_str("this", depth);
        return;
    case ITEM_Long_2:
        print_str("J2", depth);
        return;
    case ITEM_Double_2:
        print_str("D2", depth);
        return;
    case ITEM_NewObject:
	sprintf(buf, "new %ld", (long)Vfy_retrieveIpForNew(key));
	print_str(buf, depth);
	return;
    case ITEM_Reference:
        print_str("Ref", depth);
        return;
    default: /* ITEM_Object and ITEM_InitObject and Item_NULL ? */
	objectType = GET_EXTRA_INFO(key);
	if (objectType == 0){
	    print_str("null", depth);
	}else{
	    CVMtypeidFieldTypeToCString(GET_EXTRA_INFO(key), buf, sizeof(buf));
	    print_str(buf, depth);
	}
        return;
    }
}

static CVMsplitVerifyMapElement*
printStackInfo(
    int n,
    char designator,
    CVMsplitVerifyMapElement* actual,
    int nActual,
    CVMsplitVerifyMapElement* claimed,
    int nClaimed
){
    int i;
    for (i = 0; i < n; i++){
        CVMsplitVerifyMapElement ty;
        if (i < 100) {
            CVMconsolePrintf(" ");
            if (i < 10) {
                CVMconsolePrintf(" ");
            }
        }
        CVMconsolePrintf("%c[%ld]  ", designator, (long)i);
        if (i < nActual) {
            ty = actual[i];
        } else {
            ty = (CVMsplitVerifyMapElement)EMPTY_FLAG;
        }
        printStackType(ty);
        if (claimed != NULL) {
            if (i < nClaimed) {
                ty = *claimed++;
            } else {
                ty = (CVMsplitVerifyMapElement)EMPTY_FLAG;
            }
            printStackType(ty);
        }
        CVMconsolePrintf("\n");
    }
    return claimed;
}


#undef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))

/* Print the derived stackmap and recorded stackmap (if it exists) at
 * a given ip.
 */
static void
printStackMap(VfyContext* cntxt, unsigned short ip)
{
    CVMsplitVerifyMap* stackMap = getStackMap(cntxt, ip);
    CVMsplitVerifyMapElement* mep;
    int nlocals, nstack;
    int lTop;

    for (lTop = cntxt->vFrameSize; lTop > 0; lTop--) {
        if (cntxt->vLocals[lTop - 1] != ITEM_Bogus) {
            break;
        }
    }

    CVMconsolePrintf("__SLOT__DERIVED_______________________");
    if (stackMap != NULL) {
        CVMconsolePrintf("RECORDED______________________\n");
	nlocals = stackMap->nLocals;
	nstack = stackMap->sp;
	mep = stackMap->mapElements;
    } else {
        CVMconsolePrintf("\n");
	nlocals = 0;
	nstack = 0;
        mep = NULL;
    }

    mep = printStackInfo(MAX(lTop, nlocals), 'L', 
			  cntxt->vLocals, lTop, mep, nlocals);
    printStackInfo(MAX(cntxt->vSP, nstack), 'S', 
			  cntxt->vStack, cntxt->vSP, mep, nstack);
}

#endif /* INCLUDEDEBUGCODE */

/*=========================================================================
 * FUNCTION:      Vfy_initializeLocals
 * TYPE:          private operation
 * OVERVIEW:      Initialize the local variable types from the method
 *                signature
 *
 * INTERFACE:
 *   parameters:  None
 *   returns:     Nothing
 *=======================================================================*/

static void 
Vfy_initializeLocals(VfyContext* cntxt) {
    /* Fill in the initial derived stack map with argument types. */
    METHOD meth = cntxt->methodBeingVerified;
    /* Signature handling taken from full verifier */

    int nargs;
    int i;
    int n;
    CVMSigIterator iter;
    CVMTypeID retDummy[2];
    CVMMethodTypeID mname = Mth_getName(meth);

    n = 0;
    cntxt->vNeedInitialization = CVM_FALSE;
    if (!Mth_isStatic(meth)) {
	/* add one extra argument for instance methods */
        n++;
        if (cntxt->vIsConstructor &&
	    Mth_getClass(meth)!= CVMsystemClass(java_lang_Object)) 
	{
            cntxt->vLocals[0] = MAKE_FULLINFO(ITEM_InitObject, 0, 0);
            cntxt->vNeedInitialization = CVM_TRUE;
        } else {
	    cntxt->vLocals[0] = 
			Vfy_toVerifierType(Cls_getKey(Mth_getClass(meth)));
	}
    }
    CVMtypeidGetSignatureIterator(CVMtypeidGetTypePart(mname), &iter);
    nargs = CVM_SIGNATURE_ITER_PARAMCOUNT(iter);
    for (i = 0; i < nargs; i++) {
        n += change_Field_to_StackType(
	    CVM_SIGNATURE_ITER_NEXT(iter), &(cntxt->vLocals[n]));
    }

    change_Field_to_StackType(
    	CVM_SIGNATURE_ITER_RETURNTYPE(iter), &retDummy[0]);
    cntxt->returnSig = retDummy[0];
}

/*=========================================================================
 * FUNCTION:      Vfy_returnVoid
 * TYPE:          private operation
 * OVERVIEW:      Check that a return is valid for this method. If the
 *                method is <init> make sure that 'this' was initialized
 *
 * INTERFACE:
 *   parameters:  None.
 *   returns:     Nothing.
 *=======================================================================*/

static void 
Vfy_returnVoid(VfyContext* cntxt) {
    if (cntxt->returnSig != MAKE_FULLINFO(ITEM_Void, 0, 0)){
        Vfy_throw(cntxt, VE_EXPECT_RETVAL);
    }
    if (cntxt->vIsConstructor){
        if (cntxt->vNeedInitialization) { 
            Vfy_throw(cntxt, VE_RETURN_UNINIT_THIS);
        }
    }
}

/*=========================================================================
 * FUNCTION:      Vfy_popReturn
 * TYPE:          private operation
 * OVERVIEW:      Check that a return is valid for this method.
 *
 * INTERFACE:
 *   parameters:  returnType: the type to be returned.
 *   returns:     Nothing.
 *=======================================================================*/

static void
Vfy_popReturn(VfyContext* cntxt, VERIFIERTYPE returnType) {

    returnType = Vfy_pop(cntxt, returnType);

    if (GET_ITEM_TYPE(cntxt->returnSig) == ITEM_Void){
        Vfy_throw(cntxt, VE_EXPECT_NO_RETVAL);
    }
    if (!Vfy_isAssignable(returnType, cntxt->returnSig)) {
        Vfy_throw(cntxt, VE_RETVAL_BAD_TYPE);
    }

   /*
    * Is this needed here as well as in Vfy_popReturnVoid()?
    */

    CVMassert(!cntxt->vIsConstructor);
     /*
     //   if (vLocals[0] == ITEM_InitObject) {
     //       Vfy_throw(VE_RETURN_UNINIT_THIS);
     //   }
     */
}

/*=========================================================================
 * FUNCTION:      Vfy_setupCalleeContext
 * TYPE:          private operation
 * OVERVIEW:      Pop the arguments of the callee context
 *
 * INTERFACE:
 *   parameters:  Class key for the methodTypeKey
 *   returns:     Nothing.
 *=======================================================================*/

static void
Vfy_setupCalleeContext(VfyContext* cntxt, CVMMethodTypeID methodTypeKey) {
    cntxt->calleeContext = methodTypeKey;
}

/*=========================================================================
 * FUNCTION:      Vfy_popInvokeArguments
 * TYPE:          private operation on stack
 * OVERVIEW:      Pop the arguments of the callee context
 *		  NOT including "this," if any.
 *
 * INTERFACE:
 *   parameters:  None
 *   returns:     The number of words popped
 *=======================================================================*/

static int
Vfy_popInvokeArguments(VfyContext* cntxt) {

    int nargs;
    int nwords = 0;
    CVMClassTypeID ty[2];
    int i, j, k;
    CVMSigIterator iter;

    CVMtypeidGetSignatureIterator(cntxt->calleeContext, &iter);
    nargs = CVM_SIGNATURE_ITER_PARAMCOUNT(iter);
    nwords = CVMtypeidGetArgsSize(cntxt->calleeContext);

   /*
    * Are there the required number of words on the stack?
    */
    if (cntxt->vSP < nwords) {
        Vfy_throw(cntxt, VE_ARGS_NOT_ENOUGH);
    }
    cntxt->vSP -= nwords;

    k = 0;
    for (i = 0; i < nargs; i++) {
        int n = change_Field_to_StackType(
	    CVM_SIGNATURE_ITER_NEXT(iter), ty);
        for (j = 0; j < n; j++) {
            VERIFIERTYPE arg = cntxt->vStack[cntxt->vSP + k];
            if (!Vfy_isAssignable(arg, ty[j])) {
                Vfy_throw(cntxt, VE_ARGS_BAD_TYPE);
            }
            k++;
        }
    }

   cntxt->sigResult = CVM_SIGNATURE_ITER_RETURNTYPE(iter);

   return nwords;
}

/*=========================================================================
 * FUNCTION:      Vfy_pushInvokeResult
 * TYPE:          private operation on stack
 * OVERVIEW:      Push the callee's result
 *
 * INTERFACE:
 *   parameters:  None
 *   returns:     Nothing
 *=======================================================================*/

static void
Vfy_pushInvokeResult(VfyContext* cntxt) {

   /*
    * Push the result type.
    */
    if (cntxt->sigResult != CVM_TYPEID_VOID) {
        CVMClassTypeID ty[2];
        int i;
        int n = change_Field_to_StackType(cntxt->sigResult, ty);
        for (i = 0 ; i < n; i++) {
            Vfy_push(cntxt, ty[i]);
        }
    }
}

void Vfy_ReplaceTypeWithType(VfyContext* cntxt, VERIFIERTYPE fromType,
    VERIFIERTYPE toType)
{
    int i, limit;
    limit = cntxt->vSP;
    for (i = 0; i < limit; i++) {
        if (cntxt->vStack[i] == fromType) {
            cntxt->vStack[i] = toType;
        }
    }
    limit = cntxt->vFrameSize;
    for (i = 0; i < limit; i++) {
        if (cntxt->vLocals[i] == fromType) {
            cntxt->vLocals[i] = toType;
        }
    }

}

/*=========================================================================
 * FUNCTION:      Mth_checkStackMapOffset
 * TYPE:          private operation
 * OVERVIEW:      Validates the stackmap offset to ensure that it does
 *                not exceed code size. 
 *
 * INTERFACE:
 *   parameters:  thisMethod: the method, stackMapIndex: the index
 *   returns:     true if ok, false otherwise. 
 *=======================================================================*/

CVMBool Mth_checkStackMapOffset(VfyContext* cntxt, int stackMapIndex) {
    CVMsplitVerifyStackMaps* stackMaps = cntxt->maps;
    if (stackMaps && stackMapIndex != stackMaps->nEntries) {
        return CVM_FALSE; 
    } 
    return CVM_TRUE;
}

#if CVM_DEBUG

/*=========================================================================
 * FUNCTION:      getByteCodeName()
 * TYPE:          public debugging operation
 * OVERVIEW:      Given a bytecode value, get the mnemonic name of
 *                the bytecode.
 * INTERFACE:
 *   parameters:  bytecode as an integer
 *   returns:     constant string containing the name of the bytecode
 *=======================================================================*/

static const char*
getByteCodeName(CVMUint8 token) {
    if (token <= opc_jsr_w){
	 /* there should be no quick ops in what we're verifying! */
         return CVMopnames[token];
    }
    return "<INVALID>";
}

/*=========================================================================
 * FUNCTION:      Vfy_printVerifyStartMessage
 * TYPE:          private operation
 * OVERVIEW:      Print trace info
 *
 * INTERFACE:
 *   parameters:  vMethod: the method
 *   returns:     Nothing
 *=======================================================================*/

static void Vfy_printVerifyStartMessage(METHOD vMethod) {
    if (TRACEVERIFIER) {
	CVMClassBlock* thisClass = CVMmbClassBlock(vMethod);
            CVMconsolePrintf("Verifying method %C.%M\n",
                    thisClass, vMethod);
    }
}

/*=========================================================================
 * FUNCTION:      Vfy_printVerifyLoopMessage
 * TYPE:          private operation
 * OVERVIEW:      Print trace info
 *
 * INTERFACE:
 *   parameters:  vMethod: the method,ip: the IP address
 *   returns:     Nothing
 *=======================================================================*/

void Vfy_printVerifyLoopMessage(VfyContext* cntxt, METHOD vMethod, IPINDEX ip) {
    if (TRACEVERIFIER) {
        unsigned char *code = Mth_getBytecodes(vMethod);
        CVMUint8 token = code[ip];
        CVMconsolePrintf("Display derived/recorded stackmap at %ld:\n",
	    (long)ip);
        printStackMap(cntxt, ip);
        CVMconsolePrintf("%ld %s\n", (long)ip, getByteCodeName(token));
    }
}


/*=========================================================================
 * FUNCTION:      Vfy_printVerifyEndMessage
 * TYPE:          private operation
 * OVERVIEW:      Print trace info
 *
 * INTERFACE:
 *   parameters:  vMethod: the method
 *   returns:     Nothing
 *=======================================================================*/
static void Vfy_printVerifyEndMessage(VfyContext *cntxt, int result) {
    if (result != 0) {
	METHOD vMethod = cntxt->methodBeingVerified;
	CLASS  vClass  = cntxt->classBeingVerified;
        const char* info = getMsg(result);
        CVMconsolePrintf("Error verifying method %C.%M ", 
			  vClass, vMethod);
        CVMconsolePrintf("Approximate bytecode offset %ld: %s\n", 
			  cntxt->vErrorIp, info);
    }
}

/*=========================================================================
 * FUNCTION:      Vfy_trace1
 * TYPE:          private operation
 * OVERVIEW:      Print trace info
 *
 * INTERFACE:
 *   parameters:  msg: a printf format string, p1: a parameter
 *   returns:     Nothing
 *=======================================================================*/

void Vfy_trace1(char *msg, long p1) {
    if (TRACEVERIFIER) {
        CVMconsolePrintf(msg, p1);
    }
}

#endif /* INCLUDEDEBUGCODE */

 /*
  * Main verity routine
  */

/*=========================================================================
 * FUNCTION:      Vfy_verifyMethod
 * TYPE:          private operation
 * OVERVIEW:      Verify a method
 *
 * INTERFACE:
 *   parameters:  
		  cntxt: the verification context structure
		  vClass:  the method's class
 		  vMethod: the method
 *   returns:     result code: 0 for success, else a VE_* code as returned
 *		  via Vfy_throw.
 *=======================================================================*/

static int
Vfy_verifyMethod(
    VfyContext* cntxt, CLASS vClass, METHOD vMethod)
{

    int res;

#if CVM_DEBUG
    Vfy_printVerifyStartMessage(vMethod);
#endif

    /* set max values for this method. Memory has already been allocated */
    cntxt->vMaxStack = CVMjmdMaxStack(CVMmbJmd(vMethod));
    memset(cntxt->vStack, 0, cntxt->vMaxStack*sizeof(VERIFIERTYPE));

    cntxt->vFrameSize = CVMmbMaxLocals(vMethod);
    memset(cntxt->vLocals, 0, cntxt->vFrameSize*sizeof(VERIFIERTYPE));

   /*
    * This bitmap keeps track of all the NEW instructions that we have
    * already seen.
    */
    memset(cntxt->NEWInstructions, 0, (Mth_getBytecodeLength(vMethod) + 7) / 8);

    cntxt->vSP = 0;                   /* initial stack is empty. */
    cntxt->vContainsSwitch = CVM_FALSE;

   /*
    * All errors from Vfy_verifyMethodOrAbort are longjmped here. If no error
    * is thrown then res will be zero from the call to setjmp
    */
    res = setjmp(cntxt->vJumpBuffer);
    if (res == 0) {
       /*
	* Setup the verification context and call the core function.
	*/
	cntxt->methodBeingVerified    = vMethod;
	cntxt->bytecodesBeingVerified = Mth_getBytecodes(vMethod);
	cntxt->constantpoolLength     = CVMcbConstantPoolCount(vClass);
	cntxt->maps		      = CVMsplitVerifyGetMaps(vClass, vMethod);
	cntxt->vIsConstructor	      = CVMtypeidIsConstructor(
					CVMmbNameAndTypeID(vMethod));

	Vfy_verifyMethodOrAbort(cntxt, vMethod);
	if (!Vfy_checkNewInstructions(cntxt, vMethod)) {
	    Vfy_throw(cntxt, VE_BAD_NEW_OFFSET);
	}
    }

#if CVM_DEBUG
    Vfy_printVerifyEndMessage(cntxt, res);
#endif

    if (res == 0){
	/*
	 * Note if we found a switch & require alignment
	 */
	if (cntxt->vContainsSwitch){
	    CVMjmdFlags(CVMmbJmd(vMethod)) |= CVM_JMD_NEEDS_ALIGNMENT;
	}
#if 0
	/*
	 * before destroying the information for this method, 
	 * rewrite it for the GC.
	 */
	CVMjmdStackMaps(CVMmbJmd(vMethod)) =
	    CVMsplitVerifyRewriteStackMapsAsPointerMaps(cntxt);
#endif
    }

    return res;
}

/*=========================================================================
 * FUNCTION:      Vfy_throw
 * TYPE:          private operation
 * OVERVIEW:      Throw a verification exception
 *
 * INTERFACE:
 *   parameters:  code: the VE_xxxx code
 *   returns:     Does a longjmp back to Vfy_verifyMethod
 *=======================================================================*/

static void
Vfy_throw(VfyContext *cntxt, int code) {
     longjmp(cntxt->vJumpBuffer, code);
 }

/*=========================================================================
 * FUNCTION:      Vfy_checkNewInstructions
 * TYPE:          private operation
 * OVERVIEW:      ITEM_new were all used
 *
 * INTERFACE:
 *   parameters:  thisMethod: the method
 *   returns:     CVM_FALSE if there is an error
 *=======================================================================*/

static CVMBool Vfy_checkNewInstructions(VfyContext *cntxt, METHOD thisMethod) {

   /* We need to double check that all of the ITEM_new's that
    * we stack in the stack map were legitimate.
    */
    const int codeLength = CVMmbCodeLength(thisMethod);
    int i, mapSize;
    CVMsplitVerifyStackMaps* stackMaps = cntxt->maps;
    if (stackMaps != NULL) {
	CVMsplitVerifyMap*  stackMap = stackMaps->maps;
	mapSize = stackMaps->entrySize;
        for (i = stackMaps->nEntries; --i >= 0;
	    stackMap = (CVMsplitVerifyMap*)((char*)stackMap+mapSize))
	{
	    unsigned count = stackMap->nLocals + stackMap->sp;
	    CVMsplitVerifyMapElement* ep = stackMap->mapElements;
	    while (count-- > 0) {
		CVMsplitVerifyMapElement typeKey = *ep++;
		if (GET_ITEM_TYPE(typeKey) == ITEM_NewObject) {
		    int index = Vfy_retrieveIpForNew(typeKey);
		    if (index >= codeLength
			   || cntxt->NEWInstructions == NULL
			   || !IS_NTH_BIT(cntxt->NEWInstructions, index)) {
			return CVM_FALSE;
		    }
		}
	    }
        }
    }
    return CVM_TRUE;
}

/*
 * SPLIT VERIFIER STACKMAP CACHE.
 * The idea here is that we don't want to take extra room in the
 * classblock or in the methodblock for something that only lives
 * from class creation until method verification. So instead we
 * keep the information in this simple cache, using CVMClassTypeID
 * and CVMMethodTypeID as the key.
 * Since we assume that there will be very little information present
 * at any one time, we'll use some really simple data structures.
 */

struct CVMmapCacheClass{
    CVMClassBlock*		cb;
    struct CVMmapCacheClass*	prev;
    struct CVMmapCacheClass*	next;
    struct CVMmapCacheMethod*	methods;
};

struct CVMmapCacheMethod{
    CVMMethodTypeID		methodname;
    CVMsplitVerifyStackMaps*	maps;
    struct CVMmapCacheMethod*	next;
};

/* NOTE: add the following to CVMglobals!! */
static struct CVMmapCacheClass*	mapCaches;

/*
 * Lookup existing. Add if necessary.
 */
static struct CVMmapCacheClass*
getMapCacheClass(CVMClassBlock* cb, CVMBool doAddition){
    struct CVMmapCacheClass* mccp = mapCaches;
    while (mccp != NULL){
	if (mccp->cb == cb)
	    return mccp;
	mccp = mccp->next;
    }
    /* didn't find. */
    if (doAddition == CVM_FALSE)
	return NULL;
    /* Add new */
    mccp = (struct CVMmapCacheClass*)calloc(1, sizeof(*mccp));
    mccp->cb = cb;
    mccp->next = mapCaches;
    if (mapCaches != NULL){
	mapCaches->prev = mccp;
    }
    mapCaches = mccp;
    return mccp;
}

/*
 * Lookup existing. Add if necessary.
 */
static struct CVMmapCacheMethod*
getMapCacheMethod(
    struct CVMmapCacheClass* mccp,
    CVMMethodTypeID   methodname,
    CVMBool	      doAddition)
{
    struct CVMmapCacheMethod* mcmp = mccp->methods;
    while (mcmp != NULL){
	if (mcmp->methodname == methodname)
	    return mcmp;
	mcmp = mcmp->next;
    }
    /* didn't find. */
    if (doAddition == CVM_FALSE)
	return NULL;
    /* Add new */
    mcmp = (struct CVMmapCacheMethod*)calloc(1, sizeof(*mcmp));
    mcmp->methodname = methodname;
    mcmp->next = mccp->methods;
    mccp->methods = mcmp;
    return mcmp;
}

void
CVMsplitVerifyClassDeleteMaps(CVMClassBlock* cb){
    struct CVMmapCacheClass* mccp;
    struct CVMmapCacheMethod* mcmp;
    struct CVMmapCacheMethod* nextMcmp;
    mccp = getMapCacheClass(cb, CVM_FALSE);
    if (mccp == NULL){
	return;
    }
    /* free method structures */
    for (mcmp = mccp->methods; mcmp != NULL; mcmp = nextMcmp){
	nextMcmp = mcmp->next;
	free(mcmp->maps);
	free(mcmp);
    }
    /* free class structure */
    if (mccp->prev == NULL){
	CVMassert(mapCaches == mccp);
	mapCaches = mccp->next;
    } else {
	mccp->prev->next = mccp->next;
    }
    if (mccp->next != NULL){
	mccp->next->prev = mccp->prev;
    }
    free(mccp);
}

static CVMsplitVerifyStackMaps*
CVMsplitVerifyGetMaps(CVMClassBlock* cb, CVMMethodBlock* mb){
    CVMMethodTypeID mid = CVMmbNameAndTypeID(mb);
    struct CVMmapCacheClass* mccp = getMapCacheClass(cb, CVM_FALSE);
    struct CVMmapCacheMethod* mcmp;
    if (mccp == NULL)
	return NULL;
    mcmp = getMapCacheMethod(mccp, mid, CVM_FALSE);
    if (mcmp == NULL)
	return NULL;
    return mcmp->maps;
}

CVMBool
CVMsplitVerifyAddMaps(
    CVMExecEnv*		ee,
    CVMClassBlock*	cb,
    CVMMethodBlock*	mb,
    CVMsplitVerifyStackMaps* mapp)
{
    CVMMethodTypeID mid = CVMmbNameAndTypeID(mb);
    struct CVMmapCacheClass* mccp  = getMapCacheClass(cb, CVM_TRUE);
    struct CVMmapCacheMethod* mcmp = getMapCacheMethod(mccp, mid, CVM_TRUE);

    CVMassert(mid != 0);
    if (mcmp->maps != NULL){
	/*
	 * This is an error because it means that there are multiple
	 * StackMap attributes for this method.
	 * Minimize the amount of garbage left behind and throw a
	 * ClassFormatError.
	 */
	CVMdebugPrintf(("Duplicate split verifier maps for %C.%M\n",
                        cb, mb));
	CVMsplitVerifyClassDeleteMaps(cb);
	if (!CVMexceptionOccurred(ee)){
	    CVMthrowClassFormatError(ee, 
			"%C.%M: has multiple stackmap attributes", cb, mb);
	}
	return CVM_FALSE;
    }
    CVMassert(mapp != NULL);
    mcmp->maps = mapp;
    return CVM_TRUE;
}

CVMBool CVMsplitVerifyClassHasMaps(CVMExecEnv* ee, CVMClassBlock* cb){
    return getMapCacheClass(cb, CVM_FALSE) != NULL;
}

#if 0
/*
 * Rewrite verifier maps as garbage-collector maps.
 * Use the new format.
 */
static int 
verifierToGC(
    CVMsplitVerifyMap* mp,
    CVMPrecomputedStackMapEntry* rp,
    int nLocals,
    CVMBool isReturn)
{
    int bitNo = 1;    /* lowest bit of first word is reserved */
    int bit = 1 << 1; /* lowest bit of first word is reserved */
    int wordNo = 0;
    int nEntries;
    int i, j;
    CVMsplitVerifyMapElement *ep = mp->mapElements;

    rp->v.pc = mp->pc;
    rp->sp = mp->sp;
    /* locals come first */
    /* Handle all the locals in the map, but pad up to the nLocals indicated */
    /*
     * If this map is at a return instruction, then
     * label all locals as non-references, because they are all dead.
     */
    if (isReturn){
	j = 1;
	ep += nEntries;
	bitNo = (nLocals+1) & 0xf;
	wordNo = (nLocals+1) >> 4;
	bit = 1 << bitNo;
	/* do to the end of the stack elements */
	nEntries = mp->sp;
    } else {
	j = 0;
	nEntries = mp->nLocals;
	CVMassert(nEntries <= nLocals);
    }
    for (; j <= 1; j++){
	/*
	 * j == 0 : do locals.
	 * j == 1 : do stack elements.
	 */
	for (i = 0; i < nEntries; i++){
	    CVMsplitVerifyMapElement e = *ep++;
	    switch (GET_ITEM_TYPE(e)){
	    case ITEM_Object:
	    case ITEM_NewObject:
	    case ITEM_InitObject:
		/* for sure this is a pointer */
		rp->v.state[wordNo] |= bit;
		break;
	    case ITEM_Integer:
	    case ITEM_Float:
	    case ITEM_Double:
	    case ITEM_Long:
	    case ITEM_Byte:
	    case ITEM_Short:
	    case ITEM_Char:
		if (GET_INDIRECTION(e) > 0){
		    /* an array is a pointer */
		    rp->v.state[wordNo] |= bit;
		}
		break;
	    case ITEM_Double_2:
	    case ITEM_Long_2:
	    case ITEM_Bogus:
		break; /* these are never pointers */
	    default:
		CVMassert(CVM_FALSE); /* should never see others */
	    }

	    bitNo += 1;
	    bit <<= 1;
	    if (bitNo > 15){
		bitNo = 0;
		bit = 1;
		wordNo += 1;
	    }
	}
	if (j == 0){
	    /* Round up as needed */
	    bitNo = (nLocals+1) & 0xf;
	    wordNo = (nLocals+1) >> 4;
	    bit = 1 << bitNo;
	    /* do to the end of the stack elements */
	    nEntries = mp->sp;
	}
    }

    return wordNo + (bitNo != 0);
}

int precomputedMemoryAllocated;

static CVMBool
CVMincrementalMapsEqual(
    CVMPrecomputedStackMapEntry* a,
    CVMPrecomputedStackMapEntry* b,
    int nChunks)
{
    int i;
    if (a->sp != b->sp || a->v.pc != b->v.pc)
	return CVM_FALSE;
    if (((a->v.state[0] ^ b->v.state[0]) & ~1) != 0)
	return CVM_FALSE;
    for (i = 2; i < nChunks; i++){
	if (a->v.state[i] != b->v.state[i])
	    return CVM_FALSE;
    }
    return CVM_TRUE;
}

static void
CVMincrementalCopyMap(
    CVMPrecomputedStackMapEntry* to,
    CVMPrecomputedStackMapEntry* from,
    int nChunks)
{
    memcpy(to, from, 
	sizeof(CVMPrecomputedStackMapEntry) + (nChunks-1)*sizeof(CVMUint16));
}

struct CVMPrecomputedStackMaps*
CVMsplitVerifyRewriteStackMapsAsPointerMaps(VfyContext* cntxt){
    CVMsplitVerifyStackMaps*      sm = cntxt->maps;
    CVMsplitVerifyMap* 	    mp;
    CVMPrecomputedStackMaps*        smp;
    CVMPrecomputedStackMapEntry*    currEntry;
    METHOD			    vMethod = cntxt->methodBeingVerified;
    CVMPrecomputedStackMapEntry*    end;
    CVMUint8*			    code = CVMmbJavaCode(vMethod);
    int				    nMaps;
    int				    nGcMaps;
    int				    mapSize;
    int				    nExtraBytes;
    int				    i;
    int				    nLocals;
    CVMBool			    oldMapAllocated;
    CVMPrecomputedStackMapEntry	    defaultSigMap;
    CVMPrecomputedStackMapEntry*    oldMap;
    int				    oldPc;

    if (sm == NULL){
	/* many small methods have no maps. This one for instance */
	/* indicate that it HAS been checked */
	return (CVMPrecomputedStackMaps*)1;
    }
    nMaps = sm->nEntries;
    mapSize = sm->entrySize;
    nLocals = cntxt->vFrameSize;
    nExtraBytes = 0;
    nGcMaps = 0;

    if (nLocals + cntxt->vMaxStack > 15){
	/*
	 * iterate through all the maps calculating how much
	 * big map space we'll need. The answer is apparently not zero
	 */
	mp = sm->maps;
	for (i = 0; i < nMaps; i++){
	    /* nWords is sp + maxLocals + 1(reserved bit) / 16 bits per short */
	    /* then round up */
	    int nBits = nLocals + mp->sp + 1;
	    int nWords = (nBits + 15) / 16;
	    /* extra words is one fewer than total number of words */
	    nExtraBytes += (nWords-1) * sizeof(CVMUint16);
	    mp = (CVMsplitVerifyMap*)((char*)mp + mapSize);
	}
	oldMapAllocated = CVM_TRUE;
	oldMap = (CVMPrecomputedStackMapEntry*)calloc(
	    sizeof(CVMPrecomputedStackMapEntry) 
		+ sizeof(CVMUint16) * (nLocals+cntxt->vMaxStack) / 16,
	    1);

    }else{
	oldMapAllocated = CVM_FALSE;
	oldMap = &defaultSigMap;
    }
    /*
     * Now know that total allocation is 
     *   sizeof(CVMPrecomputedStackMaps) + 
     *   (nMaps-1)*sizeof(CVMPrecomputedStackMapEntry) + nExtraBytes
     */
    i = sizeof(CVMPrecomputedStackMaps) + 
        (nMaps-1)*sizeof(CVMPrecomputedStackMapEntry) + nExtraBytes;
    smp = (CVMPrecomputedStackMaps*)calloc(1, i);
    currEntry = smp->smEntries;
    end = (CVMPrecomputedStackMapEntry*)((char*)smp + i);
    /*
     * Now iterate through all verifier maps, creating gc maps from them.
     */
    mp = sm->maps;
    CVMincrementalSignatureToStackMapEntry(vMethod, &(oldMap->v));
    oldMap->sp = 0;
    /*
     * oldPc is the origin point of oldMap. Since
     * CVMincrementalInterpret changes the map and updates its pc,
     * oldPc has to be kept outside oldMap so we know when
     * "too far away" is.
     */
    oldPc = 0;

    for (i = 0; i < nMaps; i++){
	int nMapChunks;
	/* CVMBool isReturn = CVMbcAttr(code[mp->pc], RETURN); */
	nMapChunks = verifierToGC(mp, currEntry, nLocals, CVM_FALSE/*isReturn*/);
	/* ARBITRARY BYTECODE DISTANCE OF 20 HERE */
	if ((currEntry->v.pc - oldPc < 20) &&
	    CVMincrementalInterpret(vMethod, oldMap, currEntry->v.pc) &&
	    CVMincrementalMapsEqual(oldMap, currEntry, nMapChunks))
	{
	    /* there is not really any need for this map so skip it */
	}else{
	    /* will keep this map. Also copy it to oldMap so it can be
	     * used as the basis of incremental computing for the next one
	     */
	    if (nMapChunks != 1){
		currEntry->v.state[0] |= 1; /* flag for special treatment */
	    }
	    CVMincrementalCopyMap(oldMap, currEntry, nMapChunks);
	    oldPc = currEntry->v.pc;
	    currEntry = (CVMPrecomputedStackMapEntry*)
			    &(currEntry->v.state[nMapChunks]);
	    nGcMaps += 1;
	}

	mp = (CVMsplitVerifyMap*)((char*)mp + mapSize);
    }
    CVMassert(currEntry <= end);
    if (nGcMaps == 0){
	/* got rid of them all! */
#ifdef CVM_DEBUG
	/*DEBUG*/ CVMconsolePrintf("Eliminated ALL %d precomputed GC maps in %C.%M\n", nMaps, cntxt->classBeingVerified, vMethod);
#endif
	free(smp);
	smp = (CVMPrecomputedStackMaps*)1;
    }else{
#ifdef CVM_DEBUG
	/*DEBUG*/ if (nGcMaps<nMaps) CVMconsolePrintf("Eliminated %d precomputed GC maps in %C.%M leaving %d\n", (nMaps-nGcMaps), cntxt->classBeingVerified, vMethod, nGcMaps);
	/* DEBUG */ precomputedMemoryAllocated += (char*)currEntry - (char*)smp;
#endif
	if (currEntry < end){
	    smp = realloc(smp, (char*)currEntry - (char*)smp);
	}
	smp->noMaps = nGcMaps;
	smp->noGcPoints = cntxt->noGcPoints + 1; /* account for offset == 0 */
#ifdef CVM_DEBUG
	/* DEBUG */ CVMconsolePrintf("precomputedMemoryAllocated = %d\n",
		    precomputedMemoryAllocated);
#endif
    }
    if (oldMapAllocated){
	free(oldMap);
    }
    return smp;
}

#endif

/*
 * NOTICE:
 * This file is considered "Shared Part" under the CLDC SCSL
 * and commercial licensing terms, and is therefore not to
 * be modified by licensees.  For definition of the term
 * "Shared Part", please refer to the CLDC SCSL license
 * attachment E, Section 2.1 (Definitions) paragraph "e",
 * or your commercial licensing terms as applicable.
 */

/*=========================================================================
 * KVM
 *=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Class file verifier (runtime part)
 * FILE:      verifier.c
 * OVERVIEW:  KVM has a two-phase class file verifier.  In order to
 *            run in the KVM, class files must first be processed with
 *            a special "pre-verifier" tool. This phase is typically
 *            done on the development workstation.  During execution,
 *            the runtime verifier (defined in this file) of the KVM
 *            performs the actual class file verification based on
 *            both runtime information and pre-verification information.
 *=======================================================================*/



/*=========================================================================
 * FUNCTION:      Vfy_getOpcode
 * TYPE:          private operation
 * OVERVIEW:      Get an opcode from the bytecode array.
 *                If it is a BREAKPOINT then get
 *                the real instruction from the debugger
 *
 * INTERFACE:
 *   parameters:  IPINDEX of the bytecode
 *   returns:     The opcode
 *=======================================================================*/

static int
Vfy_getOpcode(VfyContext* cntxt, IPINDEX ip) {
    int opcode = Vfy_getUByte(cntxt, ip);
#if CVM_JVMTI
    if (opcode == opc_breakpoint) {
	CVMExecEnv* ee = cntxt->ee;
	CVMD_gcSafeExec(ee, {
            opcode = CVMjvmtiGetBreakpointOpcode(ee, (CVMUint8*)ip, CVM_TRUE);
	});
    }
#else
    CVMassert(opcode != opc_breakpoint);
#endif
    return opcode;
}

/*=========================================================================
 * FUNCTION:      Vfy_saveStackState
 * TYPE:          private operation
 * OVERVIEW:      Save and initialize the type stack
 *
 * INTERFACE:
 *   parameters:  None
 *   returns:     Nothing
 *=======================================================================*/

static void
Vfy_saveStackState(VfyContext* cntxt) {
    cntxt->vSP_bak = cntxt->vSP;
    cntxt->vStack0_bak = cntxt->vStack[0];
    cntxt->vSP = 0;
}

/*=========================================================================
 * FUNCTION:      Vfy_restoreStackState
 * TYPE:          private operation
 * OVERVIEW:      Restore the saved type stack
 *
 * INTERFACE:
 *   parameters:  None
 *   returns:     Nothing
 *=======================================================================*/

static void
Vfy_restoreStackState(VfyContext* cntxt) {
    cntxt->vStack[0] = cntxt->vStack0_bak;
    cntxt->vSP = cntxt->vSP_bak;
}

/*=========================================================================
 * FUNCTION:      Vfy_getLocal
 * TYPE:          private operation on the virtual local frame
 * OVERVIEW:      Get a type key from the virtual local frame maintained by
 *                the verifier. Performs index check and type check.
 *
 * INTERFACE:
 *   parameters:  index: the local index
 *                typeKey: the type expected.
 *   returns:     The actual type from the slot
 *=======================================================================*/

static
VERIFIERTYPE Vfy_getLocal(
    VfyContext* cntxt, SLOTINDEX index, VERIFIERTYPE typeKey)
{
    VERIFIERTYPE k;
    if (index >= cntxt->vFrameSize) {
        Vfy_throw(cntxt, VE_LOCALS_OVERFLOW);
    }
    k = cntxt->vLocals[index];
    if (!Vfy_isAssignable(k, typeKey)) {
        Vfy_throw(cntxt, VE_LOCALS_BAD_TYPE);
    }
    return k;
}

/*=========================================================================
 * FUNCTION:      Vfy_setLocal
 * TYPE:          private operation on the virtual local frame
 * OVERVIEW:      Set a type key in the virtual local frame maintained by
 *                the verifier. Performs index check and type check.
 *
 * INTERFACE:
 *   parameters:  index: local index.
 *                typeKey: the supplied type.
 *   returns:     Nothing
 *=======================================================================*/

void Vfy_setLocal(VfyContext* cntxt, SLOTINDEX index, VERIFIERTYPE typeKey) {
    if (index >= cntxt->vFrameSize) {
        Vfy_throw(cntxt, VE_LOCALS_OVERFLOW);
    }

    if (cntxt->vLocals[index] == ITEM_Long_2
        || cntxt->vLocals[index] == ITEM_Double_2
        ) {
        if (index < 1) {
            Vfy_throw(cntxt, VE_LOCALS_UNDERFLOW);
        }
        cntxt->vLocals[index - 1] = ITEM_Bogus;
    }

    if (cntxt->vLocals[index] == ITEM_Long
        || cntxt->vLocals[index] == ITEM_Double
        ) {
        if (index >= cntxt->vFrameSize - 1) {
            Vfy_throw(cntxt, VE_LOCALS_OVERFLOW);
        }
        cntxt->vLocals[index + 1] = ITEM_Bogus;
    }
    cntxt->vLocals[index] = typeKey;
}

/*=========================================================================
 * FUNCTION:      vPushStack
 * TYPE:          private operation on the virtual stack
 * OVERVIEW:      Push a type key onto the virtual stack maintained by
 *                the verifier. Performs stack overflow check.
 *
 * INTERFACE:
 *   parameters:  typeKey: the type to be pushed.
 *   returns:     Nothing
 *=======================================================================*/

void Vfy_push(VfyContext* cntxt, VERIFIERTYPE typeKey) {
    if (cntxt->vSP >= cntxt->vMaxStack) {
        Vfy_throw(cntxt, VE_STACK_OVERFLOW);
    }
    cntxt->vStack[cntxt->vSP++] = typeKey;
}

/*=========================================================================
 * FUNCTION:      Vfy_pop
 * TYPE:          private operation on the virtual stack
 * OVERVIEW:      Pop an item from the virtual stack maintained by
 *                the verifier. Performs stack underflow check and type check.
 *
 * INTERFACE:
 *   parameters:  typeKey: The expected type
 *   returns:     The actual type popped
 *=======================================================================*/
#ifdef CVM_DEBUG_ASSERTS
/* indexed by enum ITEM_Tag */
CVMBool stackTypeOk[] = {
	CVM_FALSE,	/* ITEM_Bogus */
	CVM_FALSE,	/* ITEM_Void */
	CVM_TRUE,	/* ITEM_Integer */
	CVM_TRUE,	/* ITEM_Float */
	CVM_TRUE,	/* ITEM_Double */
	CVM_TRUE,	/* ITEM_Double_2 */
	CVM_TRUE,	/* ITEM_Long */
	CVM_TRUE,	/* ITEM_Long_2 */
	CVM_FALSE,	/* ITEM_Array */
	CVM_TRUE,	/* ITEM_Object */
	CVM_TRUE,	/* ITEM_NewObject */
	CVM_TRUE,	/* ITEM_InitObject */
	CVM_FALSE,	/* ITEM_ReturnAddress */
	CVM_FALSE,	/* ITEM_Byte */
	CVM_FALSE,	/* ITEM_Short */
	CVM_FALSE,	/* ITEM_Char */
	CVM_TRUE,	/* ITEM_Reference */
};
#endif

VERIFIERTYPE Vfy_pop(VfyContext* cntxt, VERIFIERTYPE typeKey) {
    VERIFIERTYPE resultKey;

#ifdef CVM_DEBUG_ASSERTS
    int typetype = GET_ITEM_TYPE(typeKey);
    CVMassert((typetype > 0) && 
	(typetype < (sizeof(stackTypeOk)/sizeof(CVMBool))) &&
        (stackTypeOk[typetype] || GET_INDIRECTION(typeKey) > 0));
#endif

    if (cntxt->vSP == 0) { /* vSP is unsigned, See bug 4323211 */
        Vfy_throw(cntxt, VE_STACK_UNDERFLOW);
    }

    resultKey = cntxt->vStack[cntxt->vSP - 1];
    cntxt->vSP--;

    if (!Vfy_isAssignable(resultKey, typeKey)) {
        Vfy_throw(cntxt, VE_STACK_BAD_TYPE);
    }
    return resultKey;
}

/*=========================================================================
 * FUNCTION:      Vfy_popCategory2_secondWord
 * TYPE:          private operation on the virtual stack
 * OVERVIEW:      Pop an the second word of an ITEM_DoubleWord or ITEM_Category2
 *                from the virtual stack maintained by the verifier.
 *                Performs stack underflow check and type check.
 *                (This is always called before vPopStackCategory2_firstWord)
 *
 * INTERFACE:
 *   parameters:  None.
 *   returns:     The actual type popped
 *=======================================================================*/

static VERIFIERTYPE
Vfy_popCategory2_secondWord(VfyContext* cntxt) {
    VERIFIERTYPE resultKey;
    if (cntxt->vSP <= 1) {
        Vfy_throw(cntxt, VE_STACK_UNDERFLOW);
    }

    resultKey = cntxt->vStack[cntxt->vSP - 1];
    cntxt->vSP--;

    return resultKey;
}

/*=========================================================================
 * FUNCTION:      Vfy_popCategory2_firstWord
 * TYPE:          private operation on the virtual stack
 * OVERVIEW:      Pop an the first word of an ITEM_DoubleWord or ITEM_Category2
 *                from the virtual stack maintained by the verifier.
 *                Performs stack underflow check and type check.
 *
 * INTERFACE:
 *   parameters:  None.
 *   returns:     The actual type popped
 *=======================================================================*/

static VERIFIERTYPE
Vfy_popCategory2_firstWord(VfyContext* cntxt) {
    VERIFIERTYPE resultKey;
    if (cntxt->vSP <= 0) {
        Vfy_throw(cntxt, VE_STACK_UNDERFLOW);
    }

    resultKey = cntxt->vStack[cntxt->vSP - 1];
    cntxt->vSP--;

   /*
    * The only think known about this operation is that it
    * cannot result in an ITEM_Long_2 or ITEM_Double_2 being
    * popped.
    */

    if ((resultKey == ITEM_Long_2) || (resultKey == ITEM_Double_2)) {
        Vfy_throw(cntxt, VE_STACK_BAD_TYPE);
    }

    return resultKey;
}

/*=========================================================================
 * FUNCTION:      Vfy_popCategory1
 * TYPE:          private operation on the virtual stack
 * OVERVIEW:      Pop a ITEM_Category1 from the virtual stack maintained by
 *                the verifier. Performs stack underflow check and type check.
 *
 * INTERFACE:
 *   parameters:  None.
 *   returns:     The actual type popped
 *=======================================================================*/

static VERIFIERTYPE
Vfy_popCategory1(VfyContext* cntxt) {
    VERIFIERTYPE resultKey;
    if (cntxt->vSP == 0) { /* vSP is unsigned, See bug 4323211 */
        Vfy_throw(cntxt, VE_STACK_UNDERFLOW);
    }
    resultKey = cntxt->vStack[cntxt->vSP - 1];
    cntxt->vSP--;

    switch(GET_ITEM_TYPE(resultKey)){
    case ITEM_Integer:
    case ITEM_Float:
    /* case ITEM_Null: This isn't an item, its an ITEM_Object */
    case ITEM_Object:
    case ITEM_InitObject:
    case ITEM_NewObject:
	break; /* its ok */
    default:
	if (GET_INDIRECTION(resultKey) > 0)
	    break; /* ok */
        Vfy_throw(cntxt, VE_STACK_EXPECT_CAT1);
    }
    return resultKey;
}


/*=========================================================================
 * FUNCTION:      Vfy_pushClassKey
 * TYPE:          private operation on type keys
 * OVERVIEW:      Push the equivalent VERIFIERTYPES for CVMClassTypeID
 *
 * INTERFACE:
 *   parameters:  fieldType: CVMClassTypeID
 *   returns:     Nothing
 *=======================================================================*/

static void
Vfy_pushClassKey(VfyContext* cntxt, CVMClassTypeID fieldType) {
    switch (fieldType) {
        case CVM_TYPEID_INT:
        case CVM_TYPEID_BYTE:
        case CVM_TYPEID_BOOLEAN:
        case CVM_TYPEID_CHAR:
        case CVM_TYPEID_SHORT: {
            Vfy_push(cntxt, ITEM_Integer);
            break;
        }
        case CVM_TYPEID_FLOAT: {
            Vfy_push(cntxt, ITEM_Float);
            break;
        }
        case CVM_TYPEID_DOUBLE: {
            Vfy_push(cntxt, ITEM_Double);
            Vfy_push(cntxt, ITEM_Double_2);
            break;
        }
        case CVM_TYPEID_LONG: {
            Vfy_push(cntxt, ITEM_Long);
            Vfy_push(cntxt, ITEM_Long_2);
            break;
        }
        default: {
            Vfy_push(cntxt, Vfy_toVerifierType(fieldType));
            break;
        }
    }
}

/*=========================================================================
 * FUNCTION:      Vfy_popClassKey
 * TYPE:          private operation on type keys
 * OVERVIEW:      Pop the equivalent VERIFIERTYPES for CVMClassTypeID
 *
 * INTERFACE:
 *   parameters:  fieldType: CVMClassTypeID
 *   returns:     Nothing
 *=======================================================================*/

static void
Vfy_popClassKey(VfyContext* cntxt, CVMClassTypeID fieldType) {
    switch (fieldType) {
        case CVM_TYPEID_INT:
        case CVM_TYPEID_BYTE:
        case CVM_TYPEID_BOOLEAN:
        case CVM_TYPEID_CHAR:
        case CVM_TYPEID_SHORT: {
            Vfy_pop(cntxt, ITEM_Integer);
            break;
        }
        case CVM_TYPEID_FLOAT: {
            Vfy_pop(cntxt, ITEM_Float);
            break;
        }
        case CVM_TYPEID_DOUBLE: {
            Vfy_pop(cntxt, ITEM_Double_2);
            Vfy_pop(cntxt, ITEM_Double);
            break;
        }
        case CVM_TYPEID_LONG: {
            Vfy_pop(cntxt, ITEM_Long_2);
            Vfy_pop(cntxt, ITEM_Long);
            break;
        }
        default: {
            Vfy_pop(cntxt, Vfy_toVerifierType(fieldType));
            break;
        }
    }
}

/*=========================================================================
 * FUNCTION:      Mth_getStackMapEntryIP
 * TYPE:          private operation
 * OVERVIEW:      Get the ip address of an entry in the stack map table
 *
 * INTERFACE:
 *   parameters:  thisMethod: the method, stackMapIndex: the index
 *   returns:     The ip address of the indexed entry. Or 0x7FFFFFFF if
 *                the index is off the end of the table.
 *		  Also caches the entry and its ip in the context. See
 *		  getStackMap.
 *=======================================================================*/

static int
Mth_getStackMapEntryIP(VfyContext* cntxt, METHOD thisMethod, int stackMapIndex) {
    CVMsplitVerifyStackMaps* stackMaps = cntxt->maps;
    if (stackMaps && stackMapIndex < stackMaps->nEntries) {
	CVMsplitVerifyMap*  map;
	map = CVM_SPLIT_VERIFY_MAPS_INDEX(stackMaps,stackMapIndex);
	cntxt->cachedMap = map;
	cntxt->cachedMapIP = map->pc;
	return map->pc;
    } else {
	cntxt->cachedMap = NULL;
	cntxt->cachedMapIP = 0x7FFFFFFF;
        return 0x7FFFFFFF;
    }
}

/*=========================================================================
 * The main verification Function
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:      Vfy_verifyMethodOrAbort
 * TYPE:          private operation on methods.
 * OVERVIEW:      Perform byte-code verification of a given method.
 *
 * INTERFACE:
 *   parameters:  vMethod: method to be verified.
 *   returns:     Normally if verification succeeds, otherwise longjmp
 *                is called (in Vfy_throw) to return control to a handler
 *                in the calling code.
 *=======================================================================*/

static void
Vfy_verifyMethodOrAbort(VfyContext* cntxt, const METHOD vMethod) {

   /*
    * The following variables are constant in this function
    */
    const CLASS vClass       = cntxt->classBeingVerified;
    const CLASS vSuperClass  = Cls_getSuper(vClass);
    const int codeLength     = Mth_getBytecodeLength(vMethod);
    const int handlerCount   = Mth_getExceptionTableLength(vMethod);
    const CVMConstantPool* vPool = Cls_getPool(vClass);
    IPINDEX nextStackMapIP;

   /*
    * The virtual IP used by the verifier
    */
    IPINDEX ip = 0;
    IPINDEX last_ip = ip;

   /*
    * The following is set to CVM_TRUE where is no direct control
    * flow from current instruction to the next instruction
    * in sequence.
    */
    CVMBool noControlFlow = CVM_FALSE;

   /*
    * Pointer to the "current" stackmap entry
    */
    int currentStackMapIndex = 0;

   /*
    * Check that a virtual method does not subclass a "final"
    * method higher up the inheritance chain
    *
    * Was bug 4336036
    */
    if (!Mth_isStatic(vMethod)) {

       /*
        * Don't check methods in java.lang.Object.
        */
        if (!Cls_isJavaLangObject(vClass)) {

           /*
            * Lookup the method being verified in the superclass
            */
            METHOD superMethod = Cls_lookupMethod(vClass, vSuperClass, vMethod);

           /*
            * If it exists then it must not be final
            */
            if (superMethod != NULL && Mth_isFinal(superMethod)) {
                Vfy_throw(cntxt, VE_FINAL_METHOD_OVERRIDE);
            }
        }
    }

   /*
    * Verify that all exception handlers have a reasonable exception type.
    */
    if (handlerCount > 0) {
        int i;
        VERIFIERTYPE exceptionVerifierType;

       /*
        * Iterate through the handler table
        */
        for (i = 0; i < handlerCount; i++) {

           /*
            * Get the catch type from the exception table
            */
            POOLINDEX catchTypeIndex = Mth_getExceptionTableCatchType(vMethod, i);

           /*
            * If the catch type index is zero then this is a try/finally entry
            * and there is no  exception type, If it is not zero then it needs
            * to be checked.
            */
            if (catchTypeIndex != 0) {

               /*
                * Check that the entry is there and that it is a CONSTANT_Class
                */
                Pol_checkTagIsClass(cntxt, vPool, catchTypeIndex);

               /*
                * Get the class key
                */
                exceptionVerifierType = Vfy_toVerifierType(
				    Pol_getClassKey(vPool, catchTypeIndex));

               /*
                * Check that it is subclass of java.lang.Throwable
                */
                if (!Vfy_isAssignable(exceptionVerifierType, 
			Vfy_getThrowableVerifierType()))
		{
                    Vfy_throw(cntxt, VE_EXPECT_THROWABLE);
                }
            }
        }
    }

   /*
    * Initialize the local variable type table with the method argument types
    */
    Vfy_initializeLocals(cntxt);

    /*
     * Instruction index of the next verifier map
     */
    nextStackMapIP = Mth_getStackMapEntryIP(cntxt, vMethod, 
					    currentStackMapIndex);

   /*
    * The main loop
    */
    while (ip < codeLength) {
       /*
        * Keep track of whether or not this instruction is in
	* a try block.
	*/
	CVMBool inTryBlock = CVM_FALSE;
       /*
        * Used to hold the current opcode
        */
        int opcode;

       /*
        * Setup the ip used for error messages in case it is needed later
        */
        Vfy_setErrorIp(cntxt, ip);

       /*
        * Output the debug trace message
        */
        Vfy_printVerifyLoopMessage(cntxt, vMethod, ip);

       /*
        * Check that stackmaps are ordered according to offset and that
        * every offset in stackmaps point to the beginning to an instruction.
        */
        if (nextStackMapIP < ip) {
	    /*
	     * The ip got advanced past the ip in the next stackmap entry.
	     * This means that the stackmap entry pointed into the middle
	     * of the previous instruction. This is bad.
	     */
            Vfy_throw(cntxt, VE_BAD_STACKMAP);    
        }

       /*
        * Trace the instruction ip
        */
        Vfy_trace1("Check instruction %ld\n", ip);

       /*
        * Merge with the next instruction (note how noControlFlow is used here).
        */
        if (nextStackMapIP == ip) {
	    /*
	     * There is a stackmap here.
	     * It might be more efficient to avoid the getStackMap call
	     * from matchStackMap, since we already know the map index
	     * is currentStackMapIndex. If this starts to look like
	     * a bottleneck, we shall.
	     */
            currentStackMapIndex++;         /* This offset is good. */
	    nextStackMapIP = Mth_getStackMapEntryIP(cntxt, 
	    			vMethod, currentStackMapIndex);
	    /* Also, this macro can be simplified since we are pretty
	     * certain it won't fail due to not finding a map.
	     */
	    Vfy_checkCurrentTarget(cntxt, ip, noControlFlow);
	} else if (noControlFlow){
	    /*
	     * There is no stackmap nor is there fall-thru from the
	     * instruction above. Flag this as an error
	     */
	    Vfy_throw(cntxt, VE_SEQ_BAD_TYPE);
	}

       /*
        * Set noControlFlow to its default state
        */
        noControlFlow = CVM_FALSE;

       /*
        * Look for a possible jump target in one or more exception handlers.
        * If found the current local variable types must be the same as those
        * of the handler entry point.
        */
        if (handlerCount > 0) {
            int i;

           /*
            * Iterate through the handler table
            */
            for (i = 0 ; i < handlerCount ; i++) {
                IPINDEX startPC = Mth_getExceptionTableStartPC(vMethod, i);
                IPINDEX endPC   = Mth_getExceptionTableEndPC(vMethod, i);

               /*
                * Check to see if the "current" ip falls in the
		* range of this exception.
                */
		if (ip >= startPC && ip < endPC) {
                   VERIFIERTYPE exceptionVerifierType;

                   /*
                    * Get the ip offset for the exception handler 
		    * and the catch type index
                    */
		    IPINDEX handlerPC        
		    		= Mth_getExceptionTableHandlerPC(vMethod, i);
                    POOLINDEX catchTypeIndex 
		    		= Mth_getExceptionTableCatchType(vMethod, i);

                   /*
                    * If the catch type index is zero then this is a 
		    * try/finally entry.
                    * Unlike J2SE (1.3) there are no jsr/ret instructions in 
		    * J2SE/CLDC. The code in a finally block will be copied to 
		    * the end of the try block and an anonymous handler entry 
		    * is make that points to another
                    * copy of the code. This will only be called if 
		    * an exception is thrown in which case there will be an 
		    * exception object of some kind on the
                    * stack. On the other hand if catch type index is 
		    * non-zero then it will indicate a specific throwable type.
                    */
		    if (catchTypeIndex != 0) {
			exceptionVerifierType = Vfy_toVerifierType(
				       Pol_getClassKey(vPool, catchTypeIndex));
                    } else {
                        exceptionVerifierType = Vfy_getThrowableVerifierType();
                    }

                   /*
                    * Save the current stack types somewhere and 
		    * re-initialize the stack.
                    */
                    Vfy_saveStackState(cntxt);

                   /*
                    * Push the exception type and check the target 
		    * has just this one type
                    * on the stack along with the current local variable types.
                    */
                    Vfy_push(cntxt, exceptionVerifierType);
                    Vfy_checkHandlerTarget(cntxt, handlerPC);

                   /*
                    * Restore the old stack types
                    */
                    Vfy_restoreStackState(cntxt);
		    /*
		     * remember that we found a ip in a try block.
		     */
		    inTryBlock = CVM_TRUE;
                }
		/* Make sure that there is no exception handler whose 
		 * start or end is between last_ip and ip, because that's
		 * not allowed.
		 */
		if (ip > last_ip + 1){
		    /* only a concern if prev instruction was multi-byte.
		     * Notice how the test here is sufficiently
		     * different from the simple "in tryblock"
		     * test above because of a lot of possible deviant
		     * behavior: e.g. forming a try block completely
		     * within the previous instruction.
		     */
		    if ((startPC > last_ip && startPC < ip)
			|| (endPC > last_ip && endPC < ip))
		    {
			Vfy_throw(cntxt, VE_BAD_EXCEPTION_HANDLER_RANGE); 
		    }
		}
	    }
        }
       /*
	* Track the ip of the start of this instruction.
	*/
	last_ip = ip;

       /*
        * Get the next bytecode
        */
	{
	    unsigned char *i = cntxt->bytecodesBeingVerified + ip;
	    unsigned char *e = cntxt->bytecodesBeingVerified + codeLength;
	    int len = CVMopcodeGetLengthWithBoundsCheck(i, e);
	    if (len < 0 || ip + len > codeLength) {
		Vfy_throw(cntxt, VE_BAD_INSTR); 
	    }
	}
        opcode = Vfy_getOpcode(cntxt, ip);
	if (CVMbcAttr(opcode, BRANCH) || CVMbcAttr(opcode, GCPOINT)){
	    cntxt->noGcPoints++;
	}
        switch (opcode) {

            case NOP: {
                ip++;
                break;

            }

            case ACONST_NULL: {
                Vfy_push(cntxt, ITEM_Null);
                ip++;
                break;

            }

            case ICONST_M1:
            case ICONST_0:
            case ICONST_1:
            case ICONST_2:
            case ICONST_3:
            case ICONST_4:
            case ICONST_5: {
                Vfy_push(cntxt, ITEM_Integer);
                ip++;
                break;

            }

            case LCONST_0:
            case LCONST_1: {
                Vfy_push(cntxt, ITEM_Long);
                Vfy_push(cntxt, ITEM_Long_2);
                ip++;
                break;

            }


            case FCONST_0:
            case FCONST_1:
            case FCONST_2: {
                Vfy_push(cntxt, ITEM_Float);
                ip++;
                break;

            }

            case DCONST_0:
            case DCONST_1: {
                Vfy_push(cntxt, ITEM_Double);
                Vfy_push(cntxt, ITEM_Double_2);
                ip++;
                break;

            }


            case BIPUSH: {
                Vfy_push(cntxt, ITEM_Integer);
                ip += 2;
                break;
            }

            case SIPUSH: {
                ip++;
                Vfy_push(cntxt, ITEM_Integer);
                ip += 2;
                break;

            }

            case LDC:
            case LDC_W:
            case LDC2_W: {
                POOLTAG tag;
                POOLINDEX index;

               /*
                * Get the constant pool index and advance the 
		* ip to the next instruction
                */
                if (opcode == LDC) {                 /* LDC */
                    index = Vfy_getUByte(cntxt, ip + 1);
                    ip += 2;
                } else {                             /* LDC_W or LDC2_W */
                    index = Vfy_getUShort(cntxt, ip + 1);
                    ip += 3;
                }

               /*
                * Get the tag
                */
                tag = Pol_getTag(cntxt, vPool, index);

               /*
                * Check for the right kind of LDC and push the required type
                */

                if (opcode == LDC2_W) {              /* LDC2_W */
                    if (tag == CONSTANT_Long) {
                        Vfy_push(cntxt, ITEM_Long);
                        Vfy_push(cntxt, ITEM_Long_2);
                        break;
                    }
                    if (tag == CONSTANT_Double) {
                        Vfy_push(cntxt, ITEM_Double);
                        Vfy_push(cntxt, ITEM_Double_2);
                        break;
                    }
                } else {                             /* LDC or LDC_W */
                    if (tag == CONSTANT_String) {
                        Vfy_push(cntxt, Vfy_getStringVerifierType());
                        break;
                    }

                    if (tag == CONSTANT_Integer) {
                        Vfy_push(cntxt, ITEM_Integer);
                        break;
                    }
                    if (tag == CONSTANT_Float) {
                        Vfy_push(cntxt, ITEM_Float);
                        break;
                    }

		    if (tag == CONSTANT_Class) {
			if (cntxt->classBeingVerified->major_version >= 49) {
			    CVMClassTypeID typeKey =
				Vfy_getSystemVfyType(java_lang_Class);
			    Vfy_push(cntxt, typeKey);
			    break;
			}
		    }

                }
                Vfy_throw(cntxt, VE_BAD_LDC);
            }

            case ILOAD_0:
            case ILOAD_1:
            case ILOAD_2:
            case ILOAD_3:
            case ILOAD: {
                SLOTINDEX index;
                if (opcode == ILOAD) {
                    index = Vfy_getUByte(cntxt, ip + 1);
                    ip += 2;
                } else {
                    index = opcode - ILOAD_0;
                    ip++;
                }
                Vfy_getLocal(cntxt, index, ITEM_Integer);
                Vfy_push(cntxt, ITEM_Integer);
                break;
            }

            case LLOAD_0:
            case LLOAD_1:
            case LLOAD_2:
            case LLOAD_3:
            case LLOAD: {
                SLOTINDEX index;
                if (opcode == LLOAD) {
                    index = Vfy_getUByte(cntxt, ip + 1);
                    ip += 2;
                } else {
                    index = opcode - LLOAD_0;
                    ip++;
                }
                Vfy_getLocal(cntxt, index,     ITEM_Long);
                Vfy_getLocal(cntxt, index + 1, ITEM_Long_2);
                Vfy_push(cntxt, ITEM_Long);
                Vfy_push(cntxt, ITEM_Long_2);
                break;

            }


            case FLOAD_0:
            case FLOAD_1:
            case FLOAD_2:
            case FLOAD_3:
            case FLOAD: {
                SLOTINDEX index;
                if (opcode == FLOAD) {
                    index = Vfy_getUByte(cntxt, ip + 1);
                    ip += 2;
                } else {
                    index = opcode - FLOAD_0;
                    ip++;
                }
                Vfy_getLocal(cntxt, index, ITEM_Float);
                Vfy_push(cntxt, ITEM_Float);
                break;
            }

            case DLOAD_0:
            case DLOAD_1:
            case DLOAD_2:
            case DLOAD_3:
            case DLOAD: {
                SLOTINDEX index;
                if (opcode == DLOAD) {
                    index = Vfy_getUByte(cntxt, ip + 1);
                    ip += 2;
                } else {
                    index = opcode - DLOAD_0;
                    ip++;
                }
                Vfy_getLocal(cntxt, index, ITEM_Double);
                Vfy_getLocal(cntxt, index + 1, ITEM_Double_2);
                Vfy_push(cntxt, ITEM_Double);
                Vfy_push(cntxt, ITEM_Double_2);
                break;

            }


            case ALOAD_0:
            case ALOAD_1:
            case ALOAD_2:
            case ALOAD_3:
            case ALOAD: {
                SLOTINDEX    index;
                VERIFIERTYPE refType;
                if (opcode == ALOAD) {
                    index = Vfy_getUByte(cntxt, ip + 1);
                    ip += 2;
                } else {
                    index = opcode - ALOAD_0;
                    ip++;
                }
                refType = Vfy_getLocal(cntxt, index, ITEM_Reference);
                Vfy_push(cntxt, refType);
                break;

            }

            case IALOAD: {
                Vfy_pop(cntxt, ITEM_Integer);
                Vfy_pop(cntxt, Vfy_getIntArrayVerifierType());
                Vfy_push(cntxt, ITEM_Integer);
                ip++;
                break;

            }

            case BALOAD: {
                VERIFIERTYPE dummy     = Vfy_pop(cntxt, ITEM_Integer);
                VERIFIERTYPE arrayType = Vfy_pop(cntxt, Vfy_getObjectVerifierType());
                if (arrayType != Vfy_getByteArrayVerifierType() &&
                    arrayType != Vfy_getBooleanArrayVerifierType() &&
                    arrayType != ITEM_Null) {
                    Vfy_throw(cntxt, VE_BALOAD_BAD_TYPE);
                }
                Vfy_push(cntxt, ITEM_Integer);
                ip++;
                (void)dummy;
                break;
            }

            case CALOAD: {
                Vfy_pop(cntxt, ITEM_Integer);
                Vfy_pop(cntxt, Vfy_getCharArrayVerifierType());
                Vfy_push(cntxt, ITEM_Integer);
                ip++;
                break;

            }

            case SALOAD: {
                Vfy_pop(cntxt, ITEM_Integer);
                Vfy_pop(cntxt, Vfy_getShortArrayVerifierType());
                Vfy_push(cntxt, ITEM_Integer);
                ip++;
                break;

            }

            case LALOAD: {
                Vfy_pop(cntxt, ITEM_Integer);
                Vfy_pop(cntxt, Vfy_getLongArrayVerifierType());
                Vfy_push(cntxt, ITEM_Long);
                Vfy_push(cntxt, ITEM_Long_2);
                ip++;
                break;
            }


            case FALOAD: {
                Vfy_pop(cntxt, ITEM_Integer);
                Vfy_pop(cntxt, Vfy_getFloatArrayVerifierType());
                Vfy_push(cntxt, ITEM_Float);
                ip++;
                break;

            }

            case DALOAD: {
                Vfy_pop(cntxt, ITEM_Integer);
                Vfy_pop(cntxt, Vfy_getDoubleArrayVerifierType());
                Vfy_push(cntxt, ITEM_Double);
                Vfy_push(cntxt, ITEM_Double_2);
                ip++;
                break;

            }


            case AALOAD: {

                VERIFIERTYPE dummy     = Vfy_pop(cntxt, ITEM_Integer);
                VERIFIERTYPE arrayType = Vfy_pop(cntxt, Vfy_getObjectVerifierType());
                VERIFIERTYPE arrayElementType;
                if (!Vfy_isAssignable(arrayType, Vfy_getObjectArrayVerifierType())) {
                    Vfy_throw(cntxt, VE_AALOAD_BAD_TYPE);
                }

/* Alternative implementation:
                VERIFIERTYPE dummy     = Vfy_pop(ITEM_Integer);
                VERIFIERTYPE arrayType = Vfy_pop(Vfy_getObjectArrayVerifierType());
                VERIFIERTYPE arrayElementType;
*/

                arrayElementType = Vfy_getReferenceArrayElementType(arrayType);
                Vfy_push(cntxt, arrayElementType);
                ip++;
                (void)dummy;
                break;
            }

            case ISTORE_0:
            case ISTORE_1:
            case ISTORE_2:
            case ISTORE_3:
            case ISTORE: {
                SLOTINDEX index;
                if (opcode == ISTORE) {
                    index = Vfy_getUByte(cntxt, ip + 1);
                    ip += 2;
                } else {
                    index = opcode - ISTORE_0;
                    ip++;
                }
                Vfy_pop(cntxt, ITEM_Integer);
                Vfy_setLocal(cntxt, index, ITEM_Integer);
                break;

            }

            case LSTORE_0:
            case LSTORE_1:
            case LSTORE_2:
            case LSTORE_3:
            case LSTORE: {
                SLOTINDEX index;
                if (opcode == LSTORE) {
                    index = Vfy_getUByte(cntxt, ip + 1);
                    ip += 2;
                } else {
                    index = opcode - LSTORE_0;
                    ip++;
                }
                Vfy_pop(cntxt, ITEM_Long_2);
                Vfy_pop(cntxt, ITEM_Long);
                Vfy_setLocal(cntxt, index + 1, ITEM_Long_2);
                Vfy_setLocal(cntxt, index, ITEM_Long);
                break;
            }


            case FSTORE_0:
            case FSTORE_1:
            case FSTORE_2:
            case FSTORE_3:
            case FSTORE: {
                SLOTINDEX index;
                if (opcode == FSTORE) {
                    index = Vfy_getUByte(cntxt, ip + 1);
                    ip += 2;
                } else {
                    index = opcode - FSTORE_0;
                    ip++;
                }
                Vfy_pop(cntxt, ITEM_Float);
                Vfy_setLocal(cntxt, index, ITEM_Float);
                break;
            }

            case DSTORE_0:
            case DSTORE_1:
            case DSTORE_2:
            case DSTORE_3:
            case DSTORE: {
                SLOTINDEX index;
                if (opcode == DSTORE) {
                    index = Vfy_getUByte(cntxt, ip + 1);
                    ip += 2;
                } else {
                    index = opcode - DSTORE_0;
                    ip++;
                }
                Vfy_pop(cntxt, ITEM_Double_2);
                Vfy_pop(cntxt, ITEM_Double);
                Vfy_setLocal(cntxt, index + 1, ITEM_Double_2);
                Vfy_setLocal(cntxt, index, ITEM_Double);
                break;
            }


            case ASTORE_0:
            case ASTORE_1:
            case ASTORE_2:
            case ASTORE_3:
            case ASTORE: {
                SLOTINDEX index;
                VERIFIERTYPE arrayElementType;
                if (opcode == ASTORE) {
                    index = Vfy_getUByte(cntxt, ip + 1);
                    ip += 2;
                } else {
                    index = opcode - ASTORE_0;
                    ip++;
                }
                arrayElementType = Vfy_pop(cntxt, ITEM_Reference);
                Vfy_setLocal(cntxt, index, arrayElementType);
                break;
            }

            case IASTORE: {
                Vfy_pop(cntxt, ITEM_Integer);
                Vfy_pop(cntxt, ITEM_Integer);
                Vfy_pop(cntxt, Vfy_getIntArrayVerifierType());
                ip++;
                break;

            }

            case BASTORE: {
                VERIFIERTYPE dummy1    = Vfy_pop(cntxt, ITEM_Integer);
                VERIFIERTYPE dummy2    = Vfy_pop(cntxt, ITEM_Integer);
                VERIFIERTYPE arrayType = Vfy_pop(cntxt, Vfy_getObjectVerifierType());
                if (arrayType != Vfy_getByteArrayVerifierType() &&
                    arrayType != Vfy_getBooleanArrayVerifierType() &&
                    arrayType != ITEM_Null) {
                    Vfy_throw(cntxt, VE_BASTORE_BAD_TYPE);
                }
                ip++;
                (void)dummy1;
                (void)dummy2;
                break;
            }

            case CASTORE: {
                Vfy_pop(cntxt, ITEM_Integer);
                Vfy_pop(cntxt, ITEM_Integer);
                Vfy_pop(cntxt, Vfy_getCharArrayVerifierType());
                ip++;
                break;

            }

            case SASTORE: {
                Vfy_pop(cntxt, ITEM_Integer);
                Vfy_pop(cntxt, ITEM_Integer);
                Vfy_pop(cntxt, Vfy_getShortArrayVerifierType());
                ip++;
                break;

            }

            case LASTORE: {
                Vfy_pop(cntxt, ITEM_Long_2);
                Vfy_pop(cntxt, ITEM_Long);
                Vfy_pop(cntxt, ITEM_Integer);
                Vfy_pop(cntxt, Vfy_getLongArrayVerifierType());
                ip++;
                break;

            }


            case FASTORE: {
                Vfy_pop(cntxt, ITEM_Float);
                Vfy_pop(cntxt, ITEM_Integer);
                Vfy_pop(cntxt, Vfy_getFloatArrayVerifierType());
                ip++;
                break;

            }

            case DASTORE: {
                Vfy_pop(cntxt, ITEM_Double_2);
                Vfy_pop(cntxt, ITEM_Double);
                Vfy_pop(cntxt, ITEM_Integer);
                Vfy_pop(cntxt, Vfy_getDoubleArrayVerifierType());
                ip++;
                break;

            }


            case AASTORE: {
                VERIFIERTYPE value            = Vfy_pop(cntxt, Vfy_getObjectVerifierType());
                VERIFIERTYPE dummy            = Vfy_pop(cntxt, ITEM_Integer);
                VERIFIERTYPE arrayType        = Vfy_pop(cntxt, Vfy_getObjectArrayVerifierType());
                VERIFIERTYPE arrayElementType;

               /*
                * The value to be stored must be some kind of object and the
                * array must be some kind of reference array.
                */
/* NOTE: It seems to me that Vfy_pop just did both these tests. So what
does this do?
*/
                if (
                    !Vfy_isAssignable(value,     Vfy_getObjectVerifierType()) ||
                    !Vfy_isAssignable(arrayType, Vfy_getObjectArrayVerifierType())
                   ) {
                    Vfy_throw(cntxt, VE_AASTORE_BAD_TYPE);
                }

               /*
                * Get the actual element type of the array
                */
                arrayElementType = Vfy_getReferenceArrayElementType(arrayType);

               /*
                * This part of the verifier is far from obvious, but the logic
                * appears to be as follows:
                *
                * 1, Because not all stores into a reference array can be
                *    statically checked then they never are in the case
                *    where the array is of one dimension and the object
                *    being inserted is a non-array, The verifier will
                *    ignore such errors and they will all be found at runtime.
                *
                * 2, However, if the array is of more than one dimension or
                *    the object being inserted is some kind of an array then
                *    a check is made by the verifier and errors found at
                *    this time (statically) will cause the method to fail
                *    verification. Presumable not all errors will will be found
                *    here and so some runtime errors can occur in this case
                *    as well.
                */
                if (!Vfy_isArray(arrayElementType) && !Vfy_isArray(value)) {
                    /* Success */
                } else if (Vfy_isAssignable(value, arrayElementType)) {
                    /* Success */
                } else {
                    Vfy_throw(cntxt, VE_AASTORE_BAD_TYPE);
                }
                ip++;
                (void)dummy;
                break;
            }

            case POP: {
                Vfy_popCategory1(cntxt);
                ip++;
                break;

            }

            case POP2: {
                Vfy_popCategory2_secondWord(cntxt);
                Vfy_popCategory2_firstWord(cntxt);
                ip++;
                break;

            }

            case DUP: {
                VERIFIERTYPE type = Vfy_popCategory1(cntxt);
                Vfy_push(cntxt, type);
                Vfy_push(cntxt, type);
                ip++;
                break;

            }

            case DUP_X1:{
                VERIFIERTYPE type1 = Vfy_popCategory1(cntxt);
                VERIFIERTYPE type2 = Vfy_popCategory1(cntxt);
                Vfy_push(cntxt, type1);
                Vfy_push(cntxt, type2);
                Vfy_push(cntxt, type1);
                ip++;
                break;
            }

            case DUP_X2: {
                VERIFIERTYPE cat1type = Vfy_popCategory1(cntxt);
                VERIFIERTYPE second   = Vfy_popDoubleWord_secondWord(cntxt);
                VERIFIERTYPE first    = Vfy_popDoubleWord_firstWord(cntxt);
                Vfy_push(cntxt, cat1type);
                Vfy_push(cntxt, first);
                Vfy_push(cntxt, second);
                Vfy_push(cntxt, cat1type);
                ip++;
                break;
            }

            case DUP2: {
                VERIFIERTYPE second = Vfy_popDoubleWord_secondWord(cntxt);
                VERIFIERTYPE first  = Vfy_popDoubleWord_firstWord(cntxt);
                Vfy_push(cntxt, first);
                Vfy_push(cntxt, second);
                Vfy_push(cntxt, first);
                Vfy_push(cntxt, second);
                ip++;
                break;
            }

            case DUP2_X1: {
                VERIFIERTYPE second   = Vfy_popDoubleWord_secondWord(cntxt);
                VERIFIERTYPE first    = Vfy_popDoubleWord_firstWord(cntxt);
                VERIFIERTYPE cat1type = Vfy_popCategory1(cntxt);
                Vfy_push(cntxt, first);
                Vfy_push(cntxt, second);
                Vfy_push(cntxt, cat1type);
                Vfy_push(cntxt, first);
                Vfy_push(cntxt, second);
                ip++;
                break;
            }

            case DUP2_X2: {
                VERIFIERTYPE item1second = Vfy_popDoubleWord_secondWord(cntxt);
                VERIFIERTYPE item1first  = Vfy_popDoubleWord_firstWord(cntxt);
                VERIFIERTYPE item2second = Vfy_popDoubleWord_secondWord(cntxt);
                VERIFIERTYPE item2first  = Vfy_popDoubleWord_firstWord(cntxt);
                Vfy_push(cntxt, item1first);
                Vfy_push(cntxt, item1second);
                Vfy_push(cntxt, item2first);
                Vfy_push(cntxt, item2second);
                Vfy_push(cntxt, item1first);
                Vfy_push(cntxt, item1second);
                ip++;
                break;
            }

            case SWAP: {
                VERIFIERTYPE type1 = Vfy_popCategory1(cntxt);
                VERIFIERTYPE type2 = Vfy_popCategory1(cntxt);
                Vfy_push(cntxt, type1);
                Vfy_push(cntxt, type2);
                ip++;
                break;
            }

            case IADD:
            case ISUB:
            case IMUL:
            case IDIV:
            case IREM:
            case ISHL:
            case ISHR:
            case IUSHR:
            case IOR:
            case IXOR:
            case IAND: {
                Vfy_pop(cntxt, ITEM_Integer);
                Vfy_pop(cntxt, ITEM_Integer);
                Vfy_push(cntxt, ITEM_Integer);
                ip++;
                break;
            }

            case INEG:
                Vfy_pop(cntxt, ITEM_Integer);
                Vfy_push(cntxt, ITEM_Integer);
                ip++;
                break;

            case LADD:
            case LSUB:
            case LMUL:
            case LDIV:
            case LREM:
            case LAND:
            case LOR:
            case LXOR: {
                Vfy_pop(cntxt, ITEM_Long_2);
                Vfy_pop(cntxt, ITEM_Long);
                Vfy_pop(cntxt, ITEM_Long_2);
                Vfy_pop(cntxt, ITEM_Long);
                Vfy_push(cntxt, ITEM_Long);
                Vfy_push(cntxt, ITEM_Long_2);
                ip++;
                break;

            }

            case LNEG: {
                Vfy_pop(cntxt, ITEM_Long_2);
                Vfy_pop(cntxt, ITEM_Long);
                Vfy_push(cntxt, ITEM_Long);
                Vfy_push(cntxt, ITEM_Long_2);
                ip++;
                break;
            }

            case LSHL:
            case LSHR:
            case LUSHR: {
                Vfy_pop(cntxt, ITEM_Integer);
                Vfy_pop(cntxt, ITEM_Long_2);
                Vfy_pop(cntxt, ITEM_Long);
                Vfy_push(cntxt, ITEM_Long);
                Vfy_push(cntxt, ITEM_Long_2);
                ip++;
                break;

            }


            case FADD:
            case FSUB:
            case FMUL:
            case FDIV:
            case FREM: {
                Vfy_pop(cntxt, ITEM_Float);
                Vfy_pop(cntxt, ITEM_Float);
                Vfy_push(cntxt, ITEM_Float);
                ip++;
                break;

            }

            case FNEG: {
                Vfy_pop(cntxt, ITEM_Float);
                Vfy_push(cntxt, ITEM_Float);
                ip++;
                break;
            }

            case DADD:
            case DSUB:
            case DMUL:
            case DDIV:
            case DREM: {
                Vfy_pop(cntxt, ITEM_Double_2);
                Vfy_pop(cntxt, ITEM_Double);
                Vfy_pop(cntxt, ITEM_Double_2);
                Vfy_pop(cntxt, ITEM_Double);
                Vfy_push(cntxt, ITEM_Double);
                Vfy_push(cntxt, ITEM_Double_2);
                ip++;
                break;

            }

            case DNEG: {
                Vfy_pop(cntxt, ITEM_Double_2);
                Vfy_pop(cntxt, ITEM_Double);
                Vfy_push(cntxt, ITEM_Double);
                Vfy_push(cntxt, ITEM_Double_2);
                ip++;
                break;
            }


            case IINC: {
                SLOTINDEX index = Vfy_getUByte(cntxt, ip + 1);
                ip += 3;
                Vfy_getLocal(cntxt, index, ITEM_Integer);
                Vfy_setLocal(cntxt, index, ITEM_Integer); /* NOTE superfluous? */
                break;

            }

            case I2L: {
                Vfy_pop(cntxt, ITEM_Integer);
                Vfy_push(cntxt, ITEM_Long);
                Vfy_push(cntxt, ITEM_Long_2);
                ip++;
                break;

            }

            case L2I: {
                Vfy_pop(cntxt, ITEM_Long_2);
                Vfy_pop(cntxt, ITEM_Long);
                Vfy_push(cntxt, ITEM_Integer);
                ip++;
                break;

            }


            case I2F: {
                Vfy_pop(cntxt, ITEM_Integer);
                Vfy_push(cntxt, ITEM_Float);
                ip++;
                break;

            }

            case I2D: {
                Vfy_pop(cntxt, ITEM_Integer);
                Vfy_push(cntxt, ITEM_Double);
                Vfy_push(cntxt, ITEM_Double_2);
                ip++;
                break;

            }

            case L2F: {
                Vfy_pop(cntxt, ITEM_Long_2);
                Vfy_pop(cntxt, ITEM_Long);
                Vfy_push(cntxt, ITEM_Float);
                ip++;
                break;

            }

            case L2D: {
                Vfy_pop(cntxt, ITEM_Long_2);
                Vfy_pop(cntxt, ITEM_Long);
                Vfy_push(cntxt, ITEM_Double);
                Vfy_push(cntxt, ITEM_Double_2);
                ip++;
                break;

            }

            case F2I: {
                Vfy_pop(cntxt, ITEM_Float);
                Vfy_push(cntxt, ITEM_Integer);
                ip++;
                break;

            }

            case F2L: {
                Vfy_pop(cntxt, ITEM_Float);
                Vfy_push(cntxt, ITEM_Long);
                Vfy_push(cntxt, ITEM_Long_2);
                ip++;
                break;

            }

            case F2D: {
                Vfy_pop(cntxt, ITEM_Float);
                Vfy_push(cntxt, ITEM_Double);
                Vfy_push(cntxt, ITEM_Double_2);
                ip++;
                break;

            }

            case D2I: {
                Vfy_pop(cntxt, ITEM_Double_2);
                Vfy_pop(cntxt, ITEM_Double);
                Vfy_push(cntxt, ITEM_Integer);
                ip++;
                break;

            }

            case D2L: {
                Vfy_pop(cntxt, ITEM_Double_2);
                Vfy_pop(cntxt, ITEM_Double);
                Vfy_push(cntxt, ITEM_Long);
                Vfy_push(cntxt, ITEM_Long_2);
                ip++;
                break;

            }

            case D2F: {
                Vfy_pop(cntxt, ITEM_Double_2);
                Vfy_pop(cntxt, ITEM_Double);
                Vfy_push(cntxt, ITEM_Float);
                ip++;
                break;

            }


            case I2B:
            case I2C:
            case I2S: {
                Vfy_pop(cntxt, ITEM_Integer);
                Vfy_push(cntxt, ITEM_Integer);
                ip++;
                break;

            }

            case LCMP: {
                Vfy_pop(cntxt, ITEM_Long_2);
                Vfy_pop(cntxt, ITEM_Long);
                Vfy_pop(cntxt, ITEM_Long_2);
                Vfy_pop(cntxt, ITEM_Long);
                Vfy_push(cntxt, ITEM_Integer);
                ip++;
                break;

            }


            case FCMPL:
            case FCMPG: {
                Vfy_pop(cntxt, ITEM_Float);
                Vfy_pop(cntxt, ITEM_Float);
                Vfy_push(cntxt, ITEM_Integer);
                ip++;
                break;

            }

            case DCMPL:
            case DCMPG: {
                Vfy_pop(cntxt, ITEM_Double_2);
                Vfy_pop(cntxt, ITEM_Double);
                Vfy_pop(cntxt, ITEM_Double_2);
                Vfy_pop(cntxt, ITEM_Double);
                Vfy_push(cntxt, ITEM_Integer);
                ip++;
                break;

            }


            case IF_ICMPEQ:
            case IF_ICMPNE:
            case IF_ICMPLT:
            case IF_ICMPGE:
            case IF_ICMPGT:
            case IF_ICMPLE: {
                Vfy_pop(cntxt, ITEM_Integer);
                Vfy_pop(cntxt, ITEM_Integer);
                Vfy_checkJumpTarget(cntxt, ip, (IPINDEX)(ip + Vfy_getShort(cntxt, ip + 1)));
                ip += 3;
                break;
            }

            case IFEQ:
            case IFNE:
            case IFLT:
            case IFGE:
            case IFGT:
            case IFLE: {
                Vfy_pop(cntxt, ITEM_Integer);
                Vfy_checkJumpTarget(cntxt, ip, (IPINDEX)(ip + Vfy_getShort(cntxt, ip + 1)));
                ip += 3;
                break;
            }

            case IF_ACMPEQ:
            case IF_ACMPNE: {
                Vfy_pop(cntxt, ITEM_Reference);
                Vfy_pop(cntxt, ITEM_Reference);
                Vfy_checkJumpTarget(cntxt, ip, (IPINDEX)(ip + Vfy_getShort(cntxt, ip + 1)));
                ip += 3;
                break;
            }

            case IFNULL:
            case IFNONNULL:
                Vfy_pop(cntxt, ITEM_Reference);
                Vfy_checkJumpTarget(cntxt, ip, (IPINDEX)(ip + Vfy_getShort(cntxt, ip + 1)));
                ip += 3;
                break;

            case GOTO: {
                Vfy_checkJumpTarget(cntxt, ip, (IPINDEX)(ip + Vfy_getShort(cntxt, ip + 1)));
                ip += 3;
                noControlFlow = CVM_TRUE;
                break;
            }

            case GOTO_W: {
                Vfy_checkJumpTarget(cntxt, ip, (ip + Vfy_getCell(cntxt, ip + 1)));
                ip += 5;
                noControlFlow = CVM_TRUE;
                break;
            }

            case TABLESWITCH:
            case LOOKUPSWITCH: {
                IPINDEX lpc = ((ip + 1) + 3) & ~3; /* Advance one byte and align to the next word boundary */
                IPINDEX lptr;
                int keys;
                int k, delta;

		cntxt->vContainsSwitch = CVM_TRUE;
		
	       /*
                * Pop the switch argument
                */
                Vfy_pop(cntxt, ITEM_Integer);

                /* 6819090: Verify 0 padding */
                {
                    IPINDEX padpc =  ip + 1;
                    for (; padpc < lpc; padpc++) {
                        if (Vfy_getUByte(cntxt, padpc) != 0) {
                            Vfy_throw(cntxt, VE_SWITCH_NONZERO_PADDING);
                        }
                    }
                }

               /*
                * Get the number of keys and the delta between each entry
                */
                if (opcode == TABLESWITCH) {
                    CVMInt32 low  = Vfy_getCell(cntxt, lpc + 4);
                    CVMInt32 high = Vfy_getCell(cntxt, lpc + 8);

                    CVMassert((CVMUint32)high - (CVMUint32)low <= 0xffff);

                    keys = high - low + 1;
                    delta = 4;
                } else {
                    keys = Vfy_getCell(cntxt, lpc + 4);
                    CVMassert(keys >= 0);
                    delta = 8;
                    /*
                     * Make sure that the lookupswitch items are sorted
                     */
                    for (k = keys - 1, lptr = lpc + 8 ; --k >= 0 ; lptr += 8) {
                        long this_key = Vfy_getCell(cntxt, lptr);
                        long next_key = Vfy_getCell(cntxt, lptr + 8);
                        if (this_key >= next_key) {
                            Vfy_throw(cntxt, VE_BAD_LOOKUPSWITCH);
                        }
                    }
                }

               /*
                * Check the default case
                */
                Vfy_checkJumpTarget(cntxt, ip, (ip + Vfy_getCell(cntxt, lpc)));

               /*
                * Check all the other cases
                */
                for (k = keys, lptr = lpc + 12 ; --k >= 0; lptr += delta) {
                    Vfy_checkJumpTarget(cntxt, ip, (ip + Vfy_getCell(cntxt, lptr)));
                }

               /*
                * Advance the virtual ip to the next instruction
                */
                ip = lptr - delta + 4;
                noControlFlow = CVM_TRUE;

                break;
            }

            case IRETURN: {
                Vfy_popReturn(cntxt, ITEM_Integer);
                ip++;
                noControlFlow = CVM_TRUE;
                break;

            }

            case LRETURN: {
                Vfy_pop(cntxt, ITEM_Long_2);
                Vfy_popReturn(cntxt, ITEM_Long);
                ip++;
                noControlFlow = CVM_TRUE;
                break;

            }


            case FRETURN: {
                Vfy_popReturn(cntxt, ITEM_Float);
                ip++;
                noControlFlow = CVM_TRUE;
                break;

            }

            case DRETURN: {
                Vfy_pop(cntxt, ITEM_Double_2);
                Vfy_popReturn(cntxt, ITEM_Double);
                ip++;
                noControlFlow = CVM_TRUE;
                break;
            }


            case ARETURN: {
                Vfy_popReturn(cntxt, Vfy_getObjectVerifierType());
                ip++;
                noControlFlow = CVM_TRUE;
                break;

            }

            case RETURN: {
                Vfy_returnVoid(cntxt);
                ip++;
                noControlFlow = CVM_TRUE;
                break;
            }

            case GETSTATIC:
            case PUTSTATIC:
            case GETFIELD:
            case PUTFIELD: {
                POOLINDEX fieldNameAndTypeIndex;
                CVMClassTypeID fieldTypeKey;

               /*
                * Get the index into the constant pool where the field is defined
                */
                POOLINDEX fieldIndex = Vfy_getUShort(cntxt, ip + 1);

               /*
                * Check that the tag is correct
                */
                Pol_checkTagIs(cntxt, vPool, fieldIndex, CONSTANT_Fieldref, VE_EXPECT_FIELDREF);

               /*
                * Get the indexto the name-and-type entry
                */
                fieldNameAndTypeIndex = Pol_getNameAndTypeIndex(vPool, fieldIndex);

               /*
                * Get the field type key ("I", "J, "Ljava.lang.Foobar" etc)
                */
                fieldTypeKey = Pol_getFieldTypeKey(vPool, fieldNameAndTypeIndex);

                if (opcode == GETFIELD || opcode == PUTFIELD) {

                   /*
                    * Get the index to the class of the field
                    */
                    POOLINDEX fieldClassIndex = Pol_getClassIndex(vPool, fieldIndex);

                   /*
                    * Verification of access to protected fields is done here because
                    * the superclass must be loaded at this time (other checks are
                    * done at runtime in order to implement lazy class loading.)
                    *
                    * In order for the access to a protected field to be legal the
                    * class of this method must be a direct descendant of the class
                    * who's field is being accessed. By placing the method's class
                    * in place of the class specified in the constant pool this
                    * will be checked.
                    */
                    VERIFIERTYPE targetVerifierType;

                    if (Vfy_isProtectedField(vClass, fieldIndex)) {
                        targetVerifierType = Vfy_toVerifierType(Cls_getKey(vClass));
                    } else {
                        targetVerifierType = Vfy_toVerifierType(Pol_getClassKey(vPool, fieldClassIndex));
                    }

                   /*
                    * GETFIELD / PUTFIELD
                    */
                    if (opcode == GETFIELD) {
                        Vfy_pop(cntxt, targetVerifierType);
                        Vfy_pushClassKey(cntxt, fieldTypeKey);
                    } else {
                        VERIFIERTYPE obj_type;
                        Vfy_popClassKey(cntxt, fieldTypeKey);

                        /* 2nd edition JVM allows field initializations
                         * before the super class initializer if the
                         * field is defined within the current class.
                         * Section 4.8.2, JVM spec 2nd edition.
                         */
                        obj_type = Vfy_popCategory1(cntxt); 
                        if (obj_type == ITEM_InitObject) {
                            /* special case for uninitialized objects */
                            FIELD thisField = NULL;
                            NameTypeKey nameTypeKey = 
                              Pol_getFieldNameTypeKey(vPool, fieldNameAndTypeIndex);
                            CVMClassTypeID fieldClassKey =
                              Pol_getClassKey(vPool, fieldClassIndex);
                            if (Cls_getKey(vClass) == fieldClassKey) {
                                thisField =
                                    Cls_lookupField((CLASS) vClass, nameTypeKey);
                            }
                            if (thisField == NULL || 
                                Fld_getClass(thisField) != vClass) {
                                Vfy_throw(cntxt, VE_EXPECTING_OBJ_OR_ARR_ON_STK);
                            }
                        } else {
                            Vfy_push(cntxt, obj_type);
                            Vfy_pop(cntxt, targetVerifierType);
                        }
                    }
                } else {

                   /*
                    * GETSTATIC / PUTSTATIC
                    */
                    if (opcode == GETSTATIC) {
                        Vfy_pushClassKey(cntxt, fieldTypeKey);
                    } else {
                        Vfy_popClassKey(cntxt, fieldTypeKey);
                    }
                }

                ip += 3;
                break;
            }

            case INVOKEVIRTUAL:
            case INVOKESPECIAL:
            case INVOKESTATIC:
            case INVOKEINTERFACE: {
                POOLINDEX      methodNameAndTypeIndex;
                POOLINDEX      methodClassIndex;
                CVMMethodTypeID  methodNameKey;
                CVMMethodTypeID  methodTypeKey;
                int            nwords;

               /*
                * Get the index into the constant pool where the method is defined
                */
                POOLINDEX methodIndex = Vfy_getUShort(cntxt, ip + 1);

               /*
                * Check that the tag is correct
                */
                Pol_checkTag2Is(cntxt, vPool,
                                methodIndex,
                                CONSTANT_Methodref,           /* this or...*/
                                CONSTANT_InterfaceMethodref,  /* this */
                                VE_EXPECT_METHODREF);

               /*
                * Get the index to the class of the method
                */
                methodClassIndex = Pol_getClassIndex(vPool, methodIndex);;

               /*
                * Get the indexto the name-and-type entry
                */
                methodNameAndTypeIndex = Pol_getNameAndTypeIndex(vPool, methodIndex);

               /*
                * Get the method type key ("(I)J", "(III)D", "(Ljava.lang.Foobar)V" etc.)
                */
                methodTypeKey = Pol_getMethodTypeKey(vPool, methodNameAndTypeIndex);

               /*
                * Get the method name
                */
                methodNameKey = Pol_getMethodDescriptorKey(vPool, methodNameAndTypeIndex);

               /*
                * Setup the callee context
                */
                Vfy_setupCalleeContext(cntxt, methodTypeKey);

               /*
                * Pop the arguments from the stack
                */
                nwords = Vfy_popInvokeArguments(cntxt);

	       /*
	        * The format verifier has already ensured that the only
		* method names starting with "<" are "<clinit>" and "<init>"
		* With that in mind, make sure that the one is never invoked,
		* and the other referenced only by INVOKESPECIAL.
		*/
		if (CVMtypeidIsClinit(methodNameKey)){
		    Vfy_throw(cntxt, VE_CALLS_CLINIT);
		}
		if (CVMtypeidIsConstructor(methodNameKey) && 
		    opcode != INVOKESPECIAL)
		{
		    Vfy_throw(cntxt, VE_EXPECT_INVOKESPECIAL);
                }

               /*
                * Receiver type processing
                */
                if (opcode != INVOKESTATIC) {
                    CVMClassTypeID methodClassKey = Pol_getClassKey(vPool, 
							    methodClassIndex);
                    CVMClassTypeID targetClassKey;

                   /*
                    * Special processing for calls to <init>
                    */
                    if (CVMtypeidIsConstructor(methodNameKey)) {

                       /*
                        * Pop the receiver type
                        */
                        VERIFIERTYPE receiverType = Vfy_popCategory1(cntxt);

                        if (Vfy_isVerifierTypeForNew(receiverType)) {
                           /*
                            * This for is a call to <init> that is made to an object that has just been
                            * created with a NEW bytecode. The address of the new was encoded
                            * as a VERIFIERTYPE by the NEW bytecode using a function called
                            * Vfy_createVerifierTypeForNewAt(). This is converted back into
                            * an IP address and is used to lookup the receiver CVMClassTypeID
                            */

                            POOLINDEX newIndex;

                           /*
                            * receiverType is a special encoding of the ip offset into the
                            * method where a NEW instruction should be. Get the index
                            * and check this out.
                            */
                            IPINDEX newIP = Vfy_retrieveIpForNew(receiverType);
                            if (newIP > codeLength - 3 || Vfy_getOpcode(cntxt, newIP) != NEW) {
                                Vfy_throw(cntxt, VE_EXPECT_NEW);
				/* NOTREACHED */
                            }

                           /*
                            * Get the pool index reference by the NEW bytecode 
			    * and check that it is a CONSTANT_class
                            */
                            newIndex = Vfy_getUShort(cntxt, newIP + 1);
                            Pol_checkTagIsClass(cntxt, vPool, newIndex);

                           /*
                            * Get the CVMClassTypeID for is entry
                            */
                            targetClassKey = Pol_getClassKey(vPool, newIndex);

                           /*
                            * The method must be an <init> method of the 
			    * indicated class
                            */
                            if (targetClassKey != methodClassKey) {
                                Vfy_throw(cntxt, VE_BAD_INIT_CALL);
				/* NOTREACHED */
                            }

                        } else if (receiverType == ITEM_InitObject) {

                           /*
                            * This for is a call to <init> that is made as a result of
                            * a constructor calling another constructor (e.g. this() or super())
                            * The receiver CVMClassTypeID must be the same as the one for the class
                            * of the method being verified or its superclass
                            */
                            targetClassKey = Cls_getKey(vClass);

                            /*
                             * Check that it is this() or super()
                             */
                            if ((methodClassKey != targetClassKey && 
                                  methodClassKey != Cls_getKey(vSuperClass))
                                || !cntxt->vNeedInitialization)
			    {
                                Vfy_throw(cntxt, VE_BAD_INIT_CALL);
				/* NOTREACHED */
                            }

                            /* Make sure that the current opcode is not
                             * inside any exception handlers.
                             */
                            if (inTryBlock){
				Vfy_throw(cntxt, VE_BAD_INIT_CALL);
				/* NOTREACHED */
                            }

                            cntxt->vNeedInitialization = CVM_FALSE;
                        } else {
			    targetClassKey = 0; /* placate compiler */
                            Vfy_throw(cntxt, VE_EXPECT_UNINIT);
			    /* NOTREACHED */
                        }

                       /*
                        * Replace all the <init> receiver type with the 
			* real target type
                        */
                        Vfy_ReplaceTypeWithType(cntxt, receiverType, 
					Vfy_toVerifierType(targetClassKey));

                    } else {

                       /*
                        * This is for the non INVOKESTATIC case where <init> is not being called.
                        *
                        * Check that an INVOKESPECIAL is either to the method being verified
                        * or a superclass.
                        */
                        if (opcode == INVOKESPECIAL && methodClassKey != Cls_getKey(vClass)) {
                            CLASS superClass = vSuperClass;
                            while (superClass != NULL && Cls_getKey(superClass) != methodClassKey) {
                                superClass = Cls_getSuper(superClass);
                            }
                            if (superClass == NULL) {
                                Vfy_throw(cntxt, VE_INVOKESPECIAL);
				/* NOTREACHED */
                            }
                        }

                       /*
                        * Verification of access to protected methods is done here because
                        * the superclass must be loaded at this time (other checks are
                        * done at runtime in order to implement lazy class loading.)
                        *
                        * In order for the access to a protected method to be legal the
                        * class of this method must be a direct descendant of the class
                        * who's field is being accessed. By placing the method's class
                        * in place of the class specified in the constant pool this
                        * will be checked.
                        */
                        if (
                             (opcode == INVOKESPECIAL || opcode == INVOKEVIRTUAL) &&
                             (Vfy_isProtectedMethod(vClass, methodIndex))
                           )
                        {
                            /* This is ugly. Special dispensation.  Arrays
                               pretend to implement public Object clone()
                               even though they don't */
                            int is_clone = 
                                CVMtypeidIsSameName(methodNameKey,
                                                    CVMglobals.cloneTid);
                            int is_object_class =
                                (methodClassKey ==
                                 Vfy_getSystemClassName(java_lang_Object));
                            int is_array_object =
                                (cntxt->vSP > 0 &&
                                 Vfy_isArray(cntxt->vStack[cntxt->vSP - 1]));
                            if (is_clone && is_object_class && is_array_object){
                                Vfy_pop(cntxt,
                                        Vfy_toVerifierType(methodClassKey));
                            } else {
                                Vfy_pop(cntxt,
                                        Vfy_toVerifierType(Cls_getKey(vClass)));
                            }
                        } else {
                            Vfy_pop(cntxt,  Vfy_toVerifierType(methodClassKey));
                        }
                    }
                }

               /*
                * Push the result type.
                */
                Vfy_pushInvokeResult(cntxt);

               /*
                * Test a few INVOKEINTERFACE things and increment the ip
                */
                if (opcode == INVOKEINTERFACE) {
                    if (Vfy_getUByte(cntxt, ip + 3) != nwords + 1) {
                        Vfy_throw(cntxt, VE_NARGS_MISMATCH);
                    }
                    if (Vfy_getUByte(cntxt, ip + 4) != 0) {
                        Vfy_throw(cntxt, VE_EXPECT_ZERO);
                    }
                    ip += 5;
                } else {
                    ip += 3;
                }
                break;
            }

            case NEW: {
                CVMClassTypeID typeKey;

               /*
                * Get the pool index and check that it is a CONSTANT_Class
                */
                POOLINDEX index = Vfy_getUShort(cntxt, ip + 1);
                Pol_checkTagIsClass(cntxt, vPool, index);

               /*
                * Get the class and check that is it not an array class
                */
                typeKey = Pol_getClassKey(vPool, index);
                if (Vfy_isArrayClassKey(typeKey, 1)) {
                    Vfy_throw(cntxt, VE_EXPECT_CLASS);
		    /* NOTREACHED */
                }

               /*
                * Convert the IP address to a VERIFIERTYPE that indicates the
                * result of executing a NEW
                */
                Vfy_push(cntxt, Vfy_createVerifierTypeForNewAt(ip));

               /*
                * Mark the new instruction as existing. This is used later to check
                * that all the stackmap ITEM_new entries were valid.
                */
                Vfy_markNewInstruction(cntxt, ip, codeLength);

               /*
                * Advance the IP to next instruction
                */
                ip += 3;
                break;
            }

            case NEWARRAY: {
                CVMClassTypeID typeKey;

               /*
                * Get the pool tag and convert it a VERIFIERTYPE
                */
                POOLTAG tag = Vfy_getUByte(cntxt, ip + 1);
                switch (tag) {
                    case T_BOOLEAN: 
			typeKey = Vfy_getBooleanArrayVerifierType();
			break;
                    case T_CHAR:    
			typeKey = Vfy_getCharArrayVerifierType();
			break;
                    case T_FLOAT:
			typeKey = Vfy_getFloatArrayVerifierType();
			break;
                    case T_DOUBLE:
			typeKey = Vfy_getDoubleArrayVerifierType(); 
			break;
                    case T_BYTE:
			typeKey = Vfy_getByteArrayVerifierType();
			break;
                    case T_SHORT:
			typeKey = Vfy_getShortArrayVerifierType();
			break;
                    case T_INT:
			typeKey = Vfy_getIntArrayVerifierType();
			break;
                    case T_LONG:
			typeKey = Vfy_getLongArrayVerifierType();
			break;
                    default:
			typeKey = 0; /* placate compiler */
			Vfy_throw(cntxt, VE_BAD_INSTR);
			/* NOTREACHED */
                }

               /*
                * Pop the length and push the array type
                */
                Vfy_pop(cntxt, ITEM_Integer);
                Vfy_push(cntxt, typeKey);

               /*
                * Advance the IP to next instruction
                */
                ip += 2;
                break;
            }

            case ANEWARRAY: {
                CVMClassTypeID     arrayKey;
                VERIFIERTYPE arrayType;

               /*
                * Get the pool index and check it is CONSTANT_Class
                */
                POOLINDEX index = Vfy_getUShort(cntxt, ip + 1);
                Pol_checkTagIsClass(cntxt, vPool, index);

               /*
                * Get the CVMClassTypeID
                */
                arrayKey = Pol_getClassKey(vPool, index);

               /*
                * Convert the CVMClassTypeID to a VERIFIERTYPE 
		* of an array of typeKey elements
                */
                arrayType = Vfy_getClassArrayVerifierType(arrayKey);

               /*
                * Pop the length and push the array type
                */
                Vfy_pop(cntxt, ITEM_Integer);
                Vfy_push(cntxt, arrayType);

               /*
                * Advance the IP to next instruction
                */
                ip += 3;
                break;
            }

            case ARRAYLENGTH: {

               /*
                * Pop some kind of array
                */
                VERIFIERTYPE arrayType = Vfy_pop(cntxt, Vfy_getObjectVerifierType());

               /*
                * Check that this is an array of at least one dimension
                */
                if (!Vfy_isArrayOrNull(arrayType)) {
                    Vfy_throw(cntxt, VE_EXPECT_ARRAY);
                }

               /*
                * Push the integer result and advance the IP to the next instruction
                */
                Vfy_push(cntxt, ITEM_Integer);
                ip += 1;
                break;

            }

            case CHECKCAST: {
                CVMClassTypeID typeKey;

               /*
                * Get the pool index and check it is CONSTANT_Class
                */
                POOLINDEX index = Vfy_getUShort(cntxt, ip + 1);
                Pol_checkTagIsClass(cntxt, vPool, index);

               /*
                * Pop the type casting from and push the type casting to
                */
                typeKey = Pol_getClassKey(vPool, index);
                Vfy_pop(cntxt, Vfy_getObjectVerifierType());
                Vfy_push(cntxt, Vfy_toVerifierType(typeKey));

               /*
                * Advance the IP to next instruction
                */
                ip += 3;
                break;

            }

            case INSTANCEOF: {

               /*
                * Get the pool index and check it is CONSTANT_Class
                */
                POOLINDEX index = Vfy_getUShort(cntxt, ip + 1);
                Pol_checkTagIsClass(cntxt, vPool, index);

               /*
                * Pop the type casting from and push a boolean (int)
                */
                Vfy_pop(cntxt, Vfy_getObjectVerifierType());
                Vfy_push(cntxt, ITEM_Integer);

               /*
                * Advance the IP to next instruction
                */
                ip += 3;
                break;

            }

            case MONITORENTER:
            case MONITOREXIT: {

               /*
                * Pop some kind of object and advance the IP to next instruction
                */
                Vfy_pop(cntxt, Vfy_getObjectVerifierType());
                ip++;
                break;

            }

            case MULTIANEWARRAY: {
                CVMClassTypeID typeKey;
                int dim, i;

               /*
                * Get the pool index and check it is CONSTANT_Class
                */
                POOLINDEX index = Vfy_getUShort(cntxt, ip + 1);
                Pol_checkTagIsClass(cntxt, vPool, index);

               /*
                * Get the CVMClassTypeID and the number of dimensions
                */
                typeKey = Pol_getClassKey(vPool, index);
                dim = Vfy_getUByte(cntxt, ip + 3);

               /*
                * Check that the number of dimensions is not zero and that
                * the number specified in the array type is at least as great
                */
                if (dim == 0 || !Vfy_isArrayClassKey(typeKey, dim)) {
                    Vfy_throw(cntxt, VE_MULTIANEWARRAY);
                }

               /*
                * Pop all the dimension sizes
                */
                for (i = 0; i < dim; i++) {
                    Vfy_pop(cntxt, ITEM_Integer);
                }

               /*
                * Push the array type and advance the IP to next instruction
                */
                Vfy_push(cntxt, Vfy_toVerifierType(typeKey));
                ip += 4;
                break;
            }
            case ATHROW: {
                Vfy_pop(cntxt, Vfy_getThrowableVerifierType());
                ip++;
                noControlFlow = CVM_TRUE;
                break;
            }

            case WIDE: {
                SLOTINDEX index;
                switch (Vfy_getUByte(cntxt, ip + 1)) {

                    case IINC: {
                        index = Vfy_getUShort(cntxt, ip + 2);
                        ip += 6;
                        Vfy_getLocal(cntxt, index, ITEM_Integer);
                        Vfy_setLocal(cntxt, index, ITEM_Integer);
                        break;
                    }

                    case ILOAD: {
                        index = Vfy_getUShort(cntxt, ip + 2);
                        ip += 4;
                        Vfy_getLocal(cntxt, index, ITEM_Integer);
                        Vfy_push(cntxt, ITEM_Integer);
                        break;
                    }

                    case ALOAD: {
                        CVMClassTypeID refType;
                        index = Vfy_getUShort(cntxt, ip + 2);
                        ip += 4;
                        refType = Vfy_getLocal(cntxt, index, ITEM_Reference);
                        Vfy_push(cntxt, refType);
                        break;
                    }

                    case LLOAD: {
                        index = Vfy_getUShort(cntxt, ip + 2);
                        ip += 4;
                        Vfy_getLocal(cntxt, index,     ITEM_Long);
                        Vfy_getLocal(cntxt, index + 1, ITEM_Long_2);
                        Vfy_push(cntxt, ITEM_Long);
                        Vfy_push(cntxt, ITEM_Long_2);
                        break;
                    }

                    case ISTORE: {
                        index = Vfy_getUShort(cntxt, ip + 2);
                        ip += 4;
                        Vfy_pop(cntxt, ITEM_Integer);
                        Vfy_setLocal(cntxt, index, ITEM_Integer);
                        break;
                    }

                    case ASTORE: {
                        CVMClassTypeID arrayElementType;
                        index = Vfy_getUShort(cntxt, ip + 2);
                        ip += 4;
                        arrayElementType = Vfy_pop(cntxt, ITEM_Reference);
                        Vfy_setLocal(cntxt, index, arrayElementType);
                        break;
                    }

                    case LSTORE: {
                        index = Vfy_getUShort(cntxt, ip + 2);
                        ip += 4;
                        Vfy_pop(cntxt, ITEM_Long_2);
                        Vfy_pop(cntxt, ITEM_Long);
                        Vfy_setLocal(cntxt, index + 1, ITEM_Long_2);
                        Vfy_setLocal(cntxt, index, ITEM_Long);
                        break;
                    }


                    case FLOAD: {
                        index = Vfy_getUShort(cntxt, ip + 2);
                        ip += 4;
                        Vfy_getLocal(cntxt, index, ITEM_Float);
                        Vfy_push(cntxt, ITEM_Float);
                        break;

                    }

                    case DLOAD: {
                        index = Vfy_getUShort(cntxt, ip + 2);
                        ip += 4;
                        Vfy_getLocal(cntxt, index, ITEM_Double);
                        Vfy_getLocal(cntxt, index + 1, ITEM_Double_2);
                        Vfy_push(cntxt, ITEM_Double);
                        Vfy_push(cntxt, ITEM_Double_2);
                        break;

                    }

                    case FSTORE: {
                        index = Vfy_getUShort(cntxt, ip + 2);
                        ip += 4;
                        Vfy_pop(cntxt, ITEM_Float);
                        Vfy_setLocal(cntxt, index, ITEM_Float);
                        break;

                    }

                    case DSTORE: {
                        index = Vfy_getUShort(cntxt, ip + 2);
                        ip += 4;
                        Vfy_pop(cntxt, ITEM_Double_2);
                        Vfy_pop(cntxt, ITEM_Double);
                        Vfy_setLocal(cntxt, index + 1, ITEM_Double_2);
                        Vfy_setLocal(cntxt, index, ITEM_Double);
                        break;
                    }


                    default: {
                        Vfy_throw(cntxt, VE_BAD_INSTR);
                    }
                }
                break;
            }

            default: {
                Vfy_throw(cntxt, VE_BAD_INSTR);
                break;
            }
        } /* End of switch statement */
    }

    /* Make sure that the bytecode offset within 
     * stackmaps does not exceed code size
     */

    if (Mth_checkStackMapOffset(cntxt, currentStackMapIndex) == 0) {
        Vfy_throw(cntxt, VE_BAD_STACKMAP);
    }

    /* Make sure that control flow does not fall through 
     * the end of the method. 
     */

    if (ip > codeLength) {
        Vfy_throw(cntxt, VE_MIDDLE_OF_BYTE_CODE);
    }

    if (!noControlFlow) {
        Vfy_throw(cntxt, VE_FALL_THROUGH);
    }

    /* All done */
}

