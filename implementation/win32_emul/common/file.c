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

/**
 * @file
 *
 * win32 implemenation for file javacall functions
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <io.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>

#include "javacall_file.h"
#include "javacall_dir.h"
#include "javacall_logging.h"
#include "javautil_unicode.h"
#include "file.h"

#define MAX_NULL_TERMINATED_NAME_LENGTH (JAVACALL_MAX_FILE_NAME_LENGTH + 1)

static int stringlastindexof(unsigned short *str,
                             int strLen,
                             unsigned short ch);


/**
 * Returns the index within this string of the last occurrence of the
 * specified character. That is, the index returned is the largest
 * value <i>k</i> such that:
 * <blockquote><pre>
 * string.data[<i>k</i>] == ch
 * </pre></blockquote>
 * is true.
 * The String is searched backwards starting at the last character.
 *
 * @param   ch	 a character.
 * @return  the index of the last occurrence of the character in the
 *	    character sequence represented by this object, or
 *	    <code>-1</code> if the character does not occur.
 */
static int stringlastindexof(unsigned short *str,
                             int strLen,
                             unsigned short ch) {

    int i;

    if (strLen <= 0) {
        return	-1;
    }

    for (i = strLen - 1; i >= 0; i--) {
        if (str[i] == ch) {
            return i;
        }
    }

    return -1;
} /* end of stringlastindexof */



char* unicode_to_char(unsigned short* str) {

    static char char_name[JAVACALL_MAX_FILE_NAME_LENGTH*2+1];
    javacall_int32 strLen = wcslen(str);
    javacall_int32 charLen;

    javacall_result res = javautil_unicode_utf16_to_utf8((javacall_utf16*)str,
                                                         strLen,
                                                         char_name,
                                                         sizeof(char_name)-1,
                                                         &charLen);
    if (res != JAVACALL_OK)
        charLen = 0;
    char_name[charLen] = '\0';

    return char_name;
}



unsigned short* char_to_unicode(char* str) {

    static unsigned short unicode_name[JAVACALL_MAX_FILE_NAME_LENGTH+1];
    javacall_int32 strLen = strlen(str);
    javacall_int32 utf16len;

    javacall_result res = javautil_unicode_utf8_to_utf16((unsigned char*)str,
                                                         strLen,
                                                         (javacall_utf16*)unicode_name,
                                                         sizeof(unicode_name)-1,
                                                         &utf16len);
    if (res != JAVACALL_OK)
        utf16len = 0;
    unicode_name[utf16len] = 0;
    
    return unicode_name;
}

/**
 * Initializes the File System
 * @return JAVACALL_OK on success, JAVACALL_FAIL otherwise
 */
javacall_result javacall_file_init(void) {
    return JAVACALL_OK;
}

/**
 * Cleans up resources used by file system
 * @return JAVACALL_OK on success, JAVACALL_FAIL otherwise
 */
javacall_result javacall_file_finalize(void) {
    return JAVACALL_OK;
}

/**
 * The open a file
 * @param unicodeFileName name in UNICODE of file to be opened
 * @param fileNameLen length of file name
 * @param flags open control flags
 *              Applications must specify exactly one of the first three
 *              values (file access modes) below in the value of "flags"
 *              JAVACALL_FILE_O_RDONLY,
 *              JAVACALL_FILE_O_WRONLY,
 *              JAVACALL_FILE_O_RDWR
 *
 *              Any combination (bitwise-inclusive-OR) of the following may
 *              be used:
 *              JAVACALL_FILE_O_CREAT,
 *              JAVACALL_FILE_O_TRUNC,
 *              JAVACALL_FILE_O_APPEND,
 *
 * @param handle address of pointer to file identifier
 *               on successful completion, file identifier is returned in this
 *               argument. This identifier is platform specific and is opaque
 *               to the caller.
 * @return JAVACALL_OK on success, JAVACALL_FAIL otherwise
 *
 */
javacall_result javacall_file_open(const javacall_utf16*  unicodeFileName,
                                   int                      fileNameLen,
                                   int                      flags,
                                   /*OUT*/ javacall_handle* handle) {

    int fd;
    int oFlag =  O_BINARY;
    int creationMode = 0;

    wchar_t wOsFilename[MAX_NULL_TERMINATED_NAME_LENGTH]; // max file name

    if( fileNameLen > JAVACALL_MAX_FILE_NAME_LENGTH ) {
	 javautil_debug_print (JAVACALL_LOG_ERROR, "file", "javacall_file_open: ERROR - File name length exceeds max file length");
        return JAVACALL_FAIL;
    }

    memcpy(wOsFilename, unicodeFileName, fileNameLen*sizeof(wchar_t));
    wOsFilename[fileNameLen] = 0;

    /* compute open control flag */
    if ((flags & JAVACALL_FILE_O_WRONLY) == JAVACALL_FILE_O_WRONLY) {
        oFlag |= O_WRONLY;
        //javacall_print(" JAVACALL_FILE_O_WRONLY ");

    }
    if ((flags & JAVACALL_FILE_O_RDWR) == JAVACALL_FILE_O_RDWR) {
        oFlag |= O_RDWR;
        //javacall_print(" JAVACALL_FILE_O_RDWR ");
    }
    if ((flags & JAVACALL_FILE_O_CREAT) == JAVACALL_FILE_O_CREAT) {
        oFlag |= O_CREAT;
        creationMode = _S_IREAD | _S_IWRITE;
        //javacall_print(" JAVACALL_FILE_O_CREAT ");

    }
    if ((flags & JAVACALL_FILE_O_TRUNC) == JAVACALL_FILE_O_TRUNC) {
        oFlag |= O_TRUNC;
        //javacall_print(" JAVACALL_FILE_O_TRUNC ");

    }
    if ((flags & JAVACALL_FILE_O_APPEND) == JAVACALL_FILE_O_APPEND) {
        oFlag |= O_APPEND;
        //javacall_print(" JAVACALL_FILE_O_APPEND ");

    }

    //javacall_print("\n");

    fd = _wopen(wOsFilename, oFlag, creationMode);

    if(fd < 0) {
        *handle = NULL;
        javautil_debug_print(JAVACALL_LOG_WARNING, "rms", "javacall_file_open: _wopen failed for: %s\n",unicode_to_char(wOsFilename));
        return JAVACALL_FAIL;
    }

    *handle = (void *)fd;
    return JAVACALL_OK;
} /* end of javacall_file_open */


/**
 * Closes the file with the specified handle
 * @param handle handle of file to be closed
 * @return JAVACALL_OK on success, JAVACALL_FAIL otherwise
 */
javacall_result javacall_file_close(javacall_handle handle){

    int res;
    res = _close((int) handle);
    if(res == -1) {
        return JAVACALL_FAIL;
    }
    return JAVACALL_OK;
}

/**
 * Reads a specified number of bytes from a file,
 * @param handle handle of file
 * @param buf buffer to which data is read
 * @param size number of bytes to be read. Actual number of bytes
 *              read may be less, if an end-of-file is encountered
 * @return the number of bytes actually read
 *         If the function tries to read at end of file, it returns 0.
 *         If the handle is invalid, or the file is not open for reading,
 *         or the file is locked, the function returns ?1
 */
long javacall_file_read(javacall_handle handle,
                        unsigned char *buf,
                        long size){

    return _read((int) handle, buf, size);
}

/**
 * Writes bytes to file
 * @param handle handle of file
 * @param buf buffer to be written
 * @param size number of bytes to write
 * @return the number of bytes actually written. This is normally the same
 *         as size, but might be less (for example, if the persistent storage
 *         being written to fills up).
 *         A return value of ?1 indicates an error.
 */
long javacall_file_write(javacall_handle handle,
                         const unsigned char* buf,
                         long size) {

    return _write((int) handle, buf, size);
}

/**
 * Force the data to be written into the file system storage
 * @param handle identifier of file
 *               This is the identifier returned by javacall_file_open()
 * @return 0 on success, -1 otherwise
 */
javacall_result javacall_file_flush(javacall_handle handle) {

    int res;
    res = _commit((int) handle);
    if(res == -1) {
        return JAVACALL_FAIL;
    }
    return JAVACALL_OK;
}

/**
 * Deletes a file from the persistent storage.
 * @param unicodeFileName name of file to be deleted
 * @param fileNameLen length of file name
 * @return JAVACALL_OK on success, or JAVACALL_FAIL on failure
 */
javacall_result javacall_file_delete(const javacall_utf16* unicodeFileName,
                                     int fileNameLen){

    int res;

    wchar_t wOsFilename[MAX_NULL_TERMINATED_NAME_LENGTH]; // max file name

    if( fileNameLen > JAVACALL_MAX_FILE_NAME_LENGTH ) {
	 javautil_debug_print (JAVACALL_LOG_ERROR, "file", "javacall_file_delete: File name length exceeds max file length");
        return JAVACALL_FAIL;
    }

    memcpy(wOsFilename, unicodeFileName, fileNameLen*sizeof(wchar_t));
    wOsFilename[fileNameLen] = 0;


    res = _wunlink(wOsFilename);
    if (res == -1) {
    	return JAVACALL_FAIL;
    }

    return JAVACALL_OK;
}


/**
 * The  truncate function is used to truncate the size of an open file in storage.
 * Java uses truncate only to make file size smaller.
 * @param handle identifier of file to be truncated
 *               This is the identifier returned by javacall_file_open()
 * @param size size to truncate to
 * @return JAVACALL_OK on success, or JAVACALL_FAIL on failure
 */
javacall_result javacall_file_truncate(javacall_handle handle, javacall_int64 size){

    int res;

    res = _chsize((int)handle, (long)size);
    if(res == -1) {
        return JAVACALL_FAIL;
    }
    return JAVACALL_OK;
}

/**
 * Sets the file pointer associated with a file identifier
 * @param handle identifier of file
 *               This is the identifier returned by javacall_file_open()
 * @param offset number of bytes to offset file position by
 * @param position controls from where offset is applied, from
 *                 the beginning, current position or the end
 *                 Can be one of
 *                 JAVACALL_FILE_SEEK_CUR,
 *                 JAVACALL_FILE_SEEK_SET or JAVACALL_FILE_SEEK_END
 * @return on success the actual resulting offset from beginning of file
 *         is returned, otherwise -1 is returned
 */
javacall_int64 javacall_file_seek(javacall_handle handle,
                        javacall_int64 offset,
                        javacall_file_seek_flags position) {

    return _lseek((int)handle, (long)offset, position);
}

/**
 * Get file size
 * @param handle identifier of file
 *               This is the identifier returned by pcsl_file_open()
 * @return size of file in bytes if successful, -1 otherwise
 */
javacall_int64 javacall_file_sizeofopenfile(javacall_handle handle){

    struct _stat stat_buf;

    if (_fstat((int)handle, &stat_buf) == -1) {
        return -1;
    }
    return stat_buf.st_size;
}

/**
 * Get file size
 * @param fileName name of file in unicode format
 * @param fileNameLen length of file name
 * @return size of file in bytes if successful, -1 otherwise
 */
javacall_int64 javacall_file_sizeof(const javacall_utf16* fileName,
                          int fileNameLen) {

    struct _stat stat_buf;
    int result;

    wchar_t wOsFilename[MAX_NULL_TERMINATED_NAME_LENGTH]; // max file name

    if( fileNameLen > JAVACALL_MAX_FILE_NAME_LENGTH ) {
	 javautil_debug_print (JAVACALL_LOG_ERROR, "file", "javacall_file_sizeof: File name length exceeds max file length");
        return -1;
    }

    memcpy(wOsFilename, fileName, fileNameLen*sizeof(wchar_t));
    wOsFilename[fileNameLen] = 0;

    result = _wstat( wOsFilename, &stat_buf );

    if (result == 0) {
        return stat_buf.st_size;
    }

    return -1;

} /* end of javacall_file_sizeof */

/**
 * file flag
 */
#define S_ISREG(mode)	( ((mode) & S_IFMT) == S_IFREG )


/**
 * Check if the file exists in file system storage.
 * @param fileName name of file in unicode format
 * @param fileNameLen length of file name
 * @return JAVACALL_OK  if it exists and is a regular file,
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_file_exist(const javacall_utf16* fileName,
                                    int fileNameLen) {

    struct _stat stat_buf;
    int res;

    wchar_t wOsFilename[MAX_NULL_TERMINATED_NAME_LENGTH]; // max file name

    if( fileNameLen > JAVACALL_MAX_FILE_NAME_LENGTH ) {
	 javautil_debug_print (JAVACALL_LOG_ERROR, "file", "javacall_file_exist: File name length exceeds max file length");
        return JAVACALL_FAIL;
    }

    memcpy(wOsFilename, fileName, fileNameLen*sizeof(wchar_t));
    wOsFilename[fileNameLen] = 0;

    res = _wstat(wOsFilename, &stat_buf);

    if (res >= 0 && S_ISREG(stat_buf.st_mode)) {
        /* stat completed without error and it is a file */
        return JAVACALL_OK;
    }

    /* either stat completed with error or it is not a file */
    return JAVACALL_FAIL;
}

/**
 * Renames the filename.
 * @param unicodeOldFilename current name of file
 * @param oldNameLen current name length
 * @param unicodeNewFilename new name of file
 * @param newNameLen length of new name
 * @return <tt>JAVACALL_OK</tt>  on success,
 *         <tt>JAVACALL_FAIL</tt> or negative value otherwise
 */
javacall_result javacall_file_rename(const javacall_utf16*  unicodeOldFilename,
                                     int   oldNameLen,
                                     const javacall_utf16* unicodeNewFilename,
                                     int   newNameLen){

     int res;

     wchar_t wOldFilename[MAX_NULL_TERMINATED_NAME_LENGTH]; // max file name
     wchar_t wNewFilename[MAX_NULL_TERMINATED_NAME_LENGTH]; // max file name

    if( oldNameLen > JAVACALL_MAX_FILE_NAME_LENGTH ) {
	 javautil_debug_print (JAVACALL_LOG_ERROR, "file", "javacall_file_rename: Old file name length exceeds max file length");
        return JAVACALL_FAIL;
    }
    if( newNameLen > JAVACALL_MAX_FILE_NAME_LENGTH ) {
	 javautil_debug_print (JAVACALL_LOG_ERROR, "file", "javacall_file_rename: New file name length exceeds max file length");
        return JAVACALL_FAIL;
    }

    memcpy(wNewFilename, unicodeNewFilename, newNameLen*sizeof(wchar_t));
    wNewFilename[newNameLen] = 0;

    memcpy(wOldFilename, unicodeOldFilename, oldNameLen*sizeof(wchar_t));
    wOldFilename[oldNameLen] = 0;


    res = _wrename(wOldFilename, wNewFilename);
    if(res == 0) {
        return JAVACALL_OK;
    }
    return JAVACALL_FAIL;

} /* end of javacall_file_rename */

#ifdef __cplusplus
}
#endif

