/*
 * @(#)commProtocol.h	1.5 06/10/10
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
 * FILE:      commProtocol.h
 * OVERVIEW:  Prototypes for supporting serial communication ports.
 *            If a target platform wants to support serial ports,
 *            a platform-specific implementation of the functions
 *            must be provided in the Vm<Port>/src directory,
 *            where <Port> is the name of your target platform
 *            (e.g., Win, Unix, Pilot).
 *=======================================================================*/

#ifndef _COMM_PROTOCOL_H_
#define _COMM_PROTOCOL_H_

#ifndef __P
# ifdef __STDC__
#  define __P(p) p
# else
#  define __P(p) ()
# endif
#endif /* __P */

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

extern long openPortByNumber __P ((char** ppszError, long port, long baudRate, long options));
extern long openPortByName __P ((char** ppszError, char* pszDeviceName, long baudRate, long options));
extern void configurePort __P ((char** ppszError, int hPort, long baudRate, unsigned long options));

extern long readFromPort __P ((char** ppszError, long hPort, char* pBuffer, long nNumberOfBytesToRead));

extern long writeToPort __P ((char** ppszError, long hPort, char* pBuffer, long nNumberOfBytesToWrite));

extern void closePort __P ((long hPort));
extern void freePortError __P ((char* pszError));

#endif /* _COMM_PROTOCOL_H_ */


