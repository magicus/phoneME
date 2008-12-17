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
 * FILE:      commProtocol.c
 * OVERVIEW:  Operations to support serial communication ports
 *            (native implementation of the 'comm:' protocol).
 *
 *            These functions are mapped onto platform-specific
 *            functions defined in Vm<Port>/src directories,
 *            where <Port> indicates the name of the target
 *            platform (e.g., Win, Unix, Pilot).
 *=======================================================================*/

/*=======================================================================
 * Include files
 *=======================================================================*/

#include <string.h>
#include <kni.h>
#include <sni.h>
#include <commonKNIMacros.h>

#include <midpMalloc.h>
#include <midp_thread.h>
#include <serial_port_export.h>

/*=======================================================================
 * Protocol implementation functions
 *=======================================================================*/

/**
 * Configure a serial port optional parameters.
 *
 * @param port device port returned from open
 * @param baud baud rate to set the port at
 * @param flags options for the serial port:
 * bit 0: 0 - 1 stop bit, 1 - 2 stop bits 
 * bit 2-1: 00 - no parity, 01 - odd parity, 10 - even parity 
 * bit 4: 0 - no auto RTS, 1 - set auto RTS 
 * bit 5: 0 - no auto CTS, 1 - set auto CTS 
 * bit 7-6: 01 - 7 bits per symbol, 11 - 8 bits per symbol 
 */
KNIEXPORT KNI_RETURNTYPE_VOID
    Java_com_sun_midp_io_j2me_comm_Protocol_native_1configurePort() {

    unsigned int  flags      = (int)KNI_GetParameterAsInt(3);
    int  baud       = (int)KNI_GetParameterAsInt(2);
    int  port       = (int)KNI_GetParameterAsInt(1);

    int status = configurePort(port, baud, flags);
    if (status != PCSL_NET_SUCCESS) {
        midp_snprintf(gKNIBuffer, KNI_BUFFER_SIZE,
                "Error configure port %d \n", port);
        REPORT_INFO1(LC_PROTOCOL, "%s\n", gKNIBuffer);
        KNI_ThrowNew(midpIOException, gKNIBuffer);
    }

    KNI_ReturnVoid();
}

/**
 * Open a serial port by system dependent device name.
 *
 * @param name device name of the port
 * @param baud baud rate to set the port at
 * @param flags options for the serial port
 *
 * @return handle to a native serial port
 *
 * @exception  IOException  if an I/O error occurs.
 */
KNIEXPORT KNI_RETURNTYPE_INT
    Java_com_sun_midp_io_j2me_comm_Protocol_native_1openByName() {

    int    flags = (int)KNI_GetParameterAsInt(3);
    int    baud = (int)KNI_GetParameterAsInt(2);
    int    nameLen;
    char   szName[MAX_NAME_LEN + 1];
    jchar* temp;
    int    hPort = (int)INVALID_HANDLE;
    int    i;
    int status = PCSL_NET_IOERROR;
    void* context = NULL;
    MidpReentryData* info;

    KNI_StartHandles(1);
    KNI_DeclareHandle(nameObject);
    KNI_GetParameterAsObject(1, nameObject);
    
    info = (MidpReentryData*)SNI_GetReentryData(NULL);
    if (info == NULL) {
        nameLen = KNI_GetStringLength(nameObject);
        if (nameLen < 0 || nameLen > MAX_NAME_LEN) {
            midp_snprintf(gKNIBuffer, KNI_BUFFER_SIZE,
                "Serial device name has wrong length: %d\n", nameLen);
            REPORT_INFO1(LC_PROTOCOL, "%s\n", gKNIBuffer);
            KNI_ThrowNew(midpIllegalArgumentException, gKNIBuffer);
        } else {
            temp = (jchar*)szName;
            KNI_GetStringRegion(nameObject, 0, nameLen, temp);

            /* device names are in ASCII */
            for (i = 0; i < nameLen; i++) {
                szName[i] = (char)temp[i];
            }
            szName[nameLen] = 0;
		
            status = openPortByNameStart(szName, baud, flags, &hPort, &context);
        }
    } else {
        /* reinvocation */
        hPort = info->descriptor;
        context = info->pResult;
        status = openPortByNameFinish(szName, baud, flags, &hPort, context);	
    }

    switch (status) {
        case PCSL_NET_SUCCESS:			
            /* do nothing and return normally */
            break;
        case PCSL_NET_INTERRUPTED:			
            midp_snprintf(gKNIBuffer, KNI_BUFFER_SIZE,
                "Opening port %s has been interrupted\n", szName);
            REPORT_INFO1(LC_PROTOCOL, "%s\n", gKNIBuffer);
            KNI_ThrowNew(midpInterruptedIOException, gKNIBuffer);
            break;
        case PCSL_NET_WOULDBLOCK:			
            midp_thread_wait(COMM_OPEN_SIGNAL, hPort, context);
            break;
        default:	   
            midp_snprintf(gKNIBuffer, KNI_BUFFER_SIZE,
                "Opening port %s was failed\n", szName);
            REPORT_INFO1(LC_PROTOCOL, "%s\n", gKNIBuffer);
            KNI_ThrowNew(midpIOException, gKNIBuffer);
    }

    KNI_EndHandles();
    KNI_ReturnInt((jint)hPort);
}

/**
 * Close a serial port.
 *
 * @param hPort handle to a native serial port
 *
 * @exception  IOException  if an I/O error occurs
 */
KNIEXPORT KNI_RETURNTYPE_VOID
    Java_com_sun_midp_io_j2me_comm_Protocol_native_1close() {

    int hPort = (int)KNI_GetParameterAsInt(1);

    closePort(hPort);
    KNI_ReturnVoid();
}

/**
 * Native finalizer.
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_io_j2me_comm_Protocol_finalize()
{
    int hPort;

    KNI_StartHandles(2);
    KNI_DeclareHandle(instance);
    KNI_DeclareHandle(clazz);
    KNI_GetThisPointer(instance);

    KNI_GetObjectClass(instance, clazz);
    hPort = KNI_GetIntField(instance, KNI_GetFieldID(clazz, "handle", "I"));

    if (hPort != -1) {
        REPORT_INFO1(LC_PROTOCOL, "closePort(1st) >> hPort=%d\n",hPort);
        closePort(hPort);
    }

    KNI_EndHandles();

    KNI_ReturnVoid();
}

/**
 * Read from a serial port without blocking.
 *
 * @param hPort handle to a native serial port
 * @param b I/O buffer
 * @param off starting offset for data
 * @param len length of data
 *
 * @return number of bytes read
 *
 * @exception  IOException  if an I/O error occurs
 */
KNIEXPORT KNI_RETURNTYPE_INT
    Java_com_sun_midp_io_j2me_comm_Protocol_native_1readBytes() {

    int  length = (int)KNI_GetParameterAsInt(4);
    int  offset = (int)KNI_GetParameterAsInt(3);
    int  hPort  = (int)KNI_GetParameterAsInt(1);
    int  bytesRead = -1;
    int status = PCSL_NET_IOERROR;
    void* context = NULL;
    MidpReentryData* info;

    KNI_StartHandles(1);
    KNI_DeclareHandle(bufferObject);
    KNI_GetParameterAsObject(2, bufferObject);

    info = (MidpReentryData*)SNI_GetReentryData(NULL);
    if (info == NULL) {

        if (hPort <= 0) {
            midp_snprintf(gKNIBuffer, KNI_BUFFER_SIZE,
                "Read from port: handle %d is invalid\n", hPort);
            REPORT_INFO1(LC_PROTOCOL, "%s\n", gKNIBuffer);
            KNI_ThrowNew(midpIllegalArgumentException, gKNIBuffer);
        } else {   
            SNI_BEGIN_RAW_POINTERS;
            status = readFromPortStart(hPort, 
                (char*)&(JavaByteArray(bufferObject)[offset]),
                length, &bytesRead, &context);
            SNI_END_RAW_POINTERS;
        }    
    } else {
        /* reinvocation */
        hPort = info->descriptor;
        context = info->pResult;
        SNI_BEGIN_RAW_POINTERS;
        status = readFromPortFinish(hPort,
            (char*)&(JavaByteArray(bufferObject)[offset]),
             length, &bytesRead, context);
        SNI_END_RAW_POINTERS;
    }

    switch (status) {
        case PCSL_NET_SUCCESS:			
            /*do nothing and return normally */
            break;
        case PCSL_NET_INTERRUPTED:			
            midp_snprintf(gKNIBuffer, KNI_BUFFER_SIZE,
                "Reading from port %d has been interrupted\n", hPort);
            REPORT_INFO1(LC_PROTOCOL, "%s\n", gKNIBuffer);
            KNI_ThrowNew(midpInterruptedIOException, gKNIBuffer);
            break;
        case PCSL_NET_WOULDBLOCK:			
            midp_thread_wait(COMM_READ_SIGNAL, hPort, context);
            break;
        default:	   
            midp_snprintf(gKNIBuffer, KNI_BUFFER_SIZE,
                "Reading from port %d was failed\n", hPort);
            REPORT_INFO1(LC_PROTOCOL, "%s\n", gKNIBuffer);
            KNI_ThrowNew(midpIOException, gKNIBuffer);
    }

    KNI_EndHandles();
    KNI_ReturnInt((jint)bytesRead);
    
}

/**
 * Write to a serial port without blocking.
 *
 * @param hPort handle to a native serial port
 * @param b I/O buffer
 * @param off starting offset for data
 * @param len length of data
 *
 * @return number of bytes that were written
 *
 * @exception  IOException  if an I/O error occurs.
 */
KNIEXPORT KNI_RETURNTYPE_INT
    Java_com_sun_midp_io_j2me_comm_Protocol_native_1writeBytes() {

    int  length = (int)KNI_GetParameterAsInt(4);
    int  offset = (int)KNI_GetParameterAsInt(3);
    int  hPort  = (int)KNI_GetParameterAsInt(1);
    int   bytesWritten = 0;
    int status = PCSL_NET_IOERROR;
    void* context = NULL;
    MidpReentryData* info;

    KNI_StartHandles(1);
    KNI_DeclareHandle(bufferObject);
    KNI_GetParameterAsObject(2, bufferObject);

    info = (MidpReentryData*)SNI_GetReentryData(NULL);
    if (info == NULL) {

        if (hPort < 0) {
            midp_snprintf(gKNIBuffer, KNI_BUFFER_SIZE,
                "Write to port: handle %d is invalid\n", hPort);
            REPORT_INFO1(LC_PROTOCOL, "%s\n", gKNIBuffer);
            KNI_ThrowNew(midpIllegalArgumentException, gKNIBuffer);
        } else {        		
            SNI_BEGIN_RAW_POINTERS;
            status = writeToPortStart(hPort,
                (char*)&(JavaByteArray(bufferObject)[offset]),
                length, &bytesWritten, &context);
            SNI_END_RAW_POINTERS;
        }
    } else {
        /* reinvocation */
        hPort = info->descriptor;
        context = info->pResult;
        SNI_BEGIN_RAW_POINTERS;
        status = writeToPortFinish(hPort,
            (char*)&(JavaByteArray(bufferObject)[offset]),
            length, &bytesWritten, context);
        SNI_END_RAW_POINTERS;
    }

    switch (status) {
        case PCSL_NET_SUCCESS:			
            /*do nothing and return normally */
            break;
        case PCSL_NET_INTERRUPTED:			
            midp_snprintf(gKNIBuffer, KNI_BUFFER_SIZE,
                "Writing to port %d has been interrupted\n", hPort);
            REPORT_INFO1(LC_PROTOCOL, "%s\n", gKNIBuffer);
            KNI_ThrowNew(midpInterruptedIOException, gKNIBuffer);
            break;
        case PCSL_NET_WOULDBLOCK:			
            midp_thread_wait(COMM_WRITE_SIGNAL, hPort, context);
            break;
        default:	   
            midp_snprintf(gKNIBuffer, KNI_BUFFER_SIZE,
                "Writing to  port %d was failed\n", hPort);
            REPORT_INFO1(LC_PROTOCOL, "%s\n", gKNIBuffer);
            KNI_ThrowNew(midpIOException, gKNIBuffer);
    }
    	    
    KNI_EndHandles();
    KNI_ReturnInt((jint)bytesWritten);
    
}

