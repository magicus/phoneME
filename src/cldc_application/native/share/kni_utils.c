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

#include <jsrop_kni.h>
#include <javacall_defs.h>
#include <javautil_unicode.h>
#include <jsrop_memory.h>

/**
 * Create javacall_utf16_string from the specified Java platform String object.
 * The caller is responsible for freeing the created javacall_utf16_string when done.
 *
 * @param java_str pointer to the Java platform String instance. 
 * @param utf16_str address of variable to receive the javacall_utf16_string instance.
 *                  NULL if java_str is null
 * @return status of the operation
 */
javacall_result jsrop_jstring_to_utf16_string(jstring java_str,
                             javacall_utf16_string * utf16_str) {
    javacall_result res = JAVACALL_FAIL;
    if (!KNI_IsNullHandle(java_str)) {
        jsize string_length = KNI_GetStringLength(java_str);
        javacall_utf16_string str_buffer = JAVAME_MALLOC((string_length + 1) *
                                                    sizeof(javacall_utf16));
        if (str_buffer != NULL) {
            KNI_GetStringRegion(java_str, 0, string_length, (jchar *)str_buffer);
            *(str_buffer + string_length) = 0;
            *utf16_str = str_buffer;
            res = JAVACALL_OK;
        }
        else {
            res = JAVACALL_OUT_OF_MEMORY;
        }
    } else {
        *utf16_str = NULL;
        res = JAVACALL_OK;
    }
    return res;
}

/**
 * Create fill buffer of javacall_utf16 from the specified Java platform String object.
 *
 * @param java_str pointer to the Java platform String instance
 * @param utf16_str address of buffer to fill
 * @param length length of buffer to fill
 * @return status of the operation
 */
javacall_result jsrop_jstring_to_utf16(jstring java_str,
                    javacall_utf16_string utf16_str, int length) {
    javacall_result res = JAVACALL_FAIL;
    jsize string_length = KNI_GetStringLength(java_str);

    if (string_length <= length) {
        KNI_GetStringRegion(java_str, 0, string_length, (jchar *)utf16_str);
        *(utf16_str + string_length) = 0;
        res = JAVACALL_OK;
    }
    else {
        res = JAVACALL_INVALID_ARGUMENT;
    }
    return res;
}

/**
 * Create Java platform String object from the specified javacall_utf16_string.
 *
 * @param utf16_str pointer to the javacall_utf16_string instance
 * @param java_str pointer to the Java platform String instance
 * @return status of the operation
 */
javacall_result
jsrop_jstring_from_utf16_string(KNIDECLARGS const javacall_utf16_string utf16_str,
                                               jstring java_str) {
    javacall_result res = JAVACALL_FAIL;
    javacall_int32 string_length;
    
    if (JAVACALL_OK == javautil_unicode_utf16_ulength(utf16_str, 
                                                    &string_length)) {
        KNI_NewString(utf16_str, string_length, java_str);
        res = JAVACALL_OK;
    }
    return res;
}

/**
 * Create Java platform String object from the specified javacall_utf16_string.
 * exactly utf16_chars_n characters are copied
 * utf16_str can contain any number of zero characters in it
 *
 * @param utf16_str pointer to the javacall_utf16_string instance
 * @param utf16_chars_n number of utf16 characters to copy.. 
 * @param java_str pointer to the Java platform String instance
 * @return status of the operation
 */
javacall_result
jsrop_jstring_from_utf16_string_n(KNIDECLARGS const javacall_utf16_string utf16_str, int utf16_chars_n,
						 jstring java_str) {
    KNI_NewString(utf16_str, utf16_chars_n, java_str);
    return JAVACALL_OK;
}
