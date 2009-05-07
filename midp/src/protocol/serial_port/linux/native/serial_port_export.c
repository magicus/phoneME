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
 * FILE:      commProtocol_md.c
 * OVERVIEW:  Operations to support serial communication ports
 *            on Unix (native Unix support for the 'comm:' protocol)
 * AUTHOR:    
 *=======================================================================*/

/*=======================================================================
 * Include files
 *=======================================================================*/

#include <sys/types.h>
#include <sys/stat.h>

/*
 * Note: To use this file on UNIX System V platforms like Solaris
 *       remove the ioctl.h include
 */
#include <sys/ioctl.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include <string.h>
#include <stdio.h>

#include <midpServices.h>
#include <serial_port_export.h>

#include <midp_logging.h>

#define DEVICE_PORT_PREFIX "/dev/ttyS"
#define PORT_NUM_START_POS ((int)sizeof(DEVICE_PORT_PREFIX) - 1)

/**
 * Converts the given logical port name into the platform-dependent name
 * of the device to open.
 * Note that only port with numbers from 1 to 9 are supported.
 *
 * @param pLogicalPortName logical name of the port, for example, COM1
 *
 * @return pointer to a static buffer containing the platform-specific name
 *         of the port to open
 */
static char*
logical_port_name2device_port_name(const char* pLogicalPortName) {
    static char pDevicePortName[] = DEVICE_PORT_PREFIX "xx";
    int pos = PORT_NUM_START_POS;

    while (*pLogicalPortName && pos < (int)(sizeof(pDevicePortName) - 1)) {
        char ch = *pLogicalPortName++;
        if (ch >= '1' && ch <= '9') {
            pDevicePortName[pos++] = ch - 1;
            break;
        }
    }

    if (pos == PORT_NUM_START_POS) {
        pDevicePortName[pos++] = '0';
    }

    pDevicePortName[pos] = 0;
    return pDevicePortName;
}
#undef PORT_NUM_START_POS

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
                    int options, int *pHandle, void **pContext) {
    int hPort, openFlags;

    /* Linux implementation never returns PCSL_NET_WOULDBLOCK */
    (void)pContext;

    /* do not become the controlling tty */
    openFlags = O_RDWR | O_NOCTTY;

    hPort = open(logical_port_name2device_port_name(pszDeviceName), openFlags);
    if (hPort < 0) {
        *pHandle = (int)INVALID_HANDLE;
        return PCSL_NET_IOERROR;
    }
    /* Set exclusive use flag to block other open calls with EBUSY. */
    ioctl(hPort, TIOCEXCL, 0);

    if (configurePort(hPort, baudRate, options) != PCSL_NET_SUCCESS) {
        close(hPort);
        *pHandle = (int)INVALID_HANDLE;
        return PCSL_NET_IOERROR;
    }

    *pHandle = hPort;

    return PCSL_NET_SUCCESS;
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
    /*Do nothing*/
    (void)pszDeviceName;
    (void)baudRate;
    (void)options;
    (void)pHandle;
    (void)context;
    return PCSL_NET_IOERROR;
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

    struct termios attributes;
    speed_t speed;
    int linesToSet;
    int flgs;

    /* memset(&attributes, 0, sizeof (attributes)); */
    tcgetattr(hPort, &attributes);
    /* Raw input */
    attributes.c_lflag &= ~(ICANON | ECHO | ISIG);
    /* Raw output */
    attributes.c_cflag |= CREAD | CLOCAL | HUPCL;
    /* Don't make any CR/LF changes on input/output streams */
    attributes.c_iflag &= ~INLCR;
    attributes.c_iflag &= ~IGNCR;
    attributes.c_iflag &= ~ICRNL;
    attributes.c_iflag &= ~IXON;

    attributes.c_oflag &= ~(ONLCR | OCRNL | ONOCR | ONLRET);

    switch(baudRate) {
    case 50:
        speed = B50;
        break;

    case 75:
        speed = B75;
        break;

    case 110:
        speed = B110;
        break;

    case 134:
        speed = B134;
        break;

    case 150:
        speed = B150;
        break;

    case 200:
        speed = B200;
        break;

    case 300:
        speed = B300;
        break;

    case 600:
        speed = B600;
        break;

    case 1200:
        speed = B1200;
        break;

    case 1800:
        speed = B1800;
        break;

    case 2400:
        speed = B2400;
        break;

    case 4800:
        speed = B4800;
        break;

    case 9600:
        speed = B9600;
        break;

    case 19200:
        speed = B19200;
        break;

    case 38400:
        speed = B38400;
        break;

    case 57600:
        speed = B57600;
        break;

    case 115200:
        speed = B115200;
        break;

    default:
        return PCSL_NET_IOERROR;
    }

    cfsetispeed(&attributes, speed);
    cfsetospeed(&attributes, speed);

    /* default no parity */
    if (options & ODD_PARITY) {
        attributes.c_cflag |= PARENB | PARODD;
    } else if (options & EVEN_PARITY) {
        attributes.c_cflag |= PARENB;
    }

    /* CTS output flow control */
    if (options & AUTO_CTS) {
        attributes.c_cflag |= CRTSCTS;
    }

    /* RTS flow control */
    /*
     * This option is no support on Linux but if this file is used
     * on UNIX System V platform like Solaris then uncomment RTS option code. 
     *
     * if (options & AUTO_RTS) {
     *     attributes.c_cflag |= CRTSXOFF;
     * }
     */

    /* BITS_PER_CHAR_8 is 2 bits and includes BITS_PER_CHAR_7 */
    if ((options & BITS_PER_CHAR_8) == BITS_PER_CHAR_8) {
        attributes.c_cflag |= CS8;
    } else {
        attributes.c_cflag |= CS7;
    }

    /* default 1 stop bit */
    if (options & STOP_BITS_2) {
        attributes.c_cflag |= CSTOPB;
    }


    /* set non blocking since we are not using async methods */
    flgs = fcntl(hPort, F_GETFL, 0);
    fcntl(hPort, F_SETFL, flgs | O_NONBLOCK);

    /* no minimum amount of bytes, no secs */
    attributes.c_cc[VMIN] = 0;
    attributes.c_cc[VTIME] = 0;

    if (tcsetattr(hPort, TCSANOW, &attributes) == -1) {
        return PCSL_NET_IOERROR;
    }

    /* Make sure the Data Terminal Ready line is on */
    linesToSet = TIOCM_DTR;
    ioctl(hPort, TIOCMBIS, &linesToSet);

    return PCSL_NET_SUCCESS;
}

/**
 * Close a serial port.
 *
 * @param hPort handle to a native serial port
 */
void closePort(int hPort) {
    close(hPort);
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

    /* Linux implementation never returns PCSL_NET_WOULDBLOCK */
    (void)pContext;

    if (nNumberOfBytesToWrite == 0) {
        *pBytesWritten = 0;
        status = PCSL_NET_SUCCESS;
    } else {
        int bytesWritten = write((int)hPort, pBuffer, nNumberOfBytesToWrite);
        if (bytesWritten == -1) { /* error writing */
            switch (errno) {
                case EAGAIN: /* Write would block */
                    /** IMPL_NOTE.  
                     * Function should return PCSL_NET_WOULDBLOCK
                     * at this case. Will be implemented.
                     */
                    /* status = PCSL_NET_WOULDBLOCK; */
                    *pBytesWritten = 0;
                    status = PCSL_NET_SUCCESS;
                    break;
                case EINTR: /* Writing has been interrupted */
                    status = PCSL_NET_INTERRUPTED;
                    break;
                default:
                    status = PCSL_NET_IOERROR;
            }
        } else { /* Writing is correct */
            *pBytesWritten = bytesWritten;
            status = PCSL_NET_SUCCESS;
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
    (void)context;
    return writeToPortStart(hPort, pBuffer, nNumberOfBytesToWrite,
        pBytesWritten, NULL);
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
        int bytesRead = read((int)hPort, pBuffer, nNumberOfBytesToRead);
        if (bytesRead == -1) { /* error reading */
            switch (errno) {
                case EAGAIN: /* No data available */
                    /** IMPL_NOTE.  
                     * Function should return PCSL_NET_WOULDBLOCK
                     * at this case. Will be implemented.
                     */
                    /* status = PCSL_NET_WOULDBLOCK; */
                    *pBytesRead = 0;
                    status = PCSL_NET_SUCCESS;
                    break;
                case EINTR: /* Reading has been interrupted */
                    status = PCSL_NET_INTERRUPTED;
                    break;
                default:
                    status = PCSL_NET_IOERROR;
            }
        } else { /* Reading is correct */
            *pBytesRead = bytesRead;
            status = PCSL_NET_SUCCESS;
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
    (void)context;
    return readFromPortStart(hPort, pBuffer, nNumberOfBytesToRead,
        pBytesRead, NULL);
}


