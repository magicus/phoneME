/*
 * @(#)time_md.c	1.8 06/10/10
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

#include "javavm/include/porting/time.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/lwp.h>
#include <sys/priocntl.h>
#include "javavm/include/interpreter.h"

/*  this gets us the new structured proc interfaces of 5.6 & later */
#define _STRUCTURED_PROC 1  
#include <sys/procfs.h>     /*  see comment in <sys/procfs.h> */


#ifdef CVM_JVMTI
#define SEC_IN_NANOSECS  CONST64(1000000000)
static CVMBool T2_libthread;
const unsigned int thrTimeOff  = (unsigned int)(&((prusage_t *)(NULL))->pr_utime);
const unsigned int thrTimeSize = (unsigned int)(&((prusage_t *)(NULL))->pr_ttime) -
                               (unsigned int)(&((prusage_t *)(NULL))->pr_utime);

/*
 *isT2_libthread()

 * Routine to determine if we are currently using the new T2 libthread.

 * We determine if we are using T2 by reading /proc/self/lstatus and
 * looking for a thread with the ASLWP bit set.  If we find this status
 * bit set, we must assume that we are NOT using T2.  The T2 team
 * has approved this algorithm.

 * We need to determine if we are running with the new T2 libthread
 * since setting native thread priorities is handled differently 
 * when using this library.  All threads created using T2 are bound 
 * threads. Calling thr_setprio is meaningless in this case. 
 */
CVMBool isT2_libthread() {
    int i, rslt;
    static prheader_t * lwpArray = NULL;             
    static int lwpSize = 0;
    static int lwpFile = -1;
    lwpstatus_t * that;
    int aslwpcount;
    CVMBool isT2 = CVM_FALSE;

#define ADR(x)  ((unsigned int)(x))
#define LWPINDEX(ary,ix)   ((lwpstatus_t *)(((ary)->pr_entsize * (ix)) + (ADR((ary) + 1))))

    aslwpcount = 0;
    lwpSize = 16*1024;
    lwpArray = ( prheader_t *)calloc(lwpSize, sizeof(char));
    if (lwpArray == NULL) {
	return(isT2);
    }
    lwpFile = open ("/proc/self/lstatus", O_RDONLY, 0);
    if (lwpFile < 0) {
	free(lwpArray);
	return(isT2);
    }
    for (;;) {
	lseek (lwpFile, 0, SEEK_SET);
	rslt = read (lwpFile, lwpArray, lwpSize);
	if ((lwpArray->pr_nent * lwpArray->pr_entsize) <= lwpSize) {
	    break;
	}
	lwpSize = lwpArray->pr_nent * lwpArray->pr_entsize;
	free(lwpArray);
	lwpArray = ( prheader_t *)calloc(lwpSize, sizeof(char));
	if (lwpArray == NULL) {
	    return(isT2);
	}
    }

    // We got a good snapshot - now iterate over the list.
    for (i = 0; i < lwpArray->pr_nent; i++ ) {
	that = LWPINDEX(lwpArray,i);
	if (that->pr_flags & PR_ASLWP) {
	    aslwpcount++;
	}
    }
    if ( aslwpcount == 0 ) isT2 = CVM_TRUE;

    free(lwpArray);
    close (lwpFile);
    return (isT2);
}

void CVMtimeClockInit() {
    T2_libthread = isT2_libthread();
}


void CVMtimeThreadCpuClockInit(CVMThreadID *threadID) {
}

CVMInt64
CVMtimeThreadCpuTime(CVMThreadID *thread) {
    char proc_name[64];
    int count;
    prusage_t prusage;
    jlong lwp_time;
    int fd;

    sprintf(proc_name, "/proc/%d/lwp/%d/lwpusage", (int)getpid(), thread->lwp_id);
    fd = open(proc_name, O_RDONLY);
    if ( fd == -1 ) return -1;

    do {
	count = pread(fd, 
		      (void *)&prusage.pr_utime, 
		      thrTimeSize, 
		      thrTimeOff);
    } while (count < 0 && errno == EINTR);
    close(fd);
    if ( count < 0 ) return -1;
  
    // user + system CPU time
    lwp_time = (((jlong)prusage.pr_stime.tv_sec + 
                 (jlong)prusage.pr_utime.tv_sec) * SEC_IN_NANOSECS) + 
	(jlong)prusage.pr_stime.tv_nsec + 
	(jlong)prusage.pr_utime.tv_nsec;

    return(CVMInt64)(lwp_time);
}

CVMBool CVMtimeIsThreadCpuTimeSupported(void) {
    return T2_libthread;
}

CVMInt64
CVMtimeCurrentThreadCpuTime(CVMThreadID *thread) {
    (void)thread;
    return (CVMInt64)gethrvtime();
}

CVMInt64
CVMtimeNanosecs(void)
{
    return (CVMInt64) gethrtime();
}
#endif
