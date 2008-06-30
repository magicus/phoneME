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


#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "javacall_serial.h"
#include "javacall_logging.h"


#define BAUD_RATE CBR_115200 //CBR_115200;//CBR_9600;

#define REPORT(msg) javacall_print(msg)

HANDLE fd;


/**
 * Return an string the contains a list of available ports delimited by a comma
 * (COM1,COM2)
 * If there is no available port then buffer will be empty string and return JAVACALL OK.
 *
 * @param buffer lists of available ports. This value must be null terminated.
 * @param maxBufferLen the maximum length of buffer
 * @retval JAVACALL_OK success
 * @retval JAVACALL_FAIL fail or the return length is more than maxBufferLen characters.
 */
javacall_result
javacall_serial_list_available_ports(char* buffer, int maxBufLen)
{
    char *comPort[] = {"COM1","COM2","COM3","COM4","COM5","COM6","COM7","COM8","COM9"};
    int i, len, totalCount = 0;
    HANDLE hPort;
    char *port;

    buffer[0] = '\0';
    for (i = 0; i < 9; i++) {
        port = comPort[i];
        hPort =  CreateFile(port,
                GENERIC_READ | GENERIC_WRITE,
                0,
                0,OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,0);

        if (hPort != INVALID_HANDLE_VALUE) {
            len = strlen(port);
            if (totalCount == 0) {
                if ((int) len >= maxBufLen) {
                    return JAVACALL_FAIL;
                } else {
                    strcpy(buffer, port);
                }
            } else {
                if ((int) totalCount + len + 1 >= maxBufLen) {
                    return JAVACALL_FAIL;
                } else {
                    strcat(buffer, ",");
                    strcat(buffer, port);
                }
            }
            totalCount += len;
            CloseHandle(hPort);
        } /* end  if (hPort != INVALID_HANDLE_VALUE)*/
    } /* end for */

    return JAVACALL_OK;
}

static void
wincomm_serial_configurePort(HANDLE hPort, int baudRate, unsigned int options)
{
    DCB deviceControlBlock;
    COMMTIMEOUTS commTimeouts;

    memset(&deviceControlBlock, 0, sizeof (deviceControlBlock));
    deviceControlBlock.DCBlength = sizeof (deviceControlBlock);
    deviceControlBlock.BaudRate = baudRate;

    /* binary mode, no EOF check */
    deviceControlBlock.fBinary = 1;

    /* 0-4=no,odd,even,mark,space */
    if (options & JAVACALL_SERIAL_ODD_PARITY) {
        deviceControlBlock.fParity = 1;
        deviceControlBlock.Parity = 1;
    } else if (options & JAVACALL_SERIAL_EVEN_PARITY) {
        deviceControlBlock.fParity = 1;
        deviceControlBlock.Parity = 2;
    }

    /* CTS output flow control */
    if (options & JAVACALL_SERIAL_AUTO_CTS) {
        deviceControlBlock.fOutxCtsFlow = 1;
    }

    deviceControlBlock.fDtrControl = DTR_CONTROL_ENABLE;

    /* RTS flow control */
    if (options & JAVACALL_SERIAL_AUTO_RTS) {
        deviceControlBlock.fRtsControl = RTS_CONTROL_HANDSHAKE;
    } else {
        deviceControlBlock.fRtsControl = RTS_CONTROL_ENABLE;
    }

    /* BITS_PER_CHAR_8 is 2 bits and includes BITS_PER_CHAR_7 */
    if ((options & JAVACALL_SERIAL_BITS_PER_CHAR_8)
            == JAVACALL_SERIAL_BITS_PER_CHAR_8) {
        deviceControlBlock.ByteSize = 8;
    } else {
        deviceControlBlock.ByteSize = 7;
    }

    /* StopBits 0,1,2 = 1, 1.5, 2 */
    if (options & JAVACALL_SERIAL_STOP_BITS_2) {
        deviceControlBlock.StopBits = 2;
    }

    if (!SetCommState((HANDLE) hPort, &deviceControlBlock)) {
	REPORT("configurePort << ERROR SetCommState\n");
        return;
    }

    if (!GetCommTimeouts((HANDLE) hPort, &commTimeouts)) {
	REPORT("configurePort << ERROR GetCommTime timeouts\n");
        return;
    }

    /* set non blocking (async methods) */
    commTimeouts.ReadIntervalTimeout = MAXDWORD;
    commTimeouts.ReadTotalTimeoutMultiplier = 0;
    commTimeouts.ReadTotalTimeoutConstant = 0;

    commTimeouts.WriteTotalTimeoutMultiplier = 0;
    commTimeouts.WriteTotalTimeoutConstant = 0;

    if (!SetCommTimeouts((HANDLE) hPort, &commTimeouts)) {
     	 REPORT("configurePort << ERROR cannot SetCommTimeouts timeouts\n");
        return;
    }
}


/**
 * Opens serial link according to the given parameters
 *
 * @param devName the name of the port / device to be opened ("COMM1")
 * @param baudRate the baud rate for the open connection
 * @param options the serial link option (JAVACALL_SERIAL_XXX)
 * @return <tt>hPort</tt> on success,
 *         <tt>-1</tt> on error
 */
static javacall_handle
wincomm_serial_open(const char *devName, int baudRate, unsigned int options)
{
    DCB dcbSerialParams = {0};
    COMMTIMEOUTS timeouts={0};
    char lastError[1024];
    HANDLE fd;
    LPCTSTR lpFileName;

    lpFileName = devName;
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,GetLastError(),MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),lastError,1024,NULL);

    fd = CreateFile(lpFileName,
        GENERIC_READ | GENERIC_WRITE,
        0,
        0,OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,0);

    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,GetLastError(),MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),lastError,1024,NULL);

    if (fd == INVALID_HANDLE_VALUE) {
        REPORT("javacall_serial_open: couldn't open port!\n");
        return (javacall_handle) -1;
    }

    wincomm_serial_configurePort(fd, baudRate, options);

    return (javacall_handle)fd;
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
   DCB dcbSerialParams = {0};

   dcbSerialParams.BaudRate = baudRate; //2400,4800,9600,115200...
   if (!SetCommState(hPort, &dcbSerialParams)) {
        REPORT("javacall_serial_set_baudRate: error setting serial params!\n");
        return JAVACALL_FAIL;
   }

   return JAVACALL_OK;
}

/**
 * Retrive the current baudRate of the open serial port
 *
 * @param hPort the port to configure
 * @param baudRate pointer to where to return the baudRate
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> on error
 */
javacall_result /*OPTIONAL*/
javacall_serial_get_baudRate(javacall_handle hPort, int *baudRate)
{
    DCB dcbSerialParams = {0};

    if (!GetCommState(hPort, &dcbSerialParams)) {
        REPORT("javacall_serial_init: error setting serial params!\n");
        return JAVACALL_FAIL;
    }

    *baudRate = dcbSerialParams.BaudRate;

    return JAVACALL_OK;
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
javacall_result /*OPTIONAL*/ javacall_serial_configure(javacall_handle pHandle, int baudRate, int options) {
    wincomm_serial_configurePort(pHandle, baudRate, options);
	return JAVACALL_OK;
}

/**
 * Reads a specified number of bytes from serial link,

 * @param hPort the port to read the data from
 * @param buffer to which data is read
 * @param size number of bytes to be read. Actual number of bytes
 *              read may be less, if less data is available
 * @param byteRead actual number the were read from the port.
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        if there was an error
 * @retval JAVACALL_WOULD_BLOCK  if the caller must call the finish function again to complete the operation
 */
static javacall_result
wincomm_serial_read(javacall_handle hPort, unsigned char* buffer, int size, int *byteRead)
{
    if (!ReadFile((HANDLE)hPort, buffer, size, byteRead, NULL)) {
         REPORT("javacall_serial_read: error reading\n");
         return JAVACALL_FAIL;
    } else {
        return JAVACALL_OK;
    }
}


/**
 * Writes a specified number of bytes to serial link,
 * @param hPort the port to write the data to
 * @param buffer buffer to write data from
 * @param size number of bytes to be write.
 * @return the number of bytes actually written, or -1 on error
 */
static int
wincomm_serial_write(javacall_handle hPort, unsigned char* buffer, int size)
{
    DWORD dwBytesWrite = 0;
    if (!WriteFile((HANDLE)hPort, buffer, size, &dwBytesWrite, NULL)) {
        REPORT("javacall_serial_write: error reading\n");
        return JAVACALL_FAIL;
    }
    return (int)dwBytesWrite;
}

/**
 * Closes serial link

 * @param hPort the port to close
 * after this call, java is guaranteed not to call javacall_serial_read() or
 * javacall_serial_write() before issuing another javacall_serial_open( ) call.
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt> or negative value on error
 */
static javacall_result
wincomm_serial_close(javacall_handle hPort)
{
    int result;
    REPORT("javacall_serial_finalize \n");
    result = CloseHandle((HANDLE)hPort);
    if (result == 0) {
        REPORT("ERROR javacall_serial_finalize failed!!!! <<< \n");
        return JAVACALL_FAIL;
    }
    REPORT("javacall_serial_finalize <<\n");

    return JAVACALL_OK;
}

/**
 * Initiates opening serial link according to the given parameters
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
javacall_result  /*OPTIONAL*/
javacall_serial_open_start(const char *devName, int baudRate, unsigned int options ,javacall_handle *hPort)
{
    *hPort = wincomm_serial_open(devName, baudRate, options);
    if ((javacall_handle)-1 == *hPort) {
        return JAVACALL_FAIL;
    } else {
        return JAVACALL_OK;
    }
}

/**
 * Finishes opening serial link according to the given parameters
 *
 * @param handle the handle of the port to be opend
 * @retval JAVACALL_OK on success,
 * @retval JAVACALL_FAIL on error
 * @retval JAVACALL_WOULD_BLOCK  if the caller must call the finish function again to complete the operation
 */
javacall_result  /*OPTIONAL*/
javacall_serial_open_finish(javacall_handle hPort)
{
    return JAVACALL_FAIL;
}

/**
 * Initiates closing serial link
 *
 * @param hPort the port to close
 * after this call, java is guaranteed not to call javacall_serial_read() or
 * javacall_serial_write() before issuing another javacall_serial_open( ) call.
 *
 * @return <tt>JAVACALL_OK</tt> on success,
 *         <tt>JAVACALL_FAIL</tt>
 * @retval JAVACALL_WOULD_BLOCK  if the caller must call the finish function again to complete the operation
 */
javacall_result /*OPTIONAL*/
javacall_serial_close_start(javacall_handle hPort)
{
    if (wincomm_serial_close(hPort) == JAVACALL_OK)
        return JAVACALL_OK;
    else
	return JAVACALL_FAIL;
}

/**
 * Finishes closing serial link.
 *
 * @param hPort the port to close
 * @retval <tt>JAVACALL_OK</tt> on success,
 * @retval <tt>JAVACALL_FAIL</tt>
 * @retval JAVACALL_WOULD_BLOCK  if the caller must call the finish function again to complete the operation
 */
javacall_result /*OPTIONAL*/
javacall_serial_close_finish(javacall_handle hPort)
{
    return JAVACALL_OK;
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
javacall_result /*OPTIONAL*/
javacall_serial_read_start(javacall_handle hPort, unsigned char* buffer, int size ,int *bytesRead)
{
    return wincomm_serial_read(hPort, buffer, size, bytesRead);
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
 * @retval JAVACALL_WOULD_BLOCK  if the caller must call the finish function again to complete the operation
 */
javacall_result /*OPTIONAL*/
javacall_serial_read_finish(javacall_handle hPort, unsigned char* buffer, int size, int *bytesRead)
{
    return JAVACALL_OK;
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
javacall_result /*OPTIONAL*/
javacall_serial_write_start(javacall_handle hPort, unsigned char* buffer, int size, int *bytesWritten)
{
    *bytesWritten = wincomm_serial_write(hPort, buffer, size);
    if (-1 == *bytesWritten)
        return JAVACALL_FAIL;
    else
        return JAVACALL_OK;
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
javacall_result /*OPTIONAL*/
javacall_serial_write_finish(javacall_handle hPort, unsigned char* buffer, int size, int *bytesWritten)
{
    return JAVACALL_OK;
}
