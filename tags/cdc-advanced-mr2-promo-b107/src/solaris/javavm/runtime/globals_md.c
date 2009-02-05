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

#include "javavm/include/porting/globals.h"
#include "javavm/include/porting/sync.h"
#include "portlibs/posix/threads.h"
#include "generated/javavm/include/build_defs.h"
#include "javavm/include/path_md.h"
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

#if !defined(CVM_MP_SAFE)
#include <sys/types.h>
#include <sys/processor.h>
#include <sys/procset.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <procfs.h>
#endif /* !CVM_MP_SAFE */

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

CVMBool CVMinitStaticState(CVMpathInfo *pathInfo)
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

#if !defined(CVM_MP_SAFE)
    { /* pin this process and all its LWPs to the same processor. */
	int rc;
	id_t pid;
	processorid_t cpu;
	
	pid = getpid();
	if (pid < 0) {
	    perror("getpid()");
	    return CVM_FALSE;
	}

	rc = processor_bind(P_PID, pid, PBIND_QUERY, &cpu);
	if (rc < 0) {
	     perror("processor_bind(PBIND_QUERY)");
	     cpu = PBIND_NONE;
	}
	if (cpu == PBIND_NONE)  /* not already bound  */
	{ 
	    /* get psinfo from /proc filesyeste */
	    psinfo_t psinfo;
	    int fd = open("/proc/self/psinfo", O_RDONLY);
	    if (fd < 0) {
		perror("/proc/self/psinfo");
	    } else {
		if (read(fd, &psinfo, sizeof(psinfo)) == sizeof(psinfo)) {
		    cpu = psinfo.pr_lwp.pr_onpro;
		} else {
		    printf("WARNING: failed to read psinfo");
		}
		close(fd);
	    }
	}
	rc = 0;
	if (cpu != PBIND_NONE) {
	    rc = processor_bind(P_PID, pid, cpu, NULL);
	}
	if (cpu == PBIND_NONE || rc < 0) {
	    printf("WARNING: Unable to bind to a cpu\n");
	}
    }
#endif

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
