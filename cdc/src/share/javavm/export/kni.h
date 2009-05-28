/*
 * @(#)kni.h	1.43 06/10/10 10:03:30
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
 */

#ifndef _JAVASOFT_KNI_H_
#define _JAVASOFT_KNI_H_

/**
 * KNI is an implementation-level native function interface
 * for CLDC-category VMs.  KNI is intended to be significantly
 * more lightweight than JNI, so we have made some compromises:
 *
 * - Compile-time interface with static linking.
 * - Source-level (no binary level) compatibility.
 * - No argument marshalling. All native functions have
 *   signature void(*)(). Arguments are read explicitly,
 *   and return values are set explicitly.
 * - No invocation API (cannot call Java from native code).
 * - No class definition support.
 * - Limited object allocation support (strings only).
 * - Limited array region access for arrays of a primitive type.
 * - No monitorenter/monitorexit support.
 * - Limited error handling and exception support.
 *   KNI functions do not throw exceptions, but return error
 *   values/null instead, or go fatal for severe errors.
 * - Exceptions can be thrown explicitly, but no direct
 *   exception manipulation is supported.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "jni.h"
#include "javavm/include/localroots.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/directmem.h"

/*
 * New stuff for KNI on CNI
 */
#define KNIDECLARGS CVMExecEnv* _ee,		\
	  	    CVMStackVal32* _arguments,	\
		    CVMMethodBlock** _p_mb,
#define KNIPASSARGS _ee, _arguments, _p_mb,
/* Macro for declaring a KNI method */
#define KNIDECL(methodname)			\
    CNI##methodname(CVMExecEnv* _ee,		\
	  	    CVMStackVal32* _arguments,	\
		    CVMMethodBlock** _p_mb)

/**
 * KNI Basic Types
 *
 * Note: jbyte, jint and jlong are defined in the
 * machine-specific jni_md.h file.
 */
#if 0 /* these are all handled in jni.h */
typedef unsigned char   jboolean;
typedef unsigned short  jchar;
typedef short           jshort;
typedef float           jfloat;
typedef double          jdouble;
typedef jint            jsize;
#endif

/**
 * KNI Reference Types
 *
 * Note: jfieldID and jobject are intended to be opaque
 * types.  The programmer should not make any assumptions
 * about the actual type of these types, since the actual
 * type may vary from one KNI implementation to another.
 */
#if 0 /* these are all handled in jni.h */
struct _jobject;
typedef struct _jobject* jobject;
typedef jobject jclass;
typedef jobject jthrowable;
typedef jobject jstring;
typedef jobject jarray;
typedef jarray  jbooleanArray;
typedef jarray  jbyteArray;
typedef jarray  jcharArray;
typedef jarray  jshortArray;
typedef jarray  jintArray;
typedef jarray  jlongArray;
typedef jarray  jfloatArray;
typedef jarray  jdoubleArray;
typedef jarray  jobjectArray;
#endif

/**
 * KNI Field Type
 */
#if 0 /* these are all handled in jni.h */
struct _jfieldID;
typedef struct _jfieldID* jfieldID;
#endif

/**
 * jboolean constants
 */
#define KNI_FALSE 0
#define KNI_TRUE  1

/**
 * Return values for KNI functions.
 * Values correspond to JNI.
 */
#define KNI_OK           0                 // success
#define KNI_ERR          (-1)              // unknown error
#define KNI_ENOMEM       (-4)              // not enough memory
#define KNI_EINVAL       (-6)              // invalid arguments

#define KNIEXPORT JNIEXPORT

/*
 * Version information
 */
#define KNI_VERSION      0x00010000        // KNI version 1.0

/******************************************************************
 * KNI functions (refer to KNI Specification for details)
 ******************************************************************/

/**
 * Version information
 */
KNIEXPORT jint KNI_GetVersion();
#define KNI_GetVersion() (KNI_VERSION)

/**
 * Class and interface operations
 *
 * WARNING: KNI_FindClass can cause a gc.
 * Not sure if this is per the KNI spec.
 */
KNIEXPORT void
KNI_FindClassImpl(CVMExecEnv* ee, const char* name, jclass classHandle);
KNIEXPORT void
KNI_GetSuperClassImpl(CVMExecEnv* ee, jclass classHandle,
		      jclass superclassHandle);
KNIEXPORT jboolean
KNI_IsAssignableFromPriv(CVMExecEnv* ee, jclass classHandle1,
			 jclass classHandle2);

#define KNI_FindClass(name, classHandle) \
    KNI_FindClassImpl(_ee, name, classHandle)
#define KNI_GetSuperClass(classHandle, superclassHandle) \
    KNI_GetSuperClassImpl(_ee, classHandle, superclassHandle)
#define KNI_IsAssignableFrom(classHandle1, classHandle2) \
    KNI_IsAssignableFromPriv(_ee, classHandle1, classHandle2)

/**
 * Exceptions and errors
 *
 * WARNING: KNI_ThrowNewPriv can cause a gc.
 * Not sure if this is per the KNI spec.
 */
KNIEXPORT jint
KNI_ThrowNewImpl(CVMExecEnv* ee, const char* name, const char* message);
KNIEXPORT void
KNI_FatalErrorImpl(CVMExecEnv* ee, const char* message);

#define KNI_ThrowNew(name, message) \
    KNI_ThrowNewImpl(_ee, name, message)
#define KNI_FatalError(message) \
    KNI_FatalErrorImpl(_ee, message)

/**
 * Object operations
 */
KNIEXPORT void
KNI_GetObjectClass(jobject objectHandle, jclass classHandle);
KNIEXPORT jboolean
KNI_IsInstanceOfPriv(CVMExecEnv* ee, jobject objectHandle, jclass classHandle);
KNIEXPORT jboolean
KNI_IsSameObject(jobject obj1, jobject obj2);

#define KNI_GetObjectClass(objectHandle, classHandle) {			\
    CVMObject* classObject = CVMID_icellDirect(_ee, objectHandle);	\
    CVMClassBlock* cb = CVMobjectGetClass(classObject);			\
    CVMID_icellAssignDirect(_ee, classHandle, CVMcbJavaInstance(cb));	\
}
#define KNI_IsInstanceOf(objectHandle, classHandle) \
    KNI_IsInstanceOfPriv(_ee, objectHandle, classHandle)
#define KNI_IsSameObject(obj1, obj2)					\
    (obj1 == NULL || obj2 == NULL 					\
     ? obj1 == obj2							\
     : CVMID_icellDirect(_ee, obj1) == CVMID_icellDirect(_ee, obj2))

/**
 * Get an instance field or static field ID.
 */
KNIEXPORT jfieldID
KNI_GetFieldIDPriv(CVMExecEnv* ee, jclass classHandle, const char* name,
		   const char* signature, CVMBool isStatic);

#define KNI_GetFieldID(classHandle, name, signature) \
    KNI_GetFieldIDPriv(_ee, classHandle, name, signature, CVM_FALSE)

#define KNI_GetStaticFieldID(classHandle, name, signature) \
    KNI_GetFieldIDPriv(_ee, classHandle, name, signature, CVM_TRUE)


/**
 * Instance field access.
 */
KNIEXPORT jboolean KNI_GetBooleanField(jobject objectHandle, jfieldID fieldID);
KNIEXPORT jbyte    KNI_GetByteField(jobject objectHandle, jfieldID fieldID);
KNIEXPORT jchar    KNI_GetCharField(jobject objectHandle, jfieldID fieldID);
KNIEXPORT jshort   KNI_GetShortField(jobject objectHandle, jfieldID fieldID);
KNIEXPORT jint     KNI_GetIntField(jobject objectHandle, jfieldID fieldID);
KNIEXPORT jlong    KNI_GetLongField(jobject objectHandle, jfieldID fieldID);
KNIEXPORT jfloat   KNI_GetFloatField(jobject objectHandle, jfieldID fieldID);
KNIEXPORT jdouble  KNI_GetDoubleField(jobject objectHandle, jfieldID fieldID);
KNIEXPORT void     KNI_GetObjectField(jobject objectHandle, jfieldID fieldID,
                       jobject toHandle);

KNIEXPORT void     KNI_SetBooleanField(jobject objectHandle, jfieldID fieldID,
                       jboolean value);
KNIEXPORT void     KNI_SetByteField(jobject objectHandle, jfieldID fieldID,
                       jbyte value);
KNIEXPORT void     KNI_SetCharField(jobject objectHandle, jfieldID fieldID,
                       jchar value);
KNIEXPORT void     KNI_SetShortField(jobject objectHandle, jfieldID fieldID,
                       jshort value);
KNIEXPORT void     KNI_SetIntField(jobject objectHandle, jfieldID fieldID,
                       jint value);
KNIEXPORT void     KNI_SetLongField(jobject objectHandle, jfieldID fieldID,
                       jlong value);
KNIEXPORT void     KNI_SetFloatField(jobject objectHandle, jfieldID fieldID,
                       jfloat value);
KNIEXPORT void     KNI_SetDoubleField(jobject objectHandle, jfieldID fieldID,
                       jdouble value);
KNIEXPORT void     KNI_SetObjectField(jobject objectHandle, jfieldID fieldID,
                       jobject fromHandle);

/**
 * Static field access
 */
KNIEXPORT jboolean KNI_GetStaticBooleanField(jclass classHandle,
                       jfieldID fieldID);
KNIEXPORT jbyte    KNI_GetStaticByteField(jclass classHandle,
                       jfieldID fieldID);
KNIEXPORT jchar    KNI_GetStaticCharField(jclass classHandle,
                       jfieldID fieldID);
KNIEXPORT jshort   KNI_GetStaticShortField(jclass classHandle,
                       jfieldID fieldID);
KNIEXPORT jint     KNI_GetStaticIntField(jclass classHandle, jfieldID fieldID);
KNIEXPORT jlong    KNI_GetStaticLongField(jclass classHandle,
                       jfieldID fieldID);
KNIEXPORT jfloat   KNI_GetStaticFloatField(jclass classHandle,
                       jfieldID fieldID);
KNIEXPORT jdouble  KNI_GetStaticDoubleField(jclass classHandle,
                       jfieldID fieldID);
KNIEXPORT void     KNI_GetStaticObjectField(jclass classHandle,
                       jfieldID fieldID, jobject toHandle);

KNIEXPORT void     KNI_SetStaticBooleanField(jclass classHandle,
                       jfieldID fieldID, jboolean value);
KNIEXPORT void     KNI_SetStaticByteField(jclass classHandle,
                       jfieldID fieldID, jbyte value);
KNIEXPORT void     KNI_SetStaticCharField(jclass classHandle,
                       jfieldID fieldID, jchar value);
KNIEXPORT void     KNI_SetStaticShortField(jclass classHandle,
                       jfieldID fieldID, jshort value);
KNIEXPORT void     KNI_SetStaticIntField(jclass classHandle,
                       jfieldID fieldID, jint value);
KNIEXPORT void     KNI_SetStaticLongField(jclass classHandle,
                       jfieldID fieldID, jlong value);
KNIEXPORT void     KNI_SetStaticFloatField(jclass classHandle,
                       jfieldID fieldID, jfloat value);
KNIEXPORT void     KNI_SetStaticDoubleField(jclass classHandle,
                       jfieldID fieldID, jdouble value);
KNIEXPORT void     KNI_SetStaticObjectField(jclass classHandle,
                       jfieldID fieldID, jobject fromHandle);

/**
 * String operations
 */
KNIEXPORT jsize
KNI_GetStringLength(jstring stringHandle);
KNIEXPORT void
KNI_GetStringRegion(jstring stringHandle, jsize offset,
		    jsize n, jchar* jcharbuf);
KNIEXPORT void
KNI_NewStringUTFPriv(CVMExecEnv* ee, const char* utf8chars,
		     jstring stringHandle);
KNIEXPORT void
KNI_NewStringPriv(CVMExecEnv* ee, const jchar* chars,
		  jsize size, jstring stringHandle);

#define KNI_NewStringUTF(utf8chars, stringHandle) \
    KNI_NewStringUTFPriv(_ee, utf8chars, stringHandle)
#define KNI_NewString(chars, size, stringHandle) \
    KNI_NewStringPriv(_ee, chars, size, stringHandle)

/**
 * Array operations
 */
KNIEXPORT jsize    KNI_GetArrayLength(jarray arrayHandle);
#define KNI_GetArrayLength(arrayHandle)				\
    (CVMID_icellIsNull(arrayHandle) ? -1 :			\
     CVMD_arrayGetLength((CVMArrayOfAnyType*)			\
			 CVMID_icellDirect(_ee, arrayHandle)))

KNIEXPORT jboolean KNI_GetBooleanArrayElement(jbooleanArray arrayHandle,
                       jsize index);
KNIEXPORT jbyte    KNI_GetByteArrayElement(jbyteArray arrayHandle,
                       jsize index);
KNIEXPORT jchar    KNI_GetCharArrayElement(jcharArray arrayHandle,
                       jsize index);
KNIEXPORT jshort   KNI_GetShortArrayElement(jshortArray arrayHandle,
                       jsize index);
KNIEXPORT jint     KNI_GetIntArrayElement(jintArray arrayHandle,
                       jsize index);
KNIEXPORT jlong    KNI_GetLongArrayElement(jlongArray arrayHandle,
                       jsize index);
KNIEXPORT jfloat   KNI_GetFloatArrayElement(jfloatArray arrayHandle,
                       jsize index);
KNIEXPORT jdouble  KNI_GetDoubleArrayElement(jdoubleArray arrayHandle,
                       jsize index);
KNIEXPORT void     KNI_GetObjectArrayElement(jobjectArray arrayHandle,
                       jsize index, jobject toHandle);

KNIEXPORT void     KNI_SetBooleanArrayElement(jbooleanArray arrayHandle,
                       jsize index, jboolean value);
KNIEXPORT void     KNI_SetByteArrayElement(jbyteArray arrayHandle,
                       jsize index, jbyte value);
KNIEXPORT void     KNI_SetCharArrayElement(jcharArray arrayHandle,
                       jsize index, jchar value);
KNIEXPORT void     KNI_SetShortArrayElement(jshortArray arrayHandle,
                       jsize index, jshort value);
KNIEXPORT void     KNI_SetIntArrayElement(jintArray arrayHandle,
                       jsize index, jint value);
KNIEXPORT void     KNI_SetLongArrayElement(jlongArray arrayHandle,
                       jsize index, jlong value);
KNIEXPORT void     KNI_SetFloatArrayElement(jfloatArray arrayHandle,
                       jsize index, jfloat value);
KNIEXPORT void     KNI_SetDoubleArrayElement(jdoubleArray arrayHandle,
                       jsize index, jdouble value);
KNIEXPORT void     KNI_SetObjectArrayElement(jobjectArray arrayHandle,
                       jsize index, jobject fromHandle);

KNIEXPORT void     KNI_GetRawArrayRegion(jarray arrayHandle, jsize offset,
                       jsize n, jbyte* dstBuffer);
KNIEXPORT void     KNI_SetRawArrayRegion(jarray arrayHandle, jsize offset,
                       jsize n, const jbyte* srcBuffer);

/**
 * Parameter passing
 */

KNIEXPORT jboolean KNI_GetParameterAsBoolean(jint index);
KNIEXPORT jbyte    KNI_GetParameterAsByte(jint index);
KNIEXPORT jchar    KNI_GetParameterAsChar(jint index);
KNIEXPORT jshort   KNI_GetParameterAsShort(jint index);
KNIEXPORT jint     KNI_GetParameterAsInt(jint index);
KNIEXPORT jlong    KNI_GetParameterAsLong(jint index);
KNIEXPORT jfloat   KNI_GetParameterAsFloat(jint index);
KNIEXPORT jdouble  KNI_GetParameterAsDouble(jint index);
KNIEXPORT void     KNI_GetParameterAsObject(jint index, jobject toHandle);

#define KNIARG(index) \
   (_arguments[index + (CVMmbIs(*_p_mb, STATIC) ? -1 : 0)].j)

#define KNI_GetParameterAsBoolean(index) ((jboolean)KNIARG(index).i)
#define KNI_GetParameterAsByte(index)    ((jbyte)KNIARG(index).i)
#define KNI_GetParameterAsChar(index)    ((jchar)KNIARG(index).i)
#define KNI_GetParameterAsShort(index)   ((jshort)KNIARG(index).i)
#define KNI_GetParameterAsInt(index)     ((jint)KNIARG(index).i)
#define KNI_GetParameterAsFloat(index)   ((jfloat)KNIARG(index).f)

KNIEXPORT jlong
KNI_GetParameterAsLongPriv(CVMJavaVal64* val64);
KNIEXPORT jdouble
KNI_GetParameterAsDoublePriv(CVMJavaVal64* val64);
#define KNI_GetParameterAsLong(index) \
    CVMjvm2Long((CVMUint32*)&(KNIARG(index)))
#define KNI_GetParameterAsDouble(index) \
    CVMjvm2Double((CVMUint32*)&(KNIARG(index)))

#define KNI_GetParameterAsObject(index, toHandle)			\
    CVMID_icellSetDirect(_ee, toHandle,					\
			 CVMID_icellDirect(_ee, &KNIARG(index).r));

KNIEXPORT void     KNI_GetThisPointer(jobject toHandle);
KNIEXPORT void     KNI_GetClassPointer(jclass toHandle);
#define KNI_GetThisPointer(toHandle)					\
   CVMID_icellSetDirect(_ee, toHandle,					\
			CVMID_icellDirect(_ee, &_arguments[0].j.r))
#define KNI_GetClassPointer(toHandle)				\
{								\
    jclass clazz_ = CVMcbJavaInstance(CVMmbClassBlock(*_p_mb));	\
    CVMID_icellSetDirect(_ee, toHandle,				\
			 CVMID_icellDirect(_ee, clazz_));	\
}

#define KNI_CheckAndReturn(cniCode)		\
    if (CVMexceptionOccurred(_ee)) {		\
	return CNI_EXCEPTION;			\
    } else {					\
	return cniCode;				\
    }

#define            KNI_ReturnVoid()         KNI_CheckAndReturn(CNI_VOID);
#define            KNI_ReturnBoolean(value) KNI_ReturnInt((jboolean)value)
#define            KNI_ReturnByte(value)    KNI_ReturnInt((jbyte)value)
#define            KNI_ReturnChar(value)    KNI_ReturnInt((jchar)value)
#define            KNI_ReturnShort(value)   KNI_ReturnInt((jshort)value)
#define            KNI_ReturnInt(value)		\
    _arguments[0].j.i = (jint)(value);		\
    KNI_CheckAndReturn(CNI_SINGLE);
#define            KNI_ReturnFloat(value)	\
    _arguments[0].j.f = (jfloat)(value);	\
    KNI_CheckAndReturn(CNI_SINGLE);
#define            KNI_ReturnLong(value)	       	\
    CVMlong2Jvm((CVMUint32*)&_arguments[0].j, value);	\
    KNI_CheckAndReturn(CNI_DOUBLE);
#define            KNI_ReturnDouble(value)		\
    CVMdouble2Jvm((CVMUint32*)&_arguments[0].j, value);	\
    KNI_CheckAndReturn(CNI_DOUBLE);

/**
 * Handle operations
 */
#define KNI_StartHandles(n)				\
    CVMID_localrootBeginGcUnsafe(_ee) {			\
        int _dummy /* eat up the trailing semicolon */

/* jobject and  CVMObjectICell* are the same, which is why this works */
#define KNI_DeclareHandle(x) \
    CVMID_localrootDeclareGcUnsafe(CVMObjectICell, x)

#define KNI_IsNullHandle(x) \
    CVMID_icellIsNull(x)

#define KNI_ReleaseHandle(x) \
    CVMID_icellSetNull(x)

#define KNI_EndHandles()				\
        (void)_dummy; /* get rid of compiler warning */	\
    } CVMID_localrootEndGcUnsafe();

#define KNI_EndHandlesAndReturnObject(x)			\
    CVMID_icellAssignDirect(_ee, &_arguments[0].j.r, x);	\
    KNI_EndHandles();						\
    KNI_CheckAndReturn(CNI_SINGLE);

/**
 * Type macros
 */
#define KNI_RETURNTYPE_VOID    CNIResultCode
#define KNI_RETURNTYPE_BOOLEAN CNIResultCode
#define KNI_RETURNTYPE_BYTE    CNIResultCode
#define KNI_RETURNTYPE_CHAR    CNIResultCode
#define KNI_RETURNTYPE_SHORT   CNIResultCode
#define KNI_RETURNTYPE_INT     CNIResultCode
#define KNI_RETURNTYPE_FLOAT   CNIResultCode
#define KNI_RETURNTYPE_LONG    CNIResultCode
#define KNI_RETURNTYPE_DOUBLE  CNIResultCode
#define KNI_RETURNTYPE_OBJECT  CNIResultCode

//
// This definition has been added for KVM compatibility.
// It is not part of the KNI Specification.
//
#define KNI_registerCleanup(instanceHandle, callback)

#ifdef __cplusplus
}
#endif

#endif /* !_JAVASOFT_KNI_H_ */

