/*
 * @(#)eventFilter.h	1.5 06/10/10
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

#ifndef _EVENT_FILTER_H
#define _EVENT_FILTER_H

#include <jni.h>
#include <jvmdi.h>
#include "eventHandler.h"

/***** filter set-up *****/

jint eventFilter_setConditionalFilter(HandlerNode *node,
                                      jint index, jint exprID);
jint eventFilter_setCountFilter(HandlerNode *node,
                                jint index, jint count);
jint eventFilter_setThreadOnlyFilter(HandlerNode *node,
                                     jint index, jthread thread);
jint eventFilter_setLocationOnlyFilter(HandlerNode *node,
                                       jint index, 
                                       jclass clazz, 
                                       jmethodID method, 
                                       jlocation location);
jint eventFilter_setFieldOnlyFilter(HandlerNode *node,
                                    jint index, 
                                    jclass clazz, 
                                    jfieldID field);
jint eventFilter_setClassOnlyFilter(HandlerNode *node,
                                    jint index, 
                                    jclass clazz);
jint eventFilter_setExceptionOnlyFilter(HandlerNode *node,
                                        jint index, 
                                        jclass exceptionClass, 
                                        jboolean caught, 
                                        jboolean uncaught);
jint eventFilter_setInstanceOnlyFilter(HandlerNode *node,
                                       jint index, 
                                       jobject object);
jint eventFilter_setClassMatchFilter(HandlerNode *node,
                                     jint index, 
                                     char *classPattern);
jint eventFilter_setClassExcludeFilter(HandlerNode *node,
                                       jint index, 
                                       char *classPattern);
jint eventFilter_setStepFilter(HandlerNode *node,
                               jint index, 
                               jthread thread, 
                               jint size, jint depth);

/***** misc *****/

jboolean eventFilter_predictFiltering(HandlerNode *node, jframeID frame);

#endif /* _EVENT_FILTER_H */
