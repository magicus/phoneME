/*
 * @(#)popFrames.c	1.8 06/10/10
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
 */
#include <string.h>
#include <stdlib.h>
#include <jni.h>
#include <jvmdi.h>

#include "JDWP.h"
#include "popFrames.h"
#include "util.h"
#include "threadControl.h"
#include "eventHandler.h"

/*
 * popFrameLock is used to notify that the event has been received 
 */
static JVMDI_RawMonitor popFrameLock = NULL;

/*
 * popFrameSuspendLock is used to assure that the event thread is
 * re-suspended immediately after the event is acknowledged.
 */
static JVMDI_RawMonitor popFrameSuspendLock = NULL;

static void
initLocks()
{
    if (popFrameLock == NULL) {
        popFrameLock = debugMonitorCreate("JDWP PopFrame Lock");
        popFrameSuspendLock = debugMonitorCreate("JDWP PopFrame Suspend Lock");
    }
}

/**
 * special event handler for events on the popped thread
 * that occur during the pop operation.
 */
static jboolean
eventDuringPop(JVMDI_Event *event, jthread thread)
{
    switch (event->kind) {
        
    case JVMDI_EVENT_SINGLE_STEP:
      /* this is an event we requested to mark the */
      /* completion of the pop frame */

      debugMonitorEnter(popFrameSuspendLock);
      {
          /* notify that we got the event */
          debugMonitorEnter(popFrameLock);
          {
              debugMonitorNotify(popFrameLock);
          }
          debugMonitorExit(popFrameLock);

          /* make sure we get suspended again */
          debugMonitorWait(popFrameSuspendLock);
      }
      debugMonitorExit(popFrameSuspendLock);

      /* ignored by eventHandler */
      return JNI_TRUE;  


    case JVMDI_EVENT_BREAKPOINT:
    case JVMDI_EVENT_EXCEPTION:
    case JVMDI_EVENT_FIELD_ACCESS:
    case JVMDI_EVENT_FIELD_MODIFICATION:
    case JVMDI_EVENT_METHOD_ENTRY:
    case JVMDI_EVENT_METHOD_EXIT:
    case JVMDI_EVENT_THREAD_START:
    case JVMDI_EVENT_THREAD_END:
      /* ignored by us and eventHandler */
      return JNI_TRUE;  

    case JVMDI_EVENT_USER_DEFINED: 
    case JVMDI_EVENT_VM_DEATH: 
        ERROR_MESSAGE_EXIT(
             "Thread-less events should not come here\n");

    case JVMDI_EVENT_CLASS_UNLOAD: 
    case JVMDI_EVENT_CLASS_LOAD: 
    case JVMDI_EVENT_CLASS_PREPARE: 
    default:
        /* we don't handle these, don't consume them */
        return JNI_FALSE; 
    }
}

/**
 * Pop one frame off the stack of thread.
 * popFrameLock is already held
 */
static jvmdiError
popOneFrame(jthread thread)
{
    jvmdiError error;

    error = jvmdi->PopFrame(thread);
    if (error != JVMDI_ERROR_NONE) {
        return error;
    }
    
    /* resume the popped thread so that the pop occurs and so we */
    /* will get the event (step or method entry) after the pop */
    error = jvmdi->ResumeThread(thread);
    if (error != JVMDI_ERROR_NONE) {
        return error;
    }

    /* wait for the event to occur */
    debugMonitorWait(popFrameLock);

    /* make sure not to suspend until the popped thread is on the wait */
    debugMonitorEnter(popFrameSuspendLock);
    {
        /* return popped thread to suspended state */
        error = jvmdi->SuspendThread(thread);

        /* notify popped thread that we are suspended again, so */
        /* that it can proceed when resumed */
        debugMonitorNotify(popFrameSuspendLock);
    }
    debugMonitorExit(popFrameSuspendLock);

    return error;
}

/**
 * Return the number of frames that would need to be popped
 * to pop the specified frame.  Negative for error.
 */
#define RETURN_ON_ERROR(call)                     \
    do {                                          \
        if ((jvmdi->call) != JVMDI_ERROR_NONE) {  \
            return -1;                            \
        }                                         \
    } while (0)

static jint
computeFramesToPop(jthread thread, jframeID frame)
{
    jframeID candidate;
    jint popCount = 1;

    RETURN_ON_ERROR(GetCurrentFrame(thread, &candidate));
    while (frame != candidate) {
        jclass clazz;
        jmethodID method;
        jlocation location;
        jboolean isNative;

        RETURN_ON_ERROR(GetCallerFrame(candidate, &candidate));
        RETURN_ON_ERROR(GetFrameLocation(candidate, 
                                         &clazz, &method, &location));
        RETURN_ON_ERROR(IsMethodNative(clazz, method, &isNative));
        /* native methods will be popped automatically so don't count them */
        if (!isNative) {
            ++popCount;
        }
    }
    return popCount;
}

/**
 * pop frames of the stack of 'thread' until 'frame' is popped.
 */
jvmdiError
popFrames_pop(jthread thread, jframeID frame)
{
    jvmdiError error = JVMDI_ERROR_NONE;
    jint prevStepMode;
    jint framesPopped = 0;
    jint popCount;

    initLocks();

    /* enable instruction level single step, but first note prev value */
    prevStepMode = threadControl_getInstructionStepMode(thread);
    error = threadControl_setEventMode(JVMDI_ENABLE, 
                                       JVMDI_EVENT_SINGLE_STEP, thread);
    if (error != JVMDI_ERROR_NONE) {
        return error;
    }

    /* compute the number of frames to pop */
    popCount = computeFramesToPop(thread, frame);
    if (popCount < 1) {
        return JVMDI_ERROR_INVALID_FRAMEID;
    }

    /* redirect all events on this thread to eventDuringPop() */
    redirectedEventThread = thread;
    redirectedEventFunction = eventDuringPop;

    debugMonitorEnter(popFrameLock);

    /* pop frames using single step */
    while (framesPopped++ < popCount) {
        error = popOneFrame(thread);
        if (error != JVMDI_ERROR_NONE) {
            break;
        }
    }

    debugMonitorExit(popFrameLock);

    /* restore state */
    threadControl_setEventMode(prevStepMode, 
                               JVMDI_EVENT_SINGLE_STEP, thread);
    redirectedEventThread = NULL;
    redirectedEventFunction = NULL;

    return error;
}
