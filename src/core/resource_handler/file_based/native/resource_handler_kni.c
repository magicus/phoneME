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
#include <kni.h>
#include <sni.h>

#include <midpError.h>
#include <midpUtilKni.h>
#include <pcsl_string.h>

#if !ENABLE_FILE_SYSTEM && !ENABLE_NATIVE_APP_MANAGER
#include <resources_rom.h>
#endif

/**
 * Native method int lockMIDletSuite(int) of
 * com.sun.midp.midletsuite.MIDletSuiteImpl.
 * <p>
 *
 * Locks the MIDletSuite
 *
 * @param suiteId  ID of the suite
 * @param isUpdate are we updating the suite
 *
 * @exception MIDletSuiteLockedException is thrown, if the MIDletSuite is
 * locked
 *
 * @return 0 if lock otherwise OUT_OF_MEM_LEN or SUITE_LOCKED
 */

/**
 * Native method byte[] loadRomizedResource0(String resourceName) of
 * com.sun.midp.midletsuite.MIDletSuiteImpl.
 * <p>
 *
 * Retrieves a romized resource with the given name.
 *
 * @param resourceName name of the resource to load
 *
 * @return requested resource as an array of bytes or NULL if not found
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_midp_util_ResourceHandler_loadRomizedResource0) {
#if ENABLE_FILE_SYSTEM || ENABLE_NATIVE_APP_MANAGER
    KNI_StartHandles(1);
    KNI_DeclareHandle(hReturnArray);
    KNI_ReleaseHandle(hReturnArray);
    KNI_EndHandlesAndReturnObject(hReturnArray);
#else
    int resourceSize;
    const unsigned char *pResourceName, *pBufPtr;

    KNI_StartHandles(2);
    KNI_DeclareHandle(hReturnArray);

    GET_PARAMETER_AS_PCSL_STRING(1, strResourceName)

    do {
        pResourceName = (const unsigned char*)
            pcsl_string_get_utf8_data(&strResourceName);
        if (pResourceName == NULL) {
            break;
        }

        resourceSize = ams_get_resource(pResourceName, &pBufPtr);
        
        if (resourceSize > 0) {
            /* create a byte array */
            SNI_NewArray(SNI_BYTE_ARRAY, resourceSize, hReturnArray);
            if (KNI_IsNullHandle(hReturnArray)) {
                KNI_ThrowNew(midpOutOfMemoryError, NULL);
                break;
            }

            /* copy data into the array */
            KNI_SetRawArrayRegion(hReturnArray, 0,
                resourceSize * sizeof(jbyte), (jbyte*)pBufPtr);
        }
    } while(0);

    pcsl_string_release_utf8_data((jbyte *)pResourceName, &strResourceName);

    RELEASE_PCSL_STRING_PARAMETER

    KNI_EndHandlesAndReturnObject(hReturnArray);
#endif
}
