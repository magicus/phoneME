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

/*
 * Machine-dependent I/O definitions.
 */

#ifndef _WIN32_IO_MD_H
#define _WIN32_IO_MD_H

#include "javavm/include/porting/vm-defs.h"

#ifndef WINCE
#include <fcntl.h>
#else
#include "javavm/include/ansi/fcntl.h"
#endif

#if defined(_LFS_LARGEFILE) || defined(HAVE_64_BIT_IO)

#define CVMioOpen		CVMioOpen
#define CVMioSeek		CVMioSeek
#define CVMioSetLength		CVMioSetLength
#define CVMioAvailable		CVMioAvailable
#define CVMioFileSizeFD		CVMioFileSizeFD
#endif

/* Define the functions we will override */
#define CVMioRead		CVMioRead
#define CVMioClose		CVMioClose

void win32IoInit(void);

/*
 * Read data from a file decriptor into a char array.
 *
 * fd        the file descriptor to read from, must be 0.
 * buf       the buffer where to put the read data.
 * nbytes    the number of bytes to read.
 *
 * This function returns 0 on error, and number of bytes read on success.
 */
extern int	readStandardIO(CVMInt32 fd, void *buf,
		         	   CVMUint32 nBytes);

/*
 * Write data from a char array to a file decriptor.
 *
 * fd        the file descriptor to write to. 1 for stdout, 2 for stderr.
 * buf       the buffer from which to fetch the data.
 * nbytes    the number of bytes to write.
 *
 */
extern void	writeStandardIO(CVMInt32 fd, const void *buf, 
                CVMUint32 nBytes);


/*
 * Initializes the destination for the standard I/O 
 *
 * This function should be called before using either
 * writeStandardIO or readStandardIO
 *
 * The implementation may be opening a file, initializing
 * the CriticalSection, or something that is necessary
 * for the Standard IO redirection.
 */

extern void intializeStandardIO();

#endif /* _WIN32_IO_MD_H */
