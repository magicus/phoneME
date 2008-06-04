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

#include "KNICommon.h"

#include "javautil_unicode.h"

#define MIN_KEY_LENGTH 16
#define MAX_KEY_LENGTH 256

#define MIN_VALUE_LENGTH 64
#define MAX_VALUE_LENGTH 8192

/*  private native int nGetKeyCount(int hNative); */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_mmedia_DirectMetaData_nGetKeyCount) {
    jint handle = KNI_GetParameterAsInt(1);
    javacall_result ret = JAVACALL_FAIL;
    long keys;
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;

    MMP_DEBUG_STR("[kni_metadata] +nGetKeyCount\n");

    if (pKniInfo && pKniInfo->pNativeHandle) {
        ret = javacall_media_get_metadata_key_counts(pKniInfo->pNativeHandle, &keys);
    } else {
        MMP_DEBUG_STR("[nGetKeyCount] Invalid native handle");
    }
    if (ret != JAVACALL_OK) {
        keys = -1;
    }

    KNI_ReturnInt((int)keys);
}

/* private native String nGetKey(int hNative, int index); */
KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_mmedia_DirectMetaData_nGetKey) {
    jint handle = KNI_GetParameterAsInt(1);
    jint index = KNI_GetParameterAsInt(2);
    javacall_result ret = JAVACALL_FAIL;
    javacall_utf16* key = NULL;
    int keySize = MIN_KEY_LENGTH;
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;

    KNI_StartHandles(1);
    KNI_DeclareHandle(stringObj);
    KNI_ReleaseHandle(stringObj);

    MMP_DEBUG_STR("[kni_metadata] +nGetKey\n");

    if (pKniInfo && pKniInfo->pNativeHandle) {
        key = MMP_MALLOC(sizeof(javacall_utf16) * keySize);
        if (key != NULL) {
            ret = javacall_media_get_metadata_key(pKniInfo->pNativeHandle, index, keySize, key);
            while (ret == JAVACALL_OUT_OF_MEMORY && keySize < MAX_KEY_LENGTH) {
                keySize <<= 1;
                key = MMP_REALLOC(key, sizeof(javacall_utf16) * keySize);
                if (key == NULL)
                    break;
                ret = javacall_media_get_metadata_key(pKniInfo->pNativeHandle, index, keySize, key);
            }
            if (key != NULL) {
                if (ret == JAVACALL_OK) {
                    javautil_unicode_utf16_ulength(key, &keySize);
                    KNI_NewString(key, keySize, stringObj);
                }
                MMP_FREE(key);
            }
        }
    } else {
        MMP_DEBUG_STR("[nGetKey] Invalid native handle");
    }

    KNI_EndHandlesAndReturnObject(stringObj);
}

/* private native String nGetKeyValue(int hNative, String key); */
KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_mmedia_DirectMetaData_nGetKeyValue) {
    jint handle = KNI_GetParameterAsInt(1);
    javacall_result ret = JAVACALL_FAIL;
    javacall_utf16* key = NULL;
    int keyLength = 0;
    javacall_utf16* value = NULL;
    int valueSize = MIN_VALUE_LENGTH;
    KNIPlayerInfo* pKniInfo = (KNIPlayerInfo*)handle;

    KNI_StartHandles(2);
    KNI_DeclareHandle(keyObj);
    KNI_DeclareHandle(valueObj);

    KNI_GetParameterAsObject(2, keyObj);
    KNI_ReleaseHandle(valueObj);

    MMP_DEBUG_STR("[kni_metadata] +nGetKeyValue\n");

    if (0 < (keyLength = KNI_GetStringLength(keyObj))) {
        key = MMP_MALLOC((keyLength + 1) * sizeof(jchar));
        if (key) {
            KNI_GetStringRegion(keyObj, 0, keyLength, key);
            key[keyLength] = 0;
        } else {
            keyLength = 0;
        }
    }

    if (keyLength > 0) {
        if (pKniInfo && pKniInfo->pNativeHandle) {
            value = MMP_MALLOC(sizeof(javacall_utf16) * valueSize);
            if (value != NULL) {
                ret = javacall_media_get_metadata(pKniInfo->pNativeHandle, key, valueSize, value);
                while (ret == JAVACALL_OUT_OF_MEMORY && valueSize < MAX_VALUE_LENGTH) {
                    valueSize <<= 1;
                    value = MMP_REALLOC(key, sizeof(javacall_utf16) * valueSize);
                    if (value == NULL)
                        break;
                    ret = javacall_media_get_metadata(pKniInfo->pNativeHandle, key, valueSize, value);
                }
                if (key != NULL) {
                    if (ret == JAVACALL_OK) {
                        javautil_unicode_utf16_ulength(value, &valueSize);
                        KNI_NewString(value, valueSize, valueObj);
                    }
                    MMP_FREE(value);
                }
            }
        } else {
            MMP_DEBUG_STR("[nGetKeyValue] Invalid native handle");
        }
    } else {
        MMP_DEBUG_STR("[nGetKeyValue] Bad key");
    }

    KNI_EndHandlesAndReturnObject(valueObj);
}

