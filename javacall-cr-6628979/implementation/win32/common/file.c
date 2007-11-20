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

/**
 * @file
 *
 * Implemenation for file javacall functions
 */

#include "javacall_dir.h"
#include "javacall_file.h"
#include "javacall_logging.h"
#include "javacall_memory.h"
#include "javacall_events.h"

#include <wchar.h>

#include <windows.h>

/*
 * This constants are defined in "WinBase.h" when using VS7 (2003) and VS8 (2005),
 * but absent in Visual C++ 6 headers.
 * For successful build with VC6 we need to define it manually.
 */

#ifndef INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER ((DWORD)-1)
#endif

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif

 
/**
 * Creates null terminated version of string in externally allocated buffer
 *
 * @param name initial string
 * @param length of desired string or
 *        JAVACALL_UNKNOWN_LENGTH, which may be used for null terminated string 
 * @param suggest_buffer allocated space
 * @param suggest_length length of extern buffer
 */
static javacall_const_utf16_string 
    get_string_extern(javacall_const_utf16_string name, int length,
                      javacall_utf16* suggest_buffer, int suggest_length)
{
    if (length == JAVACALL_UNKNOWN_LENGTH)
        return name;
    if (length >= suggest_length)
        return NULL;
    memcpy(suggest_buffer, name, sizeof(javacall_utf16)*length);
    suggest_buffer[length] = '\0';
    return suggest_buffer;
}


/**
 * Returns null terminated version of string
 *
 * @param name initial string
 * @param length of desired string or
 *        JAVACALL_UNKNOWN_LENGTH, which may be used for null terminated string 
 * @return a buffer with null terminated string
 */
static javacall_const_utf16_string 
    get_string_alloc(javacall_const_utf16_string name, int length)
{
    javacall_utf16* nt_name;
    if (length == JAVACALL_UNKNOWN_LENGTH)
        return name;
    nt_name = (javacall_utf16*) malloc(sizeof(javacall_utf16)*(length+1));
    if (nt_name != NULL) {
        memcpy(nt_name, name, sizeof(javacall_utf16)*length);
        nt_name[length] = '\0';
    }
    return nt_name;
}

/**
 * Releases the allocated memory block
 *
 * @param orString original string
 * @suggest_buffer buffer, holding the null terminated string
 */
static void 
    release_string_alloc(javacall_const_utf16_string name, 
                         javacall_const_utf16_string nt_name)
{
    if (name != nt_name)
        free((void*)nt_name);
}

/**
 * Initializes the File System
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt> or negative value on error
 */
javacall_result javacall_file_init(void)
{
    return JAVACALL_OK;
}

/**
 * Cleans up resources used by file system
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt> or negative value on error
 */
javacall_result javacall_file_finalize(void)
{
    return JAVACALL_OK;
}

/**
 * Opens a file.
 *
 * @param fileName path name of the file to be opened.
 * @param fileNameLen length of the file name or
 *        JAVACALL_UNKNOWN_LENGTH, which may be used for null terminated string 
 * @param flags open control flags.
 *        Applications must specify exactly one of the first three
 *        values (file access modes) below in the value of "flags":
 *        JAVACALL_FILE_O_RDONLY,
 *        JAVACALL_FILE_O_WRONLY,
 *        JAVACALL_FILE_O_RDWR
 *
 *        and any combination (bitwise-inclusive-OR) of the following:
 *        JAVACALL_FILE_O_CREAT,
 *        JAVACALL_FILE_O_TRUNC,
 *        JAVACALL_FILE_O_APPEND,
 *
 * @param handle pointer to store the file identifier.
 *        On successful completion, file identifier is returned in this
 *        argument. This identifier is platform specific and is opaque
 *        to the caller.
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise.
 *
 */
javacall_result javacall_file_open(javacall_const_utf16_string fileName,
                                  int fileNameLen,
                                  int flags,
                                  javacall_handle* /* OUT */ handle)
{
    DWORD dwDesiredAccess;
    DWORD dwCreationDisposition;
    HANDLE fh = INVALID_HANDLE_VALUE;

    /* create a new unicode NULL terminated file name variable */
    javacall_const_utf16_string sFileName = get_string_alloc(fileName, fileNameLen);

    switch (flags & (JAVACALL_FILE_O_RDWR | JAVACALL_FILE_O_WRONLY)) {
        case JAVACALL_FILE_O_WRONLY:
            dwDesiredAccess = GENERIC_WRITE;
            break;
        case JAVACALL_FILE_O_RDWR:
            dwDesiredAccess = GENERIC_WRITE | GENERIC_READ;
            break;
        default:
            dwDesiredAccess = GENERIC_READ;
            break;
    }

    switch (flags & (JAVACALL_FILE_O_CREAT | JAVACALL_FILE_O_TRUNC)) {
        case JAVACALL_FILE_O_CREAT | JAVACALL_FILE_O_TRUNC:
            dwCreationDisposition = CREATE_ALWAYS;
            break;
        case JAVACALL_FILE_O_CREAT:
            dwCreationDisposition = OPEN_ALWAYS;
            break;
        case JAVACALL_FILE_O_TRUNC:
            dwCreationDisposition = TRUNCATE_EXISTING;
            break;
        default:
            dwCreationDisposition = OPEN_EXISTING;
            break;
    }

    fh = CreateFileW(sFileName,
                    dwDesiredAccess,
                    FILE_SHARE_READ|FILE_SHARE_WRITE,
                    NULL,
                    dwCreationDisposition,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);

    release_string_alloc(fileName, sFileName);

    *handle = fh;
    if (fh != INVALID_HANDLE_VALUE) {
        if ((flags & JAVACALL_FILE_O_APPEND) == JAVACALL_FILE_O_APPEND) {
            SetFilePointer(fh, 0, NULL, FILE_END);
        }
        return JAVACALL_OK;
    }
    return JAVACALL_FAIL;
}

/**
 * Closes the file with the specified handlei
 * @param handle handle of file to be closed
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> or negative value otherwise
 */
javacall_result javacall_file_close(javacall_handle handle)
{
    BOOL res;
    res = CloseHandle((HANDLE) handle);
    return (res) ? JAVACALL_OK : JAVACALL_FAIL;
}

/**
 * Reads a specified number of bytes from a file,
 * @param handle handle of file
 * @param buf buffer to which data is read
 * @param size number of bytes to be read. Actual number of bytes
 *              read may be less, if an end-of-file is encountered
 * @return the number of bytes actually read
 */
long javacall_file_read(javacall_handle handle, unsigned char *buf, long size)
{
    BOOL res;
    DWORD read_bytes = 0;

    res = ReadFile((HANDLE)handle, (LPVOID)buf, size, &read_bytes, NULL);
#if ENABLE_JAVACALL_IMPL_FILE_LOGS
    if (res == TRUE) {
        javacall_printf("javacall_file_read >> handle=%x size=%d, read=%d\n", 
                handle, size, read_bytes);
    }
#endif
    return (res) ? read_bytes :  -1;
}

/**
 * Writes bytes to file
 * @param handle handle of file
 * @param buf buffer to be written
 * @param size number of bytes to write
 * @return the number of bytes actually written. This is normally the same
 *         as size, but might be less (for example, if the persistent storage being
 *         written to fills up).
 */
long javacall_file_write(javacall_handle handle, const unsigned char *buf, long size)
{
    BOOL res;
    DWORD written_bytes = 0;

    res = WriteFile((HANDLE)handle, (LPCVOID)buf, size, &written_bytes, NULL);
    return (res) ? written_bytes : -1;
}

/**
 * Deletes a file from persistent storage.
 *
 * @param fileName name of file to be deleted
 * @param fileNameLen length of the file name or
 *        JAVACALL_UNKNOWN_LENGTH, which may be used for null terminated string 
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise.
 */
javacall_result javacall_file_delete(javacall_const_utf16_string fileName,
                                    int fileNameLen)
{
    BOOL res;
    /* create a new unicode NULL terminated file name variable */
    javacall_const_utf16_string sFileName = get_string_alloc(fileName, fileNameLen);

    res = DeleteFileW(sFileName);

    release_string_alloc(fileName, sFileName);
    return (res) ? JAVACALL_OK : JAVACALL_FAIL;
}

/**
 * Used to truncate the size of an open file in storage.
 * Function keeps current file pointer position, except case,
 * when current position is also cut
 *
 * @param handle identifier of the file to be truncated, as returned
 *               by javacall_file_open().
 * @param size size to truncate to.
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise.
 */
javacall_result javacall_file_truncate(javacall_handle handle,
                                      javacall_int64 size)
{
    int state;
    javacall_int64 cutPosition;
    javacall_int64 previousPosition;

    previousPosition = javacall_file_seek(handle, 0, JAVACALL_FILE_SEEK_CUR);
    if (previousPosition == -1)
        return JAVACALL_FAIL;

    cutPosition = javacall_file_seek(handle, size, JAVACALL_FILE_SEEK_SET);

#if ENABLE_JAVACALL_IMPL_FILE_LOGS
    javacall_printf( "javacall_file_truncate << handle=%x size=%d newPos=%d",
        handle, size, cutPosition);
#endif

    if (cutPosition == -1) {
#if ENABLE_JAVACALL_IMPL_FILE_LOGS
    javacall_printf( "javacall_file_truncate fail 1 >>");
#endif
           return JAVACALL_FAIL;
   }

    state = 1;
    if (!SetEndOfFile((HANDLE)handle)) {
#if ENABLE_JAVACALL_IMPL_FILE_LOGS
    javacall_printf( "javacall_file_truncate fail 2 (%d) >>\n", GetLastError());
#endif
        state = 0;
    } else {
        if (cutPosition <= previousPosition)
            return JAVACALL_OK;
    }
    previousPosition = javacall_file_seek(handle, previousPosition,JAVACALL_FILE_SEEK_SET);
    if (previousPosition == -1)
        state = 0;

    return (state) ? JAVACALL_OK : JAVACALL_FAIL;
}

/**
 * Sets the file pointer associated with a file identifier
 * @param handle identifier of file
 *               This is the identifier returned by javacall_file_open()
 * @param offset number of bytes to offset file position by
 * @param flag controls from where offset is applied, from
 *                 the beginning, current position or the end
 *                 Can be one of JAVACALL_FILE_SEEK_CUR, JAVACALL_FILE_SEEK_SET
 *                 or JAVACALL_FILE_SEEK_END
 * @return on success the actual resulting offset from beginning of file
 *         is returned, otherwise -1 is returned
 */
javacall_int64 javacall_file_seek(javacall_handle handle, javacall_int64 offset, 
                                  javacall_file_seek_flags flag)
{
    DWORD dwMoveMethod;
    LARGE_INTEGER newPosition;
    newPosition.QuadPart = offset;
    switch (flag) {
    case JAVACALL_FILE_SEEK_CUR:
        dwMoveMethod = FILE_CURRENT;
        break;
    case JAVACALL_FILE_SEEK_SET:
        dwMoveMethod = FILE_BEGIN;
        break;
    default:
        dwMoveMethod = FILE_END;
        break;
    }

    newPosition.LowPart = SetFilePointer((HANDLE)handle, newPosition.LowPart, 
        &newPosition.HighPart, dwMoveMethod);

#if ENABLE_JAVACALL_IMPL_FILE_LOGS
    javacall_printf( "javacall_file_seek >> handle=%x offset=%d, move=%d, flag=%d, newp=%d\n", 
           handle, offset, dwMoveMethod, flag, newPosition.QuadPart);
#endif
    if (newPosition.LowPart == INVALID_SET_FILE_POINTER) {
        if (GetLastError() != NO_ERROR)
            return -1;
    }

   return newPosition.QuadPart;
}


/**
 * Get file size
 * @param handle identifier of file
 *               This is the identifier returned by javacall_file_open()
 * @return size of file in bytes if successful, -1 otherwise
 */
javacall_int64 javacall_file_sizeofopenfile(javacall_handle handle)
{
    LARGE_INTEGER size;
    BOOL isOk;
    size.HighPart = 0;
    size.LowPart = GetFileSize((HANDLE)handle, &size.HighPart);
    isOk = (size.LowPart != INVALID_FILE_SIZE) || (GetLastError() == NO_ERROR);
    return isOk ? size.QuadPart : -1;
}

/**
 * Returns the file size.
 *
 * @param fileName name of the file.
 * @param fileNameLen length of the file name or
 *        JAVACALL_UNKNOWN_LENGTH, which may be used for null terminated string 
 * @return size of file in bytes if successful, -1 otherwise
 */
javacall_int64 javacall_file_sizeof(javacall_const_utf16_string fileName,
                                   int fileNameLen)
{
    WIN32_FILE_ATTRIBUTE_DATA fileAttributes;
    BOOL res;
    LARGE_INTEGER size;
    /* create a new unicode NULL terminated file name variable */
    javacall_const_utf16_string sFileName = get_string_alloc(fileName, fileNameLen);
    res = GetFileAttributesExW(sFileName, GetFileExInfoStandard, (LPVOID)&fileAttributes);
    release_string_alloc(fileName, sFileName);
    if (res) {
#if ENABLE_JAVACALL_IMPL_FILE_LOGS
    javacall_printf( "javacall_file_sizeof >> size=%d\n", fileAttributes.nFileSizeLow);
#endif
        size.LowPart = fileAttributes.nFileSizeLow;
        size.HighPart = fileAttributes.nFileSizeHigh;
        return size.QuadPart;
    }

#if ENABLE_JAVACALL_IMPL_FILE_LOGS
    javacall_printf( "javacall_file_sizeof >> fail\n");
#endif
    return -1;
}

/**
 * Checks if the file exists in file system storage.
 *
 * @param fileName name of the file.
 * @param fileNameLen length of the file name or
 *        JAVACALL_UNKNOWN_LENGTH, which may be used for null terminated string 
 * @return <tt>JAVACALL_OK </tt> if it exists and is a regular file,
 *         <tt>JAVACALL_FAIL</tt> otherwise.
 */
javacall_result javacall_file_exist(javacall_const_utf16_string fileName,
                                   int fileNameLen)
{
    DWORD attrib;
    /* create a new unicode NULL terminated file name variable */
    javacall_const_utf16_string sFileName = get_string_alloc(fileName, fileNameLen);
    attrib = GetFileAttributesW(sFileName);
    release_string_alloc(fileName, sFileName);
    if ((INVALID_FILE_ATTRIBUTES != attrib) && ((attrib & FILE_ATTRIBUTE_DIRECTORY) == 0))
        return JAVACALL_OK;
    return JAVACALL_FAIL;
}


/** Force the data to be written into the file system storage
 * @param handle identifier of file
 *               This is the identifier returned by javacall_file_open()
 * @return JAVACALL_OK  on success, <tt>JAVACALL_FAIL</tt> or negative value otherwise
 */
javacall_result javacall_file_flush(javacall_handle handle)
{
    return (FlushFileBuffers((HANDLE)handle)) ? JAVACALL_OK : JAVACALL_FAIL;
}

/**
 * Renames the filename.
 * @param unicodeOldFilename current name of file
 * @param oldNameLen current name length or
 *        JAVACALL_UNKNOWN_LENGTH, which may be used for null terminated string 
 * @param unicodeNewFilename new name of file
 * @param newNameLen length of new name or
 *        JAVACALL_UNKNOWN_LENGTH, which may be used for null terminated string 
 * @return <tt>JAVACALL_OK</tt>  on success, 
 *         <tt>JAVACALL_FAIL</tt> or negative value otherwise
 */
javacall_result javacall_file_rename(javacall_const_utf16_string oldFileName,
                                    int oldNameLen,
                                    javacall_const_utf16_string newFileName,
                                    int newNameLen)
{
    BOOL res;
    /* create new unicode NULL terminated file name variables */
    javacall_const_utf16_string sOldFileName = get_string_alloc(oldFileName, oldNameLen);
    javacall_const_utf16_string sNewFileName = get_string_alloc(newFileName, newNameLen);
    res = MoveFileW(sOldFileName, sNewFileName);
    release_string_alloc(oldFileName, sOldFileName);
    release_string_alloc(newFileName, sNewFileName);
    return (res) ? JAVACALL_OK : JAVACALL_FAIL;
}
