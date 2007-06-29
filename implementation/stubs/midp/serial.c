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

/**
* @file
*
*
*/

#include "javacall_serial.h"

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
javacall_result
javacall_serial_list_available_ports(char* buffer, int maxBufLen)
{
  return JAVACALL_FAIL;
}

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
javacall_result
javacall_serial_open_start(const char *devName, int baudRate, unsigned int options, javacall_handle *pHandle)
{
    return JAVACALL_FAIL;
}

/**
 * Finishes opening serial link according to the given parameters
 *
 * @param handle the handle of the port to be opend
 * @return JAVACALL_OK on success, 
 *         JAVACALL_FAIL on error
 */
javacall_result
javacall_serial_open_finish(javacall_handle handle)
{
    return JAVACALL_FAIL;
}

/**
 * Update the baudRate of an open serial port
 *
 * @param hPort the port to configure
 * @param baudRate the new baud rate for the open connection
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt> on error
 */
javacall_result
javacall_serial_set_baudRate(javacall_handle hPort, int baudRate)
{
    return JAVACALL_FAIL;
}

/**
 * Retrive the current baudRate of the open serial port
 *
 * @param hPort the port to configure
 * @param baudRate pointer to where to return the baudRate
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt> on error
 */
javacall_result
javacall_serial_get_baudRate(javacall_handle hPort, int *baudRate)
{
    return JAVACALL_FAIL;
}

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
javacall_result /*OPTIONAL*/ javacall_serial_configure(javacall_handle pHandle, int baudRate, int options)
{
    return JAVACALL_FAIL;
}

/**
 * Initiates closing serial link.
 *
 * @param hPort the port to close
 * after this call, java is guaranteed not to call javacall_serial_read() or 
 * javacall_serial_write() before issuing another javacall_serial_open( ) call.
 *
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt>
 * @retval JAVACALL_WOULD_BLOCK  if the caller must call the finish function again to complete the operation
 */
javacall_result javacall_serial_close_start(javacall_handle hPort) {
    return JAVACALL_FAIL;
}

/**
 * Finishes closing serial link.
 *
 * @param hPort the port to close
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt>
 */
javacall_result
javacall_serial_close_finish(javacall_handle hPort)
{
    return JAVACALL_FAIL;
}

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
javacall_result
javacall_serial_read_start(javacall_handle hPort, unsigned char* buffer, int size, int *byteRead)
{
    return JAVACALL_FAIL;
}

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
 */
javacall_result
javacall_serial_read_finish(javacall_handle hPort, unsigned char* buffer, int size, int *byteRead)
{
    return JAVACALL_FAIL;
}

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
javacall_result
javacall_serial_write_start(javacall_handle hPort, unsigned char* buffer, int size, int *bytesWritten)
{
    return JAVACALL_FAIL;
}

/**
 * Finishes writing a specified number of bytes to serial link, 
 *
 * @param hPort the port to write the data to
 * @param buffer buffer to write data from
 * @param size number of bytes to be write.
 * @param bytesWritten the number of bytes actually written.
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        if there was an error   
 */
javacall_result
javacall_serial_write_finish(javacall_handle hPort, unsigned char* buffer, int size, int *bytesWritten)
{
    return JAVACALL_FAIL;
}
