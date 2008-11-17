/*
 * @(#)globals_md.c	1.17 06/10/10
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

#define VXWORKS_PRIVATE
#include "javavm/include/porting/globals.h"
#include "javavm/include/threads_md.h"
#include "javavm/include/io_md.h"
#include "javavm/include/path_md.h"
#include "generated/javavm/include/build_defs.h"

CVMBool CVMinitVMTargetGlobalState()
{
    /*
     * Initialize the target global state pointed to by 'target'.
     */

    return CVM_TRUE;
}

void CVMdestroyVMTargetGlobalState()
{
    /*
     * ... and destroy it.
     */
}

static CVMProperties props;

CVMBool CVMinitStaticState(CVMpathInfo *pathInfo)
{
    char *p;
    /*
     * Initialize the static state for this address space
     */
    threadInitStaticState();
    ioInitStaticState();
    pathInfo->basePath = strdup("..");
    if (pathInfo->basePath == NULL) {
        return CVM_FALSE;
    }
    p = (char *)malloc(strlen("..") + 1 + strlen("lib") + 1);
    if (p == NULL) {
        return CVM_FALSE;
    }
    strcpy(p, "..");
    pEnd = p + strlen(p);
    *pEnd++ = CVM_PATH_LOCAL_DIR_SEPARATOR;
    strcpy(pEnd, "lib");
    pathInfo->libPath = p;
    /* lib and dll are the same so this shortcut */
    pathInfo->dllPath = strdup(p);
    if (pathInfo->dllPath == NULL) {
        return CVM_FALSE;
    }
    return CVM_TRUE;
}

void CVMdestroyStaticState()
{
    /*
     * ... and destroy it.
     */
    CVMdestroyPathValues((void *)props.java_home);
    ioDestroyStaticState();
    threadDestroyStaticState();
}

const CVMProperties *CVMgetProperties()
{
    return &props;
}
