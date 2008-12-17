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
#include <sni.h>
#include <midpError.h>
#include <midpEvents.h>
#include <midp_thread.h>
#include <midpUtilKni.h>
#include <midpServices.h>
#include <string.h>

#include <suitestore_common.h>
#include <suitestore_kni_util.h>

#include <pcsl_memory.h> /* to have definition of NULL */

#include <javacall_defs.h>
#include <javacall_memory.h>
#include <javacall_ams_installer.h>
#include <javacall_ams_platform.h>

/*
 * IMPL_NOTE: this request code is not used in
 * java[call|notify]_ams_install_ask/answer
 * and is treated as other requests for handiness. If this value is changed,
 * the corresponding value in InstallerMIDletPeer class must also be changed.
 */
#define RQ_UPDATE_STATUS 6

/* IMPL_NOTE: for now we assume that only one installer may run at a time. */
int g_installerIsolateId = -1;

/**
 * These fields are set up by the javacall_ams_install_answer() callback.
 */
jboolean g_fAnswer = JAVACALL_FALSE, g_fAnswerReady = JAVACALL_FALSE;

/**
 * Extracts strings from the given java array and places them into
 * an array of javacall strings.
 *
 * Note: the caller is responsible for freeing memory allocated for
 *       javacall strings in this function.
 *       Even if the function has returned an error, *pArraySize will
 *       hold the number of entries actually allocated in the output array.
 *
 * @param javaObj    [in]  java array of strings
 * @param tmpHandle  [in]  temporary KNI handle
 * @param pArraySize [out] on exit will hold the size
 *                         of the allocated string array
 * @param ppArray    [out] on exit will hold a pointer
 *                         to the array of javacall strings
 *
 * @return JAVACALL_OK if succeeded, JAVACAL_FAIL otherwise
 */
static javacall_result
java_str_array2javacall_impl(jobject javaObj, jobject tmpHandle,
                             javacall_int32* pArraySize,
                             javacall_utf16_string** ppArray) {
    javacall_int32 tmpArrSize;
    javacall_utf16_string* pTmpArray = NULL;
    javacall_result jcRes = JAVACALL_OK;

    tmpArrSize = (javacall_int32)KNI_GetArrayLength(javaObj);

    do {
        if (tmpArrSize > 0) {
            jint i;
            pTmpArray = (javacall_utf16_string*)javacall_malloc(
                tmpArrSize * sizeof(javacall_utf16_string));
            if (pTmpArray == NULL) {
                jcRes = JAVACALL_OUT_OF_MEMORY;
                break;
            }

            for (i = 0; i < tmpArrSize; i++) {
                jsize strLen;
                
                KNI_GetObjectArrayElement(javaObj, i, tmpHandle);
                strLen = KNI_GetStringLength(tmpHandle);

                if (strLen >= 0) {
                    pTmpArray[i] = (javacall_utf16_string)
                        javacall_malloc((strLen + 1) * sizeof(jchar));
                    if (pTmpArray[i] == NULL) {
                        /*
                         * save the current number of strings in the array
                         * to free them later
                         */
                        tmpArrSize = i;
                        jcRes = JAVACALL_OUT_OF_MEMORY;
                        break;
                    }

                    if (strLen > 0) {
                        KNI_GetStringRegion(tmpHandle, 0, strLen, pTmpArray[i]);
                    } else {
                        /* empty string */
                        pTmpArray[i][0] = 0;
                    }
                } else {
                    pTmpArray[i] = NULL;
                }
            }

            if (jcRes != JAVACALL_OK) {
                break;
            }
        } else {
            tmpArrSize = 0;
            pTmpArray  = NULL;
        }
    } while (0);

    *pArraySize = tmpArrSize;
    *ppArray    = pTmpArray;

    return jcRes;
}

/**
 * Sends a request of type defined by the given request code to
 * the party that uses this installer via the native callback.
 *
 * Note: only some of parameters are used, depending on the request code
 *
 * IMPL_NOTE: the request code passed to this method as an argument must
 *            be the same as the corresponding JAVACALL_INSTALL_REQUEST_*
 *            value defined in javacall_ams_installer.h.
 *
 * @param requestCode   code of the request to the native callback
 * @param installState  current installation state
 * @param installStatus current status of the installation, -1 if not used
 * @param newLocation   new url of the resource to install; null if not used
 *
 * @return 0 if no errors, a platform-specific error code otherwise
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_installer_InstallerPeerMIDlet_sendNativeRequest0) {
    javacall_ams_install_state jcInstallState;
    javacall_ams_install_data  jcRequestData;
    javacall_result            jcRes = JAVACALL_OK;
    javacall_utf16_string      jcProperties = NULL;
    javacall_ams_install_request_code requestCode =
        (javacall_ams_install_request_code) KNI_GetParameterAsInt(1);
    pcsl_string pcslJarUrl = PCSL_STRING_NULL_INITIALIZER,
                pcslSuiteName = PCSL_STRING_NULL_INITIALIZER;
    jint propertiesLen;

    KNI_StartHandles(6);
    KNI_DeclareHandle(javaInstallState);
    KNI_DeclareHandle(javaProperties);
    KNI_DeclareHandle(javaAuthPath);
    KNI_DeclareHandle(clazz);
    KNI_DeclareHandle(tmpHandle);

    KNI_GetParameterAsObject(2, javaInstallState);

    if (KNI_IsNullHandle(javaInstallState)) {
        KNI_ReturnInt(JAVACALL_FAIL);
    }

    /* The request is not sent yet, so the answer is not received. */
    g_fAnswerReady = JAVACALL_FALSE;

    jcRequestData.installStatus =
        (javacall_ams_install_status) KNI_GetParameterAsInt(3);

    /*
     * Converting javaInstallState state object into
     * the Javacall structure.
     */

    /*
     * This is needed because KNI_SAVE_PCSL_STRING_FIELD() macro
     * calls pcsl_mem_free(destString) for some reason.
     */
    memset((char*)&jcInstallState, 0, sizeof(jcInstallState));

    jcRes = JAVACALL_FAIL;

    do {
        pcsl_string_status pcslRes;
        jsize strSize, convertedLength;
        jint appId, suiteId, jarSize, exceptionCode;

        KNI_GetObjectClass(javaInstallState, clazz);

        KNI_SAVE_INT_FIELD(javaInstallState, clazz, "appId", appId);
        jcInstallState.appId = (javacall_app_id)appId;

        KNI_SAVE_INT_FIELD(javaInstallState, clazz, "suiteId", suiteId);
        jcInstallState.suiteId = (javacall_suite_id)suiteId;

        KNI_SAVE_INT_FIELD(javaInstallState, clazz, "jarSize", jarSize);
        jcInstallState.jarSize = (javacall_int32)jarSize;

        KNI_SAVE_INT_FIELD(javaInstallState, clazz, "exceptionCode",
                           exceptionCode);
        jcInstallState.exceptionCode =
            (javacall_ams_install_exception_code)exceptionCode;

        KNI_SAVE_PCSL_STRING_FIELD(javaInstallState, clazz, "jarUrl",
                                   &pcslJarUrl, tmpHandle);
        strSize = pcsl_string_utf16_length(&pcslJarUrl);
        if (strSize > 0) {
            strSize++; /* for terminating NULL */
            jcInstallState.jarUrl = javacall_malloc(strSize << 1);
            pcslRes = pcsl_string_convert_to_utf16(&pcslJarUrl,
                (jchar*)jcInstallState.jarUrl, strSize, &convertedLength);
            if (pcslRes != PCSL_STRING_OK) {
                jcRes = JAVACALL_OUT_OF_MEMORY;
                break;
            }
        } else {
            jcInstallState.jarUrl = NULL;
        }

        KNI_SAVE_PCSL_STRING_FIELD(javaInstallState, clazz, "suiteName",
            &pcslSuiteName, tmpHandle);
        strSize = pcsl_string_utf16_length(&pcslSuiteName);
        if (strSize > 0) {
            strSize++; /* for terminating NULL */
            jcInstallState.suiteName = javacall_malloc(strSize << 1);
            pcslRes = pcsl_string_convert_to_utf16(&pcslSuiteName,
                (jchar*)jcInstallState.suiteName, strSize, &convertedLength);
            if (pcslRes != PCSL_STRING_OK) {
                jcRes = JAVACALL_OUT_OF_MEMORY;
                break;
            }
        } else {
            jcInstallState.suiteName = NULL;
        }

        /* get the suite properties from the Java object */
        KNI_GetObjectField(javaInstallState,
            midp_get_field_id(KNIPASSARGS clazz, "suiteProperties",
                              "[Ljava/lang/String;"),
            javaProperties);

        /* get the authorization path from the java object */
        KNI_GetObjectField(javaInstallState,
            midp_get_field_id(KNIPASSARGS clazz, "authPath",
                              "[Ljava/lang/String;"),
            javaAuthPath);

        /* convert the suite properties to javacall format */
        if (!KNI_IsNullHandle(javaProperties)) {
            jcRes = java_str_array2javacall_impl(javaProperties, tmpHandle,
                &propertiesLen, &jcInstallState.suiteProperties.pStringArr);
        } else {
            jcRes = JAVACALL_OK;
            propertiesLen = 0;
            jcInstallState.suiteProperties.pStringArr = NULL;
        }

        /* 2 strings (key/value) for each property */
        jcInstallState.suiteProperties.numberOfProperties = propertiesLen >> 1;

        if (jcRes != JAVACALL_OK) {
            break;
        }

        /* convert the authorization path to javacall format */
        if (!KNI_IsNullHandle(javaAuthPath)) {
            jcRes = java_str_array2javacall_impl(javaAuthPath, tmpHandle,
                &jcInstallState.authPathLen, &jcInstallState.pAuthPath);
        } else {
            jcRes = JAVACALL_OK;
            jcInstallState.authPathLen = 0;
            jcInstallState.pAuthPath = NULL;
        }

        /* jcRes contains the result code */
    } while (0);

    pcsl_string_free(&pcslJarUrl);
    pcsl_string_free(&pcslSuiteName);

    if (jcRes != JAVACALL_OK) {
        int j;

        /* free suite properties */
        if (jcInstallState.suiteProperties.pStringArr != NULL) {
            /* free each string */
            for (j = 0;
                     j < jcInstallState.suiteProperties.numberOfProperties << 1;
                         j++) {
                javacall_free(jcInstallState.suiteProperties.pStringArr[j]);
            }
            /* free the array of pointers to the strings */
            javacall_free(jcInstallState.suiteProperties.pStringArr);
        }

        /* free authorization path */
        if (jcInstallState.pAuthPath != NULL) {
            /* free each string */
            for (j = 0; j < jcInstallState.authPathLen; j++) {
                javacall_free(jcInstallState.pAuthPath[j]);
            }
            /* free the array of pointers to the strings */
            javacall_free(jcInstallState.pAuthPath);
        }
    }

    /*
     * now general parameters are read and converted, get
     * request-specific parameters (if any) and send the request
     */
    if (jcRes == JAVACALL_OK) {
        if ((int)requestCode == RQ_UPDATE_STATUS) {
            /* reporting the current installation progress */
            int totalDone = (int)((int)jcRequestData.installStatus * 100 /
                                  (int) JAVACALL_INSTALL_STATUS_COMPLETED);
            int currStepDone = totalDone; /* IMPL_NOTE: should be changed */

            javacall_ams_install_report_progress(&jcInstallState,
                jcRequestData.installStatus, currStepDone, totalDone);
        } else {
            /* get request type-dependent parameters */
            if (requestCode ==
                    JAVACALL_INSTALL_REQUEST_CONFIRM_REDIRECTION) {
                pcsl_string_status pcslRes;
                jsize strSize, convertedLength;

                GET_PARAMETER_AS_PCSL_STRING(4, newLocation)

                strSize = pcsl_string_utf16_length(&newLocation);
                jcRequestData.newLocation =
                    javacall_malloc((strSize + 1) << 1);

                pcslRes = pcsl_string_convert_to_utf16(&newLocation,
                    (jchar*)jcRequestData.newLocation, strSize,
                        &convertedLength);

                RELEASE_PCSL_STRING_PARAMETER

                if (pcslRes != PCSL_STRING_OK) {
                    jcRes = JAVACALL_FAIL;
                }
            }

            if (jcRes == JAVACALL_OK) {
                /* sending the request */
                g_installerIsolateId = -1;
                jcRes = javacall_ams_install_ask(requestCode, &jcInstallState,
                                                 &jcRequestData);

                if (jcRes != JAVACALL_OK) {
                    /* If something is wrong, apply "No" answer */
                    g_fAnswer = JAVACALL_FALSE;
                    g_fAnswerReady = JAVACALL_TRUE;
                }
            }
        } /* if RQ_UPDATE_REQUEST */
    } /* if (jcRes == JAVACALL_OK) */

    KNI_EndHandles();

    KNI_ReturnInt(jcRes == JAVACALL_OK ? 0 : jcRes);
}

/**
 * Returns yes/no answer from the native callback.
 *
 * @return yes/no answer from the native callback
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_midp_installer_InstallerPeerMIDlet_getAnswer0) {
    if (g_fAnswerReady == JAVACALL_TRUE) {
        /* all done, the answer is ready, just return */
        g_fAnswerReady = JAVACALL_FALSE;
        g_installerIsolateId = -1;
    } else {
        g_installerIsolateId = getCurrentIsolateId();
        /* block the thread until the answer is ready */
        midp_thread_wait(INSTALLER_UI_SIGNAL, g_installerIsolateId, NULL);
    }

    KNI_ReturnBoolean(g_fAnswer == JAVACALL_TRUE ? KNI_TRUE : KNI_FALSE);
}

/**
 * Reports to the party using this installer that
 * the operation has been completed.
 *
 * @param appId this application ID
 * @param suiteId ID of the newly installed midlet suite, or
 *                MIDletSuite.UNUSED_SUITE_ID if the installation
 *                failed
 * @param resultCode result of the installation (0 if succeeded or -1 in
 *                   case of unknown error, or one of the values defined
 *                   in InvalidJadException)
 * @param errMsg error message if the installation failed, null otherwise
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_installer_InstallerPeerMIDlet_reportFinished0) {
    pcsl_string errMsgParam = PCSL_STRING_NULL_INITIALIZER;
    jint appId = KNI_GetParameterAsInt(1);
    SuiteIdType suiteId = (SuiteIdType) KNI_GetParameterAsInt(2);
    jint instResult = KNI_GetParameterAsInt(3);
    javacall_ams_install_data jcInstallData;

    KNI_StartHandles(2);
    KNI_DeclareHandle(errMsg);

    if (instResult == 0) {
        jcInstallData.installResultCode = JAVACALL_INSTALL_EXC_ALL_OK;
    } else if (instResult == -1) {
        jcInstallData.installResultCode = JAVACALL_INSTALL_EXC_GENERAL_ERROR;
    } else {
        jcInstallData.installResultCode =
            (javacall_ams_install_exception_code)instResult;
    }

    /* get request type-dependent parameters */
    if (suiteId == UNUSED_SUITE_ID) {
        GET_PARAMETER_AS_PCSL_STRING(4, errMsgParam)

        if (!pcsl_string_is_null(&errMsgParam)) {
            /* IMPL_NOTE: pass the error message to the listener */
        }

        RELEASE_PCSL_STRING_PARAMETER
    }

    jcInstallData.installStatus = JAVACALL_INSTALL_STATUS_COMPLETED;
    jcInstallData.suiteId = (javacall_suite_id)suiteId;

    javacall_ams_operation_completed(JAVACALL_OPCODE_INSTALL_SUITE,
                                     (javacall_app_id)appId,
                                     &jcInstallData);

    KNI_EndHandles();

    KNI_ReturnVoid();
}

/**
 * Reports to the party using this installer that the requested
 * operation has been completed and the result (if any) is available.
 *
 * @param appId this application ID
 * @param requestCode code of the request that was handled
 * @param resultCode completion code (0 if succeeded or -1 in case
 *                   of error)
 * @param result operation-dependent result (for OCSP operations it contains
 *               the current state (enabled/disabled) of OCSP checks)
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_installer_InstallerPeerMIDlet_notifyRequestHandled0) {
    jint  appId       = KNI_GetParameterAsInt(1);
    jint  requestCode = KNI_GetParameterAsInt(2);
    jint  resultCode  = KNI_GetParameterAsInt(3);
    jboolean result   = KNI_GetParameterAsBoolean(4);
    javacall_bool jcResult =
            (result == KNI_TRUE ? JAVACALL_TRUE : JAVACALL_FALSE);
    javacall_opcode operationCode = JAVACALL_OPCODE_INVALID;

    /* IMPL_NOTE: currently unused because the supported operations can't fail */
    (void)resultCode;

    if (requestCode == NATIVE_ENABLE_OCSP_REQUEST) {
        operationCode = JAVACALL_OPCODE_ENABLE_OCSP;
    } else if (requestCode == NATIVE_CHECK_OCSP_ENABLED_REQUEST) {
        operationCode = JAVACALL_OPCODE_IS_OCSP_ENABLED;
    }

    javacall_ams_operation_completed(operationCode,
                                     (javacall_app_id)appId,
                                     &jcResult);

    KNI_ReturnVoid();
}

/**
 * Unblocks the installer thread.
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_installer_InstallerPeerMIDlet_unblockInstaller0) {
    midp_thread_signal(INSTALLER_UI_SIGNAL, getCurrentIsolateId(), 0);

    KNI_ReturnVoid();
}
