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
#include <string.h>

#include <jvm.h>
#include <kni.h>
#include <sni.h>
#include <midpMalloc.h>
#include <midpDataHash.h>
#include <midpError.h>
#include <midpUtilKni.h>
#include <commonKNIMacros.h>

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
KNIDECL(com_sun_midp_main_MIDletSuiteVerifier_getJarHash) {
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

/**
 * Disable or enable class verifier for the current VM/Isolate in
 * SVM/MVM modes correspondingly
 * <p>
 * Java declaration:
 * <pre>
 *     useClassVerifier(Z)
 * </pre>
 *
 * @param verifier true to enable, false to disable verifier
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_main_MIDletSuiteVerifier_useClassVerifier) {
    jboolean enable = KNI_GetParameterAsBoolean(1);
    JVM_SetUseVerifier(enable); 
}

/**
 * Compare hash value of the JAR with provided hash value.
 * <p>
 * Java declaration:
 * <pre>
 *     checkJarHash(Ljava/lang/String;[B)Z
 * </pre>
 *
 * @param jarPath path to JAR file
 * @param hashValue hash value to compare with
 * @return true if JAR has hash value equal to the provided one,
 *   otherwise false
 * @throws IOException
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_midp_main_MIDletSuiteVerifier_checkJarHash) {
    unsigned char *jar_hash = NULL;
    int jar_hash_len = 0;
    jboolean res = KNI_FALSE;
    int status;

    KNI_StartHandles(2);
    KNI_DeclareHandle(hashValue);
    KNI_GetParameterAsObject(2, hashValue);
    GET_PARAMETER_AS_PCSL_STRING(1, jar_path) {
        status = midp_get_file_hash(&jar_path, &jar_hash, &jar_hash_len);
        if (status == MIDP_HASH_OK) {
            unsigned char *hash = (unsigned char *)JavaByteArray(hashValue);
            int hash_len = KNI_GetArrayLength(hashValue);
            if (hash_len == jar_hash_len) {
                res = (memcmp(jar_hash, hash, hash_len) == 0);
            }
            midpFree(jar_hash);
        } else if (status == MIDP_HASH_IO_ERROR) {
            KNI_ThrowNew(midpIOException, NULL);
        } else {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
        }
    } RELEASE_PCSL_STRING_PARAMETER;

    KNI_EndHandles();
    KNI_ReturnBoolean(res);
}
