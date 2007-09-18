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
 * Javacall interfaces for file
 */

#include "javacall_dir.h"
#include "javacall_file.h"
#include "javacall_logging.h"
#include "javacall_memory.h"
#include "javacall_events.h"

#include <wchar.h>

#include <windows.h>

/**
 * Initializes the File System
 * @return <tt>JAVACALL_OK</tt> on success, <tt>JAVACALL_FAIL</tt> or negative value on error
 */
javacall_result javacall_file_init(void) {
    return JAVACALL_OK;
}

/**
 * Cleans up resources used by file system
 * @return <tt>JAVACALL_OK</tt> on success, <tt>JAVACALL_FAIL</tt> or negative value on error
 */
javacall_result javacall_file_finalize(void) {
    return JAVACALL_OK;
}

/**
 * Opens a file.
 *
 * @param fileName path name of the file to be opened.
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
    DWORD dwDesiredAccess = GENERIC_READ;
    DWORD dwCreationDisposition = OPEN_EXISTING;
    HANDLE fh = INVALID_HANDLE_VALUE;

    if ((flags & JAVACALL_FILE_O_WRONLY) == JAVACALL_FILE_O_WRONLY) {
        dwDesiredAccess = GENERIC_WRITE;
    }
    if ((flags & JAVACALL_FILE_O_RDWR) == JAVACALL_FILE_O_RDWR) {
        dwDesiredAccess = GENERIC_WRITE | GENERIC_READ;
    }
    /* The flags order processing is important.                      
     * consider JAVACALL_FILE_O_RDWR|JAVACALL_FILE_O_APPEND|JAVACALL_FILE_O_CREAT
     * or JAVACALL_FILE_O_RDWR|JAVACALL_FILE_O_TRUNC|JAVACALL_FILE_O_CREAT
     */
    if ((flags & JAVACALL_FILE_O_TRUNC) == JAVACALL_FILE_O_TRUNC) {
        dwCreationDisposition = TRUNCATE_EXISTING;
    }
    if ((flags & JAVACALL_FILE_O_APPEND) == JAVACALL_FILE_O_APPEND) {
        dwCreationDisposition = OPEN_EXISTING;
    }
    if ((flags & JAVACALL_FILE_O_CREAT) == JAVACALL_FILE_O_CREAT) {
        dwCreationDisposition = OPEN_ALWAYS;
        if ((flags & JAVACALL_FILE_O_TRUNC) == JAVACALL_FILE_O_TRUNC) {
            dwCreationDisposition = CREATE_ALWAYS;
        }
    }

    /* create a new unicode NULL terminated file name variable */
    fh = CreateFileW(fileName,
                    dwDesiredAccess,
                    FILE_SHARE_READ|FILE_SHARE_WRITE,
                    NULL,
                    dwCreationDisposition,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);
    /* The original value is 0 but not FILE_SHARE_READ|FILE_SHARE_WRITE; */

#if ENABLE_JAVACALL_IMPL_FILE_LOGS
	javacall_tprintf(TEXT("javacall_file_open >> %s %x\n"), 
	    nullTerminatedFileName, fh);
#endif
    if (fh != INVALID_HANDLE_VALUE) {
        if ((flags & JAVACALL_FILE_O_APPEND) == JAVACALL_FILE_O_APPEND) {
            SetFilePointer(fh, 0, NULL, FILE_END);
        }
        *handle = fh;
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
javacall_result javacall_file_close(javacall_handle handle) {
    BOOL res;

#if ENABLE_JAVACALL_IMPL_FILE_LOGS
	javacall_tprintf(TEXT("javacall_file_close >> %x\n"), handle), 
#endif    

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
long javacall_file_read(javacall_handle handle, unsigned char *buf, long size) {
    BOOL res;
    DWORD read_bytes = 0;

    res = ReadFile((HANDLE)handle, (LPVOID)buf, size, &read_bytes, NULL);
    if (res == TRUE) {
#if ENABLE_JAVACALL_IMPL_FILE_LOGS
        javacall_printf("javacall_file_read >> handle=%x size=%d, read=%d\n", handle, size, read_bytes);
#endif
        return read_bytes;
    }
    return -1;
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
long javacall_file_write(javacall_handle handle, const unsigned char *buf, long size) {
    BOOL res;
    DWORD written_bytes = 0;

    res = WriteFile((HANDLE)handle, (LPCVOID)buf, size, &written_bytes, NULL);
	return (res) ? written_bytes : -1;
}

/**
 * Deletes a file from persistent storage.
 *
 * @param fileName name of file to be deleted
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt> otherwise.
 */
javacall_result javacall_file_delete(javacall_const_utf16_string fileName, 
									 int fileNameLen)
{
    BOOL res;
    res = DeleteFileW(fileName);
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
    DWORD dwCutPointerPosition;
	DWORD dwPreviousPointerPosition;
	dwPreviousPointerPosition = SetFilePointer((HANDLE)handle,
												0,
												NULL,
												FILE_CURRENT);

	if (dwPreviousPointerPosition == INVALID_SET_FILE_POINTER)
		return JAVACALL_FAIL;

    dwCutPointerPosition = SetFilePointer((HANDLE)handle,
                                          (LONG)size,
                                          NULL,
                                          FILE_BEGIN);

#if ENABLE_JAVACALL_IMPL_FILE_LOGS
    javacall_printf( "javacall_file_truncate << handle=%x size=%d newPos=%d",
        handle, size, dwNewPointerPosition);
#endif 
    
    if (dwCutPointerPosition == INVALID_SET_FILE_POINTER) {
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
		if (dwCutPointerPosition <= dwPreviousPointerPosition)
			return JAVACALL_OK;
	}
	dwPreviousPointerPosition = SetFilePointer((HANDLE)handle, dwPreviousPointerPosition, NULL, FILE_BEGIN);
	if (dwPreviousPointerPosition == INVALID_SET_FILE_POINTER)
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
javacall_int64 javacall_file_seek(javacall_handle handle, javacall_int64 offset, javacall_file_seek_flags flag) {
    DWORD dwMoveMethod;
    DWORD dwNewPointerPosition;
    LONG lOffset = (LONG)offset;
    LONG hOffset = (LONG)(offset >> 32);

    ASSERT(hOffset == 0);

    if (flag == JAVACALL_FILE_SEEK_CUR) {
        dwMoveMethod = FILE_CURRENT;
    } else if (flag == JAVACALL_FILE_SEEK_SET) {
        dwMoveMethod = FILE_BEGIN;
    } else {
        dwMoveMethod = FILE_END;
	}
    dwNewPointerPosition = SetFilePointer((HANDLE)handle, lOffset, NULL, dwMoveMethod);
#if ENABLE_JAVACALL_IMPL_FILE_LOGS
			javacall_printf( "javacall_file_seek >> handle=%x offset=%d, move=%d, flag=%d, newp=%d\n", handle, offset, dwMoveMethod, flag, dwNewPointerPosition);
#endif
    if (dwNewPointerPosition == INVALID_SET_FILE_POINTER)
        if (GetLastError() != NO_ERROR)
            return -1;
            
    return dwNewPointerPosition;
}


/**
 * Get file size
 * @param handle identifier of file
 *               This is the identifier returned by pcsl_file_open()
 * @return size of file in bytes if successful, -1 otherwise
 */
javacall_int64 javacall_file_sizeofopenfile(javacall_handle handle) {
    DWORD dwSize;

    dwSize = GetFileSize((HANDLE)handle, NULL);
	return (dwSize != INVALID_FILE_SIZE) ? dwSize : -1;
}

/**
 * Returns the file size.
 *
 * @param fileName name of the file.
 * @return size of file in bytes if successful, -1 otherwise 
 */
javacall_int64 javacall_file_sizeof(javacall_const_utf16_string fileName,
									int fileNameLen)
{
    WIN32_FILE_ATTRIBUTE_DATA fileAttributes;

    if ( GetFileAttributesExW(fileName, GetFileExInfoStandard, (LPVOID)&fileAttributes) ) {
#if ENABLE_JAVACALL_IMPL_FILE_LOGS
	javacall_printf( "javacall_file_sizeof >> size=%d\n", fileAttributes.nFileSizeLow);
#endif
        return fileAttributes.nFileSizeLow;
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
 * @return <tt>JAVACALL_OK </tt> if it exists and is a regular file, 
 *         <tt>JAVACALL_FAIL</tt> otherwise.
 */
javacall_result javacall_file_exist(javacall_const_utf16_string fileName,
									int fileNameLen)
{
    WIN32_FIND_DATAW  fd;
    HANDLE sh;

    if ( (sh = FindFirstFileW(fileName, &fd)) != INVALID_HANDLE_VALUE ) {
        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
        {
            FindClose(sh);
            return JAVACALL_FAIL;
        }
        FindClose(sh);
        return JAVACALL_OK;
    }

    return JAVACALL_FAIL;
}


/** Force the data to be written into the file system storage
 * @param handle identifier of file
 *               This is the identifier returned by javacall_file_open()
 * @return JAVACALL_OK  on success, <tt>JAVACALL_FAIL</tt> or negative value otherwise
 */
javacall_result javacall_file_flush(javacall_handle handle) {

    if (FlushFileBuffers((HANDLE)handle))
        return JAVACALL_OK;
    return JAVACALL_FAIL;
}

/**
 * Renames the file.
 *
 * @param oldFileName current name of file.
 * @param newFileName new name of file.
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise.
 */
javacall_result javacall_file_rename(javacall_const_utf16_string oldFileName,
									 int oldNameLen,
                                     javacall_const_utf16_string newFileName,
									 int newNameLen)
{
    if (MoveFileW(oldFileName, newFileName)) {
        return JAVACALL_OK;
    }

    return JAVACALL_FAIL;
}