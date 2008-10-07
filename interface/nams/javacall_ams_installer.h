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
#ifndef __JAVACALL_AMS_INSTALLER_H
#define __JAVACALL_AMS_INSTALLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "javacall_defs.h"
#include "javacall_ams_config.h"

/*
 * Functions declared in this file are exported by the Installer.
 * They should be used in case if the Installer is implemented on
 * the MIDP side and the Application Manager UI - on the Platform's
 * side, or vice-versa.
 */


/**
 * @file javacall_ams_installer.h
 * @ingroup NAMS
 * @brief Javacall interfaces of the installer
 */

/**
 * @defgroup InstallerAPI API of the installer
 * @ingroup NAMS
 *
 *
 * @{
 */

/**
 * Codes of requests that the installer may send to the application manager.
 */
typedef enum {
    /**
     * The request is sent to show a warning to the user.
     * The type of the warning is defined by the exceptionCode field
     * of the javacall_ams_install_state structure passed in the request.
     *
     * If JAVACALL_FALSE is returned as the answer to this request,
     * the installation will be stopped and an exception with exceptionCode
     * will be thrown by the Java installer.
     */
    JAVACALL_INSTALL_REQUEST_WARNING = 1,

    /**
     * The request is sent to ask the user to confirm the jar download.
     *
     * If JAVACALL_FALSE is returned as the answer to this request, the
     * installation will be stopped and an I/O exception will be thrown
     * by the Java installer.
     */
    JAVACALL_INSTALL_REQUEST_CONFIRM_JAR_DOWNLOAD,

    /**
     * The request is sent to ask the user if he wants to keep the existing
     * RMS data for the next suite.
     *
     * In answer to this request JAVACALL_TRUE should be returned to keep
     * RMS data, JAVACALL_FALSE - to remove it.
     */
    JAVACALL_INSTALL_REQUEST_KEEP_RMS,

    /**
     * The request is sent to ask the user to confirm the authentication path.
     *
     * If JAVACALL_FALSE is returned as the answer to this request, the
     * installation will be stopped and an I/O exception will be thrown
     * by the Java installer.
     */
    JAVACALL_INSTALL_REQUEST_CONFIRM_AUTH_PATH,

    /**
     * The request is sent to ask the user to confirm if he really wants
     * to install a midlet suite from the new location.
     *
     * If JAVACALL_FALSE is returned as the answer to this request, the
     * installation will be stopped and an I/O exception will be thrown
     * by the Java installer.
     */
    JAVACALL_INSTALL_REQUEST_CONFIRM_REDIRECTION
} javacall_ams_install_request_code;

/**
 * Codes reflecting the current installation status.
 */
typedef enum {
    /** Status code to signal connection to the JAD server was successful. */
    JAVACALL_INSTALL_STATUS_DOWNLOADING_JAD = 1,

    /** Status code to signal that another 1K of the JAD has been download. */
    JAVACALL_INSTALL_STATUS_DOWNLOADED_1K_OF_JAD,

    /** Status code to signal connection to the JAR server was successful. */
    JAVACALL_INSTALL_STATUS_DOWNLOADING_JAR,

    /** Status code to signal that another 1K of the JAR has been download. */
    JAVACALL_INSTALL_STATUS_DOWNLOADED_1K_OF_JAR,

    /**
     * Status code to signal that download is done and the suite is being
     * verified.
     */
    JAVACALL_INSTALL_STATUS_VERIFYING_SUITE,

    /**
     * Status code to signal that application image is being generating.
     */
    JAVACALL_INSTALL_STATUS_GENERATING_APP_IMAGE,

    /**
     * Status code to signal that suite classes are being verified.
     */
    JAVACALL_INSTALL_STATUS_VERIFYING_SUITE_CLASSES,

    /**
     * Status code for local writing of the verified MIDlet suite.
     * Stopping the install at this point has no effect, so there user
     * should not be given a chance to stop the install.
     */
    JAVACALL_INSTALL_STATUS_STORING_SUITE,

    /** Status code for corrupted suite */
    JAVACALL_INSTALL_STATUS_CORRUPTED_SUITE,

    /** Status code to indicate that the installation is completed */
    JAVACALL_INSTALL_STATUS_COMPLETED
} javacall_ams_install_status;

/**
 * Installer exception codes.
 */
typedef enum {
    /** (-1) Unspecified general error. */
    JAVACALL_INSTALL_EXC_GENERAL_ERROR = -1,

    /** (0) There are no errors. */
    JAVACALL_INSTALL_EXC_ALL_OK,

    /** (1) The server for the JAD was not found. */
    JAVACALL_INSTALL_EXC_JAD_SERVER_NOT_FOUND,

    /** (2) The JAD was not found. */
    JAVACALL_INSTALL_EXC_JAD_NOT_FOUND,

    /** (4) The content provider certificate is missing. */
    JAVACALL_INSTALL_EXC_MISSING_PROVIDER_CERT = 4,

    /** (5) The content provider certificate cannot be decoded. */
    JAVACALL_INSTALL_EXC_CORRUPT_PROVIDER_CERT,

    /**
     * (6) The CA that issued the content provider certificate is unknown.
     * The extra data will be the CA's name as a String.
     */
    JAVACALL_INSTALL_EXC_UNKNOWN_CA,

    /**
     * (7) The signature of the content provider certificate is invalid.
     * The extra data will be the subject's name as a String.
     */
    JAVACALL_INSTALL_EXC_INVALID_PROVIDER_CERT,

    /** (8) The JAR signature cannot be decoded. */
    JAVACALL_INSTALL_EXC_CORRUPT_SIGNATURE,

    /** (9) The signature of the JAR is invalid. */
    JAVACALL_INSTALL_EXC_INVALID_SIGNATURE,

    /** (10) The content provider certificate has an unsupported version. */
    JAVACALL_INSTALL_EXC_UNSUPPORTED_CERT,

    /**
     * (11) The content provider certificate is expired.
     * The extra data will be the subject's name as a String.
     */
    JAVACALL_INSTALL_EXC_EXPIRED_PROVIDER_CERT,

    /**
     * (12) The CA's public key has expired.
     * The extra data will be the CA's name as a String.
     */
    JAVACALL_INSTALL_EXC_EXPIRED_CA_KEY,

    /** (13) The name of MIDlet suite is missing. */
    JAVACALL_INSTALL_EXC_MISSING_SUITE_NAME,

    /** (14) The vendor is missing. */
    JAVACALL_INSTALL_EXC_MISSING_VENDOR,

    /** (15) The version is missing. */
    JAVACALL_INSTALL_EXC_MISSING_VERSION,

    /** (16) The format of the version is invalid. */
    JAVACALL_INSTALL_EXC_INVALID_VERSION,

    /**
     * (17) This suite is older that the one currently installed.
     * The extra data is the installed version.
     */
    JAVACALL_INSTALL_EXC_OLD_VERSION,

    /** (18) The URL for the JAR is missing. */
    JAVACALL_INSTALL_EXC_MISSING_JAR_URL,

    /**
     * (19) The server for the JAR was not found at the URL given in
     * the JAD. The extra data is the JAR URL.
     */
    JAVACALL_INSTALL_EXC_JAR_SERVER_NOT_FOUND,

    /**
     * (20) The JAR was not found at the URL given in the JAD.
     * The extra data is the JAR URL.
     */
    JAVACALL_INSTALL_EXC_JAR_NOT_FOUND,

    /** (21) The JAR size is missing. */
    JAVACALL_INSTALL_EXC_MISSING_JAR_SIZE,

    /**
     * (25) The MIDlet suite name does not match the one in the JAR
     * manifest.
     */
    JAVACALL_INSTALL_EXC_SUITE_NAME_MISMATCH = 25,

    /** (26) The version does not match the one in the JAR manifest. */
    JAVACALL_INSTALL_EXC_VERSION_MISMATCH,

    /** (27) The vendor does not match the one in the JAR manifest. */
    JAVACALL_INSTALL_EXC_VENDOR_MISMATCH,

    /**
     * (28) A key for an attribute is not formatted correctly.
     * The extra data is the key or the line of the attribute.
     */
    JAVACALL_INSTALL_EXC_INVALID_KEY,

    /**
     * (29) A value for an attribute is not formatted correctly.
     * The extra data is the key of the attribute.
     */
    JAVACALL_INSTALL_EXC_INVALID_VALUE,

    /**
     * (30) Not enough storage for this suite to be installed
     * The extra data will be storage needed for the suite in K bytes
     * rounded up.
     */
    JAVACALL_INSTALL_EXC_INSUFFICIENT_STORAGE,

    /** (31) The JAR downloaded was not size in the JAD. */
    JAVACALL_INSTALL_EXC_JAR_SIZE_MISMATCH,

    /**
     * (32) This suite is newer that the one currently installed.
     * The extra data is the installed version.
     */
    JAVACALL_INSTALL_EXC_NEW_VERSION,

    /** (33) Webserver authentication required or failed. */
    JAVACALL_INSTALL_EXC_UNAUTHORIZED,

    /**
     * (34) The JAD URL is for an installed suite but different than the
     * original JAD URL.
     * The extra data will be previous JAD URL.
     */
    JAVACALL_INSTALL_EXC_JAD_MOVED,

    /** (35) Server does not support basic authentication. */
    JAVACALL_INSTALL_EXC_CANNOT_AUTH,

    /**
     * (36) An entry could not be read from the JAR. The extra data is the
     * entry name.
     */
    JAVACALL_INSTALL_EXC_CORRUPT_JAR,

    /**
     * (37) The server did not hava a resource with the correct type
     * (code 406) or the JAD downloaded has the wrong media type. In the
     * second case the extra data is the Media-Type from the response.
     */
    JAVACALL_INSTALL_EXC_INVALID_JAD_TYPE,

    /**
     * (38) The server did not hava a resource with the correct type
     * (code 406) or the JAR downloaded has the wrong media type. In the
     * second case the extra data is the Media-Type from the response.
     */
    JAVACALL_INSTALL_EXC_INVALID_JAR_TYPE,

    /**
     * (39) The JAD matches a version of a suite already installed.
     * The extra data is the installed version.
     */
    JAVACALL_INSTALL_EXC_ALREADY_INSTALLED,

    /**
     * (40) The device does not support either the configuration or
     * profile in the JAD.
     */
    JAVACALL_INSTALL_EXC_DEVICE_INCOMPATIBLE,

    /** (41) The configuration is missing from the manifest. */
    JAVACALL_INSTALL_EXC_MISSING_CONFIGURATION,

    /** (42) The profile is missing from the manifest. */
    JAVACALL_INSTALL_EXC_MISSING_PROFILE,

    /** (43) The JAD URL is invalid. */
    JAVACALL_INSTALL_EXC_INVALID_JAD_URL,

    /** (44) The JAR URL is invalid. The extra data is the JAR URL. */
    JAVACALL_INSTALL_EXC_INVALID_JAR_URL,

    /**
     * (45) The connection in a push entry is already taken.
     * The extra data is the URL of the failed connection.
     */
    JAVACALL_INSTALL_EXC_PUSH_DUP_FAILURE,

    /**
     * (46) The format of a push attribute has an invalid format.
     * The extra data is the URL of the failed connection.
     */
    JAVACALL_INSTALL_EXC_PUSH_FORMAT_FAILURE,

    /**
     * (47) The connection in a push attribute is not supported.
     * The extra data is the URL of the failed connection.
     */
    JAVACALL_INSTALL_EXC_PUSH_PROTO_FAILURE,

    /**
     * (48) The class in a push attribute is not in MIDlet-&lt;n&gt; attribute.
     * The extra data is the URL of the failed connection.
     */
    JAVACALL_INSTALL_EXC_PUSH_CLASS_FAILURE,

    /**
     * (49) Application authorization failure.
     * The extra data is the name of the permission.
     */
    JAVACALL_INSTALL_EXC_AUTHORIZATION_FAILURE,

    /**
     * (50) A attribute in both the JAD and JAR manifest does not match.
     * For trusted suites only. The extra data is the name of the attribute.
     */
    JAVACALL_INSTALL_EXC_ATTRIBUTE_MISMATCH,

    /**
     * (51) Indicates that the user must first authenticate with
     * the proxy.
     */
    JAVACALL_INSTALL_EXC_PROXY_AUTH,

    /**
     * (52) Indicates that the user tried to overwrite a trusted suite
     * with an untrusted suite during an update.
     * The extra data is the name of signer of the current version.
     */
    JAVACALL_INSTALL_EXC_TRUSTED_OVERWRITE_FAILURE,

    /**
     * (53) Indicates that either the JAD or manifest has too many properties
     * to fit into memory.
     */
    JAVACALL_INSTALL_EXC_TOO_MANY_PROPS,

    /**
     * (54) The MicroEdition-Handler-&lt;n&gt; attribute has invalid
     * values.  The classname may be missing or there are too many fields
     */
    JAVACALL_INSTALL_EXC_INVALID_CONTENT_HANDLER,

    /**
     * (55) The installation of a content handler would
     * conflict with an already installed handler.
     */
    JAVACALL_INSTALL_EXC_CONTENT_HANDLER_CONFLICT,

    /**
     * (56) Not all classes within JAR package can be successfully
     * verified with class verifier.
     */
    JAVACALL_INSTALL_EXC_JAR_CLASSES_VERIFICATION_FAILED,

    /**
     * (57) Indicates that the payment information provided with the MIDlet
     * suite is incompatible with the current implementation.
     */
    JAVACALL_INSTALL_EXC_UNSUPPORTED_PAYMENT_INFO,

    /**
     * (58) Indicates that the payment information provided with the MIDlet
     * suite is incomplete or incorrect.
     */
    JAVACALL_INSTALL_EXC_INVALID_PAYMENT_INFO,

    /**
     * (59) Indicates that the MIDlet suite has payment provisioning
     * information but it is not trusted.
     */
    JAVACALL_INSTALL_EXC_UNTRUSTED_PAYMENT_SUITE,

    /**
     * (60) Indicates that trusted CA for this suite has been disable for
     * software authorization. The extra data contains the CA's name.
     */
    JAVACALL_INSTALL_EXC_CA_DISABLED,

    /**
     * (61) Indicates that the character encoding specified in the MIME type
     * is not supported.
     */
    JAVACALL_INSTALL_EXC_UNSUPPORTED_CHAR_ENCODING,

    /**
     * (62) The certificate has been revoked.
     * The extra data will be the subject's name as a String.
     */
    JAVACALL_INSTALL_EXC_REVOKED_CERT,

    /**
     * (63) The certificate is unknown to OCSP server.
     * The extra data will be the subject's name as a String.
     */
    JAVACALL_INSTALL_EXC_UNKNOWN_CERT_STATUS
} javacall_ams_install_exception_code;

/**
 * Contains information about the current installation state.
 */
typedef struct _javacall_ams_install_state {
    /**
     * ID uniquely identifying this operation of installation.
     */
    javacall_app_id appId;

    /**
     * Code of the last recoverable exception that stopped the install,
     * or JAVACALL_INSTALL_EXC_ALL_OK if none.
     * Non-recoverable exceptions are thrown and not saved in the state.
     */
    javacall_ams_install_exception_code exceptionCode;

    /**
     * Unique ID of the suite in the suite storage.
     */
    javacall_suite_id suiteId;

    /**
     * Suite properties given in the JAD and in the manifest (JAD entries first)
     * as an array of strings in key/value pair order.
     */
    javacall_ams_properties suiteProperties;

    /**
     * URL of the JAR.
     */
    javacall_const_utf16_string jarUrl;

    /**
     * Label for the downloaded JAR.
     */
    javacall_const_utf16_string suiteName;

    /**
     * Expected size of the JAR, in Kbytes.
     */
    javacall_int32 jarSize;

    /**
     * Authorization path of this suite. The path starts with
     * the most trusted CA that authorized this suite.
     * It is NULL if the suite was not signed.
     */
    javacall_utf16_string* pAuthPath;

    /** Length of the authorization path. */
    javacall_int32 authPathLen;
} javacall_ams_install_state;

/**
 * Installation request/response data.
 */
typedef struct _javacall_ams_install_data {
    /*
     * The following fields are valid when this structure is used
     * as a request data.
     */

    /** Identifies the current installation step. */
    javacall_ams_install_status installStatus;
    /**
     * If the installation is completed, contains the suite ID
     * of the newly installed midlet suite.
     */
    javacall_suite_id suiteId;

    /**
     * If the installation is completed, holds the code indicating
     * the result of the installation (JAVACALL_INSTALL_EXC_ALL_OK
     * if succeeded).
     */
    javacall_ams_install_exception_code installResultCode;

    /**
     * This field is valid for JAVACALL_INSTALL_REQUEST_CONFIRM_REDIRECTION request.
     * It contains an URL where the HTTP(S) request is being redirected.
     */
    javacall_utf16_string newLocation;

    /*
     * The following fields are valid when this structure is used
     * as a response data.
     */

    /**
     * Contains the user's answer (JAVACALL_TRUE for "Yes",
     * JAVACALL_FALSE for "No") to the previously asked question.
     */
    javacall_bool fAnswer;
} javacall_ams_install_data;

/**
 * Source from where the installation is requested:
 * an URL pointing to a JAD file, to a JAR file or to any of them.
 *
 * IMPL_NOTE: when changing the following values, make sure that the values
 *            in NativeAppManagerPeer are also updated.
 */
typedef enum {
    JAVACALL_INSTALL_SRC_ANY = 0,
    JAVACALL_INSTALL_SRC_JAD = 1,
    JAVACALL_INSTALL_SRC_JAR = 2
} javacall_ams_install_source_type;

/**
 * Application manager invokes this function to start a suite installation.
 *
 * NOTE: if it is desired to invoke Graphical Installer to use Java GUI,
 *       the corresponding MIDlet should be launched instead of calling
 *       this function:
 *
 * <pre>
 *   const javacall_utf16 guiInstallerClass[] = {
 *       'c', 'o', 'm', '.', 's', 'u', 'n', '.', 'm', 'i', 'd', 'p', '.',
 *       'i', 'n', 's', 't', 'a', 'l', 'l', 'e', 'r', '.',
 *       'G', 'r', 'a', 'p', 'h', 'i', 'c', 'a', 'l',
 *       'I', 'n', 's', 't', 'a', 'l', 'l', 'e', 'r',
 *       0
 *   };
 *   const javacall_utf16 argInstallStr[] = { 'I', 0 };
 *   javacall_const_utf16_string pArgs[2];
 *
 *   pArgs[0] = argInstallStr;
 *   pArgs[1] = urlToInstallFrom;
 *
 *   res = javanotify_ams_midlet_start_with_args(-1, appId, guiInstallerClass,
 *                                               pArgs, 2, NULL);
 * </pre>
 *
 * NOTE: storageId and folderId parameters are mutually exclusive because any
 *       of them uniquely identifies where the new suite will reside; i. e.,
 *       if one of these parameters is set to a valid ID, the second must be
 *       JAVACALL_INVALID_[STORAGE|FOLDER]_ID
 *
 * @param appId ID that will be used to uniquely identify this operation
 * @param srcType
 *             type of data pointed by installUrl: a JAD file, a JAR file
 *             or any of them
 * @param installUrl
 *             null-terminated http url string of MIDlet's jad or jar file.
 *             The url is of the following form:
 *             http://www.website.com/a/b/c/d.jad
 *             or
 *             file:////a/b/c/d.jad
 *             or
 *             c:\a\b\c\d.jad
 * @param storageId ID of the storage where to install the suite or
 *                  JAVACALL_INVALID_STORAGE_ID to use the default storage
 * @param folderId ID of the folder into which to install the suite or
 *                  JAVACALL_INVALID_FOLDER_ID to use the default folder
 *
 * @return status code: <tt>JAVACALL_OK</tt> if the installation was
 *                                           successfully started,
 *                      an error code otherwise
 *
 * The return of this function only tells if the installation process is started
 * successfully. The actual result of if the installation (status and ID of the
 * newly installed suite contained in the fields of javacall_ams_install_data
 * structure) will be reported later by javacall_ams_operation_completed().
 */
javacall_result
JCDECL_APPMGR_INST(install_suite)(javacall_app_id appId,
                       javacall_ams_install_source_type srcType,
                       javacall_const_utf16_string installUrl,
                       javacall_storage_id storageId,
                       javacall_folder_id folderId);

/**
 * Application manager invokes this function to start an update of
 * an installed midlet suite.
 *
 * @param appId ID that will be used to uniquely identify this operation
 * @param suiteId ID of the midlet suite to update
 *
 * @return status code: <tt>JAVACALL_OK</tt> if the update operation was
 *                                           successfully started,
 *                      an error code otherwise
 *
 * The return of this function only tells if the update process is started
 * successfully. The actual result of if the installation (status field
 * of javacall_ams_install_data structure) will be reported later by
 * javacall_ams_operation_completed().
 */
javacall_result
JCDECL_APPMGR_INST(update_suite)(javacall_app_id appId,
                                 javacall_suite_id suiteId);

/**
 * Application manager invokes this function to enable or disable
 * certificate revocation check using OCSP.
 *
 * An installation must be started before using this method.
 * It affects all running installers.
 *
 * This call is asynchronous, the new OCSP chack state (JAVACALL_TRUE if
 * OCSP is enabled, JAVACALL_FALSE - if disabled) will be reported later
 * via javacall_ams_operation_completed() with the argument
 * operation == JAVACALL_OPCODE_ENABLE_OCSP.
 *
 * @param enable JAVACALL_TRUE to enable OCSP check,
 *               JAVACALL_FALSE - to disable it
 *
 * @return status code: <tt>JAVACALL_OK</tt> if the operation was
 *                                           successfully started,
 *                      <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
JCDECL_APPMGR_INST(install_enable_ocsp)(javacall_bool enable);

/**
 * Application manager invokes this function to find out if OCSP
 * certificate revocation check is enabled.
 *
 * An installation must be started before using this method.
 *
 * This call is asynchronous, the current OCSP check state (JAVACALL_TRUE if
 * OCSP is enabled, JAVACALL_FALSE - if disabled) will be reported later
 * via javacall_ams_operation_completed() with the argument
 * operation == JAVACALL_OPCODE_IS_OCSP_ENABLED.
 *
 * @return status code: <tt>JAVACALL_OK</tt> if the operation was
 *                                           successfully started,
 *                      <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
JCDECL_APPMGR_INST(install_is_ocsp_enabled)();

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
JCDECL_INST_APPMGR(install_report_progress)(
                             const javacall_ams_install_state* pInstallState,
                             javacall_ams_install_status installStatus,
                             int currStepPercentDone,
                             int totalPercentDone);

/**
 * This function is called by the installer when some action is required
 * from the user.
 *
 * It must be implemented at that side (Java or Platform) where the
 * application manager is located.
 *
 * After processing the request, JCDECL_APPMGR_INST(install_answer)() must
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
JCDECL_INST_APPMGR(install_ask)(javacall_ams_install_request_code requestCode,
                                const javacall_ams_install_state* pInstallState,
                                const javacall_ams_install_data* pRequestData);

/**
 * This function is called by the application manager to report the results
 * of handling of the request previously sent by
 * JCDECL_INST_APPMGR(install_ask)().
 *
 * It must be implemented at that side (Java or Platform) where the installer
 * is located.
 *
 * @param requestCode   in pair with pInstallState->appId uniquely
 *                      identifies the request for which the results
 *                      are reported by this call
 * @param pInstallState pointer to a structure containing all information
 *                      about the current installation state
 * @param pResultData   pointer to request-specific results (may NOT be NULL)
 *
 * @return <tt>JAVACALL_OK</tt> if the answer was understood,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
JCDECL_APPMGR_INST(install_answer)(
                        javacall_ams_install_request_code requestCode,
                        const javacall_ams_install_state* pInstallState,
                        const javacall_ams_install_data* pResultData);

/**
 * @defgroup ImageCache	Image Cache
 * @ingroup NAMS
 * @brief Java cache the images in JAR file to increase the performance
 *
 *
 * @{
 */

/**
 * Installer informs MIDP that the image cache should be created.
 *
 * @param suiteId unique ID of the MIDlet suite
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result
javanotify_ams_create_resource_cache(javacall_suite_id suiteId);

/** @} */
/** @} */

#ifdef __cplusplus
}
#endif

#endif  /* __JAVACALL_AMS_INSTALLER_H */
