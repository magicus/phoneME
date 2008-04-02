/*
 *  
 *
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

#include <midpport_security.h>
#include <midp_thread.h>
#include <midpError.h>
#include <midpString.h>
#include <midpUtilKni.h>
#include <midpServices.h>

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
    pcsl_string v_suite = PCSL_STRING_NULL_INITIALIZER;
    pcsl_string v_permission = PCSL_STRING_NULL_INITIALIZER;
    pcsl_string *const suite = &v_suite;
    pcsl_string *const permission = &v_permission;
    jboolean granted = KNI_FALSE;
    MidpReentryData* info = (MidpReentryData*)SNI_GetReentryData(NULL);

    if (!isListenerRegistered) {
        midpport_security_set_permission_listener(midpPermissionListener);
        isListenerRegistered = 1;
    }

    if (info == NULL) {
        /* initial invocation: send request */
        int gotStrings = 0;
        KNI_StartHandles(2);
        /* IMPL NOTE: mg: is this a pattern? Do we need a macro?*/
        KNI_DeclareHandle(jSuite);
        KNI_DeclareHandle(jPermission);
        KNI_GetParameterAsObject(1, jSuite);
        KNI_GetParameterAsObject(2, jPermission);
        if(PCSL_STRING_OK == midp_jstring_to_pcsl_string(jSuite, suite)) {
            if(PCSL_STRING_OK == midp_jstring_to_pcsl_string(jPermission, permission)) {
                gotStrings = 1;
            }
        }
        KNI_EndHandles();

        if (!gotStrings) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
        } else {
          GET_PCSL_STRING_DATA_AND_LENGTH(suite)
          GET_PCSL_STRING_DATA_AND_LENGTH(permission)
          if (  (NULL != suite_data || pcsl_string_is_null(suite))
            &&  (NULL != permission_data || pcsl_string_is_null(permission))
             ) {
                jint requestHandle;
                jint result = midpport_security_check_permission(
                  (jchar*)suite_data, suite_len,
                  (jchar*)permission_data, permission_len,
                  &requestHandle);

                if (result == 1) {
                  granted = KNI_TRUE;
                } else if (result == -1) {
                  /* Block the caller until the security listener is called */
                  midp_thread_wait(SECURITY_CHECK_SIGNAL, requestHandle, NULL);
                }
          /* else permission is denied */
          } else {
              KNI_ThrowNew(midpOutOfMemoryError, NULL);
          }
          RELEASE_PCSL_STRING_DATA_AND_LENGTH
          RELEASE_PCSL_STRING_DATA_AND_LENGTH
        }
    } else {
        /* reinvocation: check result */
        granted = (jboolean)(info->status);
    }

    pcsl_string_free(permission);
    pcsl_string_free(suite);

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
    pcsl_string v_suite = PCSL_STRING_NULL_INITIALIZER;
    pcsl_string v_permission = PCSL_STRING_NULL_INITIALIZER;
    pcsl_string *const suite = &v_suite;
    pcsl_string *const permission = &v_permission;
    jint status = 0;
    int gotStrings = 0;

    KNI_StartHandles(2);
    KNI_DeclareHandle(jSuite);
    KNI_DeclareHandle(jPermission);
    
    KNI_GetParameterAsObject(1, jSuite);
    KNI_GetParameterAsObject(2, jPermission);
    if(PCSL_STRING_OK == midp_jstring_to_pcsl_string(jSuite, suite)) {
        if(PCSL_STRING_OK == midp_jstring_to_pcsl_string(jPermission, permission)) {
            gotStrings = 1;
        }
    }
    KNI_EndHandles();
    
    if (!gotStrings) {
    	KNI_ThrowNew(midpOutOfMemoryError, NULL);
    } else {
          GET_PCSL_STRING_DATA_AND_LENGTH(suite)
          GET_PCSL_STRING_DATA_AND_LENGTH(permission)
          if (  (NULL != suite_data || pcsl_string_is_null(suite))
            &&  (NULL != permission_data || pcsl_string_is_null(permission))
             ) {
                status = midpport_security_check_permission_status(
                    (jchar*)suite_data, suite_len,
                    (jchar*)permission_data, permission_len);
          } else {
              KNI_ThrowNew(midpOutOfMemoryError, NULL);
          }
          RELEASE_PCSL_STRING_DATA_AND_LENGTH
          RELEASE_PCSL_STRING_DATA_AND_LENGTH
    }
    
    pcsl_string_free(permission);
    pcsl_string_free(suite);

    KNI_ReturnInt(status);
}

