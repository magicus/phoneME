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

/*
 * This file contains the calls to use VM's resource sharing 
 * api. It holds a static shared object reference that can be
 * retrieved by all isolates.
 */
#include <stdio.h>
#include <string.h>

#include <jvmconfig.h>
#include <kni.h>
#include <jvm.h>
#include <sni.h>
#include <midpError.h>
#include <ROMStructs.h>
#include <midpMidletSuiteUtils.h>
#include <midpStorage.h>
#include <midpUtilKni.h>
#include <commonKNIMacros.h>
#include <kni_globals.h>
#include <midpMalloc.h>
#include <midp_logging.h>

#include <lfj_image_rom.h>
#include <lcdlf_export.h>

/* IMPL_NOTE : Initialize this constant each time the vm restarts! */

#if ENABLE_MULTIPLE_ISOLATES    
static int resourcePool = -1;
static int skinProperties = -1;
#endif

/* String encoding constants from SkinResourcesConstants */
static unsigned char STRING_ENCODING_USASCII;
static unsigned char STRING_ENCODING_UTF8;

/* Binary skin file data array pointers */
/*
 * These buffers may contain the following types of values:
 * - unsigned values for data length and encoding
 * - US-ASCII characters in the range 0..127
 * - UTF-8 octets in the range 0..255
 */
static unsigned char* gsSkinFileDataStart = NULL;
static unsigned char* gsSkinFileDataEnd = NULL;
static unsigned char* gsSkinFileDataPos = NULL;

/*
 * Macro to ensure that we aren't reading past
 * the end of skin data
 */
#define ENSURE_SKIN_DATA_AVAILABILITY(size) \
    if (gsSkinFileDataPos + (size) > gsSkinFileDataEnd) { \
        KNI_ThrowNew(midpIllegalStateException, \
                "Unexpected end of skin data file"); \
        break; \
    }

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_chameleon_skins_resources_SkinResourcesImpl_shareResourcePool) {

#if ENABLE_MULTIPLE_ISOLATES    
    KNI_StartHandles(1);
    KNI_DeclareHandle(obj);
    KNI_GetParameterAsObject(1, obj);
    
    if (resourcePool == -1) {
        /* 
         * We only ever allow a single resource pool to be stored. 
         * Any future attempts at storing a new one are ignored
         */
        resourcePool = SNI_AddStrongReference(obj);
    }
    
    KNI_EndHandles();                                                                                                
#endif
    KNI_ReturnVoid();

}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_chameleon_skins_resources_SkinResourcesImpl_shareSkinData) {

#if ENABLE_MULTIPLE_ISOLATES    
    KNI_StartHandles(1);
    KNI_DeclareHandle(obj);
    KNI_GetParameterAsObject(1, obj);
    
    if (skinProperties == -1) {
        /* 
         * We only ever allow a single resource pool to be stored. 
         * Any future attempts at storing a new one are ignored.
         */
        skinProperties = SNI_AddStrongReference(obj);
    }
    
    KNI_EndHandles();                                                                                                
#endif
    KNI_ReturnVoid();

}

KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_midp_chameleon_skins_resources_SkinResourcesImpl_getSharedResourcePool) {
    
#if ENABLE_MULTIPLE_ISOLATES
    KNI_StartHandles(1);
    KNI_DeclareHandle(obj);
    if (resourcePool >= 0) {
        SNI_GetReference(resourcePool, obj);
    }
    KNI_EndHandlesAndReturnObject(obj);
#else
    KNI_StartHandles(1);
    KNI_DeclareHandle(tempHandle);
    KNI_EndHandlesAndReturnObject(tempHandle);
#endif

}

KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_midp_chameleon_skins_resources_SkinResourcesImpl_getSharedSkinData) {
    
#if ENABLE_MULTIPLE_ISOLATES
    KNI_StartHandles(1);
    KNI_DeclareHandle(obj);
    if (skinProperties >= 0) {
        SNI_GetReference(skinProperties, obj);
    }
    KNI_EndHandlesAndReturnObject(obj);
#else
    KNI_StartHandles(1);
    KNI_DeclareHandle(tempHandle);
    KNI_EndHandlesAndReturnObject(tempHandle);
#endif

}
                                                                                                        
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_midp_chameleon_skins_resources_SkinResourcesImpl_ifLoadAllResources0) {
    
#if ENABLE_MULTIPLE_ISOLATES
    /* 
     * There is no need to load all images for non AMS 
     * isolates, they are to be loaded later on demand
     */
    if (JVM_CurrentIsolateID() == midpGetAmsIsolateId()) {
        KNI_ReturnBoolean(KNI_TRUE);
    } else {
        KNI_ReturnBoolean(KNI_FALSE);
    }
#else
    KNI_ReturnBoolean(KNI_FALSE);
#endif

}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_chameleon_skins_resources_LoadedSkinResources_finalize) {
#if ENABLE_MULTIPLE_ISOLATES
    if ((JVM_CurrentIsolateID() == midpGetAmsIsolateId()) &&
        (resourcePool >= 0)) {
        SNI_DeleteReference(resourcePool);
        resourcePool = -1;
    }
#endif
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_chameleon_skins_resources_LoadedSkinData_finalize) {
#if ENABLE_MULTIPLE_ISOLATES
    if ((JVM_CurrentIsolateID() == midpGetAmsIsolateId()) &&
        (skinProperties >= 0)) {
        SNI_DeleteReference(skinProperties);
        skinProperties = -1;
    }
#endif
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_chameleon_skins_resources_SkinResourcesImpl_getRomizedImageDataArrayPtr) {
    jint imageId = KNI_GetParameterAsInt(1);
    
    unsigned char* imageData;
    lfj_load_image_from_rom(imageId, &imageData);

    KNI_ReturnInt((int)(void*)imageData);
}

KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_chameleon_skins_resources_SkinResourcesImpl_getRomizedImageDataArrayLength) {
    jint imageId = KNI_GetParameterAsInt(1);
    
    unsigned char* imageData;
    int len = lfj_load_image_from_rom(imageId, &imageData);

    KNI_ReturnInt(len);
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_chameleon_skins_resources_LoadedSkinData_beginReadingSkinFile) {
    const unsigned char* skin_description = lfj_get_skin_description();

    if (skin_description != NULL) {
        gsSkinFileDataStart = gsSkinFileDataPos =
            (unsigned char*)skin_description;
        gsSkinFileDataEnd = gsSkinFileDataStart +
            lfj_get_skin_description_size();
    } else {
        char* errorStr = NULL;
        int fileHandle = -1;
        int fileSize;
        int bytesRead;
        jfieldID fid;

        KNI_StartHandles(2);
        KNI_DeclareHandle(classHandle);

        KNI_GetClassPointer(classHandle);

        fid = KNI_GetStaticFieldID(classHandle, "STRING_ENCODING_USASCII", "B");
        STRING_ENCODING_USASCII = (unsigned char)
            KNI_GetStaticByteField(classHandle, fid);

        fid = KNI_GetStaticFieldID(classHandle, "STRING_ENCODING_UTF8", "B");
        STRING_ENCODING_UTF8 = (unsigned char)
            KNI_GetStaticByteField(classHandle, fid);

        GET_PARAMETER_AS_PCSL_STRING(1, fileName);

        do {
            /*
             * Open skin file
             */
            fileHandle = storage_open(&errorStr, &fileName, OPEN_READ);
            if (errorStr != NULL) {
                KNI_ThrowNew(midpIOException, errorStr);
                storageFreeError(errorStr);
                break;
            }

            /*
             * Obtain file size
             */
            fileSize = storageSizeOf(&errorStr, fileHandle);
            if (errorStr != NULL) {
                KNI_ThrowNew(midpIOException, errorStr);
                storageFreeError(errorStr);
                break;
            }

            /*
             * Read whole file into heap memory
             */
            gsSkinFileDataStart = (unsigned char*)midpMalloc(fileSize);
            if (gsSkinFileDataStart == NULL) {
                KNI_ThrowNew(midpOutOfMemoryError, NULL);
                break;
            }

            bytesRead = storageRead(&errorStr, fileHandle,
                    (char*)gsSkinFileDataStart, fileSize);
            if (errorStr != NULL) {
                KNI_ThrowNew(midpIOException, errorStr);
                storageFreeError(errorStr);
                midpFree(gsSkinFileDataStart);
                gsSkinFileDataStart = NULL;
                break;
            }
            if (bytesRead != fileSize) {
                KNI_ThrowNew(midpIOException, "Failed to read whole file");
                midpFree(gsSkinFileDataStart);
                gsSkinFileDataStart = NULL;
                break;
            }

            gsSkinFileDataPos = gsSkinFileDataStart;
            gsSkinFileDataEnd = gsSkinFileDataStart + fileSize;

        } while (0);

        RELEASE_PCSL_STRING_PARAMETER;

        /*
         * Close skin file
         */
        if (fileHandle != -1) {
            storageClose(&errorStr, fileHandle);
        }

        KNI_EndHandles();
    }

    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_chameleon_skins_resources_LoadedSkinData_finishReadingSkinFile) {

    if (lfj_get_skin_description_size() < 0) {
        /* skin data file was not romized, so free memory allocated for it */
        midpFree(gsSkinFileDataStart);
    }

    gsSkinFileDataStart = NULL;
    gsSkinFileDataPos = NULL;
    gsSkinFileDataEnd = NULL;

    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_midp_chameleon_skins_resources_LoadedSkinData_readByteArray) {
    int arrayLength = KNI_GetParameterAsInt(1);

    KNI_StartHandles(1);
    KNI_DeclareHandle(returnArray);

    do {
        ENSURE_SKIN_DATA_AVAILABILITY(arrayLength);

        SNI_NewArray(SNI_BYTE_ARRAY, arrayLength, returnArray);
        if (KNI_IsNullHandle(returnArray)) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
            break;
        }

        KNI_SetRawArrayRegion(returnArray, 0, arrayLength, (jbyte*)gsSkinFileDataPos);
        gsSkinFileDataPos += arrayLength;

    } while (0);

    KNI_EndHandlesAndReturnObject(returnArray);
}

KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_midp_chameleon_skins_resources_LoadedSkinData_readIntArray) {
    int arrayLength;
    int totalBytes;

    KNI_StartHandles(1);
    KNI_DeclareHandle(returnArray);

    do {
        /*
         * First, read array length
         */
        ENSURE_SKIN_DATA_AVAILABILITY(sizeof(jint));
        memcpy((void*)&arrayLength, (void*)gsSkinFileDataPos, sizeof(jint));
        gsSkinFileDataPos += sizeof(jint);

        /*
         * Then create array
         */
        totalBytes = arrayLength * sizeof(jint);
        ENSURE_SKIN_DATA_AVAILABILITY(totalBytes);

        SNI_NewArray(SNI_INT_ARRAY, arrayLength, returnArray);
        if (KNI_IsNullHandle(returnArray)) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
            break;
        }

        /*
         * And finally read data into it
         */
        KNI_SetRawArrayRegion(returnArray, 0, totalBytes, (jbyte*)gsSkinFileDataPos);
        gsSkinFileDataPos += totalBytes;

    } while (0);

    KNI_EndHandlesAndReturnObject(returnArray);
}

KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_midp_chameleon_skins_resources_LoadedSkinData_readStringArray) {
    int arrayLength;
    int i;

    KNI_StartHandles(2);
    KNI_DeclareHandle(returnArray);
    KNI_DeclareHandle(stringHandle);

    do {
        /*
         * First, read array length
         */
        ENSURE_SKIN_DATA_AVAILABILITY(sizeof(jint));
        memcpy((void*)&arrayLength, (void*)gsSkinFileDataPos, sizeof(jint));
        gsSkinFileDataPos += sizeof(jint);


        /*
         * Then create array
         */
        SNI_NewArray(SNI_STRING_ARRAY, arrayLength, returnArray);
        if (KNI_IsNullHandle(returnArray)) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
            break;
        }

        /*
         * And finally populate it with strings
         */
        for (i = 0; i < arrayLength; ++i) {
            unsigned char dataLength;
            unsigned char encoding;

            /* read data length */
            ENSURE_SKIN_DATA_AVAILABILITY(sizeof(char));
            dataLength = *((unsigned char*)gsSkinFileDataPos);
            gsSkinFileDataPos += 1;

            /* read encoding */
            ENSURE_SKIN_DATA_AVAILABILITY(sizeof(char));
            encoding = *((unsigned char*)gsSkinFileDataPos);
            gsSkinFileDataPos += 1;

            ENSURE_SKIN_DATA_AVAILABILITY(dataLength * sizeof(char));

            if (encoding == STRING_ENCODING_USASCII) {    
                int j;

                /* 
                 * In case of USASCII encoding, each byte of 
                 * string data corresponds to one string char
                 */
                int stringLength = dataLength;

                /* use gKNIBuffer for storing string chars */
                jchar* stringChars = (jchar*)gKNIBuffer;
                
                /* 
                 * Safety measure to prevent gKNIBuffer overflow 
                 * (which should never happens unless something is broken) 
                 */
                if (stringLength > (int)(KNI_BUFFER_SIZE/sizeof(jchar))) {
                    stringLength = (int)(KNI_BUFFER_SIZE/sizeof(jchar));
                    REPORT_WARN(LC_HIGHUI, 
                            "gKNIBuffer is too small for skin string");
                }

                /* fill string chars array */
                for (j = 0; j < stringLength; ++j) {
                    stringChars[j] = gsSkinFileDataPos[j];
                }

                /* and create string from it */
                KNI_NewString(stringChars, stringLength, stringHandle);
            } else if (encoding == STRING_ENCODING_UTF8) {
                KNI_NewStringUTF((char*)gsSkinFileDataPos, stringHandle);
            } else {
                KNI_ThrowNew(midpIllegalStateException, 
                        "Illegal skin string encoding");
                break;
            }
            
            KNI_SetObjectArrayElement(returnArray, i, stringHandle);

            gsSkinFileDataPos += dataLength;
        }

    } while (0);

    KNI_EndHandlesAndReturnObject(returnArray);
}
