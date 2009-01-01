/*
 * @(#)UnixFileSystem_md.c	1.17 06/10/27
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

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
#include <errno.h>
#include <dirent.h>

#include "jni.h"
#include "jni_util.h"
#include "javavm/include/porting/doubleword.h"
#include "javavm/include/porting/io.h"
#include "io_util.h"
#include "jvm.h"
#include "java_io_FileSystem.h"
#include "java_io_UnixFileSystem.h"


/* -- Field IDs -- */

#include "jni_statics.h"

JNIEXPORT void JNICALL
Java_java_io_UnixFileSystem_initIDs(JNIEnv *env, jclass cls)
{
    jclass fileClass = (*env)->FindClass(env, "java/io/File");
    if (!fileClass) return;
    JNI_STATIC_MD(java_io_UnixFileSystem, ids_path) 
    	= (*env)->GetFieldID(env, fileClass, "path", "Ljava/lang/String;");
}


/* -- Large-file support -- */
#if !defined(_LFS_LARGEFILE) || !_LFS_LARGEFILE

#ifdef __GLIBC__
/* Doesn't matter what these are, there is no 64 bit support. */
typedef int u_longlong_t;
typedef int longlong_t;
typedef int timestruc_t;
#define _ST_FSTYPSZ 1
#endif /* __GLIBC__ */

/* The stat64 structure must be provided for systems without large-file support
   (e.g., Solaris 2.5.1).  These definitions are copied from the Solaris 2.6
   <sys/stat.h> and <sys/types.h> files.
 */
typedef longlong_t      off64_t;        /* offsets within files */
typedef u_longlong_t    ino64_t;        /* expanded inode type  */
typedef longlong_t      blkcnt64_t;     /* count of file blocks */

struct	stat64 {
	dev_t	st_dev;
	long	st_pad1[3];
	ino64_t	st_ino;
	mode_t	st_mode;
	nlink_t st_nlink;
	uid_t 	st_uid;
	gid_t 	st_gid;
	dev_t	st_rdev;
	long	st_pad2[2];
	off64_t	st_size;
	timestruc_t st_atime;
	timestruc_t st_mtime;
	timestruc_t st_ctime;
	long	st_blksize;
	blkcnt64_t st_blocks;
	char	st_fstype[_ST_FSTYPSZ];
	long	st_pad4[8];
};

#endif  /* !_LFS_LARGEFILE */

typedef int (*STAT64)(const char *, struct stat64 *);

#if _LARGEFILE64_SOURCE
static STAT64 stat64_ptr = &stat64;
#else
static STAT64 stat64_ptr = NULL;
#endif

/* -- Path operations -- */

/*
 * Defined in canonicalize_md.c
 */
extern int canonicalize(char *path, const char *out, int len);

JNIEXPORT jstring JNICALL
Java_java_io_UnixFileSystem_canonicalize0(JNIEnv *env, jobject this,
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


static jboolean
statMode(const char *path, int *mode)
{
    if (stat64_ptr) {
	struct stat64 sb;
	if (((*stat64_ptr)(path, &sb)) == 0) {
	    *mode = sb.st_mode;
	    return JNI_TRUE;
	}
    } else {
	struct stat sb;
	if (stat(path, &sb) == 0) {
	    *mode = sb.st_mode;
	    return JNI_TRUE;
	}
    }
    return JNI_FALSE;
}


JNIEXPORT jint JNICALL
Java_java_io_UnixFileSystem_getBooleanAttributes0(JNIEnv *env, jobject this,
						  jobject file)
{
    jint rv = 0;

    WITH_FIELD_PLATFORM_STRING(env, file, JNI_STATIC_MD(java_io_UnixFileSystem, ids_path), path) {
	int mode;
	if (statMode(path, &mode)) {
	    int fmt = mode & S_IFMT;
	    rv = (java_io_FileSystem_BA_EXISTS
                  | ((fmt == S_IFREG) ? java_io_FileSystem_BA_REGULAR : 0)
                  | ((fmt == S_IFDIR) ? java_io_FileSystem_BA_DIRECTORY : 0));
	}
    } END_PLATFORM_STRING(env, path);
    return rv;
}


JNIEXPORT jboolean JNICALL
Java_java_io_UnixFileSystem_checkAccess(JNIEnv *env, jobject this,
					jobject file, jboolean write)
{
    jboolean rv = JNI_FALSE;

    WITH_FIELD_PLATFORM_STRING(env, file, JNI_STATIC_MD(java_io_UnixFileSystem, ids_path), path) {
	if (access(path, (write ? W_OK : R_OK)) == 0) {
	    rv = JNI_TRUE;
	}
    } END_PLATFORM_STRING(env, path);
    return rv;
}


JNIEXPORT jlong JNICALL
Java_java_io_UnixFileSystem_getLastModifiedTime(JNIEnv *env, jobject this,
						jobject file)
{
    jlong rv = 0;

    WITH_FIELD_PLATFORM_STRING(env, file, JNI_STATIC_MD(java_io_UnixFileSystem, ids_path), path) {
        if (stat64_ptr) {
            struct stat64 sb;
            if (((*stat64_ptr)(path, &sb)) == 0) {
                rv = 1000 * (jlong)sb.st_mtime;
            }
        } else {
            struct stat sb;
            if (stat(path, &sb) == 0) {
                rv = 1000 * (jlong)sb.st_mtime;
            }
        }
    } END_PLATFORM_STRING(env, path);
    return rv;
}


JNIEXPORT jlong JNICALL
Java_java_io_UnixFileSystem_getLength(JNIEnv *env, jobject this,
				      jobject file)
{
    jlong rv = 0;

    WITH_FIELD_PLATFORM_STRING(env, file, JNI_STATIC_MD(java_io_UnixFileSystem, ids_path), path) {
	if (stat64_ptr) {
	    struct stat64 sb;
	    if (((*stat64_ptr)(path, &sb)) == 0) {
		rv = sb.st_size;
	    }
	} else {
	    struct stat sb;
	    if (stat(path, &sb) == 0) {
		rv = sb.st_size;
	    }
	}
    } END_PLATFORM_STRING(env, path);
    return rv;
}


/* -- File operations -- */


JNIEXPORT jboolean JNICALL
Java_java_io_UnixFileSystem_createFileExclusively(JNIEnv *env, jclass cls,
						  jstring pathname)
{
    jboolean rv = JNI_FALSE;

    WITH_PLATFORM_STRING(env, pathname, path) {
	if (strcmp(path, "/") == 0) {
	    /* The root directory always exists */
	} else {
	    int fd = open(path, O_RDWR | O_CREAT | O_EXCL, 0666);
	    if (fd < 0) {
		if (errno != EEXIST) {
		    JNU_ThrowIOExceptionWithLastError(env, path);
		}
	    } else {
		close(fd);
		rv = JNI_TRUE;
	    }
	}
    } END_PLATFORM_STRING(env, path);
    return rv;
}


JNIEXPORT jboolean JNICALL
Java_java_io_UnixFileSystem_delete0(JNIEnv *env, jobject this,
				   jobject file)
{
    jboolean rv = JNI_FALSE;

    WITH_FIELD_PLATFORM_STRING(env, file, JNI_STATIC_MD(java_io_UnixFileSystem, ids_path), path) {
	if (remove(path) == 0) {
	    rv = JNI_TRUE;
	}
    } END_PLATFORM_STRING(env, path);
    return rv;
}


JNIEXPORT jboolean JNICALL
Java_java_io_UnixFileSystem_deleteOnExit(JNIEnv *env, jobject this,
					 jobject file)
{
    WITH_FIELD_PLATFORM_STRING(env, file, JNI_STATIC_MD(java_io_UnixFileSystem, ids_path), path) {
	deleteOnExit(env, path, remove);
    } END_PLATFORM_STRING(env, path);
    return JNI_TRUE;
}


JNIEXPORT jobjectArray JNICALL
Java_java_io_UnixFileSystem_list(JNIEnv *env, jobject this,
				 jobject file)
{
    DIR *dir = NULL;
    struct dirent *ptr;
    int len, maxlen;
    jobjectArray rv, old;

    WITH_FIELD_PLATFORM_STRING(env, file, JNI_STATIC_MD(java_io_UnixFileSystem, ids_path), path) {
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
Java_java_io_UnixFileSystem_createDirectory(JNIEnv *env, jobject this,
					    jobject file)
{
    jboolean rv = JNI_FALSE;

    WITH_FIELD_PLATFORM_STRING(env, file, JNI_STATIC_MD(java_io_UnixFileSystem, ids_path), path) {
	if (mkdir(path, 0777) == 0) {
	    rv = JNI_TRUE;
	}
    } END_PLATFORM_STRING(env, path);
    return rv;
}


JNIEXPORT jboolean JNICALL
Java_java_io_UnixFileSystem_rename0(JNIEnv *env, jobject this,
				   jobject from, jobject to)
{
    jboolean rv = JNI_FALSE;

    WITH_FIELD_PLATFORM_STRING(env, from, JNI_STATIC_MD(java_io_UnixFileSystem, ids_path), fromPath) {
	WITH_FIELD_PLATFORM_STRING(env, to, JNI_STATIC_MD(java_io_UnixFileSystem, ids_path), toPath) {
	    if (rename(fromPath, toPath) == 0) {
		rv = JNI_TRUE;
	    }
	} END_PLATFORM_STRING(env, toPath);
    } END_PLATFORM_STRING(env, fromPath);
    return rv;
}

JNIEXPORT jboolean JNICALL  
Java_java_io_UnixFileSystem_setLastModifiedTime(JNIEnv *env, jobject this,
						jobject file, jlong time)
{
    jboolean rv = JNI_FALSE;

    WITH_FIELD_PLATFORM_STRING(env, file, JNI_STATIC_MD(java_io_UnixFileSystem, ids_path), path) {
	struct timeval tv[2];
#ifdef __linux__
	long ts;
#else
	timestruc_t ts;
#endif

        if (stat64_ptr) {
            struct stat64 sb;
            if (((*stat64_ptr)(path, &sb)) == 0)
#ifdef __linux__
		ts = sb.st_atime;
#else
		ts = sb.st_atim;
#endif
	    else
		goto error;
	} else {
            struct stat sb;
            if (stat(path, &sb) == 0)
#ifdef __linux__
                ts = sb.st_atime;
#else
                ts = sb.st_atim;
#endif
	    else
		goto error;
	}

	/* Preserve access time */
#ifdef __linux__
	tv[0].tv_sec = ts;
	tv[0].tv_usec = 0;
#else
	tv[0].tv_sec = ts.tv_sec;
	tv[0].tv_usec = ts.tv_nsec / 1000;
#endif

	/* Change last-modified time */
	tv[1].tv_sec = time / 1000;
	tv[1].tv_usec = (time % 1000) * 1000;

        if (utimes(path, tv) >= 0)
            rv = JNI_TRUE;

    error: ;
    } END_PLATFORM_STRING(env, path);

    return rv;
}


JNIEXPORT jboolean JNICALL  
Java_java_io_UnixFileSystem_setReadOnly(JNIEnv *env, jobject this,
					jobject file)
{
    jboolean rv = JNI_FALSE;

    WITH_FIELD_PLATFORM_STRING(env, file, JNI_STATIC_MD(java_io_UnixFileSystem, ids_path), path) {
	int mode;
	if (statMode(path, &mode)) {
	    if (chmod(path, mode & ~(S_IWUSR | S_IWGRP | S_IWOTH)) >= 0) {
                rv = JNI_TRUE;
            }
	}
    } END_PLATFORM_STRING(env, path);
    return rv;
}
