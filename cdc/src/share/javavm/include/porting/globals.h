/*
 * @(#)globals.h	1.16 06/10/10
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

#ifndef _INCLUDED_PORTING_GLOBALS_H
#define _INCLUDED_PORTING_GLOBALS_H

#include "javavm/include/porting/vm-defs.h"

/*
 * Platform must provide the following struct definition.
 *
 * struct CVMTargetGlobalState {
 *     // platform-specific global state goes here
 * };
 *
 */

/*
 * Initialize platform-specific per-VM global state
 */

extern CVMBool CVMinitVMTargetGlobalState();
extern void CVMdestroyVMTargetGlobalState();

/*
 * Initialize any private platform-specific per-address-space static state.
 */
typedef struct CVMpathInfo CVMpathInfo;

struct CVMpathInfo {
    char *basePath;
    char *libPath;
    char *dllPath;
    char *preBootclasspath;
    char *postBootclasspath;
};

extern CVMBool CVMinitStaticState(CVMpathInfo *);
extern void CVMdestroyStaticState();

/*
 * Bootstrap properties.
 */
typedef struct {
    const char *library_path;	/* java.library.path */
    const char *dll_dir;	/* sun.boot.library.path */
    const char *java_home;	/* java.home */
    const char *ext_dirs;	/* java.ext.dirs */
    const char *sysclasspath;	/* -Xbootclasspath, sun.boot.class.path */
} CVMProperties;

/*
 * Get bootstrap properties.
 */
extern const CVMProperties *CVMgetProperties(void);

#include CVM_HDR_GLOBALS_H

/*
 * A constant pointer to the target specific globals
 */
extern CVMTargetGlobalState * const CVMtargetGlobals;

#endif /* _INCLUDED_PORTING_GLOBALS_H */
