/*
 * @(#)net_md.h	1.12 06/10/10
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
 * Machine-dependent networking definitions.
 */

#ifndef _LINUX_NET_MD_H
#define _LINUX_NET_MD_H

/* Define the functions we will override */
#define CVMnetConnect		CVMnetConnect
#define CVMnetAccept		CVMnetAccept
#define CVMnetSendTo		CVMnetSendTo
#define CVMnetSend		CVMnetSend
#define CVMnetRecvFrom		CVMnetRecvFrom
#define CVMnetRecv		CVMnetRecv
#define CVMnetTimeout		CVMnetTimeout
#define CVMnetSocketClose	CVMioClose

#define POSIX_USE_SELECT

/*
 * Permits Linux platform use of portlibs/posix/posix_net_md.c
 * implementation of CVMnetGetProtoByName.
*/
#define POSIX_HAVE_GETPROTOBYNAME

#include "portlibs/posix/net.h"

void linuxNetInit(void);

#endif /* _LINUX_NET_MD_H */
