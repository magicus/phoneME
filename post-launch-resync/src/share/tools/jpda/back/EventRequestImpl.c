/*
 * @(#)EventRequestImpl.c	1.36 06/10/10
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
#include "EventRequestImpl.h"
#include "eventHandler.h"
#include "util.h"
#include "inStream.h"
#include "outStream.h"
#include "JDWP.h"
#include "stepControl.h"

#define JDWPEventType_to_JVMDIEventKind(eventType) (eventType)
    
#define ERROR_RETURN(func) \
{jvmdiError error = (func); if (error != JVMDI_ERROR_NONE) return error; }

/**
 * Take JDWP "modifiers" (which are JDI explicit filters, like
 * addCountFilter(), and implicit filters, like the LocationOnly
 * filter that goes with breakpoints) and add them as filters
 * (eventFilter) to the HandlerNode (eventHandler).
 */
static jvmdiError
readAndSetFilters(PacketInputStream *in, HandlerNode *node, 
                  jint filterCount)
{
    jint i;

    for (i = 0; i < filterCount; ++i) {
        jbyte modifier = inStream_readByte(in);
        switch (modifier) {
            case JDWP_REQUEST_MODIFIER(Conditional): {
                jint exprID = inStream_readInt(in);
                ERROR_RETURN(inStream_error(in));
                ERROR_RETURN(eventFilter_setConditionalFilter(node, i, 
                                                               exprID));
                break;
            }

            case JDWP_REQUEST_MODIFIER(Count): {
                jint count = inStream_readInt(in);
                ERROR_RETURN(inStream_error(in));
                ERROR_RETURN(eventFilter_setCountFilter(node, i, 
                                                            count));
                break;
            }

            case JDWP_REQUEST_MODIFIER(ThreadOnly): {
                jthread thread = inStream_readThreadRef(in);
                ERROR_RETURN(inStream_error(in));
                ERROR_RETURN(eventFilter_setThreadOnlyFilter(node, i, 
                                                                thread));
                break;
            }

            case JDWP_REQUEST_MODIFIER(LocationOnly): {
                jclass clazz;
                jmethodID method;
                jlocation location;

                inStream_readByte(in); /* Discard unused tag byte. */
                clazz = inStream_readClassRef(in);
                method = inStream_readMethodID(in);
                location = inStream_readLocation(in);
                ERROR_RETURN(inStream_error(in));
                ERROR_RETURN(eventFilter_setLocationOnlyFilter(node, i,
                                             clazz, method, location));
                break;
            }

            case JDWP_REQUEST_MODIFIER(FieldOnly): {
                jclass clazz = inStream_readClassRef(in);
                jfieldID field = inStream_readFieldID(in);
                ERROR_RETURN(inStream_error(in));
                ERROR_RETURN(eventFilter_setFieldOnlyFilter(node, i,
                                                      clazz, field));
                break;
            }

            case JDWP_REQUEST_MODIFIER(ClassOnly): {
                jclass clazz = inStream_readClassRef(in);
                ERROR_RETURN(inStream_error(in));
                ERROR_RETURN(eventFilter_setClassOnlyFilter(node, i, clazz));
                break;
            }

            case JDWP_REQUEST_MODIFIER(ExceptionOnly): {
                jclass exception = inStream_readClassRef(in);
                jboolean caught = inStream_readBoolean(in);
                jboolean uncaught = inStream_readBoolean(in);
                ERROR_RETURN(inStream_error(in));
                ERROR_RETURN(eventFilter_setExceptionOnlyFilter(node, i,
                                             exception, caught, uncaught));
                break;
            }

            case JDWP_REQUEST_MODIFIER(InstanceOnly): {
                jobject instance = inStream_readObjectRef(in);
                ERROR_RETURN(inStream_error(in));
                ERROR_RETURN(eventFilter_setInstanceOnlyFilter(node, i,
								instance));
                break;
            }

            case JDWP_REQUEST_MODIFIER(ClassMatch): {
                char *pattern = inStream_readString(in);
                ERROR_RETURN(inStream_error(in));
                ERROR_RETURN(eventFilter_setClassMatchFilter(node, i,
                                                                pattern));
                break;
            }

            case JDWP_REQUEST_MODIFIER(ClassExclude): {
                char *pattern = inStream_readString(in);
                ERROR_RETURN(inStream_error(in));
                ERROR_RETURN(eventFilter_setClassExcludeFilter(node, i,
                                                                 pattern));
                break;
            }
            case JDWP_REQUEST_MODIFIER(Step): {
                jthread thread = inStream_readThreadRef(in);
                jint size = inStream_readInt(in);
                jint depth = inStream_readInt(in);
                ERROR_RETURN(inStream_error(in));
                ERROR_RETURN(eventFilter_setStepFilter(node, i,
                                                 thread, size, depth));
                break;
            }

            default:
                return JVMDI_ERROR_ILLEGAL_ARGUMENT;
        }
    }
    return JVMDI_ERROR_NONE;
}

/**
 * This is the back-end implementation for enabling
 * (what are at the JDI level) EventRequests.
 *
 * Allocate the event request handler (eventHandler).
 * Add any filters (explicit or implicit).
 * Install the handler.
 * Return the handlerID which is used to map subsequent 
 * events to the EventRequest that created it.
 */
static jboolean 
setCommand(PacketInputStream *in, PacketOutputStream *out) 
{
    jint error;
    jbyte kind;
    HandlerNode *node;
    jbyte eventType = inStream_readByte(in);
    jbyte suspendPolicy = inStream_readByte(in);
    jint filterCount = inStream_readInt(in);

    if (inStream_error(in)) {
        return JNI_TRUE;
    }

    kind = JDWPEventType_to_JVMDIEventKind(eventType);
    node = eventHandler_alloc(filterCount, kind, suspendPolicy);
    if (node == NULL) {
        outStream_setError(out, JVMDI_ERROR_OUT_OF_MEMORY);
        return JNI_TRUE;
    }

    error = readAndSetFilters(in, node, filterCount);
    if (error == JVMDI_ERROR_NONE) {
        error = eventHandler_installExternal(node);
    }

    if (error == JVMDI_ERROR_NONE) {
        outStream_writeInt(out, node->handlerID);
    } else {
        eventHandler_free(node);
        outStream_setError(out, error);
    }

    return JNI_TRUE;
}

/**
 * This is the back-end implementation for disabling
 * (what are at the JDI level) EventRequests.
 */
static jboolean
clearCommand(PacketInputStream *in, PacketOutputStream *out) 
{
    jint error = JVMDI_ERROR_NONE;
    jbyte kind;
    jbyte eventType = inStream_readByte(in);
    HandlerID handlerID = inStream_readInt(in);

    if (inStream_error(in)) {
        return JNI_TRUE;
    }

    kind = JDWPEventType_to_JVMDIEventKind(eventType);

    error = eventHandler_freeByID(kind, handlerID);

    if (error != JVMDI_ERROR_NONE) {
        outStream_setError(out, error);
    }

    return JNI_TRUE;
}

static jboolean 
clearAllBreakpoints(PacketInputStream *in, PacketOutputStream *out) 
{
    jint error = eventHandler_freeAll(JVMDI_EVENT_BREAKPOINT);
    if (error != JVMDI_EVENT_BREAKPOINT) {
        outStream_setError(out, error);
    }
    return JNI_TRUE;
}

void *EventRequest_Cmds[] = { (void *)0x3
    ,(void *)setCommand
    ,(void *)clearCommand
    ,(void *)clearAllBreakpoints};

