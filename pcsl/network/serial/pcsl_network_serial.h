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

#ifndef _PCSL_NETWORK_SERIAL_H
#define _PCSL_NETWORK_SERIAL_H

/**
 * @file
 * @defgroup serial Serial Port Interface
 * @ingroup network_low
 * @brief Serial port interfaces \n
 * ##include <>
 * @{
 *
 * This file defines serial port interfaces for serial port operations 
 * such as open, read, write, close of the serial port.
 * Every platform will have its own implementation for these interfaces.
 */

/**
 * Open a serial port and set the standard attributes for it. Note that
 * the implementation of this interface will prepare an appropriate port
 * name according to the platform. For example, port_id=1 will be 
 * "/dev/ttyS0" for Linux, "/dev/ttysa" for Solaris and "COM1" for Windows
 *  
 * @param : serial port number on the host
 *
 * @return Serial port fd
 *
 */
extern int pcsl_serial_open(int port_id); 

/**
 * Read the data from serial port
 *
 * @param port : serial port fd
 * @param buffer : Output buffer to hold the characters read
 * @param size : Length of the array 
 *
 * @return No of characters read
 *
 */
extern int pcsl_serial_readchar(int port, char* buffer, int size); 

/**
 * Write a character to the serial port
 *
 * @param port : serial port fd
 * @param buffer : Input buffer to hold the data to be written
 * @param size : Length of the array 
 *
 * @return No of characters written
 *
 */
extern int pcsl_serial_writechar(int port, char* buffer, int size);

/**
 * Close the serial port buffer
 *
 * @param port : serial port fd
 *
 * @return 0 upon success, -ve value upon error
 *
 */
extern int pcsl_serial_close(int port); 

/**
 * Flush the serial port buffer
 *
 * @param port : serial port fd
 *
 */
extern void pcsl_serial_flush(int port); 


/** @} */ //End of group serial

#endif /* _PCSL_NETWORK_SERIAL_H */



