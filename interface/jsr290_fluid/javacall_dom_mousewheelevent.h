/*
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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

#ifndef __JAVACALL_DOM_MOUSEWHEELEVENT_H_
#define __JAVACALL_DOM_MOUSEWHEELEVENT_H_

/**
 * @file javacall_dom_mousewheelevent.h
 * @ingroup JSR290DOM
 * @brief Javacall DOM interfaces for MouseWheelEvent
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
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns indicates the number of "clicks" the wheel has been rotated. A positive 
 * value indicates that the wheel has been rotated away from the user 
 * (or in a right-hand manner on horizontally aligned devices) and a 
 * negative value indicates that the wheel has been rotated towards the 
 * user (or in a left-hand manner on horizontally aligned devices).
 *
 * <p>A "click" is defined to be a unit of rotation. On some devices this 
 * is a finite physical step. On devices with smooth rotation, a "click" 
 * becomes the smallest measurable amount of rotation.</p>
 * 
 * @param handle Pointer to the object representing this mousewheelevent.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_mousewheelevent_get_wheel_delta_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_mousewheelevent_get_wheel_delta_start(javacall_handle handle,
                                                   javacall_int32 invocation_id,
                                                   void **context,
                                                   /* OUT */ javacall_int32* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns indicates the number of "clicks" the wheel has been rotated. A positive 
 * value indicates that the wheel has been rotated away from the user 
 * (or in a right-hand manner on horizontally aligned devices) and a 
 * negative value indicates that the wheel has been rotated towards the 
 * user (or in a left-hand manner on horizontally aligned devices).
 *
 * <p>A "click" is defined to be a unit of rotation. On some devices this 
 * is a finite physical step. On devices with smooth rotation, a "click" 
 * becomes the smallest measurable amount of rotation.</p>
 * 
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_mousewheelevent_get_wheel_delta_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_mousewheelevent_get_wheel_delta_finish(void *context,
                                                    /* OUT */ javacall_int32* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR the <code>initMouseWheelEventNS</code> method is used to initialize the 
 * value of a <code>MouseWheelEvent</code> object and has the same
 * behavior as <code>Event.initEventNS()</code>. 
 *
 * For <code>mousewheel</code>, <code>MouseEvent.getRelatedTarget</code>
 * must indicate the element over which the pointer is located, or
 * <code>NULL</code> if there is no such element (in the case where the
 * device does not have a pointer, but does have a wheel). 
 *
 * 
 * @param handle Pointer to the object representing this mousewheelevent.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param namespace_uri_arg  Refer to the <code>Event.initEventNS()</code> 
 *   method for a description of this parameter. 
 * @param type_arg  Refer to the <code>Event.initEventNS()</code> method 
 *   for a description of this parameter. 
 * @param can_bubble_arg  Refer to the <code>Event.initEventNS()</code> 
 *   method for a description of this parameter. 
 * @param cancelable_arg  Refer to the <code>Event.initEventNS()</code> 
 *   method for a description of this parameter. 
 * @param view_arg Pointer to the object of
 *    Refer to the <code>UIEvent.initUIEvent()</code> method 
 *   for a description of this parameter. 
 * @param detail_arg  Refer to the <code>UIEvent.initUIEvent()</code> 
 *   method for a description of this parameter.
 * @param screen_x_arg Refer to the <code>MouseEvent.initMouseEventNS()</code>
 *   method for a description of this parameter.
 * @param screen_y_arg Refer to the <code>MouseEvent.initMouseEventNS()</code>
 *   method for a description of this parameter.
 * @param client_x_arg Refer to the <code>MouseEvent.initMouseEventNS()</code>
 *   method for a description of this parameter.
 * @param client_y_arg Refer to the <code>MouseEvent.initMouseEventNS()</code>
 *   method for a description of this parameter.
 * @param button_arg Refer to the <code>MouseEvent.initMouseEventNS()</code>
 *   method for a description of this parameter.
 * @param related_target_arg Pointer to the object of
 *   refer to the <code>MouseEvent.initMouseEventNS()</code>
 *   method for a description of this parameter.
 * @param ctrl_key_arg  Specifies <code>MouseEvent.ctrlKey</code>. 
 * @param alt_key_arg  Specifies <code>MouseEvent.altKey</code>. 
 * @param shift_key_arg  Specifies <code>MouseEvent.shiftKey</code>. 
 * @param meta_key_arg  Specifies <code>MouseEvent.metaKey</code>. 
 * @param wheel_delta_arg  A number indicating the distance in "clicks"
 *   (positive means rotated away from the user, negative means rotated
 *   towards the user). The default value of the wheelDelta attribute is 0. 
 *
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_mousewheelevent_init_mouse_wheel_event_ns_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_mousewheelevent_init_mouse_wheel_event_ns_start(javacall_handle handle,
                                                             javacall_int32 invocation_id,
                                                             void **context,
                                                             javacall_const_utf16_string namespace_uri_arg,
                                                             javacall_const_utf16_string type_arg,
                                                             javacall_bool can_bubble_arg,
                                                             javacall_bool cancelable_arg,
                                                             javacall_handle view_arg,
                                                             javacall_int32 detail_arg,
                                                             javacall_int32 screen_x_arg,
                                                             javacall_int32 screen_y_arg,
                                                             javacall_int32 client_x_arg,
                                                             javacall_int32 client_y_arg,
                                                             javacall_int16 button_arg,
                                                             javacall_handle related_target_arg,
                                                             javacall_bool ctrl_key_arg,
                                                             javacall_bool alt_key_arg,
                                                             javacall_bool shift_key_arg,
                                                             javacall_bool meta_key_arg,
                                                             javacall_int32 wheel_delta_arg);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR the <code>initMouseWheelEventNS</code> method is used to initialize the 
 * value of a <code>MouseWheelEvent</code> object and has the same
 * behavior as <code>Event.initEventNS()</code>. 
 *
 * For <code>mousewheel</code>, <code>MouseEvent.getRelatedTarget</code>
 * must indicate the element over which the pointer is located, or
 * <code>NULL</code> if there is no such element (in the case where the
 * device does not have a pointer, but does have a wheel). 
 *
 * 
 * @param context The context saved during asynchronous operation.
 * @param ctrl_key_arg  Specifies <code>MouseEvent.ctrlKey</code>. 
 * @param alt_key_arg  Specifies <code>MouseEvent.altKey</code>. 
 * @param shift_key_arg  Specifies <code>MouseEvent.shiftKey</code>. 
 * @param meta_key_arg  Specifies <code>MouseEvent.metaKey</code>. 
 *   (positive means rotated away from the user, negative means rotated
 *   towards the user). The default value of the wheelDelta attribute is 0. 
 *
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_mousewheelevent_init_mouse_wheel_event_ns_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_mousewheelevent_init_mouse_wheel_event_ns_finish(void *context);

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
javacall_dom_mousewheelevent_clear_references(javacall_handle handle, javacall_int32 count);


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ifndef __JAVACALL_DOM_MOUSEWHEELEVENT_H_ */
