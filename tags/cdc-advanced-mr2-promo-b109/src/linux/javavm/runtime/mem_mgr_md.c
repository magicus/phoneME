/*
 * @(#)mem_mgr_md.c	1.6 06/10/10
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

#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include "javavm/include/defs.h"
#include "javavm/include/mem_mgr.h"

void
CVMmprotect(void *startAddr, void *endAddr, CVMBool readOnly)
{
    size_t len;
    int prot = (readOnly) ? (PROT_READ | PROT_EXEC) : 
                            (PROT_READ | PROT_WRITE | PROT_EXEC);

    if (endAddr > startAddr) {
        len = (CVMUint8*)endAddr - (CVMUint8*)startAddr;
        if (mprotect(startAddr, len, prot)) {
            perror("mprotect");
        }
    }
}

CVMInt32 CVMgetPagesize()
{
    return (CVMInt32)sysconf(_SC_PAGESIZE);
}
