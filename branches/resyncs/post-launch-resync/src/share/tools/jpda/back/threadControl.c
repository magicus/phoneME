/*
 * @(#)threadControl.c	1.61 06/10/10
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
 */
#include <stdlib.h>
#include <string.h> /* for memset */

#include <jvmdi.h>
#include "eventHandler.h"
#include "threadControl.h"
#include "util.h"
#include "transport.h"
#include "commonRef.h"
#include "eventHelper.h"
#include "stepControl.h"
#include "invoker.h"
#include "bag.h"
#include "version.h"

#define NO_EVENT -1
#define HANDLING_EVENT(node) ((node)->currentEventKind != NO_EVENT)

/**
 * The main data structure in threadControl is the ThreadNode. 
 * This is a per-thread structure that is allocated on the 
 * first event that occurs in a thread. It is freed after the 
 * thread's thread end event has completed processing. The 
 * structure contains state information on its thread including 
 * suspend counts. It also acts as a repository for other 
 * per-thread state such as the current method invocation or 
 * current step. 
 */
typedef struct ThreadNode {
    jthread thread;
    JNIEnv *env;
    unsigned int toBeResumed : 1;
    unsigned int pendingInterrupt : 1;
    unsigned int isDebugThread : 1;
    unsigned int suspendOnStart : 1;
    unsigned int isStarted : 1;
    jint currentEventKind;
    jobject pendingStop;
    jint suspendCount;
    jint resumeFrameDepth; /* !=0 => This thread is in a call to Thread.resume() */
    jint instructionStepMode;
    StepRequest currentStep;
    InvokeRequest currentInvoke;
    struct bag *eventBag;
    struct ThreadNode *next;
} ThreadNode;

jint suspendAllCount;

typedef struct ThreadList {
    ThreadNode *first;
} ThreadList;

static JVMDI_RawMonitor threadLock;
#ifdef THREAD_RESUME_EXISTS
static jclass threadClass;
static jmethodID resumeMethod;
static jlocation resumeLocation;
#endif
static HandlerNode *breakpointHandlerNode;
static HandlerNode *framePopHandlerNode;
static HandlerNode *catchHandlerNode;

/*
 * Threads which have issued thread start events and not yet issued thread
 * end events are maintained in the "runningThreads" list. All other threads known
 * to this module are kept in the "otherThreads" list.
 */
static ThreadList runningThreads;
static ThreadList otherThreads;

#define MAX_DEBUG_THREADS 10
static int debugThreadCount;
static jthread debugThreads[MAX_DEBUG_THREADS];

typedef struct DeferredEventMode {
    jint event;
    jint mode;
    jthread thread;
    struct DeferredEventMode *next;
} DeferredEventMode;
                                                            
typedef struct {
    DeferredEventMode *first;
    DeferredEventMode *last;
} DeferredEventModeList;

DeferredEventModeList deferredEventModes;

#ifdef THREAD_RESUME_EXISTS
static jint
getStackDepth(jthread thread)
{
    jint count;
    jint error = frameCount(thread, &count);
    if (error != JVMDI_ERROR_NONE) {
        ERROR_CODE_EXIT(error);
    }
    return count;
}
#endif /* THREAD_RESUME_EXISTS */

static jint 
threadStatus(jthread thread, jint *suspendStatus)
{
    jint threadStatus;
    jint error = jvmdi->GetThreadStatus(thread, &threadStatus, 
                                                suspendStatus);
    if (error != JVMDI_ERROR_NONE) {
        ERROR_CODE_EXIT(error);
    }
    return threadStatus;
}

/*
 * These functions maintain the linked list of currently running threads. All assume
 * that the threadLock is held before calling.
 */
static ThreadNode *
findThread(JNIEnv *env, ThreadList *list, jthread thread) 
{
    ThreadNode *node = list->first;  
    while (node != NULL) {
        if ((*env)->IsSameObject(env, node->thread, thread)) {
            return node;
        }
        node = node->next;
    }
    return NULL;
}

static ThreadNode *
findThreadByEnv(JNIEnv *env, ThreadList *list) 
{
    ThreadNode *node = list->first;  
    while (node != NULL) {
        if (node->env == env) {
            return node;
        }
        node = node->next;
    }
    return NULL;
}

static ThreadNode *
findAnyThread(JNIEnv *env, jthread thread)
{
    ThreadNode *node = findThread(env, &runningThreads, thread);
    if (node == NULL) {
        node = findThread(env, &otherThreads, thread);
    }
    return node;
}

static ThreadNode *
insertThread(JNIEnv *env, ThreadList *list, jthread thread) 
{
    ThreadNode *node;
    struct bag *eventBag;

    node = findThread(env, list, thread);
    if (node == NULL) {
        node = jdwpAlloc(sizeof(*node));
        eventBag = eventHelper_createEventBag();
        if ((node == NULL) || (eventBag == NULL)) {
            jdwpFree(node);
            bagDestroyBag(eventBag);
            return NULL;
        }

        /*
         * Init all flags false, all refs NULL, all counts 0 
         */
        memset(node, 0, sizeof(*node));

        node->thread = (*env)->NewGlobalRef(env, thread);
        if (node->thread == NULL) {
            jdwpFree(node);
            bagDestroyBag(eventBag);
            return NULL;
        }
        /*
         * Remember if it is a debug thread
         */
        if (threadControl_isDebugThread(thread)) {
            node->isDebugThread = JNI_TRUE;
        } else if (suspendAllCount > 0){
            /*
             * If there is a pending suspendAll, all new threads should
             * be initialized as if they were suspended by the suspendAll,
             * and the thread will need to be suspended when it starts.
             */
            node->suspendCount = suspendAllCount;
            node->suspendOnStart = JNI_TRUE;
        }
        node->currentEventKind = NO_EVENT;
        node->instructionStepMode = JVMDI_DISABLE;
        node->eventBag = eventBag;
        node->next = list->first;
        list->first = node;
    }

    return node;
}

static void 
clearThread(JNIEnv *env, ThreadNode *node)
{
    if (node->pendingStop != NULL) {
        (*env)->DeleteGlobalRef(env, node->pendingStop);
    }
    stepControl_clearRequest(node->thread, &node->currentStep);
    if (node->isDebugThread) {
        threadControl_removeDebugThread(node->thread);
    }
    (*env)->DeleteGlobalRef(env, node->thread);
    bagDestroyBag(node->eventBag);
    jdwpFree(node);
}

static ThreadNode *
removeNode(JNIEnv *env, ThreadList *list, jthread thread)
{
    ThreadNode *previous = NULL;
    ThreadNode *node = list->first; 
    while (node != NULL) {
        if ((*env)->IsSameObject(env, node->thread, thread)) {
            if (previous == NULL) {
                list->first = node->next;
            } else {
                previous->next = node->next;
            }
            return node;
        }
        previous = node;
        node = node->next;
    }
    return NULL;
}

static jboolean 
removeThread(JNIEnv *env, ThreadList *list, jthread thread) 
{
    ThreadNode *node = removeNode(env, list, thread);
    if (node != NULL) {
        clearThread(env, node);
        return JNI_TRUE;
    } else {
        return JNI_FALSE;
    }
}

static void
removeResumed(JNIEnv *env, ThreadList *list)
{
    ThreadNode *node = list->first;  
    while (node != NULL) {
        ThreadNode *temp = node->next;
        if (node->suspendCount == 0) {
            removeThread(env, list, node->thread);
        }
        node = temp;
    }
}

static jboolean 
moveThread(JNIEnv *env, ThreadList *source, ThreadList *dest, jthread thread) 
{
    ThreadNode *node = removeNode(env, source, thread);
    if (node != NULL) {
        JDI_ASSERT(findThread(env, dest, thread) == NULL);
        node->next = dest->first;
        dest->first = node;
        return JNI_TRUE;
    } else {
        return JNI_FALSE;
    }
}

typedef jint (*ThreadEnumerateFunction)(JNIEnv *, ThreadNode *, void *);

static jint
enumerateOverThreadList(JNIEnv *env, ThreadList *list, 
                        ThreadEnumerateFunction function, void *arg)
{
    ThreadNode *node = list->first; 
    jint error = JVMDI_ERROR_NONE;
    for (node = list->first, error = JVMDI_ERROR_NONE;
         (node != NULL) && (error == JVMDI_ERROR_NONE);
         node = node->next) {
        error = (*function)(env, node, arg);
    }
    return error;
}

static void 
insertEventMode(DeferredEventModeList *list, DeferredEventMode *eventMode)
{
    if (list->last != NULL) {
        list->last->next = eventMode;
    } else {
        list->first = eventMode;
    }
    list->last = eventMode;
}

static void 
removeEventMode(DeferredEventModeList *list, DeferredEventMode *eventMode, DeferredEventMode *prev)
{
    if (prev == NULL) {
        list->first = eventMode->next;
    } else {
        prev->next = eventMode->next;
    }
    if (eventMode->next == NULL) {
        list->last = prev;
    }
}

static jint
addDeferredEventMode(JNIEnv *env, jint mode, jint event, jthread thread)
{
    DeferredEventMode *eventMode = jdwpAlloc(sizeof(DeferredEventMode));
    if (eventMode == NULL) {
        return JVMDI_ERROR_OUT_OF_MEMORY;
    }
    thread = (*env)->NewGlobalRef(env, thread);
    if (thread == NULL) {
        jdwpFree(eventMode);
        return JVMDI_ERROR_OUT_OF_MEMORY;
    }

    eventMode->thread = thread;
    eventMode->mode = mode;
    eventMode->event = event;
    eventMode->next = NULL;
    insertEventMode(&deferredEventModes, eventMode);
    return JVMDI_ERROR_NONE;
}

static void
freeDeferredEventModes(JNIEnv *env)
{
    DeferredEventMode *eventMode = deferredEventModes.first;
    while (eventMode != NULL) {
        DeferredEventMode *next = eventMode->next;
        (*env)->DeleteGlobalRef(env, eventMode->thread);
        jdwpFree(eventMode);
        eventMode = next;
    }
    deferredEventModes.first = NULL;
    deferredEventModes.last = NULL;
}

static void
processDeferredEventModes(JNIEnv *env, jthread thread, ThreadNode *node) 
{
    jint error;
    DeferredEventMode *eventMode;
    DeferredEventMode *prev;
    
    prev = NULL;
    eventMode = deferredEventModes.first;
    while (eventMode != NULL) {
        DeferredEventMode *next = eventMode->next;
        if ((*env)->IsSameObject(env, thread, eventMode->thread)) {
            if (eventMode->event == JVMDI_EVENT_SINGLE_STEP) {
                node->instructionStepMode = eventMode->mode;
            }
            error = jvmdi->SetEventNotificationMode(eventMode->mode,
                                                    eventMode->event,
                                                    eventMode->thread);
            if (error != JVMDI_ERROR_NONE) {
                ERROR_CODE_EXIT(error);
            }
            removeEventMode(&deferredEventModes, eventMode, prev);
            (*env)->DeleteGlobalRef(env, eventMode->thread);
            jdwpFree(eventMode);
        } else {
            prev = eventMode;
        }
        eventMode = next;
    }
}

static void 
getLocks() 
{
    /*
     * Anything which might be locked as part of the handling of 
     * a JVMDI event (which means: might be locked by an application
     * thread) needs to be grabbed here. This allows thread control
     * code to safely suspend and resume the application threads  
     * while ensuring they don't hold a critical lock.
     */

    eventHandler_lock();
    invoker_lock();
    eventHelper_lock();
    stepControl_lock();
    commonRef_lock();
    debugMonitorEnter(threadLock);
    util_lock();

}

static void 
releaseLocks() 
{
    util_unlock();
    debugMonitorExit(threadLock);
    commonRef_unlock();
    stepControl_unlock();
    eventHelper_unlock();
    invoker_unlock();
    eventHandler_unlock();
}

void 
threadControl_initialize() 
{
#ifdef THREAD_RESUME_EXISTS
    JNIEnv *env = getEnv();
    jlocation unused;
    jclass localThreadClass;
    jint error;
#endif

    suspendAllCount = 0;
    runningThreads.first = NULL;
    otherThreads.first = NULL;
    debugThreadCount = 0;
    threadLock = debugMonitorCreate("JDWP Thread Lock");
#ifdef THREAD_RESUME_EXISTS
    localThreadClass = (*env)->FindClass(env, "java/lang/Thread");
    if ((localThreadClass == NULL) || (*env)->ExceptionOccurred(env)) {
        ERROR_MESSAGE_EXIT("Can't find java.lang.Thread");
    }
    threadClass = (*env)->NewGlobalRef(env, localThreadClass);
    if ((threadClass == NULL) || (*env)->ExceptionOccurred(env))  {
        ERROR_MESSAGE_EXIT("Unable to create global ref");
    }
    resumeMethod = (*env)->GetMethodID(env, threadClass, 
                                       "resume", "()V");
    if ((resumeMethod == NULL) || (*env)->ExceptionOccurred(env)) {
        ERROR_MESSAGE_EXIT("Can't find java.lang.Thread.resume()");
    }
    error = jvmdi->GetMethodLocation(threadClass, resumeMethod, 
                                     &resumeLocation, &unused);
    if (error != JVMDI_ERROR_NONE) {
        ERROR_CODE_EXIT(error);
    }
#endif

    deferredEventModes.first = NULL;
    deferredEventModes.last = NULL;
}

#ifdef THREAD_RESUME_EXISTS
static jthread
getResumee(jthread resumingThread, jframeID *retFrame)
{
    jframeID frame;
    jthread resumee = NULL;
    jint error = jvmdi->GetCurrentFrame(resumingThread, &frame);
    if (error == JVMDI_ERROR_NONE) {
        jobject object;
        error = jvmdi->GetLocalObject(frame, 0, &object);
        if (error == JVMDI_ERROR_NONE) {
            resumee = object;
            *retFrame = frame;
        }
    }
    return resumee;
}
#endif

static jboolean
pendingAppResume(jboolean includeSuspended) 
{
    ThreadList *list = &runningThreads;
    ThreadNode *node = list->first;  
    while (node != NULL) {
        if (node->resumeFrameDepth > 0) {
            if (includeSuspended) {
                return JNI_TRUE;
            } else {
                jint suspendStatus;
                threadStatus(node->thread, &suspendStatus);
                if (~(suspendStatus & JVMDI_SUSPEND_STATUS_SUSPENDED)) {
                    return JNI_TRUE;
                }
            }
        }
        node = node->next;
    }
    return JNI_FALSE;
}

static void
notifyAppResumeComplete(void)
{
    debugMonitorNotifyAll(threadLock);
    if (!pendingAppResume(JNI_TRUE)) {
        if (framePopHandlerNode != NULL) {
            eventHandler_free(framePopHandlerNode);
            framePopHandlerNode = NULL;
        }
        if (catchHandlerNode != NULL) {
            eventHandler_free(catchHandlerNode);
            catchHandlerNode = NULL;
        }
    }
}

#ifdef THREAD_RESUME_EXISTS
static void 
handleAppResumeCompletion(JNIEnv *env, JVMDI_Event *event, 
                          HandlerNode *handlerNode,
                          struct bag *eventBag)
{
    ThreadNode *node;
    jthread thread = event->u.frame.thread;

    debugMonitorEnter(threadLock);

    node = findThread(env, &runningThreads, thread);

    if (node != NULL) {
        if (node->resumeFrameDepth > 0) {
            jint compareDepth = getStackDepth(thread);
            if (event->kind == JVMDI_EVENT_FRAME_POP) {
                compareDepth--;
            } 
            if (compareDepth < node->resumeFrameDepth) {
                node->resumeFrameDepth = 0;
                notifyAppResumeComplete();
            }
        }
    }

    debugMonitorExit(threadLock);
}

static void
blockOnDebuggerSuspend(JNIEnv *env, jthread thread)
{
    ThreadNode *node = findAnyThread(env, thread);
    if (node != NULL) {
        while (node && node->suspendCount > 0) {
            debugMonitorWait(threadLock);
            node = findAnyThread(env, thread);
        }
    }
}

static void
trackAppResume(JNIEnv *env, jthread thread, jframeID frame)
{
    jint error;
    ThreadNode *node = findThread(env, &runningThreads, thread);
    if (node != NULL) {
        JDI_ASSERT(node->resumeFrameDepth == 0);
        error = jvmdi->NotifyFramePop(frame);
        if (error == JVMDI_ERROR_NONE) {
            jint frameDepth = getStackDepth(thread);
            if ((frameDepth > 0) && (framePopHandlerNode == NULL)) {
                framePopHandlerNode = eventHandler_createInternalThreadOnly(
                                           JVMDI_EVENT_FRAME_POP, 
                                           handleAppResumeCompletion,
                                           thread);
                catchHandlerNode = eventHandler_createInternalThreadOnly(
                                           JVMDI_EVENT_EXCEPTION_CATCH,
                                           handleAppResumeCompletion,
                                           thread);
                if ((framePopHandlerNode == NULL) || 
                    (catchHandlerNode == NULL)) {
                    eventHandler_free(framePopHandlerNode);
                    framePopHandlerNode = NULL;
                    eventHandler_free(catchHandlerNode);
                    catchHandlerNode = NULL;
                }
            }
            if ((framePopHandlerNode != NULL) && 
                (catchHandlerNode != NULL) &&
                (frameDepth > 0)) {
                node->resumeFrameDepth = frameDepth;
            }
        }
    }
}

static void 
handleAppResumeBreakpoint(JNIEnv *env, JVMDI_Event *event, 
                          HandlerNode *handlerNode,
                          struct bag *eventBag)
{
    jframeID frame;
    jthread resumer = event->u.breakpoint.thread;
    jthread resumee = getResumee(resumer, &frame);

    debugMonitorEnter(threadLock);
    if (resumee != NULL) {
        /*
         * Hold up any attempt to resume as long as the debugger 
         * has suspended the resumee.
         */
        blockOnDebuggerSuspend(env, resumee);
    }

    if (resumer != NULL) {
        /*
         * Track the resuming thread by marking it as being within
         * a resume and by setting up for notification on 
         * a frame pop or exception. We won't allow the debugger
         * to suspend threads while any thread is within a 
         * call to resume. This (along with the block above)
         * ensures that when the debugger 
         * suspends a thread it will remain suspended.
         */
        trackAppResume(env, resumer, frame);
    }

    debugMonitorExit(threadLock);
}
#endif /* THREAD_RESUME_EXISTS */

void
threadControl_onConnect()
{
#ifdef THREAD_RESUME_EXISTS
    breakpointHandlerNode = eventHandler_createInternalBreakpoint(
                                 handleAppResumeBreakpoint, NULL,
                                 threadClass, resumeMethod, resumeLocation);
#endif
}

void
threadControl_onDisconnect()
{
    if (breakpointHandlerNode != NULL) {
        eventHandler_free(breakpointHandlerNode);
        breakpointHandlerNode = NULL;
    }
    if (framePopHandlerNode != NULL) {
        eventHandler_free(framePopHandlerNode);
        framePopHandlerNode = NULL;
    }
    if (catchHandlerNode != NULL) {
        eventHandler_free(catchHandlerNode);
        catchHandlerNode = NULL;
    }
}

void 
threadControl_onHook() 
{
    /*
     * As soon as the event hook is in place, we need to initialize 
     * the thread list with already-existing threads. The threadLock
     * has been held since initialize, so we don't need to worry about
     * insertions or deletions from the event handlers while we do this
     */
    jint threadCount;
    jthread *threads;
    int i;
    ThreadNode *node;
    JNIEnv *env = getEnv();

    /*
     * Prevent any event processing until OnHook has been called
     */
    debugMonitorEnter(threadLock);

    threads = allThreads(&threadCount);
    if (threads == NULL) {
        ALLOC_ERROR_EXIT();
    } else {
        for (i = 0; i < threadCount; i++) {
            jthread thread = threads[i];
            node = insertThread(env, &runningThreads, thread);
            if (node == NULL) {
                ERROR_MESSAGE_EXIT("Unable to create thread table entry");
            } 

            /*
             * This is a tiny bit risky. We have to assume that the 
             * pre-existing threads have been started because we
             * can't rely on a thread start event for them. The chances
             * of a problem related to this are pretty slim though, and
             * there's really no choice because without setting this flag
             * there is no way to enable stepping and other events on 
             * the threads that already exist (e.g. the finalizer thread).
             */
            node->isStarted = JNI_TRUE;
        }
        freeGlobalRefs(threads, threadCount);
    }

    debugMonitorExit(threadLock);
}

static jint
commonSuspendByNode(ThreadNode *node)
{
    jint error = jvmdi->SuspendThread(node->thread);

    /*
     * Mark for resume only if suspend succeeded
     */
    if (error == JVMDI_ERROR_NONE) {
        node->toBeResumed = JNI_TRUE;
    }

    /*
     * If the thread was suspended by another app thread, 
     * do nothing and report no error (we won't resume it later).
     */
     if (error == JVMDI_ERROR_THREAD_SUSPENDED) {
        error = JVMDI_ERROR_NONE;
     }

     return error;
}

/*
 * Deferred suspends happen when the suspend is attempted on a thread
 * that is not yet valid at the JVMDI level. Bookkeeping (suspendCount,etc.)
 * is handled by the original request, and once the thread actually
 * starts, a deferred suspend is attempted. This function does the
 * deferred suspend without changing the bookkeeping that is already
 * in place. 
 */
static jint 
deferredSuspendThreadByNode(ThreadNode *node) 
{
    jint error = JVMDI_ERROR_NONE;
    if (node->isDebugThread) {
        /* Ignore requests for suspending debugger threads */
        return JVMDI_ERROR_NONE;
    } 

    /*
     * Do the actual suspend only if a subsequent resume hasn't
     * made it irrelevant.
     */
    if (node->suspendCount > 0) {
        error = commonSuspendByNode(node);
        
        /*
         * Attempt to clean up from any error by decrementing the 
         * suspend count. This compensates for the increment that 
         * happens when suspendOnStart is set to true.
         */
        if (error != JVMDI_ERROR_NONE) {
          node->suspendCount--;
        }
    } 

    node->suspendOnStart = JNI_FALSE;

    debugMonitorNotifyAll(threadLock);

    return error;
}

static jint 
suspendThreadByNode(ThreadNode *node) 
{
    jint error = JVMDI_ERROR_NONE;
    if (node->isDebugThread) {
        /* Ignore requests for suspending debugger threads */
        return JVMDI_ERROR_NONE;
    } 

    /*
     * Just increment the suspend count if we are waiting
     * for a deferred suspend. 
     */
    if (node->suspendOnStart) {
        node->suspendCount++;
        return JVMDI_ERROR_NONE;
    }

    if (node->suspendCount == 0) {
        error = commonSuspendByNode(node);

        if (error == JVMDI_ERROR_INVALID_THREAD) {
            /*
             * This error means that the thread is either a zombie or not yet
             * started. In either case, we ignore the error. If the thread 
             * is a zombie, suspend/resume are no-ops. If the thread is not
             * started, it will be suspended for real during the processing
             * of its thread start event.
             */
            node->suspendOnStart = JNI_TRUE;
            error = JVMDI_ERROR_NONE;
        }
    } 

    if (error == JVMDI_ERROR_NONE) {
        node->suspendCount++;
    }

    debugMonitorNotifyAll(threadLock);

    return error;
}

static jint 
resumeThreadByNode(ThreadNode *node) 
{
    jint error = JVMDI_ERROR_NONE;

    if (node->isDebugThread) {
        /* never suspended by debugger => don't ever try to resume */
        return JVMDI_ERROR_NONE;
    }
    if (node->suspendCount > 0) {
        node->suspendCount--;
        debugMonitorNotifyAll(threadLock);
        if ((node->suspendCount == 0) && node->toBeResumed && 
            !node->suspendOnStart) {
            error = jvmdi->ResumeThread(node->thread);
            node->toBeResumed = JNI_FALSE;
        }
    }

    return error;
}

/*
 * Functions which respond to user requests to suspend/resume 
 * threads. 
 * Suspends and resumes add and subtract from a count respectively. 
 * The thread is only suspended when the count goes from 0 to 1 and 
 * resumed only when the count goes from 1 to 0.
 *
 * These functions suspend and resume application threads 
 * without changing the 
 * state of threads that were already suspended beforehand. 
 * They must not be called from an application thread because 
 * that thread may be suspended somewhere in the  middle of things.
 */
static void 
preSuspend(void)
{
    getLocks();                     /* Avoid debugger deadlocks */

    /*
     * Delay any suspend while a call to java.lang.Thread.resume is in 
     * progress (not including those in suspended threads). The wait is 
     * timed because the threads suspended through 
     * java.lang.Thread.suspend won't result in a notify even though 
     * it may change the result of pendingAppResume()
     */
    while (pendingAppResume(JNI_FALSE)) {
        /*
         * This is ugly but we need to release the locks from getLocks
         * or else the notify will never happen. The locks must be 
         * released and reacquired in the right order. else deadlocks
         * can happen. It is possible that, during this dance, the 
         * notify will be missed, but since the wait needs to be timed
         * anyway, it won't be a disaster. Note that this code will 
         * execute only on very rare occasions anyway.
         */
        releaseLocks();

        debugMonitorEnter(threadLock);
        debugMonitorTimedWait(threadLock, 1000);
        debugMonitorExit(threadLock);

        getLocks();
    }
}

static void
postSuspend(void)
{
    releaseLocks();
}

/*
 * This function must be called after preSuspend and before postSuspend.
 */
static jint 
commonSuspend(jthread thread, jboolean deferred) 
{
    JNIEnv *env = getEnv();

    /*
     * If the thread is not between its start and end events, we should
     * still suspend it. To keep track of things, add the thread
     * to a separate list of threads so that we'll resume it later. 
     */
    ThreadNode *node = findThread(env, &runningThreads, thread);
    if (node == NULL) {
        node = insertThread(env, &otherThreads, thread);
        if (node == NULL) {
            ERROR_MESSAGE_EXIT("Unable to create thread table entry");
        }
    }
    
    if (deferred) {
        return deferredSuspendThreadByNode(node);
    } else {
        return suspendThreadByNode(node);
    }
}

static jint 
commonResume(JNIEnv *env, jthread thread) 
{
    jint error = JVMDI_ERROR_NONE;
    ThreadNode *node;
    /*
     * The thread is normally between its start and end events, but if
     * not, check the auxiliary list used by threadControl_suspendThread.
     */
    node = findAnyThread(env, thread);

    /*
     * If the node is in neither list, the debugger never suspended
     * this thread, so do nothing. 
     */
    if (node != NULL) {
        error = resumeThreadByNode(node);
    }
    return error;
}


jint 
threadControl_suspendThread(jthread thread, jboolean deferred) 
{
    jint error;

    preSuspend();
    error = commonSuspend(thread, deferred);
    postSuspend();

    return error;
}

jint 
threadControl_resumeThread(jthread thread, jboolean do_unblock) 
{
    jint error;
    JNIEnv *env = getEnv();

    eventHandler_lock(); /* for proper lock order */
    debugMonitorEnter(threadLock);
    error = commonResume(env, thread);
    removeResumed(env, &otherThreads);
    debugMonitorExit(threadLock);
    eventHandler_unlock();

    if (do_unblock) {
        /* let eventHelper.c: commandLoop() know we resumed one thread */
        unblockCommandLoop();
    }

    return error;
}

jint 
threadControl_suspendCount(jthread thread, jint *count)
{
    jint error = JVMDI_ERROR_NONE;
    ThreadNode *node;
    JNIEnv *env = getEnv();

    debugMonitorEnter(threadLock);

    node = findThread(env, &runningThreads, thread);
    if (node == NULL) {
        node = findThread(env, &otherThreads, thread);
    }

    if (node != NULL) {
        *count = node->suspendCount;
    } else {
        /*
         * If the node is in neither list, the debugger never suspended
         * this thread, so the suspend count is 0. 
         */
        *count = 0;
    }

    debugMonitorExit(threadLock);

    return error;
}

static jboolean
contains(JNIEnv *env, jthread *list, jint count, jthread item) 
{
    int i;
    for (i = 0; i < count; i++) {
        if ((*env)->IsSameObject(env, list[i], item)) {
            return JNI_TRUE;
        }
    }
    return JNI_FALSE;
}


typedef struct {
    jthread *list;
    jint count;
} SuspendAllArg;

static jint
suspendAllHelper(JNIEnv *env, ThreadNode *node, void *arg)
{
    SuspendAllArg *saArg = (SuspendAllArg *)arg;
    jint error = JVMDI_ERROR_NONE;
    jthread *list = saArg->list;
    jint count = saArg->count;
    if (!contains(env, list, count, node->thread)) {
        error = commonSuspend(node->thread, JNI_FALSE);
    }
    return error;
}

jint 
threadControl_suspendAll() 
{
    JNIEnv *env = getEnv();
    jint error = JVMDI_ERROR_NONE;
    jthread *threads = NULL;
    jint count;
    int i;

    preSuspend();

    /*
     * Get a list of all threads and suspend them.
     */
    threads = allThreads(&count);
    if (threads == NULL) {
        error = JVMDI_ERROR_OUT_OF_MEMORY;
        goto cleanup;
    }
    for (i = 0; i < count; i++) {
        error = commonSuspend(threads[i], JNI_FALSE);

        if (error != JVMDI_ERROR_NONE) {
            goto cleanup;
        }
    }

    /*
     * Update the suspend count of any threads not yet (or no longer)
     * in the thread list above.  
     */
    {
        SuspendAllArg arg;
        arg.list = threads;
        arg.count = count;
        error = enumerateOverThreadList(env, &otherThreads, 
                                        suspendAllHelper, &arg);
    }

    if (error == JVMDI_ERROR_NONE) {
        suspendAllCount++;
    }

cleanup:
    if (threads != NULL) {
        freeGlobalRefs(threads, count);
    }

    postSuspend();

    return error;
}

static jint
resumeHelper(JNIEnv *env, ThreadNode *node, void *ignored)
{
    /*
     * Go through the single thread external
     * resume function. so that the otherThreads list is maintained
     * correctly.
     */
    return commonResume(env, node->thread);
}

jint 
threadControl_resumeAll() 
{
    JNIEnv *env = getEnv();
    jint error;

    eventHandler_lock(); /* for proper lock order */
    debugMonitorEnter(threadLock);

    /*
     * Resume only those threads that the debugger has suspended. All 
     * such threads must have a node in one of the thread lists, so there's
     * no need to get the whole thread list from JVMDI (unlike 
     * suspendAll).
     */
    error = enumerateOverThreadList(env, &runningThreads, 
                                    resumeHelper, NULL);
    if ((error == JVMDI_ERROR_NONE) && (otherThreads.first != NULL)) {
        error = enumerateOverThreadList(env, &otherThreads, 
                                        resumeHelper, NULL);
        removeResumed(env, &otherThreads);
    }

    if (suspendAllCount > 0) {
        suspendAllCount--;
    }

    debugMonitorExit(threadLock);
    eventHandler_unlock();
    /* let eventHelper.c: commandLoop() know we are resuming */
    unblockCommandLoop();

    return error;
}

 
StepRequest *
threadControl_getStepRequest(jthread thread) 
{ 
    ThreadNode *node;
    JNIEnv *env = getEnv();
    StepRequest *step = NULL;

    debugMonitorEnter(threadLock);

    node = findThread(env, &runningThreads, thread);
    if (node != NULL) {
        step = &node->currentStep;
    }

    debugMonitorExit(threadLock);

    return step;
}

InvokeRequest *
threadControl_getInvokeRequest(jthread thread) 
{ 
    ThreadNode *node;
    JNIEnv *env = getEnv();
    InvokeRequest *request = NULL;

    debugMonitorEnter(threadLock);

    node = findThread(env, &runningThreads, thread);
    if (node != NULL) {
         request = &node->currentInvoke;
    }

    debugMonitorExit(threadLock);

    return request;
}

jint 
threadControl_addDebugThread(jthread thread) 
{
    jint rc;
    JNIEnv *env = getEnv();

    debugMonitorEnter(threadLock);
    if (debugThreadCount >= MAX_DEBUG_THREADS) {
        rc = JVMDI_ERROR_OUT_OF_MEMORY;
    } else {
        thread = (*env)->NewGlobalRef(env, thread);
        if (thread == NULL) {
            rc = JVMDI_ERROR_OUT_OF_MEMORY;
        } else {
            debugThreads[debugThreadCount++] = thread;
            rc = JVMDI_ERROR_NONE;
        }
    }
    debugMonitorExit(threadLock);
    return rc;
}

jint 
threadControl_removeDebugThread(jthread thread)
{
    jint rc = JVMDI_ERROR_INVALID_THREAD;
    JNIEnv *env = getEnv();
    int i;
    int j;

    debugMonitorEnter(threadLock);
    for (i = 0; i< debugThreadCount; i++) {
        if ((*env)->IsSameObject(env, thread, debugThreads[i])) {
            (*env)->DeleteGlobalRef(env, debugThreads[i]);
            for (j = i+1; j < debugThreadCount; j++) {
                debugThreads[j-1] = debugThreads[j];
            }
            debugThreadCount--;
	    if (debugThreadCount == 0) {
		debugMonitorNotify(threadLock);
	    }
            rc = JVMDI_ERROR_NONE;
            break;
        }
    }
    debugMonitorExit(threadLock);
    return rc;
}

void 
threadControl_joinAllDebugThreads(void)
{
    debugMonitorEnter(threadLock);
    while (debugThreadCount > 0) {
	debugMonitorWait(threadLock);
    }
    debugMonitorExit(threadLock);
}

jboolean 
threadControl_isDebugThread(jthread thread) 
{
    int i;
    jboolean rc = JNI_FALSE;
    JNIEnv *env = getEnv();

    debugMonitorEnter(threadLock);
    for (i = 0; i < debugThreadCount; i++) {
        if ((*env)->IsSameObject(env, thread, debugThreads[i])) {
            rc = JNI_TRUE;
            break;
        }
    }
    debugMonitorExit(threadLock);
    return rc;
}

struct bag *
threadControl_onEventHandlerEntry(jbyte sessionID, jint eventKind, jthread thread) 
{
    ThreadNode *node;
    JNIEnv *env = getEnv();
    struct bag *eventBag;
    jthread threadToSuspend = NULL;

    debugMonitorEnter(threadLock);

    /*
     * Check the list of unknown threads maintained by suspend 
     * and resume. If this thread is currently present in the 
     * list, it should be 
     * moved to the runningThreads list, since it is a 
     * well-known thread now.
     */
    node = findThread(env, &otherThreads, thread);
    if (node != NULL) {
        moveThread(env, &otherThreads, &runningThreads, node->thread);
    } else {
        /*
         * Get a thread node for the reporting thread. For thread start
         * events, or if this event precedes a thread start event, 
         * the thread node may need to be created. 
         * 
         * It is possible for certain events (notably method entry/exit) 
         * to precede thread start for some VM implementations. 
         */
        node = insertThread(env, &runningThreads, thread);
        if (node == NULL) {
            /*
             * Not found in the running threads, and could not be created.
             */
            ERROR_MESSAGE_EXIT("Unable to create thread table entry");
        }
    }

    /*
     * Store the JNI environment in the thread node, if it hasn't been 
     * already. The JNI environment pointer is used in some cases as a
     * thread id.
     */
    if (node->env == NULL) {
        node->env = env;
    } else {
        /* JNIEnv should always be the same for a given thread */
        JDI_ASSERT(node->env == env);
    }

    if (eventKind == JVMDI_EVENT_THREAD_START) {
        node->isStarted = JNI_TRUE;
        processDeferredEventModes(env, thread, node);
    }

    node->currentEventKind = eventKind;
    eventBag = node->eventBag;
    if (node->suspendOnStart) {
        threadToSuspend = node->thread;
    }
    debugMonitorExit(threadLock);

    if (threadToSuspend != NULL) {
        /*
         * An attempt was made to suspend this thread before it started.
         * We must suspend it now, before it starts to run. This must
         * be done with no locks held.
         */
        eventHelper_suspendThread(sessionID, threadToSuspend);
    }

    return eventBag;
}

static void
doPendingTasks(JNIEnv *env, ThreadNode *node) 
{
    /*
     * Take care of any pending interrupts/stops, and clear out 
     * info on pending interrupts/stops.
     */
    if (node->pendingInterrupt) {
        jvmdi->InterruptThread(node->thread);
        /*
         * %comment gordonh020
         */
        node->pendingInterrupt = JNI_FALSE;
    }

    if (node->pendingStop != NULL) {
        jvmdi->StopThread(node->thread, node->pendingStop);
        /*
         * %comment gordonh020
         */
        (*env)->DeleteGlobalRef(env, node->pendingStop);
        node->pendingStop = NULL;
    }
}

void
threadControl_onEventHandlerExit(jint eventKind, jthread thread, 
                                 struct bag *eventBag) 
{
    ThreadNode *node;
    JNIEnv *env = getEnv();

    if (eventKind == JVMDI_EVENT_THREAD_END) {
        eventHandler_lock(); /* for proper lock order */
    }
    debugMonitorEnter(threadLock);

    node = findThread(env, &runningThreads, thread);
    if (node == NULL) {
        ERROR_MESSAGE_EXIT("thread list corrupted"); 
    } else {
        if (eventKind == JVMDI_EVENT_THREAD_END) {
            jboolean inResume = (node->resumeFrameDepth > 0);
            removeThread(env, &runningThreads, thread);
            node = NULL;   /* has been freed */

            /*
             * Clean up mechanism used to detect end of 
             * resume.
             */
            if (inResume) {
                notifyAppResumeComplete();
            }
        } else {
            /* No point in doing this if the thread is about to die.*/
            doPendingTasks(env, node);
            node->eventBag = eventBag;
            node->currentEventKind = NO_EVENT;
        }
    }

    debugMonitorExit(threadLock);
    if (eventKind == JVMDI_EVENT_THREAD_END) {
        eventHandler_unlock();
    }
}

jint
threadControl_applicationThreadStatus(jthread thread, jint *threadStatus,
                                                      jint *suspendStatus) 
{
    ThreadNode *node;
    JNIEnv *env = getEnv();
    jint error = JVMDI_ERROR_NONE;

    debugMonitorEnter(threadLock);

    error = jvmdi->GetThreadStatus(thread, threadStatus, 
                                           suspendStatus);
    if (error == JVMDI_ERROR_NONE) {
        node = findThread(env, &runningThreads, thread);
        if ((node != NULL) && HANDLING_EVENT(node)) {
            /*
             * While processing an event, an application thread is always
             * considered to be running even if its handler happens to be 
             * cond waiting on an internal debugger monitor, etc.
             *
             * Leave suspend status untouched since it is not possible
             * to distinguish debugger suspends from app suspends.
             */
            *threadStatus = JVMDI_THREAD_STATUS_RUNNING;
        }
    }

    debugMonitorExit(threadLock);

    return error;
}

jint 
threadControl_interrupt(jthread thread)
{
    ThreadNode *node;
    JNIEnv *env = getEnv();
    jint error = JVMDI_ERROR_NONE;

    debugMonitorEnter(threadLock);

    node = findThread(env, &runningThreads, thread);
    if ((node == NULL) || !HANDLING_EVENT(node)) {
        error = jvmdi->InterruptThread(thread);
    } else {
        /*
         * Hold any interrupts until after the event is processed.
         */
        node->pendingInterrupt = JNI_TRUE;
    }

    debugMonitorExit(threadLock);

    return error;
}

void
threadControl_setPendingInterrupt(jthread thread) 
{
    ThreadNode *node;
    JNIEnv *env = getEnv();

    debugMonitorEnter(threadLock);

    node = findThread(env, &runningThreads, thread);
    if (node != NULL) {
        node->pendingInterrupt = JNI_TRUE;
    }

    debugMonitorExit(threadLock);
}

jint 
threadControl_stop(jthread thread, jobject throwable)
{
    ThreadNode *node;
    JNIEnv *env = getEnv();
    jint error = JVMDI_ERROR_NONE;

    debugMonitorEnter(threadLock);

    node = findThread(env, &runningThreads, thread);
    if ((node == NULL) || !HANDLING_EVENT(node)) {
        error = jvmdi->StopThread(thread, throwable);
    } else {
        /*
         * Hold any stops until after the event is processed.
         */
        node->pendingStop = (*env)->NewGlobalRef(env, throwable);
        if (node->pendingStop == NULL) {
            error = JVMDI_ERROR_OUT_OF_MEMORY;
        }
    }

    debugMonitorExit(threadLock);

    return error;
}

static jint
detachHelper(JNIEnv *env, ThreadNode *node, void *arg)
{
    invoker_detach(&node->currentInvoke);
    return JVMDI_ERROR_NONE;
}

void
threadControl_detachInvokes()
{
    JNIEnv *env = getEnv();
    invoker_lock(); /* for proper lock order */
    debugMonitorEnter(threadLock);
    enumerateOverThreadList(env, &runningThreads, detachHelper, NULL);
    debugMonitorExit(threadLock);
    invoker_unlock();
}

static jint
resetHelper(JNIEnv *env, ThreadNode *node, void *arg)
{
    if (node->toBeResumed) {
        jvmdi->ResumeThread(node->thread);
    }
    stepControl_clearRequest(node->thread, &node->currentStep);
    node->toBeResumed = JNI_FALSE;
    node->suspendCount = 0;
    node->suspendOnStart = JNI_FALSE;

    return JVMDI_ERROR_NONE;
}

void 
threadControl_reset() 
{
    JNIEnv *env = getEnv();

    eventHandler_lock(); /* for proper lock order */
    debugMonitorEnter(threadLock);
    enumerateOverThreadList(env, &runningThreads, resetHelper, NULL);
    enumerateOverThreadList(env, &otherThreads, resetHelper, NULL);

    removeResumed(env, &otherThreads);

    freeDeferredEventModes(env);

    suspendAllCount = 0;

    /* Everything should have been resumed */
    JDI_ASSERT(otherThreads.first == NULL);

    debugMonitorExit(threadLock);
    eventHandler_unlock();
}

jint 
threadControl_getInstructionStepMode(jthread thread) 
{
    JNIEnv *env = getEnv();
    ThreadNode *node = findThread(env, &runningThreads, thread);
    if (node != NULL) {
        return node->instructionStepMode;
    }
    return JVMDI_DISABLE; /* should never come here */
}

jint 
threadControl_setEventMode(jint mode, jint event, jthread thread) 
{
    jint error;
    if ((thread == NULL) || version_supportsImmediateEventModeSet()) {
        /* record single step mode */
        if ((thread != NULL) && (event == JVMDI_EVENT_SINGLE_STEP)) {
            JNIEnv *env = getEnv();
            ThreadNode *node = findThread(env, &runningThreads, thread);
            if (node != NULL) {
                node->instructionStepMode = mode;
            }
        }
        error = jvmdi->SetEventNotificationMode(mode, event, thread);
    } else {
        /*
         * Workaraound for bug 4230260 in classic VM:
         * If a thread has not yet been started, the notification
         * request must be delayed until the thread start event. 
         */
        ThreadNode *node;
        JNIEnv *env = getEnv();

        debugMonitorEnter(threadLock);

        node = findThread(env, &runningThreads, thread);
        if ((node == NULL) || (!node->isStarted)) {
            error = addDeferredEventMode(env, mode, event, thread);
        } else {
            /* record single step mode */
            if (event == JVMDI_EVENT_SINGLE_STEP) {
                node->instructionStepMode = mode;
            }
            error = jvmdi->SetEventNotificationMode(mode, event, thread);
        }

        debugMonitorExit(threadLock);
    }
    return error;
}

/*
 * This function is a workaroud for bug 4215724 in the classic VM. If we have 
 * stopped in a method entry event, the current location reported by JVMDI is 
 * invalid. In that case, we get the location through other means.
 */
jint threadControl_getFrameLocation(jthread thread, jframeID frame,
                      jclass *clazzPtr, jmethodID *methodPtr, jlocation *locationPtr)
{
    JNIEnv *env = getEnv();
    ThreadNode *node;
    jint error;
    jclass clazz;
    jmethodID method;
    jlocation location;

    debugMonitorEnter(threadLock);

    error = jvmdi->GetFrameLocation(frame, &clazz, &method, &location);
    if ((error == JVMDI_ERROR_NONE) &&
        !version_supportsMethodEntryLocation()) {

        node = findThread(env, &runningThreads, thread);
        if ((node != NULL) && 
            (node->currentEventKind == JVMDI_EVENT_METHOD_ENTRY)) {
            /*
             * We can't trust the location value if inside a method entry
             * event (see above), so use another function we can trust.  
             */
            jlocation end;

            error = jvmdi->GetMethodLocation(clazz, method, &location, &end);
            if (error != JVMDI_ERROR_NONE) {
                (*env)->DeleteGlobalRef(env, clazz);
            }
        }
    } 
    debugMonitorExit(threadLock);

    if (error == JVMDI_ERROR_NONE) {
        *clazzPtr = clazz;
        *methodPtr = method;
        *locationPtr = location;
    }
    return error;
}

/*
 * Returns the current thread, if the thread has generated at least
 * one event, and has not generated a thread end event.
 */
jthread threadControl_currentThread() {
    JNIEnv *env = getEnv();
    ThreadNode *node;
    jthread thread;

    debugMonitorEnter(threadLock);
    node = findThreadByEnv(env, &runningThreads);
    thread = (node == NULL) ? NULL : node->thread;
    debugMonitorExit(threadLock);
    return thread;
}

