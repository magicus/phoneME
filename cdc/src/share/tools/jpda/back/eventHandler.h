/*
 * @(#)eventHandler.h	1.39 06/10/10
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

#ifndef _EVENT_HANDLER_H
#define _EVENT_HANDLER_H

#include <jni.h>
#include <jvmdi.h>
#include "bag.h"

typedef jint HandlerID;

/* structure is read-only for users */
typedef struct HandlerNode_ {    
    HandlerID handlerID;
    jbyte kind;
    jbyte suspendPolicy;
    /* private members follow */
} HandlerNode;

typedef void (*HandlerFunction)(JNIEnv *env,
                                JVMDI_Event *event, 
                                HandlerNode *node, 
                                struct bag *eventBag);

/***** HandlerNode create = alloc + install *****/

HandlerNode *eventHandler_alloc(jint filterCount, jbyte kind, 
                                jbyte suspendPolicy);
jint eventHandler_installExternal(HandlerNode *node);
HandlerNode *eventHandler_createInternal(jbyte kind, HandlerFunction func);
HandlerNode *eventHandler_createInternalThreadOnly(jbyte kind, 
                                                   HandlerFunction func,
                                                   jthread thread);
HandlerNode *eventHandler_createInternalBreakpoint(HandlerFunction func,
                                                   jthread thread,
                                                   jclass clazz, 
                                                   jmethodID method, 
                                                   jlocation location);

/***** HandlerNode free *****/

jint eventHandler_freeAll(jint kind);
jint eventHandler_freeByID(jint kind, HandlerID handlerID);
jint eventHandler_free(HandlerNode *node);
void eventHandler_freeClassBreakpoints(jclass clazz);

/***** HandlerNode manipulation *****/

void eventHandler_initialize(jbyte sessionID);
void eventHandler_reset(jbyte sessionID);

void eventHandler_lock(void);
void eventHandler_unlock(void);

/***** event redirection *****/
typedef jboolean (*EventRedirectHook)(JVMDI_Event *event, 
                                      jthread thread);
extern jthread redirectedEventThread;
extern EventRedirectHook redirectedEventFunction;

#endif /* _EVENT_HANDLER_H */
