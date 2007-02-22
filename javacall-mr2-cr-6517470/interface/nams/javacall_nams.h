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
#ifndef __JAVACALL_NAMS_H
#define __JAVACALL_NAMS_H



#ifdef __cplusplus
extern "C" {
#endif

#include "javacall_defs.h"

/**
 * @file javacall_nams.h
 * @ingroup MandatoryNAMS
 * @brief Javacall interfaces for nams
 */

/**
 * @defgroup MandatoryNAMS Native AMS APIs
 * @ingroup JTWI
 *
 *
 *  @{
 */

/**
 * @enum javacall_midlet_state
 *
 * IMPL_NOTE: currently a number of MIDP structures and constants
 *            (most of the bellowing) are duplicated in javacall.
 *            A possibility to avoid it via using some synchronization
 *            mechanism like Configurator tool should be considered.
 */
typedef enum {
    /** MIDlet active */
    JAVACALL_MIDLET_STATE_ACTIVE,
    /** MIDlet paused */
    JAVACALL_MIDLET_STATE_PAUSED,
    /** MIDlet destroyed */
    JAVACALL_MIDLET_STATE_DESTROYED,
    /** MIDlet error */
    JAVACALL_MIDLET_STATE_ERROR
} javacall_midlet_state;

/**
 * @enum javacall_midlet_ui_state
 */
typedef enum {
    /** MIDlet being in foreground */
    JAVACALL_MIDLET_UI_STATE_FOREGROUND,
    /** MIDlet being in background */
    JAVACALL_MIDLET_UI_STATE_BACKGROUND,
    /** MIDlet is requesting foreground */
    JAVACALL_MIDLET_UI_STATE_FOREGROUND_REQUEST,
    /** MIDlet is requesting background */
    JAVACALL_MIDLET_UI_STATE_BACKGROUND_REQUEST
} javacall_midlet_ui_state;

/**
 * @enum javacall_opcode
 */
typedef enum {
    /** Request of run-time information on an application */
    JAVACALL_REQUEST_RUNTIME_INFO
} javacall_opcode;

/**
 * @enum javacall_change_reason
 */
typedef enum {
    /**
     * MIDlet start error status, when a MIDlet's constructor
     * fails to catch a runtime exception.
     */
    JAVACALL_MIDLET_CONSTRUCTOR_FAILED,

    /** MIDlet start error status, when a MIDlet suite is not found */
    JAVACALL_MIDLET_SUITE_NOT_FOUND,

    /**
     * MIDlet start error status, when a class needed to create a MIDlet
     * is not found.
     */
    JAVACALL_MIDLET_CLASS_NOT_FOUND,

    /**
     * MIDlet start error status, when intantiation exception is
     * thrown during the intantiation of a MIDlet.
     */
    JAVACALL_MIDLET_INSTANTIATION_EXCEPTION,

    /**
     * MIDlet start error status, when illegal access exception
     * is thrown during the intantiation of a MIDlet.
     */
    JAVACALL_MIDLET_ILLEGAL_ACCESS_EXCEPTION,

    /**
     * MIDlet start error status, when a MIDlet's constructor
     * runs out of memory.
     */
    JAVACALL_MIDLET_OUT_OF_MEM_ERROR,

    /**
     * MIDlet start error status, when a the system cannot
     * reserve enough resource to start a MIDlet suite.
     */
    JAVACALL_MIDLET_RESOURCE_LIMIT,

    /**
     * MIDlet start error status, when system has exceeded
     * the maximum Isolate count.
     */
    JAVACALL_MIDLET_ISOLATE_RESOURCE_LIMIT,

    /**
     * MIDlet start error status, when a MIDlet's isolate
     * constructor throws to catch a runtime exception.
     */
    JAVACALL_MIDLET_ISOLATE_CONSTRUCTOR_FAILED,
    
    /** MIDlet was forced to be terminated */
    JAVACALL_MIDP_REASON_TERMINATED,

    /** MIDlet exit */
    JAVACALL_MIDP_REASON_EXIT,

    /** Other */
    JAVACALL_MIDP_REASON_OTHER
} javacall_change_reason;

/**
 * @enum javacall_ams_domain
 */
typedef enum {
    /** Untrusted domain, the MIDlet is untrusted MIDlet. */
    JAVACALL_AMS_DOMAIN_UNTRUSTED = 0,
    /** The MIDlet belong to trusted domain. */
    JAVACALL_AMS_DOMAIN_TRUSTED = 1
} javacall_ams_domain;

/**
 * @enum javacall_ams_permission
 */
/* one to one mapping with Permissions.java */
typedef enum {
  /** javax.microedition.io.Connector.http permission ID. */
  JAVACALL_AMS_PERMISSION_HTTP = 2,
  /** javax.microedition.io.Connector.socket permission ID. */
  JAVACALL_AMS_PERMISSION_SOCKET,
  /** javax.microedition.io.Connector.https permission ID. */
  JAVACALL_AMS_PERMISSION_HTTPS,
  /** javax.microedition.io.Connector.ssl permission ID. */
  JAVACALL_AMS_PERMISSION_SSL,
  /** javax.microedition.io.Connector.serversocket permission ID. */
  JAVACALL_AMS_PERMISSION_SERVERSOCKET,
  /** javax.microedition.io.Connector.datagram permission ID. */
  JAVACALL_AMS_PERMISSION_DATAGRAM,
  /** javax.microedition.io.Connector.datagramreceiver permission ID. */
  JAVACALL_AMS_PERMISSION_DATAGRAMRECEIVER,
  /** javax.microedition.io.Connector.comm permission ID. */
  JAVACALL_AMS_PERMISSION_COMM,
  /** javax.microedition.io.PushRegistry permission ID. */
  JAVACALL_AMS_PERMISSION_PUSH,
  /** javax.microedition.io.Connector.sms permission ID. */
  JAVACALL_AMS_PERMISSION_SMS,
  /** javax.microedition.io.Connector.cbs permission ID. */
  JAVACALL_AMS_PERMISSION_CBS,
  /** javax.wireless.messaging.sms.send permission ID. */
  JAVACALL_AMS_PERMISSION_SMS_SEND,
  /** javax.wireless.messaging.sms.receive permission ID. */
  JAVACALL_AMS_PERMISSION_SMS_RECEIVE,
  /** javax.wireless.messaging.cbs.receive permission ID. */
  JAVACALL_AMS_PERMISSION_CBS_RECEIVE,
  /** javax.microedition.media.RecordControl permission ID. */
  JAVACALL_AMS_PERMISSION_MM_RECORD,
  /** javax.microedition.media.VideoControl.getSnapshot permission ID. */
  JAVACALL_AMS_PERMISSION_MM_CAPTURE,
  /** javax.microedition.io.Connector.mms permission ID. */
  JAVACALL_AMS_PERMISSION_MMS,
  /** javax.wireless.messaging.mms.send permission ID. */ 
  JAVACALL_AMS_PERMISSION_MMS_SEND,
  /** javax.wireless.messaging.mms.receive permission ID. */
  JAVACALL_AMS_PERMISSION_MMS_RECEIVE,
  /** javax.microedition.apdu.aid permission ID. */
  JAVACALL_AMS_PERMISSION_APDU_CONNECTION,
  /** javax.microedition.jcrmi permission ID. */
  JAVACALL_AMS_PERMISSION_JCRMI_CONNECTION,
   /**
    * javax.microedition.securityservice.CMSSignatureService
    * permission ID.
    */
  JAVACALL_AMS_PERMISSION_SIGN_SERVICE,
  /** javax.microedition.apdu.sat permission ID. */
  JAVACALL_AMS_PERMISSION_ADPU_CHANNEL0,
  /** javax.microedition.content.ContentHandler permission ID. */
  JAVACALL_AMS_PERMISSION_CHAPI,
  /** javax.microedition.pim.ContactList.read ID. */
  JAVACALL_AMS_PERMISSION_PIM_CONTACT_READ,
  /** javax.microedition.pim.ContactList.write ID. */
  JAVACALL_AMS_PERMISSION_PIM_CONTACT_WRITE,
  /** javax.microedition.pim.EventList.read ID. */
  JAVACALL_AMS_PERMISSION_PIM_EVENT_READ,
  /** javax.microedition.pim.EventList.write ID. */
  JAVACALL_AMS_PERMISSION_PIM_EVENT_WRITE,
  /** javax.microedition.pim.ToDoList.read ID. */
  JAVACALL_AMS_PERMISSION_PIM_TODO_READ,
  /** javax.microedition.pim.ToDoList.write ID. */
  JAVACALL_AMS_PERMISSION_PIM_TODO_WRITE,
  /** javax.microedition.io.Connector.file.read ID. */
  JAVACALL_AMS_PERMISSION_FILE_READ,
  /** javax.microedition.io.Connector.file.write ID. */
  JAVACALL_AMS_PERMISSION_FILE_WRITE,
  /** javax.microedition.io.Connector.obex.client ID. */
  JAVACALL_AMS_PERMISSION_OBEX_CLIENT,
  /** javax.microedition.io.Connector.obex.server ID. */
  JAVACALL_AMS_PERMISSION_OBEX_SERVER,
  /** javax.microedition.io.Connector.obex.client.tcp ID. */
  JAVACALL_AMS_PERMISSION_TCP_OBEX_CLIENT,
  /** javax.microedition.io.Connector.obex.server.tcp ID. */
  JAVACALL_AMS_PERMISSION_TCP_OBEX_SERVER,
  /** javax.microedition.io.Connector.bluetooth.client ID. */
  JAVACALL_AMS_PERMISSION_BT_CLIENT,
  /** javax.microedition.io.Connector.bluetooth.server ID. */
  JAVACALL_AMS_PERMISSION_BT_SERVER,
  /** javax.bluetooth.RemoteDevice.authorize ID. */
  JAVACALL_AMS_PERMISSION_BT_AUTHORIZE,
  /** javax.microedition.location.Location ID. */
  JAVACALL_AMS_PERMISSION_LOC_LOCATION,
  /** javax.microedition.location.Orientation ID. */
  JAVACALL_AMS_PERMISSION_LOC_ORIENTATION,
  /** javax.microedition.location.ProximityListener ID. */
  JAVACALL_AMS_PERMISSION_LOC_PROXIMITY,
  /** javax.microedition.location.LandmarkStore.read ID. */
  JAVACALL_AMS_PERMISSION_LOC_LANDMARK_READ,
  /** javax.microedition.location.LandmarkStore.write ID. */
  JAVACALL_AMS_PERMISSION_LOC_LANDMARK_WRITE,
  /** javax.microedition.location.LandmarkStore.category ID. */
  JAVACALL_AMS_PERMISSION_LOC_LANDMARK_CATEGORY,
  /** javax.microedition.location.LandmarkStore.management ID. */
  JAVACALL_AMS_PERMISSION_LOC_LANDMARK_MANAGE,
  /** javax.microedition.io.Connector.sip permission ID. */
  JAVACALL_AMS_PERMISSION_SIP,
  /** javax.microedition.io.Connector.sips permission ID. */
  JAVACALL_AMS_PERMISSION_SIPS,
  /** javax.microedition.payment.process permission ID. */
  JAVACALL_AMS_PERMISSION_PAYMENT,
  /** com.qualcomm.qjae.gps.Gps permission ID. */  
  JAVACALL_AMS_PERMISSION_GPS,
  /** javax.microedition.media.protocol.Datasource permission ID. */
  JAVACALL_AMS_PERMISSION_MM_DATASOURCE,
  /** javax.microedition.media.Player permission ID. */
  JAVACALL_AMS_PERMISSION_MM_PLAYER,
  /** javax.microedition.media.Manager permission ID. */
  JAVACALL_AMS_PERMISSION_MM_MANAGER,
  
  JAVACALL_AMS_PERMISSION_LAST
} javacall_ams_permission;

/**
 * @enum javacall_ams_permission_val
 */
typedef enum {
  JAVACALL_AMS_PERMISSION_VAL_INVALID = -1,  
  JAVACALL_AMS_PERMISSION_VAL_NEVER = 0,
  JAVACALL_AMS_PERMISSION_VAL_ALLOW = 1,
  JAVACALL_AMS_PERMISSION_VAL_BLANKET_GRANTED = 2,
  JAVACALL_AMS_PERMISSION_VAL_BLANKET = 4,  
  JAVACALL_AMS_PERMISSION_VAL_SESSION = 8,  
  JAVACALL_AMS_PERMISSION_VAL_ONE_SHOT = 16
} javacall_ams_permission_val;

/**
 * @brief Holds all permssion set
 */
typedef struct _javacall_ams_permission_set {
    /** Array of javacall_ams_permission_val */
    javacall_ams_permission_val permission[JAVACALL_AMS_PERMISSION_LAST];
} javacall_ams_permission_set;

/**
 * Structure where run time information about the midlet will be placed.
 */
typedef struct _javacall_midlet_runtime_info {
    /**
     * The minimum amount of memory guaranteed to be available to the isolate
     * at any time. Used to pass a parameter to midlet_create_start(),
     * < 0 if not used.
     */
    javacall_int32 memoryReserved;

    /**
     * The total amount of memory that the isolate can reserve.
     * Used to pass a parameter to midlet_create_start(), < 0 if not used.
     */
    javacall_int32 memoryTotal;

    /**
     * The approximate amount of object heap memory currently
     * used by the isolate.
     */
    javacall_int32 usedMemory;

    /**
     * Priority of the isolate (< 0 if not set).
     */
    javacall_int32 priority;

    /**
     * Name of the VM profile that should be used for the new isolate.
     * Used (1) to pass a parameter to midlet_create_start();
     * (2) to get a profile's name of the given isolate in run time.
     */
    javacall_utf16_string profileName;
} javacall_midlet_runtime_info;

/**
 * A structure containing variable-length information (such as
 * a suite name and vendor name)about the installed midlet suites.
 */
typedef struct _javacall_ams_variable_len_suite_data {
    /** Hash value (currently MD5) of the suite's jar file. */
    unsigned char* pJarHash;

    /**
     * jint (length) + UTF16 string
     * Class name of the midlet in single-midlet suite.
     * Not used (length field is 0) if the suite contains several midlets.
     */
    javacall_utf16_string midletClassName;

    /**
     * jint (length) + UTF16 string
     * A name that will be displayed in the Application Manager.
     */
    javacall_utf16_string displayName;

    /**
     * jint (length) + UTF16 string
     * Icon's name for this suite.
     */
    javacall_utf16_string iconName;

    /**
     * jint (length) + UTF16 string
     * Vendor of the midlet suite.
     */
    javacall_utf16_string suiteVendor;

    /**
     * jint (length) + UTF16 string
     * Name of the midlet suite.
     */
    javacall_utf16_string suiteName;

    /**
     * jint (length) + UTF16 string
     * Full path to suite's jar file.
     */
    javacall_utf16_string pathToJar;

    /**
     * jint (length) + UTF16 string
     * Full path to the settings files.
     */
    javacall_utf16_string pathToSettings;
} javacall_ams_variable_len_suite_data;

/**
 * @brief Holds all permssion set
 */
typedef struct _javacall_ams_suite_data {
    /**
     * Unique ID of the midlet suite
     * (0 means that this entry was removed).
     */
    javacall_suite_id suiteId;

    /**
     * ID of the storage (INTERNAL_STORAGE_ID for the internal storage
     * or another value for external storages).
     */
    javacall_int32 storageId;

    /** True if the suite enabled, false otherwise. */
    javacall_bool isEnabled;

    /** True if the suite is trusted, false otherwise. */
    javacall_bool isTrusted;

    /** Number of midlets in this suite. */
    javacall_int32 numberOfMidlets;

    /** Installation time (timestamp). */
    long installTime;

    /** Size of the midlet suite's jad file. */
    javacall_int32 jadSize;
    
    /** Size of the midlet suite's jar file. */
    javacall_int32 jarSize;

    /** Size of the jar file hash. If it is 0, pJarHash field is empty. */
    javacall_int32 jarHashLen;

    /**
     * True if this midlet suite is preinstalled (and thus should be
     * prevented from being removed.
     */
    javacall_bool isPreinstalled;

    /** A structure with string-represented information about the suite. */
    javacall_ams_variable_len_suite_data varSuiteData;
} javacall_ams_suite_data;

/**
 * @brief running midlet unique ID
 */
typedef int javacall_app_id;

/**
 * Platform invokes this function to inform VM to start a specific MIDlet
 * suite. 
 *
 * @param suiteID      ID of the suite to start
 * @param appID        ID of runtime midlet, ID must not be Zero
 * @param jarName      The path to JAR file
 * @param className    Fully qualified name of the MIDlet class
 * @param pRuntimeInfo Quotas and profile to set for the new application
 * @return <tt>JAVACALL_OK</tt> if all parameter are valid,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 * @note this function just checks the parameters accuracy,
 *       the real status of MIDlet startup will be notified
 *       by <link>javacall_ams_midlet_stateChanged</link>
 */
javacall_result
javanotify_ams_midlet_start(const javacall_suite_id suiteID,
                            const javacall_app_id appID,
                            const javacall_utf16_string jarName,
                            const javacall_utf16_string className,
                            const javacall_midlet_runtime_info* pRuntimeInfo);

/**
 * Platform invokes this function to inform VM to start a specific MIDlet
 * suite with arguments. 
 *
 * @param suiteID      ID of the suite to start with
 * @param appID        ID of runtime midlet
 * @param jarName      The path to JAR file
 * @param className    Fully qualified name of the MIDlet class
 * @param args         An array containning up to 3 arguments for
 *                     the MIDlet to be run
 * @param argsNum      Number of arguments
 * @param pRuntimeInfo Quotas and profile to set for the new application
 * @return <tt>JAVACALL_OK</tt> if all parameter are valid,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 * @note  this function just checks the parameters accuracy, 
 *        the real status of MIDlet startup will be notified by
 *        <link>javacall_ams_midlet_stateChanged</link>
 */
javacall_result
javanotify_ams_midlet_start_with_args(const javacall_suite_id suiteID,
                                      const javacall_app_id appID,
                                      const javacall_utf16_string jarName,
                                      const javacall_utf16_string className,
                                      const javacall_utf16_string *args,
                                      int argsNum,
                                      const javacall_midlet_runtime_info*
                                          pRuntimeInfo);

/**
 * Platform invokes this function to inform VM to shutdown a specific
 * running MIDlet. If it doesn't exit in the specified amount of milliseconds,
 * it will be forcefully terminated.
 *
 * @param appID appID of the suite to shutdown
 * @param timeoutMillSecond shutdown the suite in timeout millseconds
 * @return <tt>JAVACALL_OK</tt> if <code>suiteID</code> has a proper value
 *         <tt>JAVACALL_FAIL</tt> otherwise
 * @note the real status of operation will be notified by
 *       <link>javacall_ams_midlet_stateChanged</link>
 */
javacall_result
javanotify_ams_midlet_shutdown(const javacall_app_id appID,
                               int timeoutMillSeconds);

/**
 * Platform invokes this function to inform VM to switch a specific MIDlet
 * suite to foreground.
 *
 * @param appID appID of the suite to switch
 *
 * @return <tt>JAVACALL_OK</tt> if <code>suiteID</code> has a proper value
 *         <tt>JAVACALL_FAIL</tt> otherwise
 * @note the real status of operation will be notified by
 *       <link>javacall_ams_midlet_stateChanged</link>
 */
javacall_result
javanotify_ams_midlet_switch_foreground(const javacall_app_id appID);

/**
 * Platform invokes this function to inform VM to switch current MIDlet
 * suite to background, and no MIDlet will switch to foregound.
 *
 * @return <tt>JAVACALL_OK</tt> if <code>suiteID</code> has a proper value
 *         <tt>JAVACALL_FAIL</tt> otherwise
 * @note the real status of operation will be notified by
 *       <link>javacall_ams_midlet_stateChanged</link>
 */
javacall_result javanotify_ams_midlet_switch_background();

/**
 * Platform invokes this function to inform VM to pause a specific MIDlet
 *
 * @param appID appID of the suite to pause 
 * @return <tt>JAVACALL_OK</tt> if <code>suiteID</code> has a proper value
 *         <tt>JAVACALL_FAIL</tt> otherwise
 * @note the real status of operation will be notified by
 *       <link>javacall_ams_midlet_stateChanged</link>
 */
javacall_result javanotify_ams_midlet_pause(const javacall_app_id appID);

/**
 * Platform invokes this function to inform VM to resume a specific MIDlet
 *
 * @param appID appID of the suite to resume 
 * @return <tt>JAVACALL_OK</tt> if <code>suiteID</code> has a proper value
 *         <tt>JAVACALL_FAIL</tt> otherwise
 * @note the real status of operation will be notified by
 *       <link>javacall_ams_midlet_stateChanged</link>
 */
javacall_result javanotify_ams_midlet_resume(const javacall_app_id appID);

/**
 * Platform invokes this function to get information about the suite containing
 * the specified running MIDlet. This call is synchronous.
 *
 * @param appId The ID used to identify the application
 *
 * @param pSuiteData [out] pointer to a structure where static information
 *                         about the midlet will be stored
 *
 * @return error code: <tt>JAVACALL_OK</tt> if successful,
 *                     <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
javanotify_ams_midlet_get_suite_info(const javacall_app_id appID,
                                     javacall_ams_suite_data* pSuiteData);

/**
 * Platform invokes this function to get runtime information
 * about the specified MIDlet.
 *
 * This call is asynchronous, the result will be reported later through
 * passing a MIDLET_INFO_READY_EVENT event to SYSTEM_EVENT_LISTENER.
 *
 * @param appID The ID used to identify the application
 *
 * @return error code: <tt>JAVACALL_OK<tt> if successful (operation started),
 *                     <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
javanotify_ams_midlet_request_runtime_info(const javacall_app_id appID);

/**
 * Inform on completion of the previously requested operation.
 *
 * @param appID The ID used to identify the application
 * @param pResult Pointer to a static buffer containing
 *                operation-dependent result
 */
void javacall_ams_operation_completed(javacall_opcode operation,
                                      const javacall_app_id appID,
                                      void* pResult);

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
void javacall_ams_midlet_state_changed(javacall_midlet_state state,
                                       const javacall_app_id appID,
                                       javacall_change_reason reason);

/**
 * Inform on change of the specific MIDlet's lifecycle status.
 *
 * Java will invoke this function whenever the running MIDlet has switched
 * to foreground or background.
 *
 * @param state new state of the running MIDlet. Can be either
 *        <tt>JAVACALL_MIDLET_UI_STATE_FOREGROUND</tt>,
 *        <tt>JAVACALL_MIDLET_UI_STATE_BACKGROUND</tt>
 *        <tt>JAVACALL_MIDLET_UI_STATE_FOREGROUND_REQUEST</tt>,
 *        <tt>JAVACALL_MIDLET_UI_STATE_BACKGROUND_REQUEST</tt>
 * @param appID The ID of the state-changed suite
 * @param reason The reason why the state change has happened
 */
void javacall_ams_ui_state_changed(javacall_midlet_ui_state state,
                                   const javacall_app_id appID,
                                   javacall_change_reason reason);

/**
 * Get path name of the directory which holds suite's RMS files 
 * @param suiteID Unique ID of the MIDlet suite.
 * @param path    A buffer allocated to contain the returned path name string.
 *                The returned string must be double-'\0' terminated.
 * @param maxPath Buffer length of path
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result javacall_ams_get_rms_path(javacall_suite_id suiteID, 
                                          javacall_utf16_string path, 
                                          int maxPath);

/**
 * Get domain information of the suite
 * @param suiteID Unique ID of the MIDlet suite.
 * @param pDomain Pointer to a javacall_ext_ams_domain to contain returned
 *                domain information. Only Trusted or Untrusted domain is
 *                required to be returned.
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result javacall_ams_get_domain(javacall_suite_id suiteID,
                                        javacall_ams_domain* pDomain);

/**
 * Get permission set of the suite
 * @param suiteID       Unique ID of the MIDlet suite
 * @param pPermissions  Pointer to a javacall_ext_ams_permission_set structure
 *                      to contain returned permission setttings
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result
javacall_ams_get_permissions(javacall_suite_id suiteID,
                             javacall_ams_permission_set* pPermissions);

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
                            javacall_ams_permission_val value);

/**
 * Set permission set of the suite
 * @param suiteID       Unique ID of the MIDlet suite
 * @param pPermissions  Pointer to a javacall_ext_ams_permission_set structure
 *                      to contain returned permission setttings
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result
javacall_ams_set_permissions(javacall_suite_id suiteID,
                             javacall_ams_permission_set* pPermissions);

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
                                int maxValue);

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
                          javacall_suite_id* pSuiteID);


/**
 * @defgroup ImageCache	Image Cache
 * @ingroup NAMS
 * @brief Java cache the images in JAR file to increase the performance
 *
 *
 * @{
 */

/**
 * Platform inform the VM to create the images cache.
 * @param suiteID  unique ID of the MIDlet suite
 * @param jarName  the Jar file name of the MIDlet suite
 *
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result
javanotify_ams_create_resource_cache(const javacall_suite_id suiteID,
                                     const javacall_utf16_string jarName);

/**
 * VM invokes this function to get the image cache path.
 * @param suiteID   unique ID of the MIDlet suite
 * @param cachePath buffer for Platform store the image cache path.
 * @param cachePathLen the length of cachePath
 *
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result
javacall_ams_get_resource_cache_path(const javacall_suite_id suiteID,
                                     javacall_utf16_string cachePath,
                                     int cachePathLen);

/** @} */
/** @} */

#ifdef __cplusplus
}
#endif

#endif  /* JAVACALL_NAMS_H */
