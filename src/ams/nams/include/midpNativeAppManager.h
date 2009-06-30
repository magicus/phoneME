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

#ifndef _MIDPNATIVEAPPMANAGER_H_
#define _MIDPNATIVEAPPMANAGER_H_

/**
 * @file
 * @ingroup nams_extern
 *
 * @brief External Interface for native AMS.
 *
 * @{
 */

#include <java_types.h>
#include <midp_global_status.h>
#include <suitestore_common.h>
#include <listeners_intern.h>
#include <midp_runtime_info.h>

/*
 * Definitions of the events that may cause a call of listener.
 */
/**
 * @def MIDP_NAMS_EVENT_STATE_CHANGED 1
 * state of the MIDP system or of a midlet was changed
 */
#define MIDP_NAMS_EVENT_STATE_CHANGED 1

/**
 * @def MIDP_NAMS_EVENT_OPERATION_COMPLETED 2
 * previously initiated asynchronous operation has completed
 */
#define MIDP_NAMS_EVENT_OPERATION_COMPLETED 2

/**
 * @def MIDP_NAMS_EVENT_ERROR 3
 * some error occured
 */
#define MIDP_NAMS_EVENT_ERROR 3

/**
 * @def MIDP_NAMS_EVENT_UNCAUGHT_EXCEPTION 4
 * an uncaught exception has occured in a MIDlet
 */
#define MIDP_NAMS_EVENT_UNCAUGHT_EXCEPTION 4

/**
 * @def MIDP_NAMS_EVENT_OUT_OF_MEMORY 5
 * VM can't fulfill a memory allocation request
 */
#define MIDP_NAMS_EVENT_OUT_OF_MEMORY 5

/*
 * Defines for Java platform system states.
 *
 * IMPL_NOTE: please note that for NAMS testing it is supposed that
 *            the following state constants have the consequent values;
 *            otherwise, please change NamsTestService.getStateByValue().
 */
/**
 * @def MIDP_SYSTEM_STATE_ACTIVE
 * when system is started up and ready to serve any MIDlet requests
 */
#define MIDP_SYSTEM_STATE_ACTIVE     1

/**
 * @def MIDP_SYSTEM_STATE_SUSPENDED
 * when system finishes suspending all MIDlets and resources
 */
#define MIDP_SYSTEM_STATE_SUSPENDED  2

/**
 * @def MIDP_SYSTEM_STATE_STOPPED
 * when system stops the VM and frees all resources
 */
#define MIDP_SYSTEM_STATE_STOPPED    3

/**
 * @def MIDP_SYSTEM_STATE_ERROR
 * when system cannot be started
 */
#define MIDP_SYSTEM_STATE_ERROR      4

/**
 * Defines for MIDlet states
 */
#define MIDP_MIDLET_STATE_ACTIVE     1
#define MIDP_MIDLET_STATE_PAUSED     2
#define MIDP_MIDLET_STATE_DESTROYED  3
#define MIDP_MIDLET_STATE_ERROR      4
#define MIDP_DISPLAY_STATE_FOREGROUND 5
#define MIDP_DISPLAY_STATE_BACKGROUND 6
#define MIDP_DISPLAY_STATE_FOREGROUND_REQUEST 7
#define MIDP_DISPLAY_STATE_BACKGROUND_REQUEST 8

/**
 * Reasons that may cause a state change
 */
#define MIDP_REASON_TERMINATED 1
#define MIDP_REASON_EXIT       2
#define MIDP_REASON_OTHER      3

/**
 * @def MIDP_INVALID_ISOLATE_ID
 * An ID value that an isolate never has.
 */
#define MIDP_INVALID_ISOLATE_ID -1

/**
 * Structure with information about uncaught exception thrown from the MIDlet.
 */
typedef struct _midletExceptionInfo {
    /**
     * Name of the MIDlet from where the exception was thrown.
     */
    pcsl_string midletName;

    /**
     * Name of the class containing the method that threw the exception.
     */
    pcsl_string exceptionClassName;

    /**
     * Exception message.
     */
    pcsl_string exceptionMessage;

    /**
     * KNI_TRUE if this is the last thread of the MIDlet,
     * KNI_FALSE otherwise
     */
    jboolean isLastThread;

    /**
     * In SVM mode - heap capacity, in MVM mode - memory limit for the isolate,
     * i.e. the max amount of heap memory that can possibly be allocated.
     */
    jint memoryLimit;

    /**
     * In SVM mode - heap capacity, in MVM mode - memory reservation for the
     * isolate, i.e. the max amount of heap memory guaranteed to be available.
     */
    jint memoryReserved;

    /**
     * How much memory is already allocated on behalf of this isolate
     * in MVM mode, or for the whole VM in SVM mode.
     */
    jint memoryUsed;

    /**
     * The requested amount of memory that the VM failed to allocate.
     */
    jint allocSize;
} MidletExceptionInfo;

/**
 * A structure containing all information about the event that caused
 * the call of the listener.
 */
typedef struct _namsEventData {
    /** a structure containing a common data for all types of listeners */
    GenericListener genericListener;
    /** event type */
    jint event;
    /** ID identifying the running application which this event is related to */
    jint appId;
    /**
     * The reason why the state change has happened.
     * For a midlet state change, the reason code is one of the values:
     * MIDLET_CONSTRUCTOR_FAILED,
     * MIDLET_SUITE_NOT_FOUND,
     * MIDLET_CLASS_NOT_FOUND
     * MIDLET_INSTANTIATION_EXCEPTION,
     * MIDLET_ILLEGAL_ACCESS_EXCEPTION,
     * MIDLET_OUT_OF_MEM_ERROR,
     * MIDLET_RESOURCE_LIMIT, or
     * MIDLET_ISOLATE_RESOURCE_LIMIT, or
     * MIDLET_ISOLATE_CONSTRUCTOR_FAILED.
     * See src/configuration/common/constants.xml for definitions.
     */
    jint reason;
    /** the new state of the application or system (depending on the event) */
    jint state;
    /** information about the midlet suite corresponding to this application */
    MidletSuiteData* pSuiteData;
    /** runtime information about the application */
    MidletRuntimeInfo* pRuntimeInfo;
    /** information about the uncaught exception thrown from this application */
    MidletExceptionInfo* pExceptionInfo;
} NamsEventData;

/**
 * Listener types
 */
typedef enum _namsListenerType {
    /*
     * Starting from 1 because LISTENER_TYPE_INVALID is defined as 0
     * in our implementation of listeners. If you need to change this,
     * adjust the constants in listeners_intern.h.
     */
    SYSTEM_EVENT_LISTENER  = 1,
    MIDLET_EVENT_LISTENER  = 2,
    DISPLAY_EVENT_LISTENER = 3,
    ANY_EVENT_LISTENER     = 4
} NamsListenerType;

/**
 * The typedef of the function that will be called when Java platform
 * system state changes.
 *
 * @param pEventData pointer to a structure containing all information
 *                   about the event that caused the call of the listener
 */
typedef void (*MIDP_NAMS_EVENT_LISTENER) (const NamsEventData* pEventData);

/* ------------------- API to control the system ------------------- */

/**
 * Initializes the system. This function must be called before setting
 * the listeners and calling midp_system_start().
 *
 * @return error code: (ALL_OK if successful)
 */
MIDPError midp_system_initialize(void);

/**
 * Starts the system. Does not return until the system is stopped.
 *
 * @return <tt>ALL_OK</tt> if the system is shutting down or
 *         <tt>GENERAL_ERROR</tt> if an error
 */
MIDPError midp_system_start(void);

/**
 * Initiates shutdown of the system and returns immediately. System shutdown
 * is not complete until midp_system_start() returns.
 *
 * @return error code: ALL_OK if the operation was started successfully
 */
MIDPError midp_system_stop(void);

/* ------------------- API to control individual midlets ------------------- */

/**
 * Create and start the specified MIDlet. The suiteId is passed to the
 * midletsuitestorage API to retrieve the class path. The appId is assigned by
 * the caller and is used to identify this MIDlet in subsequent API calls. The
 * appId must be an integer greater than zero. The suite and class must not
 * name a MIDlet that is already running. The appId must not have already been
 * used to identify another running MIDlet.
 *
 * @param suiteId      The application suite ID
 * @param pClassName   Fully qualified name of the MIDlet class
 * @param classNameLen Length of the MIDlet class name
 * @param appId        The application id used to identify the app
 * @param pRuntimeInfo Quotas and profile to set for the new application
 *
 * @return error code: ALL_OK if the operation was started successfully,
 *                     BAD_PARAMS if suiteId is invalid or pClassName is null
 */
MIDPError midp_midlet_create_start(SuiteIdType suiteId,
                                   const jchar *pClassName, jint classNameLen,
                                   jint appId,
                                   const MidletRuntimeInfo* pRuntimeInfo);

/**
 * Create and start the specified MIDlet passing the given arguments to it.
 * The suiteId is passed to the midletsuitestorage API to retrieve the class
 * path. The appId is assigned by the caller and is used to identify this
 * MIDlet in subsequent API calls. The appId must be an integer greater
 * than zero. The suite and class must not name a MIDlet that is already
 * running. The appId must not have already been used to identify another
 * running MIDlet.
 *
 * @param suiteId      The application suite ID
 * @param pClassName   Fully qualified name of the MIDlet class
 * @param classNameLen Length of the MIDlet class name
 * @param args         An array containning up to 3 arguments for
 *                     the MIDlet to be run
 * @param argsLen      An array containing the length of each argument
 * @param argsNum      Number of arguments
 * @param appId        The application id used to identify the app
 * @param pRuntimeInfo Quotas and profile to set for the new application
 *
 * @return error code: ALL_OK if the operation was started successfully,
 *                     BAD_PARAMS if suiteId is invalid or pClassName is null
 */
MIDPError midp_midlet_create_start_with_args(SuiteIdType suiteId,
                              const jchar *pClassName, jint classNameLen,
                              const jchar **args, const jint *argsLen,
                              jint argsNum, jint appId,
                              const MidletRuntimeInfo* pRuntimeInfo);

/**
 * Resume the specified paused MIDlet.
 *
 * If appId is invalid, or if that application is already active, this call
 * has no effect and the MIDlet state change listener will be called anyway.
 *
 * @param appId The ID used to identify the application
 *
 * @return error code: ALL_OK if the operation was started successfully
 */
MIDPError midp_midlet_resume(jint appId);

/**
 * Pause the specified MIDlet.
 *
 * If appId is invalid, or if that application is already paused, this call
 * has no effect and the MIDlet state change listener will be called anyway.
 *
 * @param appId The ID used to identify the application
 *
 * @return error code: ALL_OK if the operation was started successfully
 */
MIDPError midp_midlet_pause(jint appId);

/**
 * Stop the specified MIDlet.
 *
 * If the midlet is not terminated within the given number of milliseconds,
 * it will be forcefully terminated.
 *
 * If appId is invalid, this call has no effect, but the MIDlet state change
 * listener will be called anyway.
 *
 * @param appId The ID used to identify the application
 * @param timeout Timeout in milliseconds
 *
 * @return error code: ALL_OK if the operation was started successfully
 */
MIDPError midp_midlet_destroy(jint appId, jint timeout);

/**
 * Select which running MIDlet should have the foreground.  If appId is a
 * valid application ID, that application is placed into the foreground. If
 * appId is MIDLET_APPID_NO_FOREGROUND, the current foreground MIDlet will be
 * put into background and no MIDlet will have the foreground.
 *
 * If appId is invalid, or that application already has the foreground, this
 * has no effect, but the foreground listener will be called anyway.
 *
 * @param appId The ID of the application to be put into the foreground,
 *              or the special value MIDLET_APPID_NO_FOREGROUND (that is
 *              defined in src/configuration/common/constants.xml)
 *
 * @return error code: ALL_OK if successful
 */
MIDPError midp_midlet_set_foreground(jint appId);

/**
 * Gets ID of the suite containing the specified running MIDlet.
 * This call is synchronous.
 *
 * @param appId    [in]  The ID used to identify the application
 * @param pSuiteId [out] On exit will hold an ID of the suite the midlet
 *                       belongs to
 *
 * @return error code: ALL_OK if successful,
 *                     NOT_FOUND if the application was not found,
 *                     BAD_PARAMS if pSuiteId is null
 */
MIDPError midp_midlet_get_suite_id(jint appId, SuiteIdType* pSuiteId);

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
MIDPError midp_midlet_request_runtime_info(jint appId);

/* ------------------- API to control listeners ------------------- */

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
                                  NamsListenerType listenerType);

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
MIDPError midp_remove_event_listener(MIDP_NAMS_EVENT_LISTENER listener,
                                     NamsListenerType listenerType);

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
MIDPError midp_remove_all_event_listeners(NamsListenerType listenerType);

/* @} */

#endif /* _MIDPNATIVEAPPMANAGER_H_ */
