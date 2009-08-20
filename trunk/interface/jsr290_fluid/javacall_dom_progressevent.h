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

#ifndef __JAVACALL_DOM_PROGRESSEVENT_H_
#define __JAVACALL_DOM_PROGRESSEVENT_H_

/**
 * @file javacall_dom_progressevent.h
 * @ingroup JSR290DOM
 * @brief Javacall DOM interfaces for ProgressEvent
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
 * OR returns specifies whether the total size of the transfer is known.
 * 
 * @param handle Pointer to the object representing this progressevent.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_progressevent_get_length_computable_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_progressevent_get_length_computable_start(javacall_handle handle,
                                                       javacall_int32 invocation_id,
                                                       void **context,
                                                       /* OUT */ javacall_bool* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns specifies whether the total size of the transfer is known.
 * 
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_progressevent_get_length_computable_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_progressevent_get_length_computable_finish(void *context,
                                                        /* OUT */ javacall_bool* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns specifies the number of bytes downloaded since the beginning of the 
 * download. This refers to the content, excluding headers and overhead 
 * from the transaction,
 * and where there is a content-encoding or transfer-encoding refers
 * to the number of bytes to be transferred, i.e. with the relevant
 * encodings applied. For more details on HTTP see [RFC2616].
 * 
 * @param handle Pointer to the object representing this progressevent.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_progressevent_get_loaded_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_progressevent_get_loaded_start(javacall_handle handle,
                                            javacall_int32 invocation_id,
                                            void **context,
                                            /* OUT */ javacall_int32* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns specifies the number of bytes downloaded since the beginning of the 
 * download. This refers to the content, excluding headers and overhead 
 * from the transaction,
 * and where there is a content-encoding or transfer-encoding refers
 * to the number of bytes to be transferred, i.e. with the relevant
 * encodings applied. For more details on HTTP see [RFC2616].
 * 
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_progressevent_get_loaded_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_progressevent_get_loaded_finish(void *context,
                                             /* OUT */ javacall_int32* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns specifies the expected total number of bytes of the content 
 * transferred in the operation. Where the size of the transfer is for
 * some reason unknown, the value of this attribute <em>must</em> be zero.
 * 
 * @param handle Pointer to the object representing this progressevent.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_progressevent_get_total_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_progressevent_get_total_start(javacall_handle handle,
                                           javacall_int32 invocation_id,
                                           void **context,
                                           /* OUT */ javacall_int32* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns specifies the expected total number of bytes of the content 
 * transferred in the operation. Where the size of the transfer is for
 * some reason unknown, the value of this attribute <em>must</em> be zero.
 * 
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_progressevent_get_total_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_progressevent_get_total_finish(void *context,
                                            /* OUT */ javacall_int32* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR the initProgressEvent method is used to initialize the value of a 
 * progress event created through the DocumentEvent interface. 
 * If this method is called multiple times, the final invocation takes 
 * precedence .
 *
 * 
 * @param handle Pointer to the object representing this progressevent.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param type_arg  
 *   This <em>must</em> be one of <code>loadstart</code>, 
 *   <code>progress</code>, <code>error</code>, <code>abort</code>,
 *   <code>load</code>. If it is not one of those values then this
 *   specification does not define the resulting event.
 * @param can_bubble_arg
 *   This <em>must</em> be <var>false</var>. Where a
 *   value of <var>true</var> is passed, implementations
 *   <em>must</em> override that and change the value to
 *   <var>false</var>.
 * @param cancelable_arg
 *   This <em>must</em> be <var>false</var>. Where a
 *   value of <var>true</var> is passed, implementations 
 *   <em>must</em> override that and change the value to
 *   <var>false</var>.
 * @param length_computable_arg
 *   If the implementation has reliable information about
 *   the value of <code>total</code>, then this should be <var>true</var>. 
 *   If the implementation does not have reliable information about
 *   the value of <code>total</code>, this should be <var>false</var>.
 * @param loaded_arg
 *   Specifies the total number of bytes already loaded. If this value 
 *   is not a non-negative number, 
 *   the implementation <em>must</em> change it to zero.
 * @param total_arg
 *   Specifies the total number of bytes to be
 *   loaded. If <code>lengthComputable</code> is <var>false</var>, 
 *   this <em>must</em> be zero. If any other parameter is passed, and
 *   <code>lengthComputable</code> is <var>false</var>, the implementation 
 *   <em>must</em> override this and set the value to zero. If
 *   <code>lengthComputable</code> is <var>true</var>, and the value 
 *   of this parameter is not a non-negative number, the implementation 
 *   <em>must</em> set <code>lengthComputable</code> to <var>false</var>
 *   and the value of <code>total</code> to zero. 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_progressevent_init_progress_event_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_progressevent_init_progress_event_start(javacall_handle handle,
                                                     javacall_int32 invocation_id,
                                                     void **context,
                                                     javacall_const_utf16_string type_arg,
                                                     javacall_bool can_bubble_arg,
                                                     javacall_bool cancelable_arg,
                                                     javacall_bool length_computable_arg,
                                                     javacall_int32 loaded_arg,
                                                     javacall_int32 total_arg);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR the initProgressEvent method is used to initialize the value of a 
 * progress event created through the DocumentEvent interface. 
 * If this method is called multiple times, the final invocation takes 
 * precedence .
 *
 * 
 * @param context The context saved during asynchronous operation.
 *   Specifies the total number of bytes to be
 *   loaded. If <code>lengthComputable</code> is <var>false</var>, 
 *   this <em>must</em> be zero. If any other parameter is passed, and
 *   <code>lengthComputable</code> is <var>false</var>, the implementation 
 *   <em>must</em> override this and set the value to zero. If
 *   <code>lengthComputable</code> is <var>true</var>, and the value 
 *   of this parameter is not a non-negative number, the implementation 
 *   <em>must</em> set <code>lengthComputable</code> to <var>false</var>
 *   and the value of <code>total</code> to zero. 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_progressevent_init_progress_event_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_progressevent_init_progress_event_finish(void *context);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR the initProgressEventNS method is used to initialize the value of a 
 * namespaced progress event created through the DocumentEvent interface.
 * This method may only be called before the progress event has been 
 * dispatched via the dispatchEvent method, though it may be called 
 * multiple times during that phase if necessary. If called multiple 
 * times, the final invocation takes precedence.
 *
 * 
 * @param handle Pointer to the object representing this progressevent.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param namespace_uri_arg 
 *   Specifies the URI for the namespace of the event.
 *   For all events defined in this specification, the
 *   value of this parameter is <code>NULL</code>.
 * @param type_arg
 *   This must be one of <code>loadstart</code>, 
 *   <code>progress</code>, <code>error</code>,
 *   <code>abort</code>, <code>load</code>. If it is not one
 *   of those values then this specification does not define 
 *   the resulting event.
 * @param can_bubble_arg
 *   This <em>must</em> be <var>false</var>. Where a
 *   value of <var>true</var> is passed, implementations
 *   <em>must</em> override that and change the value to
 *   <var>false</var>.
 * @param cancelable_arg
 *   This <em>must</em> be <var>false</var>. Where a
 *   value of <var>true</var> is passed, implementations
 *   <em>must</em> override that and change the value to
 *   <var>false</var>.
 * @param length_computable_arg
 *   If the implementation has reliable information about
 *   the value of total, then this should be <var>true</var>. If the
 *   implementation does not have reliable information about
 *   the value of total, this should be <var>false</var>.
 * @param loaded_arg
 *   This parameter specifies the total number of bytes
 *   already loaded.
 *   If this value is not a non-negative number, the implementation 
 *   <em>must</em> change it to zero.
 * @param total_arg
 *   This specifies the total number of bytes to be
 *   loaded. If <code>lengthComputable</code> is <var>false</var>, 
 *   this <em>must</em> be zero. If any other parameter is passed,
 *   and <code>lengthComputable</code> is <var>false</var>, the
 *   implementation <em>must</em> override this and set the value to
 *   zero. If <code>lengthComputable</code> is <var>true</var>, and 
 *   the value of this parameter is not a non-negative number, the 
 *   implementation <em>must</em> set <code>lengthComputable</code> 
 *   to <var>false</var> and the value of <code>total</code> to zero.
 *
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_progressevent_init_progress_event_ns_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_progressevent_init_progress_event_ns_start(javacall_handle handle,
                                                        javacall_int32 invocation_id,
                                                        void **context,
                                                        javacall_const_utf16_string namespace_uri_arg,
                                                        javacall_const_utf16_string type_arg,
                                                        javacall_bool can_bubble_arg,
                                                        javacall_bool cancelable_arg,
                                                        javacall_bool length_computable_arg,
                                                        javacall_int32 loaded_arg,
                                                        javacall_int32 total_arg);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR the initProgressEventNS method is used to initialize the value of a 
 * namespaced progress event created through the DocumentEvent interface.
 * This method may only be called before the progress event has been 
 * dispatched via the dispatchEvent method, though it may be called 
 * multiple times during that phase if necessary. If called multiple 
 * times, the final invocation takes precedence.
 *
 * 
 * @param context The context saved during asynchronous operation.
 *   This specifies the total number of bytes to be
 *   loaded. If <code>lengthComputable</code> is <var>false</var>, 
 *   this <em>must</em> be zero. If any other parameter is passed,
 *   and <code>lengthComputable</code> is <var>false</var>, the
 *   implementation <em>must</em> override this and set the value to
 *   zero. If <code>lengthComputable</code> is <var>true</var>, and 
 *   the value of this parameter is not a non-negative number, the 
 *   implementation <em>must</em> set <code>lengthComputable</code> 
 *   to <var>false</var> and the value of <code>total</code> to zero.
 *
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_progressevent_init_progress_event_ns_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_progressevent_init_progress_event_ns_finish(void *context);

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
javacall_dom_progressevent_clear_references(javacall_handle handle, javacall_int32 count);


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ifndef __JAVACALL_DOM_PROGRESSEVENT_H_ */
