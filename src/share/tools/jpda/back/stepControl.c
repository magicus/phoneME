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
#include <stdio.h>
#include <jvmdi.h>
#include "stepControl.h"
#include "eventHandler.h"
#include "eventHelper.h"
#include "threadControl.h"
#include "util.h"
#include "JDWP.h"
#include "SDE.h"

static JVMDI_RawMonitor stepLock;

/*
 * Useful for debugging
 */
/*
static void printFrame(JNIEnv *env, char *prefix, jthread thread)
{
    jclass clazz;
    jmethodID method;
    jlocation location;
    jint error;
    char *csig;
    char *mname;
    char *msig;
    JVMDI_thread_info info;
    jint frameCount;
    jframeID frame;

    error = jvmdi->GetCurrentFrame(thread, &frame);
    if (error != JVMDI_ERROR_NONE) {
        return;
    }


    error = jvmdi->GetFrameLocation(frame, &clazz, &method, &location);
    if (error != JVMDI_ERROR_NONE) {
        return;
    }

    error = jvmdi->GetFrameCount(thread, &frameCount);
    if (error != JVMDI_ERROR_NONE) {
        return;
    }

    error = jvmdi->GetClassSignature(clazz, &csig);
    if (error != JVMDI_ERROR_NONE) {
        return;
    }

    error = jvmdi->GetMethodName(clazz, method, &mname, &msig);
    if (error != JVMDI_ERROR_NONE) {
        return;
    }

    error = jvmdi->GetThreadInfo(thread, &info);
    if (error != JVMDI_ERROR_NONE) {
        return;
    }

    fprintf(stderr, "%sthread=%s,method=%s.%s,location=%d,depth=%d\n",
            prefix, info.name, csig, mname, (int)location, frameCount);


    jdwpFree(info.name);
    jdwpFree(csig);
    jdwpFree(msig);
    jdwpFree(mname);
    (*env)->DeleteGlobalRef(env,info.thread_group);
    if (info.context_class_loader != NULL) {
        (*env)->DeleteGlobalRef(env,info.context_class_loader);
    }
    (*env)->DeleteGlobalRef(env,clazz);
}
*/

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

/*                                                              
 * Most enabling/disabling of JVMDI events happens implicitly through 
 * the inserting and freeing of handlers for those events. Stepping is
 * different because requested steps are usually not identical to JVMDI steps. 
 * They usually require multiple events step, and otherwise, before they 
 * complete. While a step request is pending, we may need to temporarily
 * disable and re-enable stepping, but we can't just remove the handlers
 * because that would break the application's ability to remove the 
 * events. So, for step events only, we directly enable and disable stepping.
 * This is safe because there can only ever be one pending step request 
 * per thread. 
 */
static void 
enableStepping(jthread thread)
{
    jint error = threadControl_setEventMode(JVMDI_ENABLE, 
                                            JVMDI_EVENT_SINGLE_STEP,
                                            thread);
    if (error != JVMDI_ERROR_NONE) {
        ERROR_CODE_EXIT(error);
    }
}

static void 
disableStepping(jthread thread)
{
    jint error = threadControl_setEventMode(JVMDI_DISABLE, 
                                            JVMDI_EVENT_SINGLE_STEP,
                                            thread);
    if (error != JVMDI_ERROR_NONE) {
        ERROR_CODE_EXIT(error);
    }
}

static jint 
findLineNumber(JNIEnv *env, jthread thread, jframeID frame, 
               JVMDI_line_number_entry *lines, jint count) 
{
    int i;
    jint line = -1;
    jclass clazz;
    jmethodID method;
    jlocation location;
    jint error;

    error = threadControl_getFrameLocation(thread, frame, 
                                  &clazz, &method, &location);
    if (error == JVMDI_ERROR_NONE) {
        (*env)->DeleteGlobalRef(env,clazz);
    }
    if ((error == JVMDI_ERROR_NONE) && (location != -1)) {
        if (count > 0) {
            /* any preface before first line is assigned to first line */
            for (i=1; i<count; i++) {
                if (location < lines[i].start_location) {
                    break;
                }
            }
            line = lines[i-1].line_number;
        }
    } else {
        ERROR_EXIT("Unable to get frame location", error);
    }

    return line;
}

static jboolean
hasLineNumbers(JNIEnv *env, jframeID frame)
{
    jclass clazz;
    jmethodID method;
    jlocation location;
    jint error;
    jint count;
    JVMDI_line_number_entry *table;

    /* Safe to use, despite 4215724, because returned location not used */
    error = jvmdi->GetFrameLocation(frame, &clazz, &method, &location);
    if (error != JVMDI_ERROR_NONE) {
        ERROR_EXIT("Unable to get frame location", error);
    }

    error = jvmdi->GetLineNumberTable(clazz, method, &count, &table);
    if (error == JVMDI_ERROR_NONE) {
        convertLineNumberTable(env, clazz, &count, &table);
        if (count == 0) {
            /* had line numbers, but they don't exist in default stratum */
            error = JVMDI_ERROR_ABSENT_INFORMATION;
        }
        jdwpFree(table);
    }

    (*env)->DeleteGlobalRef(env, clazz);

    return (error == JVMDI_ERROR_NONE);
}


static jint
initState(JNIEnv *env, jthread thread, StepRequest *step)
{
    jint error;
    jframeID frame;
    jclass clazz;
    jmethodID method;
    jlocation location;

    /*
     * Initial values that may be changed below
     */
    step->fromLine = -1;
    step->lineEntryCount = 0;
    step->fromNative = JNI_FALSE; 
    step->frameExited = JNI_FALSE;
    step->fromStackDepth = getStackDepth(thread);

    error = jvmdi->GetCurrentFrame(thread, &frame);
    if (error == JVMDI_ERROR_NO_MORE_FRAMES) {
        /*
         * If there are no stack frames, treat the step as though
         * from a native frame. This is most likely to occur at the 
         * beginning of a debug session, right after the VM_INIT event,
         * so we need to do something intelligent.
         */
        step->fromNative = JNI_TRUE;
        return JVMDI_ERROR_NONE;
    } else if (error != JVMDI_ERROR_NONE) {
        return error;
    }

    /*error = jvmdi->GetMethodModifiers(clazz, method, &modifiers);
    if (error != JVMDI_ERROR_NONE) {
        goto done;
    }
    step->fromNative = modifiers & MOD_NATIVE;*/


    /*
     * Try to get a notification on frame pop. If we're in an opaque frame
     * we won't be able to, but we can use other methods to detect that
     * a native frame has exited.
     *
     * %comment gordonh017
     */
    error = jvmdi->NotifyFramePop(frame);
    if (error == JVMDI_ERROR_OPAQUE_FRAME) {
        step->fromNative = JNI_TRUE;
        error = JVMDI_ERROR_NONE;
        /* continue without error */
        
    } else if (error == JVMDI_ERROR_DUPLICATE) {
        error = JVMDI_ERROR_NONE;
        /* Already being notified, continue without error */
        
    } else if (error != JVMDI_ERROR_NONE) {
        return error;
    }

    /*
     * Note: we can't undo the frame pop notify, so
     * we'll just have to let the handler ignore it if 
     * there are any errors below.
     */

    if (step->granularity == JDWP_StepSize_LINE) {
        error = threadControl_getFrameLocation(thread, frame, 
                                      &clazz, &method, &location);
        if (error != JVMDI_ERROR_NONE) {
            return error;
        }
    
        if (step->lineEntries != NULL) {
            /* free line entries from previous round */
            jdwpFree(step->lineEntries);
            step->lineEntries = NULL;
        }

        if (location != -1) {
            /* An error here does not get returned to caller, we can continue */
            jint lineError = jvmdi->GetLineNumberTable(clazz, method, 
                                             &step->lineEntryCount, 
                                             &step->lineEntries);
            if (lineError == JVMDI_ERROR_NONE) {
                convertLineNumberTable(env, clazz,
                                       &step->lineEntryCount, 
                                       &step->lineEntries);
                step->fromLine = findLineNumber(env, thread, frame,
                                     step->lineEntries, step->lineEntryCount);
            }
        }
        (*env)->DeleteGlobalRef(env,clazz);
    }
    
    return error;
}

/* %comment gordonh018 */
static void 
handleFramePopEvent(JNIEnv *env, JVMDI_Event *event, 
                    HandlerNode *node, 
                    struct bag *eventBag)
{
    StepRequest *step;
    jthread thread = event->u.frame.thread;

    debugMonitorEnter(stepLock);

    /*printFrame(env, "FP:",thread);*/
    step = threadControl_getStepRequest(thread);
    if (step == NULL) {
        ERROR_CODE_EXIT(JVMDI_ERROR_INVALID_THREAD);
    }

    if (step->pending) {
        /*                                
         * Note: current depth is reported as *before* the pending frame
         * pop. 
         */
        jint currentDepth = getStackDepth(thread);
        jint fromDepth = step->fromStackDepth;

        /*
         * If we are exiting the original stepping frame, record that 
         * fact here. Once the next step event comes in, we can safely
         * stop stepping there. 
         */
        if (fromDepth > currentDepth - 1) {
            step->frameExited = JNI_TRUE;
        }

        if (step->depth == JDWP_StepDepth_OVER) {
            /*
             * Either 
             * 1) the original stepping frame is about to be popped
             *    [fromDepth == currentDepth]. Re-enable stepping to
             *    reach a point where we can stop.
             * 2) a method called from the stepping frame has returned 
             *    (during which we had stepping disabled)
             *    [fromDepth == currentDepth - 1]. Re-enable stepping 
             *    so that we can continue instructions steps in the 
             *    original stepping frame.  
             * 3) a method further down the call chain has notified
             *    of a frame pop [fromDepth < currentDepth - 1]. This
             *    *might* represent case (2) above if the stepping frame
             *    was calling a native method which in turn called a 
             *    java method. If so, we must enable stepping to 
             *    ensure that we get control back after the intervening
             *    native frame is popped (you can't get frame pop 
             *    notifications on native frames). If the native caller
             *    calls another Java method before returning, 
             *    stepping will be diabled again and another frame pop
             *    will be awaited. 
             *
             *    If it turns out that this is not case (2) with native
             *    methods, then the enabled stepping is benign and 
             *    will be disabled again on the next step event. 
             *
             * Note that the condition not covered above, 
             * [fromDepth > currentDepth] shouldn't happen since it means
             * that too many frames have been popped. For robustness, 
             * we enable stepping in that case too, so that the errant
             * step-over can be stopped. 
             *
             */
            enableStepping(thread);
        } else if ((step->depth == JDWP_StepDepth_OUT) && (fromDepth > currentDepth - 1)) {
            /*
             * The original stepping frame is about to be popped. Step
             * until we reach the next safe place to stop.
             */
            enableStepping(thread);
        } else if ((step->methodEnterHandlerNode != NULL) && (fromDepth >= currentDepth - 1)) {
            /*
             * We installed a method entry event handler as part of a 
             * step into operation. We've popped back to the original
             * stepping frame without finding a place to stop.
             * Resume stepping in the original frame.
             */
            enableStepping(thread);
            eventHandler_free(step->methodEnterHandlerNode);
            step->methodEnterHandlerNode = NULL;
        }
    }

    debugMonitorExit(stepLock);
}

static void 
handleExceptionCatchEvent(JNIEnv *env, JVMDI_Event *event, 
                          HandlerNode *node,
                          struct bag *eventBag)
{
    StepRequest *step;
    jthread thread = event->u.exception_catch.thread;

    debugMonitorEnter(stepLock);

    /*printFrame(env, "EC:", thread);*/
    step = threadControl_getStepRequest(thread);
    if (step == NULL) {
        ERROR_CODE_EXIT(JVMDI_ERROR_INVALID_THREAD);
    }

    if (step->pending) {
        /*
         *  Determine where we are on the call stack relative to where
         *  we started.
         */
        jint currentDepth = getStackDepth(thread);
        jint fromDepth = step->fromStackDepth;

        /*
         * If we are exiting the original stepping frame, record that 
         * fact here. Once the next step event comes in, we can safely
         * stop stepping there. 
         */
        if (fromDepth > currentDepth) {
            step->frameExited = JNI_TRUE;
        }

        if ((step->depth == JDWP_StepDepth_OVER) && (fromDepth >= currentDepth)) {
            /*
             * Either the original stepping frame is done, 
             * or a called method has returned (during which we had stepping
             * disabled). In either case we must resume stepping.
             */
            enableStepping(thread);
        } else if ((step->depth == JDWP_StepDepth_OUT) && (fromDepth > currentDepth)) {
            /*
             * The original stepping frame is done. Step
             * until we reach the next safe place to stop.
             */
            enableStepping(thread);
        } else if ((step->methodEnterHandlerNode != NULL) && (fromDepth >= currentDepth)) {
            /*
             * We installed a method entry event handler as part of a 
             * step into operation. We've popped back to the original
             * stepping frame or higher without finding a place to stop.
             * Resume stepping in the original frame.
             */
            enableStepping(thread);
            eventHandler_free(step->methodEnterHandlerNode);
            step->methodEnterHandlerNode = NULL;
        }
    }

    debugMonitorExit(stepLock);
}

static void 
handleMethodEnterEvent(JNIEnv *env, JVMDI_Event *event, 
                       HandlerNode *node,
                       struct bag *eventBag)
{
    StepRequest *step;
    jint error;
    jthread thread = event->u.frame.thread;
    jframeID frame;

    debugMonitorEnter(stepLock);

    /*printFrame(env, "ME:",thread);*/
    step = threadControl_getStepRequest(thread);
    if (step == NULL) {
        ERROR_CODE_EXIT(JVMDI_ERROR_INVALID_THREAD);
    }

    if (step->pending) {
        /*
         * This handler is relevant only to step into
         */
        JDI_ASSERT(step->depth == JDWP_StepDepth_INTO);
    
        error = jvmdi->GetCurrentFrame(thread, &frame);
        if (error != JVMDI_ERROR_NONE) {
            ERROR_CODE_EXIT(error);
        }

        if (((step->granularity != JDWP_StepSize_LINE) || hasLineNumbers(env, frame)) 
              && !eventFilter_predictFiltering(step->stepHandlerNode, frame)) {
            /*
             * We've found a suitable method in which to stop. Step
             * until we reach the next safe location to complete the step->,
             * and we can get rid of the method entry handler.
             */
            enableStepping(thread);
            eventHandler_free(step->methodEnterHandlerNode);
            step->methodEnterHandlerNode = NULL;
        }
    }

    debugMonitorExit(stepLock);
}

static void 
completeStep(JNIEnv *env, JVMDI_Event *event, StepRequest *step)
{
    jthread thread = event->u.single_step.thread;
    jint error;

    /*
     * We've completed a step; reset state for the next one, if any
     */

    if (step->methodEnterHandlerNode != NULL) {
        eventHandler_free(step->methodEnterHandlerNode);
        step->methodEnterHandlerNode = NULL;
    }

    error = initState(env, thread, step);
    if (error != JVMDI_ERROR_NONE) {
        /*
         * None of the initState errors should happen after one step
         * has successfully completed. 
         */
        ERROR_CODE_EXIT(error);
    }
}

jboolean 
stepControl_handleStep(JNIEnv *env, JVMDI_Event *event)
{
    StepRequest *step;
    jframeID frame;
    jint line;
    jint currentDepth;
    jint fromDepth;
    jthread thread = event->u.single_step.thread;
    jboolean completed = JNI_FALSE;
    jint error;

    debugMonitorEnter(stepLock);

    /*printFrame(env, "ST:",thread);*/
    step = threadControl_getStepRequest(thread);
    if (step == NULL) {
        ERROR_CODE_EXIT(JVMDI_ERROR_INVALID_THREAD);
    }

    /*
     * If no step is currently pending, ignore the event
     */
    if (!step->pending) {
        goto done;
    }

    /*
     * We never filter step into instruction. It's always over on the 
     * first step event.
     */
    if ((step->depth == JDWP_StepDepth_INTO) && (step->granularity == JDWP_StepSize_MIN)) {
        completed = JNI_TRUE;
        goto done;
    }

    /*
     * If we have left the method in which 
     * stepping started, the step is always complete.
     */
    if (step->frameExited) {
        completed = JNI_TRUE;
        goto done;
    }

    /*
     *  Determine where we are on the call stack relative to where
     *  we started.
     */
    currentDepth = getStackDepth(thread);
    fromDepth = step->fromStackDepth;

    if (currentDepth < fromDepth) {
        /*
         * We have returned from the caller. There are cases where 
         * we don't get frame pop notifications
         * (e.g. stepping from opaque frames), and that's when 
         * this code will be reached. Complete the step->
         */
        completed = JNI_TRUE;
        goto done;
    } else if (currentDepth > fromDepth) {
        error = jvmdi->GetCurrentFrame(thread, &frame);
        if (error != JVMDI_ERROR_NONE) {
            ERROR_CODE_EXIT(error);
        }

        /* We have dropped into a called method. */
        if ((step->depth == JDWP_StepDepth_INTO) &&
            hasLineNumbers(env, frame) &&
            !eventFilter_predictFiltering(step->stepHandlerNode, frame)) {

            /* Stepped into a method with lines, so we're done */
            completed = JNI_TRUE;
            goto done;
        } else {
            /*
             * We need to continue, but don't want the overhead of step
             * events from this method. So, we disable stepping and 
             * enable a frame pop. If we're stepping into, we also
             * enable method enter events because a called frame may be
             * where we want to stop.
             */
            disableStepping(thread);

            if (step->depth == JDWP_StepDepth_INTO) {
                step->methodEnterHandlerNode =
                    eventHandler_createInternalThreadOnly(
                                       JVMDI_EVENT_METHOD_ENTRY, 
                                       handleMethodEnterEvent, thread);
                if (step->methodEnterHandlerNode == NULL) {
                    ERROR_MESSAGE_EXIT("Unable to install event handler");
                }
            }

            error = jvmdi->NotifyFramePop(frame);
            if (error == JVMDI_ERROR_DUPLICATE) {
                error = JVMDI_ERROR_NONE;
            } else if (error != JVMDI_ERROR_NONE) {
                ERROR_CODE_EXIT(error);
            }
            goto done;
        }
    } else {
        /*
         * We are at the same stack depth where stepping started.
         * Instruction steps are complete at this point. For line
         * steps we must check to see whether we've moved to a 
         * different line.
         */
        if (step->granularity == JDWP_StepSize_MIN) {
            completed = JNI_TRUE;
            goto done;
        } else {
            error = jvmdi->GetCurrentFrame(thread, &frame);
            if (error != JVMDI_ERROR_NONE) {
                ERROR_CODE_EXIT(error);
            }
            if (step->fromLine != -1) {
                line = findLineNumber(env, thread, frame,
                                      step->lineEntries, step->lineEntryCount);
                if (line != step->fromLine) {
                    completed = JNI_TRUE;
                }
                goto done;
            } else {
                /*
                 * This is a rare case. We have stepped from a location
                 * inside a native method to a location within a Java
                 * method at the same stack depth. This means that 
                 * the original native method returned to another 
                 * native method which, in turn, invoked a Java method.
                 * 
                 * Since the original frame was  native, we were unable
                 * to ask for a frame pop event, and, thus, could not
                 * set the step->frameExited flag when the original
                 * method was done. Instead we end up here
                 * and act just as though the frameExited flag was set
                 * and complete the step immediately.
                 */
                completed = JNI_TRUE;
                goto done;
            }
        }
    }
done:
    if (completed) {
        completeStep(env, event, step);
    }
    debugMonitorExit(stepLock);
    return completed;
}


void 
stepControl_initialize(void) 
{
    stepLock = debugMonitorCreate("JDWP Step Handler Lock");
}

void 
stepControl_reset(void) 
{
}

void
initEvents(JNIEnv *env, jthread thread, StepRequest *step)
{
    /* Need to install frame pop handler and exception catch handler when
     * single-stepping is enabled (i.e. step-into or step-over/step-out when fromStackDepth > 0).
     */
    if (step->depth == JDWP_StepDepth_INTO || step->fromStackDepth > 0) {
        /*
         * %comment gordonh019
         */
        step->catchHandlerNode = eventHandler_createInternalThreadOnly(
                                     JVMDI_EVENT_EXCEPTION_CATCH, 
                                     handleExceptionCatchEvent, 
                                     thread);
        step->framePopHandlerNode = eventHandler_createInternalThreadOnly(
                                        JVMDI_EVENT_FRAME_POP, 
                                        handleFramePopEvent, 
                                        thread);
    
        if ((step->catchHandlerNode == NULL) || 
            (step->framePopHandlerNode == NULL)) {
            ERROR_MESSAGE_EXIT("Unable to install event handlers");
        }

    }
    /*
     * Initially enable stepping:
     * 1) For step into, always
     * 2) For step over, unless right after the VM_INIT or line stepping
     *    from a method without line  numbers
     * 3) For step out, only if stepping from native, except right after VM_INIT
     *
     * (right after VM_INIT, a step->over or out is identical to running 
     * forever)
     */
    switch (step->depth) {
        case JDWP_StepDepth_INTO:
            enableStepping(thread);
            break;
        case JDWP_StepDepth_OVER:
            if ((step->fromStackDepth > 0) && 
                !((step->granularity == JDWP_StepSize_LINE) && 
                  !step->fromNative && 
                  (step->fromLine == -1))) {
                enableStepping(thread);
            }
            break;
        case JDWP_StepDepth_OUT:
            if (step->fromNative &&
                (step->fromStackDepth > 0)) {
                enableStepping(thread);
            }
            break;
        default:
            JDI_ASSERT(JNI_FALSE);
    }
}

jint 
stepControl_beginStep(jthread thread, jint size, jint depth, 
                      HandlerNode *node) 
{
    StepRequest *step;
    jint error;
    jint error2;
    JNIEnv *env = getEnv();

    eventHandler_lock(); /* for proper lock order */
    debugMonitorEnter(stepLock);

    step = threadControl_getStepRequest(thread);
    if (step == NULL) {
        error = JVMDI_ERROR_INVALID_THREAD;
    } else {
        /*
         * In case the thread isn't already suspended, do it again.
         */
        error = threadControl_suspendThread(thread, JNI_FALSE);
        if (error == JVMDI_ERROR_NONE) {
            /*
             * Overwrite any currently executing step.
             */
            step->granularity = size;
            step->depth = depth;
            step->catchHandlerNode = NULL;
            step->framePopHandlerNode = NULL;
            step->methodEnterHandlerNode = NULL;
            step->stepHandlerNode = node;
            error = initState(env, thread, step);
            if (error == JVMDI_ERROR_NONE) {
                initEvents(env, thread, step);
            }
            /* false means it is not okay to unblock the commandLoop thread */
            error2 = threadControl_resumeThread(thread, JNI_FALSE);
            if ((error2 != JVMDI_ERROR_NONE) &&(error == JVMDI_ERROR_NONE)) {
                error = error2;
            }

            /*
             * If everything went ok, indicate a step is pending.
             */
            if (error == JVMDI_ERROR_NONE) {
                step->pending = JNI_TRUE;
            }
        }

    }

    debugMonitorExit(stepLock);
    eventHandler_unlock();

    return error;
}


static void
clearStep(jthread thread, StepRequest *step)
{
    if (step->pending) {
        disableStepping(thread);
        eventHandler_free(step->catchHandlerNode);
        eventHandler_free(step->framePopHandlerNode);
        eventHandler_free(step->methodEnterHandlerNode);
        step->pending = JNI_FALSE;
        if (step->lineEntries != NULL) {
            jdwpFree(step->lineEntries);
            step->lineEntries = NULL;
        }
    }
}

jint 
stepControl_endStep(jthread thread) 
{
    StepRequest *step;
    jint error;

    eventHandler_lock(); /* for proper lock order */
    debugMonitorEnter(stepLock);

    step = threadControl_getStepRequest(thread);
    if (step != NULL) {
        clearStep(thread, step);
        error = JVMDI_ERROR_NONE;
    } else {
        error = JVMDI_ERROR_INVALID_THREAD;
    }

    debugMonitorExit(stepLock);
    eventHandler_unlock();

    return error;
}

void 
stepControl_clearRequest(jthread thread, StepRequest *step)
{
    clearStep(thread, step);
}

void
stepControl_lock()
{
    debugMonitorEnter(stepLock);
}

void
stepControl_unlock()
{
    debugMonitorExit(stepLock);
}

