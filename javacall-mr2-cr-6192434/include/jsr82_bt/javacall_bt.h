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

/**
 * @file javacall_bt.h
 * @ingroup JSR82Bluetooth
 * @brief Javacall interfaces for JSR-82 Bluetooth API
 */

#ifndef __JAVACALL_BT_H
#define __JAVACALL_BT_H
 
#ifdef __cplusplus
extern "C" {
#endif

#include "javacall_defs.h" 

/**
 * @defgroup Bluetooth JSR82 Bluetooth API
 *
 * This API covers porting layers for Bluetooth Stack.
 *
 * <b> BCC and Bluetooth stack</b>
 * 
 * The following APIs can be optional if BCC does not support them.
 *  - Asks user whether Bluetooth radio is allowed to be turned on.
 *  - Retrieves PIN code to use for pairing with a remote device.
 *  - Attempts to authorize a connection.
 *
 * <b> Service Discovery and Registration </b>
 *
 * We assume the underlying stack of Platform has SDP server 
 * which is responsible for advertising and accepting SDP requests. So,
 * Platform must provide the APIs like updating and removing from 
 * service discovery database (SDDB).
 * As for SDP client, it is implemented above this porting layer API.
 *
 * <b> L2CAP and RFCOMM protocol </b>
 *
 * Platform must provide L2CAP and RFCOMM protocol APIs. 
 * As for OBEX protocol, it is implemented above this porting layer API.
 *
 * <b> Implementation Notes </b>
 *
 *  - buffer allocation
 *
 * The buffer for output parameter should be allocated by the caller. 
 * There should be a parameter for the size of the buffer if there is 
 * no predefined maximum size. 
 *
 *  - asynchronous operation
 *
 * In the asynchronous case, the functions returns quickly 
 * with <code>JAVACALL_WOULD_BLOCK</code> instead of blocking. 
 * When the operation has been completed, the relevant javanotify function 
 * should be called in platform context.
 *
 *  - service record handle
 *
 * The service record handle values are not related to ServiceRecordHandle 
 * attributes associated with service records. Instead, these handles are 
 * only used to uniquely identify service records in upper-level code,
 * and may or may not match ServiceRecordHandle attribute values used 
 * by SDDB internally.
 *
 * \par
 *
 */

/**
 * @defgroup jsrMandatoryBluetooth Mandatory Bluetooth API
 * @ingroup Bluetooth
 * @{
 */

/**
 * @def BT_RFCOMM_INVALID_CN
 * Defines invalid channel number (CN) value.
 */
#define BT_RFCOMM_INVALID_CN  -1

#define JAVACALL_BT_MAX_USER_FRIENDLY_NAME 248
#define JAVACALL_BT_MAX_PREKNOWN_DEVICES   10
#define JAVACALL_BT_ADDRESS_SIZE           6
#define JAVACALL_BT_MAX_PIN_LEN            (16 + 1)

/**
 * @typedef javacall_bt_address
 * @brief Bluetooth address.
 *        The bytes are stored in Bluetooth order (little-endian).
 */
typedef unsigned char javacall_bt_address[JAVACALL_BT_ADDRESS_SIZE];

/***************************************************
 * Bluetooth Control Center
 **************************************************/

/**
 * Allocates BCC related native resources.
 *
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> if an error occurred
 */
javacall_result javacall_bt_bcc_initialize(void);

/**
 * Releases BCC related native resources.
 *
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 */
javacall_result javacall_bt_bcc_finalize(void);

/**
 * Determines if the local device is in connectable mode.
 *
 * @param pBool pointer to variable where the result is to be stored:
 *              <code>JAVACALL_TRUE</code> if the device is connectable,
 *              <code>JAVACALL_FALSE</code> otherwise
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 */
javacall_result javacall_bt_bcc_is_connectable(/*OUT*/javacall_bool *pBool);

/**
 * Determines if there is a connection to the remote device.
 *
 * @param addr the Bluetooth address of the remote device
 * @param pBool pointer to variable where the result is to be stored:
 *              <code>JAVACALL_TRUE</code> if the device is connected,
 *              <code>JAVACALL_FALSE</code> otherwise
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 */
javacall_result javacall_bt_bcc_is_connected(
		const javacall_bt_address addr, 
		javacall_bool *pBool);

/**
 * Checks if the local device has a bond with a remote device.
 *
 * @param addr Bluetooth address of a remote device
 * @param pBool pointer to variable where the result is to be stored:
 *              <code>JAVACALL_TRUE</code> if the devices are paired,
 *              <code>JAVACALL_FALSE</code> otherwise
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 */
javacall_result javacall_bt_bcc_is_paired(
        const javacall_bt_address addr, 
        /*OUT*/javacall_bool *pBool);

/**
 * Checks if a remote device was authenticated.
 *
 * @param addr Bluetooth address of a remote device
 * @param pBool pointer to variable where the result is to be stored:
 *              <code>JAVACALL_TRUE</code> if authenticated,
 *              <code>JAVACALL_FALSE</code> otherwise
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 */
javacall_result javacall_bt_bcc_is_authenticated(
        const javacall_bt_address addr, 
        /*OUT*/javacall_bool *pBool);

/**
 * Checks if a remote device is trusted (authorized for all services).
 *
 * BCC may allow the user to permanently authorize a remote device 
 * for all local services. When a device is authorized in this way, 
 * it is known as a "trusted device".
 *
 * @param addr Bluetooth address of a remote device
 * @param pBool pointer to variable where the result is to be stored:
 *              <code>JAVACALL_TRUE</code> if the remote device is trusted,
 *              <code>JAVACALL_FALSE</code> otherwise
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 */
javacall_result javacall_bt_bcc_is_trusted(
        const javacall_bt_address addr, 
        /*OUT*/javacall_bool *pBool);

/**
 * Determines if data exchanges with a remote device are being encrypted.
 *
 * @param addr Bluetooth address of a remote device
 * @param pBool pointer to variable where the result is to be stored:
 *              <code>JAVACALL_TRUE</code> if connection is encrypted,
 *              <code>JAVACALL_FALSE</code> otherwise
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 */
javacall_result javacall_bt_bcc_is_encrypted(
        const javacall_bt_address addr, 
        /*OUT*/javacall_bool *pBool);

/**
 * Initiates pairing with a remote device.
 *
 * If this function returns <code>JAVACALL_WOULD_BLOCK</code>, the notification
 * will be sent via <code>javanotify_bt_remote_device_event()</code>
 * with <code>JAVACALL_EVENT_BT_BOND_COMPLETE</code> type.
 *
 * @param addr the Bluetooth address of the device with which to pair
 * @param pin an array containing the PIN code (null-terminated UTF-8 encoded).
 * @param pBool pointer to variable where the result is to be stored:
 *              <code>JAVACALL_TRUE</code> if the device was paired,
 *              <code>JAVACALL_FALSE</code> otherwise
 * @retval <code>JAVACALL_OK</code> if the device was paired
 * @retval <code>JAVACALL_FAIL</code> on failure
 * @retval <code>JAVACALL_WOULD_BLOCK</code> in case of asynchronous operation
 */
javacall_result javacall_bt_bcc_bond(
        const javacall_bt_address addr, 
        const char *pin,
		javacall_bool *pBool);

/**
 * Returns list of preknown devices.
 *
 * @param devices an output array to receive the list of preknown devices
 * @param pCount pointer to variable where the result is to be stored:
 *               number of records stored in the output array.
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 */
javacall_result javacall_bt_bcc_get_preknown_devices(
        /*OUT*/javacall_bt_address devices[JAVACALL_BT_MAX_PREKNOWN_DEVICES],
        /*OUT*/int *pCount);

/**
 * Increases or decreases encryption request counter for a remote device.
 *
 * @param addr the Bluetooth address of the remote device
 * @param enable indicated whether the encryption needs to be enabled
 * @param pBool pointer to variable where the result is to be stored:
 *              <code>JAVACALL_TRUE</code> if the encryption must be changed,
 *              <code>JAVACALL_FALSE</code> otherwise
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 */
javacall_result javacall_bt_bcc_set_encryption(
		const javacall_bt_address addr, 
		javacall_bool enable,
        javacall_bool *pBool);

/***************************************************
 * Bluetooth Stack
 **************************************************/

/**
 * Acquires the contorl of Bluetooth stack.
 *
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 */
javacall_result javacall_bt_stack_initialize(void);

/**
 * Releases the contorl of Bluetooth stack.
 *
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 */
javacall_result javacall_bt_stack_finalize(void);

/**
 * Checks if the Bluetooth radio is enabled.
 *
 * @param pBool pointer to variable where the result is to be stored:
 *              <code>JAVACALL_TRUE</code> if the Bluetooth radio is enabled,
 *              <code>JAVACALL_FALSE</code> otherwise
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 */
javacall_result javacall_bt_stack_is_enabled(
        /*OUT*/javacall_bool *pBool);

/**
 * Enables Bluetooth radio.
 *
 * If this function returns <code>JAVACALL_WOULD_BLOCK</code>, the notification
 * will be sent via <code>javanotify_bt_event()</code>
 * with <code>JAVACALL_EVENT_BT_ENABLE_RADIO_COMPLETE</code> type.
 *
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 * @retval <code>JAVACALL_WOULD_BLOCK</code> in case of asynchronous operation
 */
javacall_result javacall_bt_stack_enable(void);

/**
 * Returns Bluetooth address of the local device.
 *
 * If this function returns <code>JAVACALL_WOULD_BLOCK</code>, the notification
 * will be sent via <code>javanotify_bt_event()</code>
 * with <code>JAVACALL_EVENT_BT_LOCAL_ADDRESS_COMPLETE</code> type.
 *
 * @param pAddr pointer to variable where the result is to be stored:
 *              Bluetooth address of the local device
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 * @retval <code>JAVACALL_WOULD_BLOCK</code> in case of asynchronous operation
 */
javacall_result javacall_bt_stack_get_local_address(
        /*OUT*/javacall_bt_address *pAddr);

/**
 * Retrieves user-friendly name for the local device.
 *
 * If this function returns <code>JAVACALL_WOULD_BLOCK</code>, the notification
 * will be sent via <code>javanotify_bt_event()</code>
 * with <code>JAVACALL_EVENT_BT_LOCAL_NAME_COMPLETE</code> type.
 *
 * @param pName string to store the name of the local device
 *              (null-terminated in UTF-8 encoding). The length should be
 *              at least <code>JAVACALL_BT_MAX_USER_FRIENDLY_NAME</code> bytes
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 * @retval <code>JAVACALL_WOULD_BLOCK</code> in case of asynchronous operation
 */
javacall_result javacall_bt_stack_get_local_name(char *pName);

/**
 * Sets major service class bits of the device.
 *
 * @param classes an integer whose binary representation indicates the major
 *                service class bits that should be set
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 */
javacall_result javacall_bt_stack_set_service_classes(int classes);

/**
 * Retrieves the class of local device value that represents the service 
 * classes, major device class, and minor device class of the local device.
 *
 * @param pValue pointer to variable where the result is to be stored:
 *               class of device value
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 */
javacall_result javacall_bt_stack_get_device_class(
        /*OUT*/int *pValue);

/**
 * Retrieves the inquiry access code that the local Bluetooth device is
 * scanning for during inquiry scans.
 *
 * @param pValue pointer to variable where the result is to be stored:
 *               inquiry access code
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 */
javacall_result javacall_bt_stack_get_access_code(
        /*OUT*/int *pValue);

/**
 * Sets the inquiry access code that the local Bluetooth device is
 * scanning for during inquiry scans.
 *
 * @param accessCode inquiry access code to be set (valid values are in the
 *                   range <code>0x9e8b00-0x9e8b3f</code>),
 *                   or <code>0</code> to take the device out
 *                   of discoverable mode
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 */
javacall_result javacall_bt_stack_set_access_code(int accessCode);

/**
 * Starts asynchronous device discovery.
 *
 * Whenever a device is discovered,
 * <code>javanotify_bt_device_discovered()</code> gets called.
 *
 * The notification of inquiry completion will be sent via
 * <code>javanotify_bt_inquiry_complete()</code>.
 *
 * @param accessCode the type of inquiry
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 */
javacall_result javacall_bt_stack_start_inquiry(int accessCode);

/**
 * Cancels asynchronous device discovery.
 *
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 */
javacall_result javacall_bt_stack_cancel_inquiry(void);

/**
 * Retrieves friendly name of the specified Bluetooth device.
 *
 * If this function should always peform asynchronously. Upon completion of the
 * operation, <code>javanotify_bt_remote_name_complete()</code> is expected
 * to be called.
 *
 * @param addr Bluetooth address of the device which name is to be retrieved
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 */
javacall_result javacall_bt_stack_ask_friendly_name(
        const javacall_bt_address addr);

/**
 * Attempts to authenticate a remote device.
 *
 * If this function should always peform asynchronously. Upon completion of the
 * operation, <code>javanotify_bt_authentication_complete()</code> is expected
 * to be called.
 *
 * @param addr address of the remote device
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 */
javacall_result javacall_bt_stack_authenticate(const javacall_bt_address addr);

/**
 * Attempts to change encryption for all connections to the remote device.
 *
 * If this function should always peform asynchronously. Upon completion of the
 * operation, <code>javanotify_bt_encryption_change()</code> is expected
 * to be called.
 *
 * @param addr address of the remote device
 * @param enable specifies if encryption to be enabled or disabled
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 */
javacall_result javacall_bt_stack_encrypt(
        const javacall_bt_address addr,
        javacall_bool enable);

/**
 * Checks if Bluetooth events are available.
 *
 * @param pBool pointer to variable where the result is to be stored:
 *              <code>JAVACALL_TRUE</code> if there are pending events,
 *              <code>JAVACALL_FALSE</code> otherwise
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 */
javacall_result javacall_bt_stack_check_events(javacall_bool *pBool);

/**
 * Reads stack implementation-specific event data.
 *
 * @param data buffer where the data will be written to
 * @param len length of the buffer in bytes
 * @param pBytes pointer to variable where the result is to be stored:
 *               actual number of bytes read
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 */
javacall_result javacall_bt_stack_read_data(void *data, int len, int *pBytes);

/***************************************************
 * Service Discovery Database
 **************************************************/

/**
 * Initializes service discovery database.
 *
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 */
javacall_result javacall_bt_sddb_initialize(void);

/**
 * Closes service discovery database. All remaining Java-supplied service
 * records must be removed.
 *
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 */
javacall_result javacall_bt_sddb_finalize(void);

/**
 * Creates or updates service record in the service discovery database (SDDB).
 *
 * @param pId pointer to handle of a service record to be modified;
              <code>*pId == 0</code> for new record, and will be assigned
              to a new value
 * @param classes service classes
 * @param data service record data
 * @param size length of the data
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 */
javacall_result javacall_bt_sddb_update_record(
        /*OUT*/unsigned long *pId,
        unsigned long classes,
        void *data,
        unsigned long size);

/**
 * Updates PSM parameter value of the L2CAP protocol in the service record.
 * This function is required for push implementation.
 *
 * @param id handle of the service record to be updated
 * @param psm new PSM value
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 */
javacall_result javacall_bt_sddb_update_psm(
        unsigned long id,
        unsigned short psm);

/**
 * Updates channel parameter value of the RFCOMM protocol in the service record.
 * This function is required for push implementation.
 *
 * @param id handle of the service record to be updated
 * @param cn new channel value
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 */
javacall_result javacall_bt_sddb_update_channel(
        unsigned long id,
        unsigned char cn);

/**
 * Checks if a service record exists in SDDB.
 *
 * @param id handle of a service record to look up
 * @retval <code>JAVACALL_TRUE</code> if record exists
 * @retval <code>JAVACALL_FALSE</code> if record does not exist
 */
javacall_bool javacall_bt_sddb_exists_record(unsigned long id);

/**
 * Reads a service record from SDDB.
 * This function is required for push implementation.
 *
 * @param id service record handle
 * @param pClasses pointer to variable which will receive service classes
 * @param data buffer to be written to
 * @param pSize points to size of the buffer, or size query if *pSize == 0
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 */
javacall_result javacall_bt_sddb_read_record(unsigned long id,
        /*OUT*/unsigned long *pClasses, void *data,
        /*OUT*/unsigned long *pSize);

/**
 * Removes service record from SDDB.
 *
 * @param id handle of a service record to remove
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 */
javacall_result javacall_bt_sddb_remove_record(unsigned long id);

/**
 * Returns handles for all service records in SDDB.
 * This function is required for the emulation build, when SDP server is
 * maintained by Java, rather than by Bluetooth stack.
 *
 * @param array buffer to receive service record handles
 * @param count number of entries in the array, or 0 for count query
 * @return number of entries available/saved to the array,
 *         or <code>0</code> if an error occurs
 */
unsigned long javacall_bt_sddb_get_records(
        /*OUT*/unsigned long *array,
        unsigned long count);

/**
 * Retrieves service classes for either a single service record,
 * or representing the current state of entire SDDB.
 *
 * @param id service record handle
 * @return major device service classes
 */
unsigned long javacall_bt_sddb_get_service_classes(unsigned long id);

/***************************************************
 * L2CAP protocol
 **************************************************/

/**
 * @def BT_L2CAP_INVALID_PSM
 * Defines invalid protocol/service multiplexer (PSM) value.
 *
 * Valid PSM range: 0x0001-0x0019 (0x1001-0xFFFF dynamically assigned,
 * 0x0019-0x0100 reserved for future use).
 */
#define BT_L2CAP_INVALID_PSM -1

/**
 * @def BT_L2CAP_MIN_MTU
 * Minimum MTU size according to Bluetooth specification.
 */
#define BT_L2CAP_MIN_MTU 48

/**
 * Closes L2CAP connection.
 *
 * Determines whether the connection is not closed, if so closes it.
 *
 * @param handle connection handle
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure,
 *                                    or if the connection was already closed
 */
javacall_result javacall_bt_l2cap_close(javacall_handle handle);

/**
 * Retrieves code and string description of the last occured error.
 *
 * @param handle connection handle
 * @param pErrStr pointer to string pointer initialized with
 *                result string pointer,
                  if <code>NULL</code> error string is not returned
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 */
/*OPTIONAL*/ javacall_result javacall_bt_l2cap_get_error(
        javacall_handle handle,
        /*OUT*/char **pErrStr);

/**
 * Creates a new server-side L2CAP connection.
 *
 * The method creates a server connection instance
 * but does not put it in listen mode.
 * Anyway it selects and reserves a free PSM to listen for
 * incoming connections on after the listen method is called.
 *
 * Returned connection is put in non-blocking mode.
 *
 * @param receiveMTU receive MTU, <code>0</code> to use default value
 * @param transmitMTU transmit MTU, <code>0</code> to use default value
 * @param authenticate <code>JAVACALL_TRUE</code> if authication is required
 * @param authorize <code>JAVACALL_TRUE</code> if authorization is required
 * @param encrypt <code>JAVACALL_TRUE</code> if required to be encrypted
 * @param master <code>JAVACALL_TRUE</code> if required to be a master
 * @param pHandle pointer to connection handle variable,
 *                new connection handle returned in result
 * @param pPsm pointer to variable, where reserved PSM is returned in
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 */
javacall_result javacall_bt_l2cap_create_server(
        int receiveMTU,
        int transmitMTU,
        javacall_bool authenticate,
        javacall_bool authorize,
        javacall_bool encrypt,
        javacall_bool master,
        /*OUT*/javacall_handle *pHandle,
        /*OUT*/int *pPsm);

/**
 * Puts L2CAP server connection into listening mode.
 *
 * @param handle server connection handle
 * @retval <code>JAVACALL_OK</code> on success,
 * @retval <code>JAVACALL_FAIL</code> otherwise
 */
javacall_result javacall_bt_l2cap_listen(javacall_handle handle);

/**
 * Accepts incoming L2CAP connection.
 *
 * @param handle server connection handle
 * @param pPeerHandle pointer to peer handle to store new connection handle
 *                    to work with accepted incoming client connection
 * @param pPeerAddr Bluetooth address variable to store
 *                  the address of accepted client,
 *                  if <code>NULL</code> the value is not returned
 * @param pReceiveMTU pointer to store receive MTU size for a new connection
 * @param pTransmitMTU pointer to store transmit MTU size for a new connection
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> if an error occurred
 * @retval <code>JAVACALL_WOULD_BLOCK</code> in case of asynchronous operation
 */
javacall_result javacall_bt_l2cap_accept(
        javacall_handle handle, 
        /*OUT*/javacall_handle *pPeerHandle,
        /*OUT*/javacall_bt_address *pPeerAddr,
        /*OUT*/int *pReceiveMTU,
        /*OUT*/int *pTransmitMTU);

/**
 * Creates a new client L2CAP connection.
 *
 * The method does not establishes real Bluetooth connection
 * just creates a client connection instance.
 *
 * @param receiveMTU receive MTU, <code>0</code> to use default value
 * @param transmitMTU transmit MTU, <code>0</code> to use default value
 * @param authenticate <code>JAVACALL_TRUE</code> if authication is required
 * @param encrypt <code>JAVACALL_TRUE</code> if required to be encrypted
 * @param master <code>JAVACALL_TRUE</code> if required to be a master
 * @param pHandle pointer to connection handle variable,
 *                new connection handle returned in result.
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 */
javacall_result javacall_bt_l2cap_create_client(
        int receiveMTU,
        int transmitMTU,
        javacall_bool authenticate,
        javacall_bool encrypt,
        javacall_bool master,
        /*OUT*/javacall_handle *pHandle);

/**
 * Establishes L2CAP connection with the Bluetooth device.
 *
 * If this function returns <code>JAVACALL_WOULD_BLOCK</code>, the notification
 * will be sent via <code>javanotify_bt_protocol_event()</code>
 * with <code>JAVACALL_EVENT_BT_CONNECT_COMPLETE</code> type.
 *
 * @param handle connection handle
 * @param addr pointer to the address of device to connect to
 * @param psm PSM port to connect to
 * @param pReceiveMTU pointer to variable to store negotiated receive MTU
 * @param pTransmitMTU pointer to variable to store negotiated transmit MTU
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 * @retval <code>JAVACALL_WOULD_BLOCK</code> in case of asynchronous operation
 */
javacall_result javacall_bt_l2cap_connect(
        javacall_handle handle,
        const javacall_bt_address addr,
        int psm,
        /*OUT*/int *pReceiveMTU,
        /*OUT*/int *pTransmitMTU);

/**
 * Sends data through L2CAP connection.
 *
 * If the size of pData is greater than the Transmit MTU, 
 * then only the first Transmit MTU bytes of the packet are sent.
 * Even if size of the buffer is zero, an empty L2CAP packet should be sent.
 *
 * If this function returns <code>JAVACALL_WOULD_BLOCK</code>, the notification
 * will be sent via <code>javanotify_bt_protocol_event()</code>
 * with <code>JAVACALL_EVENT_BT_SEND_COMPLETE</code> type.
 *
 * @param handle connection handle
 * @param pData pointer to data buffer
 * @param len length of the data
 * @param pBytesSent number of bytes that were really sent
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 * @retval <code>JAVACALL_WOULD_BLOCK</code> in case of asynchronous operation
 */
javacall_result javacall_bt_l2cap_send(
        javacall_handle handle,
        const char *pData,
        int len,
        /*OUT*/int *pBytesSent);

/**
 * Receives data from L2CAP connection.
 *
 * If size of the buffer is less than size of the received packet,
 * the rest of the packet is discarded.
 *
 * If this function returns <code>JAVACALL_WOULD_BLOCK</code>, the notification
 * will be sent via <code>javanotify_bt_protocol_event()</code>
 * with <code>JAVACALL_EVENT_BT_RECEIVE_COMPLETE</code> type.
 *
 * @param handle connection handle
 * @param pData pointer to data buffer
 * @param len length of the buffer
 * @param pBytesReceived number of bytes that were received,
 *                       <code>0</code> indicates end of data
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 * @retval <code>JAVACALL_WOULD_BLOCK</code> in case of asynchronous operation
 */
javacall_result javacall_bt_l2cap_receive(
        javacall_handle handle,
        /*OUT*/char *pData,
        int len,
        /*OUT*/int *pBytesReceived);

/**
 * Determines if there is data to be read from L2CAP connection
 * without blocking.
 * 
 * @param handle connection handle
 * @param pReady pointer to variable result is stored in
 *               <code>JAVACALL_TRUE</code> if there are data available
 *               <code>JAVACALL_FALSE</code> otherwise
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 */
javacall_result javacall_bt_l2cap_get_ready(
        javacall_handle handle,
        /*OUT*/javacall_bool *pReady);

/***************************************************
 * RFCOMM protocol
 **************************************************/

/**
 * Closes RFCOMM connection.
 *
 * @param handle connection handle
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure,
 *                                    or if connection was already closed
 */
javacall_result javacall_bt_rfcomm_close(javacall_handle handle);

/**
 * Retrieves code and string description of the last occured error.
 *
 * @param handle connection handle
 * @param pErrStr pointer to string pointer initialized with
 *                result string pointer,
                  if <code>NULL</code> error string is not returned
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 */
javacall_result javacall_bt_rfcomm_get_error(javacall_handle handle,
    /*OUT*/char **pErrStr);

/**
 * Creates a new server-side RFCOMM connection.
 *
 * The method creates a server connection instance
 * but does not put it in listen mode.
 * Anyway it selects and reserves a free channel to listen for
 * incoming connections on after the listen method is called.
 *
 * @param authenticate <code>JAVACALL_TRUE</code> if authication is required
 * @param authorize <code>JAVACALL_TRUE</code> if authorization is required
 * @param encrypt <code>JAVACALL_TRUE</code> if required to be encrypted
 * @param master <code>JAVACALL_TRUE</code> if required to be a master
 * @param pHandle pointer to connection handle variable,
 *                new connection handle returned in result
 * @param pCn pointer to variable, where reserved channel is returned in
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 */
javacall_result javacall_bt_rfcomm_create_server(
        javacall_bool authenticate,
        javacall_bool authorize,
        javacall_bool encrypt,
        javacall_bool master,
        /*OUT*/javacall_handle *pHandle,
        /*OUT*/int *pCn);

/**
 * Puts RFCOMM server connection into listening mode.
 *
 * @param handle server connection handle
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 */
javacall_result javacall_bt_rfcomm_listen(javacall_handle handle);

/**
 * Accepts incoming RFCOMM connection.
 *
 * If this function returns <code>JAVACALL_WOULD_BLOCK</code>, the notification
 * will be sent via <code>javanotify_bt_protocol_event()</code>
 * with <code>JAVACALL_EVENT_BT_ACCEPT_COMPLETE</code> type.
 *
 * @param handle server connection handle
 * @param pPeerHandle pointer to peer handle to store new connection handle
 *                    to work with accepted incoming client connection
 * @param pPeerAddr Bluetooth address variable to store
 *                  the address of accepted client, 
 *                  if <code>NULL</code> the value is not returned
 *
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 * @retval <code>JAVACALL_WOULD_BLOCK</code> in case of asynchronous operation
 */
javacall_result javacall_bt_rfcomm_accept(
        javacall_handle handle, 
        /*OUT*/javacall_handle *pPeerHandle,
        /*OUT*/javacall_bt_address *pPeerAddr);

/**
 * Creates a new client RFCOMM connection.
 *
 * The method does not establishes real Bluetooth connection,
 * it just creates a client connection instance.
 *
 * @param authenticate <code>JAVACALL_TRUE</code> if authication is required
 * @param encrypt <code>JAVACALL_TRUE</code> if required to be encrypted
 * @param master <code>JAVACALL_TRUE</code> if required to be a master
 * @param pHandle pointer to connection handle variable,
 *                new connection handle returned in result.
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 */
javacall_result javacall_bt_rfcomm_create_client(
        javacall_bool authenticate,
        javacall_bool encrypt,
        javacall_bool master,
        /*OUT*/javacall_handle *pHandle);

/**
 * Establishes RFCOMM connection with the Bluetooth device.
 *
 * If this function returns <code>JAVACALL_WOULD_BLOCK</code>, the notification
 * will be sent via <code>javanotify_bt_protocol_event()</code>
 * with <code>JAVACALL_EVENT_BT_CONNECT_COMPLETE</code> type.
 *
 * @param handle connection handle
 * @param addr pointer to the address of device to connect to
 * @param cn channel number to connect to
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 * @retval <code>JAVACALL_WOULD_BLOCK</code> in case of asynchronous operation
 */
javacall_result javacall_bt_rfcomm_connect(
        javacall_handle handle,
        const javacall_bt_address addr, 
        int cn);

/**
 * Sends data through RFCOMM connection.
 *
 * If this function returns <code>JAVACALL_WOULD_BLOCK</code>, the notification
 * will be sent via <code>javanotify_bt_protocol_event()</code>
 * with <code>JAVACALL_EVENT_BT_SEND_COMPLETE</code> type.
 *
 * @param handle connection handle
 * @param pData pointer to data buffer
 * @param len length of the data
 * @param pBytesSent number of bytes that were really sent
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 * @retval <code>JAVACALL_WOULD_BLOCK</code> in case of asynchronous operation
 */
javacall_result javacall_bt_rfcomm_send(
        javacall_handle handle,
        const char *pData, 
        int len, 
        /*OUT*/int *pBytesSent);

/**
 * Receives data from RFCOMM connection.
 *
 * If this function returns <code>JAVACALL_WOULD_BLOCK</code>, the notification
 * will be sent via <code>javanotify_bt_protocol_event()</code>
 * with <code>JAVACALL_EVENT_BT_RECEIVE_COMPLETE</code> type.
 *
 * @param handle connection handle
 * @param pData pointer to data buffer
 * @param len length of the buffer
 * @param pBytesRead number of bytes that were received,
 *                   <code>0</code> indicates end of data
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 * @retval <code>JAVACALL_WOULD_BLOCK</code> in case of asynchronous operation
 */
javacall_result javacall_bt_rfcomm_receive(
        javacall_handle handle,
        char *pData, 
        int len, 
        /*OUT*/int *pBytesReceived);

/**
 * Returns the number of bytes available for read from RFCOMM connection
 * without blocking.
 * 
 * @param handle connection handle
 * @param pCount pointer to variable the number of available bytes is stored in
 *
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 */
javacall_result javacall_bt_rfcomm_get_available(
        javacall_handle handle,
        /*OUT*/int *pCount);

/** @} */
    
/******************************************************************************
 ******************************************************************************
 ******************************************************************************

  NOTIFICATION FUNCTIONS
  - - - - - - - - - - - -
  The following functions are implemented by Sun.
  Platform is required to invoke these function for each occurrence of the
  undelying event.
  The functions need to be executed in platform's task/thread

 ******************************************************************************
 ******************************************************************************
 ******************************************************************************/
    
/**
 * @defgroup MiscNotification Notification API for Bluetooth
 * @ingroup Bluetooth
 * @{
 */

/**
 * @enum javacall_bt_callback_type
 * @brief Event names
 */
typedef enum {
    JAVACALL_EVENT_BT_CONNECT_COMPLETE = 1000,
    JAVACALL_EVENT_BT_SEND_COMPLETE,
    JAVACALL_EVENT_BT_RECEIVE_COMPLETE,
    JAVACALL_EVENT_BT_ACCEPT_COMPLETE, 
} javacall_bt_callback_type;

/**
 * A callback function to be called for notification of user confirmation 
 * to enable bluetooth radio.
 *
 * @param answer confirmation from user
 *      - JAVACALL_TRUE if the user has allowed to enable Bluetooth,
 *      - JAVACALL_FALSE otherwise
 * @param operation_result operation result: Either
 *      - JAVACALL_OK if operation completed successfully, 
 *      - otherwise, JAVACALL_FAIL
 */
/*OPTIONAL*/ void javanotify_bt_confirm_enable(
        javacall_bool answer,
        javacall_result operation_result);

/**
 * A callback function to be called for notification of non-blocking 
 * protocol related events.
 * The platform will invoke the call back in platform context for
 * connection related occurrence. 
 *
 * @param type type of indication: Either
 *     - JAVACALL_EVENT_BT_CONNECT_COMPLETE
 *     - JAVACALL_EVENT_BT_SEND_COMPLETE
 *     - JAVACALL_EVENT_BT_RECEIVE_COMPLETE
 *     - JAVACALL_EVENT_BT_ACCEPT_COMPLETE
 *
 * @param handle related to the notification
 * @param operation_result operation result: Either
 *      - JAVACALL_OK if operation completed successfully, 
 *      - otherwise, JAVACALL_FAIL
 */
void javanotify_bt_protocol_event(
        javacall_bt_callback_type event,
        javacall_handle handle,
        javacall_result operation_result);

/**
 * Reports to the application that remote devices have been discovered.
 *
 * @param addr Bluetooth address of the discovered device
 * @param deviceClass class of the discovered device
 */
void javanotify_bt_device_discovered(
        const javacall_bt_address addr,
        int deviceClass);

/**
 * Reports to the application that the inquiry has been completed.
 *
 * @param success indicates whether the inquiry operation succeeded
 */
void javanotify_bt_inquiry_complete(javacall_bool success);

/**
 * Reports to the application that authentication request has been completed.
 *
 * @param addr Bluetooth address of the remote device
 * @param success indicates whether the authentication succeeded
 */
void javanotify_bt_authentication_complete(
        const javacall_bt_address addr,
        javacall_bool success);

/**
 * Reports to the application that the remote device name has been retrieved.
 *
 * @param addr Bluetooth address of the remote device
 * @param name user-friendly name of the remote device
 */
void javanotify_bt_remote_name_complete(
        const javacall_bt_address addr,
        const char *name);

/**
 * Reports to the application that link encryption has been changed.
 *
 * @param address Bluetooth address of the remote device
 * @param success indicates whether the change succeeded
 * @param on indicates whether link encryption is enabled
 */
void javanotify_bt_encryption_change(
        const javacall_bt_address addr,
        javacall_bool success,
        javacall_bool on);

/** @} */
    
/******************************************************************************
 ******************************************************************************
 ******************************************************************************
    OPTIONAL FUNCTIONS
 ******************************************************************************
 ******************************************************************************
 ******************************************************************************/
    
/**
 * @defgroup jsrOptionalBluetooth Optional Bluetooth API
 * @ingroup Bluetooth
 * 
 * @{
 */

/***************************************************
 * Bluetooth Control Center
 **************************************************/

/**
 * Asks user whether Bluetooth radio is allowed to be turned on.
 *
 * This function can be either synchronous or asynchronous.
 *
 * For Synchronous:
 *  1. return <code>JAVACALL_OK</code> on success.
 *  2. return <code>JAVACALL_FAIL</code> in case prompting the dialog failed.
 *
 * For Asynchronous:
 *  1. <code>JAVACALL_WOULD_BLOCK</code> is returned immediately. 
 *  2. The notification for the user confirmation will be sent later through 
 *     javanotify_bt_confirm_enable()
 *
 * @param pBool pointer to variable where the result is to be stored:
 *              <code>JAVACALL_TRUE</code> if the permission was granted,
 *              <code>JAVACALL_FALSE</code> otherwise
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 * @retval <code>JAVACALL_WOULD_BLOCK</code> in case of asynchronous operation
 */
/*OPTIONAL*/ javacall_result javacall_bt_bcc_confirm_enable(
        /*OUT*/javacall_bool *pBool);

/**
 * Retrieves PIN code to use for pairing with a remote device. 
 *
 * If the PIN code is not known, PIN entry dialog is displayed.
 *
 * This function should be called after javanotify_bt_remote_device_event() 
 * with JAVACALL_EVENT_BT_PASSKEY_REQUEST type. 
 *
 * @param addr the Bluetooth address of the remote device
 * @param pin array to receive the PIN code (null-terminated UTF-8 encoded).
 *            The array size should be at least
 *            <code>JAVACALL_BT_MAX_PIN_LEN</code> bytes.
 *            <code>NULL</code> value of pin means invalid pass key.
 * @param ask indicates whether PIN can be retrieved from cache, or user must be
 *            asked to enter one (regardless whether cached PIN is available)
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 * @retval <code>JAVACALL_WOULD_BLOCK</code> in case of asynchronous operation
 */
/*OPTIONAL*/ javacall_result javacall_bt_bcc_put_passkey(
        const javacall_bt_address addr, 
        char *pin,
		javacall_bool ask);

/**
 * Attempts to authorize a connection.
 *
 * The BCC may be expected to consult with the user to obtain approval 
 * based on the given service record and client device information.
 *
 * This function is automatically called after
 * <code>javanotify_bt_authorize_request()</code> call.
 *
 * @param addr Bluetooth address of a remote device
 * @param record handle for the service record of the service the remote
 *               device is trying to access
 * @param pBool pointer to variable where the result is to be stored:
 *              <code>JAVACALL_TRUE</code> if the device was paired,
 *              <code>JAVACALL_FALSE</code> otherwise
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 */
/*OPTIONAL*/ javacall_result javacall_bt_bcc_authorize(
            const javacall_bt_address addr, 
            javacall_handle record,
            javacall_bool *pBool);

/**
 * Retrieves default ACL connection handle for the specified remote device.
 *
 * @param addr the Bluetooth address of the remote device
 * @param pHandle pointer to variable where the result is to be stored:
 *         default ACL connection handle in range 0x0000-0x0FFF, or
 *         <code>-1</code> when there is no connection to the remote device.
 * @retval <code>JAVACALL_OK</code> on success
 * @retval <code>JAVACALL_FAIL</code> on failure
 */
/*OPTIONAL*/ javacall_result javacall_bt_stack_get_acl_handle(
		const javacall_bt_address addr, 
		int *pHandle);

/** @} */
    
#ifdef __cplusplus
}
#endif

#endif /* _JAVACALL_BT_H */
