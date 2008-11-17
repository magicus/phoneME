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
#include "javavm/include/porting/threads.h"
#include "javavm/include/porting/ansi/stdlib.h"
#include "portlibs/posix/threads.h"
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <dlfcn.h>
#include <unistd.h>
#include <string.h>
#include "javavm/include/interpreter.h"
#include "javavm/include/assert.h"

#ifdef CVM_JVMTI
#ifndef __UCLIBC__
#include <gnu/libc-version.h>
#endif
#ifndef clockid_t
typedef int clockid_t;
#endif
static int (*_clock_gettime)(clockid_t, struct timespec *);
static int (*_clock_getres)(clockid_t, struct timespec *);
static int (*_pthread_getcpuclockid)(pthread_t, clockid_t *);
static CVMBool supportsFastThreadCpuTime = CVM_FALSE;
static CVMBool isNPTL = CVM_FALSE;
static int clockTicsPerSec = 100;
#define SEC_IN_NANOSECS  CONST64(1000000000)

void CVMtimeClockInit(void) {
    /* we do dlopen's in this particular order due to bug in linux */
    /* dynamical loader (see 6348968) leading to crash on exit */
#ifdef CLOCK_PROCESS_CPUTIME_ID
    void* handle = dlopen("librt.so.1", RTLD_LAZY);
    if (handle == NULL) {
	handle = dlopen("librt.so", RTLD_LAZY);
    }

    if (handle) {
	int (*clock_getres_func)(clockid_t, struct timespec*) = 
	    (int(*)(clockid_t, struct timespec*))dlsym(handle, "clock_getres");
	int (*clock_gettime_func)(clockid_t, struct timespec*) = 
	    (int(*)(clockid_t, struct timespec*))dlsym(handle, "clock_gettime");
	if (clock_getres_func && clock_gettime_func) {
	    /*
	     * See if monotonic clock is supported by the kernel. Note that some
	     * early implementations simply return kernel jiffies (updated every
	     * 1/100 or 1/1000 second). It would be bad to use such a low res clock
	     * for nano time (though the monotonic property is still nice to have).
	     * It's fixed in newer kernels, however clock_getres() still returns
	     * 1/HZ. We check if clock_getres() works, but will ignore its reported
	     * resolution for now. Hopefully as people move to new kernels, this
	     * won't be a problem.
	     */
	    struct timespec res;
	    struct timespec tp;
	    if (clock_getres_func (CLOCK_PROCESS_CPUTIME_ID, &res) == 0 &&
		clock_gettime_func(CLOCK_PROCESS_CPUTIME_ID, &tp)  == 0) {
		/* yes, monotonic clock is supported */
		_clock_gettime = clock_gettime_func;
		_clock_getres = clock_getres_func;
	    } else {
		/* close librt if there is no monotonic clock */
		dlclose(handle);
	    }
	}
    }
    {
#ifndef __UCLIBC__
	char _gnu_libc_version[32];
	char *glibc_version = _gnu_libc_version;
	char *libpthread_version;
	/*
	 * Save glibc and pthread version strings. Note that _CS_GNU_LIBC_VERSION
	 * and _CS_GNU_LIBPTHREAD_VERSION are supported in glibc >= 2.3.2. Use a 
	 * generic name for earlier versions.
	 * Define macros here so we can build HotSpot on old systems.
	 */
# ifndef _CS_GNU_LIBC_VERSION
# define _CS_GNU_LIBC_VERSION 2
# endif
# ifndef _CS_GNU_LIBPTHREAD_VERSION
# define _CS_GNU_LIBPTHREAD_VERSION 3
# endif
	
	size_t n = confstr(_CS_GNU_LIBC_VERSION, NULL, 0);
	if (n > 0) {
	    char *str = (char *)malloc(n);
	    confstr(_CS_GNU_LIBC_VERSION, str, n);
	    glibc_version = str;
	} else {
	    /* _CS_GNU_LIBC_VERSION is not supported, try gnu_get_libc_version()*/
	    snprintf(_gnu_libc_version, sizeof(_gnu_libc_version), 
		     "glibc %s %s", glibc_version, gnu_get_libc_release());
	}
	
	n = confstr(_CS_GNU_LIBPTHREAD_VERSION, NULL, 0);
	if (n > 0) {
	    char *str = (char *)malloc(n);
	    confstr(_CS_GNU_LIBPTHREAD_VERSION, str, n);
	    /*	    
	     * Vanilla RH-9 (glibc 2.3.2) has a bug that confstr() always tells
	     * us "NPTL-0.29" even we are running with LinuxThreads. Check if
	     * this is the case:
	     */
	    if (strcmp(glibc_version, "glibc 2.3.2") == 0 &&
		strstr(str, "NPTL")) {
		/*
		 * LinuxThreads has a hard limit on max number of threads. So
		 * sysconf(_SC_THREAD_THREADS_MAX) will return a positive
		 * value. On the other hand, NPTL does not have such a limit,
		 * sysconf() will return -1 and errno is not changed.
		 * Check if it is really NPTL:
		 */
		if (sysconf(_SC_THREAD_THREADS_MAX) > 0) {
		    free(str);
		    str = "linuxthreads";
		}
	    }
	    libpthread_version = str;
	} else {
	    /* glibc before 2.3.2 only has LinuxThreads. */
	    libpthread_version = "linuxthreads";
	}
	
	if (strstr(libpthread_version, "NPTL")) {
	    isNPTL = CVM_TRUE;
	}
#endif /* __UCLIBC__ */
    }
#endif
    clockTicsPerSec = sysconf(_SC_CLK_TCK);
}

void CVMtimeThreadCpuClockInit(CVMThreadID *threadID) {
    clockid_t clockid;
    struct timespec tp;
    int (*pthread_getcpuclockid_func)(pthread_t, clockid_t *) = 
	(int(*)(pthread_t, clockid_t *)) dlsym(RTLD_DEFAULT, "pthread_getcpuclockid");
    /*
     * Switch to using fast clocks for thread cpu time if
     * the sys_clock_getres() returns 0 error code.
     * Note, that some kernels may support the current thread
     * clock (CLOCK_THREAD_CPUTIME_ID) but not the clocks
     * returned by the pthread_getcpuclockid().
     * If the fast Posix clocks are supported then the sys_clock_getres()
     * must return at least tp.tv_sec == 0 which means a resolution
     * better than 1 sec. This is extra check for reliability.
     */

    if(pthread_getcpuclockid_func &&
       pthread_getcpuclockid_func(POSIX_COOKIE(threadID), &clockid) == 0 &&
       _clock_getres != NULL &&
       _clock_getres(clockid, &tp) == 0 && tp.tv_sec == 0) {
	_pthread_getcpuclockid = pthread_getcpuclockid_func;
	supportsFastThreadCpuTime = CVM_TRUE;
    }
}

/*
 * current_thread_cpu_time(bool) and thread_cpu_time(Thread*, bool) 
 * are used by JVM M&M and JVMTI to get user+sys or user CPU time
 * of a thread.
 *
 * current_thread_cpu_time() and thread_cpu_time(Thread*) returns
 * the fast estimate available on the platform.
 *
 *
 *  -1 on error.
 */ 

static CVMInt64
threadCpuTimeX(CVMThreadID *thread, CVMBool user_sys_cpu_time) {
    static CVMBool proc_pid_cpu_avail = CVM_TRUE;
    static CVMBool proc_task_unchecked = CVM_TRUE;
    static const char *proc_stat_path = "/proc/%d/stat";
    pid_t  tid = POSIX_COOKIE(thread);
    int i;
    char *s;
    char stat[2048];
    int statlen;
    char proc_name[64];
    int count;
    long sys_time, user_time;
    char string[64];
    int idummy;
    long ldummy;
    FILE *fp;

    /* We first try accessing /proc/<pid>/cpu since this is faster to
     * process.  If this file is not present (linux kernels 2.5 and above)
     * then we open /proc/<pid>/stat.
     */
    if ( proc_pid_cpu_avail ) {
	sprintf(proc_name, "/proc/%d/cpu", tid);
	fp =  fopen(proc_name, "r");
	if ( fp != NULL ) {
	    count = fscanf( fp, "%s %lu %lu\n", string, &user_time, &sys_time);
	    fclose(fp);
	    if ( count != 3 ) return -1;

	    if (user_sys_cpu_time) {
		return ((CVMInt64)sys_time + (CVMInt64)user_time) * (SEC_IN_NANOSECS / clockTicsPerSec);
	    } else {
		return (CVMInt64)user_time * (SEC_IN_NANOSECS / clockTicsPerSec);
	    }
	}
	else proc_pid_cpu_avail = CVM_FALSE;
    }
    /*
     * The /proc/<tid>/stat aggregates per-process usage on
     * new Linux kernels 2.6+ where NPTL is supported.
     * The /proc/self/task/<tid>/stat still has the per-thread usage.
     * See bug 6328462.
     * There can be no directory /proc/self/task on kernels 2.4 with NPTL
     * and possibly in some other cases, so we check its availability.
     */
    if (proc_task_unchecked && isNPTL) {
	/* This is executed only once */
	proc_task_unchecked = CVM_FALSE;
	fp = fopen("/proc/self/task", "r");
	if (fp != NULL) {
	    proc_stat_path = "/proc/self/task/%d/stat";
	    fclose(fp);
	}
    }

    sprintf(proc_name, proc_stat_path, tid);
    fp = fopen(proc_name, "r");
    if ( fp == NULL ) return -1;
    statlen = fread(stat, 1, 2047, fp);
    stat[statlen] = '\0';
    fclose(fp);
    /*
     * Skip pid and the command string. Note that we could be dealing with
     * weird command names, e.g. user could decide to rename java launcher
     * to "java 1.4.2 :)", then the stat file would look like
     *                1234 (java 1.4.2 :)) R ... ...
     * We don't really need to know the command string, just find the last
     * occurrence of ")" and then start parsing from there. See bug 4726580.
     */
    s = strrchr(stat, ')');
    i = 0;
    if (s == NULL ) return -1;

    /* Skip blank chars */
    do s++; while (isspace(*s));

    count = sscanf(s,"%c %d %d %d %d %d %lu %lu %lu %lu %lu %lu %lu", 
		   (char *)&idummy, &idummy, &idummy, &idummy, &idummy,
		   &idummy, &ldummy, &ldummy, &ldummy, &ldummy, &ldummy, 
		   &user_time, &sys_time);
    if ( count != 13 ) return -1;
    if (user_sys_cpu_time) {
	return ((jlong)sys_time + (jlong)user_time) * (SEC_IN_NANOSECS / clockTicsPerSec);
    } else {
	return (jlong)user_time * (SEC_IN_NANOSECS / clockTicsPerSec);
    }
}

/*
 * This is the fastest way to get thread cpu time on Linux.
 * Returns cpu time (user+sys) for any thread, not only for current.
 * POSIX compliant clocks are implemented in the kernels 2.6.16+.
 * It might work on 2.6.10+ with a special kernel/glibc patch.
 * For reference, please, see IEEE Std 1003.1-2004:
 *   http://www.unix.org/single_unix_specification
 */

static CVMInt64
fastThreadCpuTime(clockid_t clockid) {
  struct timespec tp;
  int rc = 0;
  rc = _clock_gettime(clockid, &tp);
  CVMassert(rc == 0);
  return (tp.tv_sec * SEC_IN_NANOSECS) + tp.tv_nsec;
}

CVMInt64
CVMtimeThreadCpuTime(CVMThreadID *thread) {
    /* consistent with what current_thread_cpu_time() returns */
    if (supportsFastThreadCpuTime) {
	pthread_t tid = POSIX_COOKIE(thread);
	clockid_t clockid;
	int rc;
	/* Get the thread clockid */
	rc = _pthread_getcpuclockid(tid, &clockid);
	CVMassert(rc == 0);
	return fastThreadCpuTime(clockid);
    } else {
	return threadCpuTimeX(thread, CVM_TRUE /* user + sys */);
    }
}

CVMBool CVMtimeIsThreadCpuTimeSupported(void) {
    return CVM_TRUE;
}

CVMInt64
CVMtimeCurrentThreadCpuTime(CVMThreadID *thread) {
#ifdef CLOCK_PROCESS_CPUTIME_ID
    if (supportsFastThreadCpuTime) {
	return fastThreadCpuTime(CLOCK_THREAD_CPUTIME_ID);
    } else
#endif
    {
	/* return user + sys since the cost is the same */
	return threadCpuTimeX(&(CVMgetEE()->threadInfo), CVM_TRUE /* user + sys */);
    }
}


CVMInt64
CVMtimeNanosecs(void)
{
#ifdef CLOCK_PROCESS_CPUTIME_ID
    if (_clock_gettime != NULL) {
        struct timespec tp;
        int status = _clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tp);
        (void)status;
        CVMassert(status == 0);
        return (CVMInt64)(((CVMInt64)tp.tv_sec) * SEC_IN_NANOSECS +
                          (CVMInt64)tp.tv_nsec);
    } else
#endif
    {
        struct timeval t;
        gettimeofday(&t, 0);
        return (CVMInt64)(((CVMInt64)t.tv_sec) * 1000000 + (CVMInt64)(t.tv_usec));
    }
}
#endif

CVMInt64
CVMtimeMillis(void)
{
    struct timeval t;
    gettimeofday(&t, 0);
    return (CVMInt64)(((CVMInt64)t.tv_sec) * 1000 + (CVMInt64)(t.tv_usec/1000));
}
