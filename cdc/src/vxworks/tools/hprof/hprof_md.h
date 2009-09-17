/*
 * @(#)hprof_md.h	1.7 06/10/10
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

#ifndef _VXWORKS_HPROF_MD_H_
#define _VXWORKS_HPROF_MD_H_

#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ppp/pppd.h>
#include <ioLib.h>
#include <sockLib.h>

#ifdef LP64
#define HASH_OBJ_SHIFT 4    /* objects aligned on 32-byte boundary */
#else /* !LP64 */
#define HASH_OBJ_SHIFT 3    /* objects aligned on 16-byte boundary */
#endif /* LP64 */

char *hprof_strdup(const char *str);
#define strdup(str) hprof_strdup(str)

#endif /*_VXWORKS_HPROF_MD_H_*/
