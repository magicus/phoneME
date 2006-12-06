/*
 * @(#)threadControl.h	1.39 06/10/10
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
#include <jni.h>
#include "stepControl.h"
#include "invoker.h"
#include "bag.h"

void threadControl_initialize(void);
void threadControl_reset(void);
void threadControl_detachInvokes(void);

void threadControl_onHook(void);
void threadControl_onConnect(void);
void threadControl_onDisconnect(void);
struct bag *threadControl_onEventHandlerEntry(jbyte sessionID, 
                                              jint kind, jthread thread);
void threadControl_onEventHandlerExit(jint kind, jthread thread, struct bag *);


jint threadControl_suspendThread(jthread thread, jboolean deferred);
jint threadControl_resumeThread(jthread thread, jboolean do_unblock);
jint threadControl_suspendCount(jthread thread, jint *count);

jint threadControl_suspendAll(void);
jint threadControl_resumeAll(void);

StepRequest *threadControl_getStepRequest(jthread);
InvokeRequest *threadControl_getInvokeRequest(jthread);

jboolean threadControl_isDebugThread(jthread thread);
jint threadControl_addDebugThread(jthread thread);
jint threadControl_removeDebugThread(jthread thread);
void threadControl_joinAllDebugThreads(void);

jint threadControl_applicationThreadStatus(jthread thread, jint *threadStatus, jint *suspendStatus);
jint threadControl_interrupt(jthread thread);
jint threadControl_stop(jthread thread, jobject throwable);

jint threadControl_setEventMode(jint mode, jint event, jthread thread);
jint threadControl_getInstructionStepMode(jthread thread);

jint threadControl_getFrameLocation(jthread thread, jframeID frame,
                      jclass *clazz, jmethodID *method, jlocation *location);
jthread threadControl_currentThread(void);
void threadControl_setPendingInterrupt(jthread thread);



