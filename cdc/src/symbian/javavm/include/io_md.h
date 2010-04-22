/*
 * @(#)io_md.h	1.4 06/10/10
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

#ifndef _SYMBIAN_IO_MD_H
#define _SYMBIAN_IO_MD_H

#include "javavm/include/porting/vm-defs.h"

#include <e32def.h>
#include <sys/param.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#if defined(_LFS_LARGEFILE) || defined(HAVE_64_BIT_IO)
#define POSIX_HAVE_64_BIT_IO
#define CVMioOpen		CVMioOpen
#define CVMioSeek		CVMioSeek
#define CVMioSetLength		CVMioSetLength
#define CVMioAvailable		CVMioAvailable
#define CVMioFileSizeFD		CVMioFileSizeFD
#endif

#define POSIX_HAVE_FSYNC
#undef  POSIX_HAVE_FTRUNCATE
#define POSIX_HAVE_CUSTOM_IO_AVAILABLE

#define read(x,y,z) read(x,(char *)y,z)
#define write(x,y,z) write(x,(char *)y,z)

/* Define the functions we will override */
#define CVMioRead		CVMioRead
#define CVMioClose		CVMioClose

CVMBool linuxIoInit(void);

#endif /* _SYMBIAN_IO_MD_H */
