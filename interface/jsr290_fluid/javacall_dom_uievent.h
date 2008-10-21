/*
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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

#ifndef __JAVACALL_DOM_UIEVENT_H_
#define __JAVACALL_DOM_UIEVENT_H_

/**
 * @file javacall_dom_uievent.h
 * @ingroup JSR290DOM
 * @brief Javacall DOM interfaces for UIEvent
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <javacall_dom.h>

/**
 * @defgroup JSR290DOM JSR290 DOM API
 *
 * The following API definitions are required by DOM part of the JSR-290.
 *
 * @{
 */

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code OR
 * returns  Specifies some detail information about the <code>Event</code>, 
 * depending on the type of event. 
 * 
 * @param handle Pointer to the object representing this uievent.
 * @param invocationId Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_uievent_get_detail_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_uievent_get_detail_start(javacall_handle handle,
                                      javacall_int32 invocationId,
                                      void **context,
                                      /* OUT */ javacall_int32* ret_value);

/**
 * Returns  Specifies some detail information about the <code>Event</code>, 
 * depending on the type of event. 
 * 
 * @param handle Pointer to the object representing this uievent.
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_uievent_get_detail_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_uievent_get_detail_finish(javacall_handle handle,
                                       void *context,
                                       /* OUT */ javacall_int32* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code OR
 *  The <code>initUIEvent</code> method is used to initialize the value of 
 * a <code>UIEvent</code> object and has the same behavior as 
 * <code>Event.initEvent()</code>. 
 * 
 * @param handle Pointer to the object representing this uievent.
 * @param invocationId Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param type_arg  Refer to the <code>Event.initEvent()</code> method for 
 *   a description of this parameter. 
 * @param can_bubble_arg  Refer to the <code>Event.initEvent()</code> 
 *   method for a description of this parameter. 
 * @param cancelable_arg  Refer to the <code>Event.initEvent()</code> 
 *   method for a description of this parameter. 
 * @param detail_arg  Specifies <code>UIEvent.detail</code>.   
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_uievent_init_ui_event_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_uievent_init_ui_event_start(javacall_handle handle,
                                         javacall_int32 invocationId,
                                         void **context,
                                         javacall_const_utf16_string type_arg,
                                         javacall_bool can_bubble_arg,
                                         javacall_bool cancelable_arg,
                                         javacall_int32 detail_arg);

/**
 *  The <code>initUIEvent</code> method is used to initialize the value of 
 * a <code>UIEvent</code> object and has the same behavior as 
 * <code>Event.initEvent()</code>. 
 * 
 * @param handle Pointer to the object representing this uievent.
 * @param context The context saved during asynchronous operation.
 * @param type_arg  Refer to the <code>Event.initEvent()</code> method for 
 *   a description of this parameter. 
 * @param can_bubble_arg  Refer to the <code>Event.initEvent()</code> 
 *   method for a description of this parameter. 
 * @param cancelable_arg  Refer to the <code>Event.initEvent()</code> 
 *   method for a description of this parameter. 
 * @param detail_arg  Specifies <code>UIEvent.detail</code>.   
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_uievent_init_ui_event_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_uievent_init_ui_event_finish(javacall_handle handle,
                                          void *context,
                                          javacall_const_utf16_string type_arg,
                                          javacall_bool can_bubble_arg,
                                          javacall_bool cancelable_arg,
                                          javacall_int32 detail_arg);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code OR
 *  The <code>initUIEventNS</code> method is used to initialize the value 
 * of a <code>UIEvent</code> object and has the same behavior as 
 * <code>Event.initEventNS()</code>. 
 * 
 * @param handle Pointer to the object representing this uievent.
 * @param invocationId Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param namespace_uri  Refer to the <code>Event.initEventNS()</code> 
 *   method for a description of this parameter. 
 * @param type_arg  Refer to the <code>Event.initEventNS()</code> method 
 *   for a description of this parameter. 
 * @param can_bubble_arg  Refer to the <code>Event.initEventNS()</code> 
 *   method for a description of this parameter. 
 * @param cancelable_arg  Refer to the <code>Event.initEventNS()</code> 
 *   method for a description of this parameter. 
 * @param detail_arg  Refer to the <code>UIEvent.initUIEvent()</code> 
 *   method for a description of this parameter.
 *
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_uievent_init_ui_event_ns_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_uievent_init_ui_event_ns_start(javacall_handle handle,
                                            javacall_int32 invocationId,
                                            void **context,
                                            javacall_const_utf16_string namespace_uri,
                                            javacall_const_utf16_string type_arg,
                                            javacall_bool can_bubble_arg,
                                            javacall_bool cancelable_arg,
                                            javacall_int32 detail_arg);

/**
 *  The <code>initUIEventNS</code> method is used to initialize the value 
 * of a <code>UIEvent</code> object and has the same behavior as 
 * <code>Event.initEventNS()</code>. 
 * 
 * @param handle Pointer to the object representing this uievent.
 * @param context The context saved during asynchronous operation.
 * @param namespace_uri  Refer to the <code>Event.initEventNS()</code> 
 *   method for a description of this parameter. 
 * @param type_arg  Refer to the <code>Event.initEventNS()</code> method 
 *   for a description of this parameter. 
 * @param can_bubble_arg  Refer to the <code>Event.initEventNS()</code> 
 *   method for a description of this parameter. 
 * @param cancelable_arg  Refer to the <code>Event.initEventNS()</code> 
 *   method for a description of this parameter. 
 * @param detail_arg  Refer to the <code>UIEvent.initUIEvent()</code> 
 *   method for a description of this parameter.
 *
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_uievent_init_ui_event_ns_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_uievent_init_ui_event_ns_finish(javacall_handle handle,
                                             void *context,
                                             javacall_const_utf16_string namespace_uri,
                                             javacall_const_utf16_string type_arg,
                                             javacall_bool can_bubble_arg,
                                             javacall_bool cancelable_arg,
                                             javacall_int32 detail_arg);

/** 
 * Decrements ref counter of the native object specified number of times
 * 
 * @param handle Pointer to the object representing this node.
 * @param count number of times to decrement.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_uievent_clear_references(javacall_handle handle, javacall_int32 count);


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ifndef __JAVACALL_DOM_UIEVENT_H_ */
