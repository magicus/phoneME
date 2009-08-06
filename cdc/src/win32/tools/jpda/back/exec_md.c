/*
 * @(#)exec_md.c	1.9 06/10/10
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

#include <windows.h>
#include <string.h>
#include "sys.h"
#include "javavm/include/winntUtil.h"
#ifdef _WIN32_WINNT
#include "javavm/include/winntUtil.h"
#endif
  

int 
dbgsysExec(char *cmdLine)
{
    PROCESS_INFORMATION pi;
    STARTUPINFO si;
    int ret;
    TCHAR *tcmdLine;

    if (cmdLine == 0) {
        return SYS_ERR;
    }
    tcmdLine = createTCHAR(cmdLine);

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);

    ret = CreateProcess(0,                /* executable name */
                        tcmdLine,         /* command line */
                        0,                /* process security attribute */
                        0,                /* thread security attribute */
                        TRUE,             /* inherits system handles */
                        0,                /* normal attached process */
                        0,                /* environment block */
                        0,                /* inherits the current directory */
                        &si,              /* (in)  startup information */
                        &pi);             /* (out) process information */
    
    freeTCHAR(tcmdLine);        
    if (ret == 0) {
        return SYS_ERR;
    } else {
        return SYS_OK;
    }
}

