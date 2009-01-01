/*
 * @(#)io_md.h	1.14 06/10/10
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

#ifndef _VXWORKS_IO_MD_H
#define _VXWORKS_IO_MD_H

#include <ioLib.h>

/* S_ISSOCK not supported in VxWorks */
#define S_ISSOCK(m)	0

/* %comment: mam053 */
#define POSIX_HAVE_FIOFLUSH
#define POSIX_HAVE_FIOTRUNC

#define POSIX_HAVE_CUSTOM_NATIVE_PATH
#define POSIX_HAVE_CUSTOM_LSEEK
#define POSIX_HAVE_CUSTOM_IO_AVAILABLE

#define CVMioOpen	CVMioOpen
#define CVMioWrite	CVMioWrite
#define CVMioNativePath	CVMioNativePath
#define CVMioSeek	CVMioSeek
#define CVMioAvailable	CVMioAvailable

#include "portlibs/posix/io.h"

extern void ioInitStaticState(void);
extern void ioDestroyStaticState(void);

#endif /* _VXWORKS_IO_MD_H */
