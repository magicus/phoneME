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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <javacall_defs.h>
#include <javacall_memory.h>
#include <javacall_logging.h>
#include <javacall_carddevice.h>
#include <kni_globals.h>
#include "carddevice.h"

/** 
 * Clears error state.
 */
void jsr177_clear_error() {
    javacall_carddevice_clear_error();
}

/** 
 * Sets error state and stores message (like printf).
 * @param fmt printf-like format string
 */
void jsr177_set_error(const char *fmt, ...) {
	va_list ap;
    int len = 0;
    
	va_start(ap, fmt);
    len += sprintf(gKNIBuffer, "JSR_177 ERROR: ");
    len += javacall_carddevice_vsnprintf(gKNIBuffer + len, 
                                         KNI_BUFFER_SIZE - len, 
                                         fmt, ap);
	javacall_carddevice_set_error(gKNIBuffer);
	va_end(ap);
}

/** 
 * Retrives error message into provided buffer and clears state.
 * @param buf Buffer to store message
 * @param buf_size Size of the buffer in bytes
 * @return PCSL_TRUE if error messages were returned, PCSL_FALSE otherwise
 */
jboolean jsr177_get_error(jbyte *buf, jsize buf_size) {
    return javacall_carddevice_get_error(buf, buf_size) == 
        JAVACALL_FALSE ? PCSL_FALSE : PCSL_TRUE;
}

/*
 * JSR177 Card driver API functions
 */

/** 
 * Initializes the driver.
 * @return JSR177_STATUSCODE_OK if all done successfuly, 
 *         JSR177_STATUSCODE_NOT_IMPLEMENTED when the stub was called
 *         JSR177_STATUSCODE_FAIL otherwise
 */
JSR177_STATUSCODE jsr177_init() {
    javacall_result result = javacall_carddevice_init();
    switch (result) {
    case JAVACALL_OK:
        return JSR177_STATUSCODE_OK;
    case JAVACALL_FAIL:
        return JSR177_STATUSCODE_FAIL;
    case JAVACALL_NOT_IMPLEMENTED:
    default:
        return JSR177_STATUSCODE_NOT_IMPLEMENTED;
    }
}

/** 
 * De-initializes the driver.
 * @return JSR177_STATUSCODE_OK is if all done successfuly, JSR177_STATUSCODE_FAIL otherwise
 */
JSR177_STATUSCODE jsr177_finalize() {
    javacall_carddevice_finalize();
    return JSR177_STATUSCODE_OK;
}

javacall_result jsr177_set_property(const char *prop_name, 
                                      const char *prop_value) {

    return javacall_carddevice_set_property(prop_name, prop_value);
}
/** 
 * Returns number of slots which available for selection.
 * @param slot_cnt Buffer for number of slots.
 * @return JSR177_STATUSCODE_OK if all done successfuly, JSR177_STATUSCODE_FAIL otherwise
 */
JSR177_STATUSCODE jsr177_get_slot_count(jint *slot_cnt) {
    int s_cnt;
    javacall_result result = javacall_carddevice_get_slot_count(&s_cnt);
    switch (result) {
    case JAVACALL_OK:
        *slot_cnt = s_cnt;
        return JSR177_STATUSCODE_OK;
    case JAVACALL_FAIL:
    default:
        return JSR177_STATUSCODE_FAIL;
    }
}

/** 
 * Checks if this slot is SAT slot.
 * @param slot Slot number.
 * @param result <code>PCSL_TRUE</code> if the slot is dedicated for SAT,
 *               <code>PCSL_FALSE</code> otherwise
 * @return JSR177_STATUSCODE_OK if all done successfuly, JSR177_STATUSCODE_FAIL otherwise
 */
JSR177_STATUSCODE jsr177_is_sat(jint slot, jboolean *result) {
    javacall_bool rez;
    javacall_result jresult = javacall_carddevice_is_sat(slot, &rez);
    switch (jresult) {
    case JAVACALL_OK:
        if (rez == JAVACALL_FALSE) {
            *result = PCSL_FALSE;
        } else {
            *result = PCSL_TRUE;
        }
        return JSR177_STATUSCODE_OK;
    case JAVACALL_FAIL:
    default:
        return JSR177_STATUSCODE_FAIL;
    }
}

/** 
 * Selects specified slot (if possible).
 * @return JSR177_STATUSCODE_OK is if all done successfuly, JSR177_STATUSCODE_FAIL otherwise
 */
JSR177_STATUSCODE jsr177_select_slot(jint slot_index) {
    javacall_result result = javacall_carddevice_select_slot(slot_index);
    switch (result) {
    case JAVACALL_OK:
        return JSR177_STATUSCODE_OK;
    case JAVACALL_FAIL:
    default:
        return JSR177_STATUSCODE_FAIL;
    }
}

/** 
 * Sends 'RESET' (POWER UP) command to device and gets ATR into specified buffer.
 * @param atr Buffer to store ATR.
 * @param atr_size Before call: size of provided buffer
 *                 After call: size of received ATR.
 * @return JSR177_STATUSCODE_OK is if all done successfuly, JSR177_STATUSCODE_FAIL otherwise
 */
JSR177_STATUSCODE jsr177_reset_start(jbyte *atr, jsize *atr_size, void **context) {
    int tmp_atr_size;
    javacall_result result = javacall_carddevice_reset_start(atr, &tmp_atr_size, context);
    switch (result) {
    case JAVACALL_OK:
        *atr_size = tmp_atr_size;
        return JSR177_STATUSCODE_OK;
    case JAVACALL_FAIL:
    default:
        return JSR177_STATUSCODE_FAIL;
    case JSR177_STATUSCODE_WOULD_BLOCK:
        return JSR177_STATUSCODE_WOULD_BLOCK;
    }
}

JSR177_STATUSCODE jsr177_reset_finish(jbyte *atr, jsize *atr_size, void *context) {
    int tmp_atr_size;
    javacall_result result = javacall_carddevice_reset_finish(atr, &tmp_atr_size, context);
    switch (result) {
    case JAVACALL_OK:
        *atr_size = tmp_atr_size;
        return JSR177_STATUSCODE_OK;
    case JAVACALL_FAIL:
    default:
        return JSR177_STATUSCODE_FAIL;
    case JSR177_STATUSCODE_WOULD_BLOCK:
        return JSR177_STATUSCODE_WOULD_BLOCK;
    }
}

/** 
 * Performs platform lock of the device. This is intended to make
 * sure that no other native application
 * uses the same device during a transaction.
 * @return JSR177_STATUSCODE_OK if all done successfuly, 
           JSR177_STATUSCODE_WOULD_BLOCK if the device is locked by the other
 *         JSR177_STATUSCODE_FAIL if error occured
 */
JSR177_STATUSCODE jsr177_lock() {
    javacall_result result = javacall_carddevice_lock();
    switch (result) {
    case JAVACALL_OK:
        return JSR177_STATUSCODE_OK;
    case JAVACALL_FAIL:
    default:
        return JSR177_STATUSCODE_FAIL;
    case JSR177_STATUSCODE_WOULD_BLOCK:
        return JSR177_STATUSCODE_WOULD_BLOCK;
    }
}

/** 
 * Unlocks the device.
 * @return JSR177_STATUSCODE_OK is if all done successfuly, JSR177_STATUSCODE_FAIL otherwise
 */
JSR177_STATUSCODE jsr177_unlock() {
    javacall_result result = javacall_carddevice_unlock();
    switch (result) {
    case JAVACALL_OK:
        return JSR177_STATUSCODE_OK;
    case JAVACALL_FAIL:
    default:
        return JSR177_STATUSCODE_FAIL;
    }
}

/** 
 * Retrieves current slot's card movement events from driver.
 * Events is retrieved as bit mask. It can include
 * all movements from last reading, but can contain only the last.
 * Enum JSR177_CARD_MOVEMENT should be used to specify type of movement.
 * Clears the slot event state.
 * @param mask Movements retrived.
 * @return JSR177_STATUSCODE_OK if all done successfuly, JSR177_STATUSCODE_FAIL otherwise.
 */
JSR177_STATUSCODE jsr177_card_movement_events(JSR177_CARD_MOVEMENT *mask) {
    JAVACALL_CARD_MOVEMENT tmp_mask;
    javacall_result result = javacall_carddevice_card_movement_events(&tmp_mask);
    switch (result) {
    case JAVACALL_OK:
        *mask = tmp_mask;
        return JSR177_STATUSCODE_OK;
    case JAVACALL_FAIL:
    default:
        return JSR177_STATUSCODE_FAIL;
    }
}

/** 
 * Transfers APDU data to the device and receives response from the device.
 * @param tx_buffer Buffer with APDU to be sent.
 * @param tx_size Size of APDU.
 * @param rx_buffer Buffer to store the response.
 * @param rx_size Before call: size of <tt>rx_buffer</tt>
 *                 After call: size of received response.
 * @return JSR177_STATUSCODE_OK is if all done successfuly, JSR177_STATUSCODE_FAIL otherwise
 */
JSR177_STATUSCODE jsr177_xfer_data_start(jbyte *tx_buffer, jsize tx_size,
    jbyte *rx_buffer, jsize *rx_size, void **context) {
    int tmp_rx_size = *rx_size;
    javacall_result result = 
        javacall_carddevice_xfer_data_start(tx_buffer, tx_size,
                                            rx_buffer, &tmp_rx_size, context);
    switch (result) {
    case JAVACALL_OK:
        *rx_size = tmp_rx_size;
        return JSR177_STATUSCODE_OK;
    case JAVACALL_FAIL:
    default:
        return JSR177_STATUSCODE_FAIL;
    case JSR177_STATUSCODE_WOULD_BLOCK:
        return JSR177_STATUSCODE_WOULD_BLOCK;
    }
}

JSR177_STATUSCODE jsr177_xfer_data_finish(jbyte *tx_buffer, jsize tx_size,
    jbyte *rx_buffer, jsize *rx_size, void *context) {
    int tmp_rx_size = *rx_size;
    javacall_result result = 
        javacall_carddevice_xfer_data_finish(tx_buffer, tx_size,
                                             rx_buffer, &tmp_rx_size,
                                             context);
    switch (result) {
    case JAVACALL_OK:
        *rx_size = tmp_rx_size;
        return JSR177_STATUSCODE_OK;
    case JAVACALL_FAIL:
    default:
        return JSR177_STATUSCODE_FAIL;
    case JSR177_STATUSCODE_WOULD_BLOCK:
        return JSR177_STATUSCODE_WOULD_BLOCK;
    }
}

