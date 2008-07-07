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
 * Returns indicates the number of "clicks" the wheel has been rotated. A positive 
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
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_mousewheelevent_get_wheel_delta(javacall_handle handle,
                                             /* OUT */ javacall_int32* ret_value);

/**
 * The <code>initMouseWheelEventNS</code> method is used to initialize the 
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
 * @param namespace_uri_arg  Refer to the <code>Event.initEventNS()</code> 
 *   method for a description of this parameter. 
 * @param type_arg  Refer to the <code>Event.initEventNS()</code> method 
 *   for a description of this parameter. 
 * @param can_bubble_arg  Refer to the <code>Event.initEventNS()</code> 
 *   method for a description of this parameter. 
 * @param cancelable_arg  Refer to the <code>Event.initEventNS()</code> 
 *   method for a description of this parameter. 
 * @param viewArg  Refer to the <code>UIEvent.initUIEvent()</code> method 
 *   for a description of this parameter. 
 * @param detailArg  Refer to the <code>UIEvent.initUIEvent()</code> 
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
 * @param modifiers_list_arg Refer to the <code>MouseEvent.initMouseEventNS()</code>
 *   method for a description of this parameter.
 * @param wheel_delta_arg  A number indicating the distance in "clicks"
 *   (positive means rotated away from the user, negative means rotated
 *   towards the user). The default value of the wheelDelta attribute is 0. 
 *
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_mousewheelevent_init_mouse_wheel_event_ns(javacall_handle handle,
                                                       javacall_const_utf16_string namespace_uri_arg,
                                                       javacall_const_utf16_string type_arg,
                                                       javacall_bool can_bubble_arg,
                                                       javacall_bool cancelable_arg,
                                                       javacall_int32 screen_x_arg,
                                                       javacall_int32 screen_y_arg,
                                                       javacall_int32 client_x_arg,
                                                       javacall_int32 client_y_arg,
                                                       javacall_int16 button_arg,
                                                       javacall_handle related_target_arg,
                                                       javacall_const_utf16_string modifiers_list_arg,
                                                       javacall_int32 wheel_delta_arg);

/** 
 * Deletes object representing this mousewheelevent
 * 
 * @param handle Pointer to the object representing this mousewheelevent.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_mousewheelevent_finalize(javacall_handle handle);


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ifndef __JAVACALL_DOM_MOUSEWHEELEVENT_H_ */
