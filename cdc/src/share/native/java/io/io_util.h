/*
 * @(#)io_util.h	1.27 06/10/10
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

#include "jni.h"
#include "jni_util.h"

#include "jdkio2cvm.h"

#include "jni_statics.h"

/*
 * Macros to set/get fd from the java.io.FileDescriptor.  These
 * macros rely on having an appropriately defined 'thisObj' object
 * within the scope in which they're used.
 */
#define SET_FD(fd, fid) (*env)->SetIntField(env, \
	                    (*env)->GetObjectField(env, thisObj, (fid)), \
			    JNI_STATIC(java_io_FileDescriptor, IO_fd_fdID), fd)

#define GET_FD(fid) (*env)->GetIntField(env, \
			 (*env)->GetObjectField(env, thisObj, (fid)), \
			 JNI_STATIC(java_io_FileDescriptor, IO_fd_fdID))


/*
 * IO helper functions
 */

jint readSingle(JNIEnv *env, jobject thisObj, jfieldID fid);
jint readBytes(JNIEnv *env, jobject thisObj, jbyteArray bytes, jint off,
	       jint len, jfieldID fid);
void writeSingle(JNIEnv *env, jobject thisObj, jint byte, jfieldID fid);
void writeBytes(JNIEnv *env, jobject thisObj, jbyteArray bytes, jint off,
		jint len, jfieldID fid);
void fileOpen(JNIEnv *env, jobject thisObj, jstring path, jfieldID fid, int flags);


/* Delete-on-exit support */

typedef int (*DELETEPROC)(const char *path);
void deleteOnExit(JNIEnv *env, const char *path, DELETEPROC dp);


/*
 * Macros for managing platform strings.  The typical usage pattern is:
 *
 *     WITH_PLATFORM_STRING(env, string, var) {
 *         doSomethingWith(var);
 *     } END_PLATFORM_STRING(env, var);
 *
 *  where  env      is the prevailing JNIEnv,
 *         string   is a JNI reference to a java.lang.String object, and
 *         var      is the char * variable that will point to the string,
 *                  after being converted into the platform encoding.
 *
 * The related macro WITH_FIELD_PLATFORM_STRING first extracts the string from
 * a given field of a given object:
 *
 *     WITH_FIELD_PLATFORM_STRING(env, object, id, var) {
 *         doSomethingWith(var);
 *     } END_PLATFORM_STRING(env, var);
 *
 *  where  env      is the prevailing JNIEnv,
 *         object   is a jobject,
 *         id       is the field ID of the String field to be extracted, and
 *         var      is the char * variable that will point to the string.
 *
 * Uses of these macros may be nested as long as each WITH_.._STRING macro
 * declares a unique variable.
 */

#define WITH_PLATFORM_STRING(env, strexp, var)                                \
    if (1) {                                                                  \
        const char *var;                                                      \
        jstring _##var##str = (strexp);					      \
        if (_##var##str == NULL) {					      \
            JNU_ThrowNullPointerException((env), NULL);			      \
            goto _##var##end;                                                 \
        }                                                                     \
        var = JNU_GetStringPlatformChars((env), _##var##str, NULL);	      \
        if (var == NULL) goto _##var##end;

#define WITH_FIELD_PLATFORM_STRING(env, object, id, var)                      \
    WITH_PLATFORM_STRING(env,						      \
			 ((object == NULL)				      \
			  ? NULL					      \
			  : (*(env))->GetObjectField((env), (object), (id))), \
			 var)

#define END_PLATFORM_STRING(env, var)                                         \
        JNU_ReleaseStringPlatformChars(env, _##var##str, var);                \
    _##var##end: ;                                                            \
    } else ((void)NULL)

/* Macros for transforming Java Strings into native Unicode strings.
 * Works analogously to WITH_PLATFORM_STRING.
 */

#define WITH_UNICODE_STRING(env, strexp, var)                                 \
    if (1) {                                                                  \
        jchar *var;                                                           \
        jint _##var##length;                                                  \
        jstring _##var##str = (strexp);                                       \
        if (_##var##str == NULL) {                                            \
            JNU_ThrowNullPointerException((env), NULL);                       \
            goto _##var##end;                                                 \
        }                                                                     \
        _##var##length = (*(env))->GetStringLength(env, _##var##str);         \
	var = (jchar*) malloc((_##var##length + 1) * sizeof(jchar));          \
	if (var == NULL) {                                                    \
            JNU_ThrowOutOfMemoryError((env), NULL);                           \
            goto _##var##end;                                                 \
	}                                                                     \
        (*(env))->GetStringRegion((env), _##var##str, 0,_##var##length, var); \
        (*(var + _##var##length)) = 0;

#define END_UNICODE_STRING(env, var)                                          \
        free(var);                                                            \
    _##var##end: ;                                                            \
    } else ((void)NULL)
