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
#include <stdio.h>

#include <jvmconfig.h>
#include <kni.h>
#include <jvm.h>
#include <jvmspi.h>
#include <sni.h>

#include <midpAMS.h>
#include <suitestore_export.h>
#include <midpEvents.h>
#include <midpServices.h>
#include <midpMidletSuiteLoader.h>
#include <midpNativeAppManager.h>
#include <midpEventUtil.h>
#include <pcsl_string.h>
#include <midpMalloc.h>
#include <midpString.h>
#include <midpError.h>
#include <midpUtilKni.h>

/** The name of the native application manager peer internal class. */
#define APP_MANAGER_PEER "com.sun.midp.main.NativeAppManagerPeer"

static MIDP_SYSTEM_STATE_CHANGE_LISTENER systemStateChangeListener = NULL;

static MIDP_MIDLET_STATE_CHANGE_LISTENER midletStateChangeListener = NULL;

static MIDP_SUITE_TERMINATION_LISTENER midletSuiteTerminationListener = NULL;

/**
 * Starts the system. Does not return until the system is stopped.
 *
 * @return <tt>MIDP_SHUTDOWN_STATUS</tt> if the system is shutting down or
 *         <tt>MIDP_ERROR_STATUS</tt> if an error
 */
jint midp_system_start(void) {
    jint status;

    status = (jint)midpRunMainClass(NULL, APP_MANAGER_PEER, 0, NULL);
    if (systemStateChangeListener != NULL) {
        systemStateChangeListener(MIDP_SYSTEM_STATE_STOPPED);
    }

    return status;
}

/**
 * Stops the system.
 */
void midp_system_stop(void) {
    MidpEvent evt;

    MIDP_EVENT_INITIALIZE(evt);

    evt.type = SHUTDOWN_EVENT;

    midpStoreEventAndSignalAms(evt);
}

/**
 * Sets the system state listener.
 *
 * @param listener The system state listener
 */
void midp_system_set_state_change_listener(
        MIDP_SYSTEM_STATE_CHANGE_LISTENER listener) {
    systemStateChangeListener = listener;
}

/**
 * Sets the midlet state change listener.
 *
 * @param listener            The midlet state change listener
 */
void midp_midlet_set_state_change_listener(
        MIDP_MIDLET_STATE_CHANGE_LISTENER listener) {
    midletStateChangeListener = listener;
}

/**
 * Sets the suite termination listener.
 *
 * @param listener            The termination listener
 */
void midp_suite_set_termination_listener(
        MIDP_SUITE_TERMINATION_LISTENER listener) {
    midletSuiteTerminationListener = listener;
}


/**
 * Create and start the specified MIDlet. The suiteId is passed to the
 * midletsuitestorage API to retrieve the class path.
 *
 * @param suiteId             The application suite ID
 * @param suiteIdLen          Length of the application suite ID
 * @param className           Fully qualified name of the MIDlet class
 * @param classNameLen        Length of the MIDlet class name
 * @param args                An array containning up to 3 arguments for
 *                            the MIDlet to be run
 * @param argsLen             An array containing the length of each argument
 * @param argsNum             Number of arguments
 * @param appId               The application id used to identify the app
 */
void midp_midlet_create_start_with_args(jchar *suiteId, jint suiteIdLen,
        jchar *className, jint classNameLen, jchar **args, jint *argsLen,
        jint argsNum, jint appId) {
    MidpEvent evt;
    pcsl_string temp;
    /* evt.stringParam3 is a display name, evt.stringParam4-6 - the arguments */
    pcsl_string* params[] = {
        &evt.stringParam4, &evt.stringParam5, &evt.stringParam6
    };
    int i;

    MIDP_EVENT_INITIALIZE(evt);

    if (PCSL_STRING_OK ==
        pcsl_string_convert_from_utf16(suiteId, suiteIdLen, &temp)) {
        if (pcsl_string_utf16_length(&temp) > 0) {
            evt.stringParam1 = temp;
        } else {
            pcsl_string_free(&temp);
        }
    }

    if (PCSL_STRING_OK ==
        pcsl_string_convert_from_utf16(className, classNameLen, &temp)) {
        if (pcsl_string_utf16_length(&temp) > 0) {
            evt.stringParam2 = temp;
        } else {
            pcsl_string_free(&temp);
        }
    }

    // Initialize arguments for the midlet to be run.
    for (i = 0; i < 3; i++) {
        if ((i >= argsNum) || (argsLen[i] == 0)) {
            *params[i] = PCSL_STRING_NULL;
        } else {
            if (PCSL_STRING_OK ==
                pcsl_string_convert_from_utf16(args[i], argsLen[i], &temp)) {
                if (pcsl_string_utf16_length(&temp) > 0) {
                    *params[i] = temp;
                } else {
                    pcsl_string_free(&temp);
                    *params[i] = PCSL_STRING_NULL;
                }
            } else {
                *params[i] = PCSL_STRING_NULL;
            }
        }
    }

    evt.type = NATIVE_MIDLET_EXECUTE_REQUEST;
    evt.intParam1 = appId;

    midpStoreEventAndSignalAms(evt);
}

/**
 * Create and start the specified MIDlet. The suiteId is passed to the
 * midletsuitestorage API to retrieve the class path.
 *
 * @param suiteId             The application suite ID
 * @param suiteIdLen          Length of the application suite ID
 * @param className           Fully qualified name of the MIDlet class
 * @param classNameLen        Length of the MIDlet class name
 * @param appId               The application id used to identify the app
 */
void midp_midlet_create_start(jchar *suiteId, jint suiteIdLen,
                              jchar *className, jint classNameLen,
                              jint appId) {
    midp_midlet_create_start_with_args(suiteId, suiteIdLen,
        className, classNameLen, NULL, NULL, 0, appId);
}

/**
 * Resume the specified paused MIDlet.
 *
 * @param appId               The application id used to identify the app
 */
void midp_midlet_resume(jint appId) {
    MidpEvent evt;

    MIDP_EVENT_INITIALIZE(evt);

    evt.type = NATIVE_MIDLET_RESUME_REQUEST;
    evt.intParam1 = appId;

    midpStoreEventAndSignalAms(evt);
}

/**
 * Pause the specified MIDlet.
 *
 * @param appId               The application id used to identify the app
 *
 * Returns KNI_FALSE if there is an error accepting this request, KNI_TRUE otherwise.
 */
void midp_midlet_pause(jint appId) {
    MidpEvent evt;

    MIDP_EVENT_INITIALIZE(evt);

    evt.type = NATIVE_MIDLET_PAUSE_REQUEST;
    evt.intParam1 = appId;

    midpStoreEventAndSignalAms(evt);
}

/**
 * Stop the specified MIDlet.
 *
 * @param appId               The application id used to identify the app
 */
void midp_midlet_destroy(jint appId) {
    MidpEvent evt;

    MIDP_EVENT_INITIALIZE(evt);

    evt.type = NATIVE_MIDLET_DESTROY_REQUEST;
    evt.intParam1 = appId;

    midpStoreEventAndSignalAms(evt);
}

/**
 * Notify the native application manager that the system had an error starting.
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_NativeAppManagerPeer_notifySystemStartError(void) {
    if (systemStateChangeListener != NULL) {
        systemStateChangeListener(MIDP_SYSTEM_STATE_ERROR);
    }

    KNI_ReturnVoid();
}

/**
 * Notify the native application manager of the system start up.
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_NativeAppManagerPeer_notifySystemStart(void) {
    if (systemStateChangeListener != NULL) {
        systemStateChangeListener(MIDP_SYSTEM_STATE_STARTED);
    }

    KNI_ReturnVoid();
}

/**
 * Notify the native application manager of the MIDlet creation.
 *
 * @param externalAppId ID assigned by the external application manager
 * @param error error code
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_NativeAppManagerPeer_notifyMidletStartError(void) {
    jint externalAppId = KNI_GetParameterAsInt(1);
    jint error = KNI_GetParameterAsInt(2);

    if (midletStateChangeListener != NULL) {
        midletStateChangeListener(externalAppId, MIDP_MIDLET_STATE_ERROR,
                                  error);
    }

    KNI_ReturnVoid();
}

/**
 * Notify the native application manager of the MIDlet creation.
 *
 * @param externalAppId ID assigned by the external application manager
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_NativeAppManagerPeer_notifyMidletCreated(void) {
    jint externalAppId = KNI_GetParameterAsInt(1);

    if (midletStateChangeListener != NULL) {
        midletStateChangeListener(externalAppId, MIDP_MIDLET_STATE_PAUSED, 0);
    }

    KNI_ReturnVoid();
}

/**
 * Notify the native application manager that the MIDlet is active.
 *
 * @param externalAppId ID assigned by the external application manager
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_NativeAppManagerPeer_notifyMidletActive(void) {
    jint externalAppId = KNI_GetParameterAsInt(1);

    if (midletStateChangeListener != NULL) {
        midletStateChangeListener(externalAppId, MIDP_MIDLET_STATE_STARTED, 0);
    }

    KNI_ReturnVoid();
}

/**
 * Notify the native application manager that the MIDlet is paused.
 *
 * @param externalAppId ID assigned by the external application manager
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_NativeAppManagerPeer_notifyMidletPaused(void) {
    jint externalAppId = KNI_GetParameterAsInt(1);

    if (midletStateChangeListener != NULL) {
        midletStateChangeListener(externalAppId, MIDP_MIDLET_STATE_PAUSED, 0);
    }

    KNI_ReturnVoid();
}

/**
 * Notify the native application manager that the MIDlet is destroyed.
 *
 * @param externalAppId ID assigned by the external application manager
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_NativeAppManagerPeer_notifyMidletDestroyed(void) {
    jint externalAppId = KNI_GetParameterAsInt(1);

    if (midletStateChangeListener != NULL) {
        midletStateChangeListener(externalAppId, MIDP_MIDLET_STATE_DESTROYED,
                                  0);
    }

    KNI_ReturnVoid();
}

/**
 * Notify the native application manager that the suite is terminated.
 *
 * @param suiteId ID of the MIDlet suite
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_NativeAppManagerPeer_notifySuiteTerminated(void) {
    if (NULL == midletSuiteTerminationListener) {
        return;
    }

    KNI_StartHandles(1);
    GET_PARAMETER_AS_PCSL_STRING(1, v_suiteID) {
        pcsl_string* const suiteID = &v_suiteID;
        GET_PCSL_STRING_DATA_AND_LENGTH(suiteID)
        if (PCSL_STRING_PARAMETER_ERROR(suiteID)) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
        } else {
            midletSuiteTerminationListener((jchar*)suiteID_data, suiteID_len);
        }
        RELEASE_PCSL_STRING_DATA_AND_LENGTH
    } RELEASE_PCSL_STRING_PARAMETER

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Register the Isolate ID of the AMS Isolate by making a native
 * method call that will call JVM_CurrentIsolateId and set
 * it in the proper native variable.
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_NativeAppManagerPeer_registerAmsIsolateId(void) {
    midpRegisterAmsIsolateId();
    KNI_ReturnVoid();
}

/**
 * Stops the VM and exit immediately.
 * <p>
 * Java declaration:
 * <pre>
 *     exitInternal(I)V
 * </pre>
 *
 * @param value The return code of the VM.
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_NativeAppManagerPeer_exitInternal(void) {
    int value = (int)KNI_GetParameterAsInt(1);

    midp_exitVM(value);
    KNI_ReturnVoid();
}
