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

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: networking
 * FILE:      serial_port_export.h
 * OVERVIEW:  Prototypes for supporting serial communication ports.
 *            If a target platform wants to support serial ports,
 *            a platform-specific implementation of the functions
 *            must be provided in the Vm<Port>/src directory,
 *            where <Port> is the name of your target platform
 *            (e.g., Win, Unix, Pilot).
 *=======================================================================*/

#ifndef _SERIAL_PORT_EXPORT_H_
#define _SERIAL_PORT_EXPORT_H_

#include <pcsl_network.h>
#include <midpError.h>
#include <midp_libc_ext.h>
#include <kni_globals.h>
#define MAX_NAME_LEN 80

/* COMM options */
#define STOP_BITS_2     0x01
#define ODD_PARITY      0x02
#define EVEN_PARITY     0x04
#define AUTO_RTS        0x10
#define AUTO_CTS        0x20
#define BITS_PER_CHAR_7 0x80
#define BITS_PER_CHAR_8 0xC0

/*=========================================================================
 * Serial protocol prototypes (each platform/port must supply definitions
 * of these prototypes)    
 *=======================================================================*/

/**
 * Open a serial port by logical device name.
 *
 * @param pszDeviceName logical name of the port (for example, COM1)
 * @param baudRate baud rate to set the port at
 * @param options options for the serial port
 * bit 0: 0 - 1 stop bit, 1 - 2 stop bits 
 * bit 2-1: 00 - no parity, 01 - odd parity, 10 - even parity 
 * bit 4: 0 - no auto RTS, 1 - set auto RTS 
 * bit 5: 0 - no auto CTS, 1 - set auto CTS 
 * bit 7-6: 01 - 7 bits per symbol, 11 - 8 bits per symbol 
 * @param pHandle returns the connection handle; it is
 *        set only when this function returns PCSL_NET_SUCCESS
 * @param pContext filled by ptr to data for reinvocations
 *
 * @return PCSL_NET_SUCCESS for successful read operation;\n 
 *       PCSL_NET_WOULDBLOCK if the operation would block,\n 
 *       PCSL_NET_INTERRUPTED for an Interrupted IO Exception,\n
 *       PCSL_NET_IOERROR for all other errors
 */
int openPortByNameStart(char* pszDeviceName, int baudRate,
    int options, int *pHandle, void **pContext);

/**
 * Open a serial port by system dependent device name.
 *
 * @param pszDeviceName device name of the port
 * @param baudRate baud rate to set the port at
 * @param options options for the serial port
 * bit 0: 0 - 1 stop bit, 1 - 2 stop bits 
 * bit 2-1: 00 - no parity, 01 - odd parity, 10 - even parity 
 * bit 4: 0 - no auto RTS, 1 - set auto RTS 
 * bit 5: 0 - no auto CTS, 1 - set auto CTS 
 * bit 7-6: 01 - 7 bits per symbol, 11 - 8 bits per symbol 
 * @param pHandle returns the connection handle; it is
 *        set only when this function returns PCSL_NET_SUCCESS
 * @param context ptr to data saved before sleeping
 *
 * @return PCSL_NET_SUCCESS for successful read operation;\n 
 *       PCSL_NET_WOULDBLOCK if the operation would block,\n 
 *       PCSL_NET_INTERRUPTED for an Interrupted IO Exception,\n
 *       PCSL_NET_IOERROR for all other errors
 */
int openPortByNameFinish(char* pszDeviceName, int baudRate,
    int options, int *pHandle, void *context);

/**
 * Configure a serial port optional parameters.
 *
 * @param hPort port number
 * @param baudRate baudRate rate to set the port at
 * @param options options for the serial port:
 * bit 0: 0 - 1 stop bit, 1 - 2 stop bits 
 * bit 2-1: 00 - no parity, 01 - odd parity, 10 - even parity 
 * bit 4: 0 - no auto RTS, 1 - set auto RTS 
 * bit 5: 0 - no auto CTS, 1 - set auto CTS 
 * bit 7-6: 01 - 7 bits per symbol, 11 - 8 bits per symbol 
 *
 * @return PCSL_NET_SUCCESS for successful configure operation;\n 
 *       PCSL_NET_IOERROR for any error
 */
int configurePort(int hPort, int baudRate, unsigned int options);

/**
 * Read from a serial port.
 *
 * @param hPort handle to a native serial port
 * @param pBuffer I/O buffer
 * @param nNumberOfBytesToRead length of data
 * @param pBytesRead returns the number of bytes actually read; it is
 *        set only when this function returns PCSL_NET_SUCCESS
 * @param pContext filled by ptr to data for reinvocations
 *
 * @return PCSL_NET_SUCCESS for successful read operation;\n 
 *       PCSL_NET_WOULDBLOCK if the operation would block,\n 
 *       PCSL_NET_INTERRUPTED for an Interrupted IO Exception,\n
 *       PCSL_NET_IOERROR for all other errors
 */
int readFromPortStart(int hPort, char* pBuffer, 
    int nNumberOfBytesToRead, int* pBytesRead, void **pContext);

/**
 * Read from a serial port.
 *
 * @param hPort handle to a native serial port
 * @param pBuffer I/O buffer
 * @param nNumberOfBytesToRead length of data
 * @param pBytesRead returns the number of bytes actually read; it is
 *        set only when this function returns PCSL_NET_SUCCESS
 * @param context ptr to data saved before sleeping
 *
 * @return PCSL_NET_SUCCESS for successful read operation;\n 
 *       PCSL_NET_WOULDBLOCK if the operation would block,\n 
 *       PCSL_NET_INTERRUPTED for an Interrupted IO Exception,\n
 *       PCSL_NET_IOERROR for all other errors
 */
int readFromPortFinish(int hPort, char* pBuffer, 
    int nNumberOfBytesToRead, int* pBytesRead, void *context);

/**
 * Write to a serial port without blocking.
 *
 * @param hPort handle to a native serial port
 * @param pBuffer I/O buffer
 * @param nNumberOfBytesToWrite length of data
 * @param pBytesWritten returns the number of bytes written after
 *        successful write operation; only set if this function returns
 *        PCSL_NET_SUCCESS
 * @param pContext filled by ptr to data for reinvocations
 *
 * @return PCSL_NET_SUCCESS for successful write operation;\n 
 *       PCSL_NET_WOULDBLOCK if the operation would block,\n 
 *       PCSL_NET_INTERRUPTED for an Interrupted IO Exception\n 
 *       PCSL_NET_IOERROR for all other errors
 */
int writeToPortStart(int hPort, char* pBuffer,
        int nNumberOfBytesToWrite, int* pBytesWritten, void **pContext);

/**
 * Write to a serial port without blocking.
 *
 * @param hPort handle to a native serial port
 * @param pBuffer I/O buffer
 * @param nNumberOfBytesToWrite length of data
 * @param pBytesWritten returns the number of bytes written after
 *        successful write operation; only set if this function returns
 *        PCSL_NET_SUCCESS
 * @param context ptr to data saved before sleeping
 *
 * @return PCSL_NET_SUCCESS for successful write operation;\n 
 *       PCSL_NET_WOULDBLOCK if the operation would block,\n 
 *       PCSL_NET_INTERRUPTED for an Interrupted IO Exception\n 
 *       PCSL_NET_IOERROR for all other errors
 */
int writeToPortFinish(int hPort, char* pBuffer,
        int nNumberOfBytesToWrite, int* pBytesWritten, void *context);

/**
 * Close a serial port.
 *
 * @param hPort handle to a native serial port
 */
void closePort(int hPort);

#endif /* _SERIAL_PORT_EXPORT_H_ */


