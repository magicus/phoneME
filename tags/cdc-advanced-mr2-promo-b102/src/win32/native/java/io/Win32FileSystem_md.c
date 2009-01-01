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
 */

#ifndef WINCE
#define CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif
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

#include "jni_statics.h"

#define ids_path	JNI_STATIC_MD(java_io_Win32FileSystem, ids_path)

#define WITH_UNICODE_PATH(env, object, id, var)                               \
    WITH_UNICODE_STRING(env,                                                  \
             ((object == NULL)                                                \
              ? NULL                                                          \
              : (*(env))->GetObjectField((env), (object), (id))),             \
             var)                                                             

#define END_UNICODE_PATH(env, var) END_UNICODE_STRING(env, var)

#define MAX_PATH_LENGTH 1024

#define SHORT_DIR_LIMIT 248

#ifdef _WIN32_WINNT
#include "javavm/include/winntUtil.h"
#endif

JNIEXPORT void JNICALL 
Java_java_io_Win32FileSystem_initIDs(JNIEnv *env, jclass cls)
{
    jclass fileClass = (*env)->FindClass(env, "java/io/File");
    if (!fileClass) return;
    ids_path = 
             (*env)->GetFieldID(env, fileClass, "path", "Ljava/lang/String;");
}

/* -- Path operations -- */

extern int wcanonicalize(const WCHAR *path, WCHAR *out, int len);
extern int canonicalize(char *path, const char *out, int len);

JNIEXPORT jstring JNICALL 
Java_java_io_Win32FileSystem_canonicalize0(JNIEnv *env, jobject this,
                                           jstring pathname)
{
    jstring rv = NULL;
    WCHAR canonicalPath[MAX_PATH_LENGTH];

    WITH_UNICODE_STRING(env, pathname, path) {
        if (wcanonicalize(path, canonicalPath, MAX_PATH_LENGTH) >= 0) {
            rv = (*env)->NewString(env, canonicalPath, wcslen(canonicalPath));
        }
    } END_UNICODE_STRING(env, path); 
    if (rv == NULL) {
     	JNU_ThrowIOExceptionWithLastError(env, "Bad pathname");
    }
    return rv;
}

/* -- Attribute accessors -- */

JNIEXPORT jint JNICALL
Java_java_io_Win32FileSystem_getBooleanAttributes(JNIEnv *env, jobject this,
                                                  jobject file) 
{

  jint rv = 0;
  DWORD a;
  jint pathlen = 0;
  WCHAR *pathbuf = NULL;
  jstring file_path = NULL;
#define PAGEFILE_NAMELEN 12 
 
  WITH_UNICODE_PATH(env, file, ids_path, path) {
    file_path = (*env)->GetObjectField((env), file, ids_path);
    pathlen = (*env)->GetStringLength(env, file_path);
    if (pathlen != 0) {
      if (pathlen > MAX_PATH - 1) {
        // copy \\?\ to the front of path 
        pathbuf = (WCHAR*)malloc((pathlen + 10) * 2);
        if (pathbuf != 0) {                    
          pathbuf[0] = L'\0';
          wcscpy(pathbuf, L"\\\\?\\\0");
          wcscat(pathbuf, path);
          // We need to null terminate the unicode string here.
          pathbuf[pathlen + 4] = L'\0';
        }
      } else {
        pathbuf = (WCHAR*)malloc((pathlen + 6) * 2);
        if (pathbuf != 0) {
          pathbuf[0] = L'\0';
          wcscpy(pathbuf, path);
          // We need to null terminate the unicode string here.
          pathbuf[pathlen] = L'\0';
        }
      }
    }
  } END_UNICODE_PATH(env, path);
  
  a = GetFileAttributesW(pathbuf);
  if (a != ((DWORD)-1)) {
    rv = (java_io_FileSystem_BA_EXISTS
          | ((a & FILE_ATTRIBUTE_DIRECTORY)
             ? java_io_FileSystem_BA_DIRECTORY
             : java_io_FileSystem_BA_REGULAR)
          | ((a & FILE_ATTRIBUTE_HIDDEN)
            ? java_io_FileSystem_BA_HIDDEN : 0));
  } else { // pagefile.sys is a special case 
    if (GetLastError() == ERROR_SHARING_VIOLATION) 
      if ((pathlen  >= PAGEFILE_NAMELEN && 
           _wcsicmp(pathbuf + pathlen - PAGEFILE_NAMELEN, 
                    L"pagefile.sys") == 0))
        rv = java_io_FileSystem_BA_EXISTS | 
          java_io_FileSystem_BA_REGULAR;                 
  }
  free(pathbuf);  
  return rv;
}



JNIEXPORT jboolean 
JNICALL Java_java_io_Win32FileSystem_checkAccess(JNIEnv *env, jobject this, 
                                                 jobject file, jboolean write) 
{
    jboolean rv = JNI_FALSE;
    jint pathlen;
    WCHAR *pathbuf = NULL;
    jstring file_path = NULL;
  
    WITH_UNICODE_PATH(env, file, ids_path, path) {
      file_path = (*env)->GetObjectField((env), file, ids_path);
      pathlen = (*env)->GetStringLength(env, file_path);
      if (pathlen != 0) {
        pathbuf = (WCHAR*)malloc((pathlen + 6) * 2);
        if (pathbuf != 0) {
          if (pathlen > MAX_PATH - 1) {
            // copy \\?\ to the front of path 
            pathbuf = (WCHAR*)malloc((pathlen + 10) * 2);
            if (pathbuf != 0) {                    
              pathbuf[0] = L'\0';
              wcscpy(pathbuf, L"\\\\?\\\0");
              wcscat(pathbuf, path);
              // We need to null terminate the unicode string here.
              pathbuf[pathlen + 4] = L'\0';
            }
          } else {
            pathbuf[0] = L'\0';
            wcscpy(pathbuf, path);
            // We need to null terminate the unicode string here.
            pathbuf[pathlen] = L'\0';
          }
        }
        if (_waccess(pathbuf, (write ? 2 : 4)) == 0) {
            rv = JNI_TRUE;
        }
      }
    } END_UNICODE_PATH(env, path);
    free(pathbuf);  
    return rv;
}

JNIEXPORT jlong JNICALL
Java_java_io_Win32FileSystem_getLastModifiedTime(JNIEnv *env, jobject this,
                                                 jobject file) 
{
    jlong rv = 0;
    WITH_UNICODE_PATH(env, file, ids_path, path) {
        WIN32_FILE_ATTRIBUTE_DATA fileAttrData;

        if (GetFileAttributesExW(path, GetFileExInfoStandard, &fileAttrData)) {
            ULARGE_INTEGER modTime;
            modTime.LowPart = fileAttrData.ftLastWriteTime.dwLowDateTime;
            modTime.HighPart = fileAttrData.ftLastWriteTime.dwHighDateTime;
            rv = modTime.QuadPart / 10000;
            rv -= 11644473600000;
        }
    } END_UNICODE_PATH(env, path);
    return rv;
}

JNIEXPORT jlong JNICALL
Java_java_io_Win32FileSystem_getLength(JNIEnv *env, jobject this, jobject file)
{
    jlong rv = 0;

    WITH_UNICODE_PATH(env, file, ids_path, path) {
        WIN32_FILE_ATTRIBUTE_DATA fileAttributes;

        if (GetFileAttributesExW(path, GetFileExInfoStandard, (LPVOID)&fileAttributes)) {
            rv = ((jlong)fileAttributes.nFileSizeHigh << 32) + fileAttributes.nFileSizeLow;
        }
    } END_UNICODE_PATH(env, path);
    return rv;
}

/* -- File operations -- */

JNIEXPORT jboolean JNICALL
Java_java_io_Win32FileSystem_createFileExclusively(JNIEnv *env, jclass cls,
                                                   jstring path)
{
    HANDLE h = NULL;
    WCHAR *pathbuf = NULL;
    // pathToNTPath(env, path, JNI_FALSE);
    WITH_UNICODE_STRING(env, path, ps) {
      int pathlen = wcslen(ps);
        if (pathlen > MAX_PATH - 1) {
            /* copy \\?\ to the front of path */
            pathbuf = (WCHAR*)malloc((pathlen + 10) * 2);
            if (pathbuf != 0) {
                pathbuf[0] = L'\0';
                wcscpy(pathbuf, L"\\\\?\\\0");
                wcscat(pathbuf, ps);
            }
        } else {
            pathbuf = (WCHAR*)malloc((pathlen + 6) * 2);
            if (pathbuf != 0) {
                pathbuf[0] = L'\0';
                wcscpy(pathbuf, ps);
            }
        }
    } END_UNICODE_STRING(env, ps);

    if (pathbuf == NULL) {
        return JNI_FALSE;
    }

    h = CreateFileW(
        pathbuf,                             /* Wide char path name */
        GENERIC_READ | GENERIC_WRITE,  /* Read and write permission */
        FILE_SHARE_READ | FILE_SHARE_WRITE,   /* File sharing flags */
        NULL,                                /* Security attributes */
        CREATE_NEW,                         /* creation disposition */
        FILE_ATTRIBUTE_NORMAL,              /* flags and attributes */
        NULL);

    free(pathbuf);

    if (h == INVALID_HANDLE_VALUE) {
        int error = GetLastError();
        if ((error == ERROR_FILE_EXISTS)||(error == ERROR_ALREADY_EXISTS)) {
            return JNI_FALSE;
        }
        JNU_ThrowIOExceptionWithLastError(env, "Could not open file");
        return JNI_FALSE;
    }
    CloseHandle(h);
    return JNI_TRUE;
}

static int 
removeFileOrDirectory(const jchar *path) 
{ 
    /* Returns 0 on success */ 
    DWORD a;

    SetFileAttributesW(path, 0);
    a = GetFileAttributesW(path);
    if (a == ((DWORD)-1)) {
        return 1;
    } else if (a & FILE_ATTRIBUTE_DIRECTORY) {
        return !RemoveDirectoryW(path);
    } else {
        return !DeleteFileW(path);
    }
}

JNIEXPORT jboolean JNICALL
Java_java_io_Win32FileSystem_delete0(JNIEnv *env, jobject this, jobject file)
{
    jboolean rv = JNI_FALSE;
    WCHAR *pathbuf = NULL;
    DWORD a;

    WITH_UNICODE_PATH(env, file, ids_path, ps) {
        int pathlen = wcslen(ps);
        if (pathlen > MAX_PATH - 1) {
            /* copy \\?\ to the front of path */
            pathbuf = (WCHAR*)malloc((pathlen + 10) * 2);
            if (pathbuf != 0) {
                pathbuf[0] = L'\0';
                wcscpy(pathbuf, L"\\\\?\\\0");
                wcscat(pathbuf, ps);
            }
        } else {
            pathbuf = (WCHAR*)malloc((pathlen + 6) * 2);
            if (pathbuf != 0) {
                pathbuf[0] = L'\0';
                wcscpy(pathbuf, ps);
            }
        }
    } END_UNICODE_PATH(env, ps);

    if (pathbuf == 0) {
        JNU_ThrowOutOfMemoryError(env, 0);
        return JNI_FALSE;
    }

    if (removeFileOrDirectory(pathbuf) == 0) {
        rv = JNI_TRUE;
    }

    free(pathbuf);

    return rv;
}

typedef int (*WDELETEPROC)(const jchar *path);

static struct wdlEntry {
    struct wdlEntry *next;
    WDELETEPROC wdeleteProc;
    jchar name[JVM_MAXPATHLEN + 1];
} *wdeletionList = NULL;

static void
wdeleteOnExitHook(void)		/* Called by the VM on exit */
{
    struct wdlEntry *e, *next;
    for (e = wdeletionList; e; e = next) {
	    next = e->next;
	    e->wdeleteProc(e->name);
	    free(e);
    }
}

void
wdeleteOnExit(JNIEnv *env, const jchar *path, WDELETEPROC dp)
{
    struct wdlEntry *dl = wdeletionList;
    struct wdlEntry *e = (struct wdlEntry *)malloc(sizeof(struct wdlEntry));

    if (e == NULL) {
        JNU_ThrowOutOfMemoryError(env, 0);
        return;
    }

    wcscpy(e->name, path);
    e->wdeleteProc = dp;

    if (dl == NULL) {
        JVM_OnExit(wdeleteOnExitHook);
    }

    e->next = wdeletionList;
    wdeletionList = e;
}

JNIEXPORT jboolean JNICALL
Java_java_io_Win32FileSystem_deleteOnExit(JNIEnv *env, jobject this, 
                                          jobject file) 
{
    WITH_UNICODE_PATH(env, file, ids_path, path) {
        wdeleteOnExit(env, path, removeFileOrDirectory);
    } END_UNICODE_PATH(env, path);
    return JNI_TRUE;
}

JNIEXPORT jobjectArray JNICALL 
Java_java_io_Win32FileSystem_list(JNIEnv *env, jobject this, jobject file)
{
    WCHAR *search_path = NULL;
    HANDLE handle;
    WIN32_FIND_DATAW find_data;
    int len, maxlen;
    jobjectArray rv, old;
    DWORD fattr;
    jstring name;

    WITH_UNICODE_PATH(env, file, ids_path, path) {
        search_path = (WCHAR*)malloc(2*wcslen(path) + 6);
        if (search_path == 0) {
            errno = ENOMEM;
            return NULL;
        }
        wcscpy(search_path, path);
    } END_UNICODE_PATH(env, path);
    fattr = GetFileAttributesW(search_path);
    if (fattr == ((DWORD)-1)) {
        free(search_path);
        return NULL;
    } else if (fattr & FILE_ATTRIBUTE_DIRECTORY == 0) {
        free(search_path);
        return NULL;
    }
    
    /* Append "*", or possibly "\\*", to path */
    if ((search_path[0] == L'\\' && search_path[1] == L'\0') ||
        (search_path[1] == L':'
        && (search_path[2] == L'\0'
        || (search_path[2] == L'\\' && search_path[3] == L'\0')))) {
        /* No '\\' needed for cases like "\" or "Z:" or "Z:\" */
        wcscat(search_path, L"*");
    } else {
        wcscat(search_path, L"\\*");
    }

    /* Open handle to the first file */
    handle = FindFirstFileW(search_path, &find_data);
    free(search_path);
    if (handle == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        if (err != ERROR_FILE_NOT_FOUND && err != ERROR_NO_MORE_FILES) {
            // error
            return NULL;
        } else {
            // No files found - return an empty array
            rv = (*env)->NewObjectArray(env, 0, JNU_ClassString(env), NULL);
            return rv;
        }
    }

    /* Allocate an initial String array */
    len = 0;
    maxlen = 16;
    rv = (*env)->NewObjectArray(env, maxlen, JNU_ClassString(env), NULL);
    if (rv == NULL) // Couldn't allocate an array
        return NULL;
    /* Scan the directory */
    do {
        if (!wcscmp(find_data.cFileName, L".") 
                                || !wcscmp(find_data.cFileName, L".."))
           continue;
        name = (*env)->NewString(env, find_data.cFileName, 
                                 wcslen(find_data.cFileName));
        if (name == NULL)
            return NULL; // error;
        if (len == maxlen) {
            old = rv;
            rv = (*env)->NewObjectArray(env, maxlen <<= 1,
                                            JNU_ClassString(env), NULL);
            if ( rv == NULL 
                         || JNU_CopyObjectArray(env, rv, old, len) < 0)
                return NULL; // error
            (*env)->DeleteLocalRef(env, old);
        }
        (*env)->SetObjectArrayElement(env, rv, len++, name);
        (*env)->DeleteLocalRef(env, name);
        
    } while (FindNextFileW(handle, &find_data));

    if (GetLastError() != ERROR_NO_MORE_FILES)
        return NULL; // error
    FindClose(handle);

    /* Copy the final results into an appropriately-sized array */    
    old = rv;
    rv = (*env)->NewObjectArray(env, len, JNU_ClassString(env), NULL);
    if (rv == NULL)
        return NULL; /* error */
    if (JNU_CopyObjectArray(env, rv, old, len) < 0)
        return NULL; /* error */    
    return rv;
}


JNIEXPORT jboolean JNICALL
Java_java_io_Win32FileSystem_createDirectory(JNIEnv *env, jobject this, 
                                             jobject file) 
{
    WCHAR *pathbuf = NULL;
    BOOL h = FALSE;

    WITH_UNICODE_PATH(env, file, ids_path, ps) {
        int pathlen = wcslen(ps);
        if (pathlen > SHORT_DIR_LIMIT - 1) {
            /* copy \\?\ to the front of path */
            pathbuf = (WCHAR*)malloc((pathlen + 10) * 2);
            if (pathbuf != 0) {
                pathbuf[0] = L'\0';
                wcscpy(pathbuf, L"\\\\?\\\0");
                wcscat(pathbuf, ps);
            }
        } else {
            pathbuf = (WCHAR*)malloc((pathlen + 6) * 2);
            if (pathbuf != 0) {
                pathbuf[0] = L'\0';
                wcscpy(pathbuf, ps);
            }
        }
    } END_UNICODE_PATH(env, ps);

    if (pathbuf == 0) {
        JNU_ThrowOutOfMemoryError(env, 0);
        return JNI_FALSE;
    }

    h = CreateDirectoryW(pathbuf, NULL);

    free(pathbuf);
    
    if (h == 0) {
        return JNI_FALSE;
    }
        
    return JNI_TRUE;
}
									 
JNIEXPORT jboolean JNICALL
Java_java_io_Win32FileSystem_rename0(JNIEnv *env, jobject this, jobject from, 
                                    jobject to) 
{

    jboolean rv = JNI_FALSE;

    WITH_UNICODE_PATH(env, from, ids_path, fromPath) {
        WITH_UNICODE_PATH(env, to, ids_path, toPath) {
            if (_wrename(fromPath, toPath) == 0) {
                rv = JNI_TRUE;
            }
        } END_UNICODE_PATH(env, toPath);
    } END_UNICODE_PATH(env, fromPath);
    return rv;
}


JNIEXPORT jboolean JNICALL
Java_java_io_Win32FileSystem_setLastModifiedTime(JNIEnv *env, jobject this,
                                                 jobject file, jlong time) 
{
    jboolean rv = JNI_FALSE;

    WITH_UNICODE_PATH(env, file, ids_path, path) {
        HANDLE h;
        h = CreateFileW(path, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                   FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS, 0);
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
    } END_UNICODE_PATH(env, path);

    return rv;
}


JNIEXPORT jboolean JNICALL
Java_java_io_Win32FileSystem_setReadOnly(JNIEnv *env, jobject this, 
                                         jobject file) 
{
    jboolean rv = JNI_FALSE;

    WITH_UNICODE_PATH(env, file, ids_path, path) {
        DWORD a;
        a = GetFileAttributesW(path);
        if (a != ((DWORD)-1)) {
            if (SetFileAttributesW(path, a | FILE_ATTRIBUTE_READONLY))
            rv = JNI_TRUE;
        }
    } END_UNICODE_PATH(env, path);
    return rv;
}
