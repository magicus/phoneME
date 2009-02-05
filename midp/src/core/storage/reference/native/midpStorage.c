/*
 *
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
 */

/**
 * @file
 *
 * Manage storage for internal API's.
 */
/*
 * Functions in this file make file system calls that are
 * the same (similar) across platforms with Posix
 * library support.  Calls which are system dependent and may vary
 * across platforms are encapsulated by functions defined in the file
 * storagePosix_md.h.
 */

#include <string.h>

#ifndef UNDER_CE
#include <errno.h>
#else
#include <windows.h>
#endif

#include <midpUtilKni.h>
#include <midpMalloc.h>
#include <midpString.h>
#include <midpStorage.h>

#include <midp_logging.h>
#include <midpResourceLimit.h>
#include <pcsl_file.h>
#include <pcsl_string.h>
#include <pcsl_esc.h>
#include <pcsl_string_status.h>
#include <pcsl_memory.h>

/* local prototypes */
static int initializeConfigRoot (char*);
static char* getLastError(char*);
static char* storage_get_last_file_error(char*, const pcsl_string*);

static const char* const OUT_OF_MEM_ERROR =
    "out of memory, cannot perform file operation";
static const char* const NOT_EXIST_RENAME_ERROR =
    "The file to be renamed does not exist.";
static const char* const FILE_LIMIT_ERROR =
    "Resource limit exceeded for file handles";
static const char* const STRING_CORRUPT_ERROR =
    "string data corrupt or invalid, cannot perform i/o operation";

/*
 * Name of the external storage directory.
 */
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(MEMORY_CARD_DIR)
    {'m', 'e', 'm', 'o', 'r', 'y', '_', 'c', 'a', 'r', 'd', '\0'}
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(MEMORY_CARD_DIR);

/*
 * IMPL_NOTE: currently the following two strings are unused (APP_DIR
 *            and CONFIG_DIR are defined in commandLineUtil_md.c),
 *            but this is not quite correct.
 */
/*
 * Name of the storage directory.
 */
/*
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(APP_DIR)
    {'a', 'p', 'p', 'd', 'b', '\0'}
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(APP_DIR);

PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(CONFIG_SUBDIR)
    {'l', 'i', 'b', '\0'}
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(CONFIG_SUBDIR);
*/

#if REPORT_LEVEL <= LOG_INFORMATION
#define DEBUGP2F(x, y) { \
    const char* pszTemp = pcsl_string_get_utf8_data(y); \
        if (pszTemp != NULL) { \
            REPORT_INFO1(LC_CORE, x, pszTemp); \
            pcsl_string_release_utf8_data(pszTemp,y); \
        } \
    }

#else
#define DEBUGP2F(x, y)
#endif


/*
 * Porting note: <tt>DEFAULT_TOTAL_SPACE</tt> defines the
 * default maximum space allotted for storage of MIDlet suites and their
 * associated record stores on a platform.  This storage space may
 * be set to a value other than <tt>DEFAULT_STORAGE_SPACE</tt> using
 * the <tt>storageSetTotalSpace</tt> method or providing the configuration
 * value system.jam_space (see midpInit.c).
 */
#define DEFAULT_TOTAL_SPACE (4 * 1024 * 1024) /* 4 Meg. */

/* Local variables */
static long totalSpace = DEFAULT_TOTAL_SPACE;

static pcsl_string sRoot[MAX_STORAGE_NUM];
static pcsl_string configRoot[MAX_STORAGE_NUM];
static int storageInitDone = 0;

#define MAX_ERROR_LEN 159
static char errorBuffer[MAX_ERROR_LEN + 1] = {0};

/*
 * Set the system directory for storage
 *
 * @param config_home file system path to where MIDP is installed
 * @param midp_home file system path to where MIDP should store its
 * data like suites and such
 *
 * @return 0 for success, non-zero for out of memory
 */
int
storageInitialize(char *config_home, char *app_dir) {
    jchar fsep = storageGetFileSeparator();

    if (storageInitDone) {
        /* Already initialized */
        return 0;
    }

    if (PCSL_STRING_OK != pcsl_string_initialize()) {
        REPORT_ERROR(LC_CORE, "Error: cannot initialize string library.\n");
        return -1;
    }

    pcsl_esc_init();

    if(pcsl_file_init() < 0)  {
        REPORT_ERROR(LC_CORE, "Error: out of memory.\n");
        return -1;
    }

    /* set up a path to the internal storage */
    if (PCSL_STRING_OK != pcsl_string_from_chars(app_dir, &sRoot[0])) {
        REPORT_ERROR(LC_CORE, "Error: out of memory.\n");
        storageFinalize();
        return -1;
    }

    if (PCSL_STRING_OK != pcsl_string_append_char(&sRoot[0], fsep)) {
        REPORT_ERROR(LC_CORE, "Error: out of memory.\n");
        storageFinalize();
        return -1;
    }

    if (MAX_STORAGE_NUM > 1) {
        int i;
        char* midp_home = NULL;
        char* sep_pos = NULL;

        /*
         * this is sample implementation and should
         * be rewritten for the real platforms
         */

        /* IMPL_NOTE: work around - restore midp home from appdb directory path */

        midp_home = (char*)pcsl_mem_malloc(strlen(app_dir) + 1);
        if (NULL == midp_home) {
            REPORT_ERROR(LC_CORE, "Error: out of memory.\n");
            storageFinalize();
            return -1;
        }
        sep_pos = strrchr(app_dir, fsep);
        if (NULL == sep_pos) {
            REPORT_ERROR(LC_CORE, "Error: bad application dir.\n");
            pcsl_mem_free(midp_home);
            storageFinalize();
            return -1;
        }

        i = (int)(sep_pos - app_dir) + 1;
        strncpy(midp_home, app_dir, i);
        midp_home[i] = 0;

        if (PCSL_STRING_OK != pcsl_string_from_chars(midp_home, &sRoot[1])) {
            REPORT_ERROR(LC_CORE, "Error: out of memory.\n");
            pcsl_mem_free(midp_home);
            storageFinalize();
            return -1;
        }
        pcsl_mem_free(midp_home);

        pcsl_string_predict_size(&sRoot[1], pcsl_string_length(&sRoot[1]) + 2
                                        + PCSL_STRING_LITERAL_LENGTH(MEMORY_CARD_DIR));
        if (PCSL_STRING_OK != pcsl_string_append_char(&sRoot[1], fsep)
         || PCSL_STRING_OK != pcsl_string_append(&sRoot[1], &MEMORY_CARD_DIR)
         || PCSL_STRING_OK != pcsl_string_append_char(&sRoot[1], fsep)) {
            REPORT_ERROR(LC_CORE, "Error: out of memory.\n");
            storageFinalize();
            return -1;
        }

        /*
         * If more than one external storage is needed, write extra initialization here.
         * By default we just redirect all to internal storage.
         */

        for (i = 2; i < MAX_STORAGE_NUM; i++) {
            if (PCSL_STRING_OK != pcsl_string_dup(&sRoot[0], &sRoot[i])) {
                storageFinalize();
                return -1;
            }
         }
    }

    if (0 != initializeConfigRoot(config_home)) {
        storageFinalize();
        return -1;
    }

    storageInitDone = 1;
    return 0;
}

static int
initializeConfigRoot(char* config_home) {
    jchar fileSep = storageGetFileSeparator();
    int i;

    if (PCSL_STRING_OK != pcsl_string_from_chars(config_home, &configRoot[0])) {
        return -1;
    }
    if (PCSL_STRING_OK != pcsl_string_append_char(&configRoot[0], fileSep)) {
        pcsl_string_free(&configRoot[0]);
        return -1;
    }

    for (i = 1; i < MAX_STORAGE_NUM; i++) {
        if (PCSL_STRING_OK != pcsl_string_dup(&configRoot[0], &configRoot[i])) {
            return -1;
        }
    }

    return 0;
}

/*
 * Set the MIDlet suite storage allowable space different than the default
 *
 * This function cannot be part of initialStorage since the space parameter
 * is normally obtained from the config system which cannot be used until
 * initializeStorage is called.
 *
 * @param space total file space allocated to MIDP
 */
void
storageSetTotalSpace(long space) {
    totalSpace = space;
}

/*
 * Finalize the storage subsystem.
 */
void
storageFinalize() {
    int i;
    for (i = 0; i < MAX_STORAGE_NUM; i++) {
        pcsl_string_free(&sRoot[i]);
        pcsl_string_free(&configRoot[i]);
    }
    storageInitDone = 0;
    pcsl_file_finalize();
}

/*
 * Returns root string that all files should begin with, including
 * a trailing file separator if needed. By including the any trailing file
 * separators the Java API does not need to know about file separators
 * or subdirectories.
 *
 * Since the lifetime of the returned object is from storageInitialize
 * until storageFinalize, you do not have to free the returned object.
 *
 * @param storageId ID of the storage the root of which must be returned
 *
 * @return prefix used for all file names in the given storage.
 *         It may be empty, but not PCSL_STRING_NULL.
 */
const pcsl_string*
storage_get_root(StorageIdType storageId) {
    /*
     * Change MAX_STORAGE_NUM value if you want to have more than one
     * external storage. This will also require modification of the functions
     * initializing and returning sRoot[<index>] and configRoot[<index>], namely
     * storageInitialize(), storage_get_root() and storage_get_config_root().
     */
    pcsl_string* pRes;
    if (storageId == INTERNAL_STORAGE_ID) {
        pRes = &sRoot[0];
    } else if ((storageId >= 0)
            && (storageId < MAX_STORAGE_NUM)) {
        pRes = &sRoot[storageId];
    } else {
        /*
         * IMPL_NOTE: if invalid storage id is specified
         * we return internal storage
         */
        pRes = &sRoot[0];
    }

    return pRes;
}

/*
 * Returns the root string that configuration files start with, including a
 * trailing file separator when necessary. If the string includes a trailing
 * file separator, callers do not need access to file separators or
 * subdirectories.
 *
 * Since the lifetime of the returned object is from initializeConfigRoot
 * until storageFinalize, you do not have to free the returned object.
 *
 * @param storageId ID of the storage the config root of which must be returned
 *
 * @return prefix used for all configuration file names. It may be empty,
 *         but not PCSL_STRING_NULL
 */
const pcsl_string*
storage_get_config_root(StorageIdType storageId) {
    pcsl_string* pRes;
    if (storageId == INTERNAL_STORAGE_ID) {
        pRes = &configRoot[0];
    } else if ((storageId >= 0)
            && (storageId < MAX_STORAGE_NUM)) {
        pRes = &configRoot[storageId];
    } else {
        /*
         * IMPL_NOTE: if invalid storage id is specified
         * we return internal storage
         */
        pRes = &configRoot[0];
    }

    return pRes;
}


/*
 * Free the error string returned from a storage function.
 * Does nothing if a NULL is passed in.
 * This allows for systems that provide error messages that are allocated
 * dynamically.
 */
void
storageFreeError(char* pszError) {
    /*
     * This is a place holder, so that an implementation, like a debuggable
     * one can create dynamic error messages.
     */
    if (NULL == pszError || OUT_OF_MEM_ERROR == pszError ||
        NOT_EXIST_RENAME_ERROR == pszError) {
        return;
    }
}

/*
 * Return a non-zero integer if a file with the given name exists else return
 * zero.
 */
int
storage_file_exists(const pcsl_string* filename_str) {
    /* Mapping 0 and -1 (error) to 0 (file not exists). */
    return pcsl_file_exist(filename_str) > 0;
}

/*
 * Returns the file separator as a string.
 *
 * This function only called by the main native code to
 * to manage files outside of the MIDP implementation simulated storage.
 */
jchar
storageGetFileSeparator() {
    return (jchar)pcsl_file_getfileseparator();
}

/*
 * The function is provided for development platforms so we can manage
 * local files outside of the MIDP implementation simulated storage.
 */
jchar
storageGetPathSeparator() {
    return (jchar)pcsl_file_getpathseparator();
}

/*
 * Return a 32 bit handle to an open a file in storage in different modes.
 *
 * See "I/O Modes" and "Filenames" above for move information.
 *
 * If not successful *ppszError will set to point to an error string,
 * on success it will be set to NULL.
 */
int
storage_open(char** ppszError, const pcsl_string* filename_str, int ioMode) {
    int flags = 0;
    int openStatus;
    void *handle;

    *ppszError = NULL;

    if (OPEN_READ == ioMode) {
        DEBUGP2F("opening for read only %s\n", filename_str);
        flags |= PCSL_FILE_O_RDONLY;
    } else {
        if (!storage_file_exists(filename_str)) {
            flags |= PCSL_FILE_O_CREAT;
        } else if (OPEN_READ_WRITE_TRUNCATE == ioMode) {
            flags |= PCSL_FILE_O_TRUNC;
        }

        if (OPEN_WRITE == ioMode) {
            DEBUGP2F("opening write only %s\n", filename_str);
            flags |= PCSL_FILE_O_WRONLY;
        } else {
            DEBUGP2F("opening read write %s\n", filename_str);
            flags |= PCSL_FILE_O_RDWR;
        }
    }

    /**
     * Verify that the resource is available well within limit as per
     * the policy in ResourceLimiter
     */
    if (midpCheckResourceLimit(RSC_TYPE_FILE, 1) == 0) {
        REPORT_INFO(LC_CORE, "Resource limit exceeded for file handles");
        *ppszError = (char *)FILE_LIMIT_ERROR;
        return -1;
    }

    openStatus = pcsl_file_open(filename_str, flags, &handle);

    REPORT_INFO1(LC_CORE, "storage_open allocated file_desc %d\n", (int)handle);

    if (-1 == openStatus) {
        *ppszError = storage_get_last_file_error("storage_open()", filename_str);
        return -1;
    }

    /* Update the resource count  */
    if (midpIncResourceCount(RSC_TYPE_FILE, 1) == 0) {
        REPORT_INFO(LC_CORE, "FILE : resource limit update error");
    }

#if REPORT_LEVEL <= LOG_INFORMATION
    DEBUGP2F("created %s\n", filename_str);
#endif

    return (int)handle;
}

/*
 * Close a opened by storage_open. Does no block.
 *
 * If not successful *ppszError will set to point to an error string,
 * on success it will be set to NULL.
 */
void
storageClose(char** ppszError, int handle) {
    int status;

    *ppszError = NULL;
    status = pcsl_file_close((void *)handle);

    REPORT_INFO2(LC_CORE, "storageClose on file_desc %d returns %d\n",
          handle, status);

    if (status < 0) {
        *ppszError = getLastError("storageClose()");
    }

    /* File is successfully closed, decrement the count */
    if (midpDecResourceCount(RSC_TYPE_FILE, 1) == 0) {
        REPORT_INFO(LC_CORE, "FILE: resource"
                             " limit update error");
    }
}

/*
 * Read from an open file in storage, returning the number of bytes read or
 * -1 for the end of the file. May read less than the length of the buffer.
 *
 * If not successful *ppszError will set to point to an error string,
 * on success it will be set to NULL.
 */
long
storageRead(char** ppszError, int handle, char* buffer, long length) {
    long bytesRead;

    *ppszError = NULL;
    if (0 == length) {
        return 0;
    }

    bytesRead = pcsl_file_read((void *)handle, (unsigned char*)buffer, length);

    REPORT_INFO2(LC_CORE, "storageRead on fd %d res = %ld\n",
          handle, bytesRead);

    if (-1 == bytesRead) {
        *ppszError = getLastError("storageRead()");
    } else if (0 == bytesRead) {
        /* end of file in java is -1 */
        bytesRead = -1;
    }

    return bytesRead;
}

/*
 * Write to an open file in storage. Will write all of the bytes in the
 * buffer or pass back an error.
 *
 * If not successful *ppszError will set to point to an error string,
 * on success it will be set to NULL.
 */
void
storageWrite(char** ppszError, int handle, char* buffer, long length) {
    long bytesWritten;

    *ppszError = NULL;
    bytesWritten = pcsl_file_write((void *)handle, (unsigned char*)buffer, length);

    REPORT_INFO2(LC_CORE, "storageWrite on fd %d res = %ld\n",
          handle, bytesWritten);

    if (-1 == bytesWritten) {
        *ppszError = getLastError("storageWrite()");
        return;
    }

    if (bytesWritten != length) {
        *ppszError = "storage full";
        return;
    }
}

/*
 * Commit or flush pending writes
 *
 * If not successful *ppszError will set to point to an error string,
 * on success it will be set to NULL.
 */
void
storageCommitWrite(char** ppszError, int handle) {
    int status;

    REPORT_INFO1(LC_CORE, "trying to flush pending writes on file handle %d\n", handle);

    status = pcsl_file_commitwrite((void *)handle);

    if (status < 0) {
        *ppszError = getLastError("storageCommit()");
        return;
    }

    *ppszError = NULL;
}

/*
 * Change the read/write position of an open file in storage.
 * The position is a number of bytes from beginning of the file.
 *
 * If not successful *ppszError will set to point to an error string,
 * on success it will be set to NULL.
 */
void
storagePosition(char** ppszError, int handle, long absolutePosition) {
    long newPosition;

    newPosition = pcsl_file_seek((void *)handle, absolutePosition,
                                  PCSL_FILE_SEEK_SET);

    REPORT_INFO2(LC_CORE, "storagePostion on fd %d res = %d\n",
          handle, newPosition);

    if (-1 == newPosition) {
        *ppszError = getLastError("storagePosition()");
        return;
    }

    *ppszError = NULL;
}

/*
 * Change the read/write position of an open file in storage.
 * The position is a number of bytes from current position.
 *
 * If not successful *ppszError will set to point to an error string,
 * on success it will be set to NULL.
 *
 * @return new absolute read/write position
 */
long
storageRelativePosition(char** ppszError, int handle, long offset) {
    long newPosition;

    newPosition = pcsl_file_seek((void *)handle, offset, PCSL_FILE_SEEK_CUR);

    REPORT_INFO2(LC_CORE, "storageRelativePostion on fd %d res = %d\n",
         handle, newPosition);

    if (-1 == newPosition) {
        *ppszError = getLastError("storageRelativePosition()");
        return -1;
    }

    *ppszError = NULL;

    return newPosition;
}

/*
 * Return the size of an open file in storage.
 *
 * If not successful *ppszError will set to point to an error string,
 * on success it will be set to NULL.
 *
 * @return size of file in bytes if successful, -1 otherwise
 */
long
storageSizeOf(char** ppszError, int handle) {
    long size;

    size = pcsl_file_sizeofopenfile((void *)handle);
    if (size < 0) {
        *ppszError = getLastError("storageSizeOf()");
    } else {
        *ppszError = NULL;
    }
    return size;
}

/*
 * Return the size of file with the given name in storage.
 *
 * If not successful *ppszError will set to point to an error string,
 * on success it will be set to NULL.
 *
 * @return size of file in bytes if successful, -1 otherwise
 */
long
storage_size_of_file_by_name(char** ppszError, const pcsl_string* pFileName) {
    long size;

    size = pcsl_file_sizeof(pFileName);
    if (size < 0) {
        *ppszError = getLastError("storage_size_of_file_by_name()");
    } else {
        *ppszError = NULL;
    }
    return size;
}

/*
 * Truncate the size of an open file in storage.
 *
 * If not successful *ppszError will set to point to an error string,
 * on success it will be set to NULL.
 */
void
storageTruncate(char** ppszError, int handle, long size) {
    int rv;

    rv = pcsl_file_truncate((void *)handle, size);
    if (rv == -1) {
        *ppszError = getLastError("storageTruncate()");
    } else {
        *ppszError = NULL;
    }

    REPORT_INFO1(LC_CORE, "storageTruncate got rv %d\n", rv);
}

/*
 * Return the amount of free bytes of file storage.
 * The maximum amount of free space is limited by the value set with
 * the <tt>storageSetTotalSpace</tt> method.
 *
 * @param storageId ID of the storage to check for free space
 */
jlong
storage_get_free_space(StorageIdType storageId) {
    jlong freeSpace;
    jlong usedSpace;

    if (storageId < 0 || storageId >= MAX_STORAGE_NUM) {
        return 0; /* Invalid storage ID: no free space available */
    }

    usedSpace = pcsl_file_getusedspace(&sRoot[storageId]);

    if (usedSpace == -1) { /* PCSL error */
        return 0; /* No free space available */
    }

    if (totalSpace > usedSpace) {
        freeSpace = totalSpace - usedSpace;
    } else {
        freeSpace = 0;
    }

    REPORT_INFO1(LC_CORE, "Free space = %ld\n", freeSpace);

    return freeSpace;
}

/*
 * Delete a file in storage.
 *
 * If not successful *ppszError will set to point to an error string,
 * on success it will be set to NULL.
 */
void
storage_delete_file(char** ppszError, const pcsl_string* filename_str) {
    int status;

    DEBUGP2F("trying to delete %s\n", filename_str);

    status = pcsl_file_unlink(filename_str);

    if (status < 0) {
        *ppszError = storage_get_last_file_error("storage_delete_file", filename_str);
        return;
    }

    DEBUGP2F("deleted %s\n", filename_str);

    *ppszError = NULL;
}

/*
 * Rename a file in storage.
 *
 * If not successful *ppszError will set to point to an error string,
 * on success it will be set to NULL.
 */
void
storage_rename_file(char** ppszError, const pcsl_string* oldFilename_str,
                    const pcsl_string* newFilename_str) {
    int status;
    *ppszError = NULL;

    DEBUGP2F("renaming %s", oldFilename_str);
    DEBUGP2F(" to %s\n", newFilename_str);

    if (!storage_file_exists(oldFilename_str)) {
        *ppszError = (char *)NOT_EXIST_RENAME_ERROR;
    }

    if (storage_file_exists(newFilename_str)) {
        storage_delete_file(ppszError, newFilename_str);
        if (*ppszError != NULL) {
            return;
        }
    }

    status = pcsl_file_rename(oldFilename_str, newFilename_str);

    if (status < 0) {
        *ppszError = storage_get_last_file_error("storageRename", newFilename_str);
        return;
    }

    *ppszError = NULL;
}

static char*
getLastError(char* pszFunction) {
    char* temp = NULL;

#ifndef UNDER_CE
    temp = strerror(errno);
#endif
    if (temp == NULL) {
        return "Unspecified Error";
    }

    strcpy(errorBuffer, pszFunction);
    strcat(errorBuffer, ": ");
    strncat(errorBuffer, temp, MAX_ERROR_LEN - 2 - strlen(pszFunction));
    return errorBuffer;
}

static char*
storage_get_last_file_error(char* pszFunction,
                            const pcsl_string* filename_str) {
    char* temp;
    int charsLeft;
    int i;
    int j;
    int lim;

#ifndef UNDER_CE
    temp = strerror(errno);
#else
    LPVOID lpMsgBuf;
    int convertedLength;
    DWORD orgLength;

    orgLength = FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        GetLastError(),
        0, /* Default language */
        (LPTSTR)&lpMsgBuf,
        0,
        NULL
    );

    if (orgLength == 0) {
        return "Unspecified Error";
    }

    convertedLength = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)lpMsgBuf,
            orgLength, errorBuffer, MAX_ERROR_LEN, NULL, NULL);

    LocalFree(lpMsgBuf);

    errorBuffer[convertedLength] = 0x0;
    return errorBuffer;
#endif

    if (temp == NULL) {
        return "Unspecified Error";
    }

    strcpy(errorBuffer, pszFunction);
    strcat(errorBuffer, ": ");
    charsLeft = MAX_ERROR_LEN - strlen(errorBuffer);

    strncat(errorBuffer, temp, charsLeft);
    charsLeft = MAX_ERROR_LEN - strlen(errorBuffer);

    strncat(errorBuffer, ", ", charsLeft);
    charsLeft = MAX_ERROR_LEN - strlen(errorBuffer);

    if (NULL_LEN == pcsl_string_length(filename_str)) {
        strncat(errorBuffer, "NULL", charsLeft);
    } else {
        const jchar* filename_data = pcsl_string_get_utf16_data(filename_str);
        int filename_len = pcsl_string_length(filename_str);
        lim = charsLeft < filename_len ? charsLeft : filename_len;
        for (i = strlen(errorBuffer), j = 0; j < lim; i++, j++) {
            errorBuffer[i] = (char)filename_data[j];
        }

        errorBuffer[i] = 0;
        pcsl_string_release_utf16_data(filename_data,filename_str);
    }

    return errorBuffer;
}

/*
 * Returns the handle that represents the savedRootlength, savedDirectory
 * etc. This handle needs to be passed to storage_get_next_file_in_iterator()
 * in order to get the filename that matches with a given string. In
 * order to clean-up the memory storageCloseFileIterator() must be
 * called to close the handle properly.
 *
 * Returns NULL if memory allocation fails
 * or root directory of the input 'string' can not be found
 *
 */
void*
storage_open_file_iterator(const pcsl_string* string_str) {
    void * iterator = NULL;
    iterator = pcsl_file_openfilelist(string_str);
    return iterator;
}


/*
 * Return the filename in storage that begins with a given string.
 * The order is defined by the underlying file system.
 * This function needs to be repeatedly called for all next
 * occurrents of the file that begins with a given string.
 *
 * Returns 0 on success, -1 otherwise
 *
 */
int storage_get_next_file_in_iterator(const pcsl_string* string_str,
                                      void* handle, pcsl_string* result_str) {
    return pcsl_file_getnextentry(handle, string_str, result_str);
}

/*
 * Close the handle properly
 * and deallocate the memory
 *
 */
void
storageCloseFileIterator(void* handle) {
    pcsl_file_closefilelist(handle);
}

/**
 * Read pcsl_string from storage.
 * First read a jint with length, then read the text
 * of that length in the utf-16 encoding.
 *
 * @param ppszError in the case of error, receives address of a string
 *                  describing the problem; receives NULL in the case of success
 * @param handle    handle of the file to read from
 * @param str       string to receive the text
 */
void
storage_read_utf16_string(char** ppszError, int handle, pcsl_string* str) {
    jint bytesRead = 0;
    jchar *tempStr = NULL;
    jint tempLen = 0;
    pcsl_string_status prc;

    storageRead(ppszError, handle, (char*)&tempLen, sizeof (jint));
    if (*ppszError != NULL) {
        return;
    }

    /* special cases: null and empty strings */
    if (tempLen < 0) {
      if (0 == tempLen) {
          *str = PCSL_STRING_NULL;
      } else if (-1 == tempLen) {
          *str = PCSL_STRING_EMPTY;
      } else {
          *str = PCSL_STRING_NULL;
          *ppszError = (char *)STRING_CORRUPT_ERROR;
      }
      return;
    }

    tempStr = (jchar*)midpMalloc(tempLen * sizeof (jchar));
    if (tempStr == NULL) {
        *ppszError = (char *)OUT_OF_MEM_ERROR;
        return;
    }

    bytesRead = storageRead(ppszError, handle,
          (char*)tempStr, tempLen * sizeof (jchar));
    if (*ppszError != NULL) {
        /* do nothing: error code already there */
    } else if (bytesRead != (signed)(tempLen * sizeof (jchar))) {
        *ppszError = (char *)STRING_CORRUPT_ERROR;
    } else if (PCSL_STRING_OK != (prc
                = pcsl_string_convert_from_utf16(tempStr, tempLen, str))) {
        *ppszError = PCSL_STRING_ENOMEM == prc ? (char *)OUT_OF_MEM_ERROR
                                               : (char *)STRING_CORRUPT_ERROR;
    }
    midpFree(tempStr);
    return;
}

/**
 * Write pcsl_string to storage.
 * First write a jint with length, then the text in utf-16 encoding.
 *
 * @param ppszError  in the case of error, receives address of a string
 *                   describing the problem
 * @param handle     handle of the file to write to
 * @param str        string to be written
 */
void
storage_write_utf16_string(char** ppszError, int handle,
                           const pcsl_string* str) {
    jint length;

    if (pcsl_string_is_null(str)) {
        length = -1;
    } else {
        length = pcsl_string_length(str);
    }

    storageWrite(ppszError, handle, (char*)&length, sizeof (length));

    /* are there data to write? */
    if (NULL == *ppszError && length > 0) {
        const jchar* data = pcsl_string_get_utf16_data(str);
        if (NULL == data) {
            *ppszError = (char *)OUT_OF_MEM_ERROR;
        } else {
            storageWrite(ppszError, handle, (char*)data,
                         length * sizeof (jchar));
            pcsl_string_release_utf16_data(data, str);
        }
    }
}
