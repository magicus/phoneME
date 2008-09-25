/*
 *  
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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
 * This source file is specific for Qt-based configurations.
 */

#include <stdio.h>
#include <values.h>

#include <kni.h>
#include <jvm.h>
#include <jvmspi.h>
#include <sni.h>
#include <java_types.h>

#include <midpTimerImpl.h>
#include <moc_midpTimerImpl.cpp>

#include <midpServices.h>
#include <timer_export.h>
#include <midp_thread.h>
#include <push_server_export.h>

/**
 * @file
 *
 * Implementation of a Timer.
 */

static JVMSPI_ThreadID
findBlockedHandle(int handle) {
    JVMSPI_BlockedThreadInfo *blocked_threads;
    int n;
    blocked_threads = SNI_GetBlockedThreads(&n);
  
    for (int i = 0; i < n; i++) {
        MidpReentryData *p = 
            static_cast<MidpReentryData*>(blocked_threads[i].reentry_data);
        if (p == NULL) {
            continue;
        }

        if ((p->waitingFor == PUSH_SIGNAL) &&
            (findPushTimerBlockedHandle(handle) != 0)) {

            return blocked_threads[i].thread_id;
        }

    }
  
    return 0;
}


static void
findAndUnblockHandle(int handle) {
    JVMSPI_ThreadID id = findBlockedHandle(handle);
    if (id != 0) {
        midp_thread_unblock(id);
    }
}


VMTimer::VMTimer(int alarmHandle, QObject *parent, const char* name):
    QObject(parent, name)
{
     alarm = static_cast<AlarmEntry*>((void*)alarmHandle);
     ASSERT (alarm != NULL);
     timer = new QTimer(this);
     ASSERT (timer != NULL);
     connect(timer, SIGNAL(timeout()), SLOT(timerDone()));
}
 

void VMTimer::startTimer(int time)
{
  ASSERT (timer != NULL);
  if (timer != NULL) {
	/* start one shot timer*/
	timer->start(time, TRUE);
  }
}


void VMTimer::timerDone()
{
    findAndUnblockHandle((int)this);
}


extern "C" int
createTimerHandle(int alarmHandle, jlong time) {
    VMTimer* timer = new VMTimer(alarmHandle);

    if (time > MAXINT) {
	time = MAXINT;
    }

    timer->startTimer(time);

    return (int)timer;
}

extern "C" int
destroyTimerHandle(int timerHandle) {
    VMTimer *timer = static_cast<VMTimer *>((void*)timerHandle);
    if (timer != NULL) {
        delete timer;
    }

    return 0;
}

