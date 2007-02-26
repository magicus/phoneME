/*
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

#include <windows.h>
#include <stdio.h>
#include "nams.h"
#include "javacall_logging.h"
#include "javautil_jad_parser.h"
#include "javautil_string.h"
#include "javacall_memory.h"
#include "javacall_dir.h"
#include "javacall_lcd.h"
#include "javacall_nams.h"

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
void javacall_ams_midlet_stateChanged(javacall_midlet_state state,
                                      const javacall_app_id appID,
                                      javacall_change_reason reason) {
    int appIndex = 0;

    switch (state) {
        case JAVACALL_MIDLET_STATE_PAUSED:
            if (nams_if_midlet_exist(appID) != JAVACALL_OK) {
                /* New started midlet */
                if (nams_add_midlet(appID) != JAVACALL_OK) {
                    javacall_print("[NAMS] Add midlet failed!\n");
                }
            } else {
                javacall_print("[NAMS] Midlet state change to paused\n");
            }
            break;
        case JAVACALL_MIDLET_STATE_DESTROYED:
            if (nams_remove_midlet(appID) != JAVACALL_OK) {
                javacall_print("[NAMS] Midlet can't be removed!\n");
            }
            return;
        case JAVACALL_MIDLET_STATE_ERROR:
            javacall_print("[NAMS] Midlet state error!\n");
            break;
        default:
            break;
    }

    if (nams_set_midlet_state(state, appID, reason) != JAVACALL_OK) {
        javacall_print("[NAMS] Specified midlet's state can't be changed!\n");
    }

    if (nams_find_midlet_by_state(JAVACALL_MIDLET_STATE_FOREGROUND, &appIndex)
        != JAVACALL_OK) {
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
 *        <tt>JAVACALL_MIDLET_STATE_FOREGROUND</tt> or
 *        <tt>JAVACALL_MIDLET_STATE_BACKGROUND</tt>
 * @param appID The ID of the state-changed suite
 * @param reason The reason why the state change has happened
 */
void javacall_ams_ui_stateChanged(javacall_midlet_ui_state state,
                                  const javacall_app_id appID,
                                  javacall_change_reason reason) {
    int appIndex = 0;

    switch (state) {
        case JAVACALL_MIDLET_STATE_FOREGROUND:
            javacall_print("[NAMS] Midlet state change to foreground\n");
            break;
        case JAVACALL_MIDLET_STATE_BACKGROUND:
            javacall_print("[NAMS] Midlet state change to background\n");
            break;
        default:
            break;
    }

    if (nams_set_midlet_state(state, appID, reason) != JAVACALL_OK) {
        javacall_print("[NAMS] Specified midlet's state can't be changed!\n");
    }

    if (nams_find_midlet_by_state(JAVACALL_MIDLET_STATE_FOREGROUND, &appIndex)
        != JAVACALL_OK) {
        /* There is no midlet at fore ground, refresh the screen to blank */
        /* javacall_ams_refresh_lcd(); */
    }
}

/**
 * Inform NAMS that specific MIDlet suite wants to be FOREGROUND.
 *
 * @param appID unique ID of the runtime MIDlet 
 */
void javacall_ams_midlet_requestForeground(const javacall_app_id appID) {
    nams_set_midlet_request_foreground(appID);
}

/**
 * Get path name of the directory which holds suite's RMS files.
 * @param suiteID Unique ID of the MIDlet suite
 * @param szPath  A buffer allocated to contain the returned path name string.
 *                The returned string must be double-'\0' terminated.
 * @param maxPath Buffer length of szPath
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result javacall_ams_getRMSPath(javacall_suite_id suiteID, 
                                        javacall_utf16_string szPath, 
                                        int maxPath) {
    javacall_utf16 path[JAVACALL_MAX_FILE_NAME_LENGTH*2];
    int len;

    if (nams_db_get_suite_home(suiteID, path, &len) != JAVACALL_OK) {
        return JAVACALL_FAIL;
    }

    if (len > maxPath) {
        return JAVACALL_FAIL;
    }

    memcpy(szPath, path, len);

    return JAVACALL_OK;
}

/**
 * Get domain information of the suite.
 * @param suiteID Unique ID of the MIDlet suite
 * @param domain Pointer to a javacall_ext_ams_domain to contain returned
 *                domain information. Only Trusted or Untrusted domain is
 *                required to be returned.
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result javacall_ams_getDomain(javacall_suite_id suiteID,
                                       javacall_ams_domain* domain) {
    if (nams_get_midlet_domain(suiteID, domain) != JAVACALL_OK) {
        return JAVACALL_FAIL;
    }
    return JAVACALL_OK;
}

/**
 * Get permission set of the suite.
 * @param suiteID      Unique ID of the MIDlet suite
 * @param permissions  Pointer to a javacall_ext_ams_permission_set structure
 *                     to contain returned permission setttings
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result
javacall_ams_getPermissions(javacall_suite_id suiteID,
                            javacall_ams_permission_set* permissions) {
    if (nams_get_midlet_permissions(suiteID, permissions) != JAVACALL_OK) {
        return JAVACALL_FAIL;
    }
    return JAVACALL_OK;
}

/**
 * Set single permission of the suite when user changed it.
 * @param suiteID     Unique ID of the MIDlet suite
 * @param permission  permission be set
 * @param value       new value of permssion
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result
javacall_ams_setPermission(javacall_suite_id suiteID,
                           javacall_ams_permission permission,
                           javacall_ams_permission_val value) {
    if (nams_set_midlet_permission(suiteID, permission, value) != JAVACALL_OK) {
        return JAVACALL_FAIL;
    }
    return JAVACALL_OK;
}

/**
 * Set permission set of the suite.
 * @param suiteID      Unique ID of the MIDlet suite
 * @param permissions  Pointer to a javacall_ext_ams_permission_set structure
 *                     to contain returned permission setttings
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result
javacall_ams_setPermissions(javacall_suite_id suiteID,
                            javacall_ams_permission_set* permissions) {
    int i;
    for (i = JAVACALL_AMS_PERMISSION_HTTP;
            i < JAVACALL_AMS_PERMISSION_LAST; i ++) {
        if (nams_set_midlet_permission(suiteID, (javacall_ams_permission)i,
            permissions->permission[i]) != JAVACALL_OK) {
            return JAVACALL_FAIL;
        }
    }
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
javacall_ams_getSuiteProperty(const javacall_suite_id suiteID,
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
        javacall_print("[NAMS] javacall_ams_getSuiteProperty "
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
 * @param suiteName   Suite name
 * @param suiteID     return suiteID
 *
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result
javacall_ams_getSuiteID(const javacall_utf16_string vendorName,
                        const javacall_utf16_string suiteName,
                        javacall_suite_id *suiteID) {
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
        javacall_print("[NAMS] javacall_ams_getSuiteID "
                       "nams_string_to_utf16 error\n");
        return JAVACALL_FAIL;
    }

    if (nams_string_to_utf16(key2, strlen(key2), &uKey2, strlen(key2)) !=
            JAVACALL_OK) {
        javacall_print("[NAMS] javacall_ams_getSuiteID "
                       "nams_string_to_utf16 error\n");
        return JAVACALL_FAIL;
    }

    WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, vendorName, -1,
                        cVendorName, MAX_VALUE_NAME_LEN, NULL, NULL);

    WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, suiteName, -1,
                        cSuiteName, MAX_VALUE_NAME_LEN, NULL, NULL);

    for (index = 1; index < MAX_MIDLET_NUM; index ++) {
        if (javacall_ams_getSuiteProperty(index, uKey1, value,
                                          MAX_VALUE_NAME_LEN) != JAVACALL_OK) {
            continue;
        }

        WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, value, -1,
                            cValue, MAX_VALUE_NAME_LEN, NULL, NULL);

        if (!javautil_string_equals(cValue, cVendorName)) {
            continue;
        }

        if (javacall_ams_getSuiteProperty(index, uKey2, value,
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
        *suiteID = index;
        break;
    }

    if (index >= MAX_MIDLET_NUM) {
        return JAVACALL_FAIL;
    }

    return JAVACALL_OK;
}

/**
 * Platform inform the VM to create the images cache.
 * @param suiteID unique ID of the MIDlet suite
 * @param jarName the Jar file name of the MIDlet suite
 *
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result
javanotify_ams_createResourceCache(const javacall_suite_id suiteID,
                                   const javacall_utf16_string jarName) {
    return JAVACALL_OK;
}

/**
 * Java invoke this function to get the image cache path.
 * @param suiteID   Unique ID of the MIDlet suite
 * @param cachePath buffer for Platform store the image cache path.
 * @param cachePathLen the length of cachePath
 *
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result
javacall_ams_getResourceCachePath(const javacall_suite_id suiteID,
                                  javacall_utf16_string cachePath,
                                  int cachePathLen) {
    javacall_result res;
    int pathLen;

    res = nams_db_get_suite_home(suiteID, cachePath, &pathLen);
    if (res != JAVACALL_OK || pathLen > cachePathLen) {
        res = JAVACALL_FAIL;
    }

    return JAVACALL_OK;
}
