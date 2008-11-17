/*
 * @(#)globals_md.cpp	1.8 06/10/10
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

extern "C" {
#include "javavm/include/porting/globals.h"
#include "javavm/include/path_md.h"
#include <javavm/include/utils.h>	/* BAD */
}

#include <stdlib.h>

#include "time_impl.h"
#include "net_impl.h"
#include "threads_impl.h"
#include "io_impl.h"

static CVMProperties props;

CVMBool CVMinitStaticState(CVMpathInfo *pathInfo)
{
    char *p;
    char *home = getenv("CVM_HOME");
    if (home == NULL) {
#if defined(__WINS__)
	home = "J:\\cdc1.1\\";
#else
        home = ".\\";
#endif
    }
    _LIT(Jan1_1970, "19700101:");
    TTime epoc;
    if (epoc.Set(Jan1_1970) != KErrNone) {
	return CVM_FALSE;
    }
    SYMBIANjavaEpoc = epoc.Int64();
    
    pathInfo->basePath = strdup(home);
    if (pathInfo->basePath == NULL) {
        return CVM_FALSE;
    }
    p = (char *)malloc(strlen(home) + 1 + strlen("lib") + 1);
    if (p == NULL) {
        return CVM_FALSE;
    }
    strcpy(p, home);
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

void
CVMdestroyStaticState()
{
    CVMdestroyPathValues(&props);
}

CVMBool CVMinitVMTargetGlobalState()
{
    if (!SYMBIANnetInit()) {
	return CVM_FALSE;
    }
    if (!SYMBIANthreadsInit()) {
	goto fail1;
    }
    if (!SYMBIANioInit()) {
	goto fail2;
    }
    return CVM_TRUE;
fail2:
    SYMBIANthreadsDestroy();
fail1:
    SYMBIANnetDestroy();
    return CVM_FALSE;
}

void
CVMdestroyVMTargetGlobalState()
{
}

const CVMProperties *
CVMgetProperties(void)
{
    return &props;
}
