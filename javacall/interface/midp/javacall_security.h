/*
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
#ifndef __JAVACALL_SECURITY_H
#define __JAVACALL_SECURITY_H

/**
 * @file javacall_security.h
 * @ingroup Security
 * @brief Javacall interfaces for security
 */

#include "javacall_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup Security Security API
 * @ingroup JTWI
 * @{
 */

/**
 * create a new keystore iterator instance 
 * See example in the documentation for javacall_security_keystore_get_next()
 *
 * @param handle address of pointer to file identifier
 *        on successful completion, file identifier is returned in this 
 *        argument. This identifier is platform specific and is opaque
 *        to the caller.  
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result javacall_security_keystore_start(/*OUT*/ 
                                                 javacall_handle* handle);

/**
 * check if further keystore entries exist for the current iterator
 * See example in the documentation for javacall_security_keystore_get_next()
 *
 * @param keyStoreHandle the handle of the iterator 
 *
 * @retval JAVACALL_OK if more entries exist
 * @retval JAVACALL_FAIL if no more entries exist
 */
javacall_result 
javacall_security_keystore_has_next(javacall_handle keyStoreHandle);

/**
 * get the next keystore entry record and advance the iterator
 * The implementation should supply pointers to platform-alloacated 
 * parameters and buffers.
 * The platform is responsible for any allocations of deallocations
 * of the returned pointers of all parameters:
 * Caller of this function will not free the passed pointers.
 * The out parameters are valid for usage by the caller of this function 
 * only until calling javacall_security_keystore_end() or subsequent calls
 * to javacall_security_keystore_get_next().
 *
 * Deallocation of necessary pointers can be performed by the platform in
 * the implementation of javacall_security_keystore_end() and
 * javacall_security_keystore_get_next().
 *
 * A sample usage of this function is the following:
 *
 *      void foo(void) {
 *         unsigned short*  owner;
 *         int              ownerSize,modulusSize,exponentSize,domainSize;
 *         javacall_int64   validityStartMillissec, validityEndMillisec;
 *         unsigned char*   modulus;
 *         unsigned char*   exponentSize;,
 *         char*            domain,
 *         javacall_handle h=javacall_security_iterator_start();
 *         while(javacall_security_iterator_has_next(h)==JAVACALL_OK) {
 *
 *           javacall_security_iterator_get_next(h,
 *               &owner,&ownerSize,&validityStartMillissec,
 *               &validityEndMillisec,&modulus,&modulusSize,
 *               &exponent,&exponentSize,&domain,&domainSize);
 *           //...
 *         }
 *         javacall_security_iterator_end(h);
 *
 *
 * @param keyStoreHandle the handle of the iterator 
 * @param owner a poiner to the distinguished name of the owner
 * @param ownerSize length of the distinguished name of the owner
 * @param validityStartMillissec start time of the key's validity period 
 *                  in milliseconds since Jan 1, 1970
 * @param validityEndMillisec end time of the key's validity period 
 *                  in milliseconds since Jan 1, 1970
 * @param modulus RSA modulus for the public key
 * @param modulusSize length of RSA modulus 
 * @param exponent RSA exponent for the public key
 * @param exponentSize length of RSA exponent
 * @param domain name of the security domain
 * @param domainSize length of the security domain
 * @retval JAVACALL_OK if more entries exist
 * @retval JAVACALL_FAIL if no more entries exist
 */
javacall_result 
javacall_security_keystore_get_next(javacall_handle      keyStoreHandle,
                                    unsigned short**     owner,
                                    int*                 ownerSize,
                                    javacall_int64*      validityStartMillissec,
                                    javacall_int64*      validityEndMillisec,
                                    unsigned char**      modulus,
                                    int*                 modulusSize,
                                    unsigned char**      exponent,
                                    int*                 exponentSize,
                                    char**               domain,
                                    int*                 domainSize);

/**
 * free a keystore iterator. 
 * After calling this function, the keyStoreHandle handle will not be used.
 * The implementation may perform any platfrom-sepcific deallocations.
 *
 * @param keyStoreHandle the handle of the iterator 
 * 
 * @retval JAVACALL_OK if successful
 * @retval JAVACALL_FAIL on error
 */
javacall_result 
javacall_security_keystore_end(javacall_handle keyStoreHandle);


/**
 * Native Permission dialog APIs
 */

/**
 * @enum javacall_security_permission_type
 * @brief Permission types
 */
typedef enum {
    /** Allow until exit of the current running MIDlet */
    JAVACALL_SECURITY_ALLOW_SESSION = 0x01, 
    /** Allow this time */
    JAVACALL_SECURITY_ALLOW_ONCE =    0x02, 
    /** Allow until changed the in Settings content option */
    JAVACALL_SECURITY_ALLOW_ALWAYS =  0x04, 
    /** Denied for the duration of this session */
    JAVACALL_SECURITY_DENY_SESSION =  0x08, 
    /** Denied this time only */
    JAVACALL_SECURITY_DENY_ONCE =     0x10, 
    /** Denied until changed in the Settings content option */
    JAVACALL_SECURITY_DENY_ALWAYS =   0x20  
} javacall_security_permission_type;


/**
 * @brief Permission state values
 */
#define JAVACALL_NEVER     0
#define JAVACALL_NEVER_STR "never"
#define JAVACALL_ALLOW   1
#define JAVACALL_ALLOW_STR "allow"
#define JAVACALL_BLANKET 4
#define JAVACALL_BLANKET_STR "blanket"
#define JAVACALL_SESSION 8
#define JAVACALL_SESSION_STR "session"
#define JAVACALL_ONESHOT 16
#define JAVACALL_ONESHOT_STR "oneshot"

/**
 * Invoke the native permission dialog.
 * When the native permission dialog is displayed, Java guarantees
 * no attempt will be made to refresh the screen from Java and the 
 * LCD control will be passed to the platform.
 * 
 * This function is asynchronous.
 * Return JAVACALL_WOULD_BLOCK. The notification for the dismissal
 * of the permission dialog will be sent later via notify function,
 * see javanotify_security_permission_dialog_finish().
 *
 * @param message the message the platform should display to the user.
 *                The platform must copy the message string to its own buffer.
 * @param messageLength length of message string
 * @param options the combination of permission level options 
 *                to present to the user.
 *                The options flags are any combination (bitwise-OR) 
 *                of the following:
 *                <ul>
 *                  <li> JAVACALL_SECURITY_ALLOW_SESSION </li>
 *                  <li> JAVACALL_SECURITY_ALLOW_ ONCE </li>
 *                  <li> JAVACALL_SECURITY_ALLOW_ALWAYS </li>
 *                  <li> JAVACALL_SECURITY_DENY_SESSION </li>
 *                  <li> JAVACALL_SECURITY_DENY_ ONCE </li>
 *                  <li> JAVACALL_SECURITY_DENY_ALWAYS </li>
 *                </ul>
 * 
 * The platform is responsible for providing the coresponding strings 
 * for each permission level option according to the locale.
 *       
 * @retval JAVACALL_WOULD_BLOCK this indicates that the permission
 *         dialog will be displayed by the platform.
 * @retval JAVACALL_FAIL in case prompting the permission dialog failed.
 * @retval JAVACALL_NOT_IMPLEMENTED in case the native permission dialog
 *         is not implemented by the platform. 
 * @deprecated  not used by MIDP or JAVACALL.
 * @see javacall_security_check_permission
 */
javacall_result javacall_security_permission_dialog_display(javacall_utf16* message,
                                                            int messageLength,
                                                            int options);

/**
 * The platform calls this callback notify function when the permission dialog
 * is dismissed. The platform will invoke the callback in platform context.
 *  
 * @param userPermission the permission level the user chose
 */
void javanotify_security_permission_dialog_finish(
                        javacall_security_permission_type userPermission);

/**
 * @enum javacall_security_permission
 */
/* one to one mapping with Permissions.java */
typedef enum {
  /** javax.microedition.io.Connector.http permission ID. */
  JAVACALL_SECURITY_PERMISSION_HTTP = 2,
  /** javax.microedition.io.Connector.socket permission ID. */
  JAVACALL_SECURITY_PERMISSION_SOCKET,
  /** javax.microedition.io.Connector.https permission ID. */
  JAVACALL_SECURITY_PERMISSION_HTTPS,
  /** javax.microedition.io.Connector.ssl permission ID. */
  JAVACALL_SECURITY_PERMISSION_SSL,
  /** javax.microedition.io.Connector.serversocket permission ID. */
  JAVACALL_SECURITY_PERMISSION_SERVERSOCKET,
  /** javax.microedition.io.Connector.datagram permission ID. */
  JAVACALL_SECURITY_PERMISSION_DATAGRAM,
  /** javax.microedition.io.Connector.datagramreceiver permission ID. */
  JAVACALL_SECURITY_PERMISSION_DATAGRAMRECEIVER,
  /** javax.microedition.io.Connector.comm permission ID. */
  JAVACALL_SECURITY_PERMISSION_COMM,
  /** javax.microedition.io.PushRegistry permission ID. */
  JAVACALL_SECURITY_PERMISSION_PUSH,
  /** javax.microedition.io.Connector.sms permission ID. */
  JAVACALL_SECURITY_PERMISSION_SMS,
  /** javax.microedition.io.Connector.cbs permission ID. */
  JAVACALL_SECURITY_PERMISSION_CBS,
  /** javax.wireless.messaging.sms.send permission ID. */
  JAVACALL_SECURITY_PERMISSION_SMS_SEND,
  /** javax.wireless.messaging.sms.receive permission ID. */
  JAVACALL_SECURITY_PERMISSION_SMS_RECEIVE,
  /** javax.wireless.messaging.cbs.receive permission ID. */
  JAVACALL_SECURITY_PERMISSION_CBS_RECEIVE,
  /** javax.microedition.media.RecordControl permission ID. */
  JAVACALL_SECURITY_PERMISSION_MM_RECORD,
  /** javax.microedition.media.VideoControl.getSnapshot permission ID. */
  JAVACALL_SECURITY_PERMISSION_MM_CAPTURE,
  /** javax.microedition.io.Connector.mms permission ID. */
  JAVACALL_SECURITY_PERMISSION_MMS,
  /** javax.wireless.messaging.mms.send permission ID. */ 
  JAVACALL_SECURITY_PERMISSION_MMS_SEND,
  /** javax.wireless.messaging.mms.receive permission ID. */
  JAVACALL_SECURITY_PERMISSION_MMS_RECEIVE,
  /** javax.microedition.apdu.aid permission ID. */
  JAVACALL_SECURITY_PERMISSION_APDU_CONNECTION,
  /** javax.microedition.jcrmi permission ID. */
  JAVACALL_SECURITY_PERMISSION_JCRMI_CONNECTION,
   /**
    * javax.microedition.securityservice.CMSSignatureService
    * permission ID.
    */
  JAVACALL_SECURITY_PERMISSION_SIGN_SERVICE,
  /** javax.microedition.apdu.sat permission ID. */
  JAVACALL_SECURITY_PERMISSION_ADPU_CHANNEL0,
  /** javax.microedition.content.ContentHandler permission ID. */
  JAVACALL_SECURITY_PERMISSION_CHAPI,
  /** javax.microedition.pim.ContactList.read ID. */
  JAVACALL_SECURITY_PERMISSION_PIM_CONTACT_READ,
  /** javax.microedition.pim.ContactList.write ID. */
  JAVACALL_SECURITY_PERMISSION_PIM_CONTACT_WRITE,
  /** javax.microedition.pim.EventList.read ID. */
  JAVACALL_SECURITY_PERMISSION_PIM_EVENT_READ,
  /** javax.microedition.pim.EventList.write ID. */
  JAVACALL_SECURITY_PERMISSION_PIM_EVENT_WRITE,
  /** javax.microedition.pim.ToDoList.read ID. */
  JAVACALL_SECURITY_PERMISSION_PIM_TODO_READ,
  /** javax.microedition.pim.ToDoList.write ID. */
  JAVACALL_SECURITY_PERMISSION_PIM_TODO_WRITE,
  /** javax.microedition.io.Connector.file.read ID. */
  JAVACALL_SECURITY_PERMISSION_FILE_READ,
  /** javax.microedition.io.Connector.file.write ID. */
  JAVACALL_SECURITY_PERMISSION_FILE_WRITE,
  /** javax.microedition.io.Connector.obex.client ID. */
  JAVACALL_SECURITY_PERMISSION_OBEX_CLIENT,
  /** javax.microedition.io.Connector.obex.server ID. */
  JAVACALL_SECURITY_PERMISSION_OBEX_SERVER,
  /** javax.microedition.io.Connector.obex.client.tcp ID. */
  JAVACALL_SECURITY_PERMISSION_TCP_OBEX_CLIENT,
  /** javax.microedition.io.Connector.obex.server.tcp ID. */
  JAVACALL_SECURITY_PERMISSION_TCP_OBEX_SERVER,
  /** javax.microedition.io.Connector.bluetooth.client ID. */
  JAVACALL_SECURITY_PERMISSION_BT_CLIENT,
  /** javax.microedition.io.Connector.bluetooth.server ID. */
  JAVACALL_SECURITY_PERMISSION_BT_SERVER,
  /** javax.bluetooth.RemoteDevice.authorize ID. */
  JAVACALL_SECURITY_PERMISSION_BT_AUTHORIZE,
  /** javax.microedition.location.Location ID. */
  JAVACALL_SECURITY_PERMISSION_LOC_LOCATION,
  /** javax.microedition.location.Orientation ID. */
  JAVACALL_SECURITY_PERMISSION_LOC_ORIENTATION,
  /** javax.microedition.location.ProximityListener ID. */
  JAVACALL_SECURITY_PERMISSION_LOC_PROXIMITY,
  /** javax.microedition.location.LandmarkStore.read ID. */
  JAVACALL_SECURITY_PERMISSION_LOC_LANDMARK_READ,
  /** javax.microedition.location.LandmarkStore.write ID. */
  JAVACALL_SECURITY_PERMISSION_LOC_LANDMARK_WRITE,
  /** javax.microedition.location.LandmarkStore.category ID. */
  JAVACALL_SECURITY_PERMISSION_LOC_LANDMARK_CATEGORY,
  /** javax.microedition.location.LandmarkStore.management ID. */
  JAVACALL_SECURITY_PERMISSION_LOC_LANDMARK_MANAGE,
  /** javax.microedition.io.Connector.sip permission ID. */
  JAVACALL_SECURITY_PERMISSION_SIP,
  /** javax.microedition.io.Connector.sips permission ID. */
  JAVACALL_SECURITY_PERMISSION_SIPS,
  /** javax.microedition.payment.process permission ID. */
  JAVACALL_SECURITY_PERMISSION_PAYMENT,
  /** com.qualcomm.qjae.gps.Gps permission ID. */  
  JAVACALL_SECURITY_PERMISSION_GPS,
  /** javax.microedition.media.protocol.Datasource permission ID. */
  JAVACALL_SECURITY_PERMISSION_MM_DATASOURCE,
  /** javax.microedition.media.Player permission ID. */
  JAVACALL_SECURITY_PERMISSION_MM_PLAYER,
  /** javax.microedition.media.Manager permission ID. */
  JAVACALL_SECURITY_PERMISSION_MM_MANAGER,
  
  JAVACALL_SECURITY_PERMISSION_LAST
} javacall_security_permission;

/**
 * The result of permission check.
 */
typedef enum {
    /* status can't be checked without user action */
    JAVACALL_SECURITY_UNKNOWN = -1,
    /* resource access is forbidden */
    JAVACALL_SECURITY_DENY = 0,
    /* the application is granted to access the resource */
    JAVACALL_SECURITY_GRANT = 1
}javacall_security_permission_result;

/**
 * Checks for security permission.
 * @param suite_id      the MIDlet Suite the permission should be checked with
 * @param permission_str permission string
 * @param permission_len length of the permission string
 * @param enable_block  enable user interaction. If it is
 *                      JAVACALL_FALSE the call should never be blocked.
 * @param result        address of variable to receive the
 *                      security status or the handle of check
 *                      session that blocks this call. Actual
 *                      result will be notified through
 *                      <code>javanotify_security_permission_check_result</code>.
 * 
 * @return JAVACALL_OK if check was performed correctly and
 *         result is stored at <i>result</i>,
 *         JAVACALL_FALSE if the function fails to perform
 *         checking, JAVACALL_WOULD_BLOCK if user interaction
 *         dialog is created and actual result will be delivered
 *         later.
 * @note the function MUST NOT return JAVACALL_WOULD_BLOCK if
 *       <i>enable_block</i> equals to JAVACALL_FALSE.
 *       JAVACALL_SECURITY_UNKNOWN can be returned at
 *       <i>result</i> in this case.
 * @note it is possible to have several security session in
 *       parallel.
 * @note this function is alternate to
 *       <code>javacall_security_permission_dialog_display</code>
 *       that is deprecated
 */
javacall_result
javacall_security_check_permission(const javacall_suite_id suite_id,
                                   javacall_const_utf16_string permission_str,
                                   const int permission_len,
                                   const javacall_bool enable_block,
                                   unsigned int* const result);
/**
 * Notifies the result of permission check.
 * 
 * @param suite_id      the MIDlet Suite the permission was
 *                      checked with
 * @param session       the handle of security session the java
 *                      thread waiting for
 * @param result        the result of permission check
 *                      (JAVACALL_SECURITY_GRANT - granted,
 *                       JAVACALL_SECURITY_DENY - denied)
 */
void 
javanotify_security_permission_check_result(const javacall_suite_id suite_id, 
                                            const unsigned int session,
                                            const unsigned int result);



/**
 *  Load list of security domains. The function returns in
 *  <code>array</code> an array of C strings contains the list.
 *  The caller is responsible to free the return pointer with
 *  javacall_free()
 * @param array a placeholder for the domain strings list that returns
 * @retval The number of strings in the list or 0 on error
 * 
 */
int javacall_permissions_load_domain_list(javacall_utf8_string* array);

/**
 *  Load list of groups. The function returns in
 *  <code>array</code> an array of C strings contains the list.
 *  The caller is responsible to free the return pointer with
 *  javacall_free()
 *  @param array a placeholder for the groups strings list
 *  that returns
 *  @retval The number of strings in the list or 0 on error 
 */
int javacall_permissions_load_group_list(javacall_utf8_string* array);

/** 
 * Load list of permissions members of the requested group. The 
 * function returns in <code>list</code> an array of C strings 
 *  contains the list. The caller is responsible to free the
 *  return pointer with javacall_free()
 * @param list a placeholder for the permission members strings list that 
 * returns
 * @param group_name The name of the group which members are requested
 * @retval The number of strings in the list or 0 on error 
 */
int javacall_permissions_load_group_permissions(javacall_utf8_string* list, 
                                    javacall_utf8_string group_name);

/**
 * @param domain_name the name of domain
 * @param group_name the name of the group
 * @retval Default value of permission action should
 * be taken on that domain/group combiation 
 */
int javacall_permissions_get_default_value(javacall_utf8_string domain_name,
                               javacall_utf8_string group_name);

/**
 * @param domain_name the name of domain
 * @param group_name the name of the group
 * @retval Maximum value of permission action should
 * be taken on that domain/group combiation 
 */
int javacall_permissions_get_max_value(javacall_utf8_string domain_name,
                           javacall_utf8_string group_name);

/**
 * Load list of messages that are used in the UI dialogs when
 * propmting the user of specific action of the group. The 
 * function returns in <code>list</code> an array of C strings 
 *  contains the list. The caller is responsible to free the
 *  return pointer with javacall_free()
 * @param list a placeholder for the groups strings list that returns
 * @param group_name the name of the group
 * @retval The number of strings in the list or 0 on error
 */
int javacall_permissions_load_group_messages(javacall_utf8_string* list,
                                 javacall_utf8_string group_name);

/**
 * notify implementation that loading is finished - resources 
 * can be released 
 */
void javacall_permissions_loading_finished();

/** @} */

#ifdef __cplusplus
}
#endif

#endif  /* JAVACALL_SECURITY_H */

