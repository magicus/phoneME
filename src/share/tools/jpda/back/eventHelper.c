/*
 * @(#)eventHelper.c	1.44 06/10/10
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
#include <string.h>
#include <jvmdi.h>

#include "JDWP.h"
#include "outStream.h"
#include "eventHandler.h"
#include "threadControl.h"
#include "util.h"
#include "debugInit.h"
#include "invoker.h"

/*
 * Event helper thread command commandKinds
 */
#define COMMAND_REPORT_EVENT_COMPOSITE          1
#define COMMAND_REPORT_INVOKE_DONE              2
#define COMMAND_REPORT_VM_INIT                  3
#define COMMAND_SUSPEND_THREAD                  4

/*
 * Event helper thread command singleKinds
 */
#define COMMAND_SINGLE_EVENT                    11
#define COMMAND_SINGLE_UNLOAD                   12
#define COMMAND_SINGLE_FRAME_EVENT              13

typedef struct EventCommandSingle {
    jbyte suspendPolicy;
    jint id;
    JVMDI_Event event;
} EventCommandSingle;

typedef struct UnloadCommandSingle {
    char *classSignature;
    jint id;
} UnloadCommandSingle;

typedef struct FrameEventCommandSingle {
    jbyte suspendPolicy;
    jint id;
    jbyte kind;
    jthread thread;
    jclass clazz;
    jmethodID method;
    jlocation location;
} FrameEventCommandSingle;

typedef struct CommandSingle {
    jint singleKind;
    union {
        EventCommandSingle eventCommand;
        UnloadCommandSingle unloadCommand;
        FrameEventCommandSingle frameEventCommand;
    } u;
} CommandSingle;

typedef struct ReportInvokeDoneCommand {
    jthread thread;
} ReportInvokeDoneCommand;

typedef struct ReportVMInitCommand {
    jthread thread;
    jbyte suspendPolicy;
} ReportVMInitCommand;

typedef struct SuspendThreadCommand {
    jthread thread;
} SuspendThreadCommand;

typedef struct ReportEventCompositeCommand {
    jbyte suspendPolicy;
    jint eventCount;
    CommandSingle singleCommand[1]; /* variable length */
} ReportEventCompositeCommand;

typedef struct HelperCommand {
    jint commandKind;
    jboolean done;
    jboolean waiting;
    jbyte sessionID;
    struct HelperCommand *next;
    union {
        ReportEventCompositeCommand reportEventComposite;
        ReportInvokeDoneCommand     reportInvokeDone;
        ReportVMInitCommand         reportVMInit;
        SuspendThreadCommand        suspendThread;
    } u;
    /* composite array expand out, put nothing after */
} HelperCommand;

typedef struct {
    HelperCommand *head;
    HelperCommand *tail;
} CommandQueue;

static CommandQueue commandQueue;
static JVMDI_RawMonitor commandQueueLock;
static JVMDI_RawMonitor commandCompleteLock;
static JVMDI_RawMonitor blockCommandLoopLock;
static jint maxQueueSize = 50 * 1024; /* %comment gordonh014 */
static jboolean holdEvents;
static jboolean shutdown = JNI_FALSE;
static jint currentQueueSize = 0;
static jint currentSessionID;

#define JVMDIKind_to_JDWPEvent(kind) ((jbyte)(kind))

static jint 
commandSize(HelperCommand *command) 
{
    jint size = sizeof(HelperCommand);
    if (command->commandKind == COMMAND_REPORT_EVENT_COMPOSITE) {
        /* 
         * One event is accounted for in the Helper Command. If there are
         * more, add to size here. 
         */
        size += (sizeof(CommandSingle) * 
                     (command->u.reportEventComposite.eventCount - 1));
    }
    return size;
}

static void
enqueueCommand(HelperCommand *command, 
               jboolean wait, jboolean reportingVMDeath) 
{
    static jboolean vmDeathReported = JNI_FALSE;
    CommandQueue *queue = &commandQueue;
    jint size = commandSize(command);
    command->done = JNI_FALSE;
    command->waiting = wait;
    command->next = NULL;

    debugMonitorEnter(commandQueueLock);
    while (size + currentQueueSize > maxQueueSize) {
        debugMonitorWait(commandQueueLock);
    }
    if (vmDeathReported) {
        /* send no more events after VMDeath and don't wait */
        wait = JNI_FALSE;
    } else {
        currentQueueSize += size;

        if (queue->head == NULL) {
            queue->head = command;
        } else {
            queue->tail->next = command;
        }
        queue->tail = command;

        if (reportingVMDeath) {
            vmDeathReported = JNI_TRUE;
        }
    }
    debugMonitorNotifyAll(commandQueueLock);
    debugMonitorExit(commandQueueLock);
    
    if (wait) {
        debugMonitorEnter(commandCompleteLock);
        while (!command->done) {
            debugMonitorWait(commandCompleteLock);
        }
        jdwpFree(command);
        debugMonitorExit(commandCompleteLock);
    }
}

static void 
completeCommand(HelperCommand *command) 
{
    if (command->waiting) {
        debugMonitorEnter(commandCompleteLock);
        command->done = JNI_TRUE;
        debugMonitorNotifyAll(commandCompleteLock);
        debugMonitorExit(commandCompleteLock);
    } else {
        jdwpFree(command);
    }
}

static HelperCommand *
dequeueCommand() 
{
    HelperCommand *command = NULL;
    CommandQueue *queue = &commandQueue;
    jint size;

    debugMonitorEnter(commandQueueLock);

    while (command == NULL) {
        while (holdEvents || (queue->head == NULL && !shutdown)) {
            debugMonitorWait(commandQueueLock);
        }
    
	if (shutdown) {
	    break;
	}

        JDI_ASSERT(queue->head);
        command = queue->head;
        queue->head = command->next;
        if (queue->tail == command) {
            queue->tail = NULL;
        }
    
        size = commandSize(command);
        /*
         * Immediately close out any commands enqueued from a 
         * previously attached debugger. 
         */
        if (command->sessionID != currentSessionID) {
            completeCommand(command);
            command = NULL;
        }

        /*
         * There's room in the queue for more.
         */
        currentQueueSize -= size;
        debugMonitorNotifyAll(commandQueueLock);
    }

    debugMonitorExit(commandQueueLock);

    return command;
}

void eventHelper_holdEvents(void)
{
    debugMonitorEnter(commandQueueLock);
    holdEvents = JNI_TRUE;
    debugMonitorNotifyAll(commandQueueLock);
    debugMonitorExit(commandQueueLock);
}

void eventHelper_releaseEvents(void)
{
    debugMonitorEnter(commandQueueLock);
    holdEvents = JNI_FALSE;
    debugMonitorNotifyAll(commandQueueLock);
    debugMonitorExit(commandQueueLock);
}

static void 
writeSingleStepEvent(JNIEnv *env, PacketOutputStream *out, JVMDI_Event *event)
{
    JVMDI_single_step_event_data *eventData = &event->u.single_step;

    WRITE_GLOBAL_REF(env, out, eventData->thread);
    writeCodeLocation(out, eventData->clazz, eventData->method, eventData->location);
}

static void 
writeBreakpointEvent(JNIEnv *env, PacketOutputStream *out, JVMDI_Event *event)
{
    JVMDI_breakpoint_event_data *eventData = &event->u.breakpoint;

    WRITE_GLOBAL_REF(env, out, eventData->thread);
    writeCodeLocation(out, eventData->clazz, eventData->method, eventData->location);
}

static void 
writeFieldAccessEvent(JNIEnv *env, PacketOutputStream *out, JVMDI_Event *event)
{
    JVMDI_field_access_event_data *eventData = &event->u.field_access;
    jbyte fieldClassTag;

    fieldClassTag = referenceTypeTag(eventData->field_clazz);

    WRITE_GLOBAL_REF(env, out, eventData->thread);
    writeCodeLocation(out, eventData->clazz, eventData->method, eventData->location);
    outStream_writeByte(out, fieldClassTag);
    WRITE_GLOBAL_REF(env, out, eventData->field_clazz);
    outStream_writeFieldID(out, eventData->field);
    outStream_writeObjectTag(out, eventData->object);
    WRITE_GLOBAL_REF(env, out, eventData->object);
}

static void 
writeFieldModificationEvent(JNIEnv *env, PacketOutputStream *out, 
                            JVMDI_Event *event)
{
    JVMDI_field_modification_event_data *eventData = 
                                     &event->u.field_modification;
    jbyte fieldClassTag;

    fieldClassTag = referenceTypeTag(eventData->field_clazz);

    WRITE_GLOBAL_REF(env, out, eventData->thread);
    writeCodeLocation(out, eventData->clazz, eventData->method, eventData->location);
    outStream_writeByte(out, fieldClassTag);
    WRITE_GLOBAL_REF(env, out, eventData->field_clazz);
    outStream_writeFieldID(out, eventData->field);
    outStream_writeObjectTag(out, eventData->object);
    WRITE_GLOBAL_REF(env, out, eventData->object);
    outStream_writeValue(env, out, (jbyte)eventData->signature_type, 
                         eventData->new_value);
}

static void 
writeExceptionEvent(JNIEnv *env, PacketOutputStream *out, JVMDI_Event *event)
{
    JVMDI_exception_event_data *eventData = &event->u.exception;

    WRITE_GLOBAL_REF(env, out, eventData->thread);
    writeCodeLocation(out, eventData->clazz, eventData->method, eventData->location);
    outStream_writeObjectTag(out, eventData->exception);
    WRITE_GLOBAL_REF(env, out, eventData->exception);
    writeCodeLocation(out, eventData->catch_clazz, 
                      eventData->catch_method, eventData->catch_location);
}

static void 
writeThreadEvent(JNIEnv *env, PacketOutputStream *out, JVMDI_Event *event)
{
    JVMDI_thread_change_event_data *eventData = &event->u.thread_change;

    WRITE_GLOBAL_REF(env, out, eventData->thread);
}

static void 
writeClassEvent(JNIEnv *env, PacketOutputStream *out, JVMDI_Event *event)
{
    JVMDI_class_event_data *eventData = &event->u.class_event;
    jclass clazz = eventData->clazz;
    jbyte classTag;
    jint status;
    char *signature;

    classTag = referenceTypeTag(clazz);
    signature = classSignature(clazz);
    if (signature == NULL) {
        ALLOC_ERROR_EXIT();
    }
    status = classStatus(clazz);

    WRITE_GLOBAL_REF(env, out, eventData->thread);
    outStream_writeByte(out, classTag);
    WRITE_GLOBAL_REF(env, out, clazz);
    outStream_writeString(out, signature);
    outStream_writeInt(out, status);
    jdwpFree(signature);
}

static void 
writeVMDeathEvent(JNIEnv *env, PacketOutputStream *out, JVMDI_Event *event)
{
}

static void 
handleEventCommandSingle(JNIEnv *env, PacketOutputStream *out, 
                           EventCommandSingle *command)
{
    JVMDI_Event *event = &command->event;

    outStream_writeByte(out, JVMDIKind_to_JDWPEvent(event->kind));
    outStream_writeInt(out, command->id);

    switch (event->kind) {
        case JVMDI_EVENT_SINGLE_STEP:  
            writeSingleStepEvent(env, out, event);
            break;
        case JVMDI_EVENT_BREAKPOINT:   
            writeBreakpointEvent(env, out, event);
            break;
        case JVMDI_EVENT_FIELD_ACCESS:    
            writeFieldAccessEvent(env, out, event);
            break;
        case JVMDI_EVENT_FIELD_MODIFICATION:    
            writeFieldModificationEvent(env, out, event);
            break;
        case JVMDI_EVENT_EXCEPTION:    
            writeExceptionEvent(env, out, event);
            break;
        case JVMDI_EVENT_THREAD_START:
        case JVMDI_EVENT_THREAD_END:   
            writeThreadEvent(env, out, event);
            break;
        case JVMDI_EVENT_CLASS_LOAD:   
        case JVMDI_EVENT_CLASS_PREPARE:   
            writeClassEvent(env, out, event);
            break;
        case JVMDI_EVENT_VM_DEATH:
            writeVMDeathEvent(env, out, event);
            break;
        default:
            ERROR_MESSAGE_EXIT("Reporting invalid JVMDI event kind");
    }
}

static void 
handleUnloadCommandSingle(JNIEnv* env, PacketOutputStream *out, 
                           UnloadCommandSingle *command) 
{
    outStream_writeByte(out, JDWP_EVENT(CLASS_UNLOAD));
    outStream_writeInt(out, command->id);
    outStream_writeString(out, command->classSignature);
    jdwpFree(command->classSignature);
}

static void 
handleFrameEventCommandSingle(JNIEnv* env, PacketOutputStream *out, 
                              FrameEventCommandSingle *command) 
{
    outStream_writeByte(out, command->kind);
    outStream_writeInt(out, command->id);
    WRITE_GLOBAL_REF(env, out, command->thread);
    writeCodeLocation(out, command->clazz, command->method, command->location);
}

static void
suspendWithInvokeEnabled(jbyte policy, jthread thread)
{
    invoker_enableInvokeRequests(thread);

    if (policy == JDWP_SuspendPolicy_ALL) {
        threadControl_suspendAll();
    } else {
        threadControl_suspendThread(thread, JNI_FALSE);
    }
}

static void
handleReportEventCompositeCommand(JNIEnv *env, 
                                  ReportEventCompositeCommand *recc) 
{
    PacketOutputStream out;
    jint count = recc->eventCount;
    jint i;

    if (recc->suspendPolicy != JDWP_SuspendPolicy_NONE) {
        /* must determine thread to interrupt before writing */
        /* since writing destroys it */
        jthread thread = NULL;
        for (i = 0; i < count; i++) {
            CommandSingle *single = &(recc->singleCommand[i]);
            switch (single->singleKind) {
                case COMMAND_SINGLE_EVENT:
                    thread = eventThread(&single->u.eventCommand.event);
                    break;
                case COMMAND_SINGLE_FRAME_EVENT:
                    thread = single->u.frameEventCommand.thread;
                    break;
            }
            if (thread != NULL) {
                break;
            }
        }

        if (thread == NULL) { 
            threadControl_suspendAll();
        } else {
            suspendWithInvokeEnabled(recc->suspendPolicy, thread);
        }
    }

    outStream_initCommand(&out, uniqueID(), FLAGS_None, 
                          JDWP_COMMAND_SET(Event), 
                          JDWP_COMMAND(Event, Composite));
    outStream_writeByte(&out, recc->suspendPolicy);
    outStream_writeInt(&out, count);

    for (i = 0; i < count; i++) {
        CommandSingle *single = &(recc->singleCommand[i]);
        switch (single->singleKind) {
            case COMMAND_SINGLE_EVENT:
                handleEventCommandSingle(env, &out, 
                                         &single->u.eventCommand);
                break;
            case COMMAND_SINGLE_UNLOAD:
                handleUnloadCommandSingle(env, &out, 
                                          &single->u.unloadCommand);
                break;
            case COMMAND_SINGLE_FRAME_EVENT:
                handleFrameEventCommandSingle(env, &out, 
                                              &single->u.frameEventCommand);
                break;
        }
    }

    outStream_sendCommand(&out);
    outStream_destroy(&out);
}

static void
handleReportInvokeDoneCommand(JNIEnv* env, ReportInvokeDoneCommand *command) 
{
    invoker_completeInvokeRequest(command->thread);
    (*env)->DeleteGlobalRef(env, command->thread);
}

static void
handleReportVMInitCommand(JNIEnv* env, ReportVMInitCommand *command) 
{
    PacketOutputStream out;

    if (command->suspendPolicy == JDWP_SuspendPolicy_ALL) {
        threadControl_suspendAll();
    } else if (command->suspendPolicy == JDWP_SuspendPolicy_EVENT_THREAD) {
        threadControl_suspendThread(command->thread, JNI_FALSE);
    }

    outStream_initCommand(&out, uniqueID(), FLAGS_None, 
                          JDWP_COMMAND_SET(Event), 
                          JDWP_COMMAND(Event, Composite));
    outStream_writeByte(&out, command->suspendPolicy);
    outStream_writeInt(&out, 1);   /* Always one component */
    outStream_writeByte(&out, JDWP_EventKind_VM_INIT);
    outStream_writeInt(&out, 0);    /* Not in response to an event req. */

    WRITE_GLOBAL_REF(env, &out, command->thread);

    outStream_sendCommand(&out);
    outStream_destroy(&out);
}

static void
handleSuspendThreadCommand(JNIEnv* env, SuspendThreadCommand *command) 
{
    /*
     * For the moment, there's  nothing that can be done with the 
     * return code, so we don't check it here. 
     */
    threadControl_suspendThread(command->thread, JNI_TRUE);
    (*env)->DeleteGlobalRef(env, command->thread);
}

static void
handleCommand(JNIEnv *env, HelperCommand *command)
{
    switch (command->commandKind) {
        case COMMAND_REPORT_EVENT_COMPOSITE:
            handleReportEventCompositeCommand(env, 
                                        &command->u.reportEventComposite);
            break;
        case COMMAND_REPORT_INVOKE_DONE:
            handleReportInvokeDoneCommand(env, &command->u.reportInvokeDone);
            break;
        case COMMAND_REPORT_VM_INIT:
            handleReportVMInitCommand(env, &command->u.reportVMInit);
            break; 
        case COMMAND_SUSPEND_THREAD:
            handleSuspendThreadCommand(env, &command->u.suspendThread);
            break; 
        default:
            ERROR_MESSAGE_EXIT("Invalid Event Helper Command");
    }
}

/*
 * There was an assumption that only one event with a suspend-all
 * policy could be processed by commandLoop() at one time. It was
 * assumed that native thread suspension from the first suspend-all
 * event would prevent the second suspend-all event from making it
 * into the command queue. For the Classic VM, this was a reasonable
 * assumption. However, in HotSpot all thread suspension requires a
 * VM operation and VM operations take time.
 *
 * The solution is to add a mechanism to prevent commandLoop() from
 * processing more than one event with a suspend-all policy. This is
 * accomplished by forcing commandLoop() to wait for either
 * ThreadReferenceImpl.c: resume() or VirtualMachineImpl.c: resume()
 * when an event with a suspend-all policy has been completed.
 */
static jboolean blockCommandLoop = JNI_FALSE;

/*
 * We wait for either ThreadReferenceImpl.c: resume() or
 * VirtualMachineImpl.c: resume() to be called.
 */
static void
doBlockCommandLoop() {
    debugMonitorEnter(blockCommandLoopLock);
    while (blockCommandLoop == JNI_TRUE) {
        debugMonitorWait(blockCommandLoopLock);
    }
    debugMonitorExit(blockCommandLoopLock);
}

/*
 * If the command that we are about to execute has a suspend-all
 * policy, then prepare for either ThreadReferenceImpl.c: resume()
 * or VirtualMachineImpl.c: resume() to be called.
 */
static jboolean
needBlockCommandLoop(HelperCommand *cmd) {
    if (cmd->commandKind == COMMAND_REPORT_EVENT_COMPOSITE
    && cmd->u.reportEventComposite.suspendPolicy == JDWP_SuspendPolicy_ALL) {
        debugMonitorEnter(blockCommandLoopLock);
        blockCommandLoop = JNI_TRUE;
        debugMonitorExit(blockCommandLoopLock);

        return JNI_TRUE;
    }

    return JNI_FALSE;
}

/*
 * Used by either ThreadReferenceImpl.c: resume() or
 * VirtualMachineImpl.c: resume() to resume commandLoop().
 */
void
unblockCommandLoop() {
    debugMonitorEnter(blockCommandLoopLock);
    blockCommandLoop = JNI_FALSE;
    debugMonitorNotifyAll(blockCommandLoopLock);
    debugMonitorExit(blockCommandLoopLock);
}

/*
 * The event helper thread. Dequeues commands and processes them.
 */
static void 
commandLoop(void *unused) 
{
    JNIEnv *env = getEnv();

    while (JNI_TRUE) {
        HelperCommand *command = dequeueCommand();
        if (command != NULL) {
            /*
             * Setup for a potential doBlockCommand() call before calling
             * handleCommand() to prevent any races.
             */
            jboolean doBlock = needBlockCommandLoop(command);
            handleCommand(env, command);
            completeCommand(command);
            /* if we just finished a suspend-all cmd, then we block here */
            if (doBlock) {
                doBlockCommandLoop();
            }
        } else {
	    JDI_ASSERT(shutdown);
	    break;
	}
    }
}

void 
eventHelper_initialize(jbyte sessionID)
{
    currentSessionID = sessionID;
    holdEvents = JNI_FALSE;
    shutdown = JNI_FALSE;
    commandQueue.head = NULL;
    commandQueue.tail = NULL;

    commandQueueLock = debugMonitorCreate("JDWP Event Helper Queue Monitor");
    commandCompleteLock = debugMonitorCreate("JDWP Event Helper Completion Monitor");
    blockCommandLoopLock = debugMonitorCreate("JDWP Event Block CommandLoop Monitor");

    /* Start the event handler thread */
    spawnNewThread(commandLoop, NULL, "JDWP Event Helper Thread");
}

void
eventHelper_reset(jbyte newSessionID)
{
    debugMonitorEnter(commandQueueLock);
    currentSessionID = newSessionID;
    holdEvents = JNI_FALSE;
    debugMonitorNotifyAll(commandQueueLock);
    debugMonitorExit(commandQueueLock);
}

void
eventHelper_shutdown(void)
{
    debugMonitorEnter(commandQueueLock);
    holdEvents = JNI_FALSE;
    shutdown = JNI_TRUE;
    debugMonitorNotifyAll(commandQueueLock);
    debugMonitorExit(commandQueueLock);
}

/*
 * Provide a means for threadControl to ensure that crucial locks are not 
 * held by suspended threads.
 */
void 
eventHelper_lock() 
{
    debugMonitorEnter(commandQueueLock);
    debugMonitorEnter(commandCompleteLock);
}

void 
eventHelper_unlock() 
{
    debugMonitorExit(commandCompleteLock);
    debugMonitorExit(commandQueueLock);
}


/*
 * For events handled in a separate thread, the local refs must be converted to
 * global refs. These functions manage that conversion.
 */
static void 
makeRefsGlobal(JNIEnv *env, JVMDI_Event *event)
{
    (*env)->ExceptionClear(env);

    switch (event->kind) {
        case JVMDI_EVENT_SINGLE_STEP:
            event->u.single_step.thread = (*env)->NewGlobalRef(env, event->u.single_step.thread);
            event->u.single_step.clazz = (*env)->NewGlobalRef(env, event->u.single_step.clazz);
            break;
    
        case JVMDI_EVENT_BREAKPOINT:
            event->u.breakpoint.thread = (*env)->NewGlobalRef(env, event->u.breakpoint.thread);
            event->u.breakpoint.clazz = (*env)->NewGlobalRef(env, event->u.breakpoint.clazz);
            break;
    
        case JVMDI_EVENT_FRAME_POP:
            event->u.frame.thread = (*env)->NewGlobalRef(env, event->u.frame.thread);
            break;
    
        case JVMDI_EVENT_FIELD_MODIFICATION: {
            char sig = event->u.field_modification.signature_type;
            event->u.field_modification.thread = (*env)->NewGlobalRef(env, event->u.field_modification.thread);
            event->u.field_modification.clazz = (*env)->NewGlobalRef(env, event->u.field_modification.clazz);
            event->u.field_modification.field_clazz = (*env)->NewGlobalRef(env, event->u.field_modification.field_clazz);
            event->u.field_modification.object = (*env)->NewGlobalRef(env, event->u.field_modification.object);
            if ((sig == JDWP_Tag_ARRAY) || (sig == JDWP_Tag_OBJECT)) {
                /* an object type */
                event->u.field_modification.new_value.l =  
                    (*env)->NewGlobalRef(
                        env, 
                        event->u.field_modification.new_value.l);
            }
            break;
        }
    
        case JVMDI_EVENT_FIELD_ACCESS:
            event->u.field_access.thread = (*env)->NewGlobalRef(env, event->u.field_access.thread);
            event->u.field_access.clazz = (*env)->NewGlobalRef(env, event->u.field_access.clazz);
            event->u.field_access.field_clazz = (*env)->NewGlobalRef(env, event->u.field_access.field_clazz);
            event->u.field_access.object = (*env)->NewGlobalRef(env, event->u.field_access.object);  /* STATIC ??? */
            break;
    
        case JVMDI_EVENT_EXCEPTION:
            event->u.exception.thread = (*env)->NewGlobalRef(env, event->u.exception.thread);
            event->u.exception.clazz = (*env)->NewGlobalRef(env, event->u.exception.clazz);
            event->u.exception.exception = (*env)->NewGlobalRef(env, event->u.exception.exception);
            event->u.exception.catch_clazz = (*env)->NewGlobalRef(env, event->u.exception.catch_clazz);
            break;
    
        case JVMDI_EVENT_EXCEPTION_CATCH:
            event->u.exception_catch.thread = (*env)->NewGlobalRef(env, event->u.exception.thread);
            event->u.exception_catch.clazz = (*env)->NewGlobalRef(env, event->u.exception.clazz);
            event->u.exception_catch.exception = (*env)->NewGlobalRef(env, event->u.exception.exception);
            break;
    
        case JVMDI_EVENT_USER_DEFINED:
            event->u.user.object = (*env)->NewGlobalRef(env, event->u.user.object);
            break;
    
        case JVMDI_EVENT_THREAD_START:
        case JVMDI_EVENT_THREAD_END:
            event->u.thread_change.thread = (*env)->NewGlobalRef(env, event->u.thread_change.thread);
            break;
    
        case JVMDI_EVENT_METHOD_ENTRY:
        case JVMDI_EVENT_METHOD_EXIT:   
            event->u.frame.thread = 
                (*env)->NewGlobalRef(env, event->u.frame.thread);
            event->u.frame.clazz = 
                (*env)->NewGlobalRef(env, event->u.frame.clazz);
            break;

        case JVMDI_EVENT_CLASS_LOAD:
        case JVMDI_EVENT_CLASS_PREPARE:
            event->u.class_event.thread = (*env)->NewGlobalRef(env, event->u.class_event.thread);
            event->u.class_event.clazz = (*env)->NewGlobalRef(env, event->u.class_event.clazz);
            break;
    
        case JVMDI_EVENT_CLASS_UNLOAD:
            /*
             * We shouldn't ever try to create global refs for this event.
             */
            break;
    }

    if ((*env)->ExceptionOccurred(env)) {
        ERROR_MESSAGE_EXIT("Unable to create global references for event processing");
    }
}

struct bag *
eventHelper_createEventBag()
{
    return bagCreateBag(sizeof(CommandSingle), 5 /* events */ );
}

/* Return the combined suspend policy for the event set
 */
static jboolean
enumForCombinedSuspendPolicy(void *cv, void *arg)
{
    CommandSingle *command = cv;
    jbyte thisPolicy;
    jbyte *policy = arg;

    switch(command->singleKind) {
        case COMMAND_SINGLE_EVENT:
            thisPolicy = command->u.eventCommand.suspendPolicy;
            break;
        case COMMAND_SINGLE_FRAME_EVENT:
            thisPolicy = command->u.frameEventCommand.suspendPolicy;
            break;
        default:
            thisPolicy = JDWP_SuspendPolicy_NONE;
    }
    /* Expand running policy value if this policy demands it */
    if (*policy == JDWP_SuspendPolicy_NONE) {
        *policy = thisPolicy;
    } else if (*policy == JDWP_SuspendPolicy_EVENT_THREAD) {
        *policy = (thisPolicy == JDWP_SuspendPolicy_ALL)? 
                        thisPolicy : *policy;
    }

    /* Short circuit if we reached maximal suspend policy */
    if (*policy == JDWP_SuspendPolicy_ALL) {
        return JNI_FALSE;
    } else {
        return JNI_TRUE;
    }
}

/* Determine whether we are reporting VM death
 */
static jboolean
enumForVMDeath(void *cv, void *arg)
{
    CommandSingle *command = cv;
    jboolean *reportingVMDeath = arg;

    if (command->singleKind == COMMAND_SINGLE_EVENT) {
        if (command->u.eventCommand.event.kind == JVMDI_EVENT_VM_DEATH) {
            *reportingVMDeath = JNI_TRUE;
            return JNI_FALSE;
        }
    }
    return JNI_TRUE;
}

struct singleTracker {
    ReportEventCompositeCommand *recc;
    int index;
};

static jboolean
enumForCopyingSingles(void *command, void *tv)
{
    struct singleTracker *tracker = (struct singleTracker *)tv;
    memcpy(&tracker->recc->singleCommand[tracker->index++], 
           command, 
           sizeof(CommandSingle));
    return JNI_TRUE;
}

jbyte
eventHelper_reportEvents(jbyte sessionID, struct bag *eventBag)
{
    int size = bagSize(eventBag);
    jbyte suspendPolicy = JDWP_SuspendPolicy_NONE;
    jboolean reportingVMDeath = JNI_FALSE;
    jboolean wait;
     
    HelperCommand *command;
    ReportEventCompositeCommand *recc;
    struct singleTracker tracker;

    if (size == 0) {
        return suspendPolicy;
    }
    bagEnumerateOver(eventBag, enumForCombinedSuspendPolicy, &suspendPolicy);
    bagEnumerateOver(eventBag, enumForVMDeath, &reportingVMDeath);

    command = jdwpAlloc(sizeof(HelperCommand) + (sizeof(CommandSingle)*size));
    command->commandKind = COMMAND_REPORT_EVENT_COMPOSITE;
    command->sessionID = sessionID;
    recc = &command->u.reportEventComposite;
    recc->suspendPolicy = suspendPolicy;
    recc->eventCount = size;
    tracker.recc = recc;
    tracker.index = 0;
    bagEnumerateOver(eventBag, enumForCopyingSingles, &tracker);

    /*
     * We must wait if this thread (the event thread) is to be 
     * suspended or if the VM is about to die. (Waiting in the latter
     * case ensures that we get the event out before the process dies.)
     */
    wait = (jboolean)((suspendPolicy != JDWP_SuspendPolicy_NONE) || 
                      reportingVMDeath);
    enqueueCommand(command, wait, reportingVMDeath);
    return suspendPolicy;
}

void 
eventHelper_recordEvent(JVMDI_Event *event, jint id, jbyte suspendPolicy,
                         struct bag *eventBag)
{
    JNIEnv *env = getEnv();
    CommandSingle *command = bagAdd(eventBag);
    if (command == NULL) {
        ALLOC_ERROR_EXIT();
    }

    command->singleKind = COMMAND_SINGLE_EVENT;
    command->u.eventCommand.suspendPolicy = suspendPolicy;
    command->u.eventCommand.id = id;

    /*
     * Copy the event into the command so that it can be used 
     * asynchronously by the event helper thread.
     */
    memcpy(&command->u.eventCommand.event, event, sizeof(*event));
    makeRefsGlobal(env, &command->u.eventCommand.event);
}

void 
eventHelper_recordClassUnload(jint id, char *signature, struct bag *eventBag)
{
    CommandSingle *command = bagAdd(eventBag);
    if (command == NULL) {
        ALLOC_ERROR_EXIT();
    }
    command->singleKind = COMMAND_SINGLE_UNLOAD;
    command->u.unloadCommand.id = id;
    command->u.unloadCommand.classSignature = signature;
}

void 
eventHelper_recordFrameEvent(jint id, jbyte suspendPolicy, jbyte kind,
                             jthread thread, jclass clazz, 
                             jmethodID method, jlocation location,
                             struct bag *eventBag)
{
    JNIEnv *env = getEnv();
    FrameEventCommandSingle *frameCommand;
    CommandSingle *command = bagAdd(eventBag);
    if (command == NULL) {
        ALLOC_ERROR_EXIT();
    }

    command->singleKind = COMMAND_SINGLE_FRAME_EVENT;
    frameCommand = &command->u.frameEventCommand;
    frameCommand->suspendPolicy = suspendPolicy;
    frameCommand->id = id;
    frameCommand->kind = kind;
    frameCommand->thread = (*env)->NewGlobalRef(env, thread);
    frameCommand->clazz = (*env)->NewGlobalRef(env, clazz);
    frameCommand->method = method;
    frameCommand->location = location;
    if ((frameCommand->thread == NULL) || (frameCommand->clazz == NULL)) {
        ALLOC_ERROR_EXIT();
    }
}

void 
eventHelper_reportInvokeDone(jbyte sessionID, jthread thread)
{
    JNIEnv *env = getEnv();
    HelperCommand *command = jdwpAlloc(sizeof(*command));
    if (command == NULL) {
        ALLOC_ERROR_EXIT();
    }
    command->commandKind = COMMAND_REPORT_INVOKE_DONE;
    command->sessionID = sessionID;
    command->u.reportInvokeDone.thread = (*env)->NewGlobalRef(env, thread);
    if (command->u.reportInvokeDone.thread == NULL) {
        ERROR_MESSAGE_EXIT("Unable to create global reference for invocation completion");
    }
    enqueueCommand(command, JNI_TRUE, JNI_FALSE);
}

/*
 * This, currently, cannot go through the normal event handling code
 * because the JVMDI event does not contain a thread.
 */
void 
eventHelper_reportVMInit(jbyte sessionID, jthread thread, jbyte suspendPolicy)
{
    JNIEnv *env = getEnv();
    HelperCommand *command = jdwpAlloc(sizeof(*command));
    if (command == NULL) {
        ALLOC_ERROR_EXIT();
    }
    command->commandKind = COMMAND_REPORT_VM_INIT;
    command->sessionID = sessionID;
    command->u.reportVMInit.thread = (*env)->NewGlobalRef(env, thread);
    command->u.reportVMInit.suspendPolicy = suspendPolicy;
    if (command->u.reportVMInit.thread == NULL) {
        ERROR_MESSAGE_EXIT("Unable to create global reference for vm init");
    }
    enqueueCommand(command, JNI_TRUE, JNI_FALSE);
}

void
eventHelper_suspendThread(jbyte sessionID, jthread thread)
{
    JNIEnv *env = getEnv();
    HelperCommand *command = jdwpAlloc(sizeof(*command));
    if (command == NULL) {
        ALLOC_ERROR_EXIT();
    }
    command->commandKind = COMMAND_SUSPEND_THREAD;
    command->sessionID = sessionID;
    command->u.suspendThread.thread = (*env)->NewGlobalRef(env, thread);
    if (command->u.suspendThread.thread == NULL) {
        ERROR_MESSAGE_EXIT("Unable to create global reference for thread suspension");
    }
    enqueueCommand(command, JNI_TRUE, JNI_FALSE);
}


