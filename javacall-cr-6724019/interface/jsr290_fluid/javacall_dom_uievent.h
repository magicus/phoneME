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
 * Returns  The <code>view</code> attribute identifies the 
 * <code>AbstractView</code> from which the event was generated. 
 * 
 * @param handle Pointer to the object representing this uievent.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_uievent_get_view(javacall_handle handle,
                              /* OUT */ javacall_handle* ret_value);

/**
 * Returns  Specifies some detail information about the <code>Event</code>, 
 * depending on the type of event. 
 * 
 * @param handle Pointer to the object representing this uievent.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_uievent_get_detail(javacall_handle handle,
                                /* OUT */ javacall_int32* ret_value);

/**
 *  The <code>initUIEvent</code> method is used to initialize the value of 
 * a <code>UIEvent</code> object and has the same behavior as 
 * <code>Event.initEvent()</code>. 
 * 
 * @param handle Pointer to the object representing this uievent.
 * @param type_arg  Refer to the <code>Event.initEvent()</code> method for 
 *   a description of this parameter. 
 * @param can_bubble_arg  Refer to the <code>Event.initEvent()</code> 
 *   method for a description of this parameter. 
 * @param cancelable_arg  Refer to the <code>Event.initEvent()</code> 
 *   method for a description of this parameter. 
 * @param detail_arg  Specifies <code>UIEvent.detail</code>.   
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_uievent_init_ui_event(javacall_handle handle,
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
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_uievent_init_ui_event_ns(javacall_handle handle,
                                      javacall_const_utf16_string namespace_uri,
                                      javacall_const_utf16_string type_arg,
                                      javacall_bool can_bubble_arg,
                                      javacall_bool cancelable_arg,
                                      javacall_int32 detail_arg);

/** 
 * Deletes object representing this uievent
 * 
 * @param handle Pointer to the object representing this uievent.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_uievent_finalize(javacall_handle handle);


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ifndef __JAVACALL_DOM_UIEVENT_H_ */
