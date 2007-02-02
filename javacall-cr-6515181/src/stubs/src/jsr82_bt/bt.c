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

#ifdef __cplusplus
extern "C" {
#endif

#include "javacall_defs.h" 
#include "javacall_bt.h" 

/***************************************************
 * Bluetooth Control Center
 **************************************************/

/**
 * Allocates BCC related native resources.
 *
 * @retval JAVACALL_OK on success,
 * @retval JAVACALL_FAIL if an error occurred
 */
javacall_result javacall_bt_bcc_initialize(void) {
    return JAVACALL_FAIL;
}

/**
 * Releases BCC related native resources.
 *
 * @retval JAVACALL_OK on success,
 * @retval JAVACALL_FAIL if an error occurred
 */
javacall_result javacall_bt_bcc_finalize(void) {
    return JAVACALL_FAIL;
}

/**
 * Determines if the local device is in connectable mode.
 *
 * @param pBool pointer to variable where the result is to be stored:
 *         JAVACALL_TRUE if the device is connectable,
 *         JAVACALL_FALSE otherwise
 * @retval JAVACALL_OK on success,
 * @retval JAVACALL_FAIL on failure
 */
javacall_result javacall_bt_bcc_is_connectable(/*OUT*/javacall_bool *pBool) {
    return JAVACALL_FAIL;
}

/**
 * Checks if the local device has a bond with a remote device.
 *
 * @param addr Bluetooth address of a remote device
 * @param pBool pointer to variable where the result is to be stored:
 *         JAVACALL_TRUE if the devices are paired,
 *         JAVACALL_FALSE otherwise
 * @retval JAVACALL_OK on success,
 * @retval JAVACALL_FAIL on failure
 */
javacall_result javacall_bt_bcc_is_paired(
        const javacall_bt_address addr, 
        /*OUT*/javacall_bool *pBool) {
    return JAVACALL_FAIL;
}

/**
 * Checks if a remote device was authenticated.
 *
 * @param addr Bluetooth address of a remote device
 * @param pBool pointer to variable where the result is to be stored:
 *         JAVACALL_TRUE if the remote device is authenticated,
 *         JAVACALL_FALSE otherwise
 * @retval JAVACALL_OK on success,
 * @retval JAVACALL_FAIL on failure
 */
javacall_result javacall_bt_bcc_is_authenticated(
        const javacall_bt_address addr, 
        /*OUT*/javacall_bool *pBool) {
    return JAVACALL_FAIL;
}

/**
 * Checks if a remote device is trusted (authorized for all services).
 *
 * BCC may allow the user to permanently authorize a remote device 
 * for all local services. When a device is authorized in this way, 
 * it is known as a "trusted device"
 *
 * @param addr Bluetooth address of a remote device
 * @param pBool pointer to variable where the result is to be stored:
 *         JAVACALL_TRUE if the remote device is trusted,
 *         JAVACALL_FALSE otherwise
 * @retval JAVACALL_OK on success,
 * @retval JAVACALL_FAIL on failure
 */
javacall_result javacall_bt_bcc_is_trusted(
        const javacall_bt_address addr, 
        /*OUT*/javacall_bool *pBool) {
    return JAVACALL_FAIL;
}

/**
 * Determines if data exchanges with a remote device are being encrypted.
 *
 * @param addr Bluetooth address of a remote device
 * @param pBool pointer to variable where the result is to be stored:
 *         JAVACALL_TRUE if connection to the device is encrypted,
 *         JAVACALL_FALSE otherwise
 * @retval JAVACALL_OK on success,
 * @retval JAVACALL_FAIL on failure
 */
javacall_result javacall_bt_bcc_is_encrypted(
        const javacall_bt_address addr, 
        /*OUT*/javacall_bool *pBool) {
    return JAVACALL_FAIL;
}

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
 * @param devices an output array which will receive the list of preknown devices
 * @param pCount pointer to variable where the result is to be stored:
 *         number of records stored in the output array.
 * @retval JAVACALL_OK on success,
 * @retval JAVACALL_FAIL on failure
*/
javacall_result javacall_bt_bcc_get_preknown_devices(
        /*OUT*/javacall_bt_address devices[JAVACALL_BT_MAX_PREKNOWN_DEVICES], 
        /*OUT*/int *pCount) {
    return JAVACALL_FAIL;
}

/***************************************************
 * Bluetooth Stack
 **************************************************/

/**
 * Gets the contorl of Bluetooth stack.
 *
 * @retval JAVACALL_OK on success,
 * @retval JAVACALL_FAIL if an error occurred
 */
javacall_result javacall_bt_stack_initialize(void) {
    return JAVACALL_FAIL;
}

/**
 * Releases the contorl of Bluetooth stack.
 *
 * @retval JAVACALL_OK on success,
 * @retval JAVACALL_FAIL if an error occurred
 */
javacall_result javacall_bt_stack_finalize(void) {
    return JAVACALL_FAIL;
}

/**
 * Checks if the Bluetooth radio is enabled.
 *
 * @param pBool pointer to variable where the result is to be stored:
 *         JAVACALL_TRUE if the Bluetooth radio is enabled,
 *         JAVACALL_FALSE otherwise
 * @retval JAVACALL_OK on success,
 * @retval JAVACALL_FAIL on failure
 */
javacall_result javacall_bt_stack_is_enabled(
        /*OUT*/javacall_bool *pBool) {
    return JAVACALL_FAIL;
}

/**
 * Enables Bluetooth radio.
 *
 * If this function returns JAVACALL_WOULD_BLOCK, the notification for 
 * operation completion will be sent later through javanotify_bt_event()
 * with JAVACALL_EVENT_BT_ENABLE_RADIO_COMPLETE.
 *
 * @retval JAVACALL_OK on success,
 * @retval JAVACALL_WOULD_BLOCK if the caller needs a notification to complete the operation
 * @retval JAVACALL_FAIL on failure
 */
javacall_result javacall_bt_stack_enable(void) {
    return JAVACALL_FAIL;
}

/**
 * Returns Bluetooth address of the local device.
 *
 * If JAVACALL_WOULD_BLOCK is returned,
 * this function should be called again after the notification through 
 * javanotify_bt_event() with JAVACALL_EVENT_BT_LOCAL_ADDRESS_COMPLETE type. 
 *
 * @param pAddr pointer to variable where the result is to be stored:
 *         Bluetooth address of the local device
 * @retval JAVACALL_OK on success,
 * @retval JAVACALL_WOULD_BLOCK if the caller needs a notification to complete the operation
 * @retval JAVACALL_FAIL on failure
 */
javacall_result javacall_bt_stack_get_local_address(
        /*OUT*/javacall_bt_address* pAddr) {
    return JAVACALL_FAIL;
}

/**
 * Retrieves user-friendly name for the local device.
 *
 * If JAVACALL_WOULD_BLOCK is returned,
 * this function should be called again after the notification through 
 * javanotify_bt_event() with JAVACALL_EVENT_BT_LOCAL_NAME_COMPLETE type. 
 *
 * @param pName string to store the name of the local device
 *          (null-terminated in UTF-8 encoding).
 *          The length should be JAVACALL_BT_MAX_USER_FRIENDLY_NAME.
 * @retval JAVACALL_OK on success,
 * @retval JAVACALL_WOULD_BLOCK if the caller needs a notification to complete the operation
 * @retval JAVACALL_FAIL if an error occurred
 */
javacall_result javacall_bt_stack_get_local_name(
        /*OUT*/ char pName[JAVACALL_BT_MAX_USER_FRIENDLY_NAME]) {
    return JAVACALL_FAIL;
}

/**
 * Sets major service class bits of the device.
 *
 * @param classes an integer whose binary representation indicates the major
 *        service class bits that should be set
 * @retval JAVACALL_OK on success,
 * @retval JAVACALL_FAIL if an error occurred
 */
javacall_result javacall_bt_stack_set_service_classes(int classes) {
    return JAVACALL_FAIL;
}

/**
 * Retrieves the class of local device value that represents the service 
 * classes, major device class, and minor device class of the local device.
 *
 * @param pValue pointer to variable where the result is to be stored:
 *         class of device value.
 * @retval JAVACALL_OK on success,
 * @retval JAVACALL_FAIL if an error occurred
 */
javacall_result javacall_bt_stack_get_device_class(
        /*OUT*/ int *pValue) {
    return JAVACALL_FAIL;
}

/**
 * Retrieves the inquiry access code that the local Bluetooth device is
 * scanning for during inquiry scans.
 *
 * @param pValue pointer to variable where the result is to be stored:
 *         inquiry access code.
 * @retval JAVACALL_OK on success,
 * @retval JAVACALL_FAIL if an error occurred
 */
javacall_result javacall_bt_stack_get_access_code(
        /*OUT*/ int *pValue) {
    return JAVACALL_FAIL;
}

/**
 * Sets the inquiry access code that the local Bluetooth device is
 * scanning for during inquiry scans.
 *
 * @param accessCode inquiry access code to be set (valid values are in the
 *        range 0x9e8b00 to 0x9e8b3f), or 0 to take the device out of
 *        discoverable mode
 * @retval JAVACALL_OK on success,
 * @retval JAVACALL_FAIL if an error occurred
 */
javacall_result javacall_bt_set_access_code(int accessCode) {
    return JAVACALL_FAIL;
}

/**
 * Starts asynchronous device discovery.
 *
 * Whenever a device is discovered it is notified through 
 * javanotify_bt_device_discovered().
 * The notification of inquiry completion is delivered through 
 * javanotify_bt_event() with JAVACALL_EVENT_BT_INQUIRY_COMPLETE type. 
 *
 * @param accessCode the type of inquiry
 * @retval JAVACALL_OK on success,
 * @retval JAVACALL_FAIL on failure
 */
javacall_result javacall_bt_stack_start_inquiry(int accessCode) {
    return JAVACALL_FAIL;
}

/**
 * Cancels asynchronous device discovery.
 *
 * @retval JAVACALL_OK on success,
 * @retval JAVACALL_FAIL on failure
 */
javacall_result javacall_bt_stack_cancel_inquiry(void) {
    return JAVACALL_FAIL;
}

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
        const javacall_bt_address addr) {
    return JAVACALL_FAIL;
}

/**
 * Attempts to authenticate a remote device.
 *
 * If JAVACALL_WOULD_BLOCK is returned,
 * this function should be called again after the notification through 
 * javanotify_bt_remote_device_event with 
 * JAVACALL_EVENT_BT_AUTHENTICATE_COMPLETE type. 
 *
 * @param addr  remote device
 * @retval JAVACALL_OK on success,
 * @retval JAVACALL_WOULD_BLOCK if the caller needs a notification to complete the operation
 * @retval JAVACALL_FAIL if an error occurred
 */
javacall_result javacall_bt_stack_authenticate(
        const javacall_bt_address addr) {
    return JAVACALL_FAIL;
}

/**
 * Attempts to encrypt all connections to the given remote device.
 *
 * If JAVACALL_WOULD_BLOCK is returned,
 * the notification is delivered through javanotify_bt_remote_device_event() 
 * with JAVACALL_EVENT_BT_ENCRYPT_COMPLETE type. 
 *
 * @param addr  remote device
 * @param enable specifies if encryption to be enabled or disabled
 *
 * @retval JAVACALL_OK on success,
 * @retval JAVACALL_WOULD_BLOCK if the caller needs a notification to complete the operation
 * @retval JAVACALL_FAIL on failure
 */
javacall_result javacall_bt_stack_encrypt(
        const javacall_bt_address addr,
        javacall_bool enable) {
    return JAVACALL_FAIL;
}

/**
 * Creates or updates service record in the service discovery database (SDDB)
 * provided by native stack implementation.
 *
 * @param handle handle of the service record to be updated; if equals to
 *               0, a new record will be created
 * @param data binary data containing attribute-value pairs in the format
 *             identical to the one used in the AttributeList parameter of
 *             the SDP_ServiceAttributeResponse PDU
 *             (see Bluetooth 1.1 specification)
 * @param len length of the buffer in bytes
 * @param pHandle pointer to variable where the result is to be stored:
 *         service record handle, or 0 if the operation failed
 * @retval JAVACALL_OK on success,
 * @retval JAVACALL_FAIL on failure
 */
javacall_result javacall_bt_stack_update_record(
        javacall_handle handle, 
        const void *data,
        int len,
        /*OUT*/javacall_handle *pHandle) {
    return JAVACALL_FAIL;
}

/**
 * Removes service record from the service discovery database (SDDB)
 * provided by the native implementation.
 *
 * @param handle hanlde of the service record to be deleted
 * @retval JAVACALL_OK on success,
 * @retval JAVACALL_FAIL on failure
 */
javacall_result javacall_bt_stack_remove_record(
        javacall_handle handle) {
    return JAVACALL_FAIL;
}

/***************************************************
 * L2CAP protocol
 **************************************************/

/**
 * Closes the connection.
 *
 * Determines whether the connection is not closed, if so closes it.
 *
 * @param handle connection handle
 * @retval JAVACALL_OK on success,
 * @retval JAVACALL_FAIL if connection is already closed or
 *                       an error occured during close operation
 */
javacall_result javacall_bt_l2cap_close(javacall_handle handle) {
    return JAVACALL_FAIL;
}

/**
 * Creates a new server connection.
 *
 * The method creates a server connection instance
 * but does not put it in listen mode.
 * Anyway it selects and reserves a free PSM to listen for
 * incoming connections on after the listen method is called.
 *
 * @param receiveMTU Input MTU size, 0 means the default value.
 * @param transmitMTU Output MTU size, 0 means the default value.
 * @param authenticate  JAVACALL_TRUE if authication is required
 * @param authorize     JAVACALL_TRUE if authorization is required
 * @param encrypt       JAVACALL_TRUE if required to be encrypted
 * @param master        JAVACALL_TRUE if required to be a connection's master
 * @param pHandle pointer to connection handle variable,
 *               new connection handle returned in result.
 * @param psm pointer to variable, where reserved PSM is returned in.
 *
 * @retval JAVACALL_OK on success,
 * @retval JAVACALL_FAIL otherwise
 */
javacall_result javacall_bt_l2cap_create_server(
        int receiveMTU,
        int transmitMTU,
        javacall_bool authenticate,
        javacall_bool authorize,
        javacall_bool encrypt,
        javacall_bool master,
        /*OUT*/javacall_handle* pHandle,
        /*OUT*/int* pPsm) {
    return JAVACALL_FAIL;
}

/**
 * Puts server connection to listening mode.
 *
 * @param handle server connection handle
 * @retval JAVACALL_OK on success,
 * @retval JAVACALL_FAIL otherwise
 */
javacall_result javacall_bt_l2cap_listen(
        javacall_handle handle) {
    return JAVACALL_FAIL;
}

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
 * Creates a new client connection.
 *
 * The method does not establishes real bluetooth connection
 * just creates a client connection instance.
 *
 * @param receiveMTU Input MTU size, 0 means the default value
 * @param transmitMTU Output MTU size, 0 means the default value
 * @param authenticate  JAVACALL_TRUE if authication is required
 * @param encrypt       JAVACALL_TRUE if required to be encrypted
 * @param master        JAVACALL_TRUE if required to be a connection's master
 * @param pHandle pointer to connection handle variable,
 *               new connection handle returned in result.
 * @retval JAVACALL_OK on success,
 * @retval JAVACALL_FAIL otherwise
 */
javacall_result javacall_bt_l2cap_create_client(
        int receiveMTU,
        int transmitMTU,
        javacall_bool authenticate,
        javacall_bool encrypt,
        javacall_bool master,
        /*OUT*/javacall_handle* pHandle) {
    return JAVACALL_FAIL;
}

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
 * Sends data via connection.
 *
 * If the size of pData is greater than the Transmit MTU, 
 * then only the first Transmit MTU bytes of the packet are sent.
 * Even if size of the buffer is zero, an empty L2CAP packet should be sent.
 *
 * If JAVACALL_WOULD_BLOCK is returned,
 * this function should be called again after the notification through 
 * javanotify_bt_protocol_event() with JAVACALL_EVENT_BT_SEND_COMPLETE type. 
 *
 * @param handle connection handle
 * @param pData pointer to data buffer
 * @param len length of the data
 * @param pBytesSent number of bytes that were really sent
 * @retval JAVACALL_OK on success,
 * @retval JAVACALL_WOULD_BLOCK if the caller needs a notification to complete the operation
 * @retval JAVACALL_FAIL otherwise
 */
javacall_result javacall_bt_l2cap_send(
        javacall_handle handle,
        const char* pData, 
        int len, 
        /*OUT*/ int* pBytesSent) {
    return JAVACALL_FAIL;
}

/**
 * Receives data via connection.
 *
 * If size of the buffer is less than size of the received packet,
 * the rest of the packet is discarded.
 *
 * If JAVACALL_WOULD_BLOCK is returned,
 * this function should be called again after the notification through 
 * javanotify_bt_protocol_event() with JAVACALL_EVENT_BT_RECEIVE_COMPLETE type. 
 *
 * @param handle connection handle
 * @param pData pointer to data buffer
 * @param len length of the buffer
 * @param pBytesRead number of bytes that were received,
 *             0 indicates end-of-data
 * @retval JAVACALL_OK on success,
 * @retval JAVACALL_WOULD_BLOCK if the caller needs a notification to complete the operation
 * @retval JAVACALL_FAIL otherwise
 */
javacall_result javacall_bt_l2cap_receive(
        javacall_handle handle,
        char* pData, 
        int len, 
        /*OUT*/ int* pBytesRead) {
    return JAVACALL_FAIL;
}

/**
 * Determines if there is data to be read from the connection
 * without blocking.
 * 
 * @param handle connection handle
 * @param pReady pointer to variable result is stored in
 *          JAVACALL_TRUE if there are data available
 *          JAVACALL_FALSE otherwise
 *
 * @retval JAVACALL_OK on success,
 * @retval JAVACALL_FAIL otherwise
 */
javacall_result javacall_bt_l2cap_get_ready(
        javacall_handle handle,
        /*OUT*/javacall_bool* pReady) {
    return JAVACALL_FAIL;
}

/***************************************************
 * RFCOMM protocol
 **************************************************/

/**
 * Closes the connection.
 *
 * Determines whether the connection is not closed, if so closes it.
 *
 * @param handle connection handle
 * @retval JAVACALL_OK on success,
 * @retval JAVACALL_FAIL if connection is already closed or
 *                       an error occured during close operation
 */
javacall_result javacall_bt_rfcomm_close(javacall_handle handle) {
    return JAVACALL_FAIL;
}

/**
 * Creates a new server connection.
 *
 * The method creates a server connection instance
 * but does not put it in listen mode.
 * Anyway it selects and reserves a free channel to listen for
 * incoming connections on after the listen method is called.
 *
 * @param authenticate  JAVACALL_TRUE if authication is required
 * @param authorize     JAVACALL_TRUE if authorization is required
 * @param encrypt       JAVACALL_TRUE if required to be encrypted
 * @param master        JAVACALL_TRUE if required to be a connection's master
 * @param pHandle pointer to connection handle variable,
 *               new connection handle returned in result.
 * @param cn pointer to variable, where reserved channel is returned in.
 *
 * @retval JAVACALL_OK on success,
 * @retval JAVACALL_FAIL otherwise
 */
javacall_result javacall_bt_rfcomm_create_server(
        javacall_bool authenticate,
        javacall_bool authorize,
        javacall_bool encrypt,
        javacall_bool master,
        /*OUT*/javacall_handle* pHandle,
        /*OUT*/int* cn) {
    return JAVACALL_FAIL;
}

/**
 * Puts server connection to listening mode.
 *
 * @param handle server connection handle
 * @retval JAVACALL_OK on success,
 * @retval JAVACALL_FAIL otherwise
 */
javacall_result javacall_bt_rfcomm_listen(
        javacall_handle handle) {
    return JAVACALL_FAIL;
}

/**
 * Accepts pending incoming connection if any.
 *
 * If JAVACALL_WOULD_BLOCK is returned,
 * this function should be called again after the notification through 
 * javanotify_bt_event() with JAVACALL_EVENT_BT_ACCEPT_COMPLETE type.
 *
 * @param handle server connection handle
 * @param pPeerHandle pointer to peer handle to store new connection handle
 *             to work with accepted incoming client connection
 * @param pPeerAddr bluetooth address variable to store
 *                  the address of accepted client, 
 *                  if NULL the value is not returned
 * @retval JAVACALL_OK on success,
 * @retval JAVACALL_FAIL on error,
 * @retval JAVACALL_WOULD_BLOCK if the caller needs a notification to complete the operation
 */
javacall_result javacall_bt_rfcomm_accept(
        javacall_handle handle, 
        /*OUT*/javacall_handle* pPeerHandle,
        /*OUT*/javacall_bt_address* pPeerAddr) {
    return JAVACALL_FAIL;
}

/**
 * Creates a new client connection.
 *
 * The method does not establishes real bluetooth connection
 * just creates a client connection instance.
 *
 * @param authenticate  JAVACALL_TRUE if authication is required
 * @param encrypt       JAVACALL_TRUE if required to be encrypted
 * @param master        JAVACALL_TRUE if required to be a connection's master
 * @param pHandle pointer to connection handle variable,
 *               new connection handle returned in result.
 * @retval JAVACALL_OK on success,
 * @retval JAVACALL_FAIL on error
 */
javacall_result javacall_bt_rfcomm_create_client(
        javacall_bool authenticate,
        javacall_bool encrypt,
        javacall_bool master,
        /*OUT*/javacall_handle* pHandle) {
    return JAVACALL_FAIL;
}

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
 * Initiates data sending via connection.
 *
 * If JAVACALL_WOULD_BLOCK is returned,
 * this function should be called again after the notification through 
 * javanotify_bt_protocol_event() with JAVACALL_EVENT_BT_SEND_COMPLETE type. 
 *
 * @param handle connection handle
 * @param pData pointer to data buffer
 * @param len length of the data
 * @param pBytesSent number of bytes that were really sent
 * @retval JAVACALL_OK on success,
 * @retval JAVACALL_WOULD_BLOCK if the caller needs a notification to complete the operation
 * @retval JAVACALL_FAIL on error
 */
javacall_result javacall_bt_rfcomm_send(
        javacall_handle handle,
        const char* pData, 
        int len, 
        /*OUT*/ int* pBytesSent) {
    return JAVACALL_FAIL;
}

/**
 * Initiates data receiving via connection.
 *
 * If JAVACALL_WOULD_BLOCK is returned,
 * this function should be called again after the notification through 
 * javanotify_bt_protocol_event() with JAVACALL_EVENT_BT_RECEIVE_COMPLETE type. 
 *
 * @param handle connection handle
 * @param pData pointer to data buffer
 * @param len length of the buffer
 * @param pBytesRead number of bytes that were received,
 *             0 indicates end-of-data
 * @retval JAVACALL_OK on success,
 * @retval JAVACALL_WOULD_BLOCK if the caller needs a notification to complete the operation
 * @retval JAVACALL_FAIL on error
 */
javacall_result javacall_bt_rfcomm_receive(
        javacall_handle handle,
        char* pData, 
        int len, 
        /*OUT*/ int* pBytesRead) {
    return JAVACALL_FAIL;
}

/**
 * Returns the number of bytes available to be read from the connection
 * without blocking.
 * 
 * @param handle connection handle
 * @param pCount pointer to variable the number of available bytes is stored in
 *
 * @retval JAVACALL_OK on success,
 * @retval JAVACALL_FAIL on error
 */
javacall_result javacall_bt_rfcomm_get_available(
        javacall_handle handle,
        /*OUT*/int* pCount) {
    return JAVACALL_FAIL;
}
    
/******************************************************************************
 ******************************************************************************
 ******************************************************************************
    OPTIONAL FUNCTIONS
 ******************************************************************************
 ******************************************************************************
 ******************************************************************************/

/***************************************************
 * Bluetooth Control Center
 **************************************************/

/**
 * Asks user whether Bluetooth radio is allowed to be turned on.
 *
 * This function can be either synchronous or asynchronous.
 *
 * For Synchronous:
 *  1. return JAVACALL_OK on success.
 *  2. return JAVACALL_FAIL in case prompting the dialog failed.
 *
 * For Asynchronous:
 *  1. JAVACALL_WOULD_BLOCK is returned immediately. 
 *  2. The notification for the user confirmation will be sent later through 
 *      javanotify_bt_confirm_enable()
 *
 * @param pBool pointer to variable where the result is to be stored:
 *         JAVACALL_TRUE if the user has allowed to enable Bluetooth,
 *         JAVACALL_FALSE otherwise
 * @retval JAVACALL_OK on success,
 * @retval JAVACALL_FAIL if an error occurred
 * @retval JAVACALL_WOULD_BLOCK if the caller needs a notification to complete the operation
 */
/*OPTIONAL*/ javacall_result javacall_bt_bcc_confirm_enable(/*OUT*/javacall_bool* pBool) {
    return JAVACALL_FAIL;
}

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
/*OPTIONAL*/ 
javacall_result javacall_bt_bcc_authorize(
            const javacall_bt_address addr, 
            javacall_handle record,
            javacall_bool *pBool);


#ifdef __cplusplus
}
#endif

