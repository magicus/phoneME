/*
 *
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
#include <midpUtilKni.h>
#include <midpError.h>

#if ENABLE_IMAGE_CACHE
#include <imageCache.h>
#endif

/**
 * Loads image data of a suite icon from the image cache.
 * <p>
 * Java declaration:
 * <pre>
 *     byte[] loadCachedIcon(String suiteId, String iconName);
 * </pre>
 *
 * @param suiteId the suite Id
 * @param iconName the name of the image resource
 * @return Java byte[] object with icon data,
 *     or NULL if the data wasn't found in the cache
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_midp_midletsuite_MIDletSuiteInfo_loadCachedIcon) {
#if ENABLE_IMAGE_CACHE
    SuiteIdType suiteID;
    int length;
    unsigned char *buffer = NULL;

    KNI_StartHandles(2);
    KNI_DeclareHandle(iconBytesArray);
    suiteID = KNI_GetParameterAsInt(1);

    GET_PARAMETER_AS_PCSL_STRING(2, iconName)
    length = loadImageFromCache(suiteID, &iconName, &buffer);
    if (length != -1 && buffer != NULL) {

        // Create byte array object to return as result
        SNI_NewArray(SNI_BYTE_ARRAY, length, iconBytesArray);
        if (KNI_IsNullHandle(iconBytesArray)) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
        } else {
            KNI_SetRawArrayRegion(iconBytesArray, 0,
                length, (jbyte *)buffer);
        }
        midpFree(buffer);
    }
    RELEASE_PCSL_STRING_PARAMETER

    KNI_EndHandlesAndReturnObject(iconBytesArray);
#else
    return NULL;
#endif
}
