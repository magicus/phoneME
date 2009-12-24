/*
 * @(#)VxWorksFileSystem_md.c	1.13 06/10/10
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

#include <vxWorks.h>
#include <ioLib.h>
#include <dosFsLib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>

#include "jni.h"
#include "jni_util.h"

#include "javavm/include/porting/doubleword.h"
#include "javavm/include/porting/io.h"

#include "io_util.h"
#include "jvm.h"
#include "java_io_FileSystem.h"
#include "java_io_VxWorksFileSystem.h"

#define MAXPATHLEN	MAX_FILENAME_LENGTH
#define VXWORKS_STAT_BUG

/* -- Field IDs -- */

/*
static struct {
    jfieldID path;
} ids;
*/

#define VXWORKS_LONGPATH_BUG

#ifdef VXWORKS_LONGPATH_BUG
STATUS safeStat(const char *name, struct stat  *pStat)
{
    if (strlen(name) < MAXPATHLEN - 1) {
	return stat((char *)name, pStat);
    } else {
	errno = ENAMETOOLONG;
	return ERROR;
    }
}
#define stat(n,p) safeStat((n),(p))
#endif

JNIEXPORT void JNICALL
Java_java_io_VxWorksFileSystem_initIDs(JNIEnv *env, jclass cls)
{
    jclass fileClass = (*env)->FindClass(env, "java/io/File");
    if (!fileClass) return;
    JNI_STATIC_MD(java_io_VxWorksFileSystem, ids_path) = (*env)->GetFieldID(env, fileClass,
				  "path", "Ljava/lang/String;");
}


/* -- Path operations -- */

/*
 * Defined in canonicalize_md.c
 */
extern int canonicalize(char *path, const char *out, int len);

JNIEXPORT jstring JNICALL
Java_java_io_VxWorksFileSystem_canonicalize(JNIEnv *env, jobject this,
					    jstring pathname)
{
    jstring rv = NULL;

    WITH_PLATFORM_STRING(env, pathname, path) {
	char canonicalPath[MAXPATHLEN];
	if (canonicalize((char*)path,
			 canonicalPath, MAXPATHLEN) < 0) {
	    JNU_ThrowIOExceptionWithLastError(env, "Bad pathname");
	} else {
	    rv = JNU_NewStringPlatform(env, canonicalPath);
	}
    } END_PLATFORM_STRING(env, path);
    return rv;
}


/* -- Attribute accessors -- */


JNIEXPORT jint JNICALL
Java_java_io_VxWorksFileSystem_getBooleanAttributes0(JNIEnv *env, jobject this,
						     jobject file)
{
    jint rv = 0;

    WITH_FIELD_PLATFORM_STRING(env, file, JNI_STATIC_MD(java_io_VxWorksFileSystem, ids_path), path) {
	struct stat sb;
	if (stat(path, &sb) == 0) {
	    int fmt = sb.st_mode & S_IFMT;
	    rv = (java_io_FileSystem_BA_EXISTS
		  | ((fmt == S_IFREG) ? java_io_FileSystem_BA_REGULAR : 0)
		  | ((fmt == S_IFDIR) ? java_io_FileSystem_BA_DIRECTORY : 0));
	}
    } END_PLATFORM_STRING(env, path);
    return rv;
}


JNIEXPORT jboolean JNICALL
Java_java_io_VxWorksFileSystem_checkAccess(JNIEnv *env, jobject this,
					   jobject file, jboolean write)
{
    jboolean rv = JNI_FALSE;

    WITH_FIELD_PLATFORM_STRING(env, file, JNI_STATIC_MD(java_io_VxWorksFileSystem, ids_path), path) {
	int fd = open(path, write ? O_WRONLY : O_RDONLY, 0);
	if (fd != ERROR) {
	    close(fd);
	    rv = JNI_TRUE;
	}
    } END_PLATFORM_STRING(env, path);
    return rv;
}


JNIEXPORT jlong JNICALL
Java_java_io_VxWorksFileSystem_getLastModifiedTime(JNIEnv *env, jobject this,
						   jobject file)
{
    jlong rv = 0;

    WITH_FIELD_PLATFORM_STRING(env, file, JNI_STATIC_MD(java_io_VxWorksFileSystem, ids_path), path) {
	struct stat sb;
	if (stat(path, &sb) == 0) {
	    rv = 1000 * (jlong)sb.st_mtime;
	}
    } END_PLATFORM_STRING(env, path);
    return rv;
}

JNIEXPORT jlong JNICALL
Java_java_io_VxWorksFileSystem_getLength(JNIEnv *env, jobject this,
					 jobject file)
{
    jlong rv = 0;

    WITH_FIELD_PLATFORM_STRING(env, file, JNI_STATIC_MD(java_io_VxWorksFileSystem, ids_path), path) {
	struct stat sb;
	if (stat(path, &sb) == 0) {
	    rv = sb.st_size;
	}
    } END_PLATFORM_STRING(env, path);
    return rv;
}


/* -- File operations -- */


JNIEXPORT jboolean JNICALL
Java_java_io_VxWorksFileSystem_createFileExclusively(JNIEnv *env, jclass cls,
						     jstring pathname)
{
    jboolean rv = JNI_FALSE;

    WITH_PLATFORM_STRING(env, pathname, path) {
	int result = CVMioOpen(path, O_RDWR | O_CREAT | O_EXCL, 0666);
	if (result < 0) {
	    if (errno != EEXIST) {
		JNU_ThrowIOExceptionWithLastError(env, path);
	    }
	} else {
	    CVMioClose(result);
	    rv = JNI_TRUE;
	}
    } END_PLATFORM_STRING(env, path);
    return rv;
}


JNIEXPORT jboolean JNICALL
Java_java_io_VxWorksFileSystem_delete(JNIEnv *env, jobject this,
				      jobject file)
{
    jboolean rv = JNI_FALSE;

    WITH_FIELD_PLATFORM_STRING(env, file, JNI_STATIC_MD(java_io_VxWorksFileSystem, ids_path), path) {
	if (remove(path) == 0) {
	    rv = JNI_TRUE;
	}
    } END_PLATFORM_STRING(env, path);
    return rv;
}


JNIEXPORT jboolean JNICALL
Java_java_io_VxWorksFileSystem_deleteOnExit(JNIEnv *env, jobject this,
					    jobject file)
{
    WITH_FIELD_PLATFORM_STRING(env, file, JNI_STATIC_MD(java_io_VxWorksFileSystem, ids_path), path) {
	deleteOnExit(env, path, remove);
    } END_PLATFORM_STRING(env, path);
    return JNI_TRUE;
}


JNIEXPORT jobjectArray JNICALL
Java_java_io_VxWorksFileSystem_list(JNIEnv *env, jobject this,
				    jobject file)
{
    DIR *dir = NULL;
    struct dirent *ptr;
    int len, maxlen;
    jobjectArray rv, old;

    WITH_FIELD_PLATFORM_STRING(env, file, JNI_STATIC_MD(java_io_VxWorksFileSystem, ids_path), path) {
	dir = opendir(path);
    } END_PLATFORM_STRING(env, path);
    if (dir == NULL) return NULL;

    /* Allocate an initial String array */
    len = 0;
    maxlen = 16;
    rv = (*env)->NewObjectArray(env, maxlen, JNU_ClassString(env), NULL);
    if (rv == NULL) goto error;

    /* Scan the directory */
    while ((ptr = readdir(dir)) != NULL) {
	jstring name;
  	if (!strcmp(ptr->d_name, ".") || !strcmp(ptr->d_name, ".."))
	    continue;
	if (len == maxlen) {
	    old = rv;
	    rv = (*env)->NewObjectArray(env, maxlen <<= 1,
					JNU_ClassString(env), NULL);
	    if (rv == NULL) goto error;
	    if (JNU_CopyObjectArray(env, rv, old, len) < 0) goto error;
	    (*env)->DeleteLocalRef(env, old);
	}
	name = JNU_NewStringPlatform(env, ptr->d_name);
	if (name == NULL) goto error;
	(*env)->SetObjectArrayElement(env, rv, len++, name);
	(*env)->DeleteLocalRef(env, name);
    }
    closedir(dir);

    /* Copy the final results into an appropriately-sized array */
    old = rv;
    rv = (*env)->NewObjectArray(env, len, JNU_ClassString(env), NULL);
    if (rv == NULL) goto error;
    if (JNU_CopyObjectArray(env, rv, old, len) < 0) goto error;
    return rv;

 error:
    closedir(dir);
    return NULL;
}


JNIEXPORT jboolean JNICALL
Java_java_io_VxWorksFileSystem_createDirectory(JNIEnv *env, jobject this,
					       jobject file)
{
    jboolean rv = JNI_FALSE;

    WITH_FIELD_PLATFORM_STRING(env, file, JNI_STATIC_MD(java_io_VxWorksFileSystem, ids_path), path) {
	if (mkdir(path) == 0) {
	    rv = JNI_TRUE;
	}
    } END_PLATFORM_STRING(env, path);
    return rv;
}


JNIEXPORT jboolean JNICALL
Java_java_io_VxWorksFileSystem_rename(JNIEnv *env, jobject this,
				      jobject from, jobject to)
{
    jboolean rv = JNI_FALSE;

    WITH_FIELD_PLATFORM_STRING(env, from, JNI_STATIC_MD(java_io_VxWorksFileSystem, ids_path), fromPath) {
	WITH_FIELD_PLATFORM_STRING(env, to, JNI_STATIC_MD(java_io_VxWorksFileSystem, ids_path), toPath) {
	    int fd = open(fromPath, O_RDONLY, 0);
	    if (fd != -1) {
		if (ioctl(fd, FIORENAME, (int)toPath) == 0) {
		    rv = JNI_TRUE;
		}
		close(fd);
	    }
	} END_PLATFORM_STRING(env, toPath);
    } END_PLATFORM_STRING(env, fromPath);
    return rv;
}

JNIEXPORT jboolean JNICALL  
Java_java_io_VxWorksFileSystem_setLastModifiedTime(JNIEnv *env, jobject this,
						   jobject file, jlong time)
{
    return JNI_FALSE;
}


JNIEXPORT jboolean JNICALL  
Java_java_io_VxWorksFileSystem_setReadOnly(JNIEnv *env, jobject this,
					   jobject file)
{
    jboolean rv = JNI_FALSE;

    WITH_FIELD_PLATFORM_STRING(env, file, JNI_STATIC_MD(java_io_VxWorksFileSystem, ids_path), path) {
	struct stat sb;
	int fd = open(path, O_RDONLY, 0);
	if (fd != -1) {
	    fstat(fd, &sb);
	    ioctl(fd, FIOATTRIBSET, (sb.st_attrib | DOS_ATTR_RDONLY));
	    close(fd);                          /* close file         */
	    rv = JNI_TRUE;
	}
    } END_PLATFORM_STRING(env, path);
    return rv;
}
