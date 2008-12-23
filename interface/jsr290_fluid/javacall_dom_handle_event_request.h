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

#ifndef __JAVACALL_DOM_HANDLE_EVENT_REQUEST_H_
#define __JAVACALL_DOM_HANDLE_EVENT_REQUEST_H_

/**
 * @file javacall_dom_handle_event_request.h
 * @ingroup JSR290DOM
 * @brief Javacall DOM interfaces for HandleEventRequest
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <javacall_dom.h>

	/**
	 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code.
	 * 
	 * @param handle Pointer to the object representing this event.
	 * @param invocation_id Invocation identifier which MUST be used in the 
	 *                  corresponding javanotify function.
	 * @param context The context saved during asynchronous operation.
	 * 
	 * @return JAVACALL_WOULD_BLOCK caller must call the 
	 *             javacall_dom_event_get_current_target_finish function to complete the 
	 *             operation
	 */
	javacall_result
	javacall_dom_handle_event_get_event_listener_handle_start(javacall_handle handle,
	                                            javacall_int32 invocation_id,
	                                            void **context,
	                                            /* OUT */ javacall_handle* ret_value);
	/**
	 * Returns used to indicate the <code>EventListener</code> handle, which 
	 * represents native aprt of required event listenere 
	 * 
	 * @param context The context saved during asynchronous operation.
	 * 
	 * @return JAVACALL_OK if all done successfuly,
	 */
	javacall_result
	javacall_dom_handle_event_get_event_listener_handle_finish(void *context,
	                                             /* OUT */ javacall_handle* ret_value);

	/**
	 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code.
	 * 
	 * @param handle Pointer to the object representing this event.
	 * @param invocation_id Invocation identifier which MUST be used in the 
	 *                  corresponding javanotify function.
	 * @param context The context saved during asynchronous operation.
	 * 
	 * @return JAVACALL_WOULD_BLOCK caller must call the 
	 *             javacall_dom_event_get_namespace_uri_finish function to complete the 
	 *             operation
	 */
	javacall_result
	javacall_dom_handle_event_get_event_handle_start(javacall_handle handle,
	                                           javacall_int32 invocation_id,
	                                           void **context,
	                                           /* OUT */ javacall_handle* ret_value);
	/**
	 * Returns  the <code>Event</code> native handle which should be handled  
	 * in java part. 
	 * 
	 * @param context The context saved during asynchronous operation.
	 * 
	 * @return JAVACALL_OK if all done successfuly,
	 *         JAVACALL_OUT_OF_MEMORY if length of the returend string is more then 
	 *             specified in ret_value_len,
	 */
	javacall_result
	javacall_dom_handle_event_get_event_handle_finish(void *context,
	                                            /* OUT */ javacall_handle* ret_value);
#ifdef __cplusplus
}
#endif
	
#endif /* ifndef __JAVACALL_DOM_HANDLE_EVENT_REQUEST_H_*/
