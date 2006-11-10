/*
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

#include <string.h>
#include <kni.h>

#include <midpError.h>
#include <midpMalloc.h>
#include <suitestore_locks.h>
#include <midpUtilKni.h>
#include <pcsl_string.h>

/*
 * Get a String from a field of an object.
 *
 * returns unicode string, with a -1 length for null, and -2 for out of
 *  memory
 */
pcsl_string_status getStringField(jobject obj, jclass classObj,
                                    char* pszFieldName, jobject fieldHandle,
                                    pcsl_string * result) {

    KNI_GetObjectField(obj, midp_get_field_id(classObj, pszFieldName,
        "Ljava/lang/String;"), fieldHandle);

    return midp_jstring_to_pcsl_string(fieldHandle, result);
}

/**
 * Native method int lockMIDletSuite(String) of
 * com.sun.midp.midletsuite.MIDletSuiteImpl.
 * <p>
 *
 * Locks the MIDletSuite
 *
 * @param suiteID  ID of the suite
 * @param isUpdate are we updating the suite
 *
 * @exception MIDletSuiteLockedException is thrown, if the MIDletSuite is
 * locked
 *
 * @return 0 if lock otherwise OUT_OF_MEM_LEN or SUITE_LOCKED
 */

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_midletsuite_MIDletSuiteImpl_lockMIDletSuite() {
    jboolean isUpdate;

    KNI_StartHandles(1);

    GET_PARAMETER_AS_PCSL_STRING(1, suiteID)
    int status;
    isUpdate = KNI_GetParameterAsBoolean(2);
    /* Throw an exception here if we have an error */
    status = lock_storage(&suiteID, isUpdate);
    if (status < 0) {
        if (status == OUT_OF_MEM_LEN) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
        } else if (status == SUITE_LOCKED) {
            KNI_ThrowNew(midletsuiteLocked, NULL);
        }
    }
    RELEASE_PCSL_STRING_PARAMETER

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Native method void unlockMIDletSuite() of
 * com.sun.midp.midletsuite.MIDletSuiteImpl.
 * <p>
 *
 * unlocks the MIDletSuite
 *
 * @param suiteID  ID of the suite
 *
 */

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_midletsuite_MIDletSuiteImpl_unlockMIDletSuite() {
    KNI_StartHandles(1);

    GET_PARAMETER_AS_PCSL_STRING(1, suiteID)
    unlock_storage(&suiteID);
    RELEASE_PCSL_STRING_PARAMETER

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Native method void finalize of
 * com.sun.midp.midletsuite.MIDletSuiteImpl.
 * <p>
 *
 * native finalizer
 *
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_midletsuite_MIDletSuiteImpl_finalize() {
    pcsl_string suiteID;
    jboolean locked;

    KNI_StartHandles(3);
    KNI_DeclareHandle(object);
    KNI_DeclareHandle(clazz);
    KNI_DeclareHandle(string);

    KNI_GetThisPointer(object);

    KNI_GetObjectClass(object, clazz);

    locked = KNI_GetBooleanField(object,
				 midp_get_field_id(clazz, "locked","Z"));

    if (locked) {
        getStringField(object, clazz, "id", string, &suiteID);
        unlock_storage(&suiteID);
        pcsl_string_free(&suiteID);
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}
