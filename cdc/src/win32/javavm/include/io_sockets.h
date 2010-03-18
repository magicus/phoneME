/*
 * @(#)io_md.h	1.7 06/10/10
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
 *
 */

#ifndef _WIN32_IO_SOCKETS_H
#define _WIN32_IO_SOCKETS_H

/*
 * Initializes all listening servers and their structures,
 * if the data is already initialized 0 is returned and the call
 * has no effect. Parameters are port numbers on which the 
 * servers are supposed to listen
 */
int SIOInit(int, int);

/*
 * Terminates all running servers
 * If the module was not started using sio_init
 * 0 is returned and nothing is done.
 */
int SIOStop();

/*
 * Sends data to clients depending on file descriptor
 * 1 = stdout, 2 = stderr
 * Returns 1 on invalid descriptor, -1 if the module
 * was not initialized. Returns 0 otherwise.
 */
int SIOWrite(int fd, char * buff, int len);

/*
 * Gets port number of server asociated with given file
 * descriptor. Returns 0 on invalid descriptor, -1 if the module
 * was not initialized.
 */
int SIOGetListeningPort(int);

#endif /* _WIN32_IO_SOCKETS_H */
