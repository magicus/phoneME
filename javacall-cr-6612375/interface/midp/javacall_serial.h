/*
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

#ifndef __JAVACALL_SERIAL_H_
#define __JAVACALL_SERIAL_H_

/**
 * @file javacall_serial.h
 * @ingroup Serial
 * @brief Javacall interfaces for serial port
 */

#include "javacall_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup Serial Serial Port API
 * @ingroup JTWI
 *
 * NOTE: The following functions are optional.
 *
 * Optional API to support serial COM communication.
 *
 * @{
 */
       
/******************************************************************************
 ******************************************************************************
 ******************************************************************************
    OPTIONAL FUNCTIONS
 ******************************************************************************
 ******************************************************************************
 ******************************************************************************/
    
/**
 * @defgroup OptionalSerial Optional Serial
 * @ingroup Serial
 * @{
 */



/* COMM options */
#define JAVACALL_SERIAL_STOP_BITS_2     0x01
#define JAVACALL_SERIAL_ODD_PARITY      0x02
#define JAVACALL_SERIAL_EVEN_PARITY     0x04
#define JAVACALL_SERIAL_AUTO_RTS        0x10
#define JAVACALL_SERIAL_AUTO_CTS        0x20
#define JAVACALL_SERIAL_BITS_PER_CHAR_7 0x80
#define JAVACALL_SERIAL_BITS_PER_CHAR_8 0xC0

/*Unspecified baud rate*/
#define JAVACALL_UNSPECIFIED_BAUD_RATE  -1
/**
 * Return an string the contains a list of available ports delimited by a comma
 * (COM1,COM2)
 * If there is no available port then buffer will be empty string and return JAVACALL OK.
 *
 * @param buffer lists of available ports.This value must be null terminated.
 * @param maxBufferLen the maximum length of buffer
 * @retval JAVACALL_OK success
 * @retval JAVACALL_FAIL fail or the return length is more than maxBufferLen characters.
 */
javacall_result /*OPTIONAL*/  javacall_serial_list_available_ports(char* buffer, int maxBufLen);


/**
 * Initiates opening serial link according to the given parameters.
 *
 * @param devName the name of the port / device to be opened ("COM1")
 * @param baudRate the baud rate for the open connection. in case
 *        bauseRate=JAVACALL_UNSPECIFIED_BAUD_RATE
 *        the baudRate that was specified by the platform should be used
 * @param options the serial link option (JAVACALL_SERIAL_XXX)
 * @param pHandle the handle of the port to be opend
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        if there was an error   
 * @retval JAVACALL_WOULD_BLOCK  if the caller must call the finish function again to complete the operation
 */
javacall_result  /*OPTIONAL*/ javacall_serial_open_start(const char *devName, int baudRate, unsigned int options, /*OUT*/javacall_handle *pHandle);

/**
 * Finishes opening serial link according to the given parameters
 *
 * @param handle the handle of the port to be opend
 * @retval JAVACALL_OK on success, 
 * @retval JAVACALL_FAIL on error
 * @retval JAVACALL_WOULD_BLOCK  if the caller must call the finish function again to complete the operation
 */
javacall_result /*OPTIONAL*/ javacall_serial_open_finish(javacall_handle handle);

/**
 * Update the baudRate of an open serial port
 *
 * @param hPort the port to configure
 * @param baudRate the new baud rate for the open connection
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt> on error
 */
javacall_result /*OPTIONAL*/ javacall_serial_set_baudRate(javacall_handle pHandle, int baudRate);

/**
 * Retrive the current baudRate of the open serial port
 *
 * @param hPort the port to configure
 * @param baudRate pointer to where to return the baudRate
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt> on error
 */
javacall_result /*OPTIONAL*/ javacall_serial_get_baudRate(javacall_handle hPort, /*OUT*/ int *baudRate);

/**
 * Configure serial port
 *
 * @param hPort the port to configure
 * @param baudRate the new baud rate for the open connection
 * @param options options for the serial port:
 * bit 0: 0 - 1 stop bit, 1 - 2 stop bits 
 * bit 2-1: 00 - no parity, 01 - odd parity, 10 - even parity 
 * bit 4: 0 - no auto RTS, 1 - set auto RTS 
 * bit 5: 0 - no auto CTS, 1 - set auto CTS 
 * bit 7-6: 01 - 7 bits per symbol, 11 - 8 bits per symbol 
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt> on error
 */
javacall_result /*OPTIONAL*/ javacall_serial_configure(javacall_handle pHandle, int baudRate, int options);

/**
 * Initiates closing serial link.
 *
 * @param hPort the port to close
 * after this call, java is guaranteed not to call javacall_serial_read() or 
 * javacall_serial_write() before issuing another javacall_serial_open( ) call.
 *
 * @retval <tt>JAVACALL_OK</tt> on success, 
 * @retval <tt>JAVACALL_FAIL</tt>
 * @retval JAVACALL_WOULD_BLOCK  if the caller must call the finish function again to complete the operation
 */
javacall_result /*OPTIONAL*/ javacall_serial_close_start(javacall_handle hPort);

/**
 * Finishes closing serial link.
 *
 * @param hPort the port to close
 * @retval <tt>JAVACALL_OK</tt> on success, 
 * @retval <tt>JAVACALL_FAIL</tt>
 * @retval JAVACALL_WOULD_BLOCK  if the caller must call the finish function again to complete the operation
 */
javacall_result /*OPTIONAL*/ javacall_serial_close_finish(javacall_handle hPort);

/**
 * Initiates reading a specified number of bytes from serial link, 
 
 * @param hPort the port to read the data from
 * @param buffer to which data is read
 * @param size number of bytes to be read. Actual number of bytes
 *              read may be less, if less data is available
 * @param byteRead actual number the were read from the port.
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        if there was an error   
 * @retval JAVACALL_WOULD_BLOCK  if the caller must call the finish function again to complete the operation
 */
javacall_result /*OPTIONAL*/ javacall_serial_read_start(javacall_handle hPort, unsigned char* buffer, int size ,/*OUT*/int *bytesRead);

/**
 * Finishes reading a specified number of bytes from serial link, 
 *
 * @param hPort the port to read the data from
 * @param buffer to which data is read
 * @param size number of bytes to be read. Actual number of bytes
 *              read may be less, if less data is available
 * @param byteRead actual number the were read from the port.
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        if there was an error   
 * @retval JAVACALL_WOULD_BLOCK  if the caller must call the finish function again to complete the operation
 */
javacall_result /*OPTIONAL*/ javacall_serial_read_finish(javacall_handle hPort, unsigned char* buffer, int size, int *bytesRead);

/**
 * Initiates writing a specified number of bytes to serial link, 
 *
 * @param hPort the port to write the data to
 * @param buffer buffer to write data from
 * @param size number of bytes to be write.
 * @param bytesWritten the number of bytes actually written.
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        if there was an error   
 * @retval JAVACALL_WOULD_BLOCK  if the caller must call the finish function again to complete the operation
 */
javacall_result /*OPTIONAL*/ javacall_serial_write_start(javacall_handle hPort, unsigned char* buffer, int size, int *bytesWritten);  

/**
 * Finishes writing a specified number of bytes to serial link, 
 *
 * @param hPort the port to write the data to
 * @param buffer buffer to write data from
 * @param size number of bytes to be write.
 * @param bytesWritten the number of bytes actually written.
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        if there was an error   
 * @retval JAVACALL_WOULD_BLOCK  if the caller must call the finish function again to complete the operation
 */
javacall_result /*OPTIONAL*/ javacall_serial_write_finish(javacall_handle hPort, unsigned char* buffer, int size, int *bytesWritten);  


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
 * @defgroup Notification functions 
 * @ingroup Port
 * @{
 */
/**
 * @enum javacall_port_callback_type
 */
typedef enum {    
    JAVACALL_EVENT_SERIAL_RECEIVE = 1000,
    JAVACALL_EVENT_SERIAL_WRITE   = 1001,
    JAVACALL_EVENT_SERIAL_OPEN    = 1002,
    JAVACALL_EVENT_SERIAL_CLOSE   = 1003
} javacall_serial_callback_type;


/**
 * A callback function to be called for notification of comm port 
 * related events.
 * The platform will invoke the call back in platform context.
 *
 * @param type type of indication: 
 *          JAVACALL_EVENT_SERIAL_RECEIVE
 * @param hPort handle of port related to the notification
 * @param operation_result <tt>JAVACALL_OK</tt> if operation 
 *        completed successfully, 
 *        <tt>JAVACALL_FAIL</tt> or negative value on failure
 */
void javanotify_serial_event(
                             javacall_serial_callback_type type, 
                             javacall_handle hPort,
                             javacall_result operation_result);

#ifdef __cplusplus
}
#endif

#endif 

