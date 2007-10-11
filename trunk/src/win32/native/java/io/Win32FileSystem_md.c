/*
 * @(#)Win32FileSystem_md.c	1.24 06/10/10
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.  
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
#ifndef WINCE
#define CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <windows.h>
#ifndef WINCE
#include <io.h>
#endif

#include "jvm.h"
#include "jni.h"
#include "jni_util.h"
#include "javavm/include/porting/doubleword.h"
#include "javavm/include/porting/io.h"
#include "io_util.h"
#include "dirent_md.h"
#include "java_io_FileSystem.h"
#include "java_io_Win32FileSystem.h"

#define MAXPATHLEN   MAX_PATH

/* This macro relies upon the fact that JNU_GetStringPlatformChars always makes
   a copy of the string */

#ifdef WINCE
#define WITH_WIN32_STRING(path)   { WCHAR *wc = createWCHAR(path); 
#define END_WIN32_STRING   free(wc); }
#else
#define WITH_WIN32_STRING(path) 	{ const char *wc = path; 
#define END_WIN32_STRING  } 
#undef GetFileAttributes
#define GetFileAttributes GetFileAttributesA
#undef SetFileAttributes
#define SetFileAttributes SetFileAttributesA
#undef FindFirstFile
#define FindFirstFile FindFirstFileA
#undef FindNextFile
#define FindNextFile FindNextFileA
#define WIN32_FIND_DATA WIN32_FIND_DATAA
#undef CreateFile
#define CreateFile CreateFileA
#undef DeleteFile
#define DeleteFile DeleteFileA
#undef CreateDirectory
#define CreateDirectory CreateDirectoryA
#undef RemoveDirectory
#define RemoveDirectory RemoveDirectoryA
#endif	
/* -- Field IDs -- */

#include "jni_statics.h"

#define ids_path	JNI_STATIC_MD(java_io_Win32FileSystem, ids_path)

JNIEXPORT void JNICALL
Java_java_io_Win32FileSystem_initIDs(JNIEnv *env, jclass cls)
{
    jclass fileClass = (*env)->FindClass(env, "java/io/File");
    if (!fileClass) return;
    ids_path = (*env)->GetFieldID(env, fileClass,
				  "path", "Ljava/lang/String;");
}

/*
 * Defined in canonicalize_md.c
 */
extern int canonicalize(char *path, const char *out, int len);

JNIEXPORT jstring JNICALL
Java_java_io_Win32FileSystem_canonicalize0(JNIEnv *env, jobject this,
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
Java_java_io_Win32FileSystem_getBooleanAttributes(JNIEnv *env, jobject this,
						  jobject file)
{
    jint rv = 0;
    
    DWORD  mode;
    /* TODO: in winCE, we should be using unicode */
    const char* path = CVMjniGetStringUTFChars(env, file, FALSE);
    
    WITH_WIN32_STRING(path) {
	mode = GetFileAttributes(wc);
    } END_WIN32_STRING;
    
    if (mode != 0xffffffff) {
	rv = (java_io_FileSystem_BA_EXISTS 
	      | ((mode & FILE_ATTRIBUTE_DIRECTORY)
		 ? java_io_FileSystem_BA_DIRECTORY
		 : java_io_FileSystem_BA_REGULAR)
	      | ((mode & FILE_ATTRIBUTE_HIDDEN)
		 ? java_io_FileSystem_BA_HIDDEN : 0));
    }
    
    return rv;
}

JNIEXPORT jboolean JNICALL
Java_java_io_Win32FileSystem_checkAccess(JNIEnv *env, jobject this,
					 jobject file, jboolean write)
{
    jboolean rv = JNI_FALSE;
    
    WITH_FIELD_PLATFORM_STRING(env, file, ids_path, path) {
	if (_access(path, (write ? 2 : 4)) == 0) {
	    rv = JNI_TRUE;
	}
    } END_PLATFORM_STRING(env, path);
    return rv;
}


JNIEXPORT jlong JNICALL
Java_java_io_Win32FileSystem_getLastModifiedTime(JNIEnv *env, jobject this,
						 jobject file)
{
    jlong rv = 0;
    
    WITH_FIELD_PLATFORM_STRING(env, file, ids_path, path) {
	WITH_WIN32_STRING(path) {
	    WIN32_FIND_DATA fd;
	    jlong temp = 0;
	    LARGE_INTEGER modTime;
	    HANDLE h;
	    
	    h = FindFirstFile(wc, &fd);
	    if (h != INVALID_HANDLE_VALUE) {
		FindClose(h);
		modTime.LowPart = (DWORD) fd.ftLastWriteTime.dwLowDateTime;
		modTime.HighPart = (LONG) fd.ftLastWriteTime.dwHighDateTime;
		rv = modTime.QuadPart / 10000;
		rv -= 11644473600000;
	    }
	} END_WIN32_STRING;
    } END_PLATFORM_STRING(env, path);
    return rv;
}


JNIEXPORT jlong JNICALL
Java_java_io_Win32FileSystem_getLength(JNIEnv *env, jobject this,
				       jobject file)
{
    jlong rv = 0;
    
    WITH_FIELD_PLATFORM_STRING(env, file, ids_path, path) {
	HANDLE h;
	
	WITH_WIN32_STRING(path) {
	    h  = CreateFile(wc, GENERIC_READ ,FILE_SHARE_READ,
			    NULL, OPEN_EXISTING, 0, 0);
	}  END_WIN32_STRING;
	
	if (h != INVALID_HANDLE_VALUE) {
	    DWORD hi;
	    DWORD lo = GetFileSize(h, &hi);
		if ((lo != 0xffffffff) || (GetLastError() == NO_ERROR)) {
		    rv = lo + ((jlong)hi << 32);
		}
		CloseHandle(h);
	}
    } END_PLATFORM_STRING(env, path);
    return rv;
}


/* -- File operations -- */


JNIEXPORT jboolean JNICALL
Java_java_io_Win32FileSystem_createFileExclusively(JNIEnv *env, jclass cls,
						   jstring pathname)
{
    jboolean rv = JNI_FALSE;
    
    WITH_PLATFORM_STRING(env, pathname, path) {
        HANDLE h;
	
	WITH_WIN32_STRING(path) {
	    h= CreateFile(wc, GENERIC_READ | GENERIC_WRITE, 0, NULL, 
			  CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0);
	} END_WIN32_STRING;
	
	if ( h == INVALID_HANDLE_VALUE ) {
	    if (GetLastError() != ERROR_FILE_EXISTS ) 
		JNU_ThrowIOExceptionWithLastError(env, path);
	} else {
	    CloseHandle(h);
	    rv = JNI_TRUE;
	}
    } END_PLATFORM_STRING(env, path);
    return rv;
}


static int
removeFileOrDirectory(const char *path) /* Returns 0 on success */
{
    DWORD a;
    BOOL   flag;
    
    WITH_WIN32_STRING(path) {
       	SetFileAttributes(wc, 0);
        a = GetFileAttributes(wc);
	
	if (a == 0xffffffff) {
	    flag = FALSE;
	} else if (a & FILE_ATTRIBUTE_DIRECTORY) {
	    flag = RemoveDirectory(wc);
	} else {
	    flag = DeleteFile(wc);
	}
    } END_WIN32_STRING;
    
    return flag;
}

JNIEXPORT jboolean JNICALL
Java_java_io_Win32FileSystem_delete0(JNIEnv *env, jobject this,
                                     jobject file)
{
    jboolean rv = JNI_FALSE;
    
    WITH_FIELD_PLATFORM_STRING(env, file, ids_path, path) {
	if (removeFileOrDirectory(path)) {
	    rv = JNI_TRUE;
	}
    } END_PLATFORM_STRING(env, path);
    return rv;
}


JNIEXPORT jboolean JNICALL
Java_java_io_Win32FileSystem_deleteOnExit(JNIEnv *env, jobject this,
					  jobject file)
{
    WITH_FIELD_PLATFORM_STRING(env, file, ids_path, path) {
	deleteOnExit(env, path, removeFileOrDirectory);
    } END_PLATFORM_STRING(env, path);
    return JNI_TRUE;
}


JNIEXPORT jobjectArray JNICALL
Java_java_io_Win32FileSystem_list(JNIEnv *env, jobject this,
				  jobject file)
{
    HANDLE fileHandle = NULL;
    WIN32_FIND_DATA findData;
    int len, maxlen;
    jobjectArray rv, old;
    
    WITH_FIELD_PLATFORM_STRING(env, file, ids_path, path) {
	char str0[256];
	sprintf(str0, (const char *)"%s\\\\*", path);
	WITH_WIN32_STRING(str0) {
	    fileHandle = FindFirstFile(wc, &findData );
	} END_WIN32_STRING;
    } END_PLATFORM_STRING(env, path);
    if (fileHandle == INVALID_HANDLE_VALUE ) {
        return NULL;
    }
    
    /* Allocate an initial String array */
    len = 0;
    maxlen = 16;
    rv = (*env)->NewObjectArray(env, maxlen, JNU_ClassString(env), NULL);
    if (rv == NULL) goto error;
    
    /* Scan the directory */
    do {
	
	jstring name;
#ifdef WINCE
	char *mname = createMCHAR(findData.cFileName);
#else
	char *mname = findData.cFileName;
#endif
	if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
	    continue;
	}
	if (!strcmp(mname, ".") || !strcmp(mname, "..")) {
	    continue;
	}
	if (len == maxlen) {
	    old = rv;
	    rv = (*env)->NewObjectArray(env, maxlen <<= 1,
					JNU_ClassString(env), NULL);
	    if (rv == NULL) 
		goto error;
	    if (JNU_CopyObjectArray(env, rv, old, len) < 0)
		goto error;
	    (*env)->DeleteLocalRef(env, old);
	}
	name = JNU_NewStringPlatform(env, mname);
#ifdef WINCE
	free(mname);
#endif
	if (name == NULL)
	    goto error;
	(*env)->SetObjectArrayElement(env, rv, len++, name);
	(*env)->DeleteLocalRef(env, name);
    } while (FindNextFile(fileHandle, &findData)) ;
    FindClose(fileHandle);
    /* Copy the final results into an appropriately-sized array */
    old = rv;
    rv = (*env)->NewObjectArray(env, len, JNU_ClassString(env), NULL);
    if (rv == NULL) 
	goto error;
    if (JNU_CopyObjectArray(env, rv, old, len) < 0) 
	goto error;
    return rv;
    
 error:
    
    FindClose(fileHandle);
    return NULL;
}


JNIEXPORT jboolean JNICALL
Java_java_io_Win32FileSystem_createDirectory(JNIEnv *env, jobject this,
					     jobject file)
{
    jboolean rv = JNI_FALSE;
    
    WITH_FIELD_PLATFORM_STRING(env, file, ids_path, path) {
	WITH_WIN32_STRING(path) {
	    if (CreateDirectory(wc, NULL) ) {
	    	rv = JNI_TRUE;
	    }
	} END_WIN32_STRING;
    } END_PLATFORM_STRING(env, path);
    return rv;
}


JNIEXPORT jboolean JNICALL
Java_java_io_Win32FileSystem_rename0(JNIEnv *env, jobject this,
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
Java_java_io_Win32FileSystem_setLastModifiedTime(JNIEnv *env, jobject this,
						 jobject file, jlong time)
{
    jboolean rv = JNI_FALSE;
    
    WITH_FIELD_PLATFORM_STRING(env, file, ids_path, path) {
	WITH_WIN32_STRING(path) {
	    HANDLE h = 
		CreateFile(wc, 
			   GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
			   FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
			   0);
	    if (h != INVALID_HANDLE_VALUE) {
		LARGE_INTEGER modTime;
		FILETIME t;
		modTime.QuadPart = (time + 11644473600000L) * 10000L;
		t.dwLowDateTime = (DWORD)modTime.LowPart;
		t.dwHighDateTime = (DWORD)modTime.HighPart;
		if (SetFileTime(h, NULL, NULL, &t)) {
		    rv = JNI_TRUE;
		}
		CloseHandle(h);
	    }
	} END_WIN32_STRING;
    } END_PLATFORM_STRING(env, path);
    
    return rv;
}


JNIEXPORT jboolean JNICALL  
Java_java_io_Win32FileSystem_setReadOnly(JNIEnv *env, jobject this,
					 jobject file)
{
    jboolean rv = JNI_FALSE;
    
    WITH_FIELD_PLATFORM_STRING(env, file, ids_path, path) {
	
	WITH_WIN32_STRING(path) {
	    DWORD a = GetFileAttributes(wc);
	    if (a != 0xffffffff) {
		if (SetFileAttributes(wc, a | FILE_ATTRIBUTE_READONLY))
		    rv = JNI_TRUE;
	    }
	} END_WIN32_STRING;
    } END_PLATFORM_STRING(env, path);
    return rv;
}


/* -- Filesystem interface -- */

#ifndef _UNICODE
#include <direct.h>
#endif

JNIEXPORT jobject JNICALL
Java_java_io_Win32FileSystem_getDriveDirectory(JNIEnv *env, jclass ignored,
					       jint drive)
{
    char buf[_MAX_PATH];
    char *p = _getdcwd(drive, buf, sizeof(buf));
    if (p == NULL) return NULL;
    if (isalpha(*p) && (p[1] == ':')) p += 2;
    return JNU_NewStringPlatform(env, p);
}

JNIEXPORT jint JNICALL
Java_java_io_Win32FileSystem_listRoots0(JNIEnv *env, jclass ignored)
{
#ifdef WINCE
    return 0;
#else
    return GetLogicalDrives();
#endif
}
