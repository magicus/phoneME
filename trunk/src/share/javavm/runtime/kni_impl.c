/*
 * @(#)kni_impl.c	1.14 06/10/17
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
#include "javavm/include/interpreter.h"
#include "javavm/include/stacks.h"
#include "javavm/include/localroots.h"
#include "javavm/include/globalroots.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/typeid.h"
#include "javavm/include/common_exceptions.h"
#include "generated/offsets/java_lang_String.h"

#include "javavm/export/kni.h"

/*
 * Some macros needed by the kni implementation
 */
#undef CVMjniGcUnsafeRef2Class
#define CVMjniGcUnsafeRef2Class(ee, cl) CVMgcUnsafeClassRef2ClassBlock(ee, cl)

#undef CVMjniGcSafeRef2Class
#define CVMjniGcSafeRef2Class(ee, cl) CVMgcSafeClassRef2ClassBlock(ee, cl)

/* TODO - this should go away. */
extern void
JVMSPI_PrintRaw(const char* s) {
    CVMconsolePrintf(s);
}

KNIEXPORT void
KNI_FindClassImpl(CVMExecEnv* ee, const char* name, jclass classHandle)
{
    JNIEnv* env = CVMexecEnv2JniEnv(ee);
    CVMassert(classHandle != NULL);
    CVMD_gcSafeExec(ee, {
	jclass result;
	CVMjniPushLocalFrame(env, 4);
	/* TODO - we are only suppose to return classes that are
	   already loaded and initialized. */
	result = CVMjniFindClass(env, name);
	if (result != NULL) {
	    CVMID_icellAssign(ee, classHandle, result);
	} else {
	    CVMclearLocalException(ee);
	    CVMID_icellSetNull(classHandle);
	}
	CVMjniPopLocalFrame(env, NULL);
	CVMassert(!CVMlocalExceptionOccurred(ee));
    });
}

KNIEXPORT void
KNI_GetSuperClassImpl(CVMExecEnv* ee, 
		      jclass classHandle, jclass superclassHandle)
{
    CVMClassBlock* cb = CVMjniGcUnsafeRef2Class(ee, classHandle);
    CVMClassBlock* superCb;

    if (CVMcbIs(cb, INTERFACE)) {
	superCb = NULL;
    } else {
	superCb = CVMcbSuperclass(cb);
    }
    if (superCb == NULL) {
	CVMID_icellSetNull(superclassHandle);
    } else {
	CVMID_icellAssignDirect(ee, superclassHandle,
				CVMcbJavaInstance(superCb));
    }
    CVMassert(!CVMlocalExceptionOccurred(ee));
}

KNIEXPORT jboolean
KNI_IsAssignableFromPriv(CVMExecEnv* ee,
			 jclass classHandle1, jclass classHandle2)
{
    CVMClassBlock* srcCb = CVMjniGcUnsafeRef2Class(ee, classHandle1);
    CVMClassBlock* dstCb = CVMjniGcUnsafeRef2Class(ee, classHandle2);
    jboolean result = CVMisAssignable(ee, srcCb, dstCb);
    CVMassert(!CVMlocalExceptionOccurred(ee));
    return result;
}

KNIEXPORT jint
KNI_ThrowNewImpl(CVMExecEnv* ee, const char* name, const char* message)
{
    JNIEnv* env = CVMexecEnv2JniEnv(ee);
    jint result = KNI_OK;
    CVMD_gcSafeExec(ee, {
	jclass clazz;
	CVMjniPushLocalFrame(env, 4);
	clazz = CVMjniFindClass(env, name);
	if (clazz != NULL) {
	    CVMjniThrowNew(env, clazz, message);
	} else {
	    CVMclearLocalException(ee);
	    result = KNI_ERR;
	}
	CVMjniPopLocalFrame(env, NULL);
    });
    CVMassert(CVMlocalExceptionOccurred(ee) == (result == KNI_OK));
    return result;
}

KNIEXPORT void
KNI_FatalErrorImpl(CVMExecEnv* ee, const char* message)
{
    JNIEnv* env = CVMexecEnv2JniEnv(ee);
    CVMD_gcSafeExec(ee, {
	CVMjniPushLocalFrame(env, 4);
	CVMjniFatalError(env, message);
	CVMjniPopLocalFrame(env, NULL);
    });
}

KNIEXPORT jboolean
KNI_IsInstanceOfPriv(CVMExecEnv* ee, jobject objectHandle, jclass classHandle)
{
    if (objectHandle == NULL) {
	return KNI_TRUE;
    } else {
	CVMClassBlock* cb = CVMjniGcUnsafeRef2Class(ee, classHandle);
	CVMObject* directObj = CVMID_icellDirect(ee, objectHandle);
	jboolean result = CVMgcUnsafeIsInstanceOf(ee, directObj, cb);
	CVMassert(!CVMlocalExceptionOccurred(ee));
	return result;
    }
}

/**
 * Field access helpers
 */

#ifdef CVM_DEBUG_ASSERTS
static void
KNI_FIELD_ASSERTS(jobject obj, jfieldID jfb, CVMBool isStatic)
{
    /* Simple checks */
    CVMassert(obj != NULL);
    CVMassert(jfb != NULL);
    CVMassert(CVMfbIs(jfb, STATIC) == isStatic);
}
#else
#define KNI_FIELD_ASSERTS(obj, jfb, isStatic)
#endif


KNIEXPORT jfieldID
KNI_GetFieldIDPriv(CVMExecEnv* ee, jclass classHandle,
		   const char* name, const char* sig, CVMBool isStatic)
{
    CVMFieldBlock* fb;
    CVMClassBlock* cb = CVMjniGcUnsafeRef2Class(ee, classHandle);
    CVMFieldTypeID tid = CVMtypeidLookupFieldIDFromNameAndSig(ee, name, sig);

    if (tid != CVM_TYPEID_ERROR) {
	fb = CVMclassGetFieldBlock(cb, tid, isStatic);
	if (fb != NULL && CVMfbIs(fb, STATIC) != isStatic) {
	    fb = NULL;
	}
    } else {
	fb = NULL;
    }
    CVMassert(!CVMlocalExceptionOccurred(ee));
    return fb;
}


/**
 * Field access
 */

#define CVM_DEFINE_KNI_FIELD_GETTER(jType_, elemType_, wideType_)	\
KNIEXPORT jType_							\
KNI_Get##elemType_##Field(jobject objectHandle, jfieldID fieldID)	\
{									\
    jType_ result;							\
    CVMObject* obj;							\
    KNI_FIELD_ASSERTS(objectHandle, fieldID, CVM_FALSE);		\
    obj = CVMID_icellDirect(CVMgetEE(), objectHandle);			\
    CVMD_fieldRead##wideType_(obj, CVMfbOffset(fieldID), result);	\
    return result;							\
}

#define CVM_DEFINE_KNI_FIELD_SETTER(jType_, elemType_, wideType_)	\
KNIEXPORT void								\
KNI_Set##elemType_##Field(jobject objectHandle, jfieldID fieldID,	\
			  jType_ value)					\
{									\
    CVMObject* obj;							\
    KNI_FIELD_ASSERTS(objectHandle, fieldID, CVM_FALSE);		\
    obj = CVMID_icellDirect(CVMgetEE(), objectHandle);			\
    CVMD_fieldWrite##wideType_(obj, CVMfbOffset(fieldID), value);	\
}

CVM_DEFINE_KNI_FIELD_GETTER(jboolean, Boolean, Int)
CVM_DEFINE_KNI_FIELD_GETTER(jchar,    Char,    Int)
CVM_DEFINE_KNI_FIELD_GETTER(jshort,   Short,   Int)
CVM_DEFINE_KNI_FIELD_GETTER(jfloat,   Float,   Float)
CVM_DEFINE_KNI_FIELD_GETTER(jbyte,    Byte,    Int)
CVM_DEFINE_KNI_FIELD_GETTER(jint,     Int,     Int)

CVM_DEFINE_KNI_FIELD_SETTER(jboolean, Boolean, Int)
CVM_DEFINE_KNI_FIELD_SETTER(jchar,    Char,    Int)
CVM_DEFINE_KNI_FIELD_SETTER(jshort,   Short,   Int)
CVM_DEFINE_KNI_FIELD_SETTER(jfloat,   Float,   Float)
CVM_DEFINE_KNI_FIELD_SETTER(jbyte,    Byte,    Int)
CVM_DEFINE_KNI_FIELD_SETTER(jint,     Int,     Int)

KNIEXPORT void
KNI_GetObjectField(jobject objectHandle, jfieldID fieldID, jobject toHandle)
{
    CVMObject* obj;
    CVMObject* toObject;
    KNI_FIELD_ASSERTS(objectHandle, fieldID, CVM_FALSE);
    obj = CVMID_icellDirect(CVMgetEE(), objectHandle);
    CVMD_fieldReadRef(obj, CVMfbOffset(fieldID), toObject);
    CVMID_icellSetDirect(CVMgetEE(), toHandle, toObject);
}

KNIEXPORT void
KNI_SetObjectField(jobject objectHandle, jfieldID fieldID, jobject fromHandle)
{
    CVMObject* obj = CVMID_icellDirect(CVMgetEE(), objectHandle);
    CVMObject* fromObject = CVMID_icellDirect(CVMgetEE(), fromHandle);
    KNI_FIELD_ASSERTS(objectHandle, fieldID, CVM_FALSE);
    obj = CVMID_icellDirect(CVMgetEE(), objectHandle);
    CVMD_fieldWriteRef(obj, CVMfbOffset(fieldID), fromObject);
}

#define CVM_DEFINE_JNI_64BIT_FIELD_GETTER(jType_, elemType_)		\
KNIEXPORT jType_							\
KNI_Get##elemType_##Field(jobject objectHandle, jfieldID fieldID)	\
{									\
    jType_ v64;								\
    CVMObject* obj;							\
    KNI_FIELD_ASSERTS(objectHandle, fieldID, CVM_FALSE);		\
    obj = CVMID_icellDirect(CVMgetEE(), objectHandle);			\
    CVMD_fieldRead##elemType_(obj, CVMfbOffset(fieldID), v64);		\
    return v64;								\
}

#define CVM_DEFINE_JNI_64BIT_FIELD_SETTER(jType_, elemType_)		\
KNIEXPORT void								\
KNI_Set##elemType_##Field(jobject objectHandle, jfieldID fieldID,	\
			  jType_ rhs)					\
{									\
    CVMObject* obj;							\
    KNI_FIELD_ASSERTS(objectHandle, fieldID, CVM_FALSE);		\
    obj = CVMID_icellDirect(CVMgetEE(), objectHandle);			\
    CVMD_fieldWrite##elemType_(obj, CVMfbOffset(fieldID), rhs);		\
}

CVM_DEFINE_JNI_64BIT_FIELD_GETTER(jlong,   Long)
CVM_DEFINE_JNI_64BIT_FIELD_GETTER(jdouble, Double)
CVM_DEFINE_JNI_64BIT_FIELD_SETTER(jlong,   Long)
CVM_DEFINE_JNI_64BIT_FIELD_SETTER(jdouble, Double)

/**
 * Static field access
 */
KNIEXPORT jfieldID
KNI_GetStaticFieldIDPriv(CVMExecEnv* ee, jclass classHandle,
			 const char* name, const char* signature)
{
    JNIEnv* env = CVMexecEnv2JniEnv(ee);
    jfieldID fieldID;
    CVMD_gcSafeExec(ee, {
	fieldID = CVMjniGetStaticFieldID(env, classHandle, name, signature);
    });
    return fieldID;
}

#define CVM_DEFINE_KNI_STATIC_GETTER(jType_, elemType_, unionField_)	\
KNIEXPORT jType_							\
KNI_GetStatic##elemType_##Field(jclass classHandle, jfieldID fieldID)	\
{									\
    KNI_FIELD_ASSERTS(classHandle, fieldID, CVM_TRUE);			\
    return CVMfbStaticField(CVMgetEE(), fieldID).unionField_;		\
}

#define CVM_DEFINE_KNI_STATIC_SETTER(jType_, elemType_, unionField_)	\
KNIEXPORT void								\
KNI_SetStatic##elemType_##Field(jclass classHandle, jfieldID fieldID,	\
				jType_ value)				\
{									\
    CVMExecEnv* ee = CVMgetEE();					\
    KNI_FIELD_ASSERTS(classHandle, fieldID, CVM_TRUE);			\
    CVMfbStaticField(ee, fieldID).unionField_ = value;			\
}

CVM_DEFINE_KNI_STATIC_GETTER(jboolean, Boolean, i)
CVM_DEFINE_KNI_STATIC_GETTER(jchar,    Char,    i)
CVM_DEFINE_KNI_STATIC_GETTER(jshort,   Short,   i)
CVM_DEFINE_KNI_STATIC_GETTER(jfloat,   Float,   f)
CVM_DEFINE_KNI_STATIC_GETTER(jbyte,    Byte,    i)
CVM_DEFINE_KNI_STATIC_GETTER(jint,     Int,     i)

CVM_DEFINE_KNI_STATIC_SETTER(jboolean, Boolean, i)
CVM_DEFINE_KNI_STATIC_SETTER(jchar,    Char,    i)
CVM_DEFINE_KNI_STATIC_SETTER(jshort,   Short,   i)
CVM_DEFINE_KNI_STATIC_SETTER(jfloat,   Float,   f)
CVM_DEFINE_KNI_STATIC_SETTER(jbyte,    Byte,    i)
CVM_DEFINE_KNI_STATIC_SETTER(jint,     Int,     i)


KNIEXPORT void
KNI_GetStaticObjectField(jclass classHandle, jfieldID fieldID,
			 jobject toHandle)
{
    CVMObjectICell* fromHandle;
    KNI_FIELD_ASSERTS(classHandle, fieldID, CVM_TRUE);
    fromHandle = &CVMfbStaticField(CVMgetEE(), fieldID).r;
    CVMID_icellSetDirect(CVMgetEE(), toHandle,
			 CVMID_icellDirect(CVMgetEE(), fromHandle));
}

KNIEXPORT void
KNI_SetStaticObjectField(jclass classHandle, jfieldID fieldID,
			 jobject fromHandle)
{
    KNI_FIELD_ASSERTS(classHandle, fieldID, CVM_TRUE);
    CVMID_icellSetDirect(CVMgetEE(),
			 &CVMfbStaticField(CVMgetEE(), fieldID).r,
			 CVMID_icellDirect(CVMgetEE(), fromHandle));
}

#define CVM_DEFINE_KNI_64BIT_STATIC_GETTER(jType_, elemType_)		   \
KNIEXPORT jType_							   \
KNI_GetStatic##elemType_##Field(jclass classHandle, jfieldID fieldID)	   \
{									   \
    KNI_FIELD_ASSERTS(classHandle, fieldID, CVM_TRUE);			   \
    return CVMjvm2##elemType_(&CVMfbStaticField(CVMgetEE(), fieldID).raw); \
}

#define CVM_DEFINE_KNI_64BIT_STATIC_SETTER(jType_, elemType_, elemType2_)   \
KNIEXPORT void								    \
KNI_SetStatic##elemType_##Field(jclass classHandle, jfieldID fieldID,	    \
			  jType_ rhs)					    \
{									    \
    KNI_FIELD_ASSERTS(classHandle, fieldID, CVM_TRUE);			    \
    CVM##elemType2_##2Jvm(&CVMfbStaticField(CVMgetEE(), fieldID).raw, rhs); \
}

CVM_DEFINE_KNI_64BIT_STATIC_GETTER(jlong,   Long)
CVM_DEFINE_KNI_64BIT_STATIC_GETTER(jdouble, Double)
CVM_DEFINE_KNI_64BIT_STATIC_SETTER(jlong,   Long, long)
CVM_DEFINE_KNI_64BIT_STATIC_SETTER(jdouble, Double, double)

/**
 * String operations
 */
#ifdef CVM_DEBUG_ASSERTS
static void
CVMkniSanityCheckStringAccess(jstring string)
{
    CVMClassBlock* cb;
    CVMassert(string != NULL);
    cb = CVMobjectGetClass(CVMID_icellDirect(CVMgetEE(), string));
    CVMassert(cb == CVMsystemClass(java_lang_String));
}
#else
#define CVMkniSanityCheckStringAccess(string)
#endif

KNIEXPORT jsize
KNI_GetStringLength(jstring stringHandle)
{
    jsize strLen;
    if (CVMID_icellIsNull(stringHandle)) {
	return -1;
    }
    CVMkniSanityCheckStringAccess(stringHandle);
    CVMD_fieldReadInt(CVMID_icellDirect(CVMgetEE(), stringHandle), 
		      CVMoffsetOfjava_lang_String_count,
		      strLen);
    return strLen;
}

KNIEXPORT void 
KNI_GetStringRegion(jstring stringHandle, jsize offset,
		    jsize n, jchar* jcharbuf)
{
    CVMInt32 str_offset, str_len;
    CVMObject*      valueAsObject;
    CVMObject*      stringAsObject =
	CVMID_icellDirect(CVMgetEE(), stringHandle);
    CVMArrayOfChar* charArray;
    CVMkniSanityCheckStringAccess(stringHandle);
    CVMD_fieldReadRef(stringAsObject, 
		      CVMoffsetOfjava_lang_String_value,
		      valueAsObject);
    CVMD_fieldReadInt(stringAsObject, 
		      CVMoffsetOfjava_lang_String_offset,
		      str_offset);
    CVMD_fieldReadInt(stringAsObject, 
		      CVMoffsetOfjava_lang_String_count,
		      str_len);
    charArray = (CVMArrayOfChar*)valueAsObject;
    CVMD_arrayReadBodyChar(jcharbuf, charArray, str_offset + offset, n);
}

KNIEXPORT void
KNI_NewStringUTFPriv(CVMExecEnv* ee, 
		     const char* utf8chars, jstring stringHandle)
{
    CVMD_gcSafeExec(ee, {
	CVMnewStringUTF(ee, stringHandle, utf8chars);
    });
    if (CVMID_icellIsNull(stringHandle)) {
	CVMthrowOutOfMemoryError(ee, NULL);
    }
}

KNIEXPORT void
KNI_NewStringPriv(CVMExecEnv* ee,
		  const jchar* chars, jsize size, jstring stringHandle)
{
    if (chars != NULL && size >= 0) {
        CVMD_gcSafeExec(ee, {
	    CVMnewString(ee, stringHandle, chars, size);
        });
        if (CVMID_icellIsNull(stringHandle)) {
	    CVMthrowOutOfMemoryError(ee, NULL);
        }
    } else {
        CVMID_icellSetNull(stringHandle);
    }
}

/**
 * Array operations
 */

#define KNI_ARRAY_GETTER(_jtype, _type)					\
KNIEXPORT _jtype							\
KNI_Get##_type##ArrayElement(_jtype##Array array, jsize index)		\
{									\
    _jtype element;							\
    CVMArrayOf##_type* arrayObj =					\
        (CVMArrayOf##_type*)CVMID_icellDirect(CVMgetEE(), array);	\
    CVMD_arrayRead##_type(arrayObj, index, element);			\
    return element;							\
}

KNI_ARRAY_GETTER(jboolean, Boolean)
KNI_ARRAY_GETTER(jbyte, Byte)
KNI_ARRAY_GETTER(jchar, Char)
KNI_ARRAY_GETTER(jshort, Short)
KNI_ARRAY_GETTER(jint, Int)
KNI_ARRAY_GETTER(jlong, Long)
KNI_ARRAY_GETTER(jfloat, Float)
KNI_ARRAY_GETTER(jdouble, Double)

KNIEXPORT void
KNI_GetObjectArrayElement(jobjectArray array, jsize index, jobject toHandle)
{
    CVMArrayOfRef* arrayObj =
        (CVMArrayOfRef*)CVMID_icellDirect(CVMgetEE(), array);
    CVMD_arrayReadRef(arrayObj, index
		      , CVMID_icellDirect(CVMgetEE(), toHandle));
}

#define KNI_ARRAY_SETTER(_jtype, _type)					     \
KNIEXPORT void								     \
KNI_Set##_type##ArrayElement(_jtype##Array array, jsize index, _jtype value) \
{									     \
    CVMArrayOf##_type* arrayObj =					     \
        (CVMArrayOf##_type*)CVMID_icellDirect(CVMgetEE(), array);	     \
    CVMD_arrayWrite##_type(arrayObj, index, value);			     \
}

KNI_ARRAY_SETTER(jboolean, Boolean)
KNI_ARRAY_SETTER(jbyte, Byte)
KNI_ARRAY_SETTER(jchar, Char)
KNI_ARRAY_SETTER(jshort, Short)
KNI_ARRAY_SETTER(jint, Int)
KNI_ARRAY_SETTER(jlong, Long)
KNI_ARRAY_SETTER(jfloat, Float)
KNI_ARRAY_SETTER(jdouble, Double)

KNIEXPORT void
KNI_SetObjectArrayElement(jobjectArray array, jsize index, jobject fromHandle)
{
    CVMArrayOfRef* arrayObj =
        (CVMArrayOfRef*)CVMID_icellDirect(CVMgetEE(), array);
    CVMD_arrayWriteRef(arrayObj, index,
		       CVMID_icellDirect(CVMgetEE(), fromHandle));
}

#ifndef CVMGC_HAS_NONREF_BARRIERS
#ifdef CVM_DEBUG_ASSERTS
static void
KNI_VerifyRawArrayArgs(CVMArrayOfByte *arrayObj, jsize offset, jsize n)
{
    CVMClassBlock *arrayCb = CVMobjectGetClass(arrayObj);
    CVMassert(n + offset <= (arrayObj->length * CVMarrayElemSize(arrayCb)));
    switch (CVMarrayBaseType(arrayCb)) {
    case CVM_T_CHAR:
    case CVM_T_BYTE:
    case CVM_T_SHORT:
    case CVM_T_INT:
    case CVM_T_LONG:
    case CVM_T_FLOAT:
    case CVM_T_DOUBLE: {
	break;
    }
    default:
	CVMassert(CVM_FALSE);
    }
}
#endif /* CVM_DEBUG_ASSERTS */
#endif


KNIEXPORT void
KNI_GetRawArrayRegion(jarray array, jsize offset,
		      jsize n, jbyte* dstBuffer)
{
#ifndef CVMGC_HAS_NONREF_BARRIERS
    CVMArrayOfByte *arrayObj =
	(CVMArrayOfByte*) CVMID_icellDirect(CVMgetEE(), array);
#ifdef CVM_DEBUG_ASSERTS
    KNI_VerifyRawArrayArgs(arrayObj, offset, n);
#endif /* CVM_DEBUG_ASSERTS */
    CVMmemmoveByte((void*)dstBuffer, (void*)&arrayObj->elems[offset], n);

#else /* CVMGC_HAS_NONREF_BARRIERS */

#error "CVM_KNI=true not supported when there are non-ref GC barriers."
    /* TODO: The code below is broken if either srcBuffer is not aligned
       properly, or "n" is not a multiple of the array element type size.
    */
    CVMObject *obj = CVMID_icellDirect(CVMgetEE(), array);
    CVMClassBlock *arrayCb = CVMobjectGetClass(obj);

    switch (CVMarrayBaseType(arrayCb)) {
    case CVM_T_CHAR: {
	CVMArrayOfChar* arrayObj = (CVMArrayOfChar*)obj;
	CVMD_arrayReadBodyChar(dstBuffer, arrayObj, offset/2, n/2);
	break;
    }
    case CVM_T_BYTE: {
	CVMArrayOfByte* arrayObj = (CVMArrayOfByte*)obj;
	CVMD_arrayReadBodyByte(dstBuffer, arrayObj, offset, n);
	break;
    }
    case CVM_T_SHORT: {
	CVMArrayOfShort* arrayObj = (CVMArrayOfShort*)obj;
	CVMD_arrayReadBodyShort(dstBuffer, arrayObj, offset/2, n/2);
	break;
    }
    case CVM_T_INT: {
	CVMArrayOfInt* arrayObj = (CVMArrayOfInt*)obj;
	CVMD_arrayReadBodyInt(dstBuffer, arrayObj, offset/4, n/4);
	break;
    }
    case CVM_T_LONG: {
	CVMArrayOfLong* arrayObj = (CVMArrayOfLong*)obj;
	CVMD_arrayReadBodyLong(dstBuffer, arrayObj, offset/8, n/8);
	break;
    }
    case CVM_T_FLOAT: {
	CVMArrayOfFloat* arrayObj = (CVMArrayOfFloat*)obj;
	CVMD_arrayReadBodyFloat(dstBuffer, arrayObj, offset/4, n/4);
	break;
    }
    case CVM_T_DOUBLE: {
	CVMArrayOfDouble* arrayObj = (CVMArrayOfDouble*)obj;
	CVMD_arrayReadBodyDouble(dstBuffer, arrayObj, offset/8, n/8);
	break;
    }
    default:
	CVMassert(CVM_FALSE);
    }

#endif /* CVMGC_HAS_NONREF_BARRIERS */
}

KNIEXPORT void
KNI_SetRawArrayRegion(jarray array, jsize offset,
		      jsize n, const jbyte* srcBuffer)
{
#ifndef CVMGC_HAS_NONREF_BARRIERS

    CVMArrayOfByte *arrayObj =
	(CVMArrayOfByte*)CVMID_icellDirect(CVMgetEE(), array);
#ifdef CVM_DEBUG_ASSERTS
    KNI_VerifyRawArrayArgs(arrayObj, offset, n);
#endif /* CVM_DEBUG_ASSERTS */
    CVMmemmoveByte((void*)&arrayObj->elems[offset], (void*)srcBuffer, n);

#else /* CVMGC_HAS_NONREF_BARRIERS */

#error "CVM_KNI=true not supported when there are non-ref GC barriers."
    /* TODO: The code below is broken if either srcBuffer is not aligned
       properly, or "n" is not a multiple of the array element type size.
    */
    CVMObject *obj = CVMID_icellDirect(CVMgetEE(), array);
    CVMClassBlock *arrayCb = CVMobjectGetClass(obj);

    switch (CVMarrayBaseType(arrayCb)) {
    case CVM_T_CHAR: {
	CVMArrayOfChar* arrayObj = (CVMArrayOfChar*)obj;
	CVMD_arrayWriteBodyChar(srcBuffer, arrayObj, offset/2, n/2);
	break;
    }
    case CVM_T_BYTE: {
	CVMArrayOfByte* arrayObj = (CVMArrayOfByte*)obj;
	CVMD_arrayWriteBodyByte(srcBuffer, arrayObj, offset, n);
	break;
    }
    case CVM_T_SHORT: {
	CVMArrayOfShort* arrayObj = (CVMArrayOfShort*)obj;
	CVMD_arrayWriteBodyShort(srcBuffer, arrayObj, offset/2, n/2);
	break;
    }
    case CVM_T_INT: {
	CVMArrayOfInt* arrayObj = (CVMArrayOfInt*)obj;
	CVMD_arrayWriteBodyInt(srcBuffer, arrayObj, offset/4, n/4);
	break;
    }
    case CVM_T_LONG: {
	CVMArrayOfLong* arrayObj = (CVMArrayOfLong*)obj;
	CVMD_arrayWriteBodyLong(srcBuffer, arrayObj, offset/8, n/8);
	break;
    }
    case CVM_T_FLOAT: {
	CVMArrayOfFloat* arrayObj = (CVMArrayOfFloat*)obj;
	CVMD_arrayWriteBodyFloat(srcBuffer, arrayObj, offset/4, n/4);
	break;
    }
    case CVM_T_DOUBLE: {
	CVMArrayOfDouble* arrayObj = (CVMArrayOfDouble*)obj;
	CVMD_arrayWriteBodyDouble(srcBuffer, arrayObj, offset/8, n/8);
	break;
    }
    default:
	CVMassert(CVM_FALSE);
    }
#endif /* CVMGC_HAS_NONREF_BARRIERS */
}
