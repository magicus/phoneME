/*
 * @(#)io.h	1.19 06/10/10
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

#ifndef _INCLUDED_PORTING_IO_H
#define _INCLUDED_PORTING_IO_H

#include "javavm/include/porting/defs.h"
#include "javavm/include/porting/ansi/stddef.h"

/* This is brought over from the JDK 1.2's HPI with minimal
   conversion. It basically assumes a POSIX I/O library.

 */

#define CVM_IO_FILETYPE_REGULAR    (0)
#define CVM_IO_FILETYPE_DIRECTORY  (1)
#define CVM_IO_FILETYPE_OTHER      (2)
#define CVM_IO_EEXIST		   (-100)
#define CVM_IO_ERR		   (-1)
#define CVM_IO_INTR		   (-2)

/* %comment rt005 */
CVMInt32
CVMioGetLastErrorString(char *buf, CVMInt32 len);

/*
 * Return the last error string, without any caller buffers
 */
char *
CVMioReturnLastErrorString();

/*
 * Convert a pathname into native format.  This function does syntactic
 * cleanup, such as removing redundant separator characters.  It modifies
 * the given pathname string in place.
 */
extern char *	CVMioNativePath(char *path);

extern CVMInt32	CVMioFileType(const char *path);

/*
 * Open a file descriptor. This function returns a negative error code
 * on error, or if the file opened is a directory. The return value is
 * a non-negative integer that is the file descriptor on success.  
 */
extern CVMInt32	CVMioOpen(const char *name, CVMInt32 openMode,
			  CVMInt32 filePerm);

/*
 * Close a file descriptor. This function returns -1 on error, and 0
 * on success.
 *
 * fd        the file descriptor to close.
 */
extern CVMInt32	CVMioClose(CVMInt32 fd);

/*
 * Move the file descriptor pointer from whence by offet.
 *
 * fd        the file descriptor to move.
 * offset    the number of bytes to move it by.
 * whence    the start from where to move it.
 *
 * This function returns the resulting pointer location.
 */
extern CVMInt64	CVMioSeek(CVMInt32 fd, CVMInt64 offset, CVMInt32 whence);

/*
 * Set the length of the file associated with the given descriptor to the given
 * length.  If the new length is longer than the current length then the file
 * is extended; the contents of the extended portion are not defined.  The
 * value of the file pointer is undefined after this procedure returns.
 */
extern CVMInt32	CVMioSetLength(CVMInt32 fd, CVMInt64 length);

/*
 * Synchronize the file descriptor's in memory state with that of the
 * physical device.  Return of -1 is an error, 0 is OK. NOTE: JVM_
 * version of this used to throw an error.
 */
extern CVMInt32	CVMioSync(CVMInt32 fd);

/*
 * Returns the number of bytes available for reading from a given file
 * descriptor
 */
extern CVMInt32	CVMioAvailable(CVMInt32 fd, CVMInt64 *bytes);

/*
 * Read data from a file decriptor into a char array.
 *
 * fd        the file descriptor to read from.
 * buf       the buffer where to put the read data.
 * nbytes    the number of bytes to read.
 *
 * This function returns -1 on error, and the number of bytes
 * read on success.
 */
extern CVMInt32	CVMioRead(CVMInt32 fd, void *buf, CVMUint32 nBytes);

/*
 * Write data from a char array to a file decriptor.
 *
 * fd        the file descriptor to read from.
 * buf       the buffer from which to fetch the data.
 * nbytes    the number of bytes to write.
 *
 * This function returns -1 on error, and the number of bytes
 * written on success.
 */
extern CVMInt32	CVMioWrite(CVMInt32 fd, const void *buf,
			   CVMUint32 nBytes);
extern CVMInt32	CVMioFileSizeFD(CVMInt32 fd, CVMInt64 *size);

/*
 * POSIX interface
 *
 * Types:
 * Macros:
 *   O_CREAT, O_RDONLY, ...
 *
 * See POSIX spec for details.
 */

#include CVM_HDR_IO_H

#endif /* _INCLUDED_PORTING_IO_H */
