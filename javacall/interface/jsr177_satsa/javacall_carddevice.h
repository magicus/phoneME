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
#ifndef __JAVACALL_CARDDEVICE_H_
#define __JAVACALL_CARDDEVICE_H_

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
 * ##include <javacall_carddevice.h>
 * @{
 *
 * This file defines the SATSA porting interface to a smart card
 * device driver.
 * The functions defined here are called
 * from the SATSA native card access implementation to handle
 * specific operations for locking device communication
 * and performing data exchange with smart card applets.
 */
#include <stdarg.h>
#include "javacall_defs.h"

/**
 * @defgroup jsrMandatorySatsa Mandatory SATSA API
 * @ingroup carddevice
 * @{
 */

/** 
 * Card movement (insertion and withdrawal) events.
 *
 * If we can not specify type of movement 
 * we set JAVACALL_EVENT_CARD_CHANGED.
 */
typedef enum JAVACALL_CARD_MOVEMENT_ENUM {
    JAVACALL_EVENT_CARD_REMOVED = 0x1,
    JAVACALL_EVENT_CARD_INSERTED = 0x2,
    JAVACALL_EVENT_CARD_CHANGED = 0x4,
    JAVACALL_CARD_MOVEMENT_MASK = 0x0F
} JAVACALL_CARD_MOVEMENT;


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
void javacall_carddevice_clear_error();

/** 
 * Sets error state and stores message (like printf).
 * @param fmt printf-like format string
 */
void javacall_carddevice_set_error(const char *fmt, ...);

/** 
 * Retrieves error message into the provided buffer and clears state.
 * @param buf Buffer to store message
 * @param buf_size Size of the buffer in bytes
 * @return JAVACALL_TRUE if error messages were returned, JAVACALL_FALSE otherwise
 */
javacall_bool javacall_carddevice_get_error(char *buf, javacall_int32 buf_size);

/*
 * JSR177 Card driver API functions
 */

/** 
 * Initializes the driver.
 * @return JAVACALL_OK if all done successfuly, 
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_carddevice_init();

/** 
 * Finalizes the driver.
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_carddevice_finalize();

/** 
 * Sets property value. If the property is used during the initialization
 * process then this method must be called before <code>javacall_carddevice_init()</code>
 * @return JAVACALL_OK if all done successfuly, 
 *         JAVACALL_NOT_IMPLEMENTED when this property is not supported
 *         JAVACALL_OUT_OF_MEMORY if there is no enough memory
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_carddevice_set_property(const char *prop_name, 
    const char *prop_value);

/** 
 * Selects specified slot (if possible).
 * @return JAVACALL_OK if all done successfuly
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_carddevice_select_slot(javacall_int32 slot_index);

/** 
 * Returns number of slots which available for selection.
 * @param slot_cnt Buffer for number of slots.
 * @return JAVACALL_OK if all done successfuly
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_carddevice_get_slot_count(javacall_int32 *slot_cnt);

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
                                                 void **context);

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
                                                 void *context);

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
javacall_result javacall_carddevice_reset_start(char *atr, javacall_int32 *atr_size, 
                                                void **context);

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
javacall_result javacall_carddevice_reset_finish(char *atr, javacall_int32 *atr_size, 
                                                 void *context);
/** 
 * Performs platform lock of the device. This is intended to make
 * sure that no other native application
 * uses the same device during a transaction.
 * @return JAVACALL_OK if all done successfuly, 
 *         JAVACALL_WOULD_BLOCK if the device is locked by the other
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_carddevice_lock();

/** 
 * Unlocks the device.
 * @return JAVACALL_OK if all done successfuly
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_carddevice_unlock();

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
javacall_result javacall_carddevice_card_movement_events(JAVACALL_CARD_MOVEMENT *mask);

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
javacall_result javacall_carddevice_xfer_data_start(char *tx_buffer, javacall_int32 tx_size,
    char *rx_buffer, javacall_int32 *rx_size, void **context);

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
javacall_result javacall_carddevice_xfer_data_finish(char *tx_buffer, javacall_int32 tx_size,
    char *rx_buffer, javacall_int32 *rx_size, void *context);

/** @} */


/******************************************************************************
 ******************************************************************************
 ******************************************************************************

  NOTIFICATION FUNCTIONS
  - - - -  - - - - - - -
  The following functions are implemented by Sun.
  Platform is required to invoke these function for each occurence of the
  undelying event.
  The functions need to be executed in platform's task/thread

 ******************************************************************************
 ******************************************************************************
 ******************************************************************************/

/**
 * @defgroup NotificationCarddevice Notification API for SATSA Card Device Interface
 * @ingroup carddevice
 * @{
 */

/**
 * @enum javacall_carrdevice_event
 * @brief Card Device notification event type
 */
typedef enum {
    /** The RESET command has completed */
    JAVACALL_CARDDEVICE_RESET = 1,
    /** The XFER command has completed */
    JAVACALL_CARDDEVICE_XFER = 2,
    /** The platform lock has been released */
    JAVACALL_CARDDEVICE_UNLOCK = 3
} javacall_carddevice_event;

/**
 * A callback function to be called for notification of carddevice
 * related events.
 * The platform will invoke the call back in platform context.
 * @param event the type of carddevice-related event that occured
 *              JAVACALL_CARDDEVICE_RESET if the RESET command has completed
 *              JAVACALL_CARDDEVICE_XFER if the XFER command has completed
 *              JAVACALL_CARDDEVICE_UNLOCK if the platform lock has been released
 *
 */
void javanotify_carddevice_event(javacall_carddevice_event event, void *context);


/** @} */

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ifndef __JAVACALL_CARDDEVICE_H_ */
