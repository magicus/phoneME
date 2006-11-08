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
#include <jni.h>
#include <jvmdi.h>
#include "bag.h"
#include "invoker.h"

void eventHelper_initialize(jbyte sessionID);
void eventHelper_reset(jbyte sessionID);
void eventHelper_shutdown(void);

struct bag *eventHelper_createEventBag();

void eventHelper_recordEvent(JVMDI_Event *event, jint id, 
                             jbyte suspendPolicy, struct bag *eventBag);
void eventHelper_recordClassUnload(jint id, char *signature, struct bag *eventBag);
void eventHelper_recordFrameEvent(jint id, jbyte suspendPolicy, jbyte kind,
                                  jthread thread, jclass clazz, 
                                  jmethodID method, jlocation location,
                                  struct bag *eventBag);

jbyte eventHelper_reportEvents(jbyte sessionID, struct bag *eventBag);
void eventHelper_reportInvokeDone(jbyte sessionID, jthread thread);
void eventHelper_reportVMInit(jbyte sessionID, jthread thread, jbyte suspendPolicy);
void eventHelper_suspendThread(jbyte sessionID, jthread thread);

void eventHelper_holdEvents(void);
void eventHelper_releaseEvents(void);

void eventHelper_lock(void);
void eventHelper_unlock(void);

/*
 * Private interface for coordinating between eventHelper.c: commandLoop()
 * and ThreadReferenceImpl.c: resume() and VirtualMachineImpl.c: resume().
 */
void unblockCommandLoop();
