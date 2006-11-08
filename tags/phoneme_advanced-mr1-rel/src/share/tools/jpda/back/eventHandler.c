/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */
/*
 * eventHandler
 *
 * This module handles events as they come in directly from JVMDI
 * and also maps them to JDI events.  JDI events are those requested
 * at the JDI or JDWP level and seen on those levels.  Mapping is
 * one-to-many, a JVMDI event may map to several JDI events, or
 * to none.  Part of that mapping process is filteration, which 
 * eventFilter sub-module handles.  A JDI EventRequest corresponds
 * to a HandlerNode and a JDI filter to the hidden HandlerNode data
 * used by eventFilter.  For example, if at the JDI level the user
 * executed:
 *   
 *   EventRequestManager erm = vm.eventRequestManager();
 *   BreakpointRequest bp = erm.createBreakpointRequest();
 *   bp.enable();
 *   ClassPrepareRequest req = erm.createClassPrepareRequest();
 *   req.enable();
 *   req = erm.createClassPrepareRequest();
 *   req.addClassFilter("Foo*");
 *   req.enable();
 *
 * Three handlers would be created, the first with a LocationOnly
 * filter and the last with a ClassMatch  filter. 
 * When a JVMDI class prepare event for "Foobar"
 * comes in, the second handler will create one JDI event, the
 * third handler will compare the class signature, and since
 * it matchs create a second event.  There may also be internal
 * events as there are in this case, one created by the front-end
 * and one by the back-end.
 *
 * Each event kind has a handler chain, which is a doublely linked
 * list of handlers for that kind of event.
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <jvmdi.h>
#include "util.h"
#include "debugInit.h"
#include "eventHandler.h"
#include "eventHandlerRestricted.h"
#include "eventFilter.h"
#include "eventFilterRestricted.h"
#include "standardHandlers.h"
#include "threadControl.h"
#include "eventHelper.h"
#include "classTrack.h"
#include "commonRef.h"
#include "JDWP.h"
#include "version.h"

static HandlerID requestIdCounter;
static jbyte currentSessionID;

/* Once the VM is dead it stays that way - don't put in init */
jboolean vmDead = JNI_FALSE;

/* Allow events on a specific thread to be redirected */
jthread redirectedEventThread = NULL;
EventRedirectHook redirectedEventFunction = NULL;

/*
 * We are starting with a very simple locking scheme
 * for event handling.  All readers and writers of data in
 * the handlers[] chain must own this lock for the duration
 * of its use. If contention becomes a problem, we can:
 * 
 * 1) create a lock per event type. 
 * 2) move to a readers/writers approach where multiple threads 
 * can access the chains simultaneously while reading (the
 * normal activity of an event hook). 
 */
static JVMDI_RawMonitor handlerLock;

typedef struct HandlerChain_ {
    HandlerNode *first;
    /* add lock here */
} HandlerChain;

/*
 * This array maps event kinds to handler chains.
 * Protected by handlerLock.
 */
static HandlerChain handlers[JVMDI_MAX_EVENT_TYPE_VAL+1];

/* Given a HandlerNode, these access our private data.
 */
#define PRIVATE_DATA(node) \
       (&(((EventHandlerRestricted_HandlerNode*)(node))->private_ehpd))

#define NEXT(node) (PRIVATE_DATA(node)->private_next)
#define PREV(node) (PRIVATE_DATA(node)->private_prev)
#define CHAIN(node) (PRIVATE_DATA(node)->private_chain)
#define HANDLER_FUNCTION(node) (PRIVATE_DATA(node)->private_handlerFunction)

static jint freeHandler(HandlerNode *node);

static jint freeHandlerChain(HandlerChain *chain);

static void
insert(HandlerChain *chain, HandlerNode *node)
{
    HandlerNode *oldHead = chain->first;
    NEXT(node) = oldHead;
    PREV(node) = NULL;
    CHAIN(node) = chain;
    if (oldHead != NULL) {
        PREV(oldHead) = node;
    }
    chain->first = node;
}

static HandlerNode *
findInChain(HandlerChain *chain, HandlerID handlerID)
{
    HandlerNode *node = chain->first;
    while (node != NULL) {
        if (node->handlerID == handlerID) {
            return node;
        }
        node = NEXT(node);
    }
    return NULL;
}

static HandlerNode *
find(jint kind, HandlerID handlerID)
{
    return findInChain(&handlers[kind], handlerID);
}

/**
 * Deinsert.  Safe for non-inserted nodes.
 */
static void
deinsert(HandlerNode *node)
{
    HandlerChain *chain = CHAIN(node);

    if (chain == NULL) {
        return;
    }
    if (chain->first == node) {
        chain->first = NEXT(node);
    }
    if (NEXT(node) != NULL) {
        PREV(NEXT(node)) = PREV(node);
    }
    if (PREV(node) != NULL) {
        NEXT(PREV(node)) = NEXT(node);
    }
    CHAIN(node) = NULL;
}

jboolean 
eventHandlerRestricted_iterator(jint kind,
                              IteratorFunction func, void *arg)
{
    HandlerChain *chain = &handlers[kind];
    HandlerNode *node = chain->first;
    JNIEnv *env = getEnv();

    while (node != NULL) {
        if (((func)(env, node, arg))) {
            return JNI_TRUE;
        }
        node = NEXT(node);
    }
    return JNI_FALSE;
}

static void 
deferringHandler(JNIEnv *env, JVMDI_Event *event, 
                 HandlerNode *node, 
                 struct bag *eventBag)
{
    /* This handler is really just a way to make sure that the
     * event is handled on the next occurrence of a particular
     * event. 
     */
    freeHandler(node);
}

static jboolean
insertDeferringBreakpoint(JNIEnv *env, jthread thread, 
                       jclass clazz, jmethodID method, jlocation location) 
{
    HandlerNode *node;

    node = eventHandler_createInternalBreakpoint(deferringHandler, 
                               thread, clazz, method, location);

    return (node != NULL);
}

static jboolean
deferEventReport(JNIEnv *env, JVMDI_Event *event, jthread thread)
{ 
    jboolean deferring = JNI_FALSE;

    /* JVMDI event ordering must be supported without bugs in
     * order to defer reported events. 
     */
    if (version_supportsEventOrdering()) {
        switch (event->kind) {
            /* Do things the easy way for now. Insert a breakpoint
             * and defer the reporting of these events until we
             * get to the breakpoint. This could be done more
             * efficiently by deferring only if there's a
             * breakpoint already set, but that approach would
             * also require work to make sure that for any
             * breakpoint that is set after these events are
             * reported and before the thread continues, that
             * breakpoint is ignored the first time it is seen. 
             */
            case JVMDI_EVENT_METHOD_ENTRY: {
                JVMDI_frame_event_data *entryEvent = &event->u.frame;
                if (!isMethodNative(entryEvent->clazz, entryEvent->method)) {
                    jint error;
                    jlocation start;
                    jlocation end;
                    error = jvmdi->GetMethodLocation(entryEvent->clazz, 
                                                     entryEvent->method,
                                                     &start, &end);
                    if (error == JVMDI_ERROR_NONE) {
                        deferring = insertDeferringBreakpoint(env, 
                                                   thread, entryEvent->clazz,
                                                   entryEvent->method, start);
                    }
                }
    
                break;
            }
            case JVMDI_EVENT_SINGLE_STEP: {
                JVMDI_single_step_event_data *stepEvent =
                    &event->u.single_step;
                deferring = insertDeferringBreakpoint(env,
                                                   thread, stepEvent->clazz,
                                                   stepEvent->method, 
                                                   stepEvent->location);
                break;
            }
            /* %comment gordonh011 */
        }
    }
    return deferring;
}

static void 
reportEvents(JNIEnv *env, jbyte sessionID, jthread thread, 
             JVMDI_Event *event, struct bag *eventBag)
{
    jbyte suspendPolicy;
    jboolean invoking;

    if (bagSize(eventBag) < 1) {
        return;
    }

    /*
     * Never report events before initialization completes
     */
    if (!debugInit_isInitComplete()) {
        return;
    }

    /* We delay the reporting of some events so that they can be
     * properly grouped into event sets with upcoming events. If
     * the reporting is to be deferred, the event commands remain
     * in the event bag until a subsequent event occurs.  Event is
     * NULL for synthetic events (e.g. unload). 
     */
    if (event == NULL || !deferEventReport(env, event, thread)) {
        struct bag *completedBag = bagDup(eventBag);
        bagDeleteAll(eventBag);
        if (completedBag == NULL) {
            /*
             * %comment gordonh012
             */
        } else {
            suspendPolicy = eventHelper_reportEvents(sessionID, completedBag);
            if (thread != NULL && suspendPolicy != JDWP_SuspendPolicy_NONE) {
                do {
                    /* The events have been reported and this
                     * thread is about to continue, but it may
                     * have been started up up just to perform a
                     * requested method invocation. If so, we do
                     * the invoke now and then stop again waiting
                     * for another continue. By then another
                     * invoke request can be in place, so there is
                     * a loop around this code. 
                     */
                    invoking = invoker_doInvoke(thread);
                    if (invoking) {
                        eventHelper_reportInvokeDone(sessionID, thread);
                    }
                } while (invoking);
            }
            bagDestroyBag(completedBag);
        }
    }
}

/* A bagEnumerateFunction.  Create a synthetic class unload event
 * for every class no longer present.  Analogous to eventHook
 * combined with a handler in a unload specific (no event
 * structure) kind of way. 
 */
static jboolean
synthesizeUnloadEvent(void *signatureVoid, void *envVoid)
{
    JNIEnv *env = (JNIEnv *)envVoid;
    char *signature = *(char **)signatureVoid;
    char *classname;
    HandlerNode *node;
    jbyte eventSessionID = currentSessionID; 
    struct bag *eventBag = eventHelper_createEventBag();

    if (eventBag == NULL) {
        /* TO DO: Report, but don't die
         */
        JDI_ASSERT(eventBag != NULL);
    }

    /* Signature needs to last, so convert extra copy to
     * classname
     */
    classname = jdwpAlloc(strlen(signature)+1);
    strcpy(classname, signature);
    convertSignatureToClassname(classname);

    debugMonitorEnter(handlerLock);
    
    node = handlers[JVMDI_EVENT_CLASS_UNLOAD].first;
    while (node != NULL) {
        /* save next so handlers can remove themselves */
        HandlerNode *next = NEXT(node);
        jboolean shouldDelete;

        if (eventFilterRestricted_passesUnloadFilter(env, classname, 
                                                     node, 
                                                     &shouldDelete)) {
            /* There may be multiple handlers, the signature will
             * be freed when the event helper thread has written
             * it.  So each event needs a separate allocation. 
             */
            char *durableSignature = jdwpAlloc(strlen(signature)+1);
            strcpy(durableSignature, signature);

            eventHelper_recordClassUnload(node->handlerID, 
                                          durableSignature,
                                          eventBag);
        }
        if (shouldDelete) {
            /* We can safely free the node now that we are done
             * using it.
             */
            freeHandler(node);
        }
        node = next;
    }
    
    debugMonitorExit(handlerLock);
    
    if (eventBag != NULL) {
        reportEvents(env, eventSessionID, NULL, NULL, eventBag);

        /*
         * bag was created locally, destroy it here.
         */
        bagDestroyBag(eventBag);
    }

    jdwpFree(signature);
    jdwpFree(classname);

    return JNI_TRUE;
}

/* The JVMDI event hook. Each event is passed to a sequence of
 * handlers in a chain until the chain ends or one handler
 * consumes the event. Expensive or complex events are handled in
 * a separate thread, but for now, we wait for the event thread to
 * complete its handing before continuing.  
 */
static void 
eventHook(JNIEnv *env, JVMDI_Event *event)
{
    static unsigned int eventCount = 0;
    static unsigned int classUnloadsPending = 0;
    struct bag *eventBag = NULL;
    jbyte eventSessionID = currentSessionID; /* session could change */
    jthread thread;
    jthrowable currentException;

    /* It is not safe to process unload events as they come in, 
     * since the VM will be locked down in GC. 
     * We note that they have occurred so that we can synthesize
     * them the next time a non-unload event comes in.
     * We don't dare grab locks during the GC, so reads and
     * writes to classUnloadsPending are not protected, see the
     * code that reads it (later in this function).
     */
    if (event->kind == JVMDI_EVENT_CLASS_UNLOAD) {
        ++classUnloadsPending;
        return;
    }

    /* Workaround bug 4199400 in classic VM by avoiding class load
     * events completely. Any of the lock grabbing below can
     * result in a deadlock if we don't do this.  
     */
    if (!version_supportsClassLoadEvents() && 
        event->kind == JVMDI_EVENT_CLASS_LOAD) {
        return;
    }

    /* We want to preserve any current exception that might get
     * wiped out during event handling (e.g. JNI calls). We have
     * to rely on space for the local reference on the current
     * frame because doing a PushLocalFrame here might itself
     * generate an exception.  
     */
    currentException = (*env)->ExceptionOccurred(env);
    (*env)->ExceptionClear(env);

    /* Periodically, we want to compact the hash table of all
     * objects sent to the front end by removing objects that have
     * been collected. For now, this is done in a very primitive
     * fashion (every N events). In the future we may be able to
     * use JVMDI events for garbage collection to trigger the
     * compaction.  If an unload happenned we are sure to need a
     * compaction.  
     */
    if (((1 + (eventCount++)) % 500) == 0 || classUnloadsPending > 0) {
        commonRef_compact();
    }

    /*
     * Process any waiting unload events.
     *
     * "if" is an optimization to avoid entering the lock on every 
     * event; classUnloadsPending may be zapped before we enter
     * the lock but then this just becomes one big no-op.
     */
    if (classUnloadsPending > 0) {
        struct bag *unloadedSignatures;

        /* this lock won't do anything to prevent the
         * classUnloadsPending count from incrementing, but it
         * will prevent another event thread from blasting it
         * before it is read. It also allows classTrack to
         * analyze which classes have been unloaded without
         * class prepare events coming in.
         */
        debugMonitorEnter(handlerLock);
        /* the classUnloadsPending count could be incremented
         * here, and we would lose the count when it is set
         * to zero, however, these freshly unloaded classes 
         * will be seen when the unloaded classes are analyzed,
         * and nothing will be lost.
         */
        classUnloadsPending = 0;
        /* analyze which unloads occurred
         */
        unloadedSignatures = classTrack_processUnloads(env);
        debugMonitorExit(handlerLock);

        /* generate the synthetic events.
         */
        bagEnumerateOver(unloadedSignatures, synthesizeUnloadEvent, 
                         (void *)env);

        bagDestroyBag(unloadedSignatures);
    }

    thread = eventThread(event);

    if (thread != NULL) {
        if ((redirectedEventThread != NULL) &&
                 (*env)->IsSameObject(env, thread, redirectedEventThread)) {
            /*
             * This thread has it's events redirected.
             */
            jboolean consumed;

            consumed = (*redirectedEventFunction)(event, thread);
            if (consumed) {
                /* Always restore any exception (see below). */
                if (currentException != NULL) {
                    (*env)->Throw(env, currentException);
                } else {
                    (*env)->ExceptionClear(env);
                }
                return;
            }
        }

        /*
         * Record the fact that we're entering an event
         * handler so that thread operations (status, interrupt,
         * stop) can be done correctly and so that thread
         * resources can be allocated.  This must be done before
         * grabbing any locks.
         */
        eventBag = threadControl_onEventHandlerEntry(eventSessionID, 
                                                     event->kind, thread);
    } else {
        eventBag = eventHelper_createEventBag();
        if (eventBag == NULL) {
            /*
             * %comment gordonh013
             */
        }
    }

    debugMonitorEnter(handlerLock);
    {   
        HandlerNode *node = handlers[event->kind].first;

        while (node != NULL) {
            /* save next so handlers can remove themselves */
            HandlerNode *next = NEXT(node); 
            jboolean shouldDelete;

            if (eventFilterRestricted_passesFilter(env, event, 
                                                   node, 
                                                   &shouldDelete)) {
                (HANDLER_FUNCTION(node))(env, event, node, eventBag);
            }
            if (shouldDelete) {
                /* We can safely free the node now that we are done
                 * using it.
                 */
                freeHandler(node);
            }
            node = next;
        }
    }
    debugMonitorExit(handlerLock);
    
    if (eventBag != NULL) {
        reportEvents(env, eventSessionID, thread, event, eventBag);
    }

    /* we are continuing after VMDeathEvent - now we are dead */
    if (event->kind == JVMDI_EVENT_VM_DEATH) {
        vmDead = JNI_TRUE;
    }

    /*
     * If the bag was created locally, destroy it here.
     */
    if (thread == NULL) {
        bagDestroyBag(eventBag);
    }

    /* Always restore any exception that was set beforehand.  If
     * there is a pending async exception, StopThread will be
     * called from threadControl_onEventHandlerExit immediately
     * below.  Depending on VM implementation and state, the async
     * exception might immediately overwrite the currentException,
     * or it might be delayed until later.  */
    if (currentException != NULL) {
        (*env)->Throw(env, currentException);
    } else {
        (*env)->ExceptionClear(env);
    }

    /*
     * Release thread resources and perform any delayed operations.
     */
    if (thread != NULL) {
        threadControl_onEventHandlerExit(event->kind, thread, eventBag);
    }
}

/**
 * Delete this handler:
 * Deinsert handler from active list,
 * make it inactive, and free it's memory
 * Assumes handlerLock held.
 */
static jint
freeHandler(HandlerNode *node) {
    jint error = JVMDI_ERROR_NONE;

    /* deinsert the handler node before disableEvents() to make
     * sure the event will be disabled when no other event
     * handlers are installed. 
     */
    if (node != NULL) {
        deinsert(node);
        error = eventFilterRestricted_deinstall(node);
        jdwpFree(node);
    }

    return error;
}

/**
 * Delete all the handlers on this chain.
 * Assumes handlerLock held.
 */
static jint
freeHandlerChain(HandlerChain *chain) {
    jint error = JVMDI_ERROR_NONE;
    HandlerNode **firstPtr = &chain->first;

    /* delete until there is nothing left in the list */
    while (*firstPtr) {
        jint singleError = freeHandler(*firstPtr);
        if (singleError != JVMDI_ERROR_NONE) {
            error = singleError;
        }
    }
    return error;
}

/**
 * Deinsert and free all memory.  Safe for non-inserted nodes.
 */
jint 
eventHandler_free(HandlerNode *node)
{
    jint error;

    debugMonitorEnter(handlerLock);

    error = freeHandler(node);

    debugMonitorExit(handlerLock);

    return error;
}

/**
 * Free all handlers of this kind created by the JDWP client,
 * that is, doesn't free handlers internally created by back-end.
 */
jint
eventHandler_freeAll(jint kind) 
{
    jint error = JVMDI_ERROR_NONE;
    HandlerNode *node;

    debugMonitorEnter(handlerLock);
    node = handlers[kind].first;
    while (node != NULL) {
        HandlerNode *next = NEXT(node);    /* allows node removal */
        if (node->handlerID != 0) {        /* don't free internal handlers */
            error = freeHandler(node);
            if (error != JVMDI_ERROR_NONE) {
                break;
            }
        }
        node = next;
    }
    debugMonitorExit(handlerLock);
    return error;
}

/***
 * Delete all breakpoints on "clazz".
 */
void
eventHandler_freeClassBreakpoints(jclass clazz) 
{
    HandlerNode *node;
    JNIEnv *env = getEnv();

    debugMonitorEnter(handlerLock);
    node = handlers[JVMDI_EVENT_BREAKPOINT].first;
    while (node != NULL) {
        HandlerNode *next = NEXT(node); /* allows node removal */
        if (eventFilterRestricted_isBreakpointInClass(env, clazz, 
                                                      node)) {
            freeHandler(node);
        }
        node = next;
    }
    debugMonitorExit(handlerLock);
}

jint
eventHandler_freeByID(jint kind, HandlerID handlerID)
{
    jint error;
    HandlerNode *node;

    debugMonitorEnter(handlerLock);
    node = find(kind, handlerID);
    if (node != NULL) {
        error = freeHandler(node);
    } else {
        /* already freed */
        error = JVMDI_ERROR_NONE;
    }
    debugMonitorExit(handlerLock);
    return error;
}

void 
eventHandler_initialize(jbyte sessionID)
{
    jint error;
    jint i;

    requestIdCounter = 1;
    currentSessionID = sessionID;

    handlerLock = debugMonitorCreate("JDWP Event Handler Lock");

    for (i = 0; i <= JVMDI_MAX_EVENT_TYPE_VAL; ++i) {
        handlers[i].first = NULL;
    }

    /*
     * Permanently enabled events.
     */
    error = threadControl_setEventMode(JVMDI_ENABLE, 
                                      JVMDI_EVENT_THREAD_START, NULL);
    if (error != JVMDI_ERROR_NONE) {
        ERROR_MESSAGE_EXIT("Unable to enable thread start events");
    }
    error = threadControl_setEventMode(JVMDI_ENABLE, 
                                       JVMDI_EVENT_THREAD_END, NULL);
    if (error != JVMDI_ERROR_NONE) {
        ERROR_MESSAGE_EXIT("Unable to enable thread end events");
    }

    error = jvmdi->SetEventHook(eventHook);
    if (error != JVMDI_ERROR_NONE) {
        ERROR_MESSAGE_EXIT("Unable to set event hook");
    }

    /* Notify other modules that the event hook is in place */
    threadControl_onHook();

    /* Get the event helper thread initialized */
    eventHelper_initialize(sessionID);
}

void 
eventHandler_reset(jbyte sessionID)
{
    int i;

    debugMonitorEnter(handlerLock);

    /* We must do this first so that if any invokes complete,
     * there will be no attempt to send them to the front
     * end. Waiting for threadControl_reset leaves a window where
     * the invoke completions can sneak through.  
     */
    threadControl_detachInvokes();

    /* Reset the event helper thread, purging all queued and
     * in-process commands.  
     */
    eventHelper_reset(sessionID);

    /* delete all handlers */
    for (i = 0; i <= JVMDI_MAX_EVENT_TYPE_VAL; i++) {
        freeHandlerChain(&handlers[i]); 
    }

    requestIdCounter = 1;
    currentSessionID = sessionID;

    debugMonitorExit(handlerLock);
}

void 
eventHandler_lock()
{
    debugMonitorEnter(handlerLock);
}

void 
eventHandler_unlock()
{
    debugMonitorExit(handlerLock);
}

/***** handler creation *****/

HandlerNode *
eventHandler_alloc(jint filterCount, jbyte kind, jbyte suspendPolicy)
{
    HandlerNode *node = eventFilterRestricted_alloc(filterCount);

    if (node != NULL) {
        node->kind = kind;
        node->suspendPolicy = suspendPolicy;
    }
    
    return node;
}

static jint
installHandler(HandlerNode *node, 
              HandlerFunction func, 
              jboolean external) 
{
    jint error = JVMDI_ERROR_NONE;

    HANDLER_FUNCTION(node) = func;

    debugMonitorEnter(handlerLock);

    node->handlerID = external? ++requestIdCounter : 0;
    error = eventFilterRestricted_install(node);
    if (error == JVMDI_ERROR_NONE) {
        insert(&handlers[node->kind], node);
    }

    debugMonitorExit(handlerLock);

    return error;
}

static HandlerNode *
createInternal(jbyte kind, HandlerFunction func,
               jthread thread,
               jclass clazz, jmethodID method, 
               jlocation location)
{
    jint index = 0;
    jint error = JVMDI_ERROR_NONE;
    HandlerNode *node;

    /*
     * Start with necessary allocations 
     */
    node = eventHandler_alloc(
        ((thread == NULL)? 0 : 1) + ((clazz == NULL)? 0 : 1),
        kind, JDWP_SuspendPolicy_NONE);
    if (node == NULL) {
        return NULL;
    }

    if (thread != NULL) {
        error = eventFilter_setThreadOnlyFilter(node, index++, thread);
    }

    if ((error == JVMDI_ERROR_NONE) && (clazz != NULL)) {
        error = eventFilter_setLocationOnlyFilter(node, index++, clazz, 
                                                  method, location);
    }
    /*
     * Create the new handler node
     */
    error = installHandler(node, func, JNI_FALSE);

    if (error != JVMDI_ERROR_NONE) {
        eventHandler_free(node);
        node = NULL;
    }
    return node;
}

HandlerNode * 
eventHandler_createInternal(jbyte kind, HandlerFunction func)
{
    return createInternal(kind, func, NULL, 
                          NULL, NULL, (jlocation)NULL);
}

HandlerNode * 
eventHandler_createInternalThreadOnly(jbyte kind, 
                                      HandlerFunction func,
                                      jthread thread)
{
    return createInternal(kind, func, thread, 
                          NULL, NULL, (jlocation)NULL);
}

HandlerNode * 
eventHandler_createInternalBreakpoint(HandlerFunction func,
                                      jthread thread,
                                      jclass clazz, 
                                      jmethodID method, 
                                      jlocation location)
{
    return createInternal(JVMDI_EVENT_BREAKPOINT, func, thread, 
                          clazz, method, location);
}

jint 
eventHandler_installExternal(HandlerNode *node) 
{
    return installHandler(node, 
                          standardHandlers_defaultHandler(node->kind), 
                          JNI_TRUE);
}


