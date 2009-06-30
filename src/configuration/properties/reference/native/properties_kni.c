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
 *
 */

#include <string.h>
#include <ctype.h>

#include <kni.h>

#include <midpError.h>
#include <midpMalloc.h>
#include <midp_properties_port.h>

/**
 * @file
 */

/**
 * Converts an array of Java chars (16-bit) to an array of
 * C chars (8-bit).
 * <p>
 * <b>NOTE:<b> It is the responsibility of the caller to
 * de-allocate the value returned.
 *
 * @param uString The array of Java characters
 * @param length The length of the Java character array
 *
 * @return The converted C string upon success, otherwise
 *         <tt>NULL<tt>
 */
static char*
UnicodeToCString(jchar* uString, int length) {
    int        i;
    char*      cString;

    if (NULL == uString) {
        return NULL;
    }

    /* Add 1 for null terminator */
    cString = (char*)midpMalloc(length + 1);
    if (NULL == cString) {
        return NULL;
    }

    for (i = 0; i < length; i++) {
        cString[i] = (char)uString[i];
    }

    cString[length] = '\0';                /* Null-terminate C string */
    return cString;
}

/**
 * Gets the value of the specified property key in the internal
 * property set. If the key is not found in the internal property
 * set, the application property set is then searched.
 * <p>
 * Java declaration:
 * <pre>
 *     getProperty0(Ljava.lang.String;)Ljava.lang.String;
 * <pre>
 *
 * @param key The key to search for
 *
 * @return The value associated with <tt>key<tt> if found, otherwise
 *         <tt>null<tt>
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_midp_main_Configuration_getProperty0) {
    jchar* uStr;
    const char* key;
    const char* value;
    int strLen;

    KNI_StartHandles(2);
    KNI_DeclareHandle(str);
    KNI_DeclareHandle(result);

    KNI_GetParameterAsObject(1, str);
    strLen = KNI_GetStringLength(str);

    if (strLen <= 0 || (uStr = (jchar*) midpMalloc(strLen * sizeof(jchar))) == NULL) {
        KNI_ThrowNew(midpOutOfMemoryError, NULL);
    } else {
        KNI_GetStringRegion(str, 0, strLen, uStr);
        key = UnicodeToCString(uStr, strLen);
        midpFree(uStr);

        /* Look up the property value */
        value = getInternalProperty(key);
        midpFree((void *)key);

        if (value != NULL) {
            KNI_NewStringUTF(value, result);
        } else {
            KNI_ReleaseHandle(result);
        }
    }
    KNI_EndHandlesAndReturnObject(result);
}
