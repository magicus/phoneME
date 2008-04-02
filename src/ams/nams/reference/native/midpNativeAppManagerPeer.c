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

#include <jvmconfig.h>
#include <kni.h>
#include <jvm.h>
#include <jvmspi.h>
#include <sni.h>

#include <midpAMS.h>
#include <suitestore_common.h>
#include <suitestore_kni_util.h>
#include <midpEvents.h>
#include <midpServices.h>
#include <midpMidletSuiteUtils.h>
#include <midpNativeAppManager.h>
#include <midpEventUtil.h>
#include <pcsl_string.h>
#include <midpMalloc.h>
#include <midpString.h>
#include <midpError.h>
#include <midpUtilKni.h>
#include <midp_runtime_info.h>
#include <heap.h>

/** The name of the native application manager peer internal class. */
#define APP_MANAGER_PEER "com.sun.midp.main.NativeAppManagerPeer"

/** Static buffer where to save the runtime information passed from Java. */
static MidletRuntimeInfo g_runtimeInfoBuf;

/**
 * Notifies the registered listeners about the given event.
 *
 * @param listenerType type of listeners that should be notified
 * @param pEventData a structure containing all information about the event
 *  that caused the call of the listener
 *
 * @return error code (ALL_OK if successful)
 */
MIDPError
nams_listeners_notify(NamsListenerType listenerType,
                      const NamsEventData* pEventData) {
    NamsEventData* pListener =
        (NamsEventData*) get_event_listeners_impl((int)listenerType);

    while (pListener) {
        if (pListener->genericListener.listenerType == (int)listenerType) {
            ((MIDP_NAMS_EVENT_LISTENER)
                pListener->genericListener.fn_callback)(pEventData);
        }

        pListener = (NamsEventData*)pListener->genericListener.pNext;
    }

    return ALL_OK;
}

/**
 * Initializes the system. This function must be called before setting
 * the listeners and calling midp_system_start().
 *
 * @return error code: (ALL_OK if successful)
 */
MIDPError midp_system_initialize(void) {
	int midp_heap_requirement = getHeapRequirement();

    JVM_Initialize();

    /*
     * Set Java heap capacity now so it can been overridden from command
     * line.
     */
    JVM_SetConfig(JVM_CONFIG_HEAP_CAPACITY, midp_heap_requirement);

    return init_listeners_impl();
}

/**
 * Starts the system. Does not return until the system is stopped.
 *
 * @return <tt>ALL_OK</tt> if the system is shutting down or
 *         <tt>GENERAL_ERROR</tt> if an error
 */
MIDPError midp_system_start(void) {
    int vmStatus;
    MIDPError errCode;
    NamsEventData eventData;

    memset((char*)&eventData, 0, sizeof(NamsEventData));

    vmStatus = midpRunMainClass(NULL, APP_MANAGER_PEER, 0, NULL);

    eventData.event  = MIDP_NAMS_EVENT_STATE_CHANGED;
    eventData.state = MIDP_SYSTEM_STATE_STOPPED;
    nams_listeners_notify(SYSTEM_EVENT_LISTENER, &eventData);

    switch (vmStatus) {
        case MIDP_SHUTDOWN_STATUS: {
            errCode = ALL_OK;
            break;
        }

        default: {
            errCode = GENERAL_ERROR;
            break;
        }
    }

    return errCode;
}

/**
 * Stops the system.
 *
 * @return error code: ALL_OK if the operation was started successfully
 */
MIDPError midp_system_stop(void) {
    MidpEvent evt;

    MIDP_EVENT_INITIALIZE(evt);

    evt.type = SHUTDOWN_EVENT;

    midpStoreEventAndSignalAms(evt);
    return ALL_OK;
}

/**
 * Adds a listener of the given type.
 *
 * @param listener pointer to a callback function that will be called when
 * an event of the given type happens
 * @param listenerType defines on which type of events (SYSTEM, MIDLET or
 * DISPLAY) this listener should be invoked
 *
 * @return error code: ALL_OK if successful,
 *                     OUT_OF_MEMORY if not enough memory,
 *                     BAD_PARAMS if listener is NULL
 */
MIDPError midp_add_event_listener(MIDP_NAMS_EVENT_LISTENER listener,
                                  NamsListenerType listenerType) {
    NamsEventData newListener;

    newListener.genericListener.dataSize = sizeof(NamsEventData);
    newListener.genericListener.fn_callback  = (void*)listener;
    newListener.genericListener.listenerType = (int)listenerType;

    return add_event_listener_impl((GenericListener*)&newListener);
}

/**
 * Removes the given listener of the given type.
 *
 * @param listener listener that should be removed
 * @param listenerType defines for which type of events (SYSTEM, MIDLET or
 * DISPLAY) this listener was added
 *
 * @return error code: ALL_OK if successful,
 *                     NOT_FOUND if the listener was not found
 */
MIDPError
midp_remove_event_listener(MIDP_NAMS_EVENT_LISTENER listener,
                           NamsListenerType listenerType) {
    return remove_event_listener_impl((void*)listener, (int)listenerType);
}

/**
 * Removes all listeners of the given type.
 *
 * @param listenerType defines for which type of events (SYSTEM, MIDLET or
 * DISPLAY) the registered listeneres must be removed
 *
 * @return error code: ALL_OK if successful,
 *                     NOT_FOUND if there are no registered listeners
 *                               of the given type
 */
MIDPError
midp_remove_all_event_listeners(NamsListenerType listenerType) {
    return remove_all_event_listeners_impl((int)listenerType);
}

/**
 * Create and start the specified MIDlet. The suiteId is passed to the
 * midletsuitestorage API to retrieve the class path.
 *
 * @param suiteId             The application suite ID
 * @param className           Fully qualified name of the MIDlet class
 * @param classNameLen        Length of the MIDlet class name
 * @param args                An array containning up to 3 arguments for
 *                            the MIDlet to be run
 * @param argsLen             An array containing the length of each argument
 * @param argsNum             Number of arguments
 * @param appId               The application id used to identify the app
 * @param pRuntimeInfo Quotas and profile to set for the new application
 *
 * @return error code: ALL_OK if the operation was started successfully,
 *                     BAD_PARAMS if suiteId is invalid or pClassName is null
 */
MIDPError midp_midlet_create_start_with_args(SuiteIdType suiteId,
        const jchar *className, jint classNameLen,
        const jchar **args, const jint *argsLen,
        jint argsNum, jint appId, const MidletRuntimeInfo* pRuntimeInfo) {
    MidpEvent evt;
    pcsl_string temp;
    /*
     * evt.stringParam1 is a midlet class name,
     * evt.stringParam2 is a display name,
     * evt.stringParam3-5 - the arguments
     * evt.stringParam6 - profile name
     */
    pcsl_string* params[] = {
        &evt.stringParam3, &evt.stringParam4, &evt.stringParam5
    };
    int i;

    MIDP_EVENT_INITIALIZE(evt);

    if (PCSL_STRING_OK ==
        pcsl_string_convert_from_utf16(className, classNameLen, &temp)) {
        if (pcsl_string_utf16_length(&temp) > 0) {
            evt.stringParam1 = temp;
        } else {
            pcsl_string_free(&temp);
        }
    }

    /* Initialize arguments for the midlet to be run. */
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

    if (pRuntimeInfo) {
        /* Initialize profile's name */
        if (pRuntimeInfo->profileNameLen && pRuntimeInfo->profileName != NULL) {
            if (PCSL_STRING_OK == pcsl_string_convert_from_utf16(
                                      pRuntimeInfo->profileName,
                                      pRuntimeInfo->profileNameLen,
                                      &temp)) {
                if (pcsl_string_utf16_length(&temp) > 0) {
                    evt.stringParam6 = temp;
                } else {
                    pcsl_string_free(&temp);
                }
            } else {
                evt.stringParam6 = PCSL_STRING_NULL;
            }
        }

        evt.intParam3 = pRuntimeInfo->memoryReserved;
        evt.intParam4 = pRuntimeInfo->memoryTotal;
        evt.intParam5 = pRuntimeInfo->priority;
    } else {
        evt.stringParam6 = PCSL_STRING_NULL;
        evt.intParam3 = evt.intParam4 = evt.intParam5 = -1;
    }

    evt.type = NATIVE_MIDLET_EXECUTE_REQUEST;
    evt.intParam1 = appId;
    evt.intParam2 = suiteId;

    midpStoreEventAndSignalAms(evt);
    return ALL_OK;
}

/**
 * Create and start the specified MIDlet. The suiteId is passed to the
 * midletsuitestorage API to retrieve the class path.
 *
 * @param suiteId             The application suite ID
 * @param className           Fully qualified name of the MIDlet class
 * @param classNameLen        Length of the MIDlet class name
 * @param appId               The application id used to identify the app
 * @param pRuntimeInfo Quotas and profile to set for the new application
 *
 * @return error code: ALL_OK if the operation was started successfully,
 *                     BAD_PARAMS if suiteId is invalid or pClassName is null
 */
MIDPError midp_midlet_create_start(SuiteIdType suiteId,
                                   const jchar *className, jint classNameLen,
                                   jint appId,
                                   const MidletRuntimeInfo* pRuntimeInfo) {
    return midp_midlet_create_start_with_args(suiteId,
        className, classNameLen, NULL, NULL, 0, appId, pRuntimeInfo);
}

/**
 * Resume the specified paused MIDlet.
 *
 * @param appId The application id used to identify the app
 *
 * @return error code: ALL_OK if the operation was started successfully
 */
MIDPError midp_midlet_resume(jint appId) {
    MidpEvent evt;

    MIDP_EVENT_INITIALIZE(evt);

    evt.type = NATIVE_MIDLET_RESUME_REQUEST;
    evt.intParam1 = appId;

    midpStoreEventAndSignalAms(evt);
    return ALL_OK;
}

/**
 * Pause the specified MIDlet.
 *
 * @param appId The application id used to identify the app
 *
 * @return error code: ALL_OK if the operation was started successfully
 */
MIDPError midp_midlet_pause(jint appId) {
    MidpEvent evt;

    MIDP_EVENT_INITIALIZE(evt);

    evt.type = NATIVE_MIDLET_PAUSE_REQUEST;
    evt.intParam1 = appId;

    midpStoreEventAndSignalAms(evt);
    return ALL_OK;
}

/**
 * Stop the specified MIDlet.
 *
 * If the midlet is not terminated within the given number of milliseconds,
 * it will be forcefully terminated.
 *
 * @param appId The application id used to identify the app
 * @param timeout Timeout in milliseconds
 *
 * @return error code: ALL_OK if the operation was started successfully
 */
MIDPError midp_midlet_destroy(jint appId, jint timeout) {
    MidpEvent evt;

    MIDP_EVENT_INITIALIZE(evt);

    evt.type = NATIVE_MIDLET_DESTROY_REQUEST;
    evt.intParam1 = appId;
    evt.intParam2 = timeout;

    midpStoreEventAndSignalAms(evt);
    return ALL_OK;
}

/**
 * Gets information about the suite containing the specified running MIDlet.
 * This call is synchronous.
 *
 * @param appId The ID used to identify the application
 *
 * @param pSuiteData [out] pointer to a structure where static information
 *                         about the midlet will be stored
 *
 * @return error code: ALL_OK if successful,
 *                     NOT_FOUND if the application was not found,
 *                     BAD_PARAMS if pSuiteData is null
 */
MIDPError midp_midlet_get_suite_info(jint appId, MidletSuiteData* pSuiteData) {
    (void)appId; /* not finished */
    if (pSuiteData == NULL) {
        return BAD_PARAMS;
    }

    return ALL_OK;
}

/**
 * Gets runtime information about the specified MIDlet.
 *
 * This call is asynchronous, the result will be reported later through
 * passing a MIDLET_INFO_READY_EVENT event to SYSTEM_EVENT_LISTENER.
 *
 * @param appId The ID used to identify the application
 *
 * @return error code: ALL_OK if successful (operation started),
 *                     NOT_FOUND if the application was not found
 */
MIDPError midp_midlet_request_runtime_info(jint appId) {
    MidpEvent evt;

    MIDP_EVENT_INITIALIZE(evt);

    evt.type = NATIVE_MIDLET_GETINFO_REQUEST;
    evt.intParam1 = appId;

    midpStoreEventAndSignalAms(evt);
    return ALL_OK;
}

/**
 * Saves runtime information from the given structure
 * into the native buffer.
 *
 * @param runtimeInfo structure holding the information to save
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_NativeAppManagerPeer_saveRuntimeInfoInNative(void) {
    KNI_StartHandles(2);
    KNI_DeclareHandle(runtimeInfo);
    KNI_DeclareHandle(clazz);
    /* KNI_DeclareHandle(string); */

    KNI_GetParameterAsObject(1, runtimeInfo);
    KNI_GetObjectClass(runtimeInfo, clazz);

    KNI_SAVE_INT_FIELD(runtimeInfo, clazz, "memoryReserved",
                       g_runtimeInfoBuf.memoryReserved);
    KNI_SAVE_INT_FIELD(runtimeInfo, clazz, "memoryTotal",
                       g_runtimeInfoBuf.memoryTotal);
    KNI_SAVE_INT_FIELD(runtimeInfo, clazz, "usedMemory",
                       g_runtimeInfoBuf.usedMemory);
    KNI_SAVE_INT_FIELD(runtimeInfo, clazz, "priority",
                       g_runtimeInfoBuf.priority);

    g_runtimeInfoBuf.profileName    = NULL;
    g_runtimeInfoBuf.profileNameLen = 0;
    /*
    do {
        KNI_SAVE_PCSL_STRING_FIELD(runtimeInfo, clazz, "profileName",
                                   &g_runtimeInfoBuf.profileName, string);
    } while (0);
    */

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Notify the native application manager that the system has completed
 * the requested operation and the result (if any) is available.
 *
 * @param operation code of the operation that has completed
 * @param externalAppId ID assigned by the external application manager
 * @param retCode completion code (0 if OK)
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_NativeAppManagerPeer_notifyOperationCompleted(void) {
    NamsEventData eventData;
    int retCode = KNI_GetParameterAsInt(3);

    memset((char*)&eventData, 0, sizeof(NamsEventData));

    eventData.event  = MIDP_NAMS_EVENT_OPERATION_COMPLETED;
    eventData.reason = KNI_GetParameterAsInt(1);
    eventData.appId  = KNI_GetParameterAsInt(2);

    if (retCode == 0) {
        eventData.state = ALL_OK;
        if (eventData.reason == NATIVE_MIDLET_GETINFO_REQUEST) {
            eventData.pRuntimeInfo = &g_runtimeInfoBuf;
        }
    } else {
        eventData.state = BAD_PARAMS;
    }
    
    nams_listeners_notify(SYSTEM_EVENT_LISTENER, &eventData);

    KNI_ReturnVoid();
}

/**
 * Notify the native application manager that the system had an error starting.
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_NativeAppManagerPeer_notifySystemStartError(void) {
    NamsEventData eventData;
    memset((char*)&eventData, 0, sizeof(NamsEventData));

    eventData.event = MIDP_NAMS_EVENT_STATE_CHANGED;
    eventData.state = MIDP_SYSTEM_STATE_ERROR;
    nams_listeners_notify(SYSTEM_EVENT_LISTENER, &eventData);

    KNI_ReturnVoid();
}

/**
 * Notify the native application manager of the system start up.
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_NativeAppManagerPeer_notifySystemStart(void) {
    NamsEventData eventData;

    memset((char*)&eventData, 0, sizeof(NamsEventData));
    eventData.event = MIDP_NAMS_EVENT_STATE_CHANGED;    
    eventData.state = MIDP_SYSTEM_STATE_ACTIVE;
    nams_listeners_notify(SYSTEM_EVENT_LISTENER, &eventData);

    KNI_ReturnVoid();
}

/**
 * Notify the native application manager of the system suspension.
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_NativeAppManagerPeer_notifySystemSuspended(void) {
    NamsEventData eventData;

    memset((char*)&eventData, 0, sizeof(NamsEventData));
    eventData.event = MIDP_NAMS_EVENT_STATE_CHANGED;    
    eventData.state = MIDP_SYSTEM_STATE_SUSPENDED;
    nams_listeners_notify(SYSTEM_EVENT_LISTENER, &eventData);

    KNI_ReturnVoid();
}

/**
 * Notify the native application manager of the MIDlet creation.
 *
 * @param externalAppId ID assigned by the external application manager
 * @param suiteId ID of the midlet suite
 * @param className class name of the midlet that failed to start
 * @param error error code
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_NativeAppManagerPeer_notifyMidletStartError(void) {
    jint externalAppId = KNI_GetParameterAsInt(1);
    jint error = KNI_GetParameterAsInt(4);
    NamsEventData eventData;
    MidletSuiteData msd;
    pcsl_string_status res;

    memset((char*)&eventData, 0, sizeof(NamsEventData));
    memset((char*)&msd, 0, sizeof(MidletSuiteData));
    eventData.event = MIDP_NAMS_EVENT_STATE_CHANGED;    
    eventData.appId = externalAppId;
    eventData.state = MIDP_MIDLET_STATE_ERROR;
    eventData.reason = error;
    msd.suiteId = KNI_GetParameterAsInt(2);
    eventData.pSuiteData = &msd;

    KNI_StartHandles(1);
    KNI_DeclareHandle(handle);
    KNI_GetParameterAsObject(3, handle);

    res = midp_jstring_to_pcsl_string(handle,
                                      &msd.varSuiteData.midletClassName);

    /*
     * notify the listeners even failed to get the midlet's class name,
     * just don't use it
     */
    nams_listeners_notify(MIDLET_EVENT_LISTENER, &eventData);

    if (res == PCSL_STRING_OK) {
        pcsl_string_free(&msd.varSuiteData.midletClassName);
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Notify the native application manager of the MIDlet creation.
 *
 * @param externalAppId ID assigned by the external application manager
 * @param suiteId ID of the suite the created midlet belongs to
 * @param className class name of the midlet that was created
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_NativeAppManagerPeer_notifyMidletCreated(void) {
    jint externalAppId = KNI_GetParameterAsInt(1);
    NamsEventData eventData;
    MidletSuiteData msd;
    pcsl_string_status res;

    memset((char*)&eventData, 0, sizeof(NamsEventData));
    memset((char*)&msd, 0, sizeof(MidletSuiteData));
    eventData.event = MIDP_NAMS_EVENT_STATE_CHANGED;    
    eventData.appId = externalAppId;
    eventData.state = MIDP_MIDLET_STATE_PAUSED;
    msd.suiteId = KNI_GetParameterAsInt(2);
    eventData.pSuiteData = &msd;

    KNI_StartHandles(1);
    KNI_DeclareHandle(handle);
    KNI_GetParameterAsObject(3, handle);

    res = midp_jstring_to_pcsl_string(handle,
                                      &msd.varSuiteData.midletClassName);

    nams_listeners_notify(MIDLET_EVENT_LISTENER, &eventData);

    if (res == PCSL_STRING_OK) {
        pcsl_string_free(&msd.varSuiteData.midletClassName);
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Notify the native application manager that the MIDlet is active.
 *
 * @param externalAppId ID assigned by the external application manager
 * @param suiteId ID of the suite the started midlet belongs to
 * @param className class name of the midlet that was started
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_NativeAppManagerPeer_notifyMidletActive(void) {
    jint externalAppId = KNI_GetParameterAsInt(1);
    NamsEventData eventData;
    MidletSuiteData msd;
    pcsl_string_status res;

    memset((char*)&eventData, 0, sizeof(NamsEventData));
    memset((char*)&msd, 0, sizeof(MidletSuiteData));
    eventData.event = MIDP_NAMS_EVENT_STATE_CHANGED;    
    eventData.appId = externalAppId;
    eventData.state = MIDP_MIDLET_STATE_ACTIVE;
    msd.suiteId = KNI_GetParameterAsInt(2);
    eventData.pSuiteData = &msd;

    KNI_StartHandles(1);
    KNI_DeclareHandle(handle);
    KNI_GetParameterAsObject(3, handle);

    res = midp_jstring_to_pcsl_string(handle,
                                      &msd.varSuiteData.midletClassName);

    nams_listeners_notify(MIDLET_EVENT_LISTENER, &eventData);

    if (res == PCSL_STRING_OK) {
        pcsl_string_free(&msd.varSuiteData.midletClassName);
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Notify the native application manager that the MIDlet is paused.
 *
 * @param externalAppId ID assigned by the external application manager
 * @param suiteId ID of the suite the paused midlet belongs to
 * @param className class name of the midlet that was paused
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_NativeAppManagerPeer_notifyMidletPaused(void) {
    jint externalAppId = KNI_GetParameterAsInt(1);
    NamsEventData eventData;
    MidletSuiteData msd;
    pcsl_string_status res;

    memset((char*)&eventData, 0, sizeof(NamsEventData));
    memset((char*)&msd, 0, sizeof(MidletSuiteData));
    eventData.event = MIDP_NAMS_EVENT_STATE_CHANGED;    
    eventData.appId = externalAppId;
    eventData.state = MIDP_MIDLET_STATE_PAUSED;
    msd.suiteId = KNI_GetParameterAsInt(2);
    eventData.pSuiteData = &msd;

    KNI_StartHandles(1);
    KNI_DeclareHandle(handle);
    KNI_GetParameterAsObject(3, handle);

    res = midp_jstring_to_pcsl_string(handle,
                                      &msd.varSuiteData.midletClassName);

    nams_listeners_notify(MIDLET_EVENT_LISTENER, &eventData);

    if (res == PCSL_STRING_OK) {
        pcsl_string_free(&msd.varSuiteData.midletClassName);
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Notify the native application manager that the MIDlet is destroyed.
 *
 * @param externalAppId ID assigned by the external application manager
 * @param suiteId ID of the suite the destroyed midlet belongs to
 * @param className class name of the midlet that was destroyed
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_NativeAppManagerPeer_notifyMidletDestroyed(void) {
    jint externalAppId = KNI_GetParameterAsInt(1);
    NamsEventData eventData;
    MidletSuiteData msd;
    pcsl_string_status res;

    memset((char*)&eventData, 0, sizeof(NamsEventData));
    memset((char*)&msd, 0, sizeof(MidletSuiteData));
    eventData.event = MIDP_NAMS_EVENT_STATE_CHANGED;
    eventData.appId = externalAppId;
    eventData.state = MIDP_MIDLET_STATE_DESTROYED;
    eventData.reason = MIDP_REASON_EXIT;
    msd.suiteId = KNI_GetParameterAsInt(2);
    eventData.pSuiteData = &msd;

    KNI_StartHandles(1);
    KNI_DeclareHandle(handle);
    KNI_GetParameterAsObject(3, handle);

    res = midp_jstring_to_pcsl_string(handle,
                                      &msd.varSuiteData.midletClassName);

    nams_listeners_notify(MIDLET_EVENT_LISTENER, &eventData);

    if (res == PCSL_STRING_OK) {
        pcsl_string_free(&msd.varSuiteData.midletClassName);
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Notify the native application manager that the suite is terminated.
 *
 * @param suiteId ID of the MIDlet suite
 * @param className class name of the midlet that was terminated
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_NativeAppManagerPeer_notifySuiteTerminated(void) {
    SuiteIdType suiteId;
    NamsEventData eventData;
    MidletSuiteData msd;
    pcsl_string_status res;

    suiteId = KNI_GetParameterAsInt(1);

    memset((char*)&eventData, 0, sizeof(NamsEventData));
    memset((char*)&msd, 0, sizeof(MidletSuiteData));
    eventData.event = MIDP_NAMS_EVENT_STATE_CHANGED;    
    eventData.state = MIDP_MIDLET_STATE_DESTROYED;
    eventData.reason = MIDP_REASON_TERMINATED;
    msd.suiteId = suiteId;
    eventData.pSuiteData = &msd;

    KNI_StartHandles(1);
    KNI_DeclareHandle(handle);
    KNI_GetParameterAsObject(2, handle);

    res = midp_jstring_to_pcsl_string(handle,
                                      &msd.varSuiteData.midletClassName);

    nams_listeners_notify(MIDLET_EVENT_LISTENER, &eventData);

    if (res == PCSL_STRING_OK) {
        pcsl_string_free(&msd.varSuiteData.midletClassName);
    }

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
