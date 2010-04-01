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

#include <stdio.h>

#include <kni.h>
#include <sni.h>
#include <commonKNIMacros.h>
#include <ROMStructs.h>

#include <midpError.h>
#include <midpStorage.h>
#include <midpUtilKni.h>

/**
 * @file
 *
 * Implementation for RandomAccessStream Java native methods.
 */

/** Get a RandomAccessStream structure from a <code>jobject</code> */
#define getMidpRandomAccessStreamPtr(__handle) \
(unhand(struct Java_com_sun_midp_io_j2me_storage_RandomAccessStream, __handle))

/**
 * Opens a stream to a native file.
 * <p>
 * Java declaration:
 * <pre>
 *     open(Ljava/lang/String;I)I
 * </pre>
 *
 * @param filename the name of the file to open
 * @param mode in which mode to open the file. Must be one of
 *             <tt>READ</tt>, <tt>WRITE</tt>, or <tt>READ_WRITE</tt>
 *
 * @return handle to the native stream
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_io_j2me_storage_RandomAccessStream_open) {
    int   ioMode = KNI_GetParameterAsInt(2);
    int   handle = -1;
    char* pszError;

    KNI_StartHandles(1);
    GET_PARAMETER_AS_PCSL_STRING(1,filename) {
        handle = storage_open(&pszError, &filename, ioMode);

        if (pszError != NULL) {
                KNI_ThrowNew(midpIOException, pszError);
                storageFreeError(pszError);
        }
    } RELEASE_PCSL_STRING_PARAMETER;
    KNI_EndHandles();
    KNI_ReturnInt(handle);
}

/*
 * Closes a native stream.
 * <p>
 * Java declaration:
 * <pre>
 *     open(I)V
 * </pre>
 *
 * @param handle the native handle to the stream
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_io_j2me_storage_RandomAccessStream_close) {
    int   handle;
    char* pszError;

    handle = KNI_GetParameterAsInt(1);
    storageClose(&pszError, handle);
    if (pszError != NULL) {
        KNI_ThrowNew(midpIOException, pszError);
        storageFreeError(pszError);
    }
    KNI_ReturnVoid();
}

/*
 * Reads data from a native stream.
 * <p>
 * Java declaration:
 * <pre>
 *     read(I[BII)I
 * </pre>
 *
 * @param handle the native handle to the stream
 * @param buffer the buffer into which the data is read
 * @param offset the start offset in array <tt>buffer</tt>
 *               at which the data is written
 * @param length the maximum number of bytes to read
 *
 * @return the total number of bytes read into the buffer, or
 *         <tt>-1</tt> if there is no more data because the end of
 *         the stream has been reached.
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_io_j2me_storage_RandomAccessStream_read) {
    int   bytesRead = 0;
    int   length;
    int   offset;
    int   handle;
    char* pszError;

    length = KNI_GetParameterAsInt(4);
    offset = KNI_GetParameterAsInt(3);
    handle = KNI_GetParameterAsInt(1);

    KNI_StartHandles(1);
    KNI_DeclareHandle(buffer);

    KNI_GetParameterAsObject(2, buffer);

    SNI_BEGIN_RAW_POINTERS;
    bytesRead = storageRead(&pszError, handle, 
                    (char*)&(JavaByteArray(buffer)[offset]), length);
    SNI_END_RAW_POINTERS;
        
    KNI_EndHandles();

    if (pszError != NULL) {
        KNI_ThrowNew(midpIOException, pszError);
        storageFreeError(pszError);
    }

    KNI_ReturnInt(bytesRead);
}

/*
 * Writes data to a native stream.
 * <p>
 * Java declaration:
 * <pre>
 *     write(I[BII)V
 * </pre>
 *
 * @param handle the native handle to the stream
 * @param buffer the buffer from which the data is written
 * @param offset the start offset in array <tt>buffer</tt>
 *               at which the data is read
 * @param length the number of bytes to write
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_io_j2me_storage_RandomAccessStream_write) {
    int   length;
    int   offset;
    int   handle;
    char* pszError;

    length = KNI_GetParameterAsInt(4);
    offset = KNI_GetParameterAsInt(3);
    handle = KNI_GetParameterAsInt(1);

    KNI_StartHandles(1);
    KNI_DeclareHandle(buffer);

    KNI_GetParameterAsObject(2, buffer);

    SNI_BEGIN_RAW_POINTERS;
    storageWrite(&pszError, handle, 
                 (char*)&(JavaByteArray(buffer)[offset]), length);
    SNI_END_RAW_POINTERS;
        
    KNI_EndHandles();

    if (pszError != NULL) {
        KNI_ThrowNew(midpIOException, pszError);
        storageFreeError(pszError);
    }

    KNI_ReturnVoid();
}

/*
 * Commits pending writes
 * <p>
 * Java declaration:
 * <pre>
 *     commitwrite(I)V
 * </pre>
 *
 * @param handle the native handle to the stream
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_io_j2me_storage_RandomAccessStream_commitWrite) {
    int   handle;
    char* pszError;

    handle = KNI_GetParameterAsInt(1);
    storageCommitWrite(&pszError, handle);
    if (pszError != NULL) {
        KNI_ThrowNew(midpIOException, pszError);
        storageFreeError(pszError);
    }
    KNI_ReturnVoid();
}

/*
 * Sets the current position of a native stream.
 * <p>
 * Java declaration:
 * <pre>
 *     position(II)V
 * </pre>
 *
 * @param handle the native handle to the stream
 * @param absolutePosition the desired position from the beginning
 *                         of the stream
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_io_j2me_storage_RandomAccessStream_position) {
    long  absolutePosition;
    int   handle;
    char* pszError;

    absolutePosition = (long)KNI_GetParameterAsInt(2);
    handle           = KNI_GetParameterAsInt(1);

    storagePosition(&pszError, handle, absolutePosition);
    if (pszError != NULL) {
        KNI_ThrowNew(midpIOException, pszError);
        storageFreeError(pszError);
    }
    KNI_ReturnVoid();
}

/*
 * Gets the total size of a native stream.
 * <p>
 * Java declaration:
 * <pre>
 *     sizeOf(I)I
 * </pre>
 *
 * @param handle the native handle to the stream
 *
 * @return the size of the stream in bytes
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_io_j2me_storage_RandomAccessStream_sizeOf) {
    int   handle;
    char* pszError;
    long  size;

    handle = KNI_GetParameterAsInt(1);

    size = storageSizeOf(&pszError, handle);
    if (pszError != NULL) {
        KNI_ThrowNew(midpIOException, pszError);
        storageFreeError(pszError);
    }

    KNI_ReturnInt((jint)size);
}

/*
 * Sets the size of a native stream.
 * <p>
 * Java declaration:
 * <pre>
 *     truncateStream(II)V
 * </pre>
 *
 * @param handle the native handle to the stream
 * @param size the size to truncate the native to
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_io_j2me_storage_RandomAccessStream_truncateStream) {
    int   size;
    int   handle;
    char* pszError;

    size   = KNI_GetParameterAsInt(2);
    handle = KNI_GetParameterAsInt(1);

    storageTruncate(&pszError, handle, size);
    if (pszError != NULL) {
        KNI_ThrowNew(midpIOException, pszError);
        storageFreeError(pszError);
    }
    KNI_ReturnVoid();
}

/**
 * Releases any native resources used by this object.
 * <p>
 * Java declaration:
 * <pre>
 *     finalize(V)V
 * </pre>
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_io_j2me_storage_RandomAccessStream_finalize) {
    char* pszError;
    int   handle;

    KNI_StartHandles(1);
    KNI_DeclareHandle(thisObject);
    KNI_GetThisPointer(thisObject);

    SNI_BEGIN_RAW_POINTERS;
    handle = getMidpRandomAccessStreamPtr(thisObject)->handle;
    SNI_END_RAW_POINTERS;

    KNI_EndHandles();

    if (handle != -1) {
        storageClose(&pszError, handle);
        if (pszError != NULL) {
            storageFreeError(pszError);
        }
    }

    KNI_ReturnVoid();
}
