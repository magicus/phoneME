/*
 * @(#)midpMidletSuiteVerifier.c	1.5 06/04/05 @(#)
 *
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

#include <kni.h>
#include <sni.h>
#include <midpMalloc.h>
#include <midpDataHash.h>
#include <midpError.h>
#include <midpUtilKni.h>

/**
 * Evaluates hash value for JAR file
 * <p>
 * Java declaration:
 * <pre>
 *     nativeGetJarHash(Ljava/lang/String;)[B
 * </pre>
 *
 * Returned hash value is byte array allocated in Java heap, so
 * no efforts to free it should be done from the caller side.
 *
 * @param jarName  Filename of the JAR file to evaluate hash value for.
 * @return         Evaluated hash value as a byte array
 * @throw          IOException if JAR is corrupt or not found,
 *                 OutOfMemoryError if out of memory occured during hash
 *                 value evaluation
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
Java_com_sun_midp_main_MIDletSuiteVerifier_getJarHash(void) {
    unsigned char *hashValue = NULL;
    int hashValueLen = 0;
    int status;
    KNI_StartHandles(2);
    KNI_DeclareHandle(hashValueArr);
    GET_PARAMETER_AS_PCSL_STRING(1, jar_path) {
        status = midp_get_file_hash(&jar_path, &hashValue, &hashValueLen);
        if (status == MIDP_HASH_OK) {
            // Create byte array object to return as result
            SNI_NewArray(SNI_BYTE_ARRAY, hashValueLen, hashValueArr);
            if (KNI_IsNullHandle(hashValueArr)) {
                KNI_ThrowNew(midpOutOfMemoryError, NULL);
            } else KNI_SetRawArrayRegion(hashValueArr, 0,
                hashValueLen, (jbyte *)hashValue);
            midpFree(hashValue);
        } else if (status == MIDP_HASH_IO_ERROR) {
            KNI_ThrowNew(midpIOException, NULL);
        } else {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
        }
    } RELEASE_PCSL_STRING_PARAMETER;
    KNI_EndHandlesAndReturnObject(hashValueArr);
}
