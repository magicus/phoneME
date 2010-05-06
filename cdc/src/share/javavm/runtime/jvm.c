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

/* Implement jvm.h functions here, unless they more suitably belong
 * to somewhere else.
 */
#include "javavm/export/jvm.h"
#include "javavm/export/jni.h"
#include "native/common/jni_util.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/gc_common.h"
#include "javavm/include/porting/threads.h"
#include "javavm/include/porting/time.h"
#include "javavm/include/porting/int.h"
#include "javavm/include/porting/doubleword.h"
#include "javavm/include/clib.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/globalroots.h"
#include "javavm/include/localroots.h"
#include "javavm/include/preloader.h"
#include "javavm/include/common_exceptions.h"
#include "generated/offsets/java_lang_Class.h"
#include "generated/offsets/java_lang_Thread.h"
#include "generated/offsets/java_lang_Throwable.h"
#ifndef CDC_10
#include "javavm/include/javaAssertions.h"
#include "generated/offsets/java_lang_StackTraceElement.h"
#endif
#include "generated/javavm/include/build_defs.h"

#ifdef CVM_JVMTI
#include "javavm/include/jvmtiExport.h"
#endif

#ifdef CVM_JVMPI
#include "javavm/include/jvmpi_impl.h"
#endif

#ifdef CVM_REFLECT
# include "javavm/include/reflect.h"
#endif

#ifdef CVM_DYNAMIC_LINKING
# include "javavm/include/porting/linker.h"
#endif

#ifdef CVM_JIT
#include "javavm/include/jit_common.h"
#endif

#ifdef JAVASE
#include "javavm/include/se_init.h"
#endif

JNIEXPORT jint JNICALL
JVM_GetInterfaceVersion(void)
{
    return JVM_INTERFACE_VERSION;
}

JNIEXPORT jint JNICALL
JVM_GetLastErrorString(char *buf, int len)
{
    return CVMioGetLastErrorString(buf, len);
}

/*
 * Find primitive classes
 */
JNIEXPORT jclass JNICALL
JVM_FindPrimitiveClass(JNIEnv *env, const char *utf)
{
    CVMExecEnv*    ee  = CVMjniEnv2ExecEnv(env);
    CVMClassTypeID tid;
    CVMClassBlock* cb;

    /*
     * We need to use the "new" function, because the "lookup" function
     * is only valid when we know that the type already exists.
     */
    tid = CVMtypeidNewClassID(ee, utf, (int)strlen(utf));

    if (tid == CVM_TYPEID_ERROR) {
	/* CVMtypeidNewClassID() will always succeed if the type already
	 * exist, so it must not be primitive.
	 */
	CVMclearLocalException(ee);
	return NULL;
    }

    cb = CVMpreloaderLookupPrimitiveClassFromType(tid);
    CVMtypeidDisposeClassID(ee, tid);

    if (cb != NULL) {
	return (*env)->NewLocalRef(env, CVMcbJavaInstance(cb));
    } else {
	return NULL;
    }
}

/*
 * Link the class
 */
JNIEXPORT void JNICALL
JVM_ResolveClass(JNIEnv *env, jclass cls)
{
#ifdef CVM_CLASSLOADING
    CVMClassBlock* cb =
	CVMgcSafeClassRef2ClassBlock(CVMjniEnv2ExecEnv(env), cls);
    CVMclassLink(CVMjniEnv2ExecEnv(env), cb, CVM_FALSE);
#else
    return; /* the class must be romized and is therefore linked */
#endif
}

JNIEXPORT jclass JNICALL
JVM_FindClassFromClassLoader(JNIEnv *env, const char *name, jboolean init,
			     jobject loader, jboolean throwError)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock* cb =
	CVMclassLookupByNameFromClassLoader(ee, name, init, 
					    loader, NULL, throwError);

    if (cb != NULL) {
	return (*env)->NewLocalRef(env, CVMcbJavaInstance(cb));
    }
    return NULL;
}

JNIEXPORT jclass JNICALL
JVM_DefineClass(JNIEnv *env, const char *name, jobject loaderRef,
		const jbyte *buf, jsize bufLen, jobject pd)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    return CVMdefineClass(ee, name, loaderRef, (CVMUint8*)buf, bufLen, pd,
			  CVM_FALSE);
}

/*
 * Reflection support functions
 */

JNIEXPORT jstring JNICALL
JVM_GetClassName(JNIEnv* env, jclass cls)
{
    /* Try to avoid 1 malloc */
    char buf[128];
    char* className = buf;
    /* Consider caching the classname String with the class. */
    CVMClassBlock* cb = CVMgcSafeClassRef2ClassBlock(CVMjniEnv2ExecEnv(env),
						     cls);
    jobject res;

    if (!CVMtypeidClassNameToCString(CVMcbClassName(cb), buf, 128)) {
	className = 
	    CVMtypeidClassNameToAllocatedCString(CVMcbClassName(cb));
    }
    TranslateFromVMClassName(className);
    res = (*env)->NewLocalRef(env, (*env)->NewStringUTF(env, className));
    if (className != buf)
	free(className);
    return res;
}


JNIEXPORT jobjectArray JNICALL
JVM_GetClassInterfaces(JNIEnv *env, jclass cls)
{
#ifdef CVM_REFLECT
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    jobject obj = CVMjniCreateLocalRef(ee);
    if (obj == NULL) { /* exception thrown */
	return NULL;
    }
    CVMreflectInterfaces(ee,
			 CVMgcSafeClassRef2ClassBlock(ee, cls),
			 (CVMArrayOfRefICell*) obj);
    if (CVMexceptionOccurred(ee)) {
	CVMjniDeleteLocalRef(env, obj);
	return NULL;
    }
    return obj;
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(CVMjniEnv2ExecEnv(env), NULL);
    return NULL;
#endif /* CVM_REFLECT */
}

JNIEXPORT jobject JNICALL
JVM_GetClassLoader(JNIEnv *env, jclass cls)
{
#ifdef CVM_CLASSLOADING
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock* cb = CVMgcSafeClassRef2ClassBlock(ee, cls);
    return (*env)->NewLocalRef(env, CVMcbClassLoader(cb));
#else
    return NULL;
#endif
}

JNIEXPORT jboolean JNICALL
JVM_IsInterface(JNIEnv *env, jclass cls)
{
    CVMClassBlock* cb = CVMgcSafeClassRef2ClassBlock(CVMjniEnv2ExecEnv(env),
						     cls);
    return CVMcbIs(cb, INTERFACE);
}

#ifdef JDK12
/*
 * Get the class's signers, if any. Make sure to return a new array,
 * not the original. Returning the original is a security hole as
 * the caller could modify it.
 */
JNIEXPORT jobjectArray JNICALL
JVM_GetClassSigners(JNIEnv *env, jclass cls)
{
    Hjava_lang_Class *cb = (Hjava_lang_Class *)DeRef(env, cls);
    HArrayOfObject *copy, *orig;
    HObject **copy_data, **orig_data;
    int i;
    size_t n;

    orig = (HArrayOfObject *)  unhand(cb)->signers;
    if (orig == NULL) 
	return NULL;

    n = obj_length(orig);

    copy = (HArrayOfObject *)ArrayAlloc(T_CLASS, n);
    if (copy == NULL) {
	ThrowOutOfMemoryError(CVMjniEnv2ExecEnv(env), 0);
	return NULL;
    }

    copy_data = (HObject **) unhand(copy)->body;
    orig_data = (HObject **) unhand(orig)->body;

    /* Use i <= n, NOT i < n, to get copy_data[n] 
       which contains the type of objects in the array */

    for (i=0; i <= n; i++) {
	copy_data[i] = orig_data[i];
    }

    KEEP_POINTER_ALIVE(copy_data);
    KEEP_POINTER_ALIVE(orig_data);

    return MkRefLocal(env, copy);
}

JNIEXPORT void JNICALL
JVM_SetClassSigners(JNIEnv *env, jclass cls, jobjectArray signers)
{
    ClassClass *cb = (ClassClass *)DeRef(env, cls);
    unhand(cb)->signers = (HArrayOfObject *)DeRef(env, signers);
}
#endif

JNIEXPORT jobject JNICALL
JVM_GetProtectionDomain(JNIEnv *env, jclass cls)
{
#ifdef CVM_CLASSLOADING
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock* cb;
    cb = CVMgcSafeClassRef2ClassBlock(ee, cls);
    return (*env)->NewLocalRef(env, CVMcbProtectionDomain(cb));
#else
    return NULL;
#endif
}


#ifdef CVM_HAVE_DEPRECATED
JNIEXPORT void JNICALL
JVM_SetProtectionDomain(JNIEnv *env, jclass cls, jobject pd)
{
#ifdef CVM_CLASSLOADING
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock* cb = CVMgcSafeClassRef2ClassBlock(ee, cls);

    /* Allocate a global root for the protection domain if necessary. */
    if (CVMcbProtectionDomain(cb) == NULL) {
	CVMcbProtectionDomain(cb) = CVMID_getProtectionDomainGlobalRoot(ee);
	if (CVMcbProtectionDomain(cb) == NULL) {
	    CVMthrowOutOfMemoryError(ee, NULL);
	    return;
	}
    }

    CVMID_icellAssign(ee, CVMcbProtectionDomain(cb), pd);
#endif
}
#endif

JNIEXPORT jboolean JNICALL
JVM_IsArrayClass(JNIEnv *env, jclass cls)
{
    CVMClassBlock* cb = CVMgcSafeClassRef2ClassBlock(CVMjniEnv2ExecEnv(env),
						     cls);
    return CVMisArrayClass(cb);
}

JNIEXPORT jboolean JNICALL
JVM_IsPrimitiveClass(JNIEnv *env, jclass cls)
{
    CVMClassBlock* cb = CVMgcSafeClassRef2ClassBlock(CVMjniEnv2ExecEnv(env),
						     cls);
    return CVMcbIs(cb, PRIMITIVE);
}

JNIEXPORT jclass JNICALL
JVM_GetComponentType(JNIEnv *env, jclass cls)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock* cb = CVMgcSafeClassRef2ClassBlock(ee, cls);
    CVMClassBlock* elementCb;
    if (!CVMisArrayClass(cb)) {
	return NULL;
    }

    elementCb = CVMarrayElementCb(cb);
    CVMassert(elementCb != NULL);

    return (*env)->NewLocalRef(env, CVMcbJavaInstance(elementCb));
}

#ifdef JAVASE
JNIEXPORT jint JNICALL
JVM_GetClassAccessFlags(JNIEnv *env, jclass cls)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock* cb = CVMgcSafeClassRef2ClassBlock(ee, cls);
    jint access = CVMcbAccessFlags(cb);

    /* Remove the CVM internal bits: */
    access &= ~(CVM_CLASS_ACC_PRIMITIVE | CVM_CLASS_ACC_FINALIZABLE |
        CVM_CLASS_ACC_REFERENCE);
    return access; 
}
#endif

/* Remember that ACC_SUPER is added by the VM, and must be stripped. */
JNIEXPORT jint JNICALL
JVM_GetClassModifiers(JNIEnv *env, jclass cls)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock* cb = CVMgcSafeClassRef2ClassBlock(ee, cls);
    CVMUint32 icount = CVMcbInnerClassesInfoCount(cb);

    /* Check if this class happens to be a member class. */
    if (icount != 0) {
	CVMUint32 i;
        CVMClassTypeID classID = CVM_TYPEID_ERROR;
	for (i = 0; i < icount; i++) {
	    CVMConstantPool* cp = CVMcbConstantPool(cb);
	    CVMUint16 classIdx = CVMcbInnerClassInfo(cb, i)->innerClassIndex;
	    CVMClassBlock* innerCb;
#ifdef CVM_CLASSLOADING
            if (CVMcpCheckResolvedAndGetTID(ee, cb, cp, classIdx, &classID)) {
		innerCb = CVMcpGetCb(cp, classIdx);
	    } else {
		innerCb = NULL;
	    }
#else
	    innerCb = CVMcpGetCb(cp, classIdx);
#endif
	    if (cb == innerCb || CVMcbClassName(cb) == classID) {
		/* Aha, this is really a member class. */
		jint access =
		    CVMcbInnerClassInfo(cb, i)->innerClassAccessFlags;
		/* Unlike CVMcbAccessFlags(), these access flags are 
		 * not modifed so they will fit in one bytes. Therefore,
		 * no translation is necessary.
		 */
		return (access & (~JVM_ACC_SUPER));
	    }
	}
    }

    /* If we get here, then the class is not a member class. */

    /* The implementation of JVM_GetClassModifiers() needs to conform to the
       specification of Class.getModifiers().  Acording to that spec, the
       only bits of relevance to this method are public, final, interface,
       and abstract.  The spec also mentioned private, protected, and static
       but those don't exist in the VM spec for class access flags.
    */
    {
	jint access = CVMcbAccessFlags(cb);

	CVMassert(JVM_ACC_PUBLIC     == CVM_CLASS_ACC_PUBLIC);
	CVMassert(JVM_ACC_FINAL      == CVM_CLASS_ACC_FINAL);
	CVMassert(JVM_ACC_INTERFACE  == CVM_CLASS_ACC_INTERFACE);
	CVMassert(JVM_ACC_ABSTRACT   == CVM_CLASS_ACC_ABSTRACT);

	access &= (JVM_ACC_PUBLIC | JVM_ACC_FINAL | JVM_ACC_INTERFACE |
		   JVM_ACC_ABSTRACT);
	return access;
    }
}

/*
 * NOTE: The following routines are conditionally compiled. These can
 * not simply be removed from the build, since java.lang.Class refers
 * to them and we don't have JavaFilter incorporated yet. Therefore
 * they fail by calling CVMthrowUnsupportedOperationException when
 * reflection is not configured in.
 * JVM_GetClassFields
 * JVM_GetClassMethods
 * JVM_GetClassConstructors
 * JVM_GetClassField
 * JVM_GetClassMethod
 * JVM_GetClassConstructor
 * JVM_GetDeclaredClasses
 * JVM_GetDeclaringClass
 */

JNIEXPORT jobjectArray JNICALL
JVM_GetClassFields(JNIEnv *env, jclass cls, jint which)
{
#ifdef CVM_REFLECT
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    jobject obj = CVMjniCreateLocalRef(ee);
    if (obj == NULL) /* exception thrown */
	return NULL;
    CVMreflectFields(ee, CVMgcSafeClassRef2ClassBlock(ee, cls),
		     which, (CVMArrayOfRefICell*) obj);
    if (CVMexceptionOccurred(ee)) {
	CVMjniDeleteLocalRef(env, obj);
	return NULL;
    }
    return obj;
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(CVMjniEnv2ExecEnv(env), NULL);
    return NULL;
#endif /* CVM_REFLECT */
}

JNIEXPORT jobjectArray JNICALL
JVM_GetClassMethods(JNIEnv *env, jclass cls, jint which)
{
#ifdef CVM_REFLECT
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    jobject obj = CVMjniCreateLocalRef(ee);
    if (obj == NULL) /* exception thrown */
	return NULL;
    CVMreflectMethods(ee, CVMgcSafeClassRef2ClassBlock(ee, cls),
		      which, (CVMArrayOfRefICell*) obj);
    if (CVMexceptionOccurred(ee)) {
	CVMjniDeleteLocalRef(env, obj);
	return NULL;
    }
    return obj;
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(CVMjniEnv2ExecEnv(env), NULL);
    return NULL;
#endif /* CVM_REFLECT */
}

JNIEXPORT jobjectArray JNICALL
JVM_GetClassConstructors(JNIEnv *env, jclass cls, jint which)
{
#ifdef CVM_REFLECT
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    jobject obj = CVMjniCreateLocalRef(ee);
    if (obj == NULL) /* exception thrown */
	return NULL;
    CVMreflectConstructors(ee, CVMgcSafeClassRef2ClassBlock(ee, cls),
			   which, (CVMArrayOfRefICell*) obj);
    if (CVMexceptionOccurred(ee)) {
	CVMjniDeleteLocalRef(env, obj);
	return NULL;
    }
    return obj;
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(CVMjniEnv2ExecEnv(env), NULL);
    return NULL;
#endif /* CVM_REFLECT */
}

JNIEXPORT jobject JNICALL
JVM_GetClassField(JNIEnv *env, jclass cls, jstring name, jint which)
{
#ifdef CVM_REFLECT
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    jobject obj;
    const char *nameChars;
    
    if (name == NULL) {
	CVMthrowNullPointerException(ee, NULL);
	return NULL;
    }

    obj = CVMjniCreateLocalRef(ee);
    if (obj == NULL) /* exception thrown */
	return NULL;

    nameChars = (*env)->GetStringUTFChars(env, name, NULL);
    if (nameChars == NULL) {
	(*env)->DeleteLocalRef(env, obj);
	return NULL;
    }

    CVMreflectField(ee, CVMgcSafeClassRef2ClassBlock(ee, cls),
		    nameChars, which, (CVMObjectICell*) obj);
    (*env)->ReleaseStringUTFChars(env, name, nameChars);
    if (CVMexceptionOccurred(ee)) {
	CVMjniDeleteLocalRef(env, obj);
	return NULL;
    }
    return obj;
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(CVMjniEnv2ExecEnv(env), NULL);
    return NULL;
#endif /* CVM_REFLECT */
}

JNIEXPORT jobject JNICALL
JVM_GetClassMethod(JNIEnv *env, jclass cls, jstring name, jobjectArray types,
		   jint which)
{
#ifdef CVM_REFLECT
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    jobject obj;
    const char *nameChars;

    if (name == NULL) {
	CVMthrowNullPointerException(ee, NULL);
	return NULL;
    }
    
    obj = CVMjniCreateLocalRef(ee);
    if (obj == NULL) /* exception thrown */
	return NULL;

    nameChars = (*env)->GetStringUTFChars(env, name, NULL);
    if (nameChars == NULL) {
	(*env)->DeleteLocalRef(env, obj);
	return NULL;
    }

    CVMreflectMethod(ee, CVMgcSafeClassRef2ClassBlock(ee, cls),
		     nameChars, (CVMArrayOfRefICell*) types,
		     which, (CVMObjectICell*) obj);
    (*env)->ReleaseStringUTFChars(env, name, nameChars);
    if (CVMexceptionOccurred(ee)) {
	CVMjniDeleteLocalRef(env, obj);
	return NULL;
    }
    return obj;
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(CVMjniEnv2ExecEnv(env), NULL);
    return NULL;
#endif /* CVM_REFLECT */
}

JNIEXPORT jobject JNICALL
JVM_GetClassConstructor(JNIEnv *env, jclass cls, jobjectArray types,
			jint which)
{
#ifdef CVM_REFLECT
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    jobject obj = CVMjniCreateLocalRef(ee);
    if (obj == NULL) /* exception thrown */
	return NULL;
    CVMreflectConstructor(ee, CVMgcSafeClassRef2ClassBlock(ee, cls),
			  (CVMArrayOfRefICell*) types,
			  which, (CVMObjectICell*) obj);
    if (CVMexceptionOccurred(ee)) {
	CVMjniDeleteLocalRef(env, obj);
	return NULL;
    }
    return obj;
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(CVMjniEnv2ExecEnv(env), NULL);
    return NULL;
#endif /* CVM_REFLECT */
}

JNIEXPORT jobjectArray JNICALL
JVM_GetDeclaredClasses(JNIEnv *env, jclass ofClass)
{
#ifdef CVM_REFLECT
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock* cb = CVMgcSafeClassRef2ClassBlock(ee, ofClass);
    jobject obj = CVMjniCreateLocalRef(ee);
    if (obj == NULL)
	return NULL;
    CVMreflectInnerClasses(ee, cb, (CVMArrayOfRefICell*) obj);
    if (CVMexceptionOccurred(ee)) {
	CVMjniDeleteLocalRef(env, obj);
	return NULL;
    }
    return obj;
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(CVMjniEnv2ExecEnv(env), NULL);
    return NULL;
#endif /* CVM_REFLECT */
}

JNIEXPORT jclass JNICALL
JVM_GetDeclaringClass(JNIEnv *env, jclass ofClass)
{
#ifdef CVM_REFLECT
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock* cb = CVMgcSafeClassRef2ClassBlock(ee, ofClass);
    CVMClassBlock* parentCb;

    parentCb = CVMreflectGetDeclaringClass(ee, cb);
    if (parentCb == NULL) {
	return NULL;
    }

    return (*env)->NewLocalRef(env, CVMcbJavaInstance(parentCb));
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(CVMjniEnv2ExecEnv(env), NULL);
    return NULL;
#endif /* CVM_REFLECT */
}

JNIEXPORT jclass JNICALL
JVM_GetCallerClass(JNIEnv *env, int n)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock* cb = CVMgetCallerClass(ee, n);
    if (cb == NULL) {
	return NULL;
    } else {
	return (*env)->NewLocalRef(env, CVMcbJavaInstance(cb));
    }
}

#ifndef CDC_10
/* Assertion support. */

JNIEXPORT jboolean JNICALL
JVM_DesiredAssertionStatus(JNIEnv *env, jclass unused, jclass cls)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock* cb;
    char* classname;
    jboolean result;
    CVMassert(cls != NULL);
    cb = CVMgcSafeClassRef2ClassBlock(ee, cls);
    CVMassert(!CVMcbIs(cb, PRIMITIVE));
    classname = CVMtypeidClassNameToAllocatedCString(CVMcbClassName(cb));
    if (classname == NULL) {
	CVMthrowOutOfMemoryError(ee, NULL);
	return CVM_FALSE;
    }
    result = CVMJavaAssertions_enabled(classname,
				       CVMcbClassLoader(cb) == NULL);
    free(classname);
    return result;
}


/* Return a new AssertionStatusDirectives object with the fields filled in with
   command-line assertion arguments (i.e., -ea, -da). */
JNIEXPORT jobject JNICALL
JVM_AssertionStatusDirectives(JNIEnv *env, jclass unused)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    jobject asd = CVMJavaAssertions_createAssertionStatusDirectives(ee);
    if (CVMlocalExceptionOccurred(ee)) {
	return NULL;
    }
    return asd;
}
#endif /* !CDC_10 */

/* Note: JVM_NewInstance is not needed in CVM because Class.newInstance
   is implemented in Java. */

/*
 * Called from System.c
 */

JNIEXPORT jlong JNICALL
JVM_CurrentTimeMillis(JNIEnv *env, jclass ignored)
{
    return CVMtimeMillis();
}

/*
 * Copies an array of refs to another array of refs where an assignability
 * check is required for every element copied to the destination array.
 */
void
CVMcopyRefArrays(CVMExecEnv* ee,
		 CVMArrayOfRef* srcArr, jint src_pos,
		 CVMArrayOfRef* dstArr, jint dst_pos,
		 CVMClassBlock* dstElemCb, jint length)
{
    CVMJavaInt i, j;
    if ((dstArr != srcArr) || (dst_pos < src_pos)) {
	for (i = src_pos, j = dst_pos; i < src_pos + length;
	     i++, j++) {
	    CVMObject* elem;
	    CVMD_arrayReadRef(srcArr, i, elem);
	    if (elem != NULL) {
		CVMClassBlock* rhsType = CVMobjectGetClass(elem);
		if ((rhsType != dstElemCb) &&
		    !CVMisAssignable(ee, rhsType, dstElemCb)) {
			if (!CVMlocalExceptionOccurred(ee)) {
			    CVMthrowArrayStoreException(ee, NULL);
			}
			return;
		    }
	    }
	    CVMD_arrayWriteRef(dstArr, j, elem);
	}
    } else {
	for (i = src_pos + length - 1, j = dst_pos + length - 1; 
	     i >= src_pos;
	     i--, j--) {
	    CVMObject* elem;
	    CVMD_arrayReadRef(srcArr, i, elem);
	    if (elem != NULL) {
		CVMClassBlock* rhsType = CVMobjectGetClass(elem);
		if ((rhsType != dstElemCb) &&
		    !CVMisAssignable(ee, rhsType, dstElemCb)) {
			if (!CVMlocalExceptionOccurred(ee)) {
			   CVMthrowArrayStoreException(ee, NULL);
			}
			return;
		    }
	    }
	    CVMD_arrayWriteRef(dstArr, j, elem);
	}
    }
}

JNIEXPORT void JNICALL
JVM_ArrayCopy(JNIEnv *env, jclass ignored,
	      jobject src, jint src_pos, 
	      jobject dst, jint dst_pos, jint length)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock *srcCb, *dstCb;
    CVMClassBlock *srcElemCb, *dstElemCb;
    CVMArrayOfAnyType *srcArr;
    CVMArrayOfAnyType *dstArr;
    size_t srclen;
    size_t dstlen;

    if ((src == 0) || (dst == 0)) {
	CVMthrowNullPointerException(ee, NULL);
	return;
    }
    CVMD_gcUnsafeExec(ee, {

	srcArr = (CVMArrayOfAnyType *)CVMID_icellDirect(ee, src);
	dstArr = (CVMArrayOfAnyType *)CVMID_icellDirect(ee, dst);

	srcCb = CVMobjectGetClass(srcArr);
	dstCb = CVMobjectGetClass(dstArr);

	/*
	 * First check whether we are dealing with source and destination
	 * arrays here. If not, bail out quickly.
	 */
	if (!CVMisArrayClass(srcCb) || !CVMisArrayClass(dstCb)) {
	    CVMthrowArrayStoreException(ee, NULL);
	    goto bail;
	}

	srcElemCb = CVMarrayElementCb(srcCb);
	dstElemCb = CVMarrayElementCb(dstCb);

	srclen = CVMD_arrayGetLength(srcArr);
	dstlen = CVMD_arrayGetLength(dstArr);

	/*
	 * If we are trying to copy between
	 * arrays that don't have the same type of primitive elements,
	 * bail out quickly.
	 *
	 * The more refined assignability checks for reference arrays
	 * will be performed in the CVM_T_CLASS case below.
	 */
	/*
	 * We could make this check slightly faster by comparing
	 * type-codes of the elements of the array, if we had a
	 * quick way of getting to that.
	 */
	if ((srcElemCb != dstElemCb) && (CVMcbIs(srcElemCb, PRIMITIVE) ||
					 CVMcbIs(dstElemCb, PRIMITIVE))) {
	    CVMthrowArrayStoreException(ee, NULL);
	    goto bail;
	} else if ((length < 0) || (src_pos < 0) || (dst_pos < 0) ||
		   (length + src_pos > srclen) ||
		   (length + dst_pos > dstlen)) {
	    CVMthrowArrayIndexOutOfBoundsException(ee, NULL);
	    goto bail;
	}
	
	switch(CVMarrayElemTypeCode(srcCb)) {
	case CVM_T_BOOLEAN: 
	    CVMD_arrayCopyBoolean((CVMArrayOfBoolean*)srcArr, src_pos,
				  (CVMArrayOfBoolean*)dstArr, dst_pos,
				  length);
	    break;
	case CVM_T_BYTE: 
	    CVMD_arrayCopyByte((CVMArrayOfByte*)srcArr, src_pos,
			       (CVMArrayOfByte*)dstArr, dst_pos,
			       length);
	    break;
	    
	case CVM_T_SHORT:
	    CVMD_arrayCopyShort((CVMArrayOfShort*)srcArr, src_pos,
				(CVMArrayOfShort*)dstArr, dst_pos,
				length);
	    break;
	case CVM_T_CHAR:
	    CVMD_arrayCopyChar((CVMArrayOfChar*)srcArr, src_pos,
			       (CVMArrayOfChar*)dstArr, dst_pos,
			       length);
	    break;
	case CVM_T_INT:
	    CVMD_arrayCopyInt((CVMArrayOfInt*)srcArr, src_pos,
			      (CVMArrayOfInt*)dstArr, dst_pos,
			      length);
	    break;
	case CVM_T_LONG:
	    CVMD_arrayCopyLong((CVMArrayOfLong*)srcArr, src_pos,
			       (CVMArrayOfLong*)dstArr, dst_pos,
			       length);
	    break;
	case CVM_T_FLOAT: 
	    CVMD_arrayCopyFloat((CVMArrayOfFloat*)srcArr, src_pos,
				(CVMArrayOfFloat*)dstArr, dst_pos,
				length);
	    break;
	case CVM_T_DOUBLE: 
	    CVMD_arrayCopyDouble((CVMArrayOfDouble*)srcArr, src_pos,
				 (CVMArrayOfDouble*)dstArr, dst_pos,
				 length);
	    break;
	case CVM_T_CLASS: {
	    /*
	     * Do a quick check here for the easy case of the same
	     * array type, or copying into an array of objects. No
	     * need for element-wise checks in that case.  
	     */
	    if ((srcElemCb == dstElemCb) ||
		(dstElemCb == CVMsystemClass(java_lang_Object))) {
		CVMD_arrayCopyRef((CVMArrayOfRef*)srcArr, src_pos,
				  (CVMArrayOfRef*)dstArr, dst_pos,
				  length);
	    } else {
		/* Do it the hard way. */
		CVMcopyRefArrays(ee, 
				 (CVMArrayOfRef*)srcArr, src_pos, 
				 (CVMArrayOfRef*)dstArr, dst_pos, 
				 dstElemCb, length);
	    }
	    break;
	case CVM_T_VOID: 
	    CVMassert(CVM_FALSE);
	}
	}
    bail:;
    });
}

/* For compatibility only; no longer called by java.lang.Runtime */

#ifdef JDK12
JNIEXPORT void JNICALL
JVM_Exit(jint code)
{
    if (code == 0) {
        RunFinalizersOnExit();
    }
    Exit((int)code);
}
#endif

JNIEXPORT void JNICALL
JVM_Halt(jint code)		/* Called from java.lang.Shutdown */
{
#ifdef CVM_LVM /* %begin lvm */
    CVMExecEnv* ee = CVMgetEE();
    if (CVMLVMinMainLVM(ee)) {
	CVMexit(code);
    } else {
	CVMLVMcontextHalt(ee);
    }
#else /* %end lvm */
    CVMexit(code);
#endif /* %begin,end lvm */
}

JNIEXPORT void JNICALL
JVM_OnExit(void (*func)(void))
{
    CVMatExit(func);
}

JNIEXPORT void JNICALL
JVM_GC(void)
{
    CVMgcRunGC(CVMgetEE());
}

/* JVM_GarbageCollect is implemented to support CLDC VM compatibility. */
JNIEXPORT int JNICALL
JVM_GarbageCollect(int flags, int requested_free_bytes)
{
    CVMExecEnv* ee = CVMgetEE();

    /* We can't completely honor the JVM_COLLECT_YOUNG_SPACE_ONLY, since
       there is no API to just collect the youngGen. However, unless -1
       free bytes are requested, only the youngGen will be collected assuming
       it satisfies requested_free_bytes. Also, if this flag is not set,
       then we are suppose to do a full collection, which can be accomplished
       by forcing requested_free_bytes = ~0.
    */
    if ((flags & JVM_COLLECT_YOUNG_SPACE_ONLY) != 0) {
        requested_free_bytes = ~0;
    }
    CVMgcStopTheWorldAndGC(ee, requested_free_bytes);
    return CVMgcFreeMemory(ee);
}

JNIEXPORT jlong JNICALL
JVM_MaxObjectInspectionAge()
{
    CVMInt64 diff = CVMlongSub(CVMtimeMillis(), CVMgcimplTimeOfLastMajorGC());
    return diff;
}

JNIEXPORT void JNICALL
JVM_TraceInstructions(jboolean on)
{
#ifdef CVM_TRACE
    if (on) {
	CVMsetDebugFlags(CVM_DEBUGFLAG(TRACE_OPCODE));
    } else {
	CVMclearDebugFlags(CVM_DEBUGFLAG(TRACE_OPCODE));
    }
#endif
}

JNIEXPORT void JNICALL
JVM_TraceMethodCalls(jboolean on)
{
#ifdef CVM_TRACE
    if (on) {
	CVMsetDebugFlags(CVM_DEBUGFLAG(TRACE_METHOD));
    } else {
	CVMclearDebugFlags(CVM_DEBUGFLAG(TRACE_METHOD));
    }
#endif
}

JNIEXPORT jlong JNICALL
JVM_TotalMemory(void)
{
    CVMExecEnv* ee = CVMgetEE();
    jlong totalMem;
    
    CVMD_gcUnsafeExec(ee, {
	totalMem = CVMgcTotalMemory(ee);
    });
    return totalMem;
}

/* Called from Runtime.c */

JNIEXPORT jlong JNICALL
JVM_FreeMemory(void)
{
    CVMExecEnv* ee = CVMgetEE();
    jlong freeMem;
    
    CVMD_gcUnsafeExec(ee, {
	freeMem = CVMgcFreeMemory(ee);
    });
    return freeMem;
}

#ifndef CDC_10
/* Called from Runtime.c */

JNIEXPORT jlong JNICALL
JVM_MaxMemory(void)
{
   return CVMint2Long(CVMglobals.maxHeapSize);
}
#endif /* !CDC_10 */

/* Called from Throwable.c */

#undef min
#define min(x, y) (((int)(x) < (int)(y)) ? (x) : (y))

/* NOTE: The second argument to printStackTrace() is expected to be an object
 * with a void println(char[]) method, such as a PrintStream or a PrintWriter.
 */

JNIEXPORT void JNICALL
JVM_PrintStackTrace(JNIEnv *env, jobject throwable, jobject printable)
{
    CVMprintStackTrace(CVMjniEnv2ExecEnv(env), throwable, printable);
}

#ifndef CDC_10
static const char*
getSourceFileName(CVMClassBlock* cb)
{
#ifdef CVM_DEBUG_CLASSINFO
    const char* fn = CVMcbSourceFileName(cb);
    const char* tmp;

    if (fn == NULL) {
        return NULL;
    }

    tmp = strrchr(fn, '/');
    if (tmp != NULL) {
        fn = tmp + 1; /* skip past last separator */
    }
    return fn;
#else
    return NULL;
#endif
}
#endif /* !CDC_10 */

void 
CVMprintStackTrace(CVMExecEnv *ee, CVMThrowableICell* throwableICell,
		   CVMObjectICell* printableICell)

{
#ifndef CVM_DEBUG_STACKTRACES
    CVMconsolePrintf("\t<no backtrace available in non-debug build>\n");
#else
    CVMID_localrootBegin(ee) {
	CVMID_localrootDeclare(CVMArrayOfRefICell, backtraceICell);
	CVMID_localrootDeclare(CVMArrayOfIntICell, pcArrayICell);
#ifdef CVM_JIT
	CVMID_localrootDeclare(CVMArrayOfBooleanICell, isCompiledArrayICell);
#endif
	CVMID_localrootDeclare(CVMStringICell, stringICell);
	CVMID_localrootDeclare(CVMObjectICell, objICell);
	CVMID_localrootDeclare(CVMObjectICell, tempICell);
	CVMUint32 count;
	CVMUint32 pcArrayLen;

	/*
	 * Read in the backtrace array field of the throwable.
	 */
	CVMID_fieldReadRef(ee, throwableICell,
			   CVMoffsetOfjava_lang_Throwable_backtrace,
			   tempICell);
	CVMID_icellAssign(ee, backtraceICell, (CVMArrayOfRefICell*)tempICell);
	if (CVMID_icellIsNull(backtraceICell)) {
	    goto localRootEnd;
	}
	
	/*
	 * The first element of the backtrace array is an array of method index
         * and line number info.
	 */
	CVMID_arrayReadRef(ee, backtraceICell, 0, tempICell);
	CVMID_icellAssign(ee, pcArrayICell, (CVMArrayOfIntICell*)tempICell);
	if (CVMID_icellIsNull(pcArrayICell)) {
	    goto localRootEnd;
	}
	
	/*
	 * The 2nd element of the backtrace array is an array of isCompiled
	 * values.
	 */
#ifdef CVM_JIT
	CVMID_arrayReadRef(ee, backtraceICell, 1, tempICell);
	CVMID_icellAssign(ee, isCompiledArrayICell,
			  (CVMArrayOfBooleanICell*)tempICell);
	if (CVMID_icellIsNull(isCompiledArrayICell)) {
	    goto localRootEnd;
	}
#endif
	
	/*
	 * Print each frame in the backtrace.
	 */
	CVMID_arrayGetLength(ee, pcArrayICell, pcArrayLen);
	for (count = 0; count < pcArrayLen; count++) {
	    CVMassert(!CVMlocalExceptionOccurred(ee));
	    CVMID_arrayReadRef(ee, backtraceICell, count + 2, objICell);
	    if (CVMID_icellIsNull(objICell)) {
		goto localRootEnd;
	    }

	    /* obj is a java.lang.Class. Compute the backtrace string. */
            {
		CVMJavaInt     linenoInfo;
		CVMJavaBoolean isCompiled;
		char           buf[256];
		int            i = 0;
		CVMClassBlock* cb = CVMgcSafeClassRef2ClassBlock(ee, objICell);
		CVMMethodBlock* mb;
		
		/* get lineno, mb, and isCompiled values */
		CVMID_arrayReadInt(ee, pcArrayICell, count, linenoInfo);
#ifdef CVM_JIT
		CVMID_arrayReadBoolean(ee, isCompiledArrayICell,
				       count, isCompiled);
#else
		isCompiled = CVM_FALSE;
#endif
	        mb = CVMcbMethodSlot(cb, (CVMUint32)linenoInfo & 0xffff);

	        strncpy(buf + i, "\tat ", 4);
	        i += 4;

                /* make a string out of the lineno info. */
                CVMlineno2string((linenoInfo >> 16), mb, CVM_FALSE, isCompiled,
			         buf + i, buf + sizeof(buf));	        

	        CVMnewStringUTF(ee, stringICell, buf);
		CVMassert(!CVMlocalExceptionOccurred(ee));
            }

	    /* call the println method of the printable object */
	    if (!CVMID_icellIsNull(stringICell)) {
		CVMMethodBlock* mb; 
		CVMClassBlock*  cb;
		JNIEnv*         env = CVMexecEnv2JniEnv(ee);
		/*
		 * The only way to get a NULL printableICell is if you are
		 * debugging and call this function manually ,or when called
		 * during vm startup. In this case we use CVMconsolePrintf.
		 */
		if (printableICell == NULL) {
		    const char* str =
			CVMjniGetStringUTFChars(env, stringICell, NULL);
		    if (str == NULL) {
			break;
		    }
		    CVMconsolePrintf("%s\n", str);
		    CVMjniReleaseStringUTFChars(env, stringICell, str);
		} else {
		    CVMD_gcUnsafeExec(ee, {
			cb = CVMobjectGetClass(
                            CVMID_icellDirect(ee, printableICell));
		    });
		    mb = CVMclassGetNonstaticMethodBlock(
                        cb, CVMglobals.printlnTid);
		    (*env)->CallVoidMethod(env, printableICell,
					   mb, stringICell);
		    /* maybe we should only check local exceptions,
		       or disable remote exceptions? */
		    if ((*env)->ExceptionCheck(env)) {
			break;
		    }
		}
	    }
	}

    localRootEnd:;
    } CVMID_localrootEnd();
#endif
}

/*
 * Fill in the specified Throwable object with the current backtrace,
 */

JNIEXPORT void JNICALL
JVM_FillInStackTrace(JNIEnv *env, jobject throwable)
{
    CVMfillInStackTrace(CVMjniEnv2ExecEnv(env), throwable);    
    /* NOTE: CVMfillInStackTrace() may throw an exception if it is not able to
       allocate memory for the stack trace. */
}

static void 
CVMgcUnsafeFillInStackTrace(CVMExecEnv *ee, CVMThrowableICell* throwableICell);

void 
CVMfillInStackTrace(CVMExecEnv *ee, CVMThrowableICell* throwableICell) {
    if (CVMD_isgcSafe(ee)) {
	CVMD_gcUnsafeExec(ee, {
	    CVMgcUnsafeFillInStackTrace(ee, throwableICell);
	});
    } else {
	CVMgcUnsafeFillInStackTrace(ee, throwableICell);
    }
}

#ifdef CVM_DEBUG_STACKTRACES
static void
throwPreallocatedOutOfMemoryError(CVMExecEnv *ee,
                                  CVMThrowableICell* throwableICell)
{
    /* Nullify the back trace too: */
    CVMD_fieldWriteRef(CVMID_icellDirect(ee, throwableICell), 
        CVMoffsetOfjava_lang_Throwable_backtrace, NULL);

#ifdef CVM_DEBUG
    {
	CVMObject *exceptionObj = CVMID_icellDirect(ee, throwableICell);
        CVMClassBlock *exceptionCb = CVMobjectGetClass(exceptionObj);
        CVMdebugPrintf((
            "Could not allocate backtrace for exception object \"%C\"; "
            "throwing a pre-allocated OutOfMemoryError object instead\n",
             exceptionCb));
    }
#endif

    CVMD_gcSafeExec(ee, {
        CVMgcSafeThrowLocalException(
            ee, CVMglobals.preallocatedOutOfMemoryError);
    });
}
#endif

static void 
CVMgcUnsafeFillInStackTrace(CVMExecEnv *ee, CVMThrowableICell* throwableICell)
{
#ifdef CVM_DEBUG_STACKTRACES
    CVMFrame*          startFrame;
    CVMFrameIterator   iter;
    CVMMethodBlock*    mb;
    CVMArrayOfRef*     backtrace;
    CVMArrayOfInt*     pcArray;
#ifdef CVM_JIT
    CVMArrayOfBoolean* isCompiledArray = NULL;
#endif
    CVMObject*         tempObj;
    CVMInt32           backtraceSize;
    CVMInt32           count;
    CVMBool            noMoreFrameSkips;

    startFrame = CVMeeGetCurrentFrame(ee);

    /* how many useful frames are there on the stack? */
    CVMD_gcSafeExec(ee, {
        noMoreFrameSkips = CVM_FALSE;
	backtraceSize = 0;
        CVMframeIterateInit(&iter, startFrame);
	while (CVMframeIterateNext(&iter)) {
	    mb = CVMframeIterateGetMb(&iter);
	    /* ignore all frames that are just part of the <init> of
	     * the throwable object. */
	    if (!noMoreFrameSkips) {
		if (mb == CVMglobals.java_lang_Throwable_fillInStackTrace) {
		    continue;
		} else if (CVMtypeidIsConstructor(CVMmbNameAndTypeID(mb)) &&
			   CVMisSubclassOf(ee, CVMmbClassBlock(mb), 
				CVMsystemClass(java_lang_Throwable))) {
		    continue;
		} else {
		    noMoreFrameSkips = CVM_TRUE;
		}
	    }

	    backtraceSize++;
	}
    });


    /*
     * The backtrace array contains the following:
     *   <1> Pointer to an int[] for line number and the method index for
     *       each frame. The lineno and index each take up 16-bits of the int.
     *   <2> If the JIT is supported, a pointer to a boolean[] that contains
     *       this isCompiled flag for each frame, otherwise NULL.
     *   <3...n+2> The Class instance for each mb in each frame. This is
     *       used to force all classes that are in a backtrace to stay loaded.
     */
    backtrace = (CVMArrayOfRef*)CVMgcAllocNewArray(
            ee, CVM_T_CLASS, 
	    (CVMClassBlock*)CVMbasicTypeArrayClassblocks[CVM_T_CLASS],
	    backtraceSize + 2);
    tempObj = (CVMObject*)backtrace;
    CVMD_fieldWriteRef(CVMID_icellDirect(ee, throwableICell), 
		       CVMoffsetOfjava_lang_Throwable_backtrace,
		       tempObj);
    if (backtrace == NULL) {
        throwPreallocatedOutOfMemoryError(ee, throwableICell);
	return;
    }

    /*
     * The pcArray is a java array of ints and is stored as the first
     * element of the backtrace array.
     */
    pcArray = (CVMArrayOfInt*)CVMgcAllocNewArray(
            ee, CVM_T_INT, 
	    (CVMClassBlock*)CVMbasicTypeArrayClassblocks[CVM_T_INT],
	    backtraceSize);
    /* recache backtrace in case we became gcsafe during allocation. */
    CVMD_fieldReadRef(CVMID_icellDirect(ee, throwableICell),
		      CVMoffsetOfjava_lang_Throwable_backtrace,
		      tempObj);
    backtrace = (CVMArrayOfRef*)tempObj;
    tempObj = (CVMObject*)pcArray;
    CVMD_arrayWriteRef(backtrace, 0, tempObj);
    if (pcArray == NULL) {
        throwPreallocatedOutOfMemoryError(ee, throwableICell);
	return;
    }

#ifdef CVM_JIT
    /*
     * The isCompiledArray is an array of booleans and is stored as the
     * 2nd element of the backtrace array.
     */
    isCompiledArray = (CVMArrayOfBoolean*)CVMgcAllocNewArray(
            ee, CVM_T_BOOLEAN, 
	    (CVMClassBlock*)CVMbasicTypeArrayClassblocks[CVM_T_BOOLEAN],
	    backtraceSize);
    /* recache backtrace and pcArray in case we became gcsafe during
     * the allocation.
     */
    CVMD_fieldReadRef(CVMID_icellDirect(ee, throwableICell),
		      CVMoffsetOfjava_lang_Throwable_backtrace,
		      tempObj);
    backtrace = (CVMArrayOfRef*)tempObj;
    CVMD_arrayReadRef(backtrace, 0, tempObj);
    pcArray = (CVMArrayOfInt*)tempObj;
    tempObj = (CVMObject*)isCompiledArray;
    CVMD_arrayWriteRef(backtrace, 1, tempObj);
    if (isCompiledArray == NULL) {
        throwPreallocatedOutOfMemoryError(ee, throwableICell);
	return;
    }
#endif

    /* 
     * Fill in the pcArray, isCompiledArray, and the backtrace array.
     */
    noMoreFrameSkips = CVM_FALSE;
    count = 0;
    CVMframeIterateInit(&iter, startFrame);
    while (CVMframeIterateNext(&iter)) {
	CVMMethodBlock* mb = CVMframeIterateGetMb(&iter);
	CVMJavaInt lineno;

	/* ignore all frames that are just part of the <init> of
	 * the throwable object. */
	if (!noMoreFrameSkips) {
	    if (mb == CVMglobals.java_lang_Throwable_fillInStackTrace) {
		continue;
	    } else if (CVMtypeidIsConstructor(CVMmbNameAndTypeID(mb)) &&
		CVMisSubclassOf(ee, CVMmbClassBlock(mb), 
				CVMsystemClass(java_lang_Throwable))) {
		continue;
	    } else {
		noMoreFrameSkips = CVM_TRUE;
	    }
	}

	/* store the line number and the method index in the pcArray */
	if (CVMmbIs(mb, NATIVE)) {
	    lineno = -2; /* a native method has -2 as its lineno */
	} else {
	    CVMUint8* javaPc;
	    CVMUint16 javaPcOffset;
#ifdef CVM_JIT
	    CVMBool isCompiled;
	    if (CVMframeIterateIsInlined(&iter)) {
		isCompiled = CVM_TRUE;
	    } else {
		CVMFrame* frame = CVMframeIterateGetFrame(&iter);
		CVMassert(frame->mb == mb);
		isCompiled = CVMframeIsCompiled(frame);
	    }
	    CVMD_arrayWriteBoolean(isCompiledArray, count,
				   (CVMJavaBoolean)isCompiled);
#endif
	    javaPc = CVMframeIterateGetJavaPc(&iter);
	    if (javaPc == NULL) {
		javaPc = CVMmbJavaCode(mb);
	    }
	    javaPcOffset = (javaPc - CVMmbJavaCode(mb));
	    
	    /* We must eagerly compute the line number information for
	     * <clinit> because the jmd may be freed after the <clinit>
	     * is run. So we just get the lineno information for all.
	     */
#ifdef CVM_DEBUG_CLASSINFO
	    lineno = CVMpc2lineno(mb, javaPcOffset);
#else
	    lineno = -1;
#endif
	}
	
	CVMD_arrayWriteInt(pcArray, count, (CVMJavaInt)
			   ((lineno << 16) |
			    (CVMmbFullMethodIndex(mb) & 0xffff)));
	/* Write the class object into the backtrace array. */
	{
	    CVMClassICell* classICell;
            CVMClassBlock* cb;
#ifdef CVM_JVMTI
            if (CVMjvmtiMbIsObsolete(mb)) {
                cb = CVMcbOldData(CVMmbClassBlock(mb))->currentCb;
            } else
#endif
            {
                cb = CVMmbClassBlock(mb);
            }
            classICell = CVMcbJavaInstance(cb);
	    CVMD_arrayWriteRef(backtrace, count + 2, 
			       CVMID_icellDirect(ee, classICell));
	}
	count++;
    } 

    CVMassert(count + 2 == CVMD_arrayGetLength(backtrace));
#endif
}

#ifndef CDC_10
static CVMInt32 CVMgetStackTraceDepth(CVMExecEnv *ee,
                                      CVMThrowableICell* throwableICell)
{
#ifdef CVM_DEBUG_STACKTRACES
    CVMInt32 stackDepth = 0;

    if (CVMID_icellIsNull(throwableICell)) {
        CVMthrowNullPointerException(ee, NULL);
        return 0;
    }

    CVMID_localrootBegin(ee) {
        CVMID_localrootDeclare(CVMObjectICell, backtraceICell);
        
        CVMID_fieldReadRef(ee, throwableICell,
                           CVMoffsetOfjava_lang_Throwable_backtrace,
                           backtraceICell);
        if (!CVMID_icellIsNull(backtraceICell)) {
            CVMID_arrayGetLength(ee, (CVMArrayOfRefICell*)backtraceICell,
                                 stackDepth);
            /* The first two elements of the backtrace array are pcValue
             * array and isCompiled array. So need to subtract the size
             * by 2 */
            stackDepth -= 2;
        }
    } CVMID_localrootEnd();
    return stackDepth;
#else
    return 0;
#endif
}

JNIEXPORT jint JNICALL
JVM_GetStackTraceDepth(JNIEnv *env, jobject throwable) 
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    return CVMgetStackTraceDepth(ee, throwable);
}

#ifdef CVM_DEBUG_STACKTRACES
static CVMObjectICell* CVMgetStackTraceElement(CVMExecEnv *ee, 
            CVMThrowableICell* throwableICell, CVMInt32 index)
{
    CVMObjectICell* stElementICell;
    CVMJavaInt backtraceLen;
    CVMJavaInt linenoInfo, lineno;
    CVMClassBlock* cb;
    CVMMethodBlock* mb;
    CVMMethodTypeID tid;
    char buf[256];
    const char* fn;
    const CVMClassBlock* stackTraceElementCb;
    CVMJavaBoolean isCompiled = CVM_FALSE;

    if (CVMID_icellIsNull(throwableICell)) {
        CVMthrowNullPointerException(ee, NULL);
        return NULL;
    }

    stElementICell = CVMjniCreateLocalRef(ee);

    CVMID_localrootBegin(ee) {
        CVMID_localrootDeclare(CVMArrayOfRefICell, backtraceICell);
#ifdef CVM_JIT
        CVMID_localrootDeclare(CVMArrayOfBooleanICell, isCompiledArrayICell);
#endif
        CVMID_localrootDeclare(CVMArrayOfIntICell, pcArrayICell);
        CVMID_localrootDeclare(CVMObjectICell, tempICell);
        CVMID_localrootDeclare(CVMStringICell, stringICell);

        /* Get the backtrace array field of the throwable */
        CVMID_fieldReadRef(ee, throwableICell,
                           CVMoffsetOfjava_lang_Throwable_backtrace,
                           tempICell);
        CVMID_icellAssign(ee, backtraceICell, 
                          (CVMArrayOfRefICell*)tempICell);
        if (CVMID_icellIsNull(backtraceICell)) {
            goto failed_goto_localRootEnd;
        }

        CVMID_arrayGetLength(ee, backtraceICell, backtraceLen);
        if (index >= backtraceLen - 2) {
            CVMthrowArrayIndexOutOfBoundsException(ee, NULL);
            goto failed_goto_localRootEnd;
        }

        /* Now allocate a new java.lang.StackTraceElement object */
        stackTraceElementCb = CVMsystemClass(java_lang_StackTraceElement);
        CVMID_allocNewInstance(ee, (CVMClassBlock*)stackTraceElementCb,
                               stElementICell);
        if (CVMID_icellIsNull(stElementICell)) {
            CVMthrowOutOfMemoryError(ee, NULL);
            goto failed_goto_localRootEnd;
        }

        /* The first element of the backtrace array is an array of 
         * method index and lineno info:
         * 
         *     lineno << 16 | methodIndex
         *
         */
        CVMID_arrayReadRef(ee, backtraceICell, 0, tempICell);
        CVMID_icellAssign(ee, pcArrayICell,
                          (CVMArrayOfIntICell*)tempICell);
        if (CVMID_icellIsNull(pcArrayICell)) {
            goto failed_goto_localRootEnd;
        }
        CVMID_arrayReadInt(ee, pcArrayICell, index, linenoInfo);

#ifdef CVM_JIT
        /* The 2nd element of the backtrace array is an array of 
         * isCompiled values. */
        CVMID_arrayReadRef(ee, backtraceICell, 1, tempICell);
        CVMID_icellAssign(ee, isCompiledArrayICell,
                          (CVMArrayOfBooleanICell*)tempICell);
        if (CVMID_icellIsNull(isCompiledArrayICell)) {
            goto failed_goto_localRootEnd;
        }
        CVMID_arrayReadBoolean(ee, isCompiledArrayICell, index, isCompiled);
#endif

        /* Get the indexed backtrace element. It's java.lang.Class
         * object. */
        CVMID_arrayReadRef(ee, backtraceICell, index+2, tempICell);
        if (CVMID_icellIsNull(tempICell)) {
            goto failed_goto_localRootEnd;
        }
    
        cb = CVMgcSafeClassRef2ClassBlock(ee, tempICell);
        mb = CVMcbMethodSlot(cb, ((CVMUint32)linenoInfo & 0xffff));
        lineno = linenoInfo >> 16;
        tid = CVMmbNameAndTypeID(mb);

        /* Fill in the fields */

        /* Fill in the declaringClass field */
        CVMclassname2String(CVMcbClassName(cb), buf, sizeof(buf));
        CVMnewStringUTF(ee, stringICell, buf);
        CVMID_fieldWriteRef(ee, stElementICell,
            CVMoffsetOfjava_lang_StackTraceElement_declaringClass,
            stringICell);

        /* Fill in the methodName field */
        CVMtypeidMethodNameToCString(tid, buf, sizeof(buf));
        CVMnewStringUTF(ee, stringICell, buf);
        CVMID_fieldWriteRef(ee, stElementICell,
            CVMoffsetOfjava_lang_StackTraceElement_methodName,
            stringICell);

        /* Fill in the fileName field */
        fn = getSourceFileName(cb);
        if (fn != NULL) {
            CVMnewStringUTF(ee, stringICell, fn);
            CVMID_fieldWriteRef(ee, stElementICell,
                CVMoffsetOfjava_lang_StackTraceElement_fileName,
                stringICell);
        }

        /* Fill in the lineNumber field */
        CVMID_fieldWriteInt(ee, stElementICell,
            CVMoffsetOfjava_lang_StackTraceElement_lineNumber,
            lineno);

        /* Fill in the isCompiled field */
        CVMID_fieldWriteInt(ee, stElementICell,
	    CVMoffsetOfjava_lang_StackTraceElement_isCompiled,
	    isCompiled);

#ifdef CVM_DEBUG
        /* Fill in the methodSignature field for debug builds. For non-debug
         * leave the field as null.
         */
        CVMtypeidMethodTypeToCString(tid, buf, sizeof(buf));
        CVMnewStringUTF(ee, stringICell, buf);
        CVMID_fieldWriteRef(ee, stElementICell,
	    CVMoffsetOfjava_lang_StackTraceElement_methodSignature,
	    stringICell);
#endif

failed_goto_localRootEnd:;
    } CVMID_localrootEnd();

    return stElementICell;
}
#endif

JNIEXPORT jobject JNICALL
JVM_GetStackTraceElement(JNIEnv *env, jobject throwable, jint index) 
{
#ifdef CVM_DEBUG_STACKTRACES
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    return (jobject)CVMgetStackTraceElement(ee, throwable,
                                            (CVMInt32)index);
#else
    return NULL;
#endif
}
#endif /* !CDC_10 */

/* Called from object.c */
JNIEXPORT jint JNICALL
JVM_IHashCode(JNIEnv *env, jobject obj)
{
    if (obj == NULL) {
	return 0;
    }
    return CVMobjectGetHashSafe(CVMjniEnv2ExecEnv(env), obj);
}

JNIEXPORT void JNICALL
JVM_MonitorWait(JNIEnv *env, jobject obj, jlong millis)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    if (CVMlongLtz(millis)) {
	CVMthrowIllegalArgumentException(ee, "timeout value is negative");
        return;
    }

#ifdef CVM_JVMPI
    if (CVMjvmpiEventMonitorWaitIsEnabled()) {
        CVMjvmpiPostMonitorWaitEvent(ee, obj, millis);
    }
#endif
    ee->threadState = CVM_THREAD_WAITING | CVM_THREAD_OBJECT_WAIT;
    ee->threadState |= (millis == 0 ? CVM_THREAD_WAITING_INDEFINITE :
		       CVM_THREAD_WAITING_TIMEOUT);
#ifdef CVM_JVMTI
    if (CVMjvmtiShouldPostMonitorWait()) {
        CVMjvmtiPostMonitorWaitEvent(ee, obj, millis);
    }
#endif

    /* NOTE: CVMgcSafeObjectWait() may throw certain exceptions: */
    CVMgcSafeObjectWait(ee, obj, millis);

    ee->threadState &= ~(CVM_THREAD_WAITING | CVM_THREAD_OBJECT_WAIT |
                         CVM_THREAD_WAITING_INDEFINITE |
			 CVM_THREAD_WAITING_TIMEOUT);
#ifdef CVM_JVMPI
    if (CVMjvmpiEventMonitorWaitedIsEnabled()) {
        CVMjvmpiPostMonitorWaitedEvent(ee, obj);
    }
#endif
#ifdef CVM_JVMTI
    if (CVMjvmtiShouldPostMonitorWaited()) {
        CVMjvmtiPostMonitorWaitedEvent(ee, obj, CVM_FALSE);
    }
#endif
}

JNIEXPORT void JNICALL
JVM_MonitorNotify(JNIEnv *env, jobject obj)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    /* NOTE: CVMgcSafeObjectNotify() may throw certain exceptions: */
    CVMgcSafeObjectNotify(ee, obj);
}

JNIEXPORT void JNICALL
JVM_MonitorNotifyAll(JNIEnv *env, jobject obj)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    /* NOTE: CVMgcSafeObjectNotifyAll() may throw certain exceptions: */
    CVMgcSafeObjectNotifyAll(ee, obj);
}

JNIEXPORT jobject JNICALL
JVM_Clone(JNIEnv *env, jobject obj)
{
    CVMClassBlock*  cb;
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    jobject result;

    CVMID_localrootBegin(ee) {
	CVMID_localrootDeclare(CVMObjectICell, cloneObj);
	CVMID_objectGetClass(ee, obj, cb);
	/*
	 * Now let's see whether we are cloning an array or an object
	 */
	if (CVMisArrayClass(cb)) {
	    CVMArrayOfAnyTypeICell* arrObj = (CVMArrayOfAnyTypeICell*)obj;
	    CVMBasicType typeCode = CVMarrayElemTypeCode(cb);
	    CVMJavaInt   arrLen;

	    CVMID_arrayGetLength(ee, arrObj, arrLen);
	    /* Make the copy first */
	    CVMID_allocNewArray(ee, typeCode, cb, arrLen, cloneObj);
	    if (CVMID_icellIsNull(cloneObj)) {
		/* Allocation failed */
		result = NULL;
		CVMthrowOutOfMemoryError(ee, "cloning array object");
		goto bailOut;
	    }
	    JVM_ArrayCopy(env, NULL, obj, 0, cloneObj, 0, arrLen);
	} else {
	    if (!CVMimplementsInterface(ee, cb,
				    CVMsystemClass(java_lang_Cloneable))) {
		result = NULL;
		if (!CVMlocalExceptionOccurred(ee)) {
		   CVMthrowCloneNotSupportedException(ee,
						   "cloning regular object");
		}
		goto bailOut;
	    }
	    CVMID_allocNewInstance(ee, cb, cloneObj);
	    if (CVMID_icellIsNull(cloneObj)) {
		/* Allocation failed */
		result = NULL;
		CVMthrowOutOfMemoryError(ee, "cloning regular object");
		goto bailOut;
	    }
	    CVMD_gcUnsafeExec(ee, {
		CVMObject* srcObj = CVMID_icellDirect(ee, obj);
		CVMObject* dstObj = CVMID_icellDirect(ee, cloneObj);
		CVMClassBlock* cbp;
		CVMwithAssertsOnly(
		    CVMUint32 coveredSize = sizeof(CVMObjectHeader);
		);
		
		/*
		 * Walk all instance fields in superclass order,
		 * and copy each with the right CVMD_ call.
		 */
		for (cbp = cb; cbp != NULL; cbp = CVMcbSuperclass(cbp)) {
		    CVMUint32 i;
		    /*
		     * Go through the declared fields
		     */
		    for (i = 0; i < CVMcbFieldCount(cbp); i++) {
			CVMFieldBlock* fb = CVMcbFieldSlot(cbp, i);
			if (!CVMfbIs(fb, STATIC)) {
			    CVMUint32 offset = CVMfbOffset(fb);
			    CVMClassTypeID type =
				CVMtypeidGetType(CVMfbNameAndTypeID(fb));
			    switch (type) {
				case CVM_TYPEID_INT:
				case CVM_TYPEID_SHORT:
				case CVM_TYPEID_CHAR:
				case CVM_TYPEID_BYTE:
			        case CVM_TYPEID_BOOLEAN: {
				    CVMJavaInt elem;
				    CVMD_fieldReadInt(srcObj, offset, elem);
				    CVMD_fieldWriteInt(dstObj, offset, elem);
				    CVMwithAssertsOnly(
				        coveredSize += sizeof(CVMJavaInt);
				    );
				    break;
				}
			        case CVM_TYPEID_LONG: {
				    CVMJavaLong l;
				    CVMD_fieldReadLong(srcObj, offset, l);
				    CVMD_fieldWriteLong(dstObj, offset, l);
				    CVMwithAssertsOnly(
				        coveredSize += sizeof(CVMJavaLong);
				    );
				    break;
				}
			        case CVM_TYPEID_DOUBLE: {
				    CVMJavaDouble d;
				    CVMD_fieldReadDouble(srcObj, offset, d);
				    CVMD_fieldWriteDouble(dstObj, offset, d);
				    CVMwithAssertsOnly(
				        coveredSize += sizeof(CVMJavaDouble);
				    );
				    break;
				}
			        case CVM_TYPEID_FLOAT: {
				    CVMJavaFloat f;
				    CVMD_fieldReadFloat(srcObj, offset, f);
				    CVMD_fieldWriteFloat(dstObj, offset, f);
				    CVMwithAssertsOnly(
				        coveredSize += sizeof(CVMJavaFloat);
				    );
				    break;
				}
			        default: /* Must be a ref */ {
				    CVMObject* o;
				    CVMD_fieldReadRef(srcObj, offset, o);
				    CVMD_fieldWriteRef(dstObj, offset, o);
				    CVMwithAssertsOnly(
				        coveredSize += sizeof(CVMObject*);
				    );
				    break;
				}
			    }
			}
		    }
		}
		/*
		 * Now make sure that we have copied just the right
		 * number of bytes from one object to the other
		 */
		/* 
		 * NOTE: the assert makes only sense if all the sizeof() above
		 * are multiples of CVMJavaVal32.
		 * For a 64 bit port this might not make sense.
		 * Leave the assert in for now, until a 64-bit port makes
		 * us modify it.
		 */
		CVMassert(coveredSize == CVMcbInstanceSize(cb));
	    });
	}
	result = (*env)->NewLocalRef(env, cloneObj);
    bailOut:;
    } CVMID_localrootEnd();
    return result;
}

/*
 * Thread class calls
 */

typedef struct {
    void (*nativeFunc)(void *);
    void *nativeFuncArg;
} CVMNativeRunInfo;

#ifdef CVM_DEBUG_ASSERTS
enum {
    NATIVERUNINFO_UNDEFINED = 0,
    NATIVERUNINFO_THREAD_START_INFO,
    NATIVERUNINFO_NATIVE_RUN_INFO,
};
#endif


/* NOTE: First function called by child thread. */
/* NOTE: Every Java thread (except the main thread) starts executing from
   here. */
static void start_func(void *arg)
{
    JNIEnv *env;
    jclass clazz;
    jmethodID methodID;
    CVMThreadStartInfo *info = (CVMThreadStartInfo *) arg;
    CVMExecEnv *ee = info->ee;
    CVMThreadICell *threadCell = info->threadICell;
    CVMThreadICell *eeThreadCell = NULL;
    CVMNativeRunInfo nativeInfo = {NULL, NULL};
    CVMBool attachResult;

    /* Wait for parent if necessary */
    CVMmutexLock(&info->parentLock);
    /* attach thread */
    info->started = attachResult = CVMattachExecEnv(ee, CVM_FALSE);

    if (attachResult) {
        CVMaddThread(ee, !info->isDaemon);
        /* Store eetop */
        {
            CVMJavaLong eetopVal = CVMvoidPtr2Long(ee);
            CVMID_fieldWriteLong(ee, threadCell,
                                 CVMoffsetOfjava_lang_Thread_eetop,
                                 eetopVal);
        }
	eeThreadCell = CVMcurrentThreadICell(ee);
	CVMID_icellAssign(ee, eeThreadCell, threadCell);
	if (info->nativeFunc != NULL) {
	    nativeInfo.nativeFunc = info->nativeFunc;
	    nativeInfo.nativeFuncArg = info->nativeFuncArg;
#ifdef CVM_DEBUG_ASSERTS
	    CVMassert(ee->nativeRunInfo == NULL);
            CVMassert(ee->nativeRunInfoType == NATIVERUNINFO_UNDEFINED);
            ee->nativeRunInfoType = NATIVERUNINFO_NATIVE_RUN_INFO;
#endif
	    ee->nativeRunInfo = (void *)&nativeInfo;
	}
    }

    CVMcondvarNotify(&info->parentCond);
    CVMthreadSetPriority(&ee->threadInfo, info->priority);
#ifdef CVM_INSPECTOR
    ee->priority = info->priority;
#endif

    CVMmutexUnlock(&info->parentLock);

    /* Parent frees info, child can no longer access it */
#if defined(CVM_DEBUG) || defined(CVM_DEBUG_ASSERTS)
    info = NULL;
#endif

    env = CVMexecEnv2JniEnv(ee);

    if (!attachResult) {
	goto failed;
    }

#ifdef CVM_JVMTI
    if (CVMjvmtiIsEnabled()) {
	CVMjvmtiDebugEventsEnabled(ee) = CVM_TRUE;
    }
#endif
    /* Should have startup() call this too... */
    CVMpostThreadStartEvents(ee);

    clazz = CVMcbJavaInstance(CVMsystemClass(java_lang_Thread));

    /* Call Thread.startup() */
    methodID = (*env)->GetMethodID(env, clazz, "startup", "(Z)V");
    CVMassert(methodID != NULL);

    (*env)->CallVoidMethod(env, eeThreadCell, methodID,
	nativeInfo.nativeFunc != NULL);

    CVMassert(!(*env)->ExceptionCheck(env));

    CVMdetachExecEnv(ee);
    CVMremoveThread(ee, ee->userThread);
 failed:
    if (attachResult) {
	CVMdestroyExecEnv(ee);
	free(ee);
    } else {
	/* parent will free */
    }

    /*
     * Thread is effectively dead from the VM's point of view, so
     * no VM data structures can be accessed after this point.
     */
}

/* Purpose: Native implementation of java.lang.Thread.runNative(). */
/* NOTE: Called from child thread. */
JNIEXPORT void JNICALL
JVM_RunNativeThread(JNIEnv *env, jobject thisObj)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMNativeRunInfo *info = (CVMNativeRunInfo *)ee->nativeRunInfo;
    void *arg = info->nativeFuncArg;
    void (*nativeFunc)(void *) = info->nativeFunc;

#ifdef CVM_DEBUG_ASSERTS
    CVMassert(ee->nativeRunInfoType == NATIVERUNINFO_NATIVE_RUN_INFO);
    ee->nativeRunInfoType = NATIVERUNINFO_UNDEFINED;
#endif
    ee->nativeRunInfo = NULL;
    CVMframeSetContextArtificial(ee);
    (*nativeFunc)(arg);
}

/* Purpose: Native implementation of java.lang.Thread.start0(). */
/* NOTE: Called from parent thread. */
JNIEXPORT void JNICALL
JVM_StartThread(JNIEnv *env, jobject thisObj, jint priority)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMExecEnv *targetEE = NULL;
    CVMThreadStartInfo *info = (CVMThreadStartInfo*)ee->nativeRunInfo;
    CVMThreadICell *threadICell = CVMID_getGlobalRoot(ee);
    const char *msg = NULL;

#ifdef CVM_DEBUG_ASSERTS
    if (info != NULL) {
        /* This function will take control of it now.  Clear it in the thread
           so that no one else can access it: */
        CVMassert(ee->nativeRunInfoType == NATIVERUNINFO_THREAD_START_INFO);
        ee->nativeRunInfoType = NATIVERUNINFO_UNDEFINED;
    } else {
        CVMassert(ee->nativeRunInfoType == NATIVERUNINFO_UNDEFINED);
    }
#endif
    /* NOTE: This function is the native implementation of Thread.start0()
       and it will be called every time Thread.start() is called.  We need to
       reset the ee->nativeRunInfo here because we'll free its content when
       we free(info) below.  If we don't, the next time Thread.start() is
       called, it will mistakenly think there is already a CVMThreadStartInfo
       record available for its use when there is not.
    */
    ee->nativeRunInfo = NULL;

    if (threadICell == NULL) {
#ifdef CVM_DEBUG
	msg = "no global roots";
#endif
	goto out_of_memory;
    }

    if (info == NULL) {
      /* Normal thread */
      info = (CVMThreadStartInfo *)malloc(sizeof(CVMThreadStartInfo));
      if (info == NULL) {
#ifdef CVM_DEBUG
	    msg = "malloc failed";
#endif
 	    goto out_of_memory;
      }
      info->nativeFunc = NULL;
      info->nativeFuncArg = NULL;
      info->started = 0;
    }
    if (!CVMmutexInit(&info->parentLock)) {
#ifdef CVM_DEBUG
	msg = "CVMmutexInit failed";
#endif
	goto out_of_memory;
    }
    if (!CVMcondvarInit(&info->parentCond, &info->parentLock)) {
#ifdef CVM_DEBUG
	msg = "CVMcondvarInit failed";
#endif
	goto out_of_memory1;
    }
    info->threadICell = threadICell;

    targetEE = (CVMExecEnv *)calloc(1, sizeof *targetEE);
    if (targetEE == NULL) {
#ifdef CVM_DEBUG
	msg = "calloc failed";
#endif
	goto out_of_memory2;
    }

    CVMID_icellAssign(ee, threadICell, thisObj);
    CVMID_fieldReadInt(ee, threadICell,
		       CVMoffsetOfjava_lang_Thread_daemon,
		       info->isDaemon);

    CVMassert(info->nativeFunc == NULL || info->isDaemon);

    {
	CVMBool success;
#ifdef CVM_LVM /* %begin lvm */
	if (!CVMLVMeeInitAuxInit(ee, &info->lvmEEInitAux)) {
#ifdef CVM_DEBUG
	    msg = "CVMLVMeeInitAuxInit failed";
#endif
	    goto out_of_memory2;
	}
#endif /* %end lvm */
	success = CVMinitExecEnv(ee, targetEE, info);
#ifdef CVM_LVM /* %begin lvm */
	CVMLVMeeInitAuxDestroy(ee, &info->lvmEEInitAux);
#endif /* %end lvm */
	if (!success) {
#ifdef CVM_DEBUG
	    msg = "CVMinitExecEnv failed";
#endif
	    free(targetEE);  /* CVMdestroyExecEnv already called */
	    targetEE = NULL;
	    goto out_of_memory2;
	}
    }
#ifdef CVM_JVMTI
    if (info->nativeFunc == NULL) {
      /* "Normal" thread vs. system thread */
      /* NOTE: First pass JVMTI support has only one global environment */
      /*      targetEE->_jvmti_env = ee->_jvmti_env; */
    }
#endif

    info->ee = targetEE;

    CVMmutexLock(&info->parentLock);
    info->priority = priority;

    CVMID_fieldReadInt(ee, CVMcurrentThreadICell(ee),
		       CVMoffsetOfjava_lang_Thread_priority,
		       priority);

    {
	CVMBool success = 
	    CVMthreadCreate(&targetEE->threadInfo,
			    CVMglobals.config.nativeStackSize,
			    priority, start_func, info);

	if (!success) {
#ifdef CVM_DEBUG
	    msg = "CVMthreadCreate failed";
#endif
	    CVMmutexUnlock(&info->parentLock);
	    goto out_of_memory2;
	}
    }

    info->started = -1;

    {
	/* CVM.maskInterrupts() in Thread.start() should prevent this */
	CVMassert(!CVMthreadIsInterrupted(CVMexecEnv2threadID(ee), CVM_TRUE));

	while (info->started == -1) {
	    if (!CVMcondvarWait(&info->parentCond, &info->parentLock,
		CVMlongConstZero()))
	    {
		/* Java locking should prevent further interrupts */
		CVMassert(CVM_FALSE);
	    }
	}
    }

    CVMmutexUnlock(&info->parentLock);
#ifdef CVM_JVMTI
    CVMtimeThreadCpuClockInit(&targetEE->threadInfo);
#endif
    /* In case we want to see where threads are created
       CVMdumpStack(&ee->interpreterStack, 0, 0, 0); */

    if (!info->started) {
#ifdef CVM_DEBUG
	msg = "child init failed";
#endif
    }
    
 out_of_memory2:
    CVMcondvarDestroy(&info->parentCond);

 out_of_memory1:
    CVMmutexDestroy(&info->parentLock);

 out_of_memory:

    if (threadICell != NULL) {
	CVMID_freeGlobalRoot(ee, threadICell);
    }
    if (info == NULL || !info->started) {
	if (targetEE != NULL) {
	    CVMdestroyExecEnv(targetEE);
	    free(targetEE);
	}
	CVMthrowOutOfMemoryError(ee, msg);
    }

    if (info != NULL) {
        free(info);
    }
}

/* Purpose: Starts a system thread. */
/* NOTE: Called from parent thread. */
void
JVM_StartSystemThread(JNIEnv *env, jobject thisObj,
		      void(*nativeFunc)(void *), void* nativeFuncArg)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMThreadStartInfo *info =
	(CVMThreadStartInfo *)malloc(sizeof(CVMThreadStartInfo));
    info->nativeFunc = nativeFunc;
    info->nativeFuncArg = nativeFuncArg;
#ifdef CVM_DEBUG_ASSERTS
    CVMassert(ee->nativeRunInfo == NULL);
    CVMassert(ee->nativeRunInfoType == NATIVERUNINFO_UNDEFINED);
    ee->nativeRunInfoType = NATIVERUNINFO_THREAD_START_INFO;
#endif
    ee->nativeRunInfo = info;

    {
	/* Call Thread.start() */
	jclass clazz = CVMcbJavaInstance(CVMsystemClass(java_lang_Thread));
	jmethodID methodID = (*env)->GetMethodID(env, clazz, "start", "()V");
	CVMassert(methodID != NULL);

	(*env)->CallNonvirtualVoidMethod(env, thisObj, clazz, methodID);
    }

#ifdef CVM_DEBUG_ASSERTS
    /* We expect java.lang.Thread.start0() will have freed this up already: */
    CVMassert(ee->nativeRunInfoType == NATIVERUNINFO_UNDEFINED);
    CVMassert(ee->nativeRunInfo == NULL);
#endif
}

#if defined(CVM_HAVE_DEPRECATED) || defined(CVM_THREAD_SUSPENSION)

#ifdef JAVASE
JNIEXPORT void JNICALL
JVM_StopThread(JNIEnv* env, jobject jthread, jobject throwable)
{
    CVMconsolePrintf("unimplemented function JVM_StopThread called!");
    CVMassert(CVM_FALSE);
}
#endif

JNIEXPORT void JNICALL
JVM_SuspendThread(JNIEnv *env, jobject thread)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMThreadICell *thisThreadCell;
    CVMExecEnv *targetEE;
    CVMJavaLong eetopVal;
    CVMBool isSelf;
   
    if (!CVMglobals.suspendCheckerInitialized) {
	CVMsuspendCheckerInit();
	CVMglobals.suspendCheckerInitialized = JNI_TRUE;
    }

    thisThreadCell = CVMcurrentThreadICell(ee);
    CVMID_icellSameObject(ee, thisThreadCell, thread, isSelf);
    if (!isSelf) {
	CVMlocksForThreadSuspendAcquire(ee);

	CVMID_fieldReadLong(ee, thread,
			    CVMoffsetOfjava_lang_Thread_eetop,
			    eetopVal);
	targetEE = (CVMExecEnv *)CVMlong2VoidPtr(eetopVal);

	if (targetEE != NULL) {
            CVMBool success = CVM_FALSE;
	    CVMthreadSuspendConsistentRequest(ee);
	    CVMsysMicroLockAll(ee);
            while (!success) {
	        /* Suspend the target: */
	        targetEE->threadState |= CVM_THREAD_SUSPENDED;
	        CVMthreadSuspend(&targetEE->threadInfo);

                /* Check to see if the target is holding any ciritcal native
                   locks while suspended: */
                success = CVMsuspendCheckerIsOK(ee, targetEE);
                if (!success) {
		    /* If we get here, then the target thread is holding some
                       critical lock.  Resume the thread, back-off for a while
                       and try suspending again a bit later: */
                    CVMthreadResume(&targetEE->threadInfo);
                    targetEE->threadState &= ~CVM_THREAD_SUSPENDED;

                    /* Now sleep for a while to back off before trying to
                       suspend the target thread again: */
                    {
	                CVMSysMonitor mon;
	                if (CVMsysMonitorInit(&mon, NULL, 0)) {
			    jlong millis;
                            
                            /* Sleep for a random amount of non-zero time:
                                  sleepTime = (CVMtimeMillis() & 0xff) + 1;
                            */
			    millis = CVMtimeMillis();
                            millis = CVMlongAnd(millis, CVMint2Long(0xff));
                            millis = CVMlongAdd(millis, CVMlongConstOne());

			    CVMsysMonitorEnter(ee, &mon);
			    CVMsysMonitorWait(ee, &mon, millis);
			    CVMsysMonitorExit(ee, &mon);
			    CVMsysMonitorDestroy(&mon);
			} else {
                            CVMthreadYield();
                        }
		    }
                }
	    }
            CVMsysMicroUnlockAll(ee);
            CVMthreadSuspendConsistentRelease(ee);
	}

        CVMlocksForThreadSuspendRelease(ee);
    } else {
	/* %comment: rt034 */
	ee->threadState |= CVM_THREAD_SUSPENDED;
	CVMthreadSuspend(&ee->threadInfo);
    }
}

JNIEXPORT void JNICALL
JVM_ResumeThread(JNIEnv *env, jobject thread)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMExecEnv *targetEE;
    CVMJavaLong eetopVal;
   
    CVMsysMutexLock(ee, &CVMglobals.threadLock);

    CVMID_fieldReadLong(ee, thread,
			CVMoffsetOfjava_lang_Thread_eetop,
			eetopVal);
    targetEE = (CVMExecEnv *)CVMlong2VoidPtr(eetopVal);

    if (targetEE != NULL) {

	if (targetEE->threadState & CVM_THREAD_SUSPENDED) {
	    CVMassert(targetEE != ee);
	    CVMthreadResume(&targetEE->threadInfo);
	    targetEE->threadState &= ~CVM_THREAD_SUSPENDED;
	}
    }
    CVMsysMutexUnlock(ee, &CVMglobals.threadLock);
}

#endif /* CVM_HAVE_DEPRECATED || CVM_THREAD_SUSPENSION */

/* We want to rely on priorities as set/got via the threadX functions.
 * However, it is legal to call setPriority() and getPriority() after
 * 'new' and before start() is called, in which case the thread's TID has
 * been allocated but its thread not yet created.  So we also maintain
 * tid->priority to bridge the gap.
 *
 * The priority should already have been check to fall within a
 * valid range by the software above.  All we do is pass it on to
 * the system level.  The JAVA level also set the priority field.
 */
JNIEXPORT void JNICALL
JVM_SetThreadPriority(JNIEnv *env, jobject thread, jint prio)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMExecEnv *targetEE;
    CVMJavaLong eetopVal;
   
    CVMsysMutexLock(ee, &CVMglobals.threadLock);

    CVMID_fieldReadLong(ee, thread,
			CVMoffsetOfjava_lang_Thread_eetop,
			eetopVal);
    targetEE = (CVMExecEnv *)CVMlong2VoidPtr(eetopVal);

    if (targetEE != NULL) {
	CVMthreadSetPriority(&targetEE->threadInfo, prio);
#ifdef CVM_INSPECTOR
	targetEE->priority = prio;
#endif
    }

    CVMsysMutexUnlock(ee, &CVMglobals.threadLock);
}

JNIEXPORT void JNICALL
JVM_Yield(JNIEnv *env, jclass threadClass)
{
    CVMthreadYield();
}

JNIEXPORT void JNICALL
JVM_Sleep(JNIEnv *env, jclass threadClass, jlong millis)
{
#ifdef JDK12
    if (CVMlongLtz(millis)) {
	ThrowIllegalArgumentException(CVMjniEnv2ExecEnv(env),
	    "timeout value is negative");
	return;
    }
#endif

    {
	CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
	if (CVMlongEqz(millis)) {
	    CVMthreadYield();
	} else {
	    CVMWaitStatus st;
	    CVMSysMonitor mon;
	    if (CVMsysMonitorInit(&mon, NULL, 0)) {
		CVMsysMonitorEnter(ee, &mon);
		ee->threadState = CVM_THREAD_SLEEPING | CVM_THREAD_WAITING |
		    CVM_THREAD_WAITING_TIMEOUT;
		st = CVMsysMonitorWait(ee, &mon, millis);
		ee->threadState &= ~(CVM_THREAD_SLEEPING | CVM_THREAD_WAITING |
				     CVM_THREAD_WAITING_TIMEOUT);
		CVMsysMonitorExit(ee, &mon);
		CVMsysMonitorDestroy(&mon);
		if (st == CVM_WAIT_INTERRUPTED) {
		    CVMthrowInterruptedException(ee, "operation interrupted");
		}
	    } else {
		CVMthrowOutOfMemoryError(ee, "out of monitor resources");
	    }
	}
    }
    return;
}

JNIEXPORT jobject JNICALL
JVM_CurrentThread(JNIEnv *env, jclass threadClass)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    return (*env)->NewLocalRef(env, CVMcurrentThreadICell(ee));
}

#ifdef CVM_HAVE_DEPRECATED

/* Unlike JDK, this implementation counts native methods. */
JNIEXPORT jint JNICALL
JVM_CountStackFrames(JNIEnv *env, jobject thread)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);

    if (thread == NULL) {
	CVMthrowNullPointerException(ee, 0);
	return -1;
    } else {
	CVMExecEnv *targetEE;
	CVMJavaLong eetopVal;
	jint count = 0;
   
	CVMsysMutexLock(ee, &CVMglobals.threadLock);

	CVMID_fieldReadLong(ee, thread,
			    CVMoffsetOfjava_lang_Thread_eetop,
			    eetopVal);
	targetEE = (CVMExecEnv *)CVMlong2VoidPtr(eetopVal);

	if (targetEE != NULL) {
	    CVMFrame *frame = CVMeeGetCurrentFrame(targetEE);
	    CVMFrameIterator iter;
	    CVMframeIterateInit(&iter, frame);
	    while (CVMframeIterateNext(&iter)) {
		++count;
	    }
	} else {
	    /* Thread hasn't any state yet. */
	}
	CVMsysMutexUnlock(ee, &CVMglobals.threadLock);

	return count;
    }
}

#endif /* CVM_HAVE_DEPRECATED */

JNIEXPORT void JNICALL
Java_java_lang_Thread_interrupt0(JNIEnv *env, jobject thread)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMExecEnv *targetEE;
    CVMJavaLong eetopVal;
   
    CVMsysMutexLock(ee, &CVMglobals.threadLock);

    CVMID_fieldReadLong(ee, thread,
			CVMoffsetOfjava_lang_Thread_eetop,
			eetopVal);
    targetEE = (CVMExecEnv *)CVMlong2VoidPtr(eetopVal);

    /* %comment: rt035 */
    if (targetEE != NULL) {
	if (!targetEE->interruptsMasked) {
	    targetEE->threadState |= CVM_THREAD_INTERRUPTED;
	    CVMthreadInterruptWait(CVMexecEnv2threadID(targetEE));
	} else {
	    targetEE->maskedInterrupt = CVM_TRUE;
	}
    }

    CVMsysMutexUnlock(ee, &CVMglobals.threadLock);
}

JNIEXPORT void JNICALL
JVM_Interrupt(JNIEnv *env, jobject thread)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMJavaLong eetopVal;
    CVMExecEnv *targetEE;
    jmethodID methodID;

    CVMID_fieldReadLong(ee, thread,
			CVMoffsetOfjava_lang_Thread_eetop,
			eetopVal);
    targetEE = (CVMExecEnv *)CVMlong2VoidPtr(eetopVal);

    /* %comment: rt035 */
    if (targetEE != NULL) {
        methodID =
            CVMjniGetMethodID(env,
                CVMcbJavaInstance(CVMsystemClass(java_lang_Thread)),
                "interrupt", "()V");
        if (methodID == NULL) {
            return;
        }        
        (*env)->CallVoidMethod(env, CVMcurrentThreadICell(targetEE),
                               methodID);
    }
}

JNIEXPORT jboolean JNICALL
JVM_IsInterrupted(JNIEnv *env, jobject thread, jboolean clearInterrupted)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMExecEnv *targetEE;
    CVMThreadICell *thisThreadCell;
    CVMJavaLong eetopVal;
    jboolean result = JNI_FALSE;
    CVMBool isSelf;

    thisThreadCell = CVMcurrentThreadICell(ee);
    CVMID_icellSameObject(ee, thisThreadCell, thread, isSelf);

    if (isSelf) {
	/*
	 * The thread is the current thread, so we can avoid the
	 * locking because we know we aren't going away.
	 *
	 * Current thread will not see any interrupts while
	 * interrupts are masked.
	 */
	result = !ee->interruptsMasked &&
	    CVMthreadIsInterrupted(CVMexecEnv2threadID(ee), clearInterrupted);
	ee->threadState &= ~CVM_THREAD_INTERRUPTED;
    } else {
	/* a thread can only clear its own interrupt */
	CVMassert(!clearInterrupted);
	CVMsysMutexLock(ee, &CVMglobals.threadLock);
	CVMID_fieldReadLong(ee, thread,
			    CVMoffsetOfjava_lang_Thread_eetop,
			    eetopVal);
	if (!CVMlongEqz(eetopVal)) {
	    targetEE = (CVMExecEnv *)CVMlong2VoidPtr(eetopVal);
	    result = !targetEE->interruptsMasked ?
		CVMthreadIsInterrupted(CVMexecEnv2threadID(targetEE),
		    clearInterrupted)
		: targetEE->maskedInterrupt;
	}
	CVMsysMutexUnlock(ee, &CVMglobals.threadLock);
    }
    return result;
}

#ifndef CDC_10
/* Purpose: Checks to see if the current thread owns the lock on the specified
   object. */
JNIEXPORT jboolean JNICALL
JVM_HoldsLock(JNIEnv *env, jclass threadClass, jobject objICell)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    jboolean result;

    if (objICell == NULL) {
	CVMthrowNullPointerException(ee, NULL);
	return CVM_FALSE;
    }

    CVMD_gcUnsafeExec(ee, {
        result = CVMobjectLockedByCurrentThread(ee, objICell);
    });
    return result;
}
#endif /* !CDC_10 */

/*
 * SecurityManager support
 */

/* 
 * Implemented in startup code.
 *     bool_t InitializeSystemClassLoader(void)
 *
 * Implemented in interpreter.c as CVMisTrustedClassLoader().
 *     bool_t IsTrustedClassLoader(CVMClassLoaderICell *loader)
 */

#undef TRUSTED_CLASSLOADER
#ifdef CVM_CLASSLOADING
#define TRUSTED_CLASSLOADER(ee, loader)	\
	CVMisTrustedClassLoader((ee), (loader))
#else
#define TRUSTED_CLASSLOADER(ee, loader)	\
	(CVMassert((loader) == NULL), CVM_TRUE)
#endif

/*
 * If "frame" is a doPrivileged0 frame, then check if the
 * caller frame is trusted.
 */

#ifdef CVM_HAVE_DEPRECATED 

/* NOTE: is_trusted_frame() advances the frame iterator if we encounter a
   doPrivileged() frame. */
static CVMBool
is_trusted_frame(CVMExecEnv *ee, CVMFrameIterator *iter)
{
    CVMClassBlock *cb;
    CVMMethodBlock *mb;

    /* doPrivileged MBs: */
    CVMMethodBlock *doPrivilegedAction1Mb =
	CVMglobals.java_security_AccessController_doPrivilegedAction1;
    CVMMethodBlock *doPrivilegedExceptionAction1Mb =
	CVMglobals.java_security_AccessController_doPrivilegedExceptionAction1;
    CVMMethodBlock *doPrivilegedAction2Mb =
	CVMglobals.java_security_AccessController_doPrivilegedAction2;
    CVMMethodBlock *doPrivilegedExceptionAction2Mb =
	CVMglobals.java_security_AccessController_doPrivilegedExceptionAction2;

    mb = CVMframeIterateGetMb(iter);
    if (mb == doPrivilegedAction2Mb ||
	mb == doPrivilegedExceptionAction2Mb) {

	CVMMethodBlock *callerMb;
	CVMClassLoaderICell *loader;

	/* Get the caller frame of doPrivileged(): */
	CVMframeIterateNext(iter);
	callerMb = CVMframeIterateGetMb(iter);

	/* If the caller is one of the outher doPrivileged() methods, then
	   refetch the caller from before them: */
	if (callerMb == doPrivilegedAction1Mb ||
	    callerMb == doPrivilegedExceptionAction1Mb ) {
	    CVMframeIterateNext(iter);
	    callerMb = CVMframeIterateGetMb(iter);
	}

	/* Check to see if the classloader of this callerMb is trusted: */
	cb = CVMmbClassBlock(callerMb);
	if (cb != NULL && ((loader = CVMcbClassLoader(cb)) == NULL ||
	    TRUSTED_CLASSLOADER(ee, loader)))
	{
	    return CVM_TRUE;
	}
    }
    return CVM_FALSE;
}

/* Utility function used only by JVM_CurrentLoadedClass() and
   JVM_CurrentClassLoader() below. */
static CVMClassBlock *
current_loaded_class(JNIEnv *env)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMFrame *frame = CVMeeGetCurrentFrame(ee);
    CVMClassBlock *cb;
    CVMMethodBlock *mb;
    CVMClassLoaderICell *loader;
    CVMFrameIterator iter;

    CVMframeIterateInit(&iter, frame);
    while (CVMframeIterateNext(&iter)) {
	/* If a method in a class in a trusted loader is in a doPrivileged,
	   return NULL: */
	if (is_trusted_frame(ee, &iter)) {
	    return NULL;
	}
	mb = CVMframeIterateGetMb(&iter);
	if (!CVMmbIs(mb, NATIVE)) {
	    cb = CVMmbClassBlock(mb);
	    CVMassert(cb != NULL);
	    loader = CVMcbClassLoader(cb);
	    if ((loader != NULL) && !TRUSTED_CLASSLOADER(ee, loader)) {
	        return cb;
	    }
        }
    }
    return NULL;
}

JNIEXPORT jclass JNICALL
JVM_CurrentLoadedClass(JNIEnv *env)
{
    CVMClassBlock *cb = current_loaded_class(env);
    return (cb == NULL) ? NULL
	   : (*env)->NewLocalRef(env, CVMcbJavaInstance(cb));
}

JNIEXPORT jobject JNICALL
JVM_CurrentClassLoader(JNIEnv *env)
{
    CVMClassBlock *cb = current_loaded_class(env);
    return (cb == NULL) ? NULL
	   : (*env)->NewLocalRef(env, CVMcbClassLoader(cb));
}
#endif /* CVM_HAVE_DEPRECATED */


JNIEXPORT jobjectArray JNICALL
JVM_GetClassContext(JNIEnv *env)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMFrame* frame;
    int n;
    jobjectArray result;
    CVMClassBlock* java_lang_class_arrayCb = 
	CVMclassGetArrayOf(ee, CVMsystemClass(java_lang_Class));
    CVMFrameIterator iter;

    /* count the classes on the execution stack */
    frame = CVMeeGetCurrentFrame(ee);

    CVMframeIterateInit(&iter, frame);

    /* skip current frame (SecurityManager.getClassContext) */
    CVMframeIterateNext(&iter);
    n = CVMframeIterateCount(&iter);

    /* allocate the Class array */
    CVMID_localrootBegin(ee) {
	CVMID_localrootDeclare(CVMObjectICell, classContextICell);
	CVMID_allocNewArray(ee, CVM_T_CLASS, java_lang_class_arrayCb,
			    n, classContextICell);
	/* The allocation nulls out the contents */

	if (!CVMID_icellIsNull(classContextICell)) {
	    result = (*env)->NewLocalRef(env, classContextICell);
	} else {
	    result = NULL;
	    CVMthrowOutOfMemoryError(ee, NULL);
	}
    } CVMID_localrootEnd();
    if (result == NULL) {
	return NULL;
    }

    /* fill in the Class array */
    {
	CVMArrayOfRefICell* classArray = (CVMArrayOfRefICell*)result;
	frame = CVMeeGetCurrentFrame(ee);

	CVMframeIterateInit(&iter, frame);

	/* skip current frame (SecurityManager.getClassContext) */
	CVMframeIterateNext(&iter);
	n = 0;
	while (CVMframeIterateNext(&iter)) {
	    CVMMethodBlock *mb = CVMframeIterateGetMb(&iter);
	    CVMClassICell* clas = 
		CVMcbJavaInstance(CVMmbClassBlock(mb));
	    CVMID_arrayWriteRef(ee, classArray, n, clas);
	    n++;
	}
    }
    
    return result;
}

#ifdef CVM_HAVE_DEPRECATED

JNIEXPORT jint JNICALL
JVM_ClassDepth(JNIEnv *env, jstring name)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMFrame *frame = CVMeeGetCurrentFrame(ee);
    CVMClassBlock *cb;
    CVMMethodBlock *mb;
    CVMFrameIterator iter;
    jint depth = 0;
    char *utfName;
    CVMClassTypeID classID;
    jsize strLen;
    jsize utfLen;

    /* Lookup the typeid of the specified Class name: */
    strLen = CVMjniGetStringLength(env, name);
    utfLen = CVMjniGetStringUTFLength(env, name);

    utfName = (char *)malloc(utfLen + 1);
    if (utfName == NULL) {
        CVMthrowOutOfMemoryError(ee, NULL);
	return -1;
    }
    CVMjniGetStringUTFRegion(env, name, 0, strLen, utfName);

    VerifyFixClassname(utfName);
    classID = CVMtypeidLookupClassID(ee, utfName, (int)utfLen);
    free((void *)utfName);

    /* If there is no class in the system already loaded with this name, then
       no need to search further: */
    if (classID == CVM_TYPEID_ERROR) {
	return -1;
    }

    CVMframeIterateInit(&iter, frame);
    while (CVMframeIterateNext(&iter)) {
	mb = CVMframeIterateGetMb(&iter);
	if (!CVMmbIs(mb, NATIVE)) {
	    cb = CVMmbClassBlock(mb);
	    CVMassert(cb != NULL);
	    if (CVMcbClassName(cb) == classID) {
	        return depth;
	    }
	    depth++;
        }
    }
    return -1;
}

JNIEXPORT jint JNICALL
JVM_ClassLoaderDepth(JNIEnv *env)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMFrame *frame = CVMeeGetCurrentFrame(ee);
    CVMClassBlock *cb;
    CVMMethodBlock *mb;
    CVMClassLoaderICell *loader;
    CVMFrameIterator iter;
    jint depth = 0;

    CVMframeIterateInit(&iter, frame);
    while (CVMframeIterateNext(&iter)) {
	/* if a method in a class in a trusted loader is in a doPrivileged,
	   return -1 */
	if (is_trusted_frame(ee, &iter)) {
	    return -1;
	}
	mb = CVMframeIterateGetMb(&iter);
	if (!CVMmbIs(mb, NATIVE)) {
	    cb = CVMmbClassBlock(mb);
	    CVMassert(cb != NULL);
	    loader = CVMcbClassLoader(cb);
	    if (loader != NULL && !TRUSTED_CLASSLOADER(ee, loader)) {
	        return depth;
	    }
	    depth++;
        }
    }
    return -1;
}

#endif /* CVM_HAVE_DEPRECATED */

/*
 * The following functions from ObjectInputStream.c
 * cannot be done with JNI or reflection
 *
 * (should we rethink how these might be implemented
 * using the already-existing mechanisms?)
 */

#define CVM_JAVA_IO_PACKAGE "java/io/"

/*
 * Allocate an initialized object of the specified class.
 *
 * See ObjectInputStream.java, inputObject, for more details. This
 * function seems a little strange because currClass, the actual class
 * of the object, does not necessarily have to be the same as
 * initClass, the class from which the default no-argument constructor
 * is called. The explanation is in the middle of initObject: if on
 * the receiving machine the class to be instantiated (call it B) is
 * not declared Serializable, the VM walks up the class hierarchy to
 * find class A, the most specialized superclass declaring
 * serializability. B is allocated, but A's no-argument constructor is
 * the constructor called; this has the effect of setting the rest of
 * B's fields to their default values.
 */
JNIEXPORT jobject JNICALL
JVM_AllocateNewObject(JNIEnv *env, jobject thisObj, jclass currClass,
		      jclass initClass)
{
#ifdef CVM_SERIALIZATION
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock* cb;
    CVMClassBlock* initCb;
    CVMMethodBlock* mb = NULL;
    int len;
    CVMMethodTypeID noArgConstructorTid;
    jobject result;

    cb = CVMgcSafeClassRef2ClassBlock(ee, currClass);
    initCb = CVMgcSafeClassRef2ClassBlock(ee, initClass);

    if (CVMcbIs(cb, INTERFACE) || CVMcbIs(cb, ABSTRACT) ||
	cb == CVMsystemClass(java_lang_Class)) {
	CVMthrowInstantiationException(ee, "%C", cb);
	return NULL;
    }

    noArgConstructorTid = CVMglobals.initTid;

    for (len = CVMcbMethodCount(initCb) - 1; len >= 0; len--) {
	mb = CVMcbMethodSlot(initCb, len);
	if (CVMtypeidIsSame(CVMmbNameAndTypeID(mb), noArgConstructorTid)) {
	    break;
	}
    }
    if (len < 0) { 
	CVMthrowNoSuchMethodError(ee, "<init>");
	return NULL;
    }

    if (cb == initCb && !CVMmbIs(mb, PUBLIC)) {
	/* Calling the constructor for class 'cb'. 
	 * Only allow calls to a public no-arg constructor.
	 * This path corresponds to creating an Externalizable object.
	 */
	CVMthrowIllegalAccessException(ee, "%M", mb);
	return NULL;
    }
    else if (!CVMverifyMemberAccess(ee, cb, initCb, CVMmbAccessFlags(mb),
				    CVM_FALSE)) {
	/* subclass 'cb' does not have access to no-arg constructor of
           'initCb'.*/
	CVMthrowIllegalAccessException(ee, "%M", mb);
	return NULL;
    }

    result = CVMjniCreateLocalRef(ee);
    if (result == NULL) {
	return NULL;  /* exception already thrown */
    }

    /* Allocate the object  */
    CVMID_allocNewInstance(ee, cb, result);
    if (CVMID_icellIsNull(result)) {
	(*env)->DeleteLocalRef(env, result);
	CVMthrowOutOfMemoryError(ee,
				 "allocating receiver-side serialized object");
        return NULL;
    }

    /* Invoke the no-arg constructor */
    (*env)->CallVoidMethod(env, result, mb);

    if ((*env)->ExceptionCheck(env)) {
	/* If the constructor threw an exception, we won't need the return
	   local ref. Delete, and return NULL. */
	(*env)->DeleteLocalRef(env, result);
	return NULL;
    }

    return result;
#else /* CVM_SERIALIZATION */
    CVMthrowUnsupportedOperationException(CVMjniEnv2ExecEnv(env), NULL);
    return NULL;
#endif /* CVM_SERIALIZATION */
}

/*
 * Allocate a new array of the specified length.
 *
 * currClass is the Java class corresponding to the desired array type.
 *
 * Does not initialize the array's cells.
 */
JNIEXPORT jobject JNICALL
JVM_AllocateNewArray(JNIEnv *env, jobject thisObj, jclass currClass,
		     jint length)
{
#ifdef CVM_SERIALIZATION

    /* NOTE that this depends on reflection. This is okay because
       object serialization is meaningless without reflection support.
       In addition, our makefiles make sure to set CVM_REFLECT when
       CVM_SERIALIZATION is set. */

    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock* cb = CVMgcSafeClassRef2ClassBlock(ee, currClass);
    jobject result;

    if (!CVMisArrayClass(cb)) {
	CVMthrowInvalidClassException(ee, "%C", cb);
	return NULL;
    }
    result = CVMjniCreateLocalRef(ee);
    if (result == NULL) {
	return NULL;  /* exception already thrown */
    }
    CVMreflectNewArray(ee, CVMarrayElementCb(cb),
		       length, (CVMArrayOfAnyTypeICell*) result);
    if (CVMID_icellIsNull(result)) {
	(*env)->DeleteLocalRef(env, result);
	return NULL;  /* exception already thrown */
    }
    return result;
#else /* CVM_SERIALIZATION */
    CVMthrowUnsupportedOperationException(CVMjniEnv2ExecEnv(env), NULL);
    return NULL;
#endif /* CVM_SERIALIZATION */
}

/*
 * Return the first non-null class loader up the execution stack, or null
 * if only code from the null class loader is on the stack.
 */
JNIEXPORT jobject JNICALL
JVM_LatestUserDefinedLoader(JNIEnv *env)
{
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMFrame *frame = CVMeeGetCurrentFrame(ee);

    while (frame != NULL) {
	if (frame->mb != NULL) {
	    CVMClassLoaderICell *loader =
	        CVMcbClassLoader(CVMmbClassBlock(frame->mb));
	    if (loader != NULL) {
		return (*env)->NewLocalRef(env, loader);
	    }
	}
	frame = CVMframePrev(frame);
    }

    return NULL;
}

/*
 * Code supporting reflection: used by Array.c
 */

JNIEXPORT jint JNICALL
JVM_GetArrayLength(JNIEnv *env, jobject arr)
{
#ifdef CVM_REFLECT
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock* arrayCb;
    CVMObjectICell* arrObj = (CVMObjectICell*) arr;
    CVMJavaInt arrLen;
    
    if ((arrObj == NULL) || (CVMID_icellIsNull(arrObj))) {
	CVMthrowNullPointerException(ee, "Array.getLength()");
	return 0;
    }

    CVMID_objectGetClass(ee, arrObj, arrayCb);
    if (!CVMisArrayClass(arrayCb)) {
	CVMthrowIllegalArgumentException(ee, "object not of array type");
	return 0;
    }

    CVMID_arrayGetLength(ee, (CVMArrayOfAnyTypeICell*) arrObj, arrLen);

    return arrLen;
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(CVMjniEnv2ExecEnv(env), NULL);
    return 0;
#endif /* CVM_REFLECT */
}

JNIEXPORT jobject JNICALL
JVM_GetArrayElement(JNIEnv *env, jobject arr, jint index)
{
#ifdef CVM_REFLECT
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock* arrayCb;
    CVMClassBlock* elementCb;
    CVMArrayOfAnyTypeICell* arrObj = (CVMArrayOfAnyTypeICell*) arr;
    CVMObjectICell* result;
    CVMJavaInt arrLen;

    if ((arrObj == NULL) || (CVMID_icellIsNull(arrObj))) {
	CVMthrowNullPointerException(ee, "Array.get()");
	return NULL;
    }

    CVMID_objectGetClass(ee, (CVMObjectICell*) arrObj, arrayCb);
    if (!CVMisArrayClass(arrayCb)) {
	CVMthrowIllegalArgumentException(ee, "object not of array type");
	return NULL;
    }
    
    elementCb = CVMarrayElementCb(arrayCb);
    CVMassert(elementCb != NULL);

    result = CVMjniCreateLocalRef(ee);
    if (result == NULL) {
	return NULL;
    }

    if (CVMcbIs(elementCb, PRIMITIVE)) {
	jvalue v = JVM_GetPrimitiveArrayElement(env, arr, index,
						CVMcbBasicTypeCode(elementCb));
	if (CVMexceptionOccurred(ee) | 
	    !CVMgcSafeJavaWrap(ee, v, CVMcbBasicTypeCode(elementCb), result)) {
	    (*env)->DeleteLocalRef(env, result);
	    return NULL;
	}
    } else {
	/* This clever idea is from ExactVM. No need to test index >= 0.
	   Instead, do an unsigned comparison. Negative indices will
	   appear as positive numbers >= 2^31, which is just out of the
	   range of legal Java array indices */
	CVMID_arrayGetLength(ee, arrObj, arrLen);
	if ((CVMUint32)index >= (CVMUint32)arrLen) {
	    CVMthrowArrayIndexOutOfBoundsException(ee,
						   "accessing array element");
	    (*env)->DeleteLocalRef(env, result);
	    return NULL;
	}

	CVMID_arrayReadRef(ee, arrObj, index, result);
	if (CVMID_icellIsNull(result)) {
	    (*env)->DeleteLocalRef(env, result);
	    result = NULL;
	}
    }
    return result;
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(CVMjniEnv2ExecEnv(env), NULL);
    return NULL;
#endif /* CVM_REFLECT */
}

JNIEXPORT jvalue JNICALL
JVM_GetPrimitiveArrayElement(JNIEnv *env, jobject arr, jint index, jint wCode)
{
#ifdef CVM_REFLECT
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock* arrayCb;
    CVMClassBlock* elementCb;
    CVMArrayOfAnyTypeICell* arrObj = (CVMArrayOfAnyTypeICell*) arr;
    CVMJavaInt arrLen;
    CVMBasicType fromType;
    CVMBasicType toType = (CVMBasicType) wCode;
    jvalue v;

    /* Initialize v so compiler doesn't complain */
    v.i = 0;

    if ((arrObj == NULL) || (CVMID_icellIsNull(arrObj))) {
	CVMthrowNullPointerException(ee, "Array.getPrimitiveElement()");
	return v;
    }

    CVMID_objectGetClass(ee, (CVMObjectICell*) arrObj, arrayCb);
    if (!CVMisArrayClass(arrayCb)) {
	CVMthrowIllegalArgumentException(ee, "object not of array type");
	return v;
    }
    
    elementCb = CVMarrayElementCb(arrayCb);
    CVMassert(elementCb != NULL);

    /* This clever idea is from ExactVM. No need to test index >= 0.
       Instead, do an unsigned comparison. Negative indices will
       appear as positive numbers >= 2^31, which is just out of the
       range of legal Java array indices */
    CVMID_arrayGetLength(ee, arrObj, arrLen);
    if ((CVMUint32)index >= (CVMUint32)arrLen) {
	CVMthrowArrayIndexOutOfBoundsException(ee, "accessing array element");
	return v;
    }

    if (!CVMcbIs(elementCb, PRIMITIVE)) {
	CVMthrowIllegalArgumentException(ee,
					 "array object does not contain "
					 "elements of primitive type");
	return v;
    }

    fromType = CVMcbBasicTypeCode(elementCb);
    switch (fromType) {
    case CVM_T_BOOLEAN:
	CVMID_arrayReadBoolean(ee, (CVMArrayOfBooleanICell*) arrObj,
			       index, v.z);
	break;
    case CVM_T_CHAR:
	CVMID_arrayReadChar(ee, (CVMArrayOfCharICell*) arrObj,
			    index, v.c);
	break;
    case CVM_T_FLOAT:
	CVMID_arrayReadFloat(ee, (CVMArrayOfFloatICell*) arrObj,
			     index, v.f);
	break;
    case CVM_T_DOUBLE:
	CVMID_arrayReadDouble(ee, (CVMArrayOfDoubleICell*) arrObj,
			      index, v.d);
	break;
    case CVM_T_BYTE:
	CVMID_arrayReadByte(ee, (CVMArrayOfByteICell*) arrObj,
			    index, v.b);
	break;
    case CVM_T_SHORT:
	CVMID_arrayReadShort(ee, (CVMArrayOfShortICell*) arrObj,
			     index, v.s);
	break;
    case CVM_T_INT:
	CVMID_arrayReadInt(ee, (CVMArrayOfIntICell*) arrObj,
			   index, v.i);
	break;
    case CVM_T_LONG:
	CVMID_arrayReadLong(ee, (CVMArrayOfLongICell*) arrObj,
			    index, v.j);
	break;
    default:
	CVMassert(CVM_FALSE);
	return v;
    }
    
    if (fromType != toType)
	REFLECT_WIDEN(v, fromType, toType, fail);
    
    return v;

fail:
    CVMthrowIllegalArgumentException(ee, "widening primitive value");
    return v;
#else /* CVM_REFLECT */
    jvalue v;
    v.i = 0;
    CVMthrowUnsupportedOperationException(CVMjniEnv2ExecEnv(env), NULL);
    return v;
#endif /* CVM_REFLECT */
}

JNIEXPORT void JNICALL
JVM_SetArrayElement(JNIEnv *env, jobject arr, jint index, jobject val)
{
#ifdef CVM_REFLECT
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock* arrayCb;
    CVMClassBlock* elementCb;
    CVMArrayOfAnyTypeICell* arrObj = (CVMArrayOfAnyTypeICell*) arr;
    CVMJavaInt arrLen;
    
    if ((arrObj == NULL) || (CVMID_icellIsNull(arrObj))) {
	CVMthrowNullPointerException(ee, "Array.set()");
	return;
    }

    CVMID_objectGetClass(ee, (CVMObjectICell*) arrObj, arrayCb);
    if (!CVMisArrayClass(arrayCb)) {
	CVMthrowIllegalArgumentException(ee, "object not of array type");
	return;
    }
    
    elementCb = CVMarrayElementCb(arrayCb);
    CVMassert(elementCb != NULL);

    if (CVMcbIs(elementCb, PRIMITIVE)) {
	jvalue v;
	CVMBasicType fromType;

	if (CVMgcSafeJavaUnwrap(ee, (CVMObjectICell*) val,
				&v, &fromType, NULL))
	    JVM_SetPrimitiveArrayElement(env, arr, index, v, fromType);
    } else {
	/* See whether the object is of the appropriate type */
	CVMBool isOfCorrectType;

	if (val != NULL) {
	    CVMD_gcUnsafeExec(ee, {
		CVMObject* directObj = CVMID_icellDirect(ee, val);
		isOfCorrectType = CVMgcUnsafeIsInstanceOf(ee, directObj,
							  elementCb);
	    });
	    
	    if (!isOfCorrectType) {
		CVMthrowIllegalArgumentException(ee, "argument object is not "
						 "an instance of the array's "
						 "element type");
		return;
	    }
	}

	/* This clever idea is from ExactVM. No need to test index >= 0.
	   Instead, do an unsigned comparison. Negative indices will
	   appear as positive numbers >= 2^31, which is just out of the
	   range of legal Java array indices */
	CVMID_arrayGetLength(ee, arrObj, arrLen);
	if ((CVMUint32)index >= (CVMUint32)arrLen) {
	    CVMthrowArrayIndexOutOfBoundsException(ee,
						   "accessing array element");
	    return;
	}
	if (val != NULL) {
	    CVMID_arrayWriteRef(ee, arrObj, index, val);
	} else {
	    CVMID_localrootBegin(ee) {
		CVMID_localrootDeclare(CVMObjectICell, nullICell);
		CVMID_icellSetNull(nullICell);
		CVMID_arrayWriteRef(ee, arrObj, index, nullICell);
	    } CVMID_localrootEnd();
	}
    }
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(CVMjniEnv2ExecEnv(env), NULL);
#endif /* CVM_REFLECT */
}

JNIEXPORT void JNICALL
JVM_SetPrimitiveArrayElement(JNIEnv *env, jobject arr, jint index, jvalue v,
			     CVMBasicType vCode)
{
#ifdef CVM_REFLECT
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock* arrayCb;
    CVMClassBlock* elementCb;
    CVMArrayOfAnyTypeICell* arrObj = (CVMArrayOfAnyTypeICell*) arr;
    CVMJavaInt arrLen;
    CVMBasicType toType;
    
    if ((arrObj == NULL) || (CVMID_icellIsNull(arrObj))) {
	CVMthrowNullPointerException(ee, "Array.setPrimitiveElement()");
	return;
    }

    CVMID_objectGetClass(ee, (CVMObjectICell*) arrObj, arrayCb);
    if (!CVMisArrayClass(arrayCb)) {
	CVMthrowIllegalArgumentException(ee, "object not of array type");
	return;
    }
    
    elementCb = CVMarrayElementCb(arrayCb);
    CVMassert(elementCb != NULL);
    if (!CVMcbIs(elementCb, PRIMITIVE)) {
	CVMthrowIllegalArgumentException(ee, "illegal array element type");
	return;
    }

    /* This clever idea is from ExactVM. No need to test index >= 0.
       Instead, do an unsigned comparison. Negative indices will
       appear as positive numbers >= 2^31, which is just out of the
       range of legal Java array indices */
    CVMID_arrayGetLength(ee, arrObj, arrLen);
    if ((CVMUint32)index >= (CVMUint32)arrLen) {
	CVMthrowArrayIndexOutOfBoundsException(ee,
					       "accessing array element");
	return;
    }

    toType = CVMcbBasicTypeCode(elementCb);

    if (vCode != toType)
	REFLECT_WIDEN(v, vCode, toType, fail);

    switch (toType) {
    case CVM_T_BOOLEAN:
	CVMID_arrayWriteBoolean(ee, (CVMArrayOfBooleanICell*) arrObj,
				index, v.b);
	break;
    case CVM_T_CHAR:
	CVMID_arrayWriteChar(ee, (CVMArrayOfCharICell*) arrObj,
			     index, v.c);
	break;
    case CVM_T_FLOAT:
	CVMID_arrayWriteFloat(ee, (CVMArrayOfFloatICell*) arrObj,
			      index, v.f);
	break;
    case CVM_T_DOUBLE:
	CVMID_arrayWriteDouble(ee, (CVMArrayOfDoubleICell*) arrObj,
			       index, v.d);
	break;
    case CVM_T_BYTE:
	CVMID_arrayWriteByte(ee, (CVMArrayOfByteICell*) arrObj,
			     index, v.b);
	break;
    case CVM_T_SHORT:
	CVMID_arrayWriteShort(ee, (CVMArrayOfShortICell*) arrObj,
			      index, v.s);
	break;
    case CVM_T_INT:
	CVMID_arrayWriteInt(ee, (CVMArrayOfIntICell*) arrObj,
			    index, v.i);
	break;
    case CVM_T_LONG:
	CVMID_arrayWriteLong(ee, (CVMArrayOfLongICell*) arrObj,
			     index, v.j);
	break;
    default:
	CVMassert(CVM_FALSE);
    }

    return;

fail:
    CVMthrowIllegalArgumentException(ee, "widening primitive value");
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(CVMjniEnv2ExecEnv(env), NULL);
#endif /* CVM_REFLECT */
}

JNIEXPORT jobject JNICALL
JVM_NewArray(JNIEnv *env, jclass eltClass, jint length)
{
#ifdef CVM_REFLECT
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    /* 
     * Access member variable of type 'int'
     * and cast it to a native pointer. The java type 'int' only guarantees
     * 32 bit, but because one slot is used as storage space and
     * a slot is 64 bit on 64 bit platforms, it is possible 
     * to store a native pointer without modification of
     * java source code. This assumes that all places in the C-code
     * which set/get this member are caught.
     */
    CVMAddr cbAddr;
    CVMClassBlock* cb;
    CVMObjectICell* classObj = (CVMObjectICell*) eltClass;
    jobject result;
    
    if ((classObj == NULL) || (CVMID_icellIsNull(classObj))) {
	CVMthrowNullPointerException(ee, "null class object");
	return NULL;
    }
    
    if (length < 0) {
	CVMthrowNegativeArraySizeException(ee, NULL);
	return NULL;
    }
    
    CVMID_fieldReadAddr(ee, eltClass,
			CVMoffsetOfjava_lang_Class_classBlockPointer,
			cbAddr);
    cb = (CVMClassBlock*) cbAddr;
    CVMassert(cb != NULL);

    if (CVMisArrayClass(cb)) {
	/* Check for out-of-bounds array dimensions */
	int eltDims = CVMarrayDepth(cb);
	/* If eltDims is already 255, the we can't go on to create
	 * something of dims 256. */
	
	if (eltDims >= 255) {
	    CVMassert(eltDims == 255); /* arrays always have 255 or fewer
				        * dimensions */
	    CVMthrowIllegalArgumentException(ee, "too many dimensions");
	    return NULL;
	}
    }
    
    result = (*env)->NewLocalRef(env, CVMcurrentThreadICell(ee));
    if (result == NULL) {
	return NULL;
    }

    CVMreflectNewArray(ee, cb, length, (CVMArrayOfAnyTypeICell*)result);

    if (CVMexceptionOccurred(ee)) {
	(*env)->DeleteLocalRef(env, result);
	result = NULL;
    }

    return result;
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(CVMjniEnv2ExecEnv(env), NULL);
    return NULL;
#endif /* CVM_REFLECT */
}

JNIEXPORT jobject JNICALL
JVM_NewMultiArray(JNIEnv *env, jclass eltClass, jintArray dim)
{
#ifdef CVM_REFLECT
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    /* 
     * Access member variable of type 'int'
     * and cast it to a native pointer. The java type 'int' only guarantees
     * 32 bit, but because one slot is used as storage space and
     * a slot is 64 bit on 64 bit platforms, it is possible 
     * to store a native pointer without modification of
     * java source code. This assumes that all places in the C-code
     * which set/get this member are caught.
     */
    CVMAddr cbAddr;
    CVMClassBlock* cb;
    CVMClassTypeID arrayClassID;
    CVMBool	   computedArrayClassID = CVM_FALSE;
    CVMObjectICell* classObj = (CVMObjectICell*) eltClass;
    CVMArrayOfIntICell* arrayObj = (CVMArrayOfIntICell*) dim;
    CVMJavaInt nDimensions;
    jobject res;
    int i;

    /* 
     * CVMmultiArrayAlloc() is called from java opcode multianewarray
     * and passes the top of stack as parameter dimensions.
     * Because the width of the array dimensions is obtained via
     * dimensions[i], dimensions has to be of the same type as 
     * the stack elements for proper access.
     *
     * CVMmultiArrayAlloc() expects that the dimensions array passed to it
     * will not have a 0 dimension in the middle.  The interpreter loop
     * filters out this case for the multianewarray bytecode before calling
     * CVMmultiArrayAlloc().  We must also do the equivalent here.
     */
    CVMStackVal32* dimensionsTmp;
    CVMInt32 effectiveNDimensions;
    
    if ((classObj == NULL) || (CVMID_icellIsNull(classObj))) {
	CVMthrowNullPointerException(ee, "null class object");
	return NULL;
    }
    
    if ((arrayObj == NULL) || (CVMID_icellIsNull(arrayObj))) {
	CVMthrowNullPointerException(ee, "null dimensions array");
	return NULL;
    }
    
    /* The number of dimensions (nDimension) is effectively the length of the
       dimensions array: */
    CVMID_arrayGetLength(ee, arrayObj, nDimensions);
    if (nDimensions == 0) {
	CVMthrowIllegalArgumentException(ee,
					 "zero-dimensional dimensions array");
	return NULL;
    }

    /*
     * Now go through the dimensions. Check for negative dimensions.
     * Also check for whether there is a 0 in the dimensions array
     * which would prevent the allocation from continuing for lower layers
     * of the array dimensions.
     *
     * By default, we'll set effectiveNDimensions to nDimensions i.e. all
     * the dimension values are valid and there are no zeros in the middle of
     * the dimensions array.  If we see a 0 in the dimensions array, we'll set
     * the effectiveNDimensions at the next index i.e. includes the current
     * index.  If we don't see a 0, we'll leave it at the default i.e. the
     * entire array is included.
     */
    effectiveNDimensions = nDimensions;
    for (i = 0; i < nDimensions; i++) {
	CVMJavaInt dim;
	CVMID_arrayReadInt(ee, arrayObj, i, dim);
	if (dim < 0) {
	    CVMthrowNegativeArraySizeException(ee, NULL);
	    return NULL;
	} else if ((dim == 0) && (effectiveNDimensions == nDimensions)) {
	    /* Remember the first 0 in the dimensions array */
	    effectiveNDimensions = i + 1;
	}
    }

    /* CVMmultiArrayAlloc() expects a CB for the top-level multi-dimensional
       array while java.lang.reflect.Array.newInstance() specifies the class
       for an element.  So, we need to determine the appropriate array CB
       before calling CVMmultiArrayAlloc() below.
     */
    CVMID_fieldReadAddr(ee, eltClass,
			CVMoffsetOfjava_lang_Class_classBlockPointer,
			cbAddr);
    cb = (CVMClassBlock*)cbAddr;
    CVMassert(cb != NULL);

    /* The element CB provided by java.lang.reflect.Array.newInstance() may
       itself be an array class.  We need to make sure that the deepest
       dimension of the new multi array does not exceed the 255 limit.
    */
    {
        CVMInt32 elementDepth;
        if (CVMisArrayClass(cb)) {
            /* Check total number of dimensions against 255
               limit.  See also JVM_NewArray above. */
            elementDepth = CVMarrayDepth(cb);
        } else {
            elementDepth = 0;
        }
        
        if ((elementDepth + nDimensions) > 255) {
            CVMassert(elementDepth <= 255); /* this should never happen because
                                             * arrays always have 255 or fewer
                                             * dimensions */
            CVMthrowIllegalArgumentException(ee, "too many dimensions");
            return NULL;
        }
    }

    /* %comment: c021 */
    if (CVMcbIs(cb, PRIMITIVE)) {
	cb = CVMclassGetArrayOf(ee, cb);
	if (nDimensions > 1) {
	    arrayClassID = 
		CVMtypeidIncrementArrayDepth(ee, CVMcbClassName(cb),
					     nDimensions - 1);
	    if (arrayClassID == CVM_TYPEID_ERROR) {
		/* Exception must have been thrown. Let it stand */
		CVMassert(CVMlocalExceptionOccurred(ee));
		return NULL;
	    }
	    computedArrayClassID = CVM_TRUE;
	} else {
	    arrayClassID = CVMcbClassName(cb);
	}
    } else {
        arrayClassID = CVMtypeidIncrementArrayDepth(ee, CVMcbClassName(cb),
                                                    nDimensions);
	if (arrayClassID == CVM_TYPEID_ERROR) {
	    /* Exception must have been thrown. Let it stand */
	    CVMassert(CVMlocalExceptionOccurred(ee));
	    return NULL;
	}
	computedArrayClassID = CVM_TRUE;
    }

    cb = CVMclassLookupByTypeFromClass(ee, arrayClassID, CVM_FALSE, cb);

    if (computedArrayClassID) {
	CVMtypeidDisposeClassID(ee, arrayClassID);
    }
    if (cb == NULL) {
	/* Exception must have been thrown. Let it stand */
	CVMassert(CVMlocalExceptionOccurred(ee));
	return NULL;
    }

    /* Okay, now we have the classblock. The only other thing we need
       to be able to use CVMmultiArrayAlloc (interpreter.[ch]) is a
       copy of the dimensions array. */
    /* 
     * CVMmultiArrayAlloc() is called from java opcode multianewarray
     * and passes the top of stack as parameter dimensions.
     * Because the width of the array dimensions is obtained via
     * dimensions[i], dimensions has to be of the same type as 
     * the stack elements for proper access.
     *
     * Note: CVMmultiArrayAlloc() is expecting a dimensions array that will
     * not be moving.  Since our dimensions array is a Java object, we will
     * need to allocated a native array that doesn't move in order to pass
     * the dimensions on.  Note that CVMmultiArrayAlloc() operates in a GC
     * safe state.  Hence, Java objects may be moved by the GC.  Therefore
     * this native dimensions array is needed.
     *
     * The needed size of the dimensions array is actually determined by
     * the effectiveNDimensions which was computed earlier.
     */

    /* Allocate the native dimensions array: */
    dimensionsTmp = 
	(CVMStackVal32*) malloc(sizeof(CVMStackVal32) * effectiveNDimensions);
    if (dimensionsTmp == NULL) {
	CVMthrowOutOfMemoryError(ee, NULL);
	return NULL;
    }

    /* Copy the dimension values over to the native dimensions array: */
    for (i = 0; i < effectiveNDimensions; i++) {
	CVMID_arrayReadInt(ee, arrayObj, i, dimensionsTmp[i].j.i);
    }
    
    /* Instantiate the multi dimensional array: */
    res = CVMjniCreateLocalRef(ee);
    if (res != NULL) {
	CVMmultiArrayAlloc(ee, effectiveNDimensions, dimensionsTmp, cb,
			   (CVMObjectICell*) res);
    }

    /* Release the native dimensions array: */
    free(dimensionsTmp);
    if (CVMexceptionOccurred(ee)) {
	(*env)->DeleteLocalRef(env, res);
	res = NULL;
    }
    return res;
#else /* CVM_REFLECT */
    CVMthrowUnsupportedOperationException(CVMjniEnv2ExecEnv(env), NULL);
    return NULL;
#endif /* CVM_REFLECT */
}

/* NOTE: JVM_GetField is implemented differently in CVM. See
   javavm/native/java/lang/reflect/Field.c. */

/* NOTE: JVM_GetPrimitiveField is implemented differently in CVM. See
   javavm/native/java/lang/reflect/Field.c. */

/* NOTE: JVM_SetField is implemented differently in CVM. See
   javavm/native/java/lang/reflect/Field.c. */

/* NOTE: JVM_SetPrimitiveField is implemented differently in CVM. See
   javavm/native/java/lang/reflect/Field.c. */

/* NOTE: JVM_InvokeMethod is implemented differently in CVM. See
   javavm/native/java/lang/reflect/Method.c. */

/* NOTE: JVM_NewInstanceFromConstructor is implemented differently in 
   CVM. See javavm/native/java/lang/reflect/Constructor.c. */

/*
 * Note that this function expects a full path name.  If the file
 * doesn't exist, then it is going to throw an exception.  In case
 * you are managing LD_LIBRARY_PATH yourself, like
 * ClassLoader.NativeLibrary does, then you had better deal with the
 * exception correctly.
 */
JNIEXPORT void * JNICALL 
JVM_LoadLibrary(const char *name)
{
#ifdef CVM_DYNAMIC_LINKING
    void *handle;

    /* I believe that in JDK 1.2 and above, the existence of the
       library in java.library.path is checked by Java. Therefore
       having an error detail message provided by in CVMdynlinkOpen is
       less useful. In addition, it's a property of dynamic linking on
       Solaris that such a detail message is available (via
       dlerror()); for example, VxWorks has no such facility. */
    handle = CVMdynlinkOpen(name);
    if (handle == NULL) {
	CVMthrowUnsatisfiedLinkError(CVMgetEE(), "%s", name);
    }
    return handle;
#else /* #defined CVM_DYNAMIC_LINKING */
    CVMthrowUnsupportedOperationException(CVMgetEE(), NULL);
    return NULL;
#endif  /* #defined CVM_DYNAMIC_LINKING */
}

JNIEXPORT void JNICALL 
JVM_UnloadLibrary(void * handle)
{
#ifdef CVM_DYNAMIC_LINKING
    CVMdynlinkClose(handle);
#else /* #defined CVM_DYNAMIC_LINKING */
    CVMthrowUnsupportedOperationException(CVMgetEE(), NULL);
#endif  /* #defined CVM_DYNAMIC_LINKING */
}

JNIEXPORT void * JNICALL 
JVM_FindLibraryEntry(void * handle, const char *name)
{
#ifdef CVM_DYNAMIC_LINKING
    return CVMdynlinkSym(handle, name);
#else /* #defined CVM_DYNAMIC_LINKING */
    CVMthrowUnsupportedOperationException(CVMgetEE(), NULL);
    return NULL;
#endif  /* #defined CVM_DYNAMIC_LINKING */
}

JNIEXPORT jboolean JNICALL
JVM_IsNaN(jdouble d)
{
    return CVMdoubleCompare(d, d, -1) != 0;
}

JNIEXPORT jboolean JNICALL
JVM_IsSupportedJNIVersion(jint version)
{
    return version == JNI_VERSION_1_1 ||
           version == JNI_VERSION_1_2 ||
           version == JNI_VERSION_1_4;
}

JNIEXPORT void * JNICALL
JVM_RawMonitorCreate(void)
{
    CVMReentrantMutex *rm = (CVMReentrantMutex *)calloc(1, sizeof *rm);
    if (rm == NULL) {
	return NULL;
    }
    CVMreentrantMutexInit(rm, NULL, 0);
    return rm;
}

JNIEXPORT void JNICALL
JVM_RawMonitorDestroy(void *mon)
{
    CVMreentrantMutexDestroy((CVMReentrantMutex *)mon);
    free(mon);
}

JNIEXPORT jint JNICALL
JVM_RawMonitorEnter(void *mon)
{
    CVMreentrantMutexLock(CVMgetEE(), (CVMReentrantMutex *)mon);
    return 0;
}

JNIEXPORT void JNICALL
JVM_RawMonitorExit(void *mon)
{
    CVMreentrantMutexUnlock(CVMgetEE(), (CVMReentrantMutex *)mon);
}

/* JDK1.2 has this in classresolver.c */

JNIEXPORT jclass JNICALL
JVM_FindLoadedClass(JNIEnv *env, jobject loader, jstring name)
{
    CVMClassBlock* cb;
    CVMClassTypeID classTypeID;
    char slash_name_buf[256];
    char *slash_name;
    int uni_len = (*env)->GetStringLength(env, name);
    int utf_len = (*env)->GetStringUTFLength(env, name);
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);

    if (utf_len >= sizeof(slash_name_buf)) {
        slash_name = (char *)malloc(utf_len + 1);
	if (slash_name == NULL) {
	    CVMthrowOutOfMemoryError(ee, NULL);
	    return NULL;
	}
    } else {
        slash_name = slash_name_buf;
    }
    
    (*env)->GetStringUTFRegion(env, name, 0, uni_len, slash_name);

    /* When the passed in name is an empty string */
    if (slash_name[0] == '\0') {
	return CVMjniNewLocalRef(env, NULL);
    }

    VerifyFixClassname(slash_name);

    CVMtraceClassLookup(("LU: JVM_FindLoadedClass called for \"%s\" using "
			 "0x%x\n", slash_name, 
			 CVMloaderCacheGetGlobalRootFromLoader(ee, loader)));

    /*
     * We need to use the "new" function, because the "lookup" function
     * is only valid when we know that the type already exists.
     */
    classTypeID = CVMtypeidNewClassID(ee, slash_name, (int)strlen(slash_name));

    if (classTypeID != CVM_TYPEID_ERROR) {
	CVMBool success;
	/*
	 * Any time CVMloaderCacheLookup() is called and we want to make sure
	 * the class we look up is completely done loading, including having
	 * superclasses loaded, then we need to synchronize on the loader
	 * instance first. Otherwise we can get a class that is still in
	 * the process of loading.
	 *
	 * The loader's loadClass() method is the only client of this code,
	 * and is suppose to by synchronized, but we don't trust it to be.
	 */
	success = CVMgcSafeObjectLock(ee, loader);

	if (!success) {
	    CVMthrowOutOfMemoryError(ee, NULL);
	    cb = NULL;
	} else {
	    CVM_LOADERCACHE_LOCK(ee);
	    cb = CVMloaderCacheLookup(ee, classTypeID, loader);
	    CVM_LOADERCACHE_UNLOCK(ee);
	    
	    success = CVMgcSafeObjectUnlock(ee, loader);
	    CVMassert(success);
	}

	CVMtypeidDisposeClassID(ee, classTypeID);
    } else {
	cb = NULL;
    }

    if (slash_name != slash_name_buf) {
        free(slash_name);
    }

    return CVMjniNewLocalRef(env, (cb == NULL ? NULL : CVMcbJavaInstance(cb)));
}


/*
 * The following have been moved to packages.
 *
 * JVM_GetSystemPackage(JNIEnv *env, jstring str)
 * JVM_GetSystemPackages(JNIEnv *env)
 */


/* JDK1.2 has this in javai.c */

/*
 * Short cuts
 */
#define PUTPROP(props, key, val)			\
    if (!CVMputProp(env, putID, props, key, val)) {	\
        return NULL;					\
    }

#define PUTPROP_ForPlatformCStringDo(props, key, val, failRet)		\
    if (!CVMputPropForPlatformCString(env, putID, props, key, val)) {	\
        return failRet;							\
    }

#define PUTPROP_ForPlatformCString(props, key, val)			\
    PUTPROP_ForPlatformCStringDo(props, key, val, NULL)

#define PUTPROP_ForPlatformCStringBoolRet(props, key, val)		\
    PUTPROP_ForPlatformCStringDo(props, key, val, CVM_FALSE)

/*
 * Initialize VM specific properties.
 * sprops contains system properties that are platform-dependent.
 */
JNIEXPORT jobject JNICALL
JVM_InitProperties(JNIEnv *env, jobject props, java_props_t* sprops)
{
    const CVMProperties *_props = CVMgetProperties();

    /*
     * Use the setProperty method to enforce the use of strings as
     * key and value, and also to make sure that we have a properties
     * table at hand and not just a hashtable.
     */
    jmethodID putID = (*env)->GetMethodID(env, 
					  (*env)->GetObjectClass(env, props),
					  "setProperty",
            "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/Object;");

    if (putID == NULL) return NULL;

    /* CVM_PROP_*s are set in build_defs.h generated by defs.mk */
    PUTPROP(props, "java.vm.specification.version",	CVM_PROP_JAVA_VM_SPEC_VERSION);
    PUTPROP(props, "java.vm.specification.name",	CVM_PROP_JAVA_VM_SPEC_NAME);
    PUTPROP(props, "java.vm.specification.vendor",	CVM_PROP_JAVA_VM_SPEC_VENDOR);
    PUTPROP(props, "java.vm.version",			CVM_PROP_JAVA_VM_VERSION);
    PUTPROP(props, "java.vm.name",			CVM_PROP_JAVA_VM_NAME);
    PUTPROP(props, "java.vm.vendor",			CVM_PROP_JAVA_VM_VENDOR);
    /* Note: java.vm.info is not required by the spec, 
     * but used in the '-version' string */
    PUTPROP(props, "java.vm.info",			CVM_PROP_JAVA_VM_INFO);
    PUTPROP(props, "com.sun.package.spec.version",	CVM_PROP_COM_SUN_PACKAGE_SPEC_VERSION);

#ifdef CVM_CLASSLOADING
    PUTPROP_ForPlatformCString(props, "sun.boot.class.path",
			       CVMglobals.bootClassPath.pathString);
    PUTPROP_ForPlatformCString(props, "java.class.path",
                               CVMglobals.appClassPath.pathString);
#endif
    PUTPROP_ForPlatformCString(props, "java.home", _props->java_home);
    PUTPROP_ForPlatformCString(props, "sun.boot.library.path",
			       _props->dll_dir);
    PUTPROP_ForPlatformCString(props, "java.library.path",
			       _props->library_path);
    PUTPROP_ForPlatformCString(props, "java.ext.dirs", _props->ext_dirs);
	
    /* And finally, set some special VM-specific properties to show
       which libraries are actually built-in */
    PUTPROP_ForPlatformCString(props, "java.library.builtin.net",  "yes");
    PUTPROP_ForPlatformCString(props, "java.library.builtin.math", "yes");
    PUTPROP_ForPlatformCString(props, "java.library.builtin.zip",  "yes");

#ifdef JAVASE
    if (!CVMinitJavaSEProperties(env, putID, props)) {
	return NULL;
    }
#endif

    return (*env)->NewLocalRef(env, props);
}

/*
 * java.lang.Compiler
 */

JNIEXPORT void JNICALL
JVM_InitializeCompiler (JNIEnv *env, jclass compCls)
{
}

JNIEXPORT jboolean JNICALL
JVM_IsSilentCompiler(JNIEnv *env, jclass compCls)
{
    return JNI_TRUE;
}

JNIEXPORT jboolean JNICALL
JVM_CompileClass(JNIEnv *env, jclass compCls, jclass cls)
{
    return JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
JVM_CompileClasses(JNIEnv *env, jclass cls, jstring jname)
{
    return JNI_FALSE;
}

JNIEXPORT jobject JNICALL
JVM_CompilerCommand(JNIEnv *env, jclass compCls, jobject arg)
{
    return NULL;
}

JNIEXPORT void JNICALL
JVM_EnableCompiler(JNIEnv *env, jclass compCls)
{
}

JNIEXPORT void JNICALL
JVM_DisableCompiler(JNIEnv *env, jclass compCls)
{
}

#ifdef JAVASE

/* Need to temporarily define JVM_Socket for compatibility with
 * J2SE Net libraries that haven't been recompiled with CVM headers
 */
#undef JVM_Socket
JNIEXPORT jint JNICALL
JVM_Socket(jint domain, jint type, jint protocol)
{
    return CVMnetSocket(domain, type, protocol);
}

JNIEXPORT jboolean JNICALL
JVM_SupportsCX8()
{
    return (CVM_FALSE);
}

JNIEXPORT jboolean JNICALL
JVM_CX8Field(JNIEnv *env, jobject obj, jfieldID fid, jlong oldVal, jlong newVal)
{
    return (CVM_FALSE);
}

JNIEXPORT jint JNICALL
JVM_InitializeSocketLibrary()
{
    return(CVM_TRUE);
}

JNIEXPORT void * JNICALL
JVM_RegisterSignal(jint sig, void *handler)
{
    return (void *)-1;
}

JNIEXPORT jboolean JNICALL
JVM_RaiseSignal(jint sig)
{
    return CVM_FALSE;
}

JNIEXPORT jint JNICALL
JVM_FindSignal(const char *name)
{
    return -1;
}

#endif /* JAVASE */
