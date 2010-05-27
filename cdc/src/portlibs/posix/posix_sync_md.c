/*
 * @(#)posix_sync_md.c	1.20 06/10/10
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
#include "javavm/include/porting/int.h"
#include "javavm/include/porting/doubleword.h"
#include <pthread.h>
#include <sys/time.h>
#include <errno.h>
#include <assert.h>

CVMBool
POSIXmutexInit(POSIXMutex * m)
{
    return pthread_mutex_init(&m->m, NULL) == 0;
}

void
POSIXmutexDestroy(POSIXMutex * m)
{
    pthread_mutex_destroy(&m->m);
}

CVMBool
POSIXmutexTryLock(POSIXMutex * m)
{
    return pthread_mutex_trylock(&m->m) == 0;
}

void
POSIXmutexLock(POSIXMutex * m)
{
    int result = pthread_mutex_lock(&m->m);
    assert(result == 0);
    (void)result;
}

void
POSIXmutexUnlock(POSIXMutex * m)
{
    int result = pthread_mutex_unlock(&m->m);
    assert(result == 0);
    (void)result;
}

CVMBool
POSIXcondvarInit(POSIXCondVar * c, POSIXMutex * m)
{
    return pthread_cond_init(&c->c, NULL) == 0;
}

void
POSIXcondvarDestroy(POSIXCondVar * c)
{
    pthread_cond_destroy(&c->c);
}

int
POSIXcondvarWait(POSIXCondVar * c, POSIXMutex * m, CVMJavaLong millis)
{
    if (!CVMlongEqz(millis)) {
	struct timespec tm;
	struct timeval tv;
	CVMJavaLong ltmp, ltmp1, one_thousand;
	CVMJavaInt itmp;
 	gettimeofday(&tv, NULL);
	tm.tv_sec = tv.tv_sec;
	tm.tv_nsec = tv.tv_usec * 1000;
	one_thousand = CVMint2Long(1000);
	ltmp = CVMlongDiv(millis, one_thousand);
	itmp = CVMlong2Int(ltmp);
	ltmp1 = CVMint2Long(itmp);
	if (CVMlongEq(ltmp, ltmp1)) {
	    /* no overflow */
	    tm.tv_sec += itmp; /* millis / 1000 */
	    ltmp = CVMlongRem(millis, one_thousand);
	    itmp = CVMlong2Int(ltmp);
	    tm.tv_nsec += itmp * 1000000; /* (millis % 1000) * 1000000 */
	    if (tm.tv_nsec >= 1000000000) {
		++tm.tv_sec;
		tm.tv_nsec -= 1000000000;
	    }
	    assert(tm.tv_nsec < 1000000000);
	    if (tm.tv_sec >= tv.tv_sec) {
		/* no overflow */
		return pthread_cond_timedwait(&c->c, &m->m, &tm);
	    }
	}
    }
    /* timeout is 0 or overflow */
    return pthread_cond_wait(&c->c, &m->m);
}

void
POSIXcondvarNotify(POSIXCondVar * c)
{
    pthread_cond_signal(&c->c);
}

void
POSIXcondvarNotifyAll(POSIXCondVar * c)
{
    pthread_cond_broadcast(&c->c);
}
