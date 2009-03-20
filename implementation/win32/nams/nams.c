/*
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

#include <windows.h>
#include <stdio.h>
#include "nams.h"
#include "javacall_logging.h"
#include "javautil_jad_parser.h"
#include "javautil_string.h"
#include "javacall_memory.h"
#include "javacall_dir.h"
#include "javacall_lcd.h"
#include "javacall_ams_app_manager.h"
#include "javacall_ams_platform.h"

/**
 * Inform on completion of the previously requested operation.
 *
 * @param operation code of the completed operation
 * @param appID The ID used to identify the application
 * @param pResult Pointer to a static buffer containing
 *                operation-dependent result
 */
void javacall_ams_operation_completed(javacall_opcode operation,
                                      javacall_app_id appID,
                                      void* pResult) {
    (void)operation;
    (void)appID;
    (void)pResult;

    javacall_print("[NAMS] Operation completed.\n");
}

/**
 * Java invokes this function to inform the platform on change of the system
 * lifecycle status.
 *
 * @param state new state of the running Java system. Can be either,
 *        <tt>JAVACALL_SYSTEM_STATE_ACTIVE</tt>
 *        <tt>JAVACALL_SYSTEM_STATE_SUSPENDED</tt>
 *        <tt>JAVACALL_SYSTEM_STATE_STOPPED</tt>
 *        <tt>JAVACALL_SYSTEM_STATE_ERROR</tt>
 */
void javacall_ams_system_state_changed(javacall_system_state state) {
}

/**
 * Inform on change of the specific MIDlet's lifecycle status.
 *
 * Java will invoke this function whenever the lifecycle status of the running
 * MIDlet is changed, for example when the running MIDlet has been paused,
 * resumed, the MIDlet has shut down etc.
 * 
 * @param state new state of the running MIDlet. Can be either,
 *        <tt>JAVACALL_MIDLET_STATE_ACTIVE</tt>
 *        <tt>JAVACALL_MIDLET_STATE_PAUSED</tt>
 *        <tt>JAVACALL_MIDLET_STATE_DESTROYED</tt>
 *        <tt>JAVACALL_MIDLET_STATE_ERROR</tt>
 * @param appID The ID of the state-changed suite
 * @param reason The reason why the state change has happened
 */
void javacall_ams_midlet_state_changed(javacall_lifecycle_state state,
                                       javacall_app_id appID,
                                       javacall_change_reason reason) {
    int appIndex = 0;

    switch (state) {
        case JAVACALL_LIFECYCLE_MIDLET_PAUSED:
            if (nams_if_midlet_exist(appID) != JAVACALL_OK) {
                /* New started midlet */
                if (nams_add_midlet(appID) != JAVACALL_OK) {
                    javacall_print("[NAMS] Add midlet failed!\n");
                }
            } else {
                javacall_print("[NAMS] Midlet state change to paused\n");
            }
            break;
        case JAVACALL_LIFECYCLE_MIDLET_SHUTDOWN:
            if (nams_remove_midlet(appID) != JAVACALL_OK) {
                javacall_print("[NAMS] Midlet can't be removed!\n");
            }
            return;
        case JAVACALL_LIFECYCLE_MIDLET_ERROR:
            javacall_print("[NAMS] Midlet state error!\n");
            break;
        default:
            break;
    }

    if (nams_set_midlet_state(state, appID, reason) != JAVACALL_OK) {
        javacall_print("[NAMS] Specified midlet's state can't be changed!\n");
    }

    if (nams_find_midlet_by_state(JAVACALL_MIDLET_UI_STATE_FOREGROUND,
            &appIndex) != JAVACALL_OK) {
        /* There is no midlet at fore ground, refresh the screen to blank */
        /* javacall_ams_refresh_lcd(); */
    }
}

/**
 * Inform on change of the specific MIDlet's lifecycle status.
 *
 * Java will invoke this function whenever the running MIDlet has switched
 * to foreground or background.
 *
 * @param state new state of the running MIDlet. Can be either
 *        <tt>JAVACALL_MIDLET_STATE_UI_FOREGROUND</tt>,
 *        <tt>JAVACALL_MIDLET_STATE_UI_BACKGROUND</tt>,
 *        <tt>JAVACALL_MIDLET_STATE_UI_FOREGROUND_REQUEST</tt>,
 *        <tt>JAVACALL_MIDLET_STATE_UI_BACKGROUND_REQUEST</tt>.
 * @param appID The ID of the state-changed suite
 * @param reason The reason why the state change has happened
 */
void javacall_ams_midlet_ui_state_changed(javacall_midlet_ui_state state,
                                          javacall_app_id appID,
                                          javacall_change_reason reason) {
    int appIndex = 0;

    switch (state) {
        case JAVACALL_MIDLET_UI_STATE_FOREGROUND:
            javacall_print("[NAMS] Midlet state change to foreground\n");
            break;
        case JAVACALL_MIDLET_UI_STATE_BACKGROUND:
            javacall_print("[NAMS] Midlet state change to background\n");
            break;
        case JAVACALL_MIDLET_UI_STATE_FOREGROUND_REQUEST:
            javacall_print("[NAMS] Midlet is requesting foreground\n");
            nams_set_midlet_request_foreground(appID);
            break;
        case JAVACALL_MIDLET_UI_STATE_BACKGROUND_REQUEST:
            javacall_print("[NAMS] Midlet is requesting background\n");
            break;
        default:
            break;
    }

    if (nams_set_midlet_state(state, appID, reason) != JAVACALL_OK) {
        javacall_print("[NAMS] Specified midlet's state can't be changed!\n");
    }

    if (nams_find_midlet_by_state(JAVACALL_MIDLET_UI_STATE_FOREGROUND,
            &appIndex) != JAVACALL_OK) {
        /* There is no midlet at fore ground, refresh the screen to blank */
        /* javacall_ams_refresh_lcd(); */
    }
}

/**
 * Java invokes this function to get path name of the directory which
 * holds the suite's RMS files.
 *
 * The Platform must provide an implementation of this function if the
 * RMS is on the MIDP side.
 *
 * Note that memory for the parameter pRmsPath is allocated
 * by the callee. The caller is responsible for freeing it
 * using javacall_free().
 *
 * @param suiteId  [in]  unique ID of the MIDlet suite
 * @param pRmsPath [out] a place to hold a pointer to the buffer containing
 *                       the returned RMS path string.
 *                       The returned string must be double-'\0' terminated.
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result javacall_ams_get_rms_path(javacall_suite_id suiteID,
                                          javacall_utf16_string* pRmsPath) {
#if 1
    javacall_utf16 path[JAVACALL_MAX_FILE_NAME_LENGTH*2];
    int len;

    if (nams_db_get_suite_home(suiteID, path, &len) != JAVACALL_OK) {
        return JAVACALL_FAIL;
    }

    pRmsPath = javacall_malloc(len);
    if (pRmsPath == NULL) {
        return JAVACALL_FAIL;
    }

    memcpy(pRmsPath, path, len);

    return JAVACALL_OK;
#else
    return JAVACALL_FAIL;
#endif
}

/**
 * Get domain information of the suite.
 * @param suiteID Unique ID of the MIDlet suite.
 * @param pDomain Pointer to a javacall_ext_ams_domain to contain returned
 *                domain information. Only Trusted or Untrusted domain is
 *                required to be returned.
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result javacall_ams_get_domain(javacall_suite_id suiteID,
                                        javacall_ams_domain* pDomain) {
    if (nams_get_midlet_domain(suiteID, pDomain) != JAVACALL_OK) {
        return JAVACALL_FAIL;
    }
    return JAVACALL_OK;
}

//#if !ENABLE_NATIVE_AMS_UI

/**
 * App Manager invokes this function to get permissions of the suite.
 *
 * Note: memory for pPermissions array is allocated and freed by the caller.
 *
 * @param suiteId       [in]     unique ID of the MIDlet suite
 * @param pPermissions  [in/out] array of JAVACALL_AMS_NUMBER_OF_PERMISSIONS
 *                               elements that will be filled with the
 *                               current permissions' values on exit
 * @return <tt>JAVACALL_OK</tt> on success, an error code otherwise
 */
javacall_result
javacall_ams_suite_get_permissions(javacall_suite_id suiteID,
                                   javacall_ams_permission_val* pPermissions) {
    if (nams_get_midlet_permissions(suiteID, pPermissions) != JAVACALL_OK) {
        return JAVACALL_FAIL;
    }
    return JAVACALL_OK;
}

/**
 * Set single permission of the suite when user changed it.
 * @param suiteID     unique ID of the MIDlet suite
 * @param permission  permission be set
 * @param value       new value of permssion
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result
javacall_ams_set_permission(javacall_suite_id suiteID,
                            javacall_ams_permission permission,
                            javacall_ams_permission_val value) {
    if (nams_set_midlet_permission(suiteID, permission, value) != JAVACALL_OK) {
        return JAVACALL_FAIL;
    }
    return JAVACALL_OK;
}

/**
 * App Manager invokes this function to set permissions of the suite.
 *
 * @param suiteId       [in]  unique ID of the MIDlet suite
 * @param pPermissions  [in]  array of JAVACALL_AMS_NUMBER_OF_PERMISSIONS
 *                            elements containing the permissions' values
 *                            to be set
 *
 * @return <tt>JAVACALL_OK</tt> on success, an error code otherwise
 */
javacall_result
javacall_ams_suite_set_permissions(javacall_suite_id suiteID,
                                   javacall_ams_permission_val* pPermissions) {
    int i;
    for (i = JAVACALL_AMS_PERMISSION_HTTP;
            i < JAVACALL_AMS_PERMISSION_LAST; i ++) {
        if (nams_set_midlet_permission(suiteID, (javacall_ams_permission)i,
                                       pPermissions[i]) != JAVACALL_OK) {
            return JAVACALL_FAIL;
        }
    }
    return JAVACALL_OK;
}

//#endif /* ENABLE_NATIVE_AMS_UI */

/**
 * This function is called by the installer when some action is required
 * from the user.
 *
 * It must be implemented at that side (Java or Platform) where the
 * application manager is located.
 *
 * After processing the request, javanotify_ams_install_answer() must
 * be called to report the result to the installer.
 *
 * @param requestCode   identifies the requested action
 *                      in pair with pInstallState->appId uniquely
 *                      identifies this request
 * @param pInstallState pointer to a structure containing all information
 *                      about the current installation state
 * @param pRequestData  pointer to request-specific data (may be NULL)
 *
 * @return <tt>JAVACALL_OK</tt> if handling of the request was started
 *                              successfully,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
javacall_ams_install_ask(javacall_ams_install_request_code requestCode,
                         const javacall_ams_install_state* pInstallState,
                         const javacall_ams_install_data* pRequestData) {
    javacall_print("[NAMS] javacall_ams_install_ask()\n");

    return JAVACALL_OK;
}

/**
 * Get specified property value of the suite.
 * @param suiteID   Unique ID of the MIDlet suite
 * @param key       Property name
 * @param value     Buffer to conatain returned property value
 * @param maxValue  Buffern length of value
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result
javacall_ams_get_suite_property(const javacall_suite_id suiteID,
                                const javacall_utf16_string key,
                                javacall_utf16_string value,
                                int maxValue) {
    int   propsNum=0;
    int   i;
    int   jadLineSize;
    int   index;
    int   len;
    long  jadSize;
    char* jad;
    char* jadBuffer;
    char* jadLine;
    char* jadPropName;
    char* jadPropValue;
    char  propKey[JAVACALL_MAX_FILE_NAME_LENGTH];
    javacall_result res;
    javacall_result ret=JAVACALL_FAIL;
    javacall_utf16*  uJad;

    if (nams_get_midlet_jadpath(suiteID, &jad) != JAVACALL_OK) {
        javacall_print("[NAMS] Can not find JAD file!\n");
        return JAVACALL_FAIL;
    }

    if (nams_string_to_utf16(jad, strlen(jad), &uJad, strlen(jad)) !=
            JAVACALL_OK) {
        javacall_print("[NAMS] javacall_ams_get_suite_property "
                       "nams_string_to_utf16 error\n");
        return JAVACALL_FAIL;
    }

    jadSize = javautil_read_jad_file(uJad, strlen(jad), &jadBuffer);
    javacall_free(uJad);

    if (jadSize <= 0 || jadBuffer == NULL) {
        return JAVACALL_FAIL;
    }

    res = javautil_get_number_of_properties(jadBuffer, &propsNum);
    if ((res != JAVACALL_OK) || (propsNum <= 0)) {
        return JAVACALL_OUT_OF_MEMORY;
    }

    for (i=0; i<propsNum; i++) {
        /* read a line from the jad file. */
        res = javautil_read_jad_line(&jadBuffer, &jadLine, &jadLineSize);
        if (res == JAVACALL_END_OF_FILE) {
            break;
        }  else if ((res != JAVACALL_OK) || (jadLine == NULL) ||
                (jadLineSize == 0)) {
            return JAVACALL_FAIL;
        }

        /* find the index of ':' */
        res = javautil_string_index_of(jadLine, ':', &index);
        if ((res != JAVACALL_OK) || (index <= 0)) {
            javacall_free(jadLine);
            continue;
        }

        /* get sub string of jad property name */
        res = javautil_string_substring(jadLine, 0, index, &jadPropName);
        if (res == JAVACALL_OUT_OF_MEMORY) {
            javacall_free(jadLine);
            return res;
        }

        if ((res != JAVACALL_OK) || (jadPropName == NULL)) {
            javacall_free(jadLine);
            continue;
        }

        res = javautil_string_trim(jadPropName);
        if (res != JAVACALL_OK) {
            javacall_free(jadLine);
            javacall_free(jadPropName);
            continue;
        }

        /* check if we got the key */
        WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, key, -1,
                            propKey, JAVACALL_MAX_FILE_NAME_LENGTH,
                            NULL, NULL);
        if (!javautil_string_equals(propKey, jadPropName)) {
            continue;
        }

        /* found key, get out value */
        /* skip white space between jad property name and value */
        while (*(jadLine+index+1) == SPACE) {
            index++;
        }

        /* get sub string of jad property value */
        res = javautil_string_substring(jadLine, index+1, jadLineSize,
                                        &jadPropValue);
        if (res == JAVACALL_OUT_OF_MEMORY) {
            javacall_free(jadLine);
            javacall_free(jadPropName);
            return res;
        }
        if ((res != JAVACALL_OK) || (jadPropValue == NULL)) {
            javacall_free(jadLine);
            javacall_free(jadPropName);
            continue;
        }

        /* jad property name an value are available,
         * we can free the jad file line */
        javacall_free(jadLine);

        res = javautil_string_trim(jadPropValue);
        if (res != JAVACALL_OK) {
            javacall_free(jadPropName);
            javacall_free(jadPropValue);
            continue;
        }

        len = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, jadPropValue,
                            strlen(jadPropValue), value, maxValue-1);
        if (len==0) {
            res = JAVACALL_OUT_OF_MEMORY;
            break;
        }

        value[len] = 0;
        javacall_free(jadPropName);
        javacall_free(jadPropValue);
        ret = JAVACALL_OK;
    }

    return ret;
}

/**
 * Get suite id by vendor and suite name.
 * @param vendorName  vendor name
 * @param suiteName   suite name
 * @param pSuiteID    return suiteID
 *
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result
javacall_ams_get_suite_id(const javacall_utf16_string vendorName,
                          const javacall_utf16_string suiteName,
                          javacall_suite_id *pSuiteID) {
    int index;
    char key1[] = "MIDlet-Vendor";
    char key2[] = "MIDlet-Name";
    javacall_utf16* uKey1;
    javacall_utf16* uKey2;
    javacall_utf16 value[MAX_VALUE_NAME_LEN];
    char cVendorName[MAX_VALUE_NAME_LEN];
    char cSuiteName[MAX_VALUE_NAME_LEN];
    char cValue[MAX_VALUE_NAME_LEN];

    if (nams_string_to_utf16(key1, strlen(key1), &uKey1, strlen(key1)) !=
            JAVACALL_OK) {
        javacall_print("[NAMS] javacall_ams_get_suite_id "
                       "nams_string_to_utf16 error\n");
        return JAVACALL_FAIL;
    }

    if (nams_string_to_utf16(key2, strlen(key2), &uKey2, strlen(key2)) !=
            JAVACALL_OK) {
        javacall_print("[NAMS] javacall_ams_get_suite_id "
                       "nams_string_to_utf16 error\n");
        return JAVACALL_FAIL;
    }

    WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, vendorName, -1,
                        cVendorName, MAX_VALUE_NAME_LEN, NULL, NULL);

    WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, suiteName, -1,
                        cSuiteName, MAX_VALUE_NAME_LEN, NULL, NULL);

    for (index = 1; index < MAX_MIDLET_NUM; index ++) {
        if (javacall_ams_get_suite_property(index, uKey1, value,
                MAX_VALUE_NAME_LEN) != JAVACALL_OK) {
            continue;
        }

        WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, value, -1,
                            cValue, MAX_VALUE_NAME_LEN, NULL, NULL);

        if (!javautil_string_equals(cValue, cVendorName)) {
            continue;
        }

        if (javacall_ams_get_suite_property(index, uKey2, value,
                MAX_VALUE_NAME_LEN) != JAVACALL_OK) {
            WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, value, -1,
                                cValue, MAX_VALUE_NAME_LEN, NULL, NULL);
        }

        if (!javautil_string_equals(cValue, cSuiteName)) {
            continue;
        }

        javacall_free(uKey1);
        javacall_free(uKey2);

        /* found and return */
        *pSuiteID = index;
        break;
    }

    if (index >= MAX_MIDLET_NUM) {
        return JAVACALL_FAIL;
    }

    return JAVACALL_OK;
}

/**
 * Installer invokes this function to inform the application manager about
 * the current installation progress.
 *
 * Note: when installation is completed, javacall_ams_operation_completed()
 *       will be called to report installation status; its last parameter
 *       holding a pointer to an operation-specific data will point to a
 *       javacall_ams_install_data structure.
 *
 * @param pInstallState pointer to a structure containing all information
 *                      about the current installation state
 * @param installStatus defines current installation step
 * @param currStepPercentDone percents completed (0 - 100), -1 if unknown
 * @param totalPercentDone percents completed (0 - 100), -1 if unknown
 */
void
javacall_ams_install_report_progress(
        const javacall_ams_install_state* pInstallState,
        javacall_ams_install_status installStatus,
        int currStepPercentDone, int totalPercentDone) {
    javacall_print("[NAMS] javacall_ams_install_report_progress()\n");

    (void)pInstallState;
    (void)installStatus;
    (void)currStepPercentDone;
    (void)totalPercentDone;
}

/**
 * Java invokes this function to get the image cache path.
 *
 * The Platform must always provide an implementation of this function.
 *
 * Note that memory for the parameter pCachePath is allocated
 * by the callee. The caller is responsible for freeing it
 * using javacall_free().
 *
 * @param suiteId    [in]  unique ID of the MIDlet suite
 * @param pCachePath [out] a place to hold a pointer to the buffer where
 *                         the Platform will store the image cache path
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result
javacall_ams_get_resource_cache_path(javacall_suite_id suiteId,
                                     javacall_utf16_string* pCachePath) {
#if 0
    javacall_result res;
    int pathLen;

    res = nams_db_get_suite_home(suiteId, cachePath, &pathLen);
    if (res != JAVACALL_OK || pathLen > cachePathLen) {
        res = JAVACALL_FAIL;
    }

    return JAVACALL_OK;
#else
    return JAVACALL_FAIL;
#endif
}

/**
 * This function is called by Java to inform the Platform that a Java thread
 * has thrown an exception that was not caught. In response to this function the
 * Platform must call javanotify_ams_uncaught_exception_handled() to instruct
 * Java either to terminate the MIDlet or to terminate the thread that threw
 * the exception.
 *
 * @param appId the ID used to identify the application
 * @param midletName name of the MIDlet in which the exception occured
 * @param exceptionClassName name of the class containing the method
 * @param exceptionMessage exception message
 * @param isLastThread JAVACALL_TRUE if this is the last thread of the MIDlet,
 *                     JAVACALL_FALSE otherwise
 */
void javacall_ams_uncaught_exception(javacall_app_id appId,
        javacall_const_utf16_string midletName,
        javacall_const_utf16_string exceptionClassName,
        javacall_const_utf16_string exceptionMessage,
        javacall_bool isLastThread) {

    javacall_print("[NAMS] javacall_ams_uncaught_exception()\n");

    javanotify_ams_uncaught_exception_handled(appId, JAVACALL_TERMINATE_THREAD);
}

/**
 * This function is called by Java to inform the Platform that VM fails
 * to fulfill a memory allocation request. In response to this function the
 * Platform must call javanotify_ams_out_of_memory_handled() to instruct Java
 * either to terminate the MIDlet or to terminate the thread that trown the
 * exception, or to retry the request.
 *
 * @param appId the ID used to identify the application
 * @param midletName name of the MIDlet in which the exception occured
 * @param memoryLimit in SVM mode - heap capacity, in MVM mode - memory limit
 *                    for the isolate, i.e. the max amount of heap memory that
 *                    can possibly be allocated
 * @param memoryReserved in SVM mode - heap capacity, in MVM mode - memory
 *                       reservation for the isolate, i.e. the max amount of
 *                       heap memory guaranteed to be available
 * @param memoryUsed how much memory is already allocated on behalf of this
 *                   isolate in MVM mode, or for the whole VM in SVM mode
 * @param allocSize the requested amount of memory that the VM failed
 *                  to allocate
 * @param isLastThread JAVACALL_TRUE if this is the last thread of the MIDlet,
 *                     JAVACALL_FALSE otherwise
 */
void javacall_ams_out_of_memory(javacall_app_id appId,
                                javacall_const_utf16_string midletName,
                                int memoryLimit,
                                int memoryReserved,
                                int memoryUsed,
                                int allocSize,
                                javacall_bool isLastThread) {
    javacall_print("[NAMS] javacall_ams_out_of_memory()\n");

    javanotify_ams_out_of_memory_handled(appId, JAVACALL_TERMINATE_MIDLET);
}
