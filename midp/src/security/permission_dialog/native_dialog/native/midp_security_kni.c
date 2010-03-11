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

/**
 * @file
 *
 * Implementation of Java native methods for the <tt>PushRegistryImpl</tt>
 * class.
 */
#include <string.h>

#include <kni.h>
#include <sni.h>
#include <ROMStructs.h>
#include <commonKNIMacros.h>

#include <midp_security.h>
#include <midp_thread.h>
#include <midpError.h>
#include <midpString.h>
#include <midpUtilKni.h>
#include <midpServices.h>
#include <midpMalloc.h>
#include <javacall_security.h>

/** 
 * 0 if no security permission listener has been registered.
 * 1 if otherwise.
 */
static int isListenerRegistered;

/**
 * The typedef of the security permission listener that is notified 
 * when the native security manager has permission checking result.
 *
 * @param handle  - The handle for the permission check session
 * @param granted - true if the permission is granted, false if denied.
 */
static void midpPermissionListener(jint requestHandle, jboolean granted) {
    midp_thread_signal(SECURITY_CHECK_SIGNAL, requestHandle, (int)granted);
}

/**
 * Query native security manager for permission.
 * This call may block if user needs to be asked.
 *
 * Java prototype:
 * <pre>
 * native boolean checkPermission0(String suiteId, String permission);
 * </pre>
 * @param suiteId the MIDlet suite the permission should be checked against
 * @param permission the permission id
 * @return true if permission is granted. Otherwise, false.
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_com_sun_midp_security_SecurityHandler_checkPermission0() {
    jboolean granted = KNI_FALSE;
    MidpReentryData* info = (MidpReentryData*)SNI_GetReentryData(NULL);

    if (!isListenerRegistered) {
        security_set_permission_listener(midpPermissionListener);
        isListenerRegistered = 1;
    }

    if (info == NULL) {
        /* initial invocation: send request */
        jint requestHandle;
        jint permissionLength;
        jchar* permission = NULL;
        jint suiteId = KNI_GetParameterAsInt(1);
        jint result;
        javacall_result status;
        
        KNI_StartHandles(1);
        KNI_DeclareHandle(permissionHandle);
        KNI_GetParameterAsObject(2, permissionHandle);
        permissionLength = KNI_GetStringLength(permissionHandle);
        permission = midpMalloc(permissionLength * sizeof(jchar));
        if (permission) {
            unsigned int res;
            KNI_GetStringRegion(permissionHandle, 0, permissionLength, permission);
            status = javacall_security_check_permission((javacall_suite_id)suiteId,
                                               (javacall_const_utf16_string)permission,
                                               permissionLength,
                                               JAVACALL_TRUE,
                                               &res);
            midpFree(permission);
            switch (status) {
            case JAVACALL_OK:
                result = CONVERT_PERMISSION_STATUS(res);
                break;
            case JAVACALL_WOULD_BLOCK:
                requestHandle = res;
                result = -1;
                break;
            default:
                result = 0;
                break;
            }
            if (result == 1) {
                granted = KNI_TRUE;
            } else if (result == -1) {
            /* Block the caller until the security listener is called */
                midp_thread_wait(SECURITY_CHECK_SIGNAL, requestHandle, NULL);
            }
        } else {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
        }
        KNI_EndHandles();
    } else {
        /* reinvocation: check result */
        granted = (jboolean)(info->status);
    }

    KNI_ReturnBoolean(granted);
}

/**
 * Query native security manager for permission status.
 * This call will not block asking for user's input.
 *
 * Java prototype:
 * <pre>
 * native int checkPermissionStatus0(String suiteId, String permission);
 * </pre>
 * @param suiteId the MIDlet suite the permission should be checked against
 * @param permission the permission id
 * @return 0 if the permission is denied; 1 if the permission is allowed;
 *  -1 if the status is unknown
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_com_sun_midp_security_SecurityHandler_checkPermissionStatus0()
{
    jint result = 0;
    jint suiteId = KNI_GetParameterAsInt(1);
    jint permissionLength;
    jchar* permission = NULL;
    javacall_result status;

    KNI_StartHandles(1);
    KNI_DeclareHandle(permissionHandle);
    KNI_GetParameterAsObject(2, permissionHandle);
    permissionLength = KNI_GetStringLength(permissionHandle);
    permission = midpMalloc(permissionLength * sizeof(jchar));
    if (permission) {
        unsigned int res;
        KNI_GetStringRegion(permissionHandle, 0, permissionLength, permission);

        status = javacall_security_check_permission((javacall_suite_id)suiteId,
                                                   (javacall_const_utf16_string)permission,
                                                   permissionLength,
                                                   JAVACALL_FALSE,
                                                   &res);
        midpFree(permission);
        switch (status) {
        case JAVACALL_OK:
            result = CONVERT_PERMISSION_STATUS(res);
            break;
        case JAVACALL_WOULD_BLOCK:
        /* incorrect behaviour: regardless the fact that NAMS shows user dialog,
        application need to get result immediately */
            result = -1;
            REPORT_ERROR(LC_SECURITY,
                     "javacall_security_check_permission() returns incorrect status");
            break;
        default:
            result = 0;
            break;
        }
    } else {
        KNI_ThrowNew(midpOutOfMemoryError, NULL);
    }
    KNI_EndHandles();
    KNI_ReturnInt(result);
}

