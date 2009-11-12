/*
 * @(#)globals_md.c	1.28 06/10/10
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

#include "javavm/include/porting/sync.h"
#include "javavm/include/porting/globals.h"
#include "javavm/include/porting/net.h"
#include "javavm/include/porting/io.h"
#include "javavm/include/porting/threads.h"
#include "portlibs/posix/threads.h"
#include "generated/javavm/include/build_defs.h"
#include "javavm/include/path_md.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <stdio.h>
#include <unistd.h>
#include <dlfcn.h>
#include <assert.h>
#include <javavm/include/utils.h>

#ifdef CVM_JIT
#include "javavm/include/porting/jit/jit.h"
#include "javavm/include/globals.h"
#endif

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
	long pagesize = sysconf(_SC_PAGESIZE);
	if (pagesize == -1) {
	    return CVM_FALSE;
	}
	CVMglobals.jit.gcTrapAddr = memalign(pagesize, pagesize);
	if (CVMglobals.jit.gcTrapAddr == NULL) {
	    return CVM_FALSE;
	}
	/* offset by CVMJIT_MAX_GCTRAPADDR_WORD_OFFSET words to allow 
	   negative offsets up to CVMJIT_MAX_GCTRAPADDR_WORD_OFFSET words */
	CVMglobals.jit.gcTrapAddr += CVMJIT_MAX_GCTRAPADDR_WORD_OFFSET;
#ifndef CVMCPU_HAS_VOLATILE_GC_REG
	/* Stuff the address of the page into the page itself. Only needed
	 * when using an NV GC Reg */
	{
	    int i;
	    for (i = -CVMJIT_MAX_GCTRAPADDR_WORD_OFFSET;
		 i < CVMJIT_MAX_GCTRAPADDR_WORD_OFFSET;
		 i++) {
		CVMglobals.jit.gcTrapAddr[i] = CVMglobals.jit.gcTrapAddr;
	    }
	}
#endif
    }
#endif

    return CVM_TRUE;
}

void CVMdestroyVMTargetGlobalState()
{
    /*
     * ... and destroy it.
     */
#ifdef CVMJIT_TRAP_BASED_GC_CHECKS
    if (CVMglobals.jit.gcTrapAddr != NULL) {
	CVMglobals.jit.gcTrapAddr -= CVMJIT_MAX_GCTRAPADDR_WORD_OFFSET;
	free(CVMglobals.jit.gcTrapAddr);
    }
#endif
}

static CVMProperties props;

CVMBool CVMinitStaticState(CVMpathInfo *pathInfo)
{
    /*
     * Initialize the static state for this address space
     */
    if (!POSIXthreadInitStaticState()) {
#ifdef CVM_DEBUG
	fprintf(stderr, "POSIXthreadInitStaticState failed\n");
#endif
	return CVM_FALSE;
    }
    if (!linuxSyncInit()) {
#ifdef CVM_DEBUG
	fprintf(stderr, "linuxSyncInit failed\n");
#endif
        return CVM_FALSE;
    }

#if 0 // FIXME: rework segvhandler to model linux way
#if defined(CVM_JIT) || defined(CVM_USE_MEM_MGR)
    if (!linuxSegvHandlerInit()) {
#ifdef CVM_DEBUG
	fprintf(stderr, "linuxSegvHandlerInit failed\n");
#endif
        return CVM_FALSE;
    }
#endif
#endif

    if (!linuxIoInit()) {
#ifdef CVM_DEBUG
	fprintf(stderr, "linuxIoInit failed\n");
#endif
	return CVM_FALSE;
    }

    linuxNetInit();

    sigblock(sigmask(SIGPIPE));

    {
	char buf[MAXPATHLEN + 1], *p0, *p, *pEnd;

	Dl_info dlinfo;
	if (dladdr((void *)CVMinitStaticState, &dlinfo)) {
	    realpath((char *)dlinfo.dli_fname, buf);
	    p0 = buf;
	} else {
	    fprintf(stderr, "Could not determine cvm path\n");
	    return CVM_FALSE;
	}

	/* get rid of .../bin/cvm */
	p = strrchr(p0, '/');
	if (p == NULL) {
	    goto badpath;
	}
	p = p - 4;
	if (p >= p0 && strncmp(p, "/bin/", 5) == 0) {
	    if (p == p0) {
		p[1] = '\0'; /* this is the root directory */
	    } else {
		p[0] = '\0';
	    }
	} else {
	    goto badpath;
	}
        pathInfo->basePath = strdup(p0);
        if (pathInfo->basePath == NULL) {
            return CVM_FALSE;
        }
        p = (char *)malloc(strlen(p0) + 1 + strlen("lib") + 1);
        if (p == NULL) {
            return CVM_FALSE;
        }
        strcpy(p, p0);
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
    badpath:
	fprintf(stderr, "Invalid path %s\n", p0);
	fprintf(stderr, "Executable must be in a directory named \"bin\".\n");
	return CVM_FALSE;
    }

    return CVM_TRUE;
}

void CVMdestroyStaticState()
{
    /*
     * ... and destroy it.
     */
    CVMdestroyPathValues((void *)&props);
    POSIXthreadDestroyStaticState();
}

const CVMProperties *CVMgetProperties()
{
    return &props;
}
