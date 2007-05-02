/*
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

/*=========================================================================
 * SYSTEM:    Verifier 
 * SUBSYSTEM: System support functions 
 * FILE:      sys_support.c
 * OVERVIEW:  Routines for system support functions. The routines
 *            are for retrieving system class path and for certain 
 *            Windows specific file parsing functions.
 *
 *=======================================================================*/

/*=========================================================================
 * Include files
 *=======================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <oobj.h>
#include <jar.h>
#include <typedefs.h>
#include <sys_api.h>
#include <path.h>

/*=========================================================================
 * Globals and extern declarations
 *=======================================================================*/

static cpe_t **saved_classpath;
static cpe_t **saved_classpath_end;

extern zip_t * getZipEntry(char *zipFile, int len);

/*=========================================================================
 * FUNCTION:      sysGetClassPath 
 * OVERVIEW:      Returns the system class path using the getenv() system
 *                call for retrieving the CLASSPATH environment. 
 * INTERFACE:
 *   parameters:  none 
 *
 *   returns:     Pointer to cpe_t struct ptr (see path.h) 
 *=======================================================================*/
cpe_t **
sysGetClassPath(void)
{
    struct stat sbuf;
    int length;
    zip_t *zipEntry;
    bool_t includedDot = FALSE;

    if (saved_classpath == 0) {
        char *cps, *s;
        int ncpe = 1;
        cpe_t **cpp;
        if ((cps = getenv("CLASSPATH")) == 0) {
            cps = ".";
        }
        if ((cps = strdup(cps)) == 0) {
            return 0;
        }
        for (s = cps; *s != '\0'; s++) {
            if (*s == PATH_SEPARATOR) {
                ncpe++;
            }
        }
        /* We add 2 since we automatically append "." to the list, and we
         * need a NULL element at the end.
         * We add an extra 10 to allow pushing and popping of the classpath.
         * Generally, we'll need two, at most, but we can afford to be 
         * a little extravagant.
         */
        cpp = saved_classpath = sysMalloc((ncpe + 2 + 10) * sizeof(cpe_t *));
        if (cpp == 0) {
            return 0;
        }
        while (cps && *cps) {
            char *path = cps;
            cpe_t *cpe;
            if ((cps = strchr(cps, PATH_SEPARATOR)) != 0) {
                *cps++ = '\0';
            }
            if (*path == '\0') {
                path = ".";
            }
            cpe = sysMalloc(sizeof(cpe_t));
            if (cpe == 0) {
                return 0;
            }
            length = strlen(path);
            if (JAR_DEBUG && verbose) {
                jio_fprintf(stderr, "SysGetClassPath: Length : %d\n", length);    
                jio_fprintf(stderr, "SysGetClasspath: Path : %s\n", path);
            }
            if (stat(path, &sbuf) < 0) {
                /* we don't have to do anything */
            } else if (sbuf.st_mode & S_IFDIR) {
                /* this is a directory */
                cpe->type = CPE_DIR;
                cpe->u.dir = path;
                if (strcmp(path, ".") == 0) { 
                    includedDot = TRUE;
                }

                /* restore only for a valid directory */
                *cpp++ = cpe;
                
                if (JAR_DEBUG && verbose)
                    jio_fprintf(stderr, "SysGetClassPath: Found directory [%s]\n", path);
                
            } else if (isJARfile (path, length)) { 
                /* this looks like a JAR file */
                /* initialize the zip structure */
        
                if (JAR_DEBUG && verbose)
                    jio_fprintf(stderr, "SysGetClassPath: Found .JAR file [%s]\n", path);
                
                /* Create the zip entry for searching the JAR
                 * directories. If the zip entry is NULL, it 
                 * would indicate that we ran out of memory
                 * and would have exited already.
                 */
                zipEntry = getZipEntry(path, length); 
            
                /* search for the JAR directories */
                if (findJARDirectories(zipEntry, &sbuf)) {
                    /* this is a JAR file - initialize the cpe */
                    
                    if (JAR_DEBUG && verbose)
                        jio_fprintf(stderr, "SysGetClassPath: JAR directories OK in [%s]\n", zipEntry->name);
                    
                    zipEntry->type = 'j';
                    cpe->type = CPE_ZIP;
                    cpe->u.zip = zipEntry; 
                    /* restore entry only for a valid JAR */
                    *cpp++ = cpe;
                }
            }
        }
        if (!includedDot) { 
            cpe_t *cpe = sysMalloc(sizeof(cpe_t));
            cpe->type = CPE_DIR;
            cpe->u.dir = ".";
            *cpp++ = cpe;
        }
        *cpp = 0;
        saved_classpath_end = cpp;
    }
    
    return saved_classpath;
}

/* Puts this directory at the beginning of the class path */

void
pushDirectoryOntoClassPath(char* directory)
{ 
    cpe_t **ptr;

    cpe_t *cpe = sysMalloc(sizeof(cpe_t));
    cpe->type = CPE_DIR;
    cpe->u.dir = directory;

    sysGetClassPath();          /* just in case not done yet */

    /* Note that saved_classpath_end points to the NULL at the end. */
    saved_classpath_end++;
    for (ptr = saved_classpath_end; ptr > saved_classpath; ptr--) { 
        ptr[0] = ptr[-1];
    }
    ptr[0] = cpe;
}

/* Puts this jar file at the beginning of the class path */

void
pushJarFileOntoClassPath(zip_t *zipEntry)
{
    cpe_t **ptr;

    cpe_t *cpe = sysMalloc(sizeof(cpe_t));
    cpe->type = CPE_ZIP;
    cpe->u.zip = zipEntry; 

    sysGetClassPath();          /* just in case not done yet */

    /* Note that saved_classpath_end points to the NULL at the end. */
    saved_classpath_end++;
    for (ptr = saved_classpath_end; ptr > saved_classpath; ptr--) { 
        ptr[0] = ptr[-1];
    }
    ptr[0] = cpe;
}
    

/* Pop the first element off the class path */
void
popClassPath()
{ 
    cpe_t **ptr;

    sysFree(*saved_classpath);

    --saved_classpath_end;
    for (ptr = saved_classpath; ptr <= saved_classpath_end; ptr++) { 
        /* This copies all of the elements, including the NULL at the end */
        ptr[0] = ptr[1];
    }
}

/*=========================================================================
 * Win32 file parsing functions 
 *=======================================================================*/

#ifdef WIN32

#undef DEBUG_PATH        /* Define this to debug path code */

#define isfilesep(c) ((c) == '/' || (c) == '\\')
#define islb(c)      (IsDBCSLeadByte((BYTE)(c)))

/*=========================================================================
 * FUNCTION:      sysNativePath 
 * OVERVIEW:      Converts a path name to native format. On win32, this 
 *                involves forcing all separators to be '\\' rather than '/'
 *                (both are legal inputs, but Win95 sometimes rejects '/')
 *                and removing redundant separators. The input path is assumed
 *                to have been converted into the character encoding used by 
 *                the local system. Because this might be a double-byte 
 *                encoding, care is taken to treat double-byte lead characters
 *                correctly. 
 *               
 * INTERFACE:
 *   parameters:  char *path 
 *
 *   returns:     char * 
 *=======================================================================*/
char *
sysNativePath(char *path)
{
    char *src = path, *dst = path;
    char *colon = NULL;        /* If a drive specifier is found, this will
                   point to the colon following the drive
                   letter */

    /* Assumption: '/', '\\', ':', and drive letters are never lead bytes */
    sysAssert(!islb('/') && !islb('\\') && !islb(':'));

    /* Check for leading separators */
    while (isfilesep(*src)) src++;
    if (!islb(*src) && src[1] == ':') {
    /* Remove leading separators if followed by drive specifier.  This
       is necessary to support file URLs containing drive
       specifiers (e.g., "file://c:/path").  As a side effect,
       "/c:/path" can be used as an alternative to "c:/path". */
    *dst++ = *src++;
    colon = dst;
    *dst++ = ':'; src++;
    } else {
    src = path;
    if (isfilesep(src[0]) && isfilesep(src[1])) {
        /* UNC pathname: Retain first separator; leave src pointed at
           second separator so that further separators will be collapsed
           into the second separator.  The result will be a pathname
           beginning with "\\\\" followed (most likely) by a host name. */
        src = dst = path + 1;
        path[0] = '\\';    /* Force first separator to '\\' */
    }
    }

    /* Remove redundant separators from remainder of path, forcing all
       separators to be '\\' rather than '/' */
    while (*src != '\0') {
    if (isfilesep(*src)) {
        *dst++ = '\\'; src++;
        while (isfilesep(*src)) src++;
        if (*src == '\0') {    /* Check for trailing separator */
        if (colon == dst - 2) break;                      /* "z:\\" */
        if (dst == path + 1) break;                       /* "\\" */
        if (dst == path + 2 && isfilesep(path[0])) {
            /* "\\\\" is not collapsed to "\\" because "\\\\" marks the
               beginning of a UNC pathname.  Even though it is not, by
               itself, a valid UNC pathname, we leave it as is in order
               to be consistent with sysCanonicalPath() (below) as well
               as the win32 APIs, which treat this case as an invalid
               UNC pathname rather than as an alias for the root
               directory of the current drive. */
            break;
        }
        dst--;        /* Path does not denote a root directory, so
                   remove trailing separator */
        break;
        }
    } else {
        if (islb(*src)) {    /* Copy a double-byte character */
        *dst++ = *src++;
        if (*src) {
            *dst++ = *src++;
        }
        } else {        /* Copy a single-byte character */
        *dst++ = *src++;
        }
    }
    }

    *dst = '\0';
#ifdef DEBUG_PATH
    jio_fprintf(stderr, "sysNativePath: %s\n", path);
#endif /* DEBUG_PATH */
    return path;
}

/*=========================================================================
 * FUNCTION:      opendir
 * OVERVIEW:      open directory given dir pointer. 
 *                
 * INTERFACE:
 *   parameters:  const char *dirarg
 *
 *   returns:     Pointer to DIR structure 
 *=======================================================================*/
DIR *
opendir(const char *dirarg)
{
    DIR *dirp = (DIR *)sysMalloc(sizeof(DIR));
    unsigned long fattr;
    char alt_dirname[4] = { 0, 0, 0, 0 };
    char dirname_buf[1024];
    char *dirname = dirname_buf;

    if (dirp == 0) {
    errno = ENOMEM;
    return 0;
    }

    strcpy(dirname, dirarg);
    sysNativePath(dirname);

    /*
     * Win32 accepts "\" in its POSIX stat(), but refuses to treat it
     * as a directory in FindFirstFile().  We detect this case here and
     * prepend the current drive name.
     */
    if (dirname[1] == '\0' && dirname[0] == '\\') {
    alt_dirname[0] = _getdrive() + 'A' - 1;
    alt_dirname[1] = ':';
    alt_dirname[2] = '\\';
    alt_dirname[3] = '\0';
    dirname = alt_dirname;
    }

    dirp->path = (char *)sysMalloc(strlen(dirname) + 5);
    if (dirp->path == 0) {
    sysFree(dirp);
    errno = ENOMEM;
    return 0;
    }
    strcpy(dirp->path, dirname);

    fattr = GetFileAttributes(dirp->path);
    if (fattr == 0xffffffff) {
    sysFree(dirp->path);
    sysFree(dirp);
    errno = ENOENT;
    return 0;
    } else if ((fattr & FILE_ATTRIBUTE_DIRECTORY) == 0) {
    sysFree(dirp->path);
    sysFree(dirp);
    errno = ENOTDIR;
    return 0;
    }

    /* Append "*.*", or possibly "\\*.*", to path */
    if (dirp->path[1] == ':'
    && (dirp->path[2] == '\0'
        || (dirp->path[2] == '\\' && dirp->path[3] == '\0'))) {
    /* No '\\' needed for cases like "Z:" or "Z:\" */
    strcat(dirp->path, "*.*");
    } else {
    strcat(dirp->path, "\\*.*");
    }

    dirp->handle = FindFirstFile(dirp->path, &dirp->find_data);
    if (dirp->handle == INVALID_HANDLE_VALUE) {
        if (GetLastError() != ERROR_FILE_NOT_FOUND) {
        sysFree(dirp->path);
        sysFree(dirp);
        errno = EACCES;
        return 0;
    }
    }
    return dirp;
}

/*=========================================================================
 * FUNCTION:      readdir 
 * OVERVIEW:      read directory given pointer to DIR structure. 
 *                
 * INTERFACE:
 *   parameters:  DIR *dirp
 *
 *   returns:     Pointer to dirent structure. 
 *=======================================================================*/
struct dirent *
readdir(DIR *dirp)
{
    if (dirp->handle == INVALID_HANDLE_VALUE) {
    return 0;
    }

    strcpy(dirp->dirent.d_name, dirp->find_data.cFileName);

    if (!FindNextFile(dirp->handle, &dirp->find_data)) {
    if (GetLastError() == ERROR_INVALID_HANDLE) {
        errno = EBADF;
        return 0;
    }
    FindClose(dirp->handle);
    dirp->handle = INVALID_HANDLE_VALUE;
    }

    return &dirp->dirent;
}

/*=========================================================================
 * FUNCTION:      closedir
 * OVERVIEW:      close directory given pointer to DIR structure. 
 *                Returns non-zero status if the close fails.
 *                
 * INTERFACE:
 *   parameters:  DIR: *dirp
 *
 *   returns:     int: status 
 *=======================================================================*/
int
closedir(DIR *dirp)
{
    if (dirp->handle != INVALID_HANDLE_VALUE) {
    if (!FindClose(dirp->handle)) {
        errno = EBADF;
        return -1;
    }
    dirp->handle = INVALID_HANDLE_VALUE;
    }
    sysFree(dirp->path);
    sysFree(dirp);
    return 0;
}

#endif
