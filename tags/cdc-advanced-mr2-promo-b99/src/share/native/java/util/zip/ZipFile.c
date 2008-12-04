/*
 * @(#)ZipFile.c	1.32 06/10/10
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
 * Native method support for java.util.zip.ZipFile
 */

#include "javavm/include/porting/ansi/stdio.h"
#include "javavm/include/porting/ansi/stdlib.h"
#include "javavm/include/porting/ansi/string.h"
#include "javavm/include/porting/ansi/errno.h"
#include "javavm/include/porting/ansi/ctype.h"
#include "javavm/include/porting/ansi/assert.h"
#include "javavm/include/ansi2cvm.h"
#include "javavm/include/interpreter.h"
#include "jlong.h"
#include "jvm.h"
#include "jni.h"
#include "jni_util.h"
#include "zip_util.h"

#include "java_util_zip_ZipFile.h"
#include "java_util_jar_JarFile.h"

#define DEFLATED 8
#define STORED 0

#include "jni_statics.h"
/* make these const */
static const int OPEN_READ = java_util_zip_ZipFile_OPEN_READ;
static const int OPEN_DELETE = java_util_zip_ZipFile_OPEN_DELETE;

JNIEXPORT void JNICALL
Java_java_util_zip_ZipFile_initIDs(JNIEnv *env, jclass cls)
{
    JNI_STATIC(java_util_zip_ZipFile, jzfileID) = (*env)->GetFieldID(env, cls, "jzfile", "J");
    CVMassert(JNI_STATIC(java_util_zip_ZipFile, jzfileID) != 0);
}

static void
ThrowZipException(JNIEnv *env, const char *msg)
{
    jstring s = NULL;
    jobject x;
    
    if (msg != NULL) {
	s = JNU_NewStringPlatform(env, msg);
    }
    x = JNU_NewObjectByName(env,
			    "java/util/zip/ZipException",
			    "(Ljava/lang/String;)V", s);
    if (x != NULL) {
	(*env)->Throw(env, x);
	(*env)->DeleteLocalRef(env, x);
	if (s != NULL) {
	    (*env)->DeleteLocalRef(env, s);
	}
    }
}

JNIEXPORT jlong JNICALL
Java_java_util_zip_ZipFile_open(JNIEnv *env, jclass cls, jstring name, 
                                        jint mode, jlong lastModified)
{
    const char *path;
    jlong result = CVMlongConstZero();
    int flag = 0;
    
    /* fix 4210379: throw Null Pointer Exception if file name is null */
    if (name == 0) {
      JNU_ThrowNullPointerException(env, "null filename");
      return CVMlongConstZero();
    }

    path = JNU_GetStringPlatformChars(env, name, 0);

    /* Fix for bug #6264809 : for CDC, mode is cleared before it is passed
     * here.  So, the JVM_O_DELETE set operations below is not needed.
     *
     * For JAVASE, mode is not cleared, and the low level IO HPI expects
     * JVM_O_DELETE to be set in the flag when appropriate.
    */
#ifdef JAVASE
    if (mode & OPEN_DELETE) flag |= JVM_O_DELETE;
#endif
    if (mode & OPEN_READ) flag |= O_RDONLY;

    if (path != 0) {
	char *msg;
	jzfile *zip = ZIP_Open_Generic(path, &msg, flag, lastModified);
	JNU_ReleaseStringPlatformChars(env, name, path);
	if (zip != 0) {
	    result = ptr_to_jlong(zip);
	} else if (msg != 0) {
	    ThrowZipException(env, msg);
	} else if (errno == ENOMEM) {
	    JNU_ThrowOutOfMemoryError(env, 0);
	} else {
	    ThrowZipException(env, "error in opening zip file");
	}
    }
    return result;
}

JNIEXPORT jint JNICALL
Java_java_util_zip_ZipFile_getTotal(JNIEnv *env, jclass cls, jlong zfile)
{
    jzfile *zip = (jzfile *)jlong_to_ptr(zfile);

    return zip->total;
}

JNIEXPORT void JNICALL
Java_java_util_zip_ZipFile_close(JNIEnv *env, jclass cls, jlong zfile)
{
    ZIP_Close((jzfile *)jlong_to_ptr(zfile));
}

JNIEXPORT jlong JNICALL
Java_java_util_zip_ZipFile_getEntry(JNIEnv *env, jclass cls, jlong zfile,
				    jstring name)
{
#define MAXNAME 1024
    jzfile *zip = (jzfile *)jlong_to_ptr(zfile);
    jsize slen = (*env)->GetStringLength(env, name);
    jsize ulen = (*env)->GetStringUTFLength(env, name);
    char buf[MAXNAME+1], *path;
    jzentry *ze;

    if (ulen > MAXNAME) {
	path = (char *)malloc(ulen + 1);
	if (path == 0) {
	    JNU_ThrowOutOfMemoryError(env, 0);
	    return 0;
	}
    } else {
	path = buf;
    }
    (*env)->GetStringUTFRegion(env, name, 0, slen, path);
    path[ulen] = '\0';
    ze = ZIP_GetEntry(zip, path);
    if (path != buf) {
	free(path);
    }
    return ptr_to_jlong(ze);
}

JNIEXPORT void JNICALL
Java_java_util_zip_ZipFile_freeEntry(JNIEnv *env, jclass cls, jlong zfile,
				    jlong zentry)
{
    jzfile *zip = (jzfile *)jlong_to_ptr(zfile);
    jzentry *ze = (jzentry *)jlong_to_ptr(zentry);
    ZIP_FreeEntry(zip, ze);
}

JNIEXPORT jlong JNICALL
Java_java_util_zip_ZipFile_getNextEntry(JNIEnv *env, jclass cls, jlong zfile,
					jint n)
{
    jzentry *ze = ZIP_GetNextEntry((jzfile *)jlong_to_ptr(zfile), n);

    return ptr_to_jlong(ze);
}

JNIEXPORT jint JNICALL
Java_java_util_zip_ZipFile_getMethod(JNIEnv *env, jclass cls, jlong zentry)
{
    jzentry *ze = (jzentry *)jlong_to_ptr(zentry);

    return ze->csize != 0 ? DEFLATED : STORED;
}

JNIEXPORT jint JNICALL
Java_java_util_zip_ZipFile_getCSize(JNIEnv *env, jclass cls, jlong zentry)
{
    jzentry *ze = (jzentry *)jlong_to_ptr(zentry);

    return ze->csize != 0 ? ze->csize : ze->size;
}

JNIEXPORT jint JNICALL
Java_java_util_zip_ZipFile_getSize(JNIEnv *env, jclass cls, jlong zentry)
{
    jzentry *ze = (jzentry *)jlong_to_ptr(zentry);

    return ze->size;
}

JNIEXPORT jint JNICALL
Java_java_util_zip_ZipFile_read(JNIEnv *env, jclass cls, jlong zfile,
				jlong zentry, jint pos, jbyteArray bytes,
				jint off, jint len)
{
    jzfile *zip = (jzfile *)jlong_to_ptr(zfile);
    jbyte *buf;
    char *msg;
    CVMExecEnv *ee = CVMjniEnv2ExecEnv(env);
    char errmsg[128];
    if (len == 0) {
        return 0;
    }
    if (len > CVM_CSTACK_BUF_SIZE)
	len = CVM_CSTACK_BUF_SIZE;
    {
	char* tmp;
	CVMCstackGetBuffer(ee, tmp); /* Lock EE-buffer */
	buf = (jbyte*)tmp;
    }
    if (buf == NULL) {
        JNU_ThrowOutOfMemoryError(env, 0);
        return 0;
    }

    ZIP_Lock(zip);
    len = ZIP_Read(zip, (jzentry *)jlong_to_ptr(zentry), pos, buf, len);
    msg = zip->msg;
    ZIP_Unlock(zip);
    if (len == -1) {
	if (msg != 0) {
	    ThrowZipException(env, msg);
	} else {
	    sprintf(errmsg, "errno: %d, error: %s\n", errno, "Error reading ZIP file");
            JNU_ThrowIOExceptionWithLastError(env, errmsg);
	}
    } else {
	(*env)->SetByteArrayRegion(env, bytes, off, len, buf);
    }
    CVMCstackReleaseBuffer(ee);		/* Unlock EE-buffer */
    return len;
}

/*
 * Returns an array of strings representing the names of all entries
 * that begin with "META-INF/" (case ignored). This native method is
 * used in JarFile as an optimization when looking up manifest and
 * signature file entries. Returns null if no entries were found.
 */
JNIEXPORT jobjectArray JNICALL
Java_java_util_jar_JarFile_getMetaInfEntryNames(JNIEnv *env, jobject obj)
{
    jlong zfile = (*env)->GetLongField(env, obj, JNI_STATIC(java_util_zip_ZipFile, jzfileID));
    jzfile *zip;
    int i, count;
    jobjectArray result = 0;

    CVMassert(CVMlongNe(zfile, CVMlongConstZero()));
    zip = (jzfile *)jlong_to_ptr(zfile);

    /* count the number of valid ZIP metanames */
    count = 0;
    if (zip->metanames != 0) {
	for (i = 0; i < zip->metacount; i++) {
	    if (zip->metanames[i] != 0) {
		count++;
	    }
	}
    }

    /* If some names were found then build array of java strings */
    if (count > 0) {
	jclass cls = (*env)->FindClass(env, "java/lang/String");
	result = (*env)->NewObjectArray(env, count, cls, 0);
	if (result != 0) {
	    for (i = 0; i < count; i++) {
		jstring str = (*env)->NewStringUTF(env, zip->metanames[i]);
		if (str == 0) {
		    break;
		}
		(*env)->SetObjectArrayElement(env, result, i, str);
		(*env)->DeleteLocalRef(env, str);
	    }
	}
    }
    return result;
}

JNIEXPORT jstring JNICALL
Java_java_util_zip_ZipFile_getZipMessage(JNIEnv *env, jclass cls, jlong zfile)
{
    jzfile *zip = (jzfile *)jlong_to_ptr(zfile);
    char *msg = zip->msg;
    if (msg == NULL) {
        return NULL; 
    } 
    return JNU_NewStringPlatform(env, msg);
}
