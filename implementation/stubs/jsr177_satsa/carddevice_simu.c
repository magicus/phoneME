/*
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

#include <javacall_carddevice.h>

/** 
 * Initializes the driver. This is not thread-safe function.
 * @return JAVACALL_OK if all done successfuly, 
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_carddevice_init() {
    return JAVACALL_NOT_IMPLEMENTED;
}

/** 
 * Finalizes the driver.
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_carddevice_finalize() {
    return JAVACALL_NOT_IMPLEMENTED;
}


/** 
 * Sets property value. If the property is used during the initialization
 * process then this method must be called before <code>javacall_carddevice_init()</code>
 * @return JAVACALL_OK if all done successfuly, 
 *         JAVACALL_NOT_IMPLEMENTED when this property is not supported
 *         JAVACALL_OUT_OF_MEMORY if there is no enough memory
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_carddevice_set_property(const char *prop_name, 
                                      const char *prop_value) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/** 
 * Selects specified slot (if possible).
 * @return JAVACALL_OK if all done successfuly
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_carddevice_select_slot(javacall_int32 slot_index) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/** 
 * Returns number of slots which available for selection.
 * @param slot_cnt Buffer for number of slots.
 * @return JAVACALL_OK if all done successfuly
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_carddevice_get_slot_count(javacall_int32 *slot_cnt) {
    *slot_cnt = 1;
    return JAVACALL_NOT_IMPLEMENTED;
}

/** 
 * Checks if this slot is SAT slot.
 * @param slot Slot number.
 * @param result <code>JAVACALL_TRUE</code> if the slot is dedicated for SAT,
 *               <code>JAVACALL_FALSE</code> otherwise
 * @return JAVACALL_OK if all done successfuly
 *         JAVACALL_WOULD_BLOCK caller must call 
 *         the javacall_carddevice_is_sat_finish function to complete 
 *         the operation
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_carddevice_is_sat_start(javacall_int32 slot,
                                                 javacall_bool *result,
                                                 void **context) {
    (void)slot;    
    (void)context;
    *result = JAVACALL_FALSE;
    return JAVACALL_NOT_IMPLEMENTED;
}

/** 
 * Checks if this slot is SAT slot.
 * @param slot Slot number.
 * @param result <code>JAVACALL_TRUE</code> if the slot is dedicated for SAT,
 *               <code>JAVACALL_FALSE</code> otherwise
 * @return JAVACALL_OK if all done successfuly
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 *         JAVACALL_WOULD_BLOCK caller must call 
 *         this function again to complete the operation
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_carddevice_is_sat_finish(javacall_int32 slot,
                                                 javacall_bool *result,
                                                 void *context) {
    (void)slot;    
    (void)context;
    *result = JAVACALL_FALSE;
    return JAVACALL_NOT_IMPLEMENTED;
}

/** 
 * Sends 'RESET' command to device and gets ATR into specified buffer.
 * @param atr Buffer to store ATR.
 * @param atr_size Before call: size of provided buffer
 *                 After call: size of received ATR.
 * @param context the context saved during asynchronous operation.
 * @return JAVACALL_OK if all done successfuly
 *         JAVACALL_WOULD_BLOCK caller must call 
 *         the javacall_carddevice_reset_finish function to complete 
 *         the operation
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_carddevice_reset_start(char *atr,
                                                javacall_int32 *atr_size,
                                                void **context) {
    (void)atr;
    (void)atr_size;
    (void)context;
    return JAVACALL_NOT_IMPLEMENTED;
}

/** 
 * Finished 'RESET' command on device and gets ATR into specified buffer.
 * Must be called after CARD_READER_DATA_SIGNAL with SIGNAL_RESET parameter is
 * received.
 * @param atr Buffer to store ATR.
 * @param atr_size Before call: size of provided buffer
 *                 After call: size of received ATR.
 * @param context the context saved during asynchronous operation.
 * @return JAVACALL_OK if all done successfuly
 *         JAVACALL_WOULD_BLOCK caller must call 
 *         this function again to complete the operation
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_carddevice_reset_finish(char *atr,
                                                 javacall_int32 *atr_size,
                                                 void *context) {
    (void)atr;
    (void)atr_size;
    (void)context;
    return JAVACALL_NOT_IMPLEMENTED;
}

/** 
 * Performs platform lock of the device. This is intended to make
 * sure that no other native application
 * uses the same device during a transaction.
 * @return JAVACALL_OK if all done successfuly, 
 *         JAVACALL_WOULD_BLOCK if the device is locked by the other
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_carddevice_lock() {
    return JAVACALL_NOT_IMPLEMENTED;
}

/** 
 * Unlocks the device.
 * @return JAVACALL_OK if all done successfuly
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_carddevice_unlock() {
    return JAVACALL_NOT_IMPLEMENTED;
}

/** 
 * Retrieves current slot's card movement events from driver.
 * Events is retrieved as bit mask. It can include
 * all movements from last reading, but can contain only the last.
 * Enum JAVACALL_CARD_MOVEMENT should be used to specify type of movement.
 * Clears the slot event state.
 * @param mask Movements retrived.
 * @return JAVACALL_OK if all done successfuly
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_carddevice_card_movement_events(JAVACALL_CARD_MOVEMENT *mask) {
    *mask = 0;
    return JAVACALL_NOT_IMPLEMENTED;
}

/** 
 * Transfers APDU data to the device and receives response from the device.
 * @param tx_buffer Buffer with APDU to be sent.
 * @param tx_size Size of APDU.
 * @param rx_buffer Buffer to store the response.
 * @param rx_size Before call: size of <tt>rx_buffer</tt>
 *                 After call: size of received response.
 * @return JAVACALL_OK if all done successfuly
 *         JAVACALL_WOULD_BLOCK caller must call 
 *         the javacall_carddevice_xfer_data_finish function to complete 
 *         the operation
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_carddevice_xfer_data_start(char *tx_buffer,
                                                    javacall_int32 tx_size,
                                                    char *rx_buffer,
                                                    javacall_int32 *rx_size,
                                                    void **context) {
    (void)tx_buffer;
    (void)tx_size;
    (void)rx_buffer;
    (void)rx_size;
    (void)context;
    return JAVACALL_NOT_IMPLEMENTED;
}

/** 
 * Transfers APDU data to the device and receives response from the device.
 * @param tx_buffer Buffer with APDU to be sent.
 * @param tx_size Size of APDU.
 * @param rx_buffer Buffer to store the response.
 * @param rx_size Before call: size of <tt>rx_buffer</tt>
 *                 After call: size of received response.
 * @return JAVACALL_OK if all done successfuly
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 *         JAVACALL_WOULD_BLOCK caller must call 
 *         this function again to complete the operation
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_carddevice_xfer_data_finish(char *tx_buffer,
                                                     javacall_int32 tx_size,
                                                     char *rx_buffer,
                                                     javacall_int32 *rx_size,
                                                     void *context) {
    (void)tx_buffer;
    (void)tx_size;
    (void)rx_buffer;
    (void)rx_size;
    (void)context;
    return JAVACALL_NOT_IMPLEMENTED;
}

/** 
 * Clears error state.
 */
void javacall_carddevice_clear_error() {
}

/** 
 * Sets error state and stores message (like printf).
 * @param fmt printf-like format string
 */
void javacall_carddevice_set_error(const char *fmt, ...) {
    (void)fmt;
}

/** 
 * Retrieves error message into the provided buffer and clears state.
 * @param buf Buffer to store message
 * @param buf_size Size of the buffer in bytes
 * @return JAVACALL_TRUE if error messages were returned, JAVACALL_FALSE otherwise
 */
javacall_bool javacall_carddevice_get_error(char *buf, javacall_int32 buf_size) {
    (void)buf;
    (void)buf_size;
    return JAVACALL_FALSE;
}

