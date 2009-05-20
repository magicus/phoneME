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

#ifndef __JAVACALL_DOM_HTMLFORMELEMENT_H_
#define __JAVACALL_DOM_HTMLFORMELEMENT_H_

/**
 * @file javacall_dom_htmlformelement.h
 * @ingroup JSR290DOM
 * @brief Javacall DOM interfaces for HTMLFormElement
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
 * OR submits the form. It performs the same action as a submit button.
 * 
 * @param handle Pointer to the object representing this htmlformelement.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_htmlformelement_submit_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_htmlformelement_submit_start(javacall_handle handle,
                                          javacall_int32 invocation_id,
                                          void **context);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR submits the form. It performs the same action as a submit button.
 * 
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_htmlformelement_submit_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_htmlformelement_submit_finish(void *context);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR sets restores a form element's default values. It performs the same action 
 * as a reset button.
 * 
 * @param handle Pointer to the object representing this htmlformelement.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_htmlformelement_reset_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_htmlformelement_reset_start(javacall_handle handle,
                                         javacall_int32 invocation_id,
                                         void **context);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR sets restores a form element's default values. It performs the same action 
 * as a reset button.
 * 
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_htmlformelement_reset_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_htmlformelement_reset_finish(void *context);

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
javacall_dom_htmlformelement_clear_references(javacall_handle handle, javacall_int32 count);


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ifndef __JAVACALL_DOM_HTMLFORMELEMENT_H_ */
