/*
 * @(#)Class.c	1.127 06/10/30
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

/*-
 *      Implementation of class Class
 *
 *      former threadruntime.c, Sun Sep 22 12:09:39 1991
 */

#include "javavm/include/clib.h"
#include "javavm/include/interpreter.h"
#include "javavm/include/indirectmem.h"
#include "javavm/include/common_exceptions.h"
#include "generated/offsets/java_lang_Class.h"

#include "jni.h"
#include "jni_util.h"
#include "jvm.h"
#include "java_lang_Class.h"

#ifdef CVM_JIT
#include "javavm/include/jit/jitintrinsic.h"
#endif

#ifdef CVM_JAVALANGCLASS_REGISTER_NATIVES

#define OBJ "Ljava/lang/Object;"
#define CLS "Ljava/lang/Class;"
#define STR "Ljava/lang/String;"
#define JCL "Ljava/lang/ClassLoader;"
#define FLD "Ljava/lang/reflect/Field;"
#define MHD "Ljava/lang/reflect/Method;"
#define CTR "Ljava/lang/reflect/Constructor;"
#define PD  "Ljava/security/ProtectionDomain;"

static JNINativeMethod methods[] = {
    {"getName",          "()" STR,          (void *)&JVM_GetClassName},
#ifdef JDK12
    {"getSuperclass",    "()" CLS,          NULL},
#else
    {"getSuperclass",    "()" CLS,          (void *)&CVMjniGetSuperclass},
#endif
    {"getInterfaces",    "()[" CLS,         (void *)&JVM_GetClassInterfaces},
    {"getClassLoader0",  "()" JCL,          (void *)&JVM_GetClassLoader},
    {"getSigners",       "()[" OBJ,         (void *)&JVM_GetClassSigners},
    {"setSigners",       "([" OBJ ")V",     (void *)&JVM_SetClassSigners},
    {"isArray",          "()Z",             (void *)&JVM_IsArrayClass},
    {"isPrimitive",      "()Z",             (void *)&JVM_IsPrimitiveClass},
    {"getComponentType", "()" CLS,          (void *)&JVM_GetComponentType},
    {"getModifiers",     "()I",             (void *)&JVM_GetClassModifiers},
    {"getFields0",       "(I)[" FLD,        (void *)&JVM_GetClassFields},
    {"getMethods0",      "(I)[" MHD,        (void *)&JVM_GetClassMethods},
    {"getConstructors0", "(I)[" CTR,        (void *)&JVM_GetClassConstructors},
    {"getField0",        "(" STR "I)" FLD,  (void *)&JVM_GetClassField},
    {"getMethod0", "(" STR "[" CLS "I)" MHD,(void *)&JVM_GetClassMethod},
    {"getConstructor0",  "([" CLS "I)" CTR, (void *)&JVM_GetClassConstructor},
    {"getProtectionDomain0", "()" PD,       (void *)&JVM_GetProtectionDomain},
    {"getDeclaredClasses0",  "()[" CLS,      (void *)&JVM_GetDeclaredClasses},
    {"getDeclaringClass",   "()" CLS,      (void *)&JVM_GetDeclaringClass},
};

#undef OBJ
#undef CLS
#undef STR
#undef JCL
#undef FLD
#undef MHD
#undef CTR
#undef PD

JNIEXPORT void JNICALL
Java_java_lang_Class_registerNatives(JNIEnv *env, jclass cls)
{
#ifdef JDK12
    methods[1].fnPtr = (void *)(*env)->GetSuperclass;
#endif
    (*env)->RegisterNatives(env, cls, methods, 
			    sizeof(methods)/sizeof(JNINativeMethod));
}
#endif /* CVM_JAVALANGCLASS_REGISTER_NATIVES */

JNIEXPORT jclass JNICALL
Java_java_lang_Class_forName0(JNIEnv *env, jclass thisObj, jstring classname,
			      jboolean initialize, jobject loader)
{
    char *clname;
    jclass cls = 0;
    char buf[128];
    int len;
    int unicode_len;

    if (classname == NULL) {
        JNU_ThrowNullPointerException(env, 0);
	return 0;
    }

    len = (*env)->GetStringUTFLength(env, classname);
    unicode_len = (*env)->GetStringLength(env, classname);
    if (len >= sizeof(buf)) {
        clname = (char *)malloc(len + 1);
        if (clname == NULL) {
            JNU_ThrowOutOfMemoryError(env, NULL);
            return NULL;
        }
    } else {
        clname = buf;
    }
    (*env)->GetStringUTFRegion(env, classname, 0, unicode_len, clname);

    if (VerifyFixClassname(clname) == JNI_TRUE) {
	/* slashes present in clname, use name b4 translation for exception */
	(*env)->GetStringUTFRegion(env, classname, 0, unicode_len, clname);
	JNU_ThrowClassNotFoundException(env, clname);
	goto done;
    }

    if (!VerifyClassname(clname, JNI_TRUE)) {  /* expects slashed name */
        JNU_ThrowClassNotFoundException(env, clname);
	goto done;
    }

    cls = JVM_FindClassFromClassLoader(env, clname, initialize,
				       loader, JNI_FALSE);
 done:
    if (clname != buf) {
	free(clname);
    }

    return cls;
}

JNIEXPORT jboolean JNICALL
Java_java_lang_Class_isInstance(JNIEnv *env, jobject cls, jobject obj)
{
    if (obj == NULL) {
        return JNI_FALSE;
    }
    return (*env)->IsInstanceOf(env, obj, (jclass)cls);
}

JNIEXPORT jboolean JNICALL
Java_java_lang_Class_isAssignableFrom(JNIEnv *env, jobject cls, jobject cls2)
{
    if (cls2 == NULL) {
	JNU_ThrowNullPointerException(env, 0);
	return JNI_FALSE;
    }
    return (*env)->IsAssignableFrom(env, cls2, cls);
}

JNIEXPORT jclass JNICALL
Java_java_lang_Class_getPrimitiveClass(JNIEnv *env,
				       jclass cls,
				       jstring name)
{
    const char *utfName;
    jclass result;

    if (name == NULL) {
	JNU_ThrowNullPointerException(env, 0);
	return NULL;
    }

    utfName = (*env)->GetStringUTFChars(env, name, 0);
    if (utfName == 0)
        return NULL;

    result = JVM_FindPrimitiveClass(env, utfName);

    (*env)->ReleaseStringUTFChars(env, name, utfName);

    return result;
}

#undef gcSafeRef2Class
#define gcSafeRef2Class(ee, cl)	CVMgcSafeClassRef2ClassBlock(ee, cl)

JNIEXPORT jboolean JNICALL
Java_java_lang_Class_checkInitializedFlag(JNIEnv *env, jclass cls)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock* cb = gcSafeRef2Class(ee, cls);
    return CVMcbInitializationDoneFlag(ee, cb);
}

/*
 * Return true if this class is currently being initialized. If me == false,
 * then don't return true if the class is being initialized by this thread.
 */
JNIEXPORT jboolean JNICALL
Java_java_lang_Class_checkInitializingFlag(JNIEnv *env, jclass cls, 
					   jboolean me)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock* cb = gcSafeRef2Class(ee, cls);
    CVMExecEnv* clinitEE = CVMcbGetClinitEE(ee, cb);
#if 0
    /* the wince clmips compiler generates bad code for the following */
    return (clinitEE != NULL) && ((clinitEE == ee) == me);
#else
    if (clinitEE != NULL) {
	if (clinitEE == ee) {
	    return me;
	} else {
	    return !me;
	}
    } else {
	return CVM_FALSE;
    }
#endif
}

JNIEXPORT jboolean JNICALL
Java_java_lang_Class_checkErrorFlag(JNIEnv *env, jclass cls)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock* cb = gcSafeRef2Class(ee, cls);
    return CVMcbCheckErrorFlag(ee, cb);
}

JNIEXPORT void JNICALL
Java_java_lang_Class_setInitializedFlag(JNIEnv *env, jclass cls)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock* cb = gcSafeRef2Class(ee, cls);
    CVMcbSetInitializationDoneFlag(ee, cb);
}

JNIEXPORT void JNICALL
Java_java_lang_Class_setInitializingFlag(JNIEnv *env, jclass cls)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock* cb = gcSafeRef2Class(ee, cls);
    CVMcbSetClinitEE(ee, cb, ee);
}

JNIEXPORT void JNICALL
Java_java_lang_Class_setErrorFlag(JNIEnv *env, jclass cls)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock* cb = gcSafeRef2Class(ee, cls);
    CVMcbSetErrorFlag(ee, cb);
}

JNIEXPORT void JNICALL
Java_java_lang_Class_clearInitializingFlag(JNIEnv *env, jclass cls)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock* cb = gcSafeRef2Class(ee, cls);
    CVMcbSetClinitEE(ee, cb, NULL);
}

JNIEXPORT void JNICALL
Java_java_lang_Class_clearSuperClassesLoadedFlag(JNIEnv *env, jclass cls)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock* cb = gcSafeRef2Class(ee, cls);
    CVMcbClearRuntimeFlag(cb, ee, SUPERCLASS_LOADED);
}

/* some native accessor methods needed by loadSuperClasses(). */

JNIEXPORT jint JNICALL
Java_java_lang_Class_getClassTypeID(JNIEnv *env, jclass cls)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock* cb = gcSafeRef2Class(ee, cls);
    return CVMcbClassName(cb);
}

JNIEXPORT jint JNICALL
Java_java_lang_Class_getSuperClassTypeID(JNIEnv *env, jclass cls)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock* cb = gcSafeRef2Class(ee, cls);
    return CVMcbSuperclassTypeID(cb);
}

JNIEXPORT jstring JNICALL
Java_java_lang_Class_getSuperClassName(JNIEnv *env, jclass cls, jobject loader)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock* cb = gcSafeRef2Class(ee, cls);
    CVMClassBlock* superCb;
    CVMClassTypeID cid = CVMcbSuperclassTypeID(cb);
    char* cname;
    jstring result;

    /* See if the class is already in the loaded. If it is, then
     * signal that the class doesn't need loading by returning NULL.
     * This is just an optimization to avoid allocating
     * the String and some unecessary loadClass() calls.
     */
    superCb = CVMclassLookupClassWithoutLoading(ee, cid, loader);
    if (superCb != NULL) {
	return NULL;
    }

    cname = CVMtypeidClassNameToAllocatedCString(cid);
    if (cname == NULL) {
	CVMthrowOutOfMemoryError(ee, NULL);
	return NULL;
    }
    
    TranslateFromVMClassName(cname);
    result = (*env)->NewStringUTF(env, cname);
    free(cname);
    return result;
}

JNIEXPORT jint JNICALL
Java_java_lang_Class_getNumInterfaces(JNIEnv *env, jclass cls)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock* cb = gcSafeRef2Class(ee, cls);
    return CVMcbImplementsCountRaw(cb);
}

JNIEXPORT jint JNICALL
Java_java_lang_Class_getInterfaceTypeID(JNIEnv *env, jclass cls, jint index)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock* cb = gcSafeRef2Class(ee, cls);
    return CVMcbInterfaceTypeID(cb, index);
}

JNIEXPORT jstring JNICALL
Java_java_lang_Class_getInterfaceName(JNIEnv *env, jclass cls, jint index,
				      jobject loader)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock* cb = gcSafeRef2Class(ee, cls);
    CVMClassBlock* intfCb = gcSafeRef2Class(ee, cls);
    CVMClassTypeID cid = CVMcbInterfaceTypeID(cb, index);
    char* cname;
    jstring result;

    /* See if the class is already in the loaded. If it is, then
     * signal that the class doesn't need loading by returning NULL.
     * This is just an optimization to avoid allocating
     * the String and some unecessary loadClass() calls.
     */
    intfCb = CVMclassLookupClassWithoutLoading(ee, cid, loader);
    if (intfCb != NULL) {
	return NULL;
    }

    cname = CVMtypeidClassNameToAllocatedCString(cid);
    if (cname == NULL) {
	CVMthrowOutOfMemoryError(ee, NULL);
	return NULL;
    }
    
    TranslateFromVMClassName(cname);
    result = (*env)->NewStringUTF(env, cname);
    free(cname);
    return result;
}

JNIEXPORT void JNICALL
Java_java_lang_Class_linkSuperClasses(JNIEnv *env, jclass cls)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock* cb = gcSafeRef2Class(ee, cls);
    CVMclassLinkSuperClasses(ee, cb);
}

JNIEXPORT void JNICALL
Java_java_lang_Class_addToLoaderCache(JNIEnv *env, jclass cls, jobject loader)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock* cb = gcSafeRef2Class(ee, cls);
    CVMloaderCacheAdd(ee, cb, loader); /* may throw an exception */
}


JNIEXPORT void JNICALL
Java_java_lang_Class_notifyClassLoaded(JNIEnv *env, jclass cls)
{
#if defined(CVM_JVMPI) || defined(CVM_JVMTI) ||   \
    (defined(CVM_JIT) && defined(CVMJIT_INTRINSICS))
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock* cb = gcSafeRef2Class(ee, cls);
#endif /* defined(CVM_JVMPI) || defined(CVM_JVMTI) || defined(CVM_JIT) */
#if defined(CVM_JIT) && defined(CVMJIT_INTRINSICS)
    CVMJITintrinsicScanForIntrinsicMethods(ee, cb);
#endif
#ifdef CVM_JVMPI
    if (CVMjvmpiEventClassLoadIsEnabled()) {
	CVMjvmpiPostClassLoadEvent(ee, cb);
    }
#endif /* CVM_JVMPI */
#ifdef CVM_JVMTI
    /* Note: May change CVMisArrayOfAnyBasicType to CVMisArrayClass */
    if (CVMjvmtiIsEnabled() && !CVMisArrayOfAnyBasicType(cb) &&
	!CVMjvmtiClassBeingRedefined(ee, cb)) {
	CVMjvmtiPostClassLoadEvent(ee, CVMcbJavaInstance(cb));
    }
#endif /* CVM_JVMTI */
}

JNIEXPORT jboolean JNICALL
Java_java_lang_Class_superClassesLoaded(JNIEnv *env, jclass cls)
{
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    CVMClassBlock* cb = gcSafeRef2Class(ee, cls);
    return CVMcbCheckRuntimeFlag(cb, SUPERCLASS_LOADED);
}
