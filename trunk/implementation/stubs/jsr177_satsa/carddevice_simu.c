/*
 *   
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
           JAVACALL_FAIL otherwise
 */
javacall_result javacall_carddevice_finalize() {
    return JAVACALL_NOT_IMPLEMENTED;
}


javacall_result javacall_carddevice_set_property(const char *prop_name, 
                                      const char *prop_value) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/** 
 * Selects specified slot (if possible).
 * @return JAVACALL_OK if all done successfuly, 
           JAVACALL_FAIL otherwise
 */
javacall_result javacall_carddevice_select_slot(int slot_index) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/** 
 * Returns number of slots which available for selection.
 * @param slot_cnt Buffer for number of slots.
 * @return JAVACALL_OK if all done successfuly, 
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_carddevice_get_slot_count(int *slot_cnt) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/** 
 * Checks if this slot is SAT slot.
 * @param slot Slot number.
 * @param result <code>JAVACALL_TRUE</code> if the slot is dedicated for SAT,
 *               <code>JAVACALL_FALSE</code> otherwise
 * @return JAVACALL_OK if all done successfuly, 
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_carddevice_is_sat(int slot, javacall_bool *result) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/** 
 * Sends 'RESET' ('POWER UP') command to device and gets ATR 
 * into specified buffer.
 * @param atr Buffer to store ATR.
 * @param atr_size Before call: size of provided buffer
 *                 After call: length of received ATR.
 * @return JAVACALL_OK if all done successfuly, JAVACALL_FAIL otherwise
 */
static javacall_result javacall_carddevice_reset(char *atr, int *atr_size) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/** 
 * Sends 'POWER DOWN' command to device.
 * @return JAVACALL_OK if all done successfuly, JAVACALL_FAIL otherwise
 */
javacall_result javacall_carddevice_power_down() {
    return JAVACALL_NOT_IMPLEMENTED;
}

/** 
 * Performs platform lock of the device. 
 * @return JAVACALL_OK if all done successfuly, 
           JAVACALL_WOULD_BLOCK if the device is locked by the other
 *         JAVACALL_FAIL if error occured
 */
javacall_result javacall_carddevice_lock() {
    return JAVACALL_NOT_IMPLEMENTED;
}

/** 
 * Unlocks the device.
 * @return JAVACALL_OK if all done successfuly, 
 * JAVACALL_FAIL otherwise
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
 * @return JAVACALL_OK if all done successfuly, JAVACALL_FAIL otherwise.
 */
javacall_result javacall_carddevice_card_movement_events(JAVACALL_CARD_MOVEMENT *events) {
    return JAVACALL_NOT_IMPLEMENTED;
}

/**
 */
javacall_result javacall_carddevice_reset_start(char *rx_buffer, 
                                                int *rx_length,
                                                void **context) {
    return JAVACALL_NOT_IMPLEMENTED;
}

javacall_result javacall_carddevice_reset_finish(char *rx_buffer, int *rx_length,
                                      void *context) {
    return JAVACALL_NOT_IMPLEMENTED;
}


javacall_result javacall_carddevice_xfer_data_start(char *tx_buffer, 
                                         int tx_length,
                                         char *rx_buffer, int *rx_length,
                                         void **context) {
    return JAVACALL_NOT_IMPLEMENTED;
}

javacall_result javacall_carddevice_xfer_data_finish(char *tx_buffer, 
                                                     int tx_length,
                                                     char *rx_buffer, 
                                                     int *rx_length,
                                                     void *context) {
    return JAVACALL_NOT_IMPLEMENTED;
}

void javacall_carddevice_clear_error() { // empty
}

void javacall_carddevice_set_error(const char *fmt, ...) {
}

int javacall_carddevice_vsnprintf(char *buffer, int len, const char *fmt, va_list ap) {
    return 0;
}

javacall_bool javacall_carddevice_get_error(char *buf, int buf_size) {
    return JAVACALL_FALSE;
}

