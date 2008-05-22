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
#ifndef _SUITESTORE_KNI_UTIL_H
#define _SUITESTORE_KNI_UTIL_H

#include <midpError.h>
#include <midpString.h>
#include <midpUtilKni.h>

/**
 * @file
 * Contains native method helper functions and macros that useful in other
 * files besides midletsuitestorage.c.
 */

#define KNI_RESTORE_BYTE_FIELD(OBJECT,CLASS,NAME,VALUE) \
    KNI_SetByteField((OBJECT), KNI_GetFieldID((CLASS),(NAME),"B"), (VALUE))

#define KNI_RESTORE_BOOLEAN_FIELD(OBJECT,CLASS,NAME,VALUE) \
    KNI_SetBooleanField((OBJECT), KNI_GetFieldID((CLASS),(NAME),"Z"), (VALUE))

#define KNI_RESTORE_INT_FIELD(OBJECT,CLASS,NAME,VALUE) \
    KNI_SetIntField((OBJECT), KNI_GetFieldID((CLASS),(NAME),"I"), (VALUE))

#define KNI_RESTORE_STRING_FIELD(OBJECT,CLASS,NAME,VALUE,HANDLE) { \
    if (VALUE.len >= 0) { \
        KNI_NewString(VALUE.data, (jsize)(VALUE.len), (HANDLE)); \
    } else { \
        KNI_ReleaseHandle(HANDLE); \
    } \
 \
    KNI_SetObjectField((OBJECT), \
        KNI_GetFieldID((CLASS), (NAME), "Ljava/lang/String;"), (HANDLE)); \
}

/* we do KNI_ReleaseHandle in case of out-of-mem error */
#define KNI_RESTORE_PCSL_STRING_FIELD(OBJECT,CLASS,NAME,VALUE,HANDLE) { \
    const jsize VALUE_len = pcsl_string_length(VALUE); \
    const jchar* VALUE_data = pcsl_string_get_utf16_data(VALUE); \
    if (VALUE_data != NULL && VALUE_len >= 0) { \
        KNI_NewString(VALUE_data, VALUE_len, (HANDLE)); \
    } else { \
        KNI_ReleaseHandle(HANDLE); \
    } \
    pcsl_string_release_utf16_data(VALUE_data, VALUE); \
 \
    KNI_SetObjectField((OBJECT), \
        KNI_GetFieldID((CLASS), (NAME), "Ljava/lang/String;"), (HANDLE)); \
}


#define KNI_SAVE_BYTE_FIELD(OBJECT,CLASS,NAME,VALUE) \
    (VALUE) = KNI_GetByteField((OBJECT), KNI_GetFieldID((CLASS),(NAME),"B"))

#define KNI_SAVE_BOOLEAN_FIELD(OBJECT,CLASS,NAME,VALUE) \
    (VALUE) = KNI_GetBooleanField((OBJECT), KNI_GetFieldID((CLASS),(NAME),"Z"))

#define KNI_SAVE_INT_FIELD(OBJECT,CLASS,NAME,VALUE) \
    (VALUE) = KNI_GetIntField((OBJECT), KNI_GetFieldID((CLASS),(NAME),"I"))

#define KNI_SAVE_PCSL_STRING_FIELD(OBJECT,CLASS,NAME,VALUE,HANDLE) { \
    pcsl_string_free(VALUE); \
    if (PCSL_STRING_ENOMEM == \
        midp_get_string_field(KNIPASSARGS (OBJECT), (CLASS), (NAME), (HANDLE), (VALUE))) { \
        KNI_ThrowNew(midpOutOfMemoryError, NULL); \
        break; \
    } \
}


#define GET_STRING_PARAM(NUM,OBJECT,VALUE,STATUS) { \
    *(VALUE) = PCSL_STRING_NULL; \
    KNI_GetParameterAsObject(NUM, OBJECT); \
    if (KNI_GetStringLength(OBJECT) >= 0) { \
        if (PCSL_STRING_OK != midp_jstring_to_pcsl_string(OBJECT, VALUE)) { \
            (STATUS) = OUT_OF_MEM_LEN; \
            break; \
        } \
    } \
}

/*
 * This value is returned if one of input strings passed as a parameter
 * or fetched from parameter data structures turns out to be null or empty.
 * Applies to the following macros:
 *     GET_NON_EMPTY_STRING_PARAM
 *     GET_STRING_ARRAY_PARAM
 */
#define STATUS_FOR_NULL_OR_EMPTY_STRING BAD_PARAMS

#define GET_NON_EMPTY_STRING_PARAM(NUM,OBJECT,VALUE,STATUS) { \
    pcsl_string_status tmp; \
    KNI_GetParameterAsObject(NUM, OBJECT); \
    tmp = midp_jstring_to_pcsl_string(OBJECT, VALUE); \
    if (PCSL_STRING_OK != tmp) { \
        if (PCSL_STRING_ENOMEM == tmp) { \
            (STATUS) = OUT_OF_MEM_LEN; \
        } else { \
            (STATUS) = BAD_PARAMS; \
        } \
        break; \
    } else if (pcsl_string_length(VALUE) <= 0) { \
        (STATUS) = STATUS_FOR_NULL_OR_EMPTY_STRING; \
        break; \
    } \
}

#define GET_NON_EMPTY_BYTE_ARRAY_PARAM(NUM,PARAM,ARRAY,ARRAY_LEN,STATUS) { \
    KNI_GetParameterAsObject(NUM, PARAM); \
    (ARRAY_LEN) = (int)KNI_GetArrayLength(PARAM); \
 \
    if ((ARRAY_LEN) <= 0) { \
        (STATUS) = (ARRAY_LEN); \
        break; \
    } \
 \
    (ARRAY) = (jbyte*)midpMalloc(ARRAY_LEN); \
    if ((ARRAY) == NULL) { \
        (STATUS) = OUT_OF_MEM_LEN; \
        break; \
    } \
 \
    KNI_GetRawArrayRegion((PARAM), 0, (ARRAY_LEN), (jbyte*)(ARRAY)); \
}

#define GET_BYTE_ARRAY_PARAM(NUM,PARAM,ARRAY,ARRAY_LEN,STATUS) { \
    KNI_GetParameterAsObject(NUM, PARAM); \
    (ARRAY_LEN) = (int)KNI_GetArrayLength(PARAM); \
 \
    if ((ARRAY_LEN) <= 0) { \
        (ARRAY_LEN) = 0; \
        (ARRAY) = NULL; \
    } else { \
 \
        (ARRAY) = (jbyte*)midpMalloc(ARRAY_LEN); \
        if ((ARRAY) == NULL) { \
            (STATUS) = OUT_OF_MEM_LEN; \
            break; \
        } \
 \
        KNI_GetRawArrayRegion((PARAM), 0, (ARRAY_LEN), (jbyte*)(ARRAY)); \
    } \
}

#define GET_STRING_ARRAY_PARAM(NUM,PARAM,ARRAY,ARRAY_LEN,TEMP,STATUS) { \
    KNI_GetParameterAsObject(NUM, PARAM); \
    (ARRAY_LEN) = (int)KNI_GetArrayLength(PARAM); \
 \
    if ((ARRAY_LEN) <= 0) { \
        (ARRAY_LEN) = 0; \
        (ARRAY) = NULL; \
    } else { \
 \
        (ARRAY) = alloc_pcsl_string_list(ARRAY_LEN); \
        if ((ARRAY) == NULL) { \
            (STATUS) = OUT_OF_MEM_LEN; \
            break; \
        } \
 \
        { \
            int i; \
            pcsl_string_status tmp; \
 \
            for (i = 0; i < (ARRAY_LEN); i++) { \
                KNI_GetObjectArrayElement((PARAM), i, (TEMP)); \
                tmp = midp_jstring_to_pcsl_string(TEMP, (ARRAY) + i); \
                if (PCSL_STRING_OK != tmp) { \
                    if (PCSL_STRING_ENOMEM == tmp) { \
                        (STATUS) = OUT_OF_MEM_LEN; \
                    } else { \
                        (STATUS) = BAD_PARAMS; \
                    } \
                    break; \
                } else if (pcsl_string_length((ARRAY) + i) <= 0) { \
                    (STATUS) = STATUS_FOR_NULL_OR_EMPTY_STRING; \
                    break; \
                } \
            } \
 \
            if (i != (ARRAY_LEN)) { \
                break; \
            } \
        } \
    } \
}

#endif /*_SUITESTORE_KNI_UTIL_H*/
