/*
 * @(#)ThreadLocking.c	1.8 06/10/10
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

#include <pthread.h>
#include <gdk/gdk.h>
#include <assert.h>
#include "ThreadLocking.h"

static int lockCount = 0;
static pthread_t lockOwner = 0;

/* Define this if you need to debug calls to awt_gtk_threadsEnter. If you are locking more times than
   unlocking then the debug version will diplay where the last lock was called from for which a subsequent
   unlock was never called. */

#ifdef AWT_GTK_THREAD_LOCKING_DEBUG

static int lineNum = 0;	/* Line number of last lock. */
static const char *fileName = "No file"; /* File name of last lock. */

void
awt_gtk_threadsEnterDebug (const char *_fileName, int _lineNum)
{
	pthread_t self = pthread_self();
	
	if (self != lockOwner)
	{
		printf ("Waiting for lock currently owned by %s %d\n", fileName, lineNum);
		gdk_threads_enter();
		printf ("Lock released by %s %d\n", fileName, lineNum);
		lockOwner = self;
		lockCount = 1;
		fileName = _fileName;
		lineNum = _lineNum;
	}
	
	else lockCount++;
}

void
awt_gtk_threadsLeaveDebug(void)
{
	pthread_t self = pthread_self();
	
	assert (self == lockOwner);
	
	lockCount--;
	
	assert (lockCount >= 0);
	
	if (lockCount == 0)
	{
		lockOwner = 0;
		lineNum = 0;
		fileName = "No file";
		gdk_threads_leave();
	}
}

#else

void
awt_gtk_threadsEnter(void)
{
	pthread_t self = pthread_self();
	
	if (self != lockOwner)
	{
		gdk_threads_enter();
		lockOwner = self;
		lockCount = 1;
	}
	
	else lockCount++;
}

void
awt_gtk_threadsLeave(void)
{
	pthread_t self = pthread_self();
	
	assert (self == lockOwner);
	
	lockCount--;
	
	assert (lockCount >= 0);
	
	if (lockCount == 0)
	{
		lockOwner = 0;
		gdk_threads_leave();
	}
}

#endif

void
awt_gtk_callbackEnter(void)
{
	if (lockOwner == 0)
		lockOwner = pthread_self();
		
	assert (lockOwner == pthread_self());
		
	lockCount++;
}

void
awt_gtk_callbackLeave(void)
{
	lockCount--;
	
	assert (lockCount >= 0);
	
	if (lockCount == 0)
		lockOwner = 0;
}
