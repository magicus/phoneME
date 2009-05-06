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

#ifndef __JAVACALL_DOM_WINDOW_H_
#define __JAVACALL_DOM_WINDOW_H_

/**
 * @file javacall_dom_window.h
 * @ingroup JSR290DOM
 * @brief Javacall DOM interfaces for Window
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
 * OR returns the value of the <code>window</code> attribute.
 *
 * <p>The value <span class="rfc2119">must</span> be the same 
 * <code>Window</code> object that has this attribute: the attribute is a
 * self-reference.</p>
 *
 * 
 * @param handle Pointer to the object representing this window.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   the value of the <code>window</code> attribute.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_window_get_window_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_window_get_window_start(javacall_handle handle,
                                     javacall_int32 invocation_id,
                                     void **context,
                                     /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the value of the <code>window</code> attribute.
 *
 * <p>The value <span class="rfc2119">must</span> be the same 
 * <code>Window</code> object that has this attribute: the attribute is a
 * self-reference.</p>
 *
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   the value of the <code>window</code> attribute.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_window_get_window_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_window_get_window_finish(void *context,
                                      /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the value of the <code>self</code> attribute.
 *
 * <p>The value <span class="rfc2119">must</span> be the same 
 * <code>Window</code> object that has this attribute: the attribute is a
 * self-reference. Consequently, the value of the self attribute
 * <span class="rfc2119">must</span> be the same object as the window
 * attribute.</p>
 *
 * 
 * @param handle Pointer to the object representing this window.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   the value of the <code>self</code> attribute.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_window_get_self_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_window_get_self_start(javacall_handle handle,
                                   javacall_int32 invocation_id,
                                   void **context,
                                   /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the value of the <code>self</code> attribute.
 *
 * <p>The value <span class="rfc2119">must</span> be the same 
 * <code>Window</code> object that has this attribute: the attribute is a
 * self-reference. Consequently, the value of the self attribute
 * <span class="rfc2119">must</span> be the same object as the window
 * attribute.</p>
 *
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   the value of the <code>self</code> attribute.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_window_get_self_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_window_get_self_finish(void *context,
                                    /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the value of the <code>location</code> attribute.
 *
 * <p>The value <span class="rfc2119">must</span> be the 
 * <code>Location</code> object that represents the window's current
 * location.</p>
 *
 * 
 * @param handle Pointer to the object representing this window.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   the value of the <code>location</code> attribute.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_window_get_location_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_window_get_location_start(javacall_handle handle,
                                       javacall_int32 invocation_id,
                                       void **context,
                                       /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the value of the <code>location</code> attribute.
 *
 * <p>The value <span class="rfc2119">must</span> be the 
 * <code>Location</code> object that represents the window's current
 * location.</p>
 *
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   the value of the <code>location</code> attribute.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_window_get_location_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_window_get_location_finish(void *context,
                                        /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the value of the <code>name</code> attribute.
 *
 * <p>An attribute containing a unique name used to refer to this Window
 * object. It corresponds to the name of the <code>&lt;object&gt;</code>, 
 * <code>&lt;frame&gt;</code>, <code>&lt;iframe&gt;</code> (or similar) or
 * to the name passed at creation (i.e. in case of
 * <code>window.open()</code>).</p>
 *
 * 
 * Note: If ret_value_len is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill ret_value_len 
 *       with actual length of the returned string.
 *
 * @param handle Pointer to the object representing this window.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param ret_value The value of the <code>name</code> attribute.
 * @param ret_value_len Number of code_units of the returned string
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context or if length of the returend string is more then 
 *             specified in ret_value_len,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_window_get_name_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_window_get_name_start(javacall_handle handle,
                                   javacall_int32 invocation_id,
                                   void **context,
                                   /* OUT */ javacall_utf16** ret_value,
                                   /* INOUT */ javacall_uint32* ret_value_len);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the value of the <code>name</code> attribute.
 *
 * <p>An attribute containing a unique name used to refer to this Window
 * object. It corresponds to the name of the <code>&lt;object&gt;</code>, 
 * <code>&lt;frame&gt;</code>, <code>&lt;iframe&gt;</code> (or similar) or
 * to the name passed at creation (i.e. in case of
 * <code>window.open()</code>).</p>
 *
 * 
 * Note: If ret_value_len is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill ret_value_len 
 *       with actual length of the returned string.
 *
 * @param context The context saved during asynchronous operation.
 * @param ret_value The value of the <code>name</code> attribute.
 * @param ret_value_len Number of code_units of the returned string
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if length of the returend string is more then 
 *             specified in ret_value_len,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_window_get_name_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_window_get_name_finish(void *context,
                                    /* OUT */ javacall_utf16** ret_value,
                                    /* INOUT */ javacall_uint32* ret_value_len);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the value of the <code>parent</code> attribute.
 *
 * <p>An attribute containing a reference to Window object that contains
 * this object.</p>
 *
 * 
 * @param handle Pointer to the object representing this window.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   the value of the <code>parent</code> attribute.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_window_get_parent_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_window_get_parent_start(javacall_handle handle,
                                     javacall_int32 invocation_id,
                                     void **context,
                                     /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the value of the <code>parent</code> attribute.
 *
 * <p>An attribute containing a reference to Window object that contains
 * this object.</p>
 *
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   the value of the <code>parent</code> attribute.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_window_get_parent_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_window_get_parent_finish(void *context,
                                      /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the value of the <code>top</code> attribute.
 *
 * <p>An attribute containing a reference to the topmost Window object 
 * in the hierarchy that contains this object.</p>
 *
 * 
 * @param handle Pointer to the object representing this window.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   the value of the <code>top</code> attribute.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_window_get_top_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_window_get_top_start(javacall_handle handle,
                                  javacall_int32 invocation_id,
                                  void **context,
                                  /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the value of the <code>top</code> attribute.
 *
 * <p>An attribute containing a reference to the topmost Window object 
 * in the hierarchy that contains this object.</p>
 *
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   the value of the <code>top</code> attribute.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_window_get_top_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_window_get_top_finish(void *context,
                                   /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the value of the <code>frameElement</code> attribute.
 *
 * <p>The <code>Element</code> returned is the parent element in which
 * this child is being embedded. It corresponds e.g. to one of 
 * <code>&lt;html:object&gt;</code>, <code>&lt;html:frame&gt;</code>, 
 * <code>&lt;html:iframe&gt;</code>, 
 * <code>&lt;svg:foreignObject&gt;</code>, 
 * <code>&lt;svg:animation&gt;</code> or other embedding
 * point, or <code>NULL</code> if none.</p>
 *
 * 
 * @param handle Pointer to the object representing this window.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   the value of the <code>frameElement</code> attribute, or 
 * <code>NULL</code> if none.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_window_get_frame_element_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_window_get_frame_element_start(javacall_handle handle,
                                            javacall_int32 invocation_id,
                                            void **context,
                                            /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the value of the <code>frameElement</code> attribute.
 *
 * <p>The <code>Element</code> returned is the parent element in which
 * this child is being embedded. It corresponds e.g. to one of 
 * <code>&lt;html:object&gt;</code>, <code>&lt;html:frame&gt;</code>, 
 * <code>&lt;html:iframe&gt;</code>, 
 * <code>&lt;svg:foreignObject&gt;</code>, 
 * <code>&lt;svg:animation&gt;</code> or other embedding
 * point, or <code>NULL</code> if none.</p>
 *
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   the value of the <code>frameElement</code> attribute, or 
 * <code>NULL</code> if none.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_window_get_frame_element_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_window_get_frame_element_finish(void *context,
                                             /* OUT */ javacall_handle* ret_value);

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
javacall_dom_window_clear_references(javacall_handle handle, javacall_int32 count);


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ifndef __JAVACALL_DOM_WINDOW_H_ */
