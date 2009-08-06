/*
 * @(#)commProtocol_md.c	1.3 06/10/10
 * 
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

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: networking
 * FILE:      commProtocol_md.c
 * OVERVIEW:  Operations to support serial communication ports
 *            on Unix/Linux (native Unix support for the 'comm:' protocol)
 * AUTHOR:    
 *=======================================================================*/

/*=======================================================================
 * Include files
 *=======================================================================*/

#define LINUX


#include <sys/types.h>
#include <sys/stat.h>

#ifdef LINUX
#include <sys/ioctl.h>
#endif /* LINUX */

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include <string.h>
#include <stdio.h>

#include <commProtocol.h>

#include "jni.h"
#include "jni_util.h"


/*=======================================================================
 * Definitions and declarations
 *=======================================================================*/

static char* getLastErrorMsg(char* pHeader);

/* non system errors */
static char* COMM_BAD_PORTNUMBER = "Invalid port number";
static char* COMM_BAD_BAUDRATE   = "Invalid baud rate";

/*=========================================================================
 * Protocol implementation functions
 *=======================================================================*/

/*=========================================================================
 * FUNCTION:        freePortError
 * OVERVIEW:        
 * INTERFACE:
 *   parameters:    *pszError: Error message
 *   returns:       <nothing>
 *=======================================================================*/

void freePortError(char* pszError) {
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
    char* pFileName = NULL;

    switch (port) {
    case 0:
        pFileName = "/dev/term/a";
#ifdef LINUX
        pFileName = "/dev/ttyS0";
#endif
        break;

    case 1:
        pFileName = "/dev/term/b";
#ifdef LINUX
        pFileName = "/dev/ttyS1";
#endif
        break;

    default:
      ppszError = malloc(sizeof(char *));
      *ppszError = strdup(COMM_BAD_PORTNUMBER);
      return -1;
    }   

    return((long)openPortByName(ppszError, pFileName, baudRate, options));
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
    int hPort = 0;
    int openFlags = 0;
    ppszError = malloc(sizeof(char *));

    /* do not become the controlling tty */
    openFlags = O_RDWR | O_NOCTTY;
    hPort = open(pszDeviceName, openFlags);
    if (hPort < 0) {
      *ppszError = getLastErrorMsg("open failed");
      return -1;
    }
    /* Set exclusive use flag to block other open calls with EBUSY. */
    //ioctl(hPort, TIOCEXCL, 0);

    configurePort(ppszError, hPort, baudRate, options);

    if (*ppszError != NULL) {
        close(hPort);
        return -1;
    }

    return (int)hPort;
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

    struct termios attributes;
    speed_t speed;
    int linesToSet;
    int flgs;

    memset(&attributes, 0, sizeof (attributes));
    attributes.c_cflag = CREAD | HUPCL | CLOCAL;

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
        *ppszError = COMM_BAD_BAUDRATE;
        return;
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

/*  The following is not supported by Linux */
#ifndef LINUX
    /* RTS flow control */
    if (options & AUTO_RTS) {
        /***attributes.c_cflag |= CRTSXOFF;***/
    }
#endif

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


    /* set blocking since we are not using async methods */
    flgs = fcntl(hPort, F_GETFL, 0);
    //    fcntl(hPort, F_SETFL, flgs | O_NONBLOCK);

    /* block for at least 1 character, but wait .1 secs */
    attributes.c_cc[VMIN] = 1;
    attributes.c_cc[VTIME] = 0;

    if (tcsetattr(hPort, TCSANOW, &attributes) == -1) {
      ppszError = malloc(sizeof(char *));
      *ppszError = getLastErrorMsg("set attr failed");
      return;
    }


    /* Make sure the Data Terminal Ready line is on */
    linesToSet = TIOCM_DTR;
    ioctl(hPort, TIOCMBIS, &linesToSet);


    *ppszError = NULL;

}

/*=========================================================================
 * FUNCTION:        closePort
 * OVERVIEW:        Closes the open serial port.
 * INTERFACE:
 *   parameters:    hPort:  The open port.
 *   returns:       <nothing>
 *=======================================================================*/

void closePort(long hPort) {
    close((int)hPort);
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
    int nNumberOfBytesWritten = 0;
    ppszError = malloc(sizeof(char *));


    nNumberOfBytesWritten = write((int)hPort, pBuffer, nNumberOfBytesToWrite);
    if (nNumberOfBytesWritten < 0) {
        if (errno == EAGAIN) {
            return 0;
        }

        *ppszError = getLastErrorMsg("write failed");
        return -1;
    }

    *ppszError = NULL;
    return nNumberOfBytesWritten;
}

/*=========================================================================
 * FUNCTION:        readFromPort
 * OVERVIEW:        reads from a serial port, with blocking
 * INTERFACE:
 *   parameters:    **ppszError:          Error message
 *                  hPort:                The name of the port to read from.
 *                  pBuffer:              The buffer to store the bytes read.
 *                  nNumberOfBytesToRead: The number of bytes to read.
 *   returns:       The number of bytes read.
 *=======================================================================*/

long readFromPort(char** ppszError, long hPort, char* pBuffer,
                 long nNumberOfBytesToRead) {
    int nNumberOfBytesRead = 0;
    ppszError = malloc(sizeof(char *));

    nNumberOfBytesRead = read((int)hPort, pBuffer, nNumberOfBytesToRead);
    if (nNumberOfBytesRead < 0) {
      if (errno == EAGAIN) {
	return 0;
      }

      *ppszError = getLastErrorMsg("read failed");
      return -1;
    }

    *ppszError = NULL;
    return nNumberOfBytesRead;
}

/*=========================================================================
 * FUNCTION:        getLastErrorMsg
 * OVERVIEW:        Returns the last error message.
 * INTERFACE:
 *   parameters:    *pHeader:  The message header.
 *   returns:       The error message.
 *=======================================================================*/

static char* getLastErrorMsg(char* pHeader) {
    int lastError;
    char* pszError;

    lastError = errno;
    pszError = strerror(lastError);

#if INCLUDEDEBUGCODE
    printf("%s(%08.8X): ", pHeader, lastError);
    puts(pszError);    
#endif /* INCLUDEDEBUGCODE */

    return pszError;
}


/************************ New JNI Functions ************************/


JNIEXPORT void JNICALL 
Java_com_sun_cdc_io_j2me_comm_Protocol_native_1openByNumber(JNIEnv *env,
							    jobject this,
							    jint port,
							    jint baud,
							    jint flags)
{
  char **errorMsg = NULL;
  long portNum;
  portNum = openPortByNumber(errorMsg, (long)port, (long)baud,
			(long)flags);

}


JNIEXPORT jint JNICALL 
Java_com_sun_cdc_io_j2me_comm_Protocol_native_1openByName(JNIEnv *env,
							  jobject this,
							  jstring name,
							  jint baud,
							  jint flags)
{
  char *errorMsg = "okay";
  long portNum;
  char *portname;

  char* pFileName = NULL;


  pFileName = "/dev/ttyS0";
       
  if (IS_NULL(name)) {
    JNU_ThrowNullPointerException(env, "name is null");
    return -1;  
  }
  portname = (char *)JNU_GetStringPlatformChars(env, name, NULL);

  portNum = openPortByName((char **)errorMsg, (char *)pFileName, (long)baud,
                    (long)flags);


  return(portNum);
}


JNIEXPORT void JNICALL  
Java_com_sun_cdc_io_j2me_comm_Protocol_native_1configurePort(JNIEnv *env,
							     jobject this,
							     jint port,
							     jint baud,
							     jint flags)
{
  char **errorMsg = NULL;

  configurePort(errorMsg, (long)port, (long)baud, (long)flags);

}


JNIEXPORT void JNICALL  
Java_com_sun_cdc_io_j2me_comm_Protocol_native_1close(JNIEnv *env,
						     jobject this,
						     jint hPort)
{
  closePort((long)hPort);
}


JNIEXPORT void JNICALL  
Java_com_sun_cdc_io_j2me_comm_Protocol_registerCleanup(JNIEnv *env,
						       jobject this)
{
}


static const jint BUFSIZE = 1024;

JNIEXPORT jint JNICALL  
Java_com_sun_cdc_io_j2me_comm_Protocol_native_1readBytes(JNIEnv *env,
							 jobject this,
							 jint hPort,
							 jbyteArray bArray,
							 jint off,
							 jint len)
{
    char *errorMsg = NULL;
    jbyte b[BUFSIZE];
    jint chunk = BUFSIZE;
    jint bytesRead = BUFSIZE;
    jint result = 0;
    jint end = off + len;

    for (; off < end && bytesRead == chunk; off += chunk) {
        if (off + chunk > end) {
            chunk = end - off;
        }
        bytesRead = readFromPort(&errorMsg, (long)hPort, (char *)b,
                (long)chunk);
        
        if (bytesRead > 0) {
            result += bytesRead;
            (*env)->SetByteArrayRegion(env, bArray, off, bytesRead, (jbyte *)b);
        }
    }
    return(result);
}

JNIEXPORT jint JNICALL 
Java_com_sun_cdc_io_j2me_comm_Protocol_native_1writeBytes(JNIEnv *env,
							  jobject this,
							  jint hPort,
							  jbyteArray bArray,
							  jint off,
							  jint len)
{
    char *errorMsg = NULL;
    jbyte b[BUFSIZE];
    jint chunk = BUFSIZE;
    jint bytesWritten = BUFSIZE;
    jint result = 0;
    jint end = off + len;
    for (; off < end && bytesWritten == chunk; off += chunk) {
        if (off + chunk > end) {
            chunk = end - off;
        }
        (*env)->GetByteArrayRegion(env, bArray, off, chunk, b);
        bytesWritten = writeToPort(&errorMsg, (long)hPort, (char *)b,
                        (long)chunk);
        if (bytesWritten > 0) {
            result += bytesWritten;
        }
    }
    return(result);
}
