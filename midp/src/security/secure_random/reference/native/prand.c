
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

#include <kni.h>
#include <secure_random_port.h>
#include <pcsl_memory.h>
#include <midpError.h>

/**
 * Perform a platform-defined procedure for obtaining random bytes and
 * store the obtained bytes into b, starting from index 0.
 * (see IETF RFC 1750, Randomness Recommendations for Security,
 *  http://www.ietf.org/rfc/rfc1750.txt)
 * @param b array that receives random bytes
 * @param nbytes the number of random bytes to receive, must not be less than size of b
 * @return the number of actually obtained random bytes, -1 in case of an error
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_midp_crypto_PRand_getRandomBytes) {
    jint size;
    jboolean res = KNI_FALSE;
    unsigned char* buffer;

    KNI_StartHandles(1);
    KNI_DeclareHandle(hBytes);
    KNI_GetParameterAsObject(1, hBytes);

    size = KNI_GetParameterAsInt(2);

    buffer = pcsl_mem_malloc(size);
    if (0 == buffer) {
        KNI_ThrowNew(midpOutOfMemoryError, NULL);
    } else {
        int i;
        res = get_random_bytes_port(buffer, size);
        for(i=0; i<size; i++) {
            KNI_SetByteArrayElement(hBytes,i,(jbyte)buffer[i]);
        }
        pcsl_mem_free(buffer);
    }
    
    KNI_EndHandles();
    KNI_ReturnBoolean(res);
}

