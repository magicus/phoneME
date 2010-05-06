/*
 * @(#)io.h	1.7 06/10/10
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

#ifndef _POSIX_IO_H
#define _POSIX_IO_H

CVMInt32
POSIXioGetLastErrorString(char *buf, CVMInt32 len);
#ifndef CVMioGetLastErrorString
#define CVMioGetLastErrorString POSIXioGetLastErrorString
#endif

char *
POSIXioReturnLastErrorString(void);
#ifndef CVMioReturnLastErrorString
#define CVMioReturnLastErrorString POSIXioReturnLastErrorString
#endif

char *
POSIXioNativePath(char *path);
#ifndef CVMioNativePath
#define CVMioNativePath POSIXioNativePath
#endif

CVMInt32
POSIXioFileType(const char *path);
#ifndef CVMioFileType
#define CVMioFileType POSIXioFileType
#endif

CVMInt32
POSIXioOpen(const char *name, CVMInt32 openMode,
	    CVMInt32 filePerm);
#ifndef CVMioOpen
#define CVMioOpen POSIXioOpen
#endif

CVMInt32
POSIXioClose(CVMInt32 fd);
#ifndef CVMioClose
#define CVMioClose POSIXioClose
#endif

CVMInt64
POSIXioSeek(CVMInt32 fd, CVMInt64 offset, CVMInt32 whence);
#ifndef CVMioSeek
#define CVMioSeek POSIXioSeek
#endif

CVMInt32
POSIXioSetLength(CVMInt32 fd, CVMInt64 length);
#ifndef CVMioSetLength
#define CVMioSetLength POSIXioSetLength
#endif

CVMInt32
POSIXioSync(CVMInt32 fd);
#ifndef CVMioSync
#define CVMioSync POSIXioSync
#endif

CVMInt32
POSIXioAvailable(CVMInt32 fd, CVMInt64 *bytes);
#ifndef CVMioAvailable
#define CVMioAvailable POSIXioAvailable
#endif

CVMInt32
POSIXioRead(CVMInt32 fd, void *buf, CVMUint32 nBytes);
#ifndef CVMioRead
#define CVMioRead POSIXioRead
#endif

CVMInt32
POSIXioWrite(CVMInt32 fd, const void *buf, CVMUint32 nBytes);
#ifndef CVMioWrite
#define CVMioWrite POSIXioWrite
#endif

CVMInt32
POSIXioFileSizeFD(CVMInt32 fd, CVMInt64 *size);
#ifndef CVMioFileSizeFD
#define CVMioFileSizeFD POSIXioFileSizeFD
#endif

#endif /* _POSIX_IO_H */
