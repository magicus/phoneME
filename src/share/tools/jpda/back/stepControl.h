/*
 * @(#)stepControl.h	1.23 06/10/10
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
#ifndef STEP_CONTROL_H
#define STEP_CONTROL_H

#include <jni.h>
#include <jvmdi.h>
#include "eventFilter.h"
#include "eventHandler.h"

typedef struct {
    /* Parameters */
    jint granularity;
    jint depth;

    /* State */
    jboolean pending;
    jboolean frameExited;    /* for depth == STEP_OVER or STEP_OUT */
    jboolean fromNative;
    jint fromStackDepth;     /* for all but STEP_INTO STEP_INSTRUCTION */
    jint fromLine;           /* for granularity == STEP_LINE */
    JVMDI_line_number_entry *lineEntries;       /* STEP_LINE */
    jint lineEntryCount;     /* for granularity == STEP_LINE */

    HandlerNode *stepHandlerNode;
    HandlerNode *catchHandlerNode;
    HandlerNode *framePopHandlerNode;
    HandlerNode *methodEnterHandlerNode;
} StepRequest;


void stepControl_initialize(void);
void stepControl_reset(void);
void stepControl_OnHook(void);

jboolean stepControl_handleStep(JNIEnv *env, JVMDI_Event *event);

jint stepControl_beginStep(jthread thread, jint size, jint depth,
                           HandlerNode *node);
jint stepControl_endStep(jthread thread);

void stepControl_clearRequest(jthread thread, StepRequest *step);

void stepControl_lock(void);
void stepControl_unlock(void);

#endif

