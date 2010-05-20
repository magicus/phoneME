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
 * FILE:      serial_port_impl.c
 * OVERVIEW:  Stubs implementation of serial communication ports
 *=======================================================================*/

/*=======================================================================
 * Include files
 *=======================================================================*/

#include <serial_port_impl.h>

/*=========================================================================
 * FUNCTION:        freePortError
 * OVERVIEW:        
 * INTERFACE:
 *   parameters:    *pszError: Error message
 *   returns:       <nothing>
 *=======================================================================*/

void freePortError(char* pszError) {
    (void)pszError;
    REPORT_WARN(LC_PROTOCOL, "serial_port:freePortError: Stubbed out.");
}

/*=========================================================================
 * FUNCTION:        openPortByNumber
 * OVERVIEW:        Opens a serial port from the specified port number.
 * INTERFACE:
 *   parameters:    **ppszError: Error message
 *                  port:        Port number
 *                  baudRate:    The speed of the connection.
 *                  options:     Options to be used for the port.
 *   returns:       The open port 
 *=======================================================================*/

long openPortByNumber(char** ppszError, long port, long baudRate,
                      long options) {
    (void)ppszError;
    (void)port;
    (void)baudRate;
    (void)options;
    REPORT_WARN(LC_PROTOCOL, "serial_port:openPortByNumber: Stubbed out.");
}

/*=========================================================================
 * FUNCTION:        openPortByName
 * OVERVIEW:        Opens a serial port from a specified name ie "/dev/term/a".
 * INTERFACE:
 *   parameters:    **ppszError:    Error message
 *                  *pszDeviceName: Port number
 *                  baudRate:       The speed of the connection.
 *                  options:        Options to be used for the port.
 *   returns:       The open port
 *=======================================================================*/

long openPortByName(char** ppszError, char* pszDeviceName, long baudRate,
                    long  options) {
    (void)ppszError;
    (void)pszDeviceName;
    (void)baudRate;
    (void)options;
    REPORT_WARN(LC_PROTOCOL, "serial_port:openPortByName: Stubbed out.");
    return 0;
}

/*=========================================================================
 * FUNCTION:        configurePort
 * OVERVIEW:        Configures the open serial port.
 * INTERFACE:
 *   parameters:    **ppszError:   Error message
 *                  hPort:         The port to be configured.
 *                  baudRate:      The speed of the connection.
 *                  options:       Options to be used for the port.
 *   returns:       <nothing>
 *=======================================================================*/

void configurePort(char** ppszError, int hPort, long baudRate,
                          unsigned long options) {
    (void)ppszError;
    (void)hPort;
    (void)baudRate;
    (void)options;
    REPORT_WARN(LC_PROTOCOL, "serial_port:configurePort: Stubbed out.");
}

/*=========================================================================
 * FUNCTION:        closePort
 * OVERVIEW:        Closes the open serial port.
 * INTERFACE:
 *   parameters:    hPort:  The open port.
 *   returns:       <nothing>
 *=======================================================================*/

void closePort(long hPort) {
    (void)hPort;
    REPORT_WARN(LC_PROTOCOL, "serial_port:closePort: Stubbed out.");
}

/*=========================================================================
 * FUNCTION:        writeToPort
 * OVERVIEW:        Writes to the open port, without blocking
 * INTERFACE:
 *   parameters:    **ppszError:            Error message
 *                  hPort:                  The name of the port to write to.
 *                  *pBuffer:               The data to be written to the port.
 *                  nNumberOfBytesToWrite:  The number of bytes to write.
 *   returns:       The number of bytes written.
 *=======================================================================*/

long writeToPort(char** ppszError, long hPort, char* pBuffer,
                 long nNumberOfBytesToWrite) {
    (void)ppszError;
    (void)hPort;
    (void)pBuffer;
    (void)nNumberOfBytesToWrite;
    REPORT_WARN(LC_PROTOCOL, "serial_port:writeToPort: Stubbed out.");
}

/*=========================================================================
 * FUNCTION:        readFromPort
 * OVERVIEW:        reads from a serial port, without blocking
 * INTERFACE:
 *   parameters:    **ppszError:          Error message
 *                  hPort:                The name of the port to read from.
 *                  pBuffer:              The buffer to store the bytes read.
 *                  nNumberOfBytesToRead: The number of bytes to read.
 *   returns:       The number of bytes read.
 *=======================================================================*/

long readFromPort(char** ppszError, long hPort, char* pBuffer,
                 long nNumberOfBytesToRead) {
    (void)ppszError;
    (void)hPort;
    (void)pBuffer;
    (void)nNumberOfBytesToRead;
    REPORT_WARN(LC_PROTOCOL, "serial_port:readFromPort: Stubbed out.");
}
