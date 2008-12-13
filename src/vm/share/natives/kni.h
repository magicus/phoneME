/*
 *   
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

#ifndef _JVM_CONFIG_H_
#include "jvmconfig.h"
#endif

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

/*
 * macros to be compatible with CDC-VM
 */
#define KNIDECLARGS 
#define KNIPASSARGS 
#define KNIDECL(methodname) Java_##methodname()

#ifndef KNI_PRIMITIVE_TYPES

#define KNI_PRIMITIVE_TYPES

#include "kni_md.h"

/**
 * KNI Basic Types
 *
 * Note: jbyte, jint and jlong are defined in the
 * machine-specific kni_md.h file.
 */
typedef unsigned char   jboolean;
typedef unsigned short  jchar;
typedef short           jshort;
typedef float           jfloat;
typedef double          jdouble;
typedef jint            jsize;

#endif // KNI_PRIMITIVE_TYPES

/**
 * KNI Reference Types
 *
 * Note: jfieldID and jobject are intended to be opaque
 * types.  The programmer should not make any assumptions
 * about the actual type of these types, since the actual
 * type may vary from one KNI implementation to another.
 */
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

/**
 * KNI Field Type
 */
struct _jfieldID;
typedef struct _jfieldID* jfieldID;

/**
 * jboolean constants
 */
#define KNI_FALSE 0
#define KNI_TRUE  1

/**
 * Return values for KNI functions.
 * Values correspond to JNI.
 */
#define KNI_OK           0                 /* success */
#define KNI_ERR          (-1)              /* unknown error */
#define KNI_ENOMEM       (-4)              /* not enough memory */
#define KNI_EINVAL       (-6)              /* invalid arguments */

#ifdef __cplusplus
# define KNIEXPORT extern "C"
#else  /* __cplusplus */
# define KNIEXPORT extern
#endif /* __cplusplus */

/*
 * Version information
 */
#define KNI_VERSION      0x00010000        /* KNI version 1.0 */

/******************************************************************
 * KNI functions (refer to KNI Specification for details)
 ******************************************************************/

/**
 * Version information
 */
KNIEXPORT jint KNI_GetVersion();

/**
 * Class and interface operations
 */
KNIEXPORT void     KNI_FindClass(const char* name, jclass classHandle);
KNIEXPORT void     KNI_GetSuperClass(jclass classHandle,
                       jclass superclassHandle);
KNIEXPORT jboolean KNI_IsAssignableFrom(jclass classHandle1,
                       jclass classHandle2);

/**
 * Exceptions and errors
 */
KNIEXPORT jint     KNI_ThrowNew(const char* name, const char* message);
KNIEXPORT void     KNI_FatalError(const char* message);

/**
 * Object operations
 */
KNIEXPORT void     KNI_GetObjectClass(jobject objectHandle,
                       jclass classHandle);
KNIEXPORT jboolean KNI_IsInstanceOf(jobject objectHandle, jclass classHandle);
KNIEXPORT jboolean KNI_IsSameObject(jobject obj1, jobject obj2);

/**
 * Instance field access.
 *
 * Note specific to this VM: KNI_GetFieldID() must not be applied 
 * on a class whose 
 * fields have been renamed by the romizer. You need to use
 * the DontRenameNonPublicFields flag in your romizer configuration
 * file. See src/vm/cldc_rom.cfg in this VM source distribution
 * for an example.
 */
KNIEXPORT jfieldID KNI_GetFieldID(jclass classHandle, const char* name,
                       const char* signature);

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
KNIEXPORT jfieldID KNI_GetStaticFieldID(jclass classHandle, const char* name,
                       const char* signature);

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
KNIEXPORT jsize    KNI_GetStringLength(jstring stringHandle);
KNIEXPORT void     KNI_GetStringRegion(jstring stringHandle, jsize offset,
                       jsize n, jchar* jcharbuf);
KNIEXPORT void     KNI_NewStringUTF(const char* utf8chars,
                       jstring stringHandle);
KNIEXPORT void     KNI_NewString(const jchar*, jsize, jstring);

/**
 * Array operations
 */
KNIEXPORT jsize    KNI_GetArrayLength(jarray array);

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

extern unsigned char * _kni_parameter_base;

KNIEXPORT jboolean KNI_GetParameterAsBoolean(jint index);
KNIEXPORT jbyte    KNI_GetParameterAsByte(jint index);
KNIEXPORT jchar    KNI_GetParameterAsChar(jint index);
KNIEXPORT jshort   KNI_GetParameterAsShort(jint index);
KNIEXPORT jint     KNI_GetParameterAsInt(jint index);
KNIEXPORT jlong    KNI_GetParameterAsLong(jint index);
KNIEXPORT jfloat   KNI_GetParameterAsFloat(jint index);
KNIEXPORT jdouble  KNI_GetParameterAsDouble(jint index);
KNIEXPORT void     KNI_GetParameterAsObject(jint index, jobject toHandle);

KNIEXPORT void     KNI_GetThisPointer(jobject toHandle);
KNIEXPORT void     KNI_GetClassPointer(jclass toHandle);

#define            KNI_ReturnVoid()         return
#define            KNI_ReturnBoolean(value) return ((jint)((value) ? 1 : 0))
#define            KNI_ReturnByte(value)    return ((jint)((jbyte)(value)))
#define            KNI_ReturnChar(value)    return ((jint)((jchar)(value)))
#define            KNI_ReturnShort(value)   return ((jint)((jshort)(value)))
#define            KNI_ReturnInt(value)     return ((jint)(value))
#define            KNI_ReturnFloat(value)   return ((jfloat)(value))
#define            KNI_ReturnLong(value)    return ((jlong)(value))
#define            KNI_ReturnDouble(value)  return ((jdouble)(value))

typedef struct __KNI_HandleInfo {
  struct __KNI_HandleInfo *prev;  /* A _KNI_HandleInfo that nests
                                   * this one. */
  int total_count;                /* Number of total handles */
  int declared_count;             /* Number of handles that has been 
                                   * declared using KNI_DeclareHandle() */
  jobject *handles;               /* Points to __handles__ in this scope */
} _KNI_HandleInfo;

/*
 * Internal functions, do not use directly.
 */
KNIEXPORT int      _KNI_push_handles(int, _KNI_HandleInfo*, jobject*);
KNIEXPORT void     _KNI_pop_handles(_KNI_HandleInfo*);

/**
 * Handle operations
 */
#define KNI_StartHandles(n) \
{ \
  _KNI_HandleInfo __handle_info__; \
  jobject __handles__[n]; \
  int __dummy__ = _KNI_push_handles(n, &__handle_info__, __handles__)

#define KNI_DeclareHandle(x) \
  jobject x = (jobject)(void*)&__handles__[__handle_info__.declared_count++]

#define KNI_IsNullHandle(x) \
  (*(jobject*)x == 0)

#define KNI_ReleaseHandle(x) \
  *(jobject*)x = 0

#define KNI_EndHandles() \
  (void)_KNI_pop_handles(&__handle_info__); \
  (void)__dummy__; \
}

#define KNI_EndHandlesAndReturnObject(x) \
    (void)_KNI_pop_handles(&__handle_info__); \
    (void)__dummy__; \
    return *(jobject*)x; \
}

#ifdef __ARMCC_VERSION
/*
 * For ADS, we need to declare __softfp for all floating point Java
 * native methods, as well as all C floating point routines (such as
 * jvm_fadd) that are called directly from the interpreter, which
 * always uses integer registers for passing parameters and return
 * values.
 */
#define JVM_SOFTFP_LINKAGE __softfp
#else
/*
 * Change this if your C/C++ compiler uses a special keyword to indicate
 * floating point linkage.
 */
#define JVM_SOFTFP_LINKAGE
#endif

/**
 * Type macros
 */
#define KNI_RETURNTYPE_VOID    void
#define KNI_RETURNTYPE_BOOLEAN jint
#define KNI_RETURNTYPE_BYTE    jint
#define KNI_RETURNTYPE_CHAR    jint
#define KNI_RETURNTYPE_SHORT   jint
#define KNI_RETURNTYPE_INT     jint
#define KNI_RETURNTYPE_FLOAT   JVM_SOFTFP_LINKAGE jfloat
#define KNI_RETURNTYPE_LONG    jlong
#define KNI_RETURNTYPE_DOUBLE  JVM_SOFTFP_LINKAGE jdouble
#define KNI_RETURNTYPE_OBJECT  jobject

#ifdef __cplusplus
}
#endif

#endif /* !_JAVASOFT_KNI_H_ */
