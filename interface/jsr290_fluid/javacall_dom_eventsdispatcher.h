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

#ifndef __JAVACALL_dom_eventsdispatcher_H_
#define __JAVACALL_dom_eventsdispatcher_H_

/**
 * @file javacall_dom_eventsdispatcher.h
 * @ingroup JSR290DOM
 * @brief Javacall DOM interfaces for Node
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <javacall_dom.h>

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR this method allows the registration of event listeners on the event 
 * target. If an <code>EventListener</code> is added to an 
 * <code>EventTarget</code> while it is processing an event, it will not 
 * be triggered by the current actions but may be triggered during a 
 * later stage of event flow, such as the bubbling phase. 
 * <br> If multiple identical <code>EventListener</code>s are registered 
 * on the same <code>EventTarget</code> with the same parameters the 
 * duplicate instances are discarded. They do not cause the 
 * <code>EventListener</code> to be called twice and since they are 
 * discarded they do not need to be removed with the 
 * <code>removeEventListener</code> method. 
 * 
 * @param handle Pointer to the object representing this node.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param namespaceURI The namespace URI associated with this event at 
 *              initialization time, or null if it is unspecified.
 * @param type The event type for which the user is registering
 * @param use_capture If true, <code>use_capture</code> indicates that the 
 *   user wishes to initiate capture. After initiating capture, all 
 *   events of the specified type will be dispatched to the registered 
 *   <code>EventListener</code> before being dispatched to any 
 *   <code>EventTargets</code> beneath them in the tree. Events which 
 *   are bubbling upward through the tree will not trigger an 
 *   <code>EventListener</code> designated to use capture.
 * @param ret_value The <code>ret_value</code> parameter points to the
 *   variable which will receive the handle to EventListener
 *   registration.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_eventsdispatcher_add_event_listener_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_eventsdispatcher_add_event_listener_start(javacall_handle app_id,
                                           javacall_handle handle,
                                           javacall_int32 invocation_id,
                                           void **context,
                                           javacall_const_utf16_string namespaceURI,
                                           javacall_const_utf16_string type,
                                           javacall_bool use_capture,
                                           /*OUT*/javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR this method allows the registration of event listeners on the event 
 * target. If an <code>EventListener</code> is added to an 
 * <code>EventTarget</code> while it is processing an event, it will not 
 * be triggered by the current actions but may be triggered during a 
 * later stage of event flow, such as the bubbling phase. 
 * <br> If multiple identical <code>EventListener</code>s are registered 
 * on the same <code>EventTarget</code> with the same parameters the 
 * duplicate instances are discarded. They do not cause the 
 * <code>EventListener</code> to be called twice and since they are 
 * discarded they do not need to be removed with the 
 * <code>removeEventListener</code> method. 
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value The <code>ret_value</code> parameter points to the
 *   variable which will receive the handle to EventListener
 *   registration.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_eventsdispatcher_add_event_listener_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_eventsdispatcher_add_event_listener_finish(void *context,
                                            /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR this method allows the removal of event listeners from the event 
 * target. If an <code>EventListener</code> is removed from an 
 * <code>EventTarget</code> while it is processing an event, it will not 
 * be triggered by the current actions. <code>EventListener</code>s can 
 * never be invoked after being removed.
 * <br>Calling <code>removeEventListener</code> with arguments which do 
 * not identify any currently registered <code>EventListener</code> on 
 * the <code>EventTarget</code> has no effect.
 * 
 * @param handle Pointer to the object representing this node.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param namespaceURI The namespace URI associated with this event at 
 *              initialization time, or null if it is unspecified.
 * @param type Specifies the event type of the <code>EventListener</code> 
 *   being removed. 
 * @param listener The <code>EventListener</code> parameter indicates the 
 *   <code>EventListener </code> to be removed. 
 * @param useCapture Specifies whether the <code>EventListener</code> 
 *   being removed was registered as a capturing listener or not. If a 
 *   listener was registered twice, one with capture and one without, 
 *   each must be removed separately. Removal of a capturing listener 
 *   does not affect a non-capturing version of the same listener, and 
 *   vice versa. 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_eventsdispatcher_remove_event_listener_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_eventsdispatcher_remove_event_listener_start(javacall_handle handle,
                                              javacall_int32 invocation_id,
                                              void **context,
                                              javacall_const_utf16_string namespaceURI,
                                              javacall_const_utf16_string type,
                                              javacall_bool use_capture,
                                              javacall_handle listener);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR this method allows the removal of event listeners from the event 
 * target. If an <code>EventListener</code> is removed from an 
 * <code>EventTarget</code> while it is processing an event, it will not 
 * be triggered by the current actions. <code>EventListener</code>s can 
 * never be invoked after being removed.
 * <br>Calling <code>removeEventListener</code> with arguments which do 
 * not identify any currently registered <code>EventListener</code> on 
 * the <code>EventTarget</code> has no effect.
 * 
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_eventsdispatcher_remove_event_listener_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_eventsdispatcher_remove_event_listener_finish(void *context);


/**
 * Indicates the completion of handle event request processing by
 * Java application
 *
 * @param request pointer to native HandleEventRequest class
 *
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if an error encountered
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_eventsdispatcher_handle_completed(javacall_handle request);

/**
 * Helper function that send handle request to Java part
 *
 * @param request_handle pointer to native HandleEventRequest class
 *
 */
void 
javanotify_fluid_handle_event_request (
    javacall_handle request_handle,
    javacall_handle appId
    );

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ifndef __JAVACALL_dom_eventsdispatcher_H_ */
