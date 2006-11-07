/*
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

/**
 * @file fileio.c
 *
 * This file implements native functionality required by FileIO.java.
 * It provides I/O utilities like file creation/deletion, reading,
 * writing and seeking.
 */

/* IMPL_NOTE:
 * - temp file handling is not nice, should redesign
 *   the FileIO class
 */

#include <stdio.h>
#include <memory.h>
#ifndef WIN32
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#endif
#include "mni.h"
#ifdef WIN32
#include <windows.h>
#endif

#ifndef NO_GETENV
#ifndef HAVE_STDLIB_H		/* <stdlib.h> should declare getenv() */
extern char *getenv(const char * name);
#endif
#endif

#define MAX_CHUNK_SIZE 8192

/* initializes a variable filename which will
 * receive the filename passed as first parameter
 * in a byte array.
 * filename needs to be freed with MNI_FREE.
 */
#ifdef MIDP
#define GET_FILENAME \
    char *filename; \
    int arrayLen; \
    /* Use KNI-specific access to arrays */ \
    KNI_StartHandles(1); \
    KNI_DeclareHandle(arrHandle); \
    KNI_GetParameterAsObject(1, arrHandle); \
    arrayLen = KNI_GetArrayLength(arrHandle); \
    filename = (char*) MNI_MALLOC(arrayLen + 1); \
    KNI_GetRawArrayRegion(arrHandle, 0, arrayLen, \
                          (jbyte *) filename); \
    KNI_EndHandles(); \
    filename[arrayLen] = 0 /* Null terminate */
#else /* non-MIDP */
#define GET_FILENAME \
    char *filename; \
    int arrayLen; \
    char *tempName; \
    MNI_GET_BYTE_ARRAY_PARAM(tempName, jfilename, arrayLen, 1); \
    filename = (char*) MNI_MALLOC(arrayLen + 1); \
    memcpy(filename, tempName, arrayLen); \
    MNI_RELEASE_BYTE_ARRAY_PARAM(tempName, jfilename, JNI_ABORT); \
    filename[arrayLen] = 0 /* Null terminate */
#endif


/**
 * This function opens a file for reading or writing binary data.
 * A mode value of 0 specifies that the file should be opened
 * for reading. A mode of 1 specifies that the file should be
 * opened for writing; if the file exists it is truncated, otherwise
 * it is created.
 * @param jfilename Name of the file specified as a byte array.
 * @param jmode Mode in which to open the file.
 * @return A handle to the opened file.
 */

MNI_RET_TYPE_INT
Java_com_sun_mmedia_FileIO_nOpen
(MNI_FUNCTION_PARAMS_2(jbyteArray jfilename, jint jmode)) {
    int mode;
    int handle = 0;
    FILE *fp = NULL;

    GET_FILENAME;
    MNI_GET_INT_PARAM(mode, jmode, 2);

    if (mode == 0) {
        fp = fopen(filename, "rb");  /* Read */
    } else {
        fp = fopen(filename, "wb");  /* Write */
    }
    MNI_FREE(filename);

    if (fp != NULL) {
    	handle = (int) fp;
    }
    MNI_RET_VALUE_INT(handle);
}

/**
 * This function writes data from a byte array to a file
 * @param jhandle handle that represents the file to be written.
 * @param jdata  The byte array containing the data to be written.
 * @param joffset Array offset from which valid data begins.
 * @param jlength Number of bytes to write.
 * @return The number of bytes actually written, 0 if there was an error.
 */
MNI_RET_TYPE_INT
Java_com_sun_mmedia_FileIO_nWrite
(MNI_FUNCTION_PARAMS_4(jint jhandle, jbyteArray jdata, jint joffset,
		       jint jlength)) {
    int arrayLen;
    int bytesWritten = 0;
    char *data;
    int handle;
    int offset;
    int length;

    MNI_GET_INT_PARAM(handle, jhandle, 1);
    MNI_GET_INT_PARAM(offset, joffset, 3);
    MNI_GET_INT_PARAM(length, jlength, 4);

#ifdef MIDP
    KNI_StartHandles(1);
    KNI_DeclareHandle(arrHandle);
    KNI_GetParameterAsObject(2, arrHandle);
    arrayLen = KNI_GetArrayLength(arrHandle);
    data = (char*) MNI_MALLOC(arrayLen);
    
    if (data != NULL) {
        KNI_GetRawArrayRegion(arrHandle, 0, arrayLen, (jbyte *) data);
        bytesWritten = fwrite(data + offset, 1, length, (FILE*) handle);
        MNI_FREE(data);
    } else {
        bytesWritten = 0;
        MNI_THROW_NEW("java/lang/OutOfMemoryError", "Can't allocate file write buffer");
    }
    KNI_EndHandles();
#else /* non-MIDP */
    MNI_GET_BYTE_ARRAY_PARAM(data, jdata, arrayLen, 2);
    bytesWritten = fwrite(data + offset, 1, length, (FILE*) handle);
    MNI_RELEASE_BYTE_ARRAY_PARAM(data, jdata, JNI_ABORT);
#endif /* MIDP */

    MNI_RET_VALUE_INT(bytesWritten);
}


/**
 * This function reads data from a file into a byte array
 * @param jhandle handle that represents the file to read.
 * @param jdata  The byte array to read the data into.
 * @param joffset Data read from the file will be stored in the
 * byte array with this array offset.
 * @param jlength Number of bytes to read.
 * @return The number of bytes actually read, 0 if there was an error.
 */
MNI_RET_TYPE_INT
Java_com_sun_mmedia_FileIO_nRead
(MNI_FUNCTION_PARAMS_4(jint jhandle, jbyteArray jdata, jint joffset,
		       jint jlength)) {
    int bytesRead = 0;
    int MNI_GET_INT_PARAM(handle, jhandle, 1);
    int MNI_GET_INT_PARAM(offset, joffset, 3);
    int MNI_GET_INT_PARAM(length, jlength, 4);

    if (length == -1) {
        fflush((FILE*) handle);
        fclose((FILE*) handle);
    } else {
        char* data = (char*) MNI_MALLOC(MAX_CHUNK_SIZE);
        if (data != NULL) {
        	bytesRead = fread(data, 1, length, (FILE*) handle);
            MNI_SET_BYTE_ARRAY_REGION(data, jdata, offset, length, 2);
            MNI_FREE(data);
        } else {
        	bytesRead = 0;
            MNI_THROW_NEW("java/lang/OutOfMemoryError", "Can't allocate file read buffer");
        }
    }
    MNI_RET_VALUE_INT(bytesRead);
}


/**
 * This function attempts to move to file pointer to a location
 * specified by the second parameter.
 * @param jhandle handle that represents the file to seek.
 * @param joffset The location in the file to seek to.
 * @return True if the seek was successful, false otherwise.
 */
MNI_RET_TYPE_BOOLEAN
Java_com_sun_mmedia_FileIO_nSeek
(MNI_FUNCTION_PARAMS_2(jint jhandle, jint joffset)) {
    int MNI_GET_INT_PARAM(handle, jhandle, 1);
    int MNI_GET_INT_PARAM(offset, joffset, 2);
    int status;

    status = fseek((FILE*) handle, (long) offset, SEEK_SET);
    MNI_RET_VALUE_BOOLEAN((status == 0 ?  MNI_TRUE : MNI_FALSE));
}


/**
 * This function attempts to delete the file specified.
 * @param jfilename Name of the file specified as a byte array.
 * @return True if deletion was successful, false otherwise.
 */
MNI_RET_TYPE_BOOLEAN
Java_com_sun_mmedia_FileIO_nDelete
(MNI_FUNCTION_PARAMS_1(jbyteArray jfilename)) {
    int status;
    GET_FILENAME;

    /* Delete the file */
    status = unlink(filename);
    MNI_FREE(filename);

    MNI_RET_VALUE_BOOLEAN((status == 0 ?  MNI_TRUE : MNI_FALSE));
}

/**
 * This function attempts to close the file specified.
 * closing the file releases any resources used.
 * @param jhandle handle that represents the file to be closed.
 * @return True if the operation was successful, false otherwise.
 */
MNI_RET_TYPE_BOOLEAN
Java_com_sun_mmedia_FileIO_nClose
(MNI_FUNCTION_PARAMS_1(jint jhandle)) {
    int MNI_GET_INT_PARAM(handle, jhandle, 1);
    int status;

    status = fclose((FILE*) handle);
    MNI_RET_VALUE_BOOLEAN((status == 0 ?  MNI_TRUE : MNI_FALSE));
}


/**
 * This function returns the name of a directory where temporary files
 * can be stored.
 * @param jTempDir the byte array into which the name of the
 * temporary directory is returned.
 * @return the length of the temporary directory name.
 */
MNI_RET_TYPE_INT
Java_com_sun_mmedia_FileIO_nGetTempDirectory
(MNI_FUNCTION_PARAMS_1(jbyteArray jTempDir)) {
    int tempDirLength = 0;

#ifndef WIN32
    char *tempDir;
  /* Check for an environment variable TEMP that contains the
   * name of the temporary directory.
   * If your system doesn't support getenv(), define NO_GETENV.
   * A default temp directory will be used. The temp directory is
   * written to the byte array input/output parameter and the length of
   * the temporary directory string is returned.
   */
#ifndef NO_GETENV
    tempDir = getenv("TEMP");
    if (tempDir == NULL)
	tempDir = "/tmp";/* Can do ifdef for different platforms like WIN32 */
    tempDirLength = strlen(tempDir);

    MNI_SET_BYTE_ARRAY_REGION(tempDir, jTempDir, 0, tempDirLength, 1);
#endif
#else /* WINDOWS */
    char tempDir[MAX_PATH];
    tempDirLength = GetTempPath(MAX_PATH, tempDir);
    if (tempDirLength > 1) {
    	/* remove trailing backslash */
    	tempDirLength--;
    	tempDir[tempDirLength] = 0;
	MNI_SET_BYTE_ARRAY_REGION(tempDir, jTempDir, 0, tempDirLength, 1);
    } else {
	tempDirLength = 0;
    }
#endif
    MNI_RET_VALUE_INT(tempDirLength);
}

/* This function deletes temporary files (in the temporary
 * directory) that were created by the current or previous instances
 * of midlets.
 *
 * @param jdata The byte array that contains the temporary directory
 * name and the file prefix. All files in the temporary directory
 * that have the specified prefix will be removed if possible.
 */
MNI_RET_TYPE_VOID
Java_com_sun_mmedia_FileIO_nRemoveTempFiles
(MNI_FUNCTION_PARAMS_1(jbyteArray jdata)) {
#if defined(JTWI_HI) && defined(LINUX)
    int arrayLen;
    char *tempPattern;
    char *data;

    MNI_GET_BYTE_ARRAY_PARAM(data, jdata, arrayLen, 1);
    tempPattern = MNI_MALLOC(20 + arrayLen);
    strcpy(tempPattern, "/bin/rm -f ");
    strncat(tempPattern, data, arrayLen);
    system(tempPattern);
    MNI_FREE(tempPattern);
    MNI_RELEASE_BYTE_ARRAY_PARAM(data, jdata, JNI_ABORT);
#endif
}
