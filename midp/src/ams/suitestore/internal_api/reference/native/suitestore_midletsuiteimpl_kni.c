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

#include <string.h>
#include <kni.h>

#include <midpMalloc.h>
#include <midpError.h>
#include <midpUtilKni.h>
#include <pcsl_string.h>

#include <suitestore_locks.h>
#include <suitestore_intern.h>

#if ENABLE_JSR_82
#include "btPush.h"
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

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_midletsuite_MIDletSuiteImpl_lockMIDletSuite) {
    jboolean isUpdate;
    SuiteIdType suiteId;
    int status;

    suiteId  = KNI_GetParameterAsInt(1);
    isUpdate = KNI_GetParameterAsBoolean(2);
    /* Throw an exception here if we have an error */
    status = lock_storage(suiteId, isUpdate);
    if (status < 0) {
        if (status == OUT_OF_MEM_LEN) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
        } else if (status == SUITE_LOCKED) {
            KNI_ThrowNew(midletsuiteLocked, NULL);
        }
    }
    KNI_ReturnVoid();
}

/**
 * Native method void unlockMIDletSuite(int) of
 * com.sun.midp.midletsuite.MIDletSuiteImpl.
 * <p>
 *
 * unlocks the MIDletSuite
 *
 * @param suiteId  ID of the suite
 *
 */

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_midletsuite_MIDletSuiteImpl_unlockMIDletSuite) {
    SuiteIdType suiteId = KNI_GetParameterAsInt(1);
    unlock_storage(suiteId);
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
KNIDECL(com_sun_midp_midletsuite_MIDletSuiteImpl_finalize) {
    SuiteIdType suiteId;
    jboolean locked;

    KNI_StartHandles(2);
    KNI_DeclareHandle(object);
    KNI_DeclareHandle(clazz);

    KNI_GetThisPointer(object);

    KNI_GetObjectClass(object, clazz);

    locked = KNI_GetBooleanField(object,
                                 midp_get_field_id(KNIPASSARGS clazz, "locked", "Z"));

    if (locked) {
        suiteId = KNI_GetIntField(object,
                                  midp_get_field_id(KNIPASSARGS clazz, "id", "I"));
        unlock_storage(suiteId);
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

#if ENABLE_JSR_82
/**
 * Checks is the input connection is bluetooth.
 *
 * @param conn input connection string
 *
 * @return true when input connection is bluetooth
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_midp_midletsuite_MIDletSuiteImpl_isBluetoothUrl0) {
    int retValue;
    MidpString wsUrl;
    char *szUrl;
    KNI_StartHandles(1);
    KNI_DeclareHandle(conn);
    KNI_GetParameterAsObject(1, conn);
    wsUrl = midpNewString(conn);
    szUrl = midpJcharsToChars(wsUrl);
    retValue = bt_is_bluetooth_url(szUrl);
    midpFree(szUrl);
    MIDP_FREE_STRING(wsUrl);
    KNI_EndHandles();
    KNI_ReturnBoolean(retValue);
}

/**
 * Gets bluetooth address from last incomming push connection.
 *
 * @return bluetooth address
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(com_sun_midp_midletsuite_MIDletSuiteImpl_getInpAddr0) {
    unsigned char address[2 * BT_ADDRESS_SIZE + 1];
    getFriendlyAddr(getInpAddress(), address);
    KNI_StartHandles(1);
    KNI_DeclareHandle(stringHandle);
    KNI_NewStringUTF(address, stringHandle);
    KNI_EndHandlesAndReturnObject(stringHandle);
}
#endif
