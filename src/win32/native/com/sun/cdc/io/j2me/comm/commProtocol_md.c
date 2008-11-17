/*
 * @(#)commProtocol_md.c	1.5 06/10/10
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

/*
 **************************************************************************
 *
 * NOTE: This is a placeholder stubs file for the Win32 port of
 *       the CommConnection, Generic Connection Framework (GCF)
 *       'comm:' protocol.  Use the Linux port as the guideline
 *       to create this file.  Remove this comment after implementing.
 *
 **************************************************************************
 */

/*=========================================================================
 * SYSTEM:    CDC
 * SUBSYSTEM: networking
 * FILE:      commProtocol_md.c
 * OVERVIEW:  Operations to support serial communication ports
 *            on Win32 (native Windows support for the 'comm:' protocol)
 * AUTHOR:    
 *=======================================================================*/

/*=======================================================================
 * Include files
 *=======================================================================*/

#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
/***** #include <unistd.h> *****/
#include <errno.h>
/***** #include <termios.h> *****/
#include <string.h>
#include <stdio.h>

/***** #include <commProtocol.h> *****/

#include "jni.h"
#include "jni_util.h"


/*=======================================================================
 * Definitions and declarations
 *=======================================================================*/

static char* getLastErrorMsg(char* pHeader);

/* non system errors */
static char* COMM_BAD_PORTNUMBER = "Invalid port number";
static char* COMM_BAD_BAUDRATE   = "Invalid baud rate";




/************************ New JNI Functions ************************/


JNIEXPORT void JNICALL 
Java_com_sun_cdc_io_j2me_comm_Protocol_native_1openByNumber(JNIEnv *env,
							    jobject this,
							    jint port,
							    jint baud,
							    jint flags)
{

}


JNIEXPORT jint JNICALL 
Java_com_sun_cdc_io_j2me_comm_Protocol_native_1openByName(JNIEnv *env,
							  jobject this,
							  jstring name,
							  jint baud,
							  jint flags)
{
  return(0);
}


JNIEXPORT void JNICALL  
Java_com_sun_cdc_io_j2me_comm_Protocol_native_1configurePort(JNIEnv *env,
							     jobject this,
							     jint port,
							     jint baud,
							     jint flags)
{

}


JNIEXPORT void JNICALL  
Java_com_sun_cdc_io_j2me_comm_Protocol_native_1close(JNIEnv *env,
						     jobject this,
						     jint hPort)
{

}


JNIEXPORT void JNICALL  
Java_com_sun_cdc_io_j2me_comm_Protocol_registerCleanup(JNIEnv *env,
						       jobject this)
{

}


JNIEXPORT jint JNICALL  
Java_com_sun_cdc_io_j2me_comm_Protocol_native_1readBytes(JNIEnv *env,
							 jobject this,
							 jint hPort,
							 jbyteArray bArray,
							 jint off,
							 jint len)
{
  return(0);
}


JNIEXPORT jint JNICALL  
Java_com_sun_cdc_io_j2me_comm_Protocol_native_1writeBytes(JNIEnv *env,
							  jobject this,
							  jint hPort,
							  jbyteArray bArray,
							  jint off,
							  jint len)
{
  return(0);
}



