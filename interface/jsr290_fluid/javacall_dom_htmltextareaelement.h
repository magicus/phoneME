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

#ifndef __JAVACALL_DOM_HTMLTEXTAREAELEMENT_H_
#define __JAVACALL_DOM_HTMLTEXTAREAELEMENT_H_

/**
 * @file javacall_dom_htmltextareaelement.h
 * @ingroup JSR290DOM
 * @brief Javacall DOM interfaces for HTMLTextAreaElement
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
 * OR select the contents of the <code>TEXTAREA</code>.
 * 
 * @param handle Pointer to the object representing this htmltextareaelement.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_htmltextareaelement_select_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_htmltextareaelement_select_start(javacall_handle handle,
                                              javacall_int32 invocation_id,
                                              void **context);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR select the contents of the <code>TEXTAREA</code>.
 * 
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_htmltextareaelement_select_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_htmltextareaelement_select_finish(void *context);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns represents the current contents of the corresponding form control, in 
 * an interactive user agent. Changing this attribute changes the 
 * contents of the form control, but does not change the contents of the 
 * element. The implementation <span class="rfc2119">may</span> truncate
 * the content if the entirety of the data does not fit in the DOM
 * implementation data type (e.g. <code>DOMString</code>).
 *
 * 
 * Note: If ret_value_len is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill ret_value_len 
 *       with actual length of the returned string.
 *
 * @param handle Pointer to the object representing this htmltextareaelement.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param ret_value The current content of that text area.
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
 *             javacall_dom_htmltextareaelement_get_value_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_htmltextareaelement_get_value_start(javacall_handle handle,
                                                 javacall_int32 invocation_id,
                                                 void **context,
                                                 /* OUT */ javacall_utf16** ret_value,
                                                 /* INOUT */ javacall_uint32* ret_value_len);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns represents the current contents of the corresponding form control, in 
 * an interactive user agent. Changing this attribute changes the 
 * contents of the form control, but does not change the contents of the 
 * element. The implementation <span class="rfc2119">may</span> truncate
 * the content if the entirety of the data does not fit in the DOM
 * implementation data type (e.g. <code>DOMString</code>).
 *
 * 
 * Note: If ret_value_len is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill ret_value_len 
 *       with actual length of the returned string.
 *
 * @param context The context saved during asynchronous operation.
 * @param ret_value The current content of that text area.
 *
 * @see #setValue
 * @param ret_value_len Number of code_units of the returned string
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if length of the returend string is more then 
 *             specified in ret_value_len,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_htmltextareaelement_get_value_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_htmltextareaelement_get_value_finish(void *context,
                                                  /* OUT */ javacall_utf16** ret_value,
                                                  /* INOUT */ javacall_uint32* ret_value_len);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR sets represents the current contents of the corresponding form control, in 
 * an interactive user agent. Changing this attribute changes the 
 * contents of the form control, but does not change the contents of the 
 * element. The implementation <span class="rfc2119">may</span> truncate
 * the content if the entirety of the data does not fit in the DOM
 * implementation data type (e.g. <code>DOMString</code>).
 *
 * 
 * @param handle Pointer to the object representing this htmltextareaelement.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param value The new content of that text area. 
 *
 * @see #getValue
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_htmltextareaelement_set_value_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_htmltextareaelement_set_value_start(javacall_handle handle,
                                                 javacall_int32 invocation_id,
                                                 void **context,
                                                 javacall_const_utf16_string value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR sets represents the current contents of the corresponding form control, in 
 * an interactive user agent. Changing this attribute changes the 
 * contents of the form control, but does not change the contents of the 
 * element. The implementation <span class="rfc2119">may</span> truncate
 * the content if the entirety of the data does not fit in the DOM
 * implementation data type (e.g. <code>DOMString</code>).
 *
 * 
 * @param context The context saved during asynchronous operation.
 *
 * @see #getValue
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_htmltextareaelement_set_value_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_htmltextareaelement_set_value_finish(void *context);

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
javacall_dom_htmltextareaelement_clear_references(javacall_handle handle, javacall_int32 count);


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ifndef __JAVACALL_DOM_HTMLTEXTAREAELEMENT_H_ */
