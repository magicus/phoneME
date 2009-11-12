/*
 * @(#)SymbianFileSystem_md.cpp	1.6 06/10/10
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
#include <errno.h>
#include <dirent.h>


extern "C" {

#include "jni.h"
#include "jni_util.h"
#include "javavm/include/porting/doubleword.h"
#include "javavm/include/porting/io.h"
#include "io_util.h"
#include "jvm.h"
#include "java_io_FileSystem.h"
#include "java_io_SymbianFileSystem.h"


/* -- Field IDs -- */

#include "jni_statics.h"

}

#define ids_path JNI_STATIC_MD(java_io_SymbianFileSystem, ids_path)

#include <e32def.h>
#include <f32file.h>

/* Work-around for Symbian tool bug.  No longer needed. */
static JNINativeMethod methods[] = {
    {"listRoots0", "()I", (void *)&Java_java_io_SymbianFileSystem_listRoots0}
};

JNIEXPORT void JNICALL
Java_java_io_SymbianFileSystem_initIDs(JNIEnv *env, jclass cls)
{
    jclass fileClass = (*env)->FindClass(env, "java/io/File");
    if (!fileClass) return;
    ids_path = (*env)->GetFieldID(env, fileClass, "path", "Ljava/lang/String;");
    /* Work-around for Symbian tool bug.  No longer needed. */
    (*env)->RegisterNatives(env, cls, methods,
	sizeof methods / sizeof methods[0]);
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
typedef CVMInt64	off64_t;        /* offsets within files */
typedef CVMUint64	ino64_t;        /* expanded inode type  */
typedef CVMInt64	blkcnt64_t;     /* count of file blocks */

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
	long st_atime;
	long st_mtime;
	long st_ctime;
	long	st_blksize;
	blkcnt64_t st_blocks;
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
extern "C" int canonicalize(char *path, const char *out, int len);

JNIEXPORT jstring JNICALL
Java_java_io_SymbianFileSystem_canonicalize0(JNIEnv *env, jobject thisObj,
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
Java_java_io_SymbianFileSystem_getBooleanAttributes(JNIEnv *env,
						    jobject thisObj,
						    jobject file)
{
    jint rv = 0;

    WITH_FIELD_PLATFORM_STRING(env, file, ids_path, path) {
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
Java_java_io_SymbianFileSystem_checkAccess(JNIEnv *env, jobject thisObj,
					jobject file, jboolean write)
{
    jboolean rv = JNI_FALSE;

    WITH_FIELD_PLATFORM_STRING(env, file, ids_path, path) {
	if (access(path, (write ? W_OK : R_OK)) == 0) {
	    rv = JNI_TRUE;
	}
    } END_PLATFORM_STRING(env, path);
    return rv;
}


JNIEXPORT jlong JNICALL
Java_java_io_SymbianFileSystem_getLastModifiedTime(JNIEnv *env, jobject thisObj,
						jobject file)
{
    jlong rv = 0;

    WITH_FIELD_PLATFORM_STRING(env, file, ids_path, path) {
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
Java_java_io_SymbianFileSystem_getLength(JNIEnv *env, jobject thisObj,
				      jobject file)
{
    jlong rv = 0;

    WITH_FIELD_PLATFORM_STRING(env, file, ids_path, path) {
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
Java_java_io_SymbianFileSystem_createFileExclusively(JNIEnv *env, jclass cls,
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
Java_java_io_SymbianFileSystem_delete0(JNIEnv *env, jobject thisObj,
				   jobject file)
{
    jboolean rv = JNI_FALSE;

    WITH_FIELD_PLATFORM_STRING(env, file, ids_path, path) {
	if (remove(path) == 0) {
	    rv = JNI_TRUE;
	}
    } END_PLATFORM_STRING(env, path);
    return rv;
}


JNIEXPORT jboolean JNICALL
Java_java_io_SymbianFileSystem_deleteOnExit(JNIEnv *env, jobject thisObj,
					 jobject file)
{
    WITH_FIELD_PLATFORM_STRING(env, file, ids_path, path) {
	deleteOnExit(env, path, unlink);
    } END_PLATFORM_STRING(env, path);
    return JNI_TRUE;
}

#if 1
RHeap *SYMBIANgetGlobalHeap();
#endif

JNIEXPORT jobjectArray JNICALL
Java_java_io_SymbianFileSystem_list(JNIEnv *env, jobject thisObj,
				 jobject file)
{
    DIR *dir = NULL;
    struct dirent *ptr;
    int len, maxlen;
    jobjectArray rv, old;

#if 1
    // Switch to global heap.  RHeap should be MT-safe.
    RHeap *gHeap = SYMBIANgetGlobalHeap();
    RHeap *tHeap = User::SwitchHeap(gHeap);
#endif

    WITH_FIELD_PLATFORM_STRING(env, file, ids_path, path) {
	dir = opendir(path);
    } END_PLATFORM_STRING(env, path);
    if (dir == NULL) {
	goto done;
    }

    /* Allocate an initial String array */
    len = 0;
    maxlen = 16;
    rv = (*env)->NewObjectArray(env, maxlen, JNU_ClassString(env), NULL);
    if (rv == NULL) {
	goto error;
    }

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
#if 1
    (void)User::SwitchHeap(tHeap);
#endif

    /* Copy the final results into an appropriately-sized array */
    old = rv;
    rv = (*env)->NewObjectArray(env, len, JNU_ClassString(env), NULL);
    if (rv == NULL) {
	goto error;
    }
    if (JNU_CopyObjectArray(env, rv, old, len) < 0) {
	goto error;
    }
    return rv;

 error:
    closedir(dir);
 done:
#if 1
    (void)User::SwitchHeap(tHeap);
#endif
    return NULL;
}


JNIEXPORT jboolean JNICALL
Java_java_io_SymbianFileSystem_createDirectory(JNIEnv *env, jobject thisObj,
					    jobject file)
{
    jboolean rv = JNI_FALSE;

    WITH_FIELD_PLATFORM_STRING(env, file, ids_path, path) {
	if (mkdir(path, 0777) == 0) {
	    rv = JNI_TRUE;
	}
    } END_PLATFORM_STRING(env, path);
    return rv;
}


JNIEXPORT jboolean JNICALL
Java_java_io_SymbianFileSystem_rename0(JNIEnv *env, jobject thisObj,
				   jobject from, jobject to)
{
    jboolean rv = JNI_FALSE;

    WITH_FIELD_PLATFORM_STRING(env, from, ids_path, fromPath) {
	WITH_FIELD_PLATFORM_STRING(env, to, ids_path, toPath) {
	    if (rename(fromPath, toPath) == 0) {
		rv = JNI_TRUE;
	    }
	} END_PLATFORM_STRING(env, toPath);
    } END_PLATFORM_STRING(env, fromPath);
    return rv;
}

JNIEXPORT jboolean JNICALL  
Java_java_io_SymbianFileSystem_setLastModifiedTime(JNIEnv *env, jobject thisObj,
						jobject file, jlong time)
{
    jboolean rv = JNI_FALSE;

    WITH_FIELD_PLATFORM_STRING(env, file, ids_path, path) {
	struct timeval tv[2];
	long ts;

        if (stat64_ptr) {
            struct stat64 sb;
            if (((*stat64_ptr)(path, &sb)) == 0)
		ts = sb.st_atime;
	    else
		goto error;
	} else {
            struct stat sb;
            if (stat(path, &sb) == 0)
                ts = sb.st_atime;
	    else
		goto error;
	}

	/* Change last-modified time */
	tv[1].tv_sec = time / 1000;
	tv[1].tv_usec = (time % 1000) * 1000;

#if 0
	RFile rf;
	if (rf.Open(fs, path, mode) == KErrNone) {
	    TTime t;
	    if (rf.SetModified(...) == KErrNone) {
		rv = JNI_TRUE;
	    }
	}
	if (fs.SetModified(path, time) == KErrNone) {
	    rv = JNI_TRUE;
	}
#endif

    error: ;
    } END_PLATFORM_STRING(env, path);

    return rv;
}


JNIEXPORT jboolean JNICALL  
Java_java_io_SymbianFileSystem_setReadOnly(JNIEnv *env, jobject thisObj,
					jobject file)
{
    jboolean rv = JNI_FALSE;

    WITH_FIELD_PLATFORM_STRING(env, file, ids_path, path) {
	int mode;
	if (statMode(path, &mode)) {
	    if (chmod(path, mode & ~(S_IWUSR | S_IWGRP | S_IWOTH)) >= 0) {
                rv = JNI_TRUE;
            }
	}
    } END_PLATFORM_STRING(env, path);
    return rv;
}

JNIEXPORT jint JNICALL
Java_java_io_SymbianFileSystem_listRoots0(JNIEnv *env, jclass ignored)
{
    RFs fs;
    if (fs.Connect() != KErrNone) {
	fprintf(stderr, "Cannot connect to file system!\n");
	return 0;
    }
    TDriveList dl;
    TInt r = fs.DriveList(dl);
    fs.Close();
    if (r != KErrNone) {
	return 0;
    } else {
	jint bits = 0;
	int i;
	for (i = 0; i < 26; ++i) {
	    if (dl[i]) {
		bits |= 1 << i;
	    }
	}
	return bits;
    }
}

JNIEXPORT jstring JNICALL
Java_java_io_SymbianFileSystem_getDriveDirectory(JNIEnv *env, jclass ignored,
						 jint drive)
{
    return JNU_NewStringPlatform(env, "\\");
}
