/*
 * @(#)ClassLoader.c	1.79 06/10/10
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

/* %comment c017 */

#include "jni.h"
#include "jni_util.h"
#include "jlong.h"
#include "jvm.h"
#include "java_lang_ClassLoader.h"
#include "java_lang_ClassLoader_NativeLibrary.h"
#include "javavm/include/porting/linker.h"

#include "jni_statics.h"

#if 0  /* romized - not needed for CVM */
static JNINativeMethod methods[] = {
    {"retrieveDirectives",  "()Ljava/lang/AssertionStatusDirectives;", (void *)&JVM_AssertionStatusDirectives}
};

JNIEXPORT void JNICALL
Java_java_lang_ClassLoader_registerNatives(JNIEnv *env, jclass cls)
{
    (*env)->RegisterNatives(env, cls, methods, 
			    sizeof(methods)/sizeof(JNINativeMethod));
}
#endif

#include "javavm/include/globalroots.h"
#include "javavm/include/indirectmem.h"

#ifdef CVM_CLASSLOADING
#include "generated/offsets/java_lang_ClassLoader.h"
#endif

JNIEXPORT void JNICALL
Java_java_lang_ClassLoader_InitializeLoaderGlobalRoot(JNIEnv *env,
						      jobject loader)
{
#ifndef CVM_CLASSLOADING
    return;
#else
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);
    
    /* 
     * Allocate a ClassLoader global root for the ClassLoader instance.
     */
    /* 
     * Access member variable of type 'int'
     * and cast it to a native pointer. The java type 'int' only garanties 
     * 32 bit, but because one slot is used as storage space and
     * a slot is 64 bit on 64 bit platforms, it is possible 
     * to store a native pointer without modification of
     * java source code. This assumes that all places in the C-code
     * are catched which set/get this member.
     */
    CVMAddr loaderGlobalRootInt;
    CVMClassLoaderICell* loaderGlobalRoot = CVMID_getClassLoaderGlobalRoot(ee);
    if (loaderGlobalRoot == NULL) {
	CVMthrowOutOfMemoryError(ee, NULL);
	return;
    }

    /* Make the global root point to the ClassLoader instance. */
    CVMD_gcUnsafeExec(ee, {
	CVMID_icellAssignDirect(ee, loaderGlobalRoot, loader);
        /* This is the starting point from which the ClassLoader object
           gets registered with the VM and special handling is required
           to GC it later: */
        CVMglobals.gcCommon.loaderCreatedSinceLastGC = CVM_TRUE;
    });

    /* Store a reference to the root in the ClassLoader object. */
    loaderGlobalRootInt = (CVMAddr)loaderGlobalRoot;
    CVMID_fieldWriteAddr(ee, loader,
			 CVMoffsetOfjava_lang_ClassLoader_loaderGlobalRoot,
			 loaderGlobalRootInt);
    return;
#endif
}

JNIEXPORT jclass JNICALL
Java_java_lang_ClassLoader_defineClass0(JNIEnv *env,
					jobject loader,
					jstring name,
					jbyteArray data,
					jint offset,
					jint length,
					jobject pd)
{
    jbyte *body;
    char *utfName;
    jclass result = 0;
    char buf[128];

    if (data == NULL) {
	JNU_ThrowNullPointerException(env, 0);
	return 0;
    }

    /* Work around 4153825. malloc crashes on Solaris when passed a
     * negative size.
     */
    if (length < 0) {
        JNU_ThrowArrayIndexOutOfBoundsException(env, 0);
	return 0;
    }

    body = (jbyte *)malloc(length);

    if (body == 0) {
        JNU_ThrowOutOfMemoryError(env, 0);
	return 0;
    }

    (*env)->GetByteArrayRegion(env, data, offset, length, body);

    if ((*env)->ExceptionOccurred(env))
        goto free_body;

    if (name != NULL) {
        int len = (*env)->GetStringUTFLength(env, name);
	int unicode_len = (*env)->GetStringLength(env, name);
        if (len >= sizeof(buf)) {
            utfName = (char *)malloc(len + 1);
            if (utfName == NULL) {
                JNU_ThrowOutOfMemoryError(env, NULL);
                goto free_body;
            }
        } else {
            utfName = buf;
        }
    	(*env)->GetStringUTFRegion(env, name, 0, unicode_len, utfName);
	VerifyFixClassname(utfName);
    } else {
	utfName = NULL;
    }

    result = JVM_DefineClass(env, utfName, loader, body, length, pd);

    if (utfName && utfName != buf) 
        free(utfName);

 free_body:
    free(body);
    return result;
}

#ifdef JAVASE
JNIEXPORT jclass JNICALL
Java_java_lang_ClassLoader_defineClass1(JNIEnv *env,
					jobject loader,
					jstring name,
					jbyteArray data,
					jint offset,
					jint length,
					jobject pd,
                                        jstring source)
{
    /* FIXME: currently we ignore the "source" argument" */
    return Java_java_lang_ClassLoader_defineClass0(env, loader, name, data, offset, length, pd);
}

JNIEXPORT jclass JNICALL
Java_java_lang_ClassLoader_defineClass2(JNIEnv *env,
					jobject loader,
					jstring name,
					jobject data,
					jint offset,
					jint length,
					jobject pd,
					jstring source)
{
    /* FIXME: not implemented yet */
    CVMassert(CVM_FALSE);
    return NULL;
}
#endif

JNIEXPORT void JNICALL
Java_java_lang_ClassLoader_resolveClass0(JNIEnv *env, jobject thisObj,
					 jclass cls)
{
    if (cls == NULL) {
	JNU_ThrowNullPointerException(env, 0);
	return;
    }

    JVM_ResolveClass(env, cls);
}

JNIEXPORT jclass JNICALL
Java_java_lang_ClassLoader_loadBootstrapClass0(JNIEnv *env, jobject loader, 
					       jstring classname)
{
    char *clname;
    jclass cls = NULL;
    char buf[128];
    int len;
    int unicode_len;
    CVMClassTypeID typeID = CVM_TYPEID_ERROR;
    CVMClassBlock* cb;
    CVMExecEnv* ee = CVMjniEnv2ExecEnv(env);

    if (classname == NULL) {
        JNU_ThrowClassNotFoundException(env, 0);
	return NULL;
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

    VerifyFixClassname(clname);

    if (!VerifyClassname(clname, JNI_TRUE)) {  /* expects slashed name */
        JNU_ThrowClassNotFoundException(env, clname);
	goto done;
    }

    /* get a typeid for the class name */
    typeID = CVMtypeidNewClassID(ee, clname, len);
    if (typeID == CVM_TYPEID_ERROR) {
	goto done; /* exception already thrown */
    }

    /* See if it is a preloaded class or already in the loader cache */
    cb = CVMclassLookupClassWithoutLoading(ee, typeID, NULL);
    if (cb != NULL) {
	cls = CVMcbJavaInstance(cb);
	goto done;
    }

    /* load the class */
    cls = CVMclassLoadBootClass(ee, clname);
    if (cls == NULL) {
	goto done;
    }

    /* We must free the global root returned, but not until after we've
     * assigned it to another root to keep the Class alive until
     * Class.loadSuperClasses() is called.
     */
    {
	jclass tmp = CVMjniCreateLocalRef(ee);
	if (tmp != NULL) {
	    CVMID_icellAssign(ee, tmp, cls);
	} else {
	    /* exception already thrown. Class will be gc'd */
	}
	CVMID_freeGlobalRoot(ee, cls);
	cls = tmp;
    }

 done:
    if (typeID != CVM_TYPEID_ERROR) {
	CVMtypeidDisposeClassID(ee, typeID);
    }
    if (clname != buf) {
    	free(clname);
    }
    return cls;
}

JNIEXPORT jclass JNICALL
Java_java_lang_ClassLoader_findLoadedClass0(JNIEnv *env, jobject loader, 
					   jstring name)
{
    if (name == NULL) {
	return 0;
    } else {
        return JVM_FindLoadedClass(env, loader, name);
    }
}

JNIEXPORT jboolean JNICALL 
Java_java_lang_ClassLoader_00024NativeLibrary_initIDs(JNIEnv *env, jclass thisObj)
{
    JNI_STATIC(java_lang_ClassLoader, handleID) = (*env)->GetFieldID(env, thisObj, "handle", "J");
    if (JNI_STATIC(java_lang_ClassLoader, handleID) == 0)
	return JNI_FALSE;
    JNI_STATIC(java_lang_ClassLoader, jniVersionID) = (*env)->GetFieldID(env, thisObj, "jniVersion", "I");
    if (JNI_STATIC(java_lang_ClassLoader, jniVersionID) == 0)
	return JNI_FALSE;
    JNI_STATIC(java_lang_ClassLoader, isXrunLibraryID) = (*env)->GetFieldID(env, thisObj, "isXrunLibrary", "Z");
    if (JNI_STATIC(java_lang_ClassLoader, isXrunLibraryID) == 0)
        return JNI_FALSE;
    return JNI_TRUE;
}

typedef jint (JNICALL *JNI_OnLoad_t)(JavaVM *, void *);
typedef void (JNICALL *JNI_OnUnload_t)(JavaVM *, void *);

JNIEXPORT jboolean JNICALL 
Java_java_lang_ClassLoader_00024NativeLibrary_exists(JNIEnv *env,
                                                     jclass thisObj,
                                                     jstring name)
{
    const char *cname;
    jboolean ret;

    cname = JNU_GetStringPlatformChars(env, name, 0);
    if (cname == 0) {
        return JNI_FALSE;
    }
    ret = CVMdynlinkExists(cname);
    JNU_ReleaseStringPlatformChars(env, name, cname);
    return ret;
 }

/*
 * Class:     java_lang_ClassLoader_NativeLibrary
 * Method:    load
 * Signature: (Ljava/lang/String;)J
 */
JNIEXPORT void JNICALL 
Java_java_lang_ClassLoader_00024NativeLibrary_load
  (JNIEnv *env, jobject thisObj, jstring name)
{
    /* %comment kbr002 */
    const char *cname;
    jint jniVersion;
    void * handle;
    jboolean isXrunLibrary;

    cname = JNU_GetStringPlatformChars(env, name, 0);
    if (cname == 0)
        return;
    handle = JVM_LoadLibrary(cname);
    isXrunLibrary =  (*env)->GetBooleanField(env, thisObj, JNI_STATIC(java_lang_ClassLoader, isXrunLibraryID));
    if (handle && !isXrunLibrary) {
        JNI_OnLoad_t JNI_OnLoad =
	    (JNI_OnLoad_t)JVM_FindLibraryEntry(handle, "JNI_OnLoad");
	if (JNI_OnLoad) {
	    JavaVM *jvm;
	    (*env)->GetJavaVM(env, &jvm);
	    jniVersion = (*JNI_OnLoad)(jvm, NULL);
	} else {
	    jniVersion = JNI_VERSION_1_1;
	}

 	if ((*env)->ExceptionOccurred(env)) {
	    JNU_ThrowByNameWithLastError(env, "java/lang/UnsatisfiedLinkError",
					 "exception occurred in JNI_OnLoad");
	    JVM_UnloadLibrary(handle);
	    JNU_ReleaseStringPlatformChars(env, name, cname);
	    return;
	}
   
	if (!JVM_IsSupportedJNIVersion(jniVersion)) {
	    char msg[256];
	    jio_snprintf(msg, sizeof(msg),
			 "unsupported JNI version 0x%08X required by %s",
			 jniVersion, cname);
	    JNU_ThrowByName(env, "java/lang/UnsatisfiedLinkError", msg);
	    JVM_UnloadLibrary(handle);
	    JNU_ReleaseStringPlatformChars(env, name, cname);
	    return;
	}
	(*env)->SetIntField(env, thisObj, JNI_STATIC(java_lang_ClassLoader, jniVersionID), jniVersion);
    }
    (*env)->SetLongField(env, thisObj, JNI_STATIC(java_lang_ClassLoader, handleID), ptr_to_jlong(handle));
    JNU_ReleaseStringPlatformChars(env, name, cname);
}

/*
 * Class:     java_lang_ClassLoader_NativeLibrary
 * Method:    unload
 * Signature: ()V
 */
JNIEXPORT void JNICALL 
Java_java_lang_ClassLoader_00024NativeLibrary_unload
  (JNIEnv *env, jobject thisObj)
{
    void *handle;
    jboolean isXrunLibrary;

    handle = jlong_to_ptr((*env)->GetLongField(env, thisObj, JNI_STATIC(java_lang_ClassLoader, handleID)));
    isXrunLibrary =  (*env)->GetBooleanField(env, thisObj, JNI_STATIC(java_lang_ClassLoader, isXrunLibraryID));
    if (!isXrunLibrary) {
        JNI_OnUnload_t JNI_OnUnload;

        JNI_OnUnload = (JNI_OnUnload_t )
            JVM_FindLibraryEntry(handle, "JNI_OnUnload");

        if (JNI_OnUnload) {
            JavaVM *jvm;
            (*env)->GetJavaVM(env, &jvm);
            (*JNI_OnUnload)(jvm, NULL);
        }
    }
    JVM_UnloadLibrary(handle);
}

/*
 * Class:     java_lang_ClassLoader_NativeLibrary
 * Method:    find
 * Signature: (Ljava/lang/String;J)J
 */
JNIEXPORT jlong JNICALL 
Java_java_lang_ClassLoader_00024NativeLibrary_find
  (JNIEnv *env, jobject thisObj, jstring name)
{
    jlong handle;
    const char *cname;
    jlong res;

    handle = (*env)->GetLongField(env, thisObj, JNI_STATIC(java_lang_ClassLoader, handleID));
    cname = (*env)->GetStringUTFChars(env, name, 0);
    if (cname == 0)
        return jlong_zero;
    res = ptr_to_jlong(JVM_FindLibraryEntry(jlong_to_ptr(handle), cname));
    (*env)->ReleaseStringUTFChars(env, name, cname);
    return res;
}

JNIEXPORT jobject JNICALL
Java_java_lang_ClassLoader_getCallerClassLoader(JNIEnv *env, jobject thisObj)
{
    jclass caller = JVM_GetCallerClass(env, 2);
    return caller != 0 ? JVM_GetClassLoader(env, caller) : 0;
}
