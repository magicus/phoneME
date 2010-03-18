/*
 * @(#)globals_md.c	1.16 06/10/10
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
#include "javavm/include/defs.h"
#include "javavm/include/globals.h"
#include "javavm/include/porting/globals.h"
#include "javavm/include/porting/sync.h"
#include "javavm/include/porting/net.h"
#include "javavm/include/porting/io.h"
#include "javavm/include/porting/ansi/assert.h"
#include "javavm/include/threads_md.h"
#include "generated/javavm/include/build_defs.h"
#include "javavm/include/io_sockets.h"
#include "javavm/include/path_md.h"

#include <windows.h>
#include <tchar.h>
#include <string.h>

#ifdef CVM_JIT
#include "javavm/include/porting/jit/jit.h"
#endif

#define MAXPATHLEN MAX_PATH


void WIN32ioInit();
CVMBool WIN32threadsInit();
CVMInt32 CVMnetStartup();
void CVMnetShutdown();

CVMBool CVMinitVMTargetGlobalState()
{
    /*
     * Initialize the target global state pointed to by 'target'.
     */
#ifdef CVMJIT_TRAP_BASED_GC_CHECKS
    /*
     * Setup gcTrapAddr to point CVMJIT_MAX_GCTRAPADDR_WORD_OFFSET
     * words into a page aligned page of memory whose first
     * 2* CVMJIT_MAX_GCTRAPADDR_WORD_OFFSET words all point to gcTrapAddr.
     */
    {
        SYSTEM_INFO sysInfo;
        DWORD pagesize;
        int i;

        GetSystemInfo(&sysInfo);

        pagesize = sysInfo.dwPageSize;

        CVMglobals.jit.gcTrapAddr = VirtualAlloc(NULL, pagesize, MEM_COMMIT,
                                                 PAGE_EXECUTE_READWRITE);
        if (CVMglobals.jit.gcTrapAddr == NULL) {
            return CVM_FALSE;
        }

        /*
         * offset by CVMJIT_MAX_GCTRAPADDR_WORD_OFFSET words to allow negative
         * offsets up to CVMJIT_MAX_GCTRAPADDR_WORD_OFFSET words
         */
        CVMglobals.jit.gcTrapAddr += CVMJIT_MAX_GCTRAPADDR_WORD_OFFSET;

#ifndef CVMCPU_HAS_VOLATILE_GC_REG
        /*
         * Stuff the address of the page into the page itself. Only needed
         * when using an NV GC Reg
         */
        for (i = -CVMJIT_MAX_GCTRAPADDR_WORD_OFFSET;
             i < CVMJIT_MAX_GCTRAPADDR_WORD_OFFSET;
             i++) {
            CVMglobals.jit.gcTrapAddr[i] = CVMglobals.jit.gcTrapAddr;
        }
#endif
    }
#endif
    SIOInit(CVMglobals.target.stdoutPort, CVMglobals.target.stderrPort);
    return CVM_TRUE;
}

void CVMdestroyVMTargetGlobalState()
{
    SIOStop();
    /*
     * ... and destroy it.
     */
#ifdef CVMJIT_TRAP_BASED_GC_CHECKS
    if (CVMglobals.jit.gcTrapAddr != NULL) {
        CVMglobals.jit.gcTrapAddr -= CVMJIT_MAX_GCTRAPADDR_WORD_OFFSET;
        VirtualFree(CVMglobals.jit.gcTrapAddr, 0, MEM_RELEASE);
    }
#endif
}

static CVMProperties props;

CVMBool CVMinitStaticState(CVMpathInfo *pathInfo)
{
#if !defined(CVM_MP_SAFE) && !defined(WINCE)
    /* If we don't have MP-safe support built in, then make sure
       that we don't run on more than one processor. */
{
    HANDLE self = GetCurrentProcess();
    DWORD pMask, pSet;
    BOOL r = GetProcessAffinityMask(self, &pMask, &pSet);
    if (r == 0) {
	return CVM_FALSE;
    }
#ifdef WINDOWS_2003_SERVER
    {
	DWORD pid = GetCurrentProcessorNumber();
	assert(((1 << pid) & pMask) != 0);
	pMask = 1 << pid;
    }
#else
    /*
       Just pick the first processor we find.  We should probably
       pick the current processor (how?) or a random processor
       instead.
    */
    {
	int i;
	for (i = 0; i < 32; ++i) {
	    if ((pMask & (1 << i)) != 0) {
		pMask = 1 << i;
		break;
	    }
	}
	assert(i < 32);
    }
#endif
    r = SetProcessAffinityMask(self, pMask);
    if (r == 0) {
	return CVM_FALSE;
    }
}
#endif
    /*
     * Initialize the static state for this address space
     */
    CVMnetStartup();

    WIN32ioInit();

    if (!WIN32threadsInit()) {
	return CVM_FALSE;
    }
    
    {
	TCHAR path[MAX_PATH + 1];
	char buf[2 * MAX_PATH + 1];
	TCHAR *p0, *p1;
	char *p2, *p, *pEnd;

	GetModuleFileName(0, path, sizeof path);

	/* get rid of ...\bin\cvm.exe */
	p0 = _tcsrchr(path, _T('\\'));
	if (p0 == NULL) {
	    return CVM_FALSE;
	}
 
	p1 = p0 - 4;
	if (p1 < path || _tcsncicmp(p1, _T("\\bin"), 4) != 0) {
	    return CVM_FALSE;
	}
	p1[0] = _T('\0');

	wcstombs(buf, path, MAX_PATH);
        p2 = buf;
        pathInfo->basePath = strdup(p2);
        if (pathInfo->basePath == NULL) {
            return CVM_FALSE;
        }
        p = (char *)malloc(strlen(p2) + 1 + strlen("lib") + 1);
        if (p == NULL) {
            return CVM_FALSE;
        }
        strcpy(p, p2);
        pEnd = p + strlen(p);
        *pEnd++ = CVM_PATH_LOCAL_DIR_SEPARATOR;
        strcpy(pEnd, "lib");
        pathInfo->libPath = p;
        p = (char *)malloc(strlen(p2) + 1 + strlen("bin") + 1);
        if (p == NULL) {
            return CVM_FALSE;
        }
        strcpy(p, p2);
        pEnd = p + strlen(p);
        *pEnd++ = CVM_PATH_LOCAL_DIR_SEPARATOR;
        strcpy(pEnd, "bin");
        pathInfo->dllPath = p;
        return CVM_TRUE;
    }
}

void CVMdestroyStaticState()
{
    WIN32threadsDestroy();
    /*
     * ... and destroy it.
     */
    CVMdestroyPathValues((void *)&props);
    CVMnetShutdown();
}

const CVMProperties *CVMgetProperties()
{
    return &props;
}

