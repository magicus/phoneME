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

#ifndef __JAVACALL_DOM_MOUSEEVENT_H_
#define __JAVACALL_DOM_MOUSEEVENT_H_

/**
 * @file javacall_dom_mouseevent.h
 * @ingroup JSR290DOM
 * @brief Javacall DOM interfaces for MouseEvent
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
 * OR returns  The horizontal coordinate at which the event occurred relative to the 
 * origin of the screen coordinate system. 
 * 
 * @param handle Pointer to the object representing this mouseevent.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_mouseevent_get_screen_x_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_mouseevent_get_screen_x_start(javacall_handle handle,
                                           javacall_int32 invocation_id,
                                           void **context,
                                           /* OUT */ javacall_int32* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns  The horizontal coordinate at which the event occurred relative to the 
 * origin of the screen coordinate system. 
 * 
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_mouseevent_get_screen_x_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_mouseevent_get_screen_x_finish(void *context,
                                            /* OUT */ javacall_int32* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns  The vertical coordinate at which the event occurred relative to the 
 * origin of the screen coordinate system. 
 * 
 * @param handle Pointer to the object representing this mouseevent.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_mouseevent_get_screen_y_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_mouseevent_get_screen_y_start(javacall_handle handle,
                                           javacall_int32 invocation_id,
                                           void **context,
                                           /* OUT */ javacall_int32* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns  The vertical coordinate at which the event occurred relative to the 
 * origin of the screen coordinate system. 
 * 
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_mouseevent_get_screen_y_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_mouseevent_get_screen_y_finish(void *context,
                                            /* OUT */ javacall_int32* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns  The horizontal coordinate at which the event occurred relative to the 
 * DOM implementation's client area. 
 * 
 * @param handle Pointer to the object representing this mouseevent.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_mouseevent_get_client_x_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_mouseevent_get_client_x_start(javacall_handle handle,
                                           javacall_int32 invocation_id,
                                           void **context,
                                           /* OUT */ javacall_int32* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns  The horizontal coordinate at which the event occurred relative to the 
 * DOM implementation's client area. 
 * 
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_mouseevent_get_client_x_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_mouseevent_get_client_x_finish(void *context,
                                            /* OUT */ javacall_int32* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns  The vertical coordinate at which the event occurred relative to the 
 * DOM implementation's client area. 
 * 
 * @param handle Pointer to the object representing this mouseevent.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_mouseevent_get_client_y_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_mouseevent_get_client_y_start(javacall_handle handle,
                                           javacall_int32 invocation_id,
                                           void **context,
                                           /* OUT */ javacall_int32* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns  The vertical coordinate at which the event occurred relative to the 
 * DOM implementation's client area. 
 * 
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_mouseevent_get_client_y_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_mouseevent_get_client_y_finish(void *context,
                                            /* OUT */ javacall_int32* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns  <code>true</code> if the control (Ctrl) key modifier is activated. 
 * 
 * @param handle Pointer to the object representing this mouseevent.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_mouseevent_get_ctrl_key_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_mouseevent_get_ctrl_key_start(javacall_handle handle,
                                           javacall_int32 invocation_id,
                                           void **context,
                                           /* OUT */ javacall_bool* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns  <code>true</code> if the control (Ctrl) key modifier is activated. 
 * 
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_mouseevent_get_ctrl_key_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_mouseevent_get_ctrl_key_finish(void *context,
                                            /* OUT */ javacall_bool* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns  <code>true</code> if the shift (Shift) key modifier is activated. 
 * 
 * @param handle Pointer to the object representing this mouseevent.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_mouseevent_get_shift_key_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_mouseevent_get_shift_key_start(javacall_handle handle,
                                            javacall_int32 invocation_id,
                                            void **context,
                                            /* OUT */ javacall_bool* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns  <code>true</code> if the shift (Shift) key modifier is activated. 
 * 
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_mouseevent_get_shift_key_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_mouseevent_get_shift_key_finish(void *context,
                                             /* OUT */ javacall_bool* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns  <code>true</code> if the alt (alternative) key modifier is activated. 
 * <p ><b>Note:</b>  The Option key modifier on Macintosh systems must be 
 * represented using this key modifier. 
 * 
 * @param handle Pointer to the object representing this mouseevent.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_mouseevent_get_alt_key_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_mouseevent_get_alt_key_start(javacall_handle handle,
                                          javacall_int32 invocation_id,
                                          void **context,
                                          /* OUT */ javacall_bool* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns  <code>true</code> if the alt (alternative) key modifier is activated. 
 * <p ><b>Note:</b>  The Option key modifier on Macintosh systems must be 
 * represented using this key modifier. 
 * 
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_mouseevent_get_alt_key_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_mouseevent_get_alt_key_finish(void *context,
                                           /* OUT */ javacall_bool* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns  <code>true</code> if the meta (Meta) key modifier is activated. 
 * <p ><b>Note:</b>  The Command key modifier on Macintosh system must be 
 * represented using this meta key. 
 * 
 * @param handle Pointer to the object representing this mouseevent.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_mouseevent_get_meta_key_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_mouseevent_get_meta_key_start(javacall_handle handle,
                                           javacall_int32 invocation_id,
                                           void **context,
                                           /* OUT */ javacall_bool* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns  <code>true</code> if the meta (Meta) key modifier is activated. 
 * <p ><b>Note:</b>  The Command key modifier on Macintosh system must be 
 * represented using this meta key. 
 * 
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_mouseevent_get_meta_key_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_mouseevent_get_meta_key_finish(void *context,
                                            /* OUT */ javacall_bool* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns  During mouse events caused by the depression or release of a mouse 
 * button, <code>button</code> is used to indicate which mouse button 
 * changed state. <code>0</code> indicates the normal button of the 
 * mouse (in general on the left or the one button on Macintosh mice, 
 * used to activate a button or select text). <code>2</code> indicates 
 * the contextual property (in general on the right, used to display a 
 * context menu) button of the mouse if present. <code>1</code> 
 * indicates the extra (in general in the middle and often combined with 
 * the mouse wheel) button. Some mice may provide or simulate more 
 * buttons, and values higher than <code>2</code> can be used to 
 * represent such buttons. 
 * 
 * @param handle Pointer to the object representing this mouseevent.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_mouseevent_get_button_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_mouseevent_get_button_start(javacall_handle handle,
                                         javacall_int32 invocation_id,
                                         void **context,
                                         /* OUT */ javacall_int16* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns  During mouse events caused by the depression or release of a mouse 
 * button, <code>button</code> is used to indicate which mouse button 
 * changed state. <code>0</code> indicates the normal button of the 
 * mouse (in general on the left or the one button on Macintosh mice, 
 * used to activate a button or select text). <code>2</code> indicates 
 * the contextual property (in general on the right, used to display a 
 * context menu) button of the mouse if present. <code>1</code> 
 * indicates the extra (in general in the middle and often combined with 
 * the mouse wheel) button. Some mice may provide or simulate more 
 * buttons, and values higher than <code>2</code> can be used to 
 * represent such buttons. 
 * 
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_mouseevent_get_button_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_mouseevent_get_button_finish(void *context,
                                          /* OUT */ javacall_int16* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns  Used to identify a secondary <code>EventTarget</code> related to a UI 
 * event, depending on the type of event. 
 * 
 * @param handle Pointer to the object representing this mouseevent.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_mouseevent_get_related_target_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_mouseevent_get_related_target_start(javacall_handle handle,
                                                 javacall_int32 invocation_id,
                                                 void **context,
                                                 /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns  Used to identify a secondary <code>EventTarget</code> related to a UI 
 * event, depending on the type of event. 
 * 
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_mouseevent_get_related_target_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_mouseevent_get_related_target_finish(void *context,
                                                  /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR  The <code>initMouseEvent</code> method is used to initialize the value 
 * of a <code>MouseEvent</code> object and has the same behavior as 
 * <code>UIEvent.initUIEvent()</code>. 
 * 
 * @param handle Pointer to the object representing this mouseevent.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param type_arg  Refer to the <code>UIEvent.initUIEvent()</code> method 
 *   for a description of this parameter. 
 * @param can_bubble_arg  Refer to the <code>UIEvent.initUIEvent()</code> 
 *   method for a description of this parameter. 
 * @param cancelable_arg  Refer to the <code>UIEvent.initUIEvent()</code> 
 *   method for a description of this parameter. 
 * @param view_arg Pointer to the object of
 *    Refer to the <code>UIEvent.initUIEvent()</code> method 
 *   for a description of this parameter. 
 * @param detail_arg  Refer to the <code>UIEvent.initUIEvent()</code> 
 *   method for a description of this parameter. 
 * @param screen_x_arg  Specifies <code>MouseEvent.screenX</code>. 
 * @param screen_y_arg  Specifies <code>MouseEvent.screenY</code>. 
 * @param client_x_arg  Specifies <code>MouseEvent.clientX</code>. 
 * @param client_y_arg  Specifies <code>MouseEvent.clientY</code>. 
 * @param ctrl_key_arg  Specifies <code>MouseEvent.ctrlKey</code>. 
 * @param alt_key_arg  Specifies <code>MouseEvent.altKey</code>. 
 * @param shift_key_arg  Specifies <code>MouseEvent.shiftKey</code>. 
 * @param meta_key_arg  Specifies <code>MouseEvent.metaKey</code>. 
 * @param button_arg  Specifies <code>MouseEvent.button</code>. 
 * @param related_target_arg Pointer to the object of
 *    Specifies 
 *   <code>MouseEvent.relatedTarget</code>. This value may be 
 *   <code>NULL</code>.   
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_mouseevent_init_mouse_event_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_mouseevent_init_mouse_event_start(javacall_handle handle,
                                               javacall_int32 invocation_id,
                                               void **context,
                                               javacall_const_utf16_string type_arg,
                                               javacall_bool can_bubble_arg,
                                               javacall_bool cancelable_arg,
                                               javacall_handle view_arg,
                                               javacall_int32 detail_arg,
                                               javacall_int32 screen_x_arg,
                                               javacall_int32 screen_y_arg,
                                               javacall_int32 client_x_arg,
                                               javacall_int32 client_y_arg,
                                               javacall_bool ctrl_key_arg,
                                               javacall_bool alt_key_arg,
                                               javacall_bool shift_key_arg,
                                               javacall_bool meta_key_arg,
                                               javacall_int16 button_arg,
                                               javacall_handle related_target_arg);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR  The <code>initMouseEvent</code> method is used to initialize the value 
 * of a <code>MouseEvent</code> object and has the same behavior as 
 * <code>UIEvent.initUIEvent()</code>. 
 * 
 * @param context The context saved during asynchronous operation.
 *   <code>MouseEvent.relatedTarget</code>. This value may be 
 *   <code>NULL</code>.   
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_mouseevent_init_mouse_event_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_mouseevent_init_mouse_event_finish(void *context);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR  The <code>initMouseEventNS</code> method is used to initialize the 
 * value of a <code>MouseEvent</code> object and has the same behavior 
 * as <code>UIEvent.initUIEventNS()</code>. 
 * 
 * @param handle Pointer to the object representing this mouseevent.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param namespace_uri  Refer to the <code>UIEvent.initUIEventNS()</code> 
 *   method for a description of this parameter. 
 * @param type_arg  Refer to the <code>UIEvent.initUIEventNS()</code> 
 *   method for a description of this parameter. 
 * @param can_bubble_arg  Refer to the <code>UIEvent.initUIEventNS()</code> 
 *   method for a description of this parameter. 
 * @param cancelable_arg  Refer to the <code>UIEvent.initUIEventNS()</code>
 *   method for a description of this parameter. 
 * @param view_arg Pointer to the object of
 *    Refer to the <code>UIEvent.initUIEventNS()</code> 
 *   method for a description of this parameter. 
 * @param detail_arg  Refer to the <code>UIEvent.initUIEventNS()</code> 
 *   method for a description of this parameter. 
 * @param screen_x_arg  Refer to the 
 *   <code>MouseEvent.initMouseEvent()</code> method for a description 
 *   of this parameter. 
 * @param screen_y_arg  Refer to the 
 *   <code>MouseEvent.initMouseEvent()</code> method for a description 
 *   of this parameter. 
 * @param client_x_arg  Refer to the 
 *   <code>MouseEvent.initMouseEvent()</code> method for a description 
 *   of this parameter. 
 * @param client_y_arg  Refer to the 
 *   <code>MouseEvent.initMouseEvent()</code> method for a description 
 *   of this parameter. 
 * @param button_arg  Refer to the <code>MouseEvent.initMouseEvent()</code>
 *   method for a description of this parameter. 
 * @param related_target_arg Pointer to the object of
 *    Refer to the 
 *   <code>MouseEvent.initMouseEvent()</code> method for a description 
 *   of this parameter. 
 * @param ctrl_key_arg  Specifies <code>MouseEvent.ctrlKey</code>. 
 * @param alt_key_arg  Specifies <code>MouseEvent.altKey</code>. 
 * @param shift_key_arg  Specifies <code>MouseEvent.shiftKey</code>. 
 * @param meta_key_arg  Specifies <code>MouseEvent.metaKey</code>. 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_mouseevent_init_mouse_event_ns_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_mouseevent_init_mouse_event_ns_start(javacall_handle handle,
                                                  javacall_int32 invocation_id,
                                                  void **context,
                                                  javacall_const_utf16_string namespace_uri,
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
                                                  javacall_bool meta_key_arg);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR  The <code>initMouseEventNS</code> method is used to initialize the 
 * value of a <code>MouseEvent</code> object and has the same behavior 
 * as <code>UIEvent.initUIEventNS()</code>. 
 * 
 * @param context The context saved during asynchronous operation.
 * @param ctrl_key_arg  Specifies <code>MouseEvent.ctrlKey</code>. 
 * @param alt_key_arg  Specifies <code>MouseEvent.altKey</code>. 
 * @param shift_key_arg  Specifies <code>MouseEvent.shiftKey</code>. 
 * @param meta_key_arg  Specifies <code>MouseEvent.metaKey</code>. 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_mouseevent_init_mouse_event_ns_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_mouseevent_init_mouse_event_ns_finish(void *context);

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
javacall_dom_mouseevent_clear_references(javacall_handle handle, javacall_int32 count);


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ifndef __JAVACALL_DOM_MOUSEEVENT_H_ */
