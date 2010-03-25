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

#ifndef _MIDP_SERVICES_H_
#define _MIDP_SERVICES_H_

/**
 * @defgroup core_vmservices VM Services External Interface
 * @ingroup core
 */

/**
 * @file
 * @ingroup core_vmservices
 *
 * @brief Interface to basic system services
 *
 * <p>Accessible system services including event monitoring, VM status,
 * PNG handling, JAR file handling, and so on.
 */

#include <kni.h>

#include <midpString.h>


#ifdef __cplusplus
extern "C" {
#endif

/** The type of signal for which a native thread is waiting. */
typedef enum midp_SignalType {
    NO_SIGNAL,
    UI_SIGNAL,
    DISPLAY_DEVICE_SIGNAL,
    UI_ALL_SIGNAL,
    INSTALLER_UI_SIGNAL,
    NETWORK_READ_SIGNAL,
    NETWORK_WRITE_SIGNAL,
    NETWORK_EXCEPTION_SIGNAL,
    COMM_OPEN_SIGNAL,
    COMM_READ_SIGNAL,
    COMM_WRITE_SIGNAL,
    COMM_CLOSE_SIGNAL,
    DEBUG_SIGNAL,
    PUSH_SIGNAL,
    AUDIO_SIGNAL,
    JSR211_SIGNAL,
    JSR211_PLATFORM_FINISH_SIGNAL,
    JSR211_JAVA_INVOKE_SIGNAL,
    JSR211_REQUEST_SIGNAL,
    JSR211_RESPONSE_SIGNAL,
    AMS_SIGNAL,
    VM_DEBUG_SIGNAL,
    HOST_NAME_LOOKUP_SIGNAL,
    PUSH_ALARM_SIGNAL,
    WMA_SMS_READ_SIGNAL,
    WMA_SMS_WRITE_SIGNAL,
    WMA_CBS_READ_SIGNAL,
    /* Note: There is no WRITE signal for CBS, a receive-only protocol */
    WMA_MMS_READ_SIGNAL,
    WMA_MMS_WRITE_SIGNAL,
    SECURITY_CHECK_SIGNAL,
    PAYMENT_TRANSACTION_STORE_SIGNAL,
    CARD_READER_DATA_SIGNAL,
    JSR82_SIGNAL,
    LINK_READY_SIGNAL,
    LINK_PORTAL_SIGNAL,
    JSR179_LOCATION_SIGNAL,
    MEDIA_EVENT_SIGNAL,
    JSR256_SIGNAL,
    VOLUME_SIGNAL,
    JSR290_INVALIDATE_SIGNAL,
    JSR290_FLUID_EVENT_SIGNAL,
    JSR290_INVOCATION_COMPLETION_SIGNAL,
    JSR179_PROXIMITY_SIGNAL,
    JSR179_ORIENTATION_SIGNAL,
    JSR234_SNAPSHOT_STARTED_SIGNAL,
    JSR234_TUNER_SOUGHT_SIGNAL,
    JSR257_CONTACTLESS_SIGNAL,
    JSR257_EVENT_SIGNAL,
    JSR257_PUSH_SIGNAL,
    NETWORK_STATUS_SIGNAL,
    INTERISOLATE_MUTEX_SIGNAL,
    EVENT_QUEUE_SIGNAL,
    AMMS_EVENT_SIGNAL
} midpSignalType;


/**
 * Structure to hold contextual information across thread blocking, awakening, 
 * and native method reinvocation.
 * 
 * This structure is used at three different times:
 * 
 * (1) It is populated at the time the thread is blocked.
 *
 * (2) It is read at the time an event occurs and the notifying code wishes to 
 * awaken one or more threads. 
 *
 * (3) It is available to the native method when it is reinvoked.
 *
 * The waitingFor field defines the namespace within which descriptor values 
 * reside. When searching for threads to be signaled, both the waitingFor and 
 * descriptor fields must match the arguments to the signal call.
 *
 * The status field can be set at time (2) by the notifying code in order to 
 * pass status to the reinvoked native method at time (3).
 *
 * The pResult field can be set at time (1) for use at time (3). The notifying
 * code at time (2) should not set or get this field. The reason is that
 * pResult may require native memory allocation, and notification may occur on
 * zero or many threads, which makes dealing with native memory allocation 
 * difficult.
 */
typedef struct _MidpReentryData {
    midpSignalType waitingFor;   /**< type of signal */
    int descriptor;              /**< platform specific handle */
    int status;                  /**< error code produced by the operation
                                      that unblocked the thread */
    void* pResult;               /**< platform specific context info used
                                       by thread wait functions etc. */
} MidpReentryData;

/**
 * Immediately terminates the VM.
 * <p>
 * <b>NOTE:</b> This may not necessarily terminate the process.
 *
 * @param status The return code of the VM.
 */
void midp_exitVM(int status);

/**
 * Gets the current system time in milliseconds.
 *
 * @return The current system time in milliseconds
 */
jlong midp_getCurrentTime(void);

/**
 * Reads an entry from a JAR file.
 *
 * @param jarName The name of the JAR file to read
 * @param entryName The name of the file inside the JAR file to read
 * @param entry An <tt>Object</tt> representing the entry
 *
 * @return <tt>true</tt> if the entry was read, otherwise <tt>false</tt>
 */
jboolean midp_readJarEntry(const pcsl_string* jarName, const pcsl_string* entryName,
                           jobject* entry);

#if ENABLE_JAVA_DEBUGGER
/** Determines if the debugger is active. */
jboolean midp_isDebuggerActive(void);
#endif /* ENABLE_JAVA_DEBUGGER */

/**
 * Gets the current isolate ID from VM in case of MVM mode.
 * For SVM, simply return 0 as an isolate ID.
 *
 * @return isolated : Isolate ID
 * 
 */
int getCurrentIsolateId();

/**
 * Gets maximal allowed number of isolates in the case of MVM mode.
 * For SVM, simply return 1.
 */
int getMaxIsolates();


#ifdef __cplusplus
}
#endif

/* @} */

#endif /* _MIDP_SERVICES_H_ */
