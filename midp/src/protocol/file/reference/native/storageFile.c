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
#include <pcsl_string.h>
#include <midpUtilKni.h>

#include <midpError.h>
#include <midpMalloc.h>
#include <midpStorage.h>
#include <midpServices.h>

/**
 * @file
 *
 * Implementation of com.sun.midp.io.j2me.storage.File class.
 *
 * The native methods are a thin layer between the File class
 * and the storage interface (see storage.h) that does the
 * real work. These methods primarily handle the parameter
 * passing and exception handling, and rely upon the platform
 * specific code in storage.c to do the rest of the work.
 */

/**
 * Initializes storage root for this file instance.
 * <p>
 * Java declaration:
 * <pre>
 *     initStorageRoot(V)Ljava/lang/String;
 * </pre>
 *
 * @param storageId ID of the storage the root of which should be returned
 *
 * @return path of the storage root
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_midp_io_j2me_storage_File_initStorageRoot) {
    StorageIdType storageId;
    const pcsl_string* storageRoot;

    KNI_StartHandles(1);
    KNI_DeclareHandle(string);
    storageId = KNI_GetParameterAsInt(1);

    storageRoot = storage_get_root(storageId);

    if (pcsl_string_length(storageRoot) >= 0) {
        midp_jstring_from_pcsl_string(KNIPASSARGS storageRoot, string);
    } else {
        KNI_ReleaseHandle(string);
    }
    KNI_EndHandlesAndReturnObject(string);
}

/**
 * Initializes the configuration root for this file instance.
 * <p>
 * Java declaration:
 * <pre>
 *     initConfigRoot(V)Ljava/lang/String;
 * </pre>
 *
 * @param storageId ID of the storage the config root of which
 * should be returned
 *
 * @return path of the configuration root
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_midp_io_j2me_storage_File_initConfigRoot) {
    StorageIdType storageId;
    const pcsl_string * configRoot;

    KNI_StartHandles(1);
    KNI_DeclareHandle(string);
    storageId = KNI_GetParameterAsInt(1);
    configRoot = storage_get_config_root(storageId);

    if (pcsl_string_length(configRoot) >= 0) {
        midp_jstring_from_pcsl_string(KNIPASSARGS configRoot, string);
    } else {
        KNI_ReleaseHandle(string);
    }

    KNI_EndHandlesAndReturnObject(string);
}

/**
 * Renames storage file.
 * <p>
 * Java declaration:
 * <pre>
 *     renameStorage(Ljava/lang/String;Ljava/lang/String;)V
 * </pre>
 *
 * @param oldName The old name of the storage file
 * @param newName The new name of the storage file
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_io_j2me_storage_File_renameStorage) {
    char* pszError;

    KNI_StartHandles(2);
    GET_PARAMETER_AS_PCSL_STRING(1, oldName)
    GET_PARAMETER_AS_PCSL_STRING(2, newName) {
        storage_rename_file(&pszError, &oldName, &newName);
        if (pszError != NULL) {
            KNI_ThrowNew(midpIOException, pszError);
            storageFreeError(pszError);
        }
    } RELEASE_PCSL_STRING_PARAMETER
    RELEASE_PCSL_STRING_PARAMETER;

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Determines if the storage file with the given filename exists.
 * <p>
 * Java declaration:
 * <pre>
 *     storageExists(Ljava/lang/String;)Z
 * </pre>
 *
 * @param filename The filename of the storage file to match
 *
 * @return <tt>true</tt> if the storage file indicated by <tt>filename</tt>
 *         exists, otherwise <tt>false</tt>
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_midp_io_j2me_storage_File_storageExists) {
    int   ret = 0;

    KNI_StartHandles(1);
    GET_PARAMETER_AS_PCSL_STRING(1, filename)
        ret = storage_file_exists(&filename);
    RELEASE_PCSL_STRING_PARAMETER;
    KNI_EndHandles();
    KNI_ReturnBoolean((jboolean)ret);
}
/**
 * Removes the storage file with the given filename.
 * <p>
 * Java declaration:
 * <pre>
 *     deleteStorage(Ljava/lang/String;)V
 * </pre>
 *
 * @param filename The filename of the storage file to remove
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_io_j2me_storage_File_deleteStorage) {
    char* pszError;

    KNI_StartHandles(1);
    GET_PARAMETER_AS_PCSL_STRING(1, filename) {
        storage_delete_file(&pszError, &filename);
        if (pszError != NULL) {
            KNI_ThrowNew(midpIOException, pszError);
            storageFreeError(pszError);
        }
    } RELEASE_PCSL_STRING_PARAMETER;

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Gets the amount of remaining free storage.
 * <p>
 * Java declaration:
 * <pre>
 *     availableStorage(V)I
 * </pre>
 *
 * @param storageId ID of the storage to check for available space
 *
 * @return the size of the remaining free storage space, in bytes
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_io_j2me_storage_File_availableStorage) {
    StorageIdType storageId = KNI_GetParameterAsInt(1);
    KNI_ReturnInt((jlong)storage_get_free_space(storageId));
}

