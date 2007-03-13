/*
 * @(#)globals_md.c	1.28 06/10/10
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.  
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

#include "javavm/include/porting/globals.h"
#include "javavm/include/porting/sync.h"
#include "portlibs/posix/threads.h"
#include "generated/javavm/include/build_defs.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
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
	int i;
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
	for (i = -CVMJIT_MAX_GCTRAPADDR_WORD_OFFSET;
	     i < CVMJIT_MAX_GCTRAPADDR_WORD_OFFSET;
	     i++) {
	    CVMglobals.jit.gcTrapAddr[i] = CVMglobals.jit.gcTrapAddr;
	}
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

CVMBool CVMinitStaticState()
{
    /*
     * Initialize the static state for this address space
     */
    if (!POSIXthreadInitStaticState()) {
	return CVM_FALSE;
    }
    if (!solarisSyncInit()) {
	return CVM_FALSE;
    }

    sigignore(SIGPIPE);

    {
        char buf[MAXPATHLEN], *p;
	Dl_info dlinfo;

	dladdr((void *)CVMinitStaticState, &dlinfo);
	realpath((char *)dlinfo.dli_fname, buf);
	/* get rid of .../bin/cvm */
	p = strrchr(buf, '/');
	if (p == NULL) {
	    return CVM_FALSE;
	}
	p = p - 4;
	if (p > buf && strncmp(p, "/bin/", 5) == 0) {
	    *p = '\0';
	} else {
	    return CVM_FALSE;
	}
        return(CVMinitPathValues( &props, buf, "lib", "lib" ));
    }
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
