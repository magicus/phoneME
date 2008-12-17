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

#include <javacall_serial.h>
#include <serial_port_export.h>
#include <midpServices.h>
#include <midp_thread.h>
#include <kni.h>
#include <sni.h>

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
int
openPortByNameStart(char* pszDeviceName, int baudRate,
					int options, int *pHandle, void **pContext)
{
    javacall_handle hPort = (javacall_handle)-1;
    javacall_result ret;
	int returnStatus = PCSL_NET_IOERROR;

    /* Javacall implementation never uses contect */
    (void)pContext;
    
	ret = javacall_serial_open_start(pszDeviceName, baudRate, options, &hPort);
    *pHandle = (int)hPort;

	switch (ret) {
		case JAVACALL_OK:
			returnStatus = PCSL_NET_SUCCESS;
			break;
		case JAVACALL_WOULD_BLOCK:
			returnStatus = PCSL_NET_WOULDBLOCK;
			break;
        default: break;
	}

	return returnStatus;
}

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
    int options, int *pHandle, void *context) {

	javacall_handle hPort = (javacall_handle)*pHandle;
    javacall_result ret;
	int returnStatus = PCSL_NET_IOERROR;

    /* Javacall implementation never uses contect */
    (void)context;
    (void)pszDeviceName;
    (void)baudRate;
    (void)options;

    ret = javacall_serial_open_finish(hPort);

	switch (ret) {
		case JAVACALL_OK:
			returnStatus = PCSL_NET_SUCCESS;
			break;
		case JAVACALL_WOULD_BLOCK:
			returnStatus = PCSL_NET_WOULDBLOCK;
			break;
        default: break;
	}

	return returnStatus;
}

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
int configurePort(int hPort, int baudRate, unsigned int options) {
    int ret_v = PCSL_NET_IOERROR;
    if (JAVACALL_OK == 
	    javacall_serial_configure((javacall_handle)hPort, baudRate, options)) {
        ret_v = PCSL_NET_SUCCESS;
    }
	return ret_v;
}

/**
 * Close a serial port.
 *
 * @param hPort handle to a native serial port
 */

void closePort(int hPort)
{
    javacall_result ret;
    void* context = NULL;
    MidpReentryData* info = (MidpReentryData*)SNI_GetReentryData(NULL);

    if (info == NULL) { //first invocation    
        ret = javacall_serial_close_start((javacall_handle)hPort);
    } else { /* Reinvocation */
        hPort = info->descriptor;
        context = info->pResult;
        ret = javacall_serial_close_finish((javacall_handle)hPort);
    }

    if (JAVACALL_WOULD_BLOCK == ret) {
        midp_thread_wait(COMM_CLOSE_SIGNAL, hPort, context);
    }
}

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
    int nNumberOfBytesToWrite, int* pBytesWritten, void **pContext) {
    int status = PCSL_NET_IOERROR;

    /* Javacall implementation never returns PCSL_NET_WOULDBLOCK */
    (void)pContext;

    if (nNumberOfBytesToWrite == 0) {
        *pBytesWritten = 0;
        status = PCSL_NET_SUCCESS;
    } else {
        javacall_result ret;
        ret = javacall_serial_write_start((javacall_handle) hPort, 
           (unsigned char*) pBuffer, nNumberOfBytesToWrite, pBytesWritten);

		switch (ret) {
			case JAVACALL_OK:
				status = PCSL_NET_SUCCESS;
				break;
			case JAVACALL_WOULD_BLOCK:
				status = PCSL_NET_WOULDBLOCK;
				break;
            default: break;
		}
	}
    return status;
}

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
        int nNumberOfBytesToWrite, int* pBytesWritten, void *context) {
    int status = PCSL_NET_IOERROR;
    (void)context;

    if (nNumberOfBytesToWrite == 0) {
        *pBytesWritten = 0;
        status = PCSL_NET_SUCCESS;
    } else {
        javacall_result ret;
        ret = javacall_serial_write_finish((javacall_handle) hPort, 
           (unsigned char*) pBuffer, nNumberOfBytesToWrite, pBytesWritten);

		switch (ret) {
			case JAVACALL_OK:
				status = PCSL_NET_SUCCESS;
				break;
			case JAVACALL_WOULD_BLOCK:
				status = PCSL_NET_WOULDBLOCK;
				break;
            default: break;
		}
	}
    return status;
}

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
                 int nNumberOfBytesToRead, int* pBytesRead, void **pContext) {
    int status = PCSL_NET_IOERROR;

    /* Linux implementation never returns PCSL_NET_WOULDBLOCK */
    (void)pContext;

    if (nNumberOfBytesToRead == 0) {
        *pBytesRead = 0;
        status = PCSL_NET_SUCCESS;
    } else {

	    javacall_result ret;

		ret = javacall_serial_read_start((javacall_handle) hPort,
               (unsigned char*) pBuffer, 
               nNumberOfBytesToRead, pBytesRead);     	  

		switch (ret) {
			case JAVACALL_OK:
				status = PCSL_NET_SUCCESS;
				break;
			case JAVACALL_WOULD_BLOCK:
				status = PCSL_NET_WOULDBLOCK;
				break;
            default: break;
		}
	}
    return status;
}

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
    int nNumberOfBytesToRead, int* pBytesRead, void *context) {
    int status = PCSL_NET_IOERROR;
    (void)context;

    if (nNumberOfBytesToRead == 0) {
        *pBytesRead = 0;
        status = PCSL_NET_SUCCESS;
    } else {

	    javacall_result ret;

		ret = javacall_serial_read_finish((javacall_handle) hPort,
               (unsigned char*) pBuffer, 
               nNumberOfBytesToRead, pBytesRead);     	  

		switch (ret) {
			case JAVACALL_OK:
				status = PCSL_NET_SUCCESS;
				break;
			case JAVACALL_WOULD_BLOCK:
				status = PCSL_NET_WOULDBLOCK;
				break;
            default: break;
		}
	}
    return status;
}
