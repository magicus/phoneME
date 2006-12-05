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
#ifndef _CARDDEVICE_H_
#define _CARDDEVICE_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @defgroup satsa JSR177 Security and Trust Services API (SATSA)
 * @ingroup stack
 */

/**
 * @defgroup carddevice SATSA Native Card Driver Interface
 * @ingroup satsa
 * @brief SATSA smart card driver porting interface. \n
 * ##include <carddevice.h>
 * @{
 *
 * This file defines the SATSA porting interface to a smart card
 * device driver.
 * The functions defined here are called
 * from the SATSA native card access implementation to handle
 * specific operations for locking device communication
 * and performing data exchange with smart card applets.
 */
// #ifndef _JAVASOFT_KNI_H_
    #include <java_types.h>
// #endif /* !_JAVASOFT_KNI_H_ */
/** 
 * Porting interface status codes. 
 */
typedef enum JSR177_STATUSCODE_ENUM {
   JSR177_STATUSCODE_OK = 0,                    /* Generic success             */
   JSR177_STATUSCODE_FAIL = -1,                 /* Generic failure             */
   JSR177_STATUSCODE_NOT_IMPLEMENTED = -2,      /* Not implemented             */
   JSR177_STATUSCODE_OUT_OF_MEMORY = -3,        /* Out of memory               */
   JSR177_STATUSCODE_INVALID_ARGUMENT = -4,     /* Invalid argument            */
   JSR177_STATUSCODE_WOULD_BLOCK = -5,          /* Would block                 */
   JSR177_STATUSCODE_CONNECTION_NOT_FOUND = -6, /* Connection not found        */
   JSR177_STATUSCODE_INTERRUPTED = -7           /* Operation is interrupted    */
} JSR177_STATUSCODE;

#define SIGNAL_RESET    0x7781
#define SIGNAL_XFER     0x7782
#define SIGNAL_LOCK     0x7783

/** 
 * Card movement (insertion and withdrawal) events.
 *
 * If we can not specify type of movement 
 * we set JSR177_EVENT_CARD_CHANGED.
 */
typedef enum JSR177_CARD_MOVEMENT_ENUM {
    JSR177_EVENT_CARD_REMOVED = 0x1,
    JSR177_EVENT_CARD_INSERTED = 0x2,
    JSR177_EVENT_CARD_CHANGED = 0x4,
    JSR177_CARD_MOVEMENT_MASK = 0x0F
} JSR177_CARD_MOVEMENT;

/* 
 * Error handling functions.
 *
 * Every part of the driver can send a text message into
 * that system if it supposes that a trouble occured. 
 * In that case an error state will set.
 * After that somebody can retrive all (if be allowed by
 * memory limitations) messages concatenated to one
 * string. The error state will clear after that.
 */

/** 
 * Clears error state.
 */
void jsr177_clear_error();

/** 
 * Sets error state and stores message (like printf).
 * @param fmt printf-like format string
 */
void jsr177_set_error(const char *fmt, ...);

/** 
 * Retrieves error message into the provided buffer and clears state.
 * @param buf Buffer to store message
 * @param buf_size Size of the buffer in bytes
 * @return PCSL_TRUE if error messages were returned, PCSL_FALSE otherwise
 */
jboolean jsr177_get_error(jbyte *buf, jsize buf_size);

/*
 * JSR177 Card driver API functions
 */

/** 
 * Initializes the driver.
 * @return JSR177_STATUSCODE_OK if all done successfuly, 
 *         JSR177_STATUSCODE_NOT_IMPLEMENTED when the stub was called
 *         JSR177_STATUSCODE_FAIL otherwise
 */
JSR177_STATUSCODE jsr177_init();

/** 
 * Finalizes the driver.
 * @return JSR177_STATUSCODE_OK if all done successfuly, JSR177_STATUSCODE_FAIL otherwise
 */
JSR177_STATUSCODE jsr177_finalize();

/** 
 * Sets property value. If the property is used during the initialization
 * process then this method must be called before <code>jsr177_init()</code>
 * @return JSR177_STATUSCODE_OK if all done successfuly, 
 *         JSR177_STATUSCODE_NOT_IMPLEMENTED when this property is not supported
 *         JSR177_STATUSCODE_OUT_OF_MEMORY if there is no enough memory
 *         JSR177_STATUSCODE_FAIL otherwise
 */
JSR177_STATUSCODE jsr177_set_property(const jbyte *prop_name, 
    const jbyte *prop_value);

/** 
 * Selects specified slot (if possible).
 * @return JSR177_STATUSCODE_OK if all done successfuly, JSR177_STATUSCODE_FAIL otherwise
 */
JSR177_STATUSCODE jsr177_select_slot(jint slot_index);

/** 
 * Returns number of slots which available for selection.
 * @param slot_cnt Buffer for number of slots.
 * @return JSR177_STATUSCODE_OK if all done successfuly, JSR177_STATUSCODE_FAIL otherwise
 */
JSR177_STATUSCODE jsr177_get_slot_count(jint *slot_cnt);

/** 
 * Checks if this slot is SAT slot.
 * @param slot Slot number.
 * @param result <code>PCSL_TRUE</code> if the slot is dedicated for SAT,
 *               <code>PCSL_FALSE</code> otherwise
 * @return JSR177_STATUSCODE_OK if all done successfuly, JSR177_STATUSCODE_FAIL otherwise
 */
JSR177_STATUSCODE jsr177_is_sat(jint slot, jboolean *result);

/** 
 * Sends 'RESET' command to device and gets ATR into specified buffer.
 * @param atr Buffer to store ATR.
 * @param atr_size Before call: size of provided buffer
 *                 After call: size of received ATR.
 * @param context a context which can keep information between 
 *                <code>jsr177_reset_start</code> and <code>jsr177_reset_finish</code> calls
 * @retval JSR177_STATUSCODE_OK if all done successfuly. 
 * @retval JSR177_STATUSCODE_WOULD_BLOCK if this call has started asynchronous 
 *         operation. In this case <code>jsr177_reset_finish</code> must be used
 *         for finishing this operation.
 * @retval JSR177_STATUSCODE_FAIL otherwise
 */
JSR177_STATUSCODE jsr177_reset_start(jbyte *atr, jsize *atr_size, void **context);

/** 
 * Finishes 'RESET' command on device and gets ATR into specified buffer.
 * @param atr Buffer to store ATR.
 * @param atr_size Before call: size of provided buffer
 *                 After call: size of received ATR.
 * @param context a context which can carry information between 
 *                <code>jsr177_reset_start</code> and <code>jsr177_reset_finish</code> calls
 * @retval JSR177_STATUSCODE_OK if all done successfuly. 
 * @retval JSR177_STATUSCODE_WOULD_BLOCK if the asynchronous operation has not 
 *         completed yet. In this case a new call of 
 *         <code>jsr177_reset_finish</code> must be used for finishing 
 *         this operation.
 * @retval JSR177_STATUSCODE_FAIL otherwise
 */
JSR177_STATUSCODE jsr177_reset_finish(jbyte *atr, jsize *atr_size, void *context);

/** 
 * Sends 'POWER DOWN' command to device.
 * @return JSR177_STATUSCODE_OK if all done successfuly, JSR177_STATUSCODE_FAIL otherwise
 */
JSR177_STATUSCODE jsr177_power_down();

/** 
 * Performs platform lock of the device. This is intended to make
 * sure that no other native application
 * uses the same device during a transaction.
 * @return JSR177_STATUSCODE_OK if all done successfuly, 
           JSR177_STATUSCODE_WOULD_BLOCK if the device is locked by the other
 *         JSR177_STATUSCODE_FAIL if error occured
 */
JSR177_STATUSCODE jsr177_lock();

/** 
 * Unlocks the device.
 * @return JSR177_STATUSCODE_OK if all done successfuly, JSR177_STATUSCODE_FAIL otherwise
 */
JSR177_STATUSCODE jsr177_unlock();

/** 
 * Retrieves current slot's card movement events from driver.
 * Events is retrieved as bit mask. It can include
 * all movements from last reading, but can contain only the last.
 * Enum JSR177_CARD_MOVEMENT should be used to specify type of movement.
 * Clears the slot event state.
 * @param mask Movements retrived.
 * @return JSR177_STATUSCODE_OK if all done successfuly, JSR177_STATUSCODE_FAIL otherwise.
 */
JSR177_STATUSCODE jsr177_card_movement_events(JSR177_CARD_MOVEMENT *mask);

/** 
 * Transfers APDU data to the device and receives response from the device.
 * @param tx_buffer Buffer with APDU to be sent.
 * @param tx_size Size of APDU.
 * @param rx_buffer Buffer to store the response.
 * @param rx_size Before call: size of <tt>rx_buffer</tt>
 *                 After call: size of received response.
 * @param context a context which can carry information between 
 *                <code>jsr177_xfer_data_start</code> and <code>jsr177_xfer_data_finish</code> calls
 * @retval JSR177_STATUSCODE_OK if all done successfuly. 
 * @retval JSR177_STATUSCODE_WOULD_BLOCK if this call has started asynchronous 
 *         operation. In this case <code>jsr177_xfer_data_finish</code> must be used
 *         for finishing this operation.
 * @retval JSR177_STATUSCODE_FAIL otherwise
 */
JSR177_STATUSCODE jsr177_xfer_data_start(jbyte *tx_buffer, jsize tx_size,
    jbyte *rx_buffer, jsize *rx_size, void **context);

/** 
 * Finishes APDU data transfer to the device and receiving the response from the device.
 * @param tx_buffer Buffer with APDU to be sent.
 * @param tx_size Size of APDU.
 * @param rx_buffer Buffer to store the response.
 * @param rx_size Before call: size of <tt>rx_buffer</tt>
 *                 After call: size of received response.
 * @param context a context which can keep information between 
 *                <code>jsr177_xfer_data_start</code> and <code>jsr177_xfer_data_finish</code> calls
 * @retval JSR177_STATUSCODE_OK if all done successfuly. 
 * @retval JSR177_STATUSCODE_WOULD_BLOCK if the asynchronous operation has not 
 *         completed yet. In this case a new call of 
 *         <code>jsr177_xfer_data_finish</code> must be used for finishing 
 *         this operation.
 * @retval JSR177_STATUSCODE_FAIL otherwise
 */
JSR177_STATUSCODE jsr177_xfer_data_finish(jbyte *tx_buffer, jsize tx_size,
    jbyte *rx_buffer, jsize *rx_size, void *context);


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ifndef _CARDDEVICE_H_ */
