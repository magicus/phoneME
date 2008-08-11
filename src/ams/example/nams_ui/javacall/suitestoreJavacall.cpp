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

#include <javautil_unicode.h>
#include <javacall_ams_suitestore.h>

/*----------------------- Suite Storage: Common API -------------------------*/

/**
 * Initializes the SuiteStore subsystem.
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
java_ams_suite_storage_init() {
    return JAVACALL_OK;
}

/**
 * Finalizes the SuiteStore subsystem.
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
java_ams_suite_storage_cleanup() {
    return JAVACALL_OK;
}

/**
 * Converts the given suite ID to javacall_string.
 *
 * NOTE: this function returns a pointer to a static buffer!
 *
 * @param value suite id to convert
 *
 * @return javacall_utf16_string representation of the given suite ID
 */
const javacall_utf16_string
java_ams_suite_suiteid2javacall_string(javacall_suite_id value) {
    static javacall_utf16_string jsSuiteId = {0};
    return jsSuiteId;
}

/**
 * Converts the given suite ID to array of chars.
 *
 * NOTE: this function returns a pointer to a static buffer!
 *
 * @param value suite id to convert
 *
 * @return char[] representation of the given suite ID
 *         or NULL in case of error
 */
const char*
java_ams_suite_suiteid2chars(javacall_suite_id value) {
    static char chSuiteId[64] = "";
    return chSuiteId;
}

/**
 * Tells if a suite with the given ID exists.
 *
 * @param suiteId ID of a suite
 *
 * @return <tt>JAVACALL_OK</tt> if a suite exists,
 *         <tt>JAVACALL_FILE_NOT_FOUND</tt> if not,
 *         <tt>JAVACALL_OUT_OF_MEMORY</tt> if out of memory,
 *         <tt>JAVACALL_IO_ERROR</tt> if IO error,
 *         <tt>JAVACALL_FAIL</tt> if the suite is found in the list,
 *         but it's corrupted
 */
javacall_result
java_ams_suite_exists(javacall_suite_id suiteId) {
    return JAVACALL_OK;
}

/**
 * Gets location of the class path for the suite with the specified suiteId.
 *
 * Note that memory for the in/out parameter classPath is
 * allocated by the callee. The caller is responsible for
 * freeing it using javacall_free().
 *
 * @param suiteId The application suite ID
 * @param storageId storage ID, INTERNAL_STORAGE_ID for the internal storage
 * @param checkSuiteExists true if suite should be checked for existence or not
 * @param pClassPath The in/out parameter that contains returned class path
 *
 * @return error code that should be one of the following:
 * <pre>
 *     JAVACALL_OK, JAVACALL_OUT_OF_MEMORY, JAVACALL_FILE_NOT_FOUND,
 *     JAVACALL_INVALID_ARGUMENT, JAVACALL_FAIL (for SUITE_CORRUPTED_ERROR)
 * </pre>
 */
javacall_result
java_ams_suite_get_class_path(javacall_suite_id suiteId,
                              javacall_storage_id storageId,
                              javacall_bool checkSuiteExists,
                              javacall_utf16_string* pClassPath) {
    return JAVACALL_OK;
}

#if ENABLE_MONET

/**
 * Only for MONET--Gets the class path to binary application image
 * for the suite with the specified MIDlet suite id.
 *
 * It is different from "usual" class path in that class path points to a
 * jar file, while this binary application image path points to a MONET bundle.
 *
 * Note that memory for the in/out parameter classPath is
 * allocated by the callee. The caller is responsible for
 * freeing it using javacall_free().
 *
 * @param suiteId The application suite ID
 * @param storageId storage ID, INTERNAL_STORAGE_ID for the internal storage
 * @param checkSuiteExists true if suite should be checked for existence or not
 * @param pClassPath The in/out parameter that contains returned class path
 *
 * @return error code that should be one of the following:
 * <pre>
 *     JAVACALL_OK, JAVACALL_OUT_OF_MEMORY, JAVACALL_FILE_NOT_FOUND,
 *     JAVACALL_INVALID_ARGUMENT, JAVACALL_FAIL (for SUITE_CORRUPTED_ERROR)
 * </pre>
*/
javacall_result
java_ams_suite_get_bin_app_path(javacall_suite_id suiteId,
                                javacall_storage_id storageId,
                                javacall_utf16_string* pClassPath) {
    return JAVACALL_OK;
}
#endif /* ENABLE_MONET */

/**
 * Retrieves an ID of the storage where the midlet suite with the given suite ID
 * is stored.
 *
 * @param suiteId    [in]  unique ID of the MIDlet suite
 * @param pStorageId [out] receives an ID of the storage where
 *                         the suite is stored
 *
 * @return <tt>JAVACALL_OK</tt> on success, error code otherwise
 */
javacall_result
java_ams_suite_get_storage(javacall_suite_id suiteId,
                           javacall_storage_id* pStorageId) {
    return JAVACALL_OK;
}

/**
 * Retrieves a path to the root of the given storage.
 *
 * Note that memory for the in/out parameter pStorageRootPath is
 * allocated by the callee. The caller is responsible for
 * freeing it using javacall_free().
 *
 * @param storageId        [in]  unique ID of the storage where
 *                               the midlet suite resides
 * @param pStorageRootPath [out] receives a path to the given storage's root
 *
 * @return <tt>JAVACALL_OK</tt> on success, error code otherwise
 */
javacall_result
java_ams_storage_get_root(javacall_storage_id storageId,
                          javacall_utf16_string* pStorageRootPath) {
    return JAVACALL_OK;
}

/**
 * Retrieves the specified property value of the suite.
 *
 * @param suiteId     [in]  unique ID of the MIDlet suite
 * @param key         [in]  property name
 * @param value       [out] buffer to conatain returned property value
 * @param maxValueLen [in]  buffer length of value
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result
java_ams_suite_get_property(javacall_suite_id suiteId,
                            javacall_const_utf16_string key,
                            javacall_utf16_string value,
                            int maxValueLen) {
    return JAVACALL_OK;
}

/*----------------- Suite Storage: interface to Installer -------------------*/

/**
 * Installer invokes this function. If the suite exists, this function
 * returns an unique identifier of MIDlet suite. Note that suite may be
 * corrupted even if it exists. If the suite doesn't exist, a new suite
 * ID is created.
 *
 * @param name     [in]  name of suite
 * @param vendor   [in]  vendor of suite
 * @param pSuiteId [out] suite ID of the existing suite or a new suite ID
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
java_ams_suite_get_id(javacall_const_utf16_string vendor,
                      javacall_const_utf16_string name,
                      javacall_suite_id* pSuiteId) {
    return JAVACALL_OK;
}

/**
 * Returns a new unique identifier of MIDlet suite.
 *
 * @param pSuiteId [out] receives a new unique MIDlet suite identifier
 *
 * @return <tt>JAVACALL_OK</tt> on success, else an error code
 */
javacall_result
java_ams_suite_create_id(javacall_suite_id* pSuiteId) {
    return JAVACALL_OK;
}

/**
 * Installer invokes this function to get the install information
 * of a MIDlet suite.
 *
 * Note that memory for the strings inside the returned
 * javacall_ams_suite_install_info structure is allocated by the callee,
 * and the caller is responsible for freeing it using
 * javacall_free_install_info().
 *
 * @param suiteId      [in]     unique ID of the suite
 * @param pInstallInfo [in/out] pointer to a place where the installation
 *                              information will be saved.
 *
 * @return <tt>JAVACALL_OK</tt> on success, else an error code
 */
javacall_result
java_ams_suite_get_install_info(javacall_suite_id suiteId,
                                javacall_ams_suite_install_info*
                                    pInstallInfo) {
    return JAVACALL_OK;
}

/**
 * Installer invokes this function to frees an
 * javacall_ams_suite_install_info struct.
 * Does nothing if passed NULL.
 *
 * @param pInstallInfo installation information returned from
 *                     java_ams_suite_get_install_info
 */
void java_ams_suite_free_install_info(
        javacall_ams_suite_install_info* pInstallInfo) {
}

/**
 * Installer calls this function to store or update a midlet suite.
 *
 * @param pInstallInfo structure containing the following information:<br>
 * <pre>
 *     id - unique ID of the suite;
 *     jadUrl - where the JAD came from, can be null;
 *     jarUrl - where the JAR came from;
 *     jarFilename - name of the downloaded MIDlet suite jar file;
 *     suiteName - name of the suite;
 *     suiteVendor - vendor of the suite;
 *     authPath - authPath if signed, the authorization path starting
 *                with the most trusted authority;
 *     domain - security domain of the suite;
 *     isTrusted - true if suite is trusted;
 *     verifyHash - may contain hash value of the suite with
 *                  preverified classes or may be NULL;
 *     jadProps - properties given in the JAD as an array of strings in
 *                key/value pair order, can be null if jadUrl is null
 *     jarProps - properties of the manifest as an array of strings
 *                in key/value pair order
 * </pre>
 *
 * @param pSuiteSettings structure containing the following information:<br>
 * <pre>
 *     permissions - permissions for the suite;
 *     pushInterruptSetting - defines if this MIDlet suite interrupt
 *                            other suites;
 *     pushOptions - user options for push interrupts;
 *     suiteId - unique ID of the suite, must be equal to the one given
 *               in installInfo;
 *     boolean enabled - if true, MIDlet from this suite can be run;
 * </pre>
 *
 * @param pSuiteData structure containing the following information:<br>
 * <pre>
 *     suiteId - unique ID of the suite, must be equal to the value given
 *               in installInfo and suiteSettings parameters;
 *     storageId - ID of the storage where the MIDlet should be installed;
 *     numberOfMidlets - number of midlets in the suite;
 *     displayName - the suite's name to display to the user;
 *     midletToRunClassName - the midlet's class name if the suite contains
 *                            only one midlet, ignored otherwise;
 *     iconName - name of the icon for this suite.
 * </pre>
 *
 * @return <tt>JAVACALL_OK</tt> on success, else an error code
 */
javacall_result
java_ams_suite_store_suite(const javacall_ams_suite_install_info* pInstallInfo,
                           const javacall_ams_suite_settings* pSuiteSettings,
                           const javacall_ams_suite_data* pSuiteData) {
    return JAVACALL_OK;
}

/*------------- Suite Storage: interface to Application Manager -------------*/

/**
 * App Manager invokes this function to get the number of MIDlet suites
 * currently installed.
 *
 * @param pNumbefOfSuites [out] pointer to location where the number
 *                              of the installed suites will be saved
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
java_ams_suite_get_suites_number(int* pNumbefOfSuites) {
    return JAVACALL_OK;
}

/**
 * AppManager invokes this function get the list installed of MIDlet suite IDs.
 *
 * Note that memory for the suite IDs is allocated by the callee,
 * and the caller is responsible for freeing it using
 * java_ams_free_suite_ids().
 *
 * @param ppSuiteIds      [out] on exit will hold an address of the array
                                containing suite IDs
 * @param pNumberOfSuites [out] pointer to variable to accept the number
 *                              of suites in the returned array
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_OUT_OF_MEMORY</tt> if out of memory,
 *         <tt>JAVACALL_IO_ERROR</tt> if an IO error
 */
javacall_result
java_ams_suite_get_suite_ids(javacall_suite_id** ppSuitesIds,
                             int* pNumberOfSuites) {
    return JAVACALL_OK;
}

/**
 * App Manager invokes this function to free a list of suite IDs allocated
 * during the previous call of java_ams_suite_get_suite_ids().
 *
 * @param pSuiteIds points to an array of suite IDs
 * @param numberOfSuites number of elements in pSuites
 */
void
java_ams_suite_free_suite_ids(javacall_suite_id* pSuiteIds,
                              int numberOfSuites) {
}

/**
 * App Manager invokes this function to get information about the suite
 * containing the specified running MIDlet. This call is synchronous.
 *
 * @param appId the ID used to identify the application
 *
 * @param pSuiteData [out] pointer to a structure where static information
 *                         about the midlet will be stored
 *
 * @return error code: <tt>JAVACALL_OK</tt> if successful,
 *                     <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
java_ams_suite_get_running_app_info(javacall_app_id appId,
                                    javacall_ams_suite_data* pSuiteData) {
    return JAVACALL_OK;
}

/**
 * App Manager invokes this function to get a information about the midlets
 * contained in the given suite.
 *
 * @param suiteId          [in]  unique ID of the MIDlet suite
 * @param ppMidletData     [out] on exit will hold an address of the array
 *                               containing the midlets data
 * @param pNumberOfEntries [out] number of entries in the returned array
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
java_ams_suite_get_midlets_data(javacall_suite_id suiteId,
                                javacall_ams_midlet_data** ppMidletData,
                                int* pNumberOfEntries) {
    return JAVACALL_OK;
}

/**
 * App Manager invokes this function to free an array of structures describing
 * midlets from the given suite.
 *
 * @param pMidletData points to an array with midlets data
 * @param numberOfSuites number of elements in pMidletData
 */
void
java_ams_suite_free_midlets_data(javacall_ams_midlet_data* pMidletData,
                                 int numberOfEntries) {
}

/**
 * App Manager invokes this function to get information about the midlet suite
 * having the given ID. This call is synchronous.
 *
 * @param suiteId unique ID of the MIDlet suite
 *
 * @param pSuiteData [out] pointer to a structure where the information
 *                         about the midlet will be stored
 *
 * @return error code: <tt>JAVACALL_OK</tt> if successful,
 *                     <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
java_ams_suite_get_info(javacall_suite_id suiteId,
                        javacall_ams_suite_data* pSuiteData) {
    return JAVACALL_OK;
}

/**
 * App Manager invokes this function to get the domain the suite belongs to.
 *
 * @param suiteId unique ID of the MIDlet suite
 * @param pDomainId [out] pointer to the location where the retrieved domain
 *                        information will be saved
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
java_ams_suite_get_domain(javacall_suite_id suiteId,
                          javacall_ams_domain* pDomainId) {
    return JAVACALL_OK;
}

/**
 * App Manager invokes this function to disable a suite given its suite ID.
 * <p>
 * The method does not stop the suite if is in use. However any future
 * attepts to run a MIDlet from this suite while disabled should fail.
 *
 * @param suiteId ID of the suite
 *
 * @return ALL_OK if no errors,
 *         NOT_FOUND if the suite does not exist,
 *         SUITE_LOCKED if the suite is locked,
 *         IO_ERROR if IO error has occured,
 *         OUT_OF_MEMORY if out of memory
 */
javacall_result
java_ams_suite_disable(javacall_suite_id suiteId) {
    return JAVACALL_OK;
}

/**
 * App Manager invokes this function to enable a suite given its suite ID.
 * <p>
 * The method does update an suites that are currently loaded for
 * settings or of application management purposes.
 *
 * @param suiteId ID of the suite
 *
 * @return ALL_OK if no errors,
 *         NOT_FOUND if the suite does not exist,
 *         SUITE_LOCKED if the suite is locked,
 *         IO_ERROR if IO error has occured,
 *         OUT_OF_MEMORY if out of memory
 */
javacall_result
java_ams_suite_enable(javacall_suite_id suiteId) {
    return JAVACALL_OK;
}

/**
 * Java invokes this function to get permissions of the suite.
 *
 * @param suiteId       [in]  unique ID of the MIDlet suite
 * @param pPermissions  [out] pointer to a javacall_ams_permission_set structure
 *                            to contain returned permission setttings
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result
java_ams_suite_get_permissions(javacall_suite_id suiteId,
                               javacall_ams_permission_set* pPermissions) {
    return JAVACALL_OK;
}

/**
 * App Manager invokes this function to set a single permission of the suite
 * when user changes it.
 *
 * @param suiteId     unique ID of the MIDlet suite
 * @param permission  permission be set
 * @param value       new value of permssion
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result
java_ams_suite_set_permission(javacall_suite_id suiteId,
                              javacall_ams_permission permission,
                              javacall_ams_permission_val value) {
    return JAVACALL_OK;
}

/**
 * App Manager invokes this function to set permissions of the suite.
 *
 * @param suiteId       [in]  Unique ID of the MIDlet suite
 * @param pPermissions  [out] Pointer to a javacall_ams_permission_set structure
 *                            to contain returned permission setttings
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result
java_ams_suite_set_permissions(javacall_suite_id suiteId,
                               javacall_ams_permission_set* pPermissions) {
    return JAVACALL_OK;
}

/**
 * App Manager invokes this function to remove a suite with the given ID.
 * This call is synchronous.
 *
 * @param suiteId ID of the suite to remove
 *
 * @return error code: <tt>JAVACALL_OK</tt> if successfull,
 *                     <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
java_ams_suite_remove(javacall_suite_id suiteId) {
    return JAVACALL_OK;
}

/**
 * App Manager invokes this function to move a software package with given
 * suite ID to the specified storage.
 *
 * @param suiteId suite ID for the installed package
 * @param newStorageId new storage ID
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
java_ams_suite_change_storage(javacall_suite_id suiteId,
                              javacall_storage_id newStorageId) {
    return JAVACALL_OK;
}

/**
 * App Manager invokes this function to check if the suite with the given ID
 * is trusted.
 *
 * This is just a helper method, java_ams_suite_get_info()
 * also can be used for this purpose.
 *
 * @param suiteId unique ID of the MIDlet suite
 *
 * @return <tt>JAVACALL_TRUE</tt> if the suite is trusted,
 *         <tt>JAVACALL_FALSE</tt> otherwise
 */
javacall_bool
java_ams_suite_is_preinstalled(javacall_suite_id suiteId) {
    return JAVACALL_FALSE;
}

/**
 * App Manager invokes this function to get the amount of storage
 * on the device that this suite is using.
 * This includes the JAD, JAR, management data and RMS.
 *
 * @param suiteId ID of the suite
 *
 * @return number of bytes of storage the suite is using or less than
 *         0 if out of memory
 */
long java_ams_suite_get_storage_size(javacall_suite_id suiteId) {
    return JAVACALL_OK;
}

/**
 * App Manager invokes this function to check the integrity of the suite
 * storage database and of the installed suites.
 *
 * @param fullCheck 0 to check just an integrity of the database,
 *                    other value for full check
 * @param delCorruptedSuites != 0 to delete the corrupted suites,
 *                           0 - to keep them (for re-installation).
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
java_ams_suite_check_suites_integrity(javacall_bool fullCheck,
                                      javacall_bool delCorruptedSuites) {
    return JAVACALL_OK;
}

/*------------- Getting Information About AMS Folders ---------------*/

/**
 * App Manager invokes this function to get a number of AMS folders
 * currently defined.
 *
 * @param pNumberOfFolders [out] pointer to a place where the number
 *                               of AMS folders will be stored
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
java_ams_suite_get_folders_number(int* pNumberOfFolders) {
    return JAVACALL_OK;
}

/**
 * App Manager invokes this function to get a name of AMS folder
 * with the given ID.
 *
 * Note that memory for the in/out parameter folderName is
 * allocated by the callee. The caller is responsible for
 * freeing it using javacall_free().
 *
 * @param suiteId     [in]     unique ID of the MIDlet suite
 * @param pFolderName [in/out] pointer to a place where the name of the folder
 *                             where the suite resides will be saved
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
java_ams_suite_get_folder_name(javacall_folder_id folderId,
                               javacall_utf16_string* pFolderName) {
    return JAVACALL_OK;
}

/**
 * App Manager invokes this function to get an ID of the AMS folder where
 * the given suite resides.
 *
 * @param suiteId         [in]  unique ID of the MIDlet suite
 * @param pSuiteFolderId  [out] pointer to a place where the folder ID
 *                              will be stored
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
java_ams_suite_get_folder(javacall_suite_id suiteId,
                          javacall_folder_id* pSuiteFolderId) {
    return JAVACALL_OK;
}

/**
 * App Manager invokes this function to move the given midlet suite
 * to another folder.
 *
 * @param suiteId ID of the suite
 * @param newFolderId ID of the folder where the suite must be moved
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> otherwise
 */
javacall_result
java_ams_suite_move_to_folder(javacall_suite_id suiteId,
                              javacall_folder_id newFolderId) {
    return JAVACALL_OK;
}

/*-------------------- API to read/write secure resources -------------------*/

/**
 * Reads named secure resource of the suite with specified suiteId
 * from secure persistent storage.
 *
 * Note that the implementation of this function MUST allocate the memory
 * for the in/out parameter ppReturnValue using javacall_malloc().
 * The caller is responsible for freeing it.
 *
 * @param suiteId           The suite id used to identify the MIDlet suite
 * @param resourceName      The name of secure resource to read from storage
 * @param ppReturnValue     The in/out parameter that will return the
 *                          value part of the requested secure resource
 *                          (NULL is a valid return value)
 * @param pValueSize        The length of the secure resource value
 *
 * @return one of the error codes:
 * <pre>
 *     JAVACALL_OK, JAVACALL_OUT_OF_MEMORY, JAVACALL_INVALID_ARGUMENT,
       JAVACALL_FAIL (for NOT_FOUND and SUITE_CORRUPTED_ERROR MIDP errors).
 * </pre>
 */
javacall_result
java_ams_suite_read_secure_resource(javacall_suite_id suiteId,
                                    javacall_const_utf16_string resourceName,
                                    javacall_uint8** ppReturnValue,
                                    javacall_int32* pValueSize) {
    return JAVACALL_OK;
}

/**
 * Writes named secure resource of the suite with specified suiteId
 * to secure persistent storage.
 *
 * @param suiteId           The suite id used to identify the MIDlet suite
 * @param resourceName      The name of secure resource to read from storage
 * @param pCalue            The value part of the secure resource to be stored
 * @param valueSize         The length of the secure resource value
 *
 * @return one of the error codes:
 * <pre>
 *     JAVACALL_OK, JAVACALL_OUT_OF_MEMORY, JAVACALL_INVALID_ARGUMENT,
 *     JAVACALL_FAIL (for NOT_FOUND and SUITE_CORRUPTED_ERROR MIDP errors). 
 * </pre>
 */
javacall_result
java_ams_suite_write_secure_resource(javacall_suite_id suiteId,
                           javacall_const_utf16_string resourceName,
                           javacall_uint8* pValue,
                           javacall_int32 valueSize) {
    return JAVACALL_OK;
}
