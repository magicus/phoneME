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

#ifndef __JAVACALL_DOM_HTMLINPUTELEMENT_H_
#define __JAVACALL_DOM_HTMLINPUTELEMENT_H_

/**
 * @file javacall_dom_htmlinputelement.h
 * @ingroup JSR290DOM
 * @brief Javacall DOM interfaces for HTMLInputElement
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
 * OR select the contents of the text area. For <code>INPUT</code> elements 
 * whose <code>type</code> attribute has one of the following values: 
 * "text", "file", or "password".
 *
 * <p>Please note that this method does nothing in case the type has none
 * of the above listed values.</p>
 * 
 * @param handle Pointer to the object representing this htmlinputelement.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_htmlinputelement_select_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_htmlinputelement_select_start(javacall_handle handle,
                                           javacall_int32 invocation_id,
                                           void **context);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR select the contents of the text area. For <code>INPUT</code> elements 
 * whose <code>type</code> attribute has one of the following values: 
 * "text", "file", or "password".
 *
 * <p>Please note that this method does nothing in case the type has none
 * of the above listed values.</p>
 * 
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_htmlinputelement_select_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_htmlinputelement_select_finish(void *context);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns when the <code>type</code> attribute of the element has the value 
 * "radio" or "checkbox", this represents the current state of the form 
 * control, in an interactive user agent. Changes to this attribute 
 * change the state of the form control, but do not change the value of 
 * the HTML checked attribute of the INPUT element. 
 *
 * <p>During the handling 
 * of a click event on an input element with a type attribute that has 
 * the value "radio" or "checkbox", some implementations may change the 
 * value of this property before the event is being dispatched in the 
 * document. If the default action of the event is canceled, the value 
 * of the property may be changed back to its original value. This means 
 * that the value of this property during the handling of click events 
 * is implementation dependent.</p>
 *
 * <p>In any cases, the value returned is <code>true</code> 
 * if explicitly set.</p>
 *
 * 
 * @param handle Pointer to the object representing this htmlinputelement.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param ret_value Whether the form control is checked or not.
 *
 * @see #setChecked
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_htmlinputelement_get_checked_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_htmlinputelement_get_checked_start(javacall_handle handle,
                                                javacall_int32 invocation_id,
                                                void **context,
                                                /* OUT */ javacall_bool* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns when the <code>type</code> attribute of the element has the value 
 * "radio" or "checkbox", this represents the current state of the form 
 * control, in an interactive user agent. Changes to this attribute 
 * change the state of the form control, but do not change the value of 
 * the HTML checked attribute of the INPUT element. 
 *
 * <p>During the handling 
 * of a click event on an input element with a type attribute that has 
 * the value "radio" or "checkbox", some implementations may change the 
 * value of this property before the event is being dispatched in the 
 * document. If the default action of the event is canceled, the value 
 * of the property may be changed back to its original value. This means 
 * that the value of this property during the handling of click events 
 * is implementation dependent.</p>
 *
 * <p>In any cases, the value returned is <code>true</code> 
 * if explicitly set.</p>
 *
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value Whether the form control is checked or not.
 *
 * @see #setChecked
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_htmlinputelement_get_checked_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_htmlinputelement_get_checked_finish(void *context,
                                                 /* OUT */ javacall_bool* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR sets when the <code>type</code> attribute of the element has the value 
 * "radio" or "checkbox", this represents the current state of the form 
 * control, in an interactive user agent. Changes to this attribute 
 * change the state of the form control, but do not change the value of 
 * the HTML checked attribute of the INPUT element. 
 *
 * <p>During the handling 
 * of a click event on an input element with a type attribute that has 
 * the value "radio" or "checkbox", some implementations may change the 
 * value of this property before the event is being dispatched in the 
 * document. If the default action of the event is canceled, the value 
 * of the property may be changed back to its original value. This means 
 * that the value of this property during the handling of click events 
 * is implementation dependent.</p>
 *
 * 
 * @param handle Pointer to the object representing this htmlinputelement.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param checked The new state of the form control.
 *
 * @see #getChecked
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_htmlinputelement_set_checked_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_htmlinputelement_set_checked_start(javacall_handle handle,
                                                javacall_int32 invocation_id,
                                                void **context,
                                                javacall_bool checked);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR sets when the <code>type</code> attribute of the element has the value 
 * "radio" or "checkbox", this represents the current state of the form 
 * control, in an interactive user agent. Changes to this attribute 
 * change the state of the form control, but do not change the value of 
 * the HTML checked attribute of the INPUT element. 
 *
 * <p>During the handling 
 * of a click event on an input element with a type attribute that has 
 * the value "radio" or "checkbox", some implementations may change the 
 * value of this property before the event is being dispatched in the 
 * document. If the default action of the event is canceled, the value 
 * of the property may be changed back to its original value. This means 
 * that the value of this property during the handling of click events 
 * is implementation dependent.</p>
 *
 * 
 * @param context The context saved during asynchronous operation.
 *
 * @see #getChecked
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_htmlinputelement_set_checked_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_htmlinputelement_set_checked_finish(void *context);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns when the <code>type</code> attribute of the element has the value 
 * "text", "file" or "password", this represents the current contents of 
 * the corresponding form control, in an interactive user agent. 
 * Changing this attribute changes the contents of the form control, but 
 * does not change the value of the HTML value attribute of the element. 
 * When the <code>type</code> attribute of the element has the value 
 * "button", "hidden", "submit", "reset", "image", "checkbox" or 
 * "radio", this represents the HTML value attribute of the element. See 
 * the value attribute definition in HTML 4.01.
 *
 * 
 * Note: If ret_value_len is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill ret_value_len 
 *       with actual length of the returned string.
 *
 * @param handle Pointer to the object representing this htmlinputelement.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param ret_value The content of the value attribute of that form control.
 *
 * @see #setValue
 * @param ret_value_len Number of code_units of the returned string
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context or if length of the returend string is more then 
 *             specified in ret_value_len,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_htmlinputelement_get_value_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_htmlinputelement_get_value_start(javacall_handle handle,
                                              javacall_int32 invocation_id,
                                              void **context,
                                              /* OUT */ javacall_utf16** ret_value,
                                              /* INOUT */ javacall_uint32* ret_value_len);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns when the <code>type</code> attribute of the element has the value 
 * "text", "file" or "password", this represents the current contents of 
 * the corresponding form control, in an interactive user agent. 
 * Changing this attribute changes the contents of the form control, but 
 * does not change the value of the HTML value attribute of the element. 
 * When the <code>type</code> attribute of the element has the value 
 * "button", "hidden", "submit", "reset", "image", "checkbox" or 
 * "radio", this represents the HTML value attribute of the element. See 
 * the value attribute definition in HTML 4.01.
 *
 * 
 * Note: If ret_value_len is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill ret_value_len 
 *       with actual length of the returned string.
 *
 * @param context The context saved during asynchronous operation.
 * @param ret_value The content of the value attribute of that form control.
 *
 * @see #setValue
 * @param ret_value_len Number of code_units of the returned string
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if length of the returend string is more then 
 *             specified in ret_value_len,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_htmlinputelement_get_value_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_htmlinputelement_get_value_finish(void *context,
                                               /* OUT */ javacall_utf16** ret_value,
                                               /* INOUT */ javacall_uint32* ret_value_len);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR sets when the <code>type</code> attribute of the element has the value 
 * "text", "file" or "password", this represents the current contents of 
 * the corresponding form control, in an interactive user agent. 
 * Changing this attribute changes the contents of the form control, but 
 * does not change the value of the HTML value attribute of the element. 
 * When the <code>type</code> attribute of the element has the value 
 * "button", "hidden", "submit", "reset", "image", "checkbox" or 
 * "radio", this represents the HTML value attribute of the element. See 
 * the value attribute definition in HTML 4.01.
 *
 * 
 * @param handle Pointer to the object representing this htmlinputelement.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param value The new content of that form control.
 *
 * @see #getValue
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_htmlinputelement_set_value_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_htmlinputelement_set_value_start(javacall_handle handle,
                                              javacall_int32 invocation_id,
                                              void **context,
                                              javacall_const_utf16_string value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR sets when the <code>type</code> attribute of the element has the value 
 * "text", "file" or "password", this represents the current contents of 
 * the corresponding form control, in an interactive user agent. 
 * Changing this attribute changes the contents of the form control, but 
 * does not change the value of the HTML value attribute of the element. 
 * When the <code>type</code> attribute of the element has the value 
 * "button", "hidden", "submit", "reset", "image", "checkbox" or 
 * "radio", this represents the HTML value attribute of the element. See 
 * the value attribute definition in HTML 4.01.
 *
 * 
 * @param context The context saved during asynchronous operation.
 *
 * @see #getValue
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_htmlinputelement_set_value_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_htmlinputelement_set_value_finish(void *context);

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
javacall_dom_htmlinputelement_clear_references(javacall_handle handle, javacall_int32 count);


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ifndef __JAVACALL_DOM_HTMLINPUTELEMENT_H_ */
