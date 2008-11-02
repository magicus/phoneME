/*
 *
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

#include <javacall_file.h>
#include <javacall_dir.h>
#include <javacall_memory.h>
#include <pcsl_memory.h>
#include <pcsl_file.h>
#include <pcsl_print.h>
#include <javacall_logging.h>
#include <stdio.h>
#include <string.h>

/* The constant JAVACALL_MAX_FILE_NAME_LENGTH is defined in "javacall_file.h" */
#define MAX_FILE_LEN JAVACALL_MAX_FILE_NAME_LENGTH

/**
 * Initializes the File System
 * @return 0 on success, -1 otherwise
 */
int
pcsl_file_init() {
    int ret = javacall_file_init();

#ifdef PCSL_DEBUG
    if (ret != JAVACALL_OK)
      pcsl_print("pcsl_file_init Error\n");
#endif
    return ret;
}


/**
 * Cleans up resources used by file system
 * @return 0 on success, -1 otherwise
 */
int
pcsl_file_finalize() {
    int ret = javacall_file_finalize();

#ifdef PCSL_DEBUG
    if (ret != JAVACALL_OK)
      pcsl_print("pcsl_file_finalize Error\n");
#endif
    return ret;
}


/**
 * The open a file
 * @param fileName path name in UNICODE of file to be opened
 * @param fileNameLen length of file name
 * @param flags open control flags
 *              Applications must specify exactly one of the first three
 *              values (file access modes) below in the value of "flags"
 *                  JAVACALL_FILE_O_RDONLY, JAVACALL_FILE_O_WRONLY, JAVACALL_FILE_O_RDWR
 *
 *              Any combination (bitwise-inclusive-OR) of the following may be used:
 *                  JAVACALL_FILE_O_CREAT, JAVACALL_FILE_O_TRUNC, JAVACALL_FILE_O_APPEND,
 *
 * @param handle address of pointer to file identifier
 *               on successful completion, file identifier is returned in this 
 *               argument. This identifier is platform specific and is opaque
 *               to the caller.  
 * @return 0 on success, -1 otherwise
 * 
 */
int
pcsl_file_open(const pcsl_string * fileName, int flags, void **handle) {
  int ret;

#ifdef PCSL_DEBUG
  if (MAX_FILE_LEN < pcsl_string_utf16_length (fileName)) {
    pcsl_print("pcsl_file_open() Error file length is too large");
  }
#endif

  {
    javacall_utf16 utf16_fileName[MAX_FILE_LEN+1] = {0};
    jsize converted_length;
    
    if (PCSL_STRING_OK == pcsl_string_convert_to_utf16 (fileName, utf16_fileName,
                                                        MAX_FILE_LEN+1, &converted_length))
      ret = (JAVACALL_OK == javacall_file_open(utf16_fileName, converted_length, flags, handle)) ? 0 : -1;
    else 
      ret = -1;
  }

  return ret;
}


/**
 * Closes the file with the specified handlei
 * @param handle handle of file to be closed
 * @return 0 on success, -1 otherwise
 */
int
pcsl_file_close(void *handle) {
    int ret = javacall_file_close(handle);

#ifdef PCSL_DEBUG
    if (ret != JAVACALL_OK)
      pcsl_print("pcsl_file_close Error\n");
#endif
    return ret;
}


/**
 * Reads a specified number of bytes from a file, 
 * @param handle handle of file 
 * @param buf buffer to which data is read
 * @param size number of bytes to be read. Actual number of bytes
 *              read may be less, if an end-of-file is encountered
 * @return the number of bytes actually read
 */
int
pcsl_file_read(void *handle, unsigned char *buf, long size) {
    int ret = javacall_file_read(handle, buf, size);

#ifdef PCSL_DEBUG
    if (ret < 0)
    pcsl_print("pcsl_file_read Error\n");
#endif
    return ret;
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
int
pcsl_file_write(void *handle, unsigned char *buffer, long length) {
    int ret = javacall_file_write(handle, buffer, length);

        
#ifdef PCSL_DEBUG
    if (ret < 0)
    pcsl_print("pcsl_file_write Error\n");
#endif
    return ret;
}

/**
 * The unlink function deletes a file from the persistent storage.
 * @param fileName name of file to be deleted
 * @param fileNameLen length of file name
 * @return 0 on success, -1 otherwise
 */
int
pcsl_file_unlink(const pcsl_string * fileName) {
    int ret;

#ifdef PCSL_DEBUG
  if (MAX_FILE_LEN < pcsl_string_utf16_length (fileName)) {
    pcsl_print("pcsl_file_unlink() Error file length is too large");
  }
#endif

  {
    javacall_utf16 utf16_fileName[MAX_FILE_LEN+1] = {0};
    jsize converted_length;
    
    if (PCSL_STRING_OK == pcsl_string_convert_to_utf16 (fileName, utf16_fileName,
                                                        MAX_FILE_LEN+1, &converted_length))
      ret = (JAVACALL_OK == javacall_file_delete(utf16_fileName, converted_length)) ? 0 : -1;
    else 
      ret = -1;
  }

  return ret;
}


/**
 * The  truncate function is used to truncate the size of an open file in storage.
 * @param handle identifier of file to be truncated
 *               This is the identifier returned by javacall_file_open()
 * @param size size to truncate to
 * @return 0 on success, -1 otherwise
 */
int
pcsl_file_truncate(void *handle, long size) {
    int ret = javacall_file_truncate(handle, size);

#ifdef PCSL_DEBUG
    if (ret != JAVACALL_OK)
    pcsl_print("pcsl_file_truncate Error\n");
#endif
    return ret;
}

/**
 * Sets the file pointer associated with a file identifier 
 * @param handle identifier of file
 *               This is the identifier returned by javacall_file_open()
 * @param offset number of bytes to offset file position by
 * @param position controls from where offset is applied, from 
 *                 the beginning, current position or the end
 *                 Can be one of
 *                 JAVACALL_FILE_SEEK_CUR, JAVACALL_FILE_SEEK_SET or JAVACALL_FILE_SEEK_END
 * @return on success the actual resulting offset from beginning of file
 *         is returned, otherwise -1 is returned
 */
long
pcsl_file_seek(void *handle, long offset, long position) {
    long ret = (long)javacall_file_seek(handle, offset, position);

#ifdef PCSL_DEBUG
    if (ret == -1)
    pcsl_print("pcsl_file_seek Error\n");
#endif
    return ret;
}

/**
 * FS only need to support MIDLets to quiry the size of the file. 
 * Check the File size by file handle
 */
long
pcsl_file_sizeofopenfile(void *handle) {
    long ret = (long)javacall_file_sizeofopenfile(handle);

    return ret;
}

/**
 * FS only need to support MIDLets to quiry the size of the file. 
 * Check the File size by file name
 * 
 */
long
pcsl_file_sizeof(const pcsl_string * fileName) {
  long ret;

#ifdef PCSL_DEBUG
  if (MAX_FILE_LEN < pcsl_string_utf16_length (fileName)) {
    pcsl_print("pcsl_file_sizeof() Error file length is too large");
  }
#endif

  {
    javacall_utf16 utf16_fileName[MAX_FILE_LEN+1] = {0};
    jsize converted_length;
    
    if (PCSL_STRING_OK == pcsl_string_convert_to_utf16 (fileName, utf16_fileName,
                                                        MAX_FILE_LEN+1, &converted_length))
      ret = (long)javacall_file_sizeof(utf16_fileName, converted_length);
    else 
      ret = -1;
  }

  return ret;
}

/**
 * Check if the file exists in FS storage.
 * 
 */
int
pcsl_file_exist(const pcsl_string * fileName) {
  int ret;

#ifdef PCSL_DEBUG
  if (MAX_FILE_LEN < pcsl_string_utf16_length (fileName)) {
    pcsl_print("pcsl_file_exist() Error file length is too large");
  }
#endif

  {
    javacall_utf16 utf16_fileName[MAX_FILE_LEN+1] = {0};
    jsize converted_length;
    
    if (PCSL_STRING_OK == pcsl_string_convert_to_utf16 (fileName, utf16_fileName,
                                                        MAX_FILE_LEN+1, &converted_length))
      ret = (JAVACALL_OK == javacall_file_exist(utf16_fileName, converted_length)) ? PCSL_TRUE : PCSL_FALSE;
    else 
      ret = -1;
  }

  return ret;
}

/* Force the data to be written into the FS storage */
int
pcsl_file_commitwrite(void *handle) {
    return 0;                   /* not used */
}


/**
 * Renames the filename.
 * @param oldName current name of file
 * @param oldNameLen current name length
 * @param newName new name of file
 * @param newNameLen length of new name
 * @return 0 on success, -1 otherwise
 */
int
pcsl_file_rename(const pcsl_string * oldName, const pcsl_string * newName) {
  int ret;

#ifdef PCSL_DEBUG
  if ( (MAX_FILE_LEN < pcsl_string_utf16_length (oldName)) || 
       (MAX_FILE_LEN < pcsl_string_utf16_length (oldName)) ) {
    pcsl_print("pcsl_file_rename() Error file length is too large");
  }
#endif

  {
    javacall_utf16 utf16_oldName[MAX_FILE_LEN+1] = {0};
    jsize converted_length_old;
    javacall_utf16 utf16_newName[MAX_FILE_LEN+1] = {0};
    jsize converted_length_new;
    
    if ( (PCSL_STRING_OK == pcsl_string_convert_to_utf16 (oldName, utf16_oldName,
                                                          MAX_FILE_LEN+1, &converted_length_old)) &&
         (PCSL_STRING_OK == pcsl_string_convert_to_utf16 (newName, utf16_newName,
                                                          MAX_FILE_LEN+1, &converted_length_new)) )
      ret = (JAVACALL_OK == javacall_file_rename(utf16_oldName, converted_length_old,
                                                 utf16_newName, converted_length_new)) ? 0 : -1;
    else 
      ret = -1;
  }

  return ret;
}


/**
 * returns a handle to a file list. This handle can be used in
 * subsequent calls to javacall_dir_get_next() to iterate through
 * the file list and get the names of files that match the given string.
 * 
 * @param path the name of a directory, but it can be a
 *             partial file name
 * @param stringLen length of name
 * @return pointer to an opaque filelist structure, that can be used in
 *         javacall_dir_get_next() and javacall_dir_close
 *         NULL returned on error, for example if root directory of the
 *         input 'string' cannot be found.
 */
void *
pcsl_file_openfilelist(const pcsl_string * string) {
  void * ret;
  pcsl_string path = PCSL_STRING_NULL_INITIALIZER;
  const char wd[] = ".";
  jchar sep;
  jint pathLen;

#ifdef PCSL_DEBUG
  if (MAX_FILE_LEN < pcsl_string_utf16_length (string)) {
    pcsl_print("pcsl_file_openfilelist() Error file length is too large");
  }
#endif

  sep = pcsl_file_getfileseparator ();
  pathLen = pcsl_string_last_index_of (string, sep);
  if (pathLen <= 0) {
    if (PCSL_STRING_OK != pcsl_string_convert_from_utf8 ((jbyte *)wd, 1, &path))
      return NULL;
  } else {
    if (PCSL_STRING_OK != pcsl_string_substring (string, 0, pathLen, &path))
      return NULL;
  }

  {
    javacall_utf16 utf16_dirName[MAX_FILE_LEN+1] = {0};
    jsize converted_length;
    
    if (PCSL_STRING_OK == pcsl_string_convert_to_utf16 (&path, utf16_dirName,
                                                        MAX_FILE_LEN+1, &converted_length))
      ret = javacall_dir_open(utf16_dirName, converted_length);
    else 
      ret = NULL;
  }

  pcsl_string_free (&path);
  return ret;

}


/**
 * closes the specified file list. This handle will no longer be
 * associated with this file list.
 * @param handle pointer to opaque filelist struct returned by
 *               javacall_dir_open 
 */
int
pcsl_file_closefilelist(void *handle) {
    if (handle == 0) {
#ifdef PCSL_DEBUG
        pcsl_print("pcsl_file_closefilelist() Invalid Handle\n");
#endif
        return -1;
    }

    javacall_dir_close(handle);
    return 0;
}


/**
 * Checks the size of free space in storage. 
 * @return size of free space
 */
long
pcsl_file_getfreespace() {

  long ret = (long)javacall_dir_get_free_space_for_java();

  return ret;
}

/**
 * Checks the size of used space by all of the files
 * that are contained in the directory.
 * Note: the function does not consider files in subdirectories.
 * @param dirName name of directory
 * @return size of used spacereturns -1 in case of an error,
 */
long pcsl_file_getusedspace(const pcsl_string * dirName) {
  return 0;
}


/* The getNextEntry function search the next file which is  specified DIR */
int
pcsl_file_getnextentry(void *handle,
                       const pcsl_string * string,
                       pcsl_string * result) {

  pcsl_string prefix = PCSL_STRING_NULL_INITIALIZER;
  pcsl_string match = PCSL_STRING_NULL_INITIALIZER;
  jchar sep;
  jint pathLen;
  javacall_utf16 * tmp;
  int utfMatchLen;

  sep = pcsl_file_getfileseparator ();
  pathLen = pcsl_string_last_index_of (string, sep);
  if (pathLen < 0)
    pathLen = -1;
  
  if (PCSL_STRING_OK != pcsl_string_substring (string, pathLen+1, pcsl_string_length (string), &prefix))
    return -1;

  while (PCSL_TRUE) {
    tmp = javacall_dir_get_next (handle, &utfMatchLen);
    if (NULL == tmp) {
      pcsl_string_free (&prefix);
      return -1;
    };
    
    if (PCSL_STRING_OK != pcsl_string_convert_from_utf16 (tmp, utfMatchLen, &match)) {
      pcsl_string_free (&prefix);
      return -1;
    };

    if (pcsl_string_starts_with (&match, &prefix))
      break;

    pcsl_string_free (&match);
  };

  pcsl_string_free (&prefix);
  pcsl_string_free (&match);

  if ( (PCSL_STRING_OK == pcsl_string_substring (string, 0, pathLen+1, result))  &&
       (PCSL_STRING_OK == pcsl_string_append_buf (result, tmp, utfMatchLen)) )
    return 0;
  else {
    pcsl_string_free (result);
    return -1;
  }
}


/**
 *  Returns file separator character used by the underlying file system
 * (usually this function will return '\\';)
 * @return 16-bit Unicode encoded file separator
 */
jchar
pcsl_file_getfileseparator() {
    return javacall_get_file_separator();
}

/**
 * Returns path separator character used by the underlying file system.
 * The file separator, typically, is a single character that separates
 * directories in a path name, for example <dir1>/<dir2>/file.c
 * (usually this function will return ';';)
 * @return 16-bit Unicode encoded path separator
 */
jchar
pcsl_file_getpathseparator() {
  return ':';
}
