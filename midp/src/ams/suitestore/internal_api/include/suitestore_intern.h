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

/**
 * @file
 *
 * This header file is interface to the internal MIDlet suite storage
 * functions.
 */

#ifndef _SUITESTORE_INTERN_H_
#define _SUITESTORE_INTERN_H_

#include <kni.h>
#include <suitestore_locks.h>
#include <suitestore_installer.h>

#if VERIFY_ONCE
    #include <suitestore_verify_hash.h>
#endif

#if ENABLE_CONTROL_ARGS_FROM_JAD
    #include <midp_jad_control_args.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Transaction types.
 */
typedef enum {
    TRANSACTION_INSTALL_SUITE,
    TRANSACTION_REMOVE_SUITE,
    TRANSACTION_ENABLE_SUITE,
    TRANSACTION_CHANGE_STORAGE,
    TRANSACTION_MOVE_TO_FOLDER,
    TRANSACTION_INSTALL_COMPONENT,
    TRANSACTION_REMOVE_COMPONENT,
    TRANSACTION_REMOVE_ALL_COMPONENTS,
    /** force enum to be 4 bytes */
    TRANSACTION_DUMMY = 0x10000000
} MIDPTransactionType;

/*
 * Macro to align the given value on 4-bytes boundary.
 */
#define SUITESTORE_ALIGN_4(x) ( (((x)+3) >> 2) << 2 )

/*
 * Number of files to check in check_for_corrupted_suite()
 * and for midp_get_suite_storage_size() Task Manager API function.
 */
#define NUM_SUITE_FILES 4

/** Cache of the last suite the exists function found. */
extern SuiteIdType g_lastSuiteExistsID;

#if ENABLE_DYNAMIC_COMPONENTS
/** Cache of the last component that midp_component_exists() function found. */
extern ComponentIdType g_lastComponentExistsID;
#endif

/** Number of the installed midlet suites and dynamic components. */
extern int g_numberOfSuites;

/** List of structures with the information about the installed suites. */
extern MidletSuiteData* g_pSuitesData;

/**
 * Allocates a memory buffer enough to hold the whole file
 * and reads the given file into the buffer.
 * File contents is read as one piece.
 *
 * @param ppszError pointer to character string pointer to accept an error
 * @param pFileName file to read
 * @param outBuffer receives the address of a buffer where the file contents are stored, NULL on error
 * @param outBufferLen receives the length of outBuffer, 0 on error
 *
 * @return status code (ALL_OK if there was no errors)
 */
MIDPError
read_file(char** ppszError, const pcsl_string* pFileName,
          char** outBuffer, long* outBufferLen);

/**
 * Writes the contents of the given buffer into the given file.
 *
 * Note that if the length of the input buffer is zero or less,
 * the file will be truncated.
 *
 * @param ppszError pointer to character string pointer to accept an error
 * @param pFileName file to write
 * @param inBuffer buffer with data that will be stored
 * @param inBufferLen length of the inBuffer
 *
 * @return status code (ALL_OK if there was no errors)
 */
MIDPError
write_file(char** ppszError, const pcsl_string* pFileName,
           char* inBuffer, long inBufferLen);

/**
 * Initializes the subsystem. This wrapper is used to hide
 * global variables from the suitestore_common library.
 *
 * @return status code (ALL_OK if successful)
 */
MIDPError suite_storage_init_impl();

/**
 * Resets any persistent resources allocated by MIDlet suite storage functions.
 * This wrapper is used to hide global variables from the suitestore_common
 * library.
 */
void suite_storage_cleanup_impl();

/**
 * Frees the memory occupied by the given MidletSuiteData structure.
 *
 * @param pData MidletSuiteData entry to be freed
 */
void free_suite_data_entry(MidletSuiteData* pData);

/**
 * Gets the storage root for a MIDlet suite by ID.
 * Free the data of the string returned with pcsl_string_free().
 *
 * @param suiteId suite ID
 * @param sRoot receives storage root (gets set to NULL in the case of an error)
 *
 * @return status: ALL_OK if success,
 * OUT_OF_MEMORY if out-of-memory
 */
MIDPError
get_suite_storage_root(SuiteIdType suiteId, pcsl_string* sRoot);

/**
 * Search for a structure describing the suite by the suite's ID.
 *
 * @param suiteId unique ID of the midlet suite
 *
 * @return pointer to the MidletSuiteData structure containing
 * the suite's attributes or NULL if the suite was not found
 */
MidletSuiteData* get_suite_data(SuiteIdType suiteId);


#if ENABLE_DYNAMIC_COMPONENTS
/**
 * Search for a structure describing the component by the component's ID.
 *
 * @param componentId unique ID of the dynamic component
 *
 * @return pointer to the MidletSuiteData structure containing
 * the component's attributes or NULL if the component was not found
 */
MidletSuiteData* get_component_data(ComponentIdType componentId);
#endif /* ENABLE_DYNAMIC_COMPONENTS */

/**
 * Reads the file with information about the installed suites.
 *
 * Note that if the value of the global variable g_numberOfSuites
 * is zero, this function does nothing.
 *
 * @param ppszError pointer to character string pointer to accept an error
 *
 * @return status code: ALL_OK if no errors,
 *         OUT_OF_MEMORY if malloc failed
 *         IO_ERROR if an IO_ERROR,
 *         SUITE_CORRUPTED_ERROR if the suite database is corrupted
 */
MIDPError read_suites_data(char** ppszError);

/**
 * Writes the file with information about the installed suites.
 *
 * Note that if the value of the global variable g_numberOfSuites
 * is zero, the file will be truncated.
 *
 * @param ppszError pointer to character string pointer to accept an error
 *
 * @return status code: ALL_OK if no errors,
 *         OUT_OF_MEMORY if malloc failed
 *         IO_ERROR if an IO_ERROR
 */
MIDPError write_suites_data(char** ppszError);

/**
 * Gets the enable state of a MIDlet suite from persistent storage.
 *
 * @param ppszError pointer to character string pointer to accept an error
 * @param suiteId ID of the suite
 * @param pEnabled pointer to an enabled setting
 *
 * @return error code (ALL_OK if successful)
 */
MIDPError
read_enabled_state(char** ppszError, SuiteIdType suiteId, jboolean* pEnabled);

/**
 * Gets the settings of a MIDlet suite from persistent storage.
 * <pre>
 * The format of the properties file will be:
 *
 *   push interrupt setting as an jbyte
 *   length of a permissions as an int
 *   array of permissions jbytes
 *   push options as jint
 * </pre>
 *
 * @param ppszError pointer to character string pointer to accept an error
 * @param suiteId  ID of the suite
 * @param pEnabled pointer to an enabled setting
 * @param pPushInterrupt pointer to a push interruptSetting
 * @param pPushOptions user options for push interrupts
 * @param ppPermissions pointer a pointer to accept a permissions array
 * @param pNumberOfPermissions pointer to an int
 *
 * @return error code (ALL_OK if successful)
 */
MIDPError
read_settings(char** ppszError, SuiteIdType suiteId, jboolean* pEnabled,
             jbyte* pPushInterrupt, jint* pPushOptions,
             jbyte** ppPermissions, int* pNumberOfPermissions);

/**
 * Writes the settings of a MIDlet suite to persistent storage.
 * <pre>
 * The format of the properties file will be:
 *
 *   push interrupt setting as an jbyte
 *   length of a permissions as an int
 *   array of permissions jbytes
 *   push options as jint
 * </pre>
 *
 * @param ppszError pointer to character string pointer to accept an error
 * @param suiteId  ID of the suite
 * @param enabled enabled setting
 * @param pushInterrupt pointer to a push interruptSetting
 * @param pushOptions user options for push interrupts
 * @param pPermissions pointer a pointer to accept a permissions array
 * @param numberOfPermissions length of pPermissions
 * @param pOutDataSize [out] points to a place where the size of the
 *                           written data is saved; can be NULL
 *
 * @return error code (ALL_OK if successful)
 */
MIDPError
write_settings(char** ppszError, SuiteIdType suiteId, jboolean enabled,
               jbyte pushInterrupt, jint pushOptions,
               jbyte* pPermissions, int numberOfPermissions,
               jint* pOutDataSize);

/**
 * Read a the install information of a suite from persistent storage.
 * The caller should have make sure the suite ID is valid.
 * The caller should call midp_free_install_info when done with the information.
 * <pre>
 * The fields are
 *   jadUrl
 *   jarUrl
 *   ca
 *   domain
 *   trusted
 *
 * Unicode strings are written as an int length and a jchar array.
 * </pre>
 * @param ppszError pointer to character string pointer to accept an error
 * @param suiteId ID of a suite
 *
 * @return an InstallInfo struct, use the status macros to check for
 * error conditions in the struct
 */
MidpInstallInfo read_install_info(char** ppszError, SuiteIdType suiteId);

/**
 * Builds a full file name using the storage root and MIDlet suite by ID.
 * get_suite_filename is used to build a filename after validation checks.
 *
 * @param suiteId suite ID
 * @param filename filename without a root path
 * @param res receives full name of the file
 *
 * @return the status:
 * ALL_OK if ok,
 * NOT_FOUND mean the suite does not exist,
 * OUT_OF_MEMORY if out of memory,
 * IO_ERROR if an IO_ERROR
 */
MIDPError
build_suite_filename(SuiteIdType suiteId, const pcsl_string* filename,
                     pcsl_string* res);

/**
 * Removes the given suite and all its components from the list
 * of installed suites.
 * <p>
 * Used from suitestore_midletsuitestorage_kni.c and suitestore_task_manager.c
 * so it is non-static.
 *
 * @param suiteId ID of a suite
 *
 * @return  1 if the suite was in the list, 0 if not,
 *         -1 if out of memory
 */
int remove_from_suite_list_and_save(SuiteIdType suiteId);

#if ENABLE_DYNAMIC_COMPONENTS
/**
 * Removes all components belonging to the given suite from the list
 * of installed components.
 * <p>
 *
 * @param suiteId ID of the suite owning the components
 * @param componentId ID of the component to remove, or UNUSED_COMPONENT_ID
 *                    to remove all components of this suite
 *
 * @return  1 if the suite was in the list, 0 if not,
 *         -1 if out of memory
 */
int remove_from_component_list_and_save(SuiteIdType suiteId,
                                        ComponentIdType componentId);
#endif /* ENABLE_DYNAMIC_COMPONENTS */

/**
 * Gets filename of the secure suite resource by suiteId and resource name
 *
 * Note that memory for the in/out parameter filename
 * MUST be allocated using pcsl_mem_malloc().
 * The caller is responsible for freeing it.
 *
 * @param suiteId           The suite id used to identify the MIDlet suite
 * @param pResourceName     The name of secure resource to read from storage
 * @param pFilename         The in/out parameter that will return filename
 *                          of the specified secure resource
 * @return one of the error codes:
 * <pre>
 *     ALL_OK, OUT_OF_MEMORY, NOT_FOUND,
 *     SUITE_CORRUPTED_ERROR, BAD_PARAMS
 * </pre>
 */
MIDPError get_secure_resource_file(SuiteIdType suiteId,
        const pcsl_string* pResourceName, pcsl_string *pFilename);

/**
 * Gets location of the properties file
 * for the suite with the specified suiteId.
 *
 * Note that in/out parameter filename MUST be allocated by callee with
 * pcsl_mem_malloc(), the caller is responsible for freeing it.
 *
 * @param suiteId    - The application suite ID string
 * @param checkSuiteExists - true if suite should be checked for existence or not
 * @param pFilename - The in/out parameter that contains returned filename
 * @return  error code that should be one of the following:
 * <pre>
 *     ALL_OK, OUT_OF_MEMORY, NOT_FOUND
 * </pre>
 */
MIDPError get_property_file(SuiteIdType suiteId,
                            jboolean checkSuiteExists,
                            pcsl_string *pFilename);

/**
 * Retrieves the class path for a suite or dynamic component.
 *
 * NOTE: this method is located here because it is called from both
 *       MIDletSuiteStorage and DynamicComponentStorage native code.
 *       If dynamic components are not used, it can be moved to
 *       suitestore_midletsuitestorage_kni.c.
 *
 * @param type             type of component (midlet suite or dynamic
 *                         component) for which the information is requested
 * @param suiteOrComponentId unique ID of the suite or coponent
 *
 * @param pClassPath [out] pointer to pcsl_string where the class path will
 *                         be saved on exit; it's a caller's responsibility
 *                         to call pcsl_string_free() when this object is
 *                         not needed
 *
 * @return ALL_OK if no errors,
 *         SUITE_CORRUPTED_ERROR if suite is corrupted,
 *         OUT_OF_MEMORY if out of memory,
 *         IO_ERROR if I/O error
 */
MIDPError get_jar_path(ComponentType type, jint suiteOrComponentId,
                       pcsl_string* pClassPath);

/**
 * Check if the suite is corrupted
 * @param suiteId ID of a suite
 *
 * @return ALL_OK if the suite is not corrupted,
 *         SUITE_CORRUPTED_ERROR is suite is corrupted,
 *         OUT_OF_MEMORY if out of memory,
 *         IO_ERROR if I/O error
 */
MIDPError check_for_corrupted_suite(SuiteIdType suiteId);

/**
 * Tries to repair the suite database.
 *
 * @return ALL_OK if succeeded, other value if failed
 */
MIDPError repair_suite_db();

/**
 * Starts a new transaction of the given type.
 *
 * @param transactionType type of the new transaction
 * @param suiteId ID of the suite, may be UNUSED_SUITE_ID
 * @param pFilename name of the midlet suite's file, may be NULL 
 *
 * @return ALL_OK if no errors,
 *         IO_ERROR if I/O error
 */
MIDPError begin_transaction(MIDPTransactionType transactionType,
                            SuiteIdType suiteId,
                            const pcsl_string *pFilename);

/**
 * Rolls back the transaction being in progress.
 *
 * @return ALL_OK if the transaction was rolled back,
 *         NOT_FOUND if the transaction has not been started,
 *         IO_ERROR if I/O error
 */
MIDPError rollback_transaction();

/**
 * Finishes the previously started transaction.
 *
 * @return ALL_OK if the transaction was successfully finished,
 *         NOT_FOUND if the transaction has not been started,
 *         IO_ERROR if I/O error
 */
MIDPError finish_transaction();

/**
 * Checks if there is an unfinished transaction.
 *
 * @return 0 there is no unfinished transaction, != 0 otherwise 
 */
int unfinished_transaction_exists();

#ifdef __cplusplus
}
#endif

#endif /* _SUITESTORE_INTERN_H_ */
