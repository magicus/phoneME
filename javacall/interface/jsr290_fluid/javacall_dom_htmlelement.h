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

#ifndef __JAVACALL_DOM_HTMLELEMENT_H_
#define __JAVACALL_DOM_HTMLELEMENT_H_

/**
 * @file javacall_dom_htmlelement.h
 * @ingroup JSR290DOM
 * @brief Javacall DOM interfaces for HTMLElement
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
 * OR returns the element's identifier. See the id attribute definition in HTML 4.01.
 *
 * 
 * Note: If ret_value_len is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill ret_value_len 
 *       with actual length of the returned string.
 *
 * @param handle Pointer to the object representing this htmlelement.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param ret_value The id of the element.
 *
 * @see #setId
 * @param ret_value_len Number of code_units of the returned string
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context or if length of the returend string is more then 
 *             specified in ret_value_len,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_htmlelement_get_id_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_htmlelement_get_id_start(javacall_handle handle,
                                      javacall_int32 invocation_id,
                                      void **context,
                                      /* OUT */ javacall_utf16** ret_value,
                                      /* INOUT */ javacall_uint32* ret_value_len);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the element's identifier. See the id attribute definition in HTML 4.01.
 *
 * 
 * Note: If ret_value_len is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill ret_value_len 
 *       with actual length of the returned string.
 *
 * @param context The context saved during asynchronous operation.
 * @param ret_value The id of the element.
 *
 * @see #setId
 * @param ret_value_len Number of code_units of the returned string
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if length of the returend string is more then 
 *             specified in ret_value_len,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_htmlelement_get_id_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_htmlelement_get_id_finish(void *context,
                                       /* OUT */ javacall_utf16** ret_value,
                                       /* INOUT */ javacall_uint32* ret_value_len);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR sets the element's identifier. See the id attribute definition in HTML 4.01.
 * This attribute assigns a name to an element. This name <span 
 * class="rfc2119">should</span> be unique in a document.
 *
 * 
 * @param handle Pointer to the object representing this htmlelement.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param id The new id of the element. This string <span 
 * class="rfc2119">may</span> be <code>NULL</code>.
 *
 * @see #getId
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_htmlelement_set_id_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_htmlelement_set_id_start(javacall_handle handle,
                                      javacall_int32 invocation_id,
                                      void **context,
                                      javacall_const_utf16_string id);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR sets the element's identifier. See the id attribute definition in HTML 4.01.
 * This attribute assigns a name to an element. This name <span 
 * class="rfc2119">should</span> be unique in a document.
 *
 * 
 * @param context The context saved during asynchronous operation.
 * class="rfc2119">may</span> be <code>NULL</code>.
 *
 * @see #getId
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_htmlelement_set_id_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_htmlelement_set_id_finish(void *context);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the class attribute of the element. This attribute has been renamed due 
 * to conflicts with the "class" keyword exposed by many languages. See 
 * the class attribute definition in HTML 4.01.
 *
 * 
 * Note: If ret_value_len is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill ret_value_len 
 *       with actual length of the returned string.
 *
 * @param handle Pointer to the object representing this htmlelement.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param ret_value The class attribute of the element.
 *
 * @see #setClassName
 * @param ret_value_len Number of code_units of the returned string
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context or if length of the returend string is more then 
 *             specified in ret_value_len,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_htmlelement_get_class_name_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_htmlelement_get_class_name_start(javacall_handle handle,
                                              javacall_int32 invocation_id,
                                              void **context,
                                              /* OUT */ javacall_utf16** ret_value,
                                              /* INOUT */ javacall_uint32* ret_value_len);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the class attribute of the element. This attribute has been renamed due 
 * to conflicts with the "class" keyword exposed by many languages. See 
 * the class attribute definition in HTML 4.01.
 *
 * 
 * Note: If ret_value_len is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill ret_value_len 
 *       with actual length of the returned string.
 *
 * @param context The context saved during asynchronous operation.
 * @param ret_value The class attribute of the element.
 *
 * @see #setClassName
 * @param ret_value_len Number of code_units of the returned string
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if length of the returend string is more then 
 *             specified in ret_value_len,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_htmlelement_get_class_name_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_htmlelement_get_class_name_finish(void *context,
                                               /* OUT */ javacall_utf16** ret_value,
                                               /* INOUT */ javacall_uint32* ret_value_len);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR sets the class attribute of the element. This attribute has been renamed due 
 * to conflicts with the "class" keyword exposed by many languages. See 
 * the class attribute definition in HTML 4.01.
 *
 * <p>This attribute assigns a class name or set of class names to an
 * element. Any number of elements may be assigned the same class name or
 * names. Multiple class names <span class="rfc2119">must</span> be
 * separated by white space characters.</p>
 *
 * 
 * @param handle Pointer to the object representing this htmlelement.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param class_name The value to be set in the class attribute of the
 * element. This string <span class="rfc2119">may</span> be 
 * <code>NULL</code>.
 *
 * @see #getClassName
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_htmlelement_set_class_name_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_htmlelement_set_class_name_start(javacall_handle handle,
                                              javacall_int32 invocation_id,
                                              void **context,
                                              javacall_const_utf16_string class_name);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR sets the class attribute of the element. This attribute has been renamed due 
 * to conflicts with the "class" keyword exposed by many languages. See 
 * the class attribute definition in HTML 4.01.
 *
 * <p>This attribute assigns a class name or set of class names to an
 * element. Any number of elements may be assigned the same class name or
 * names. Multiple class names <span class="rfc2119">must</span> be
 * separated by white space characters.</p>
 *
 * 
 * @param context The context saved during asynchronous operation.
 * element. This string <span class="rfc2119">may</span> be 
 * <code>NULL</code>.
 *
 * @see #getClassName
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_htmlelement_set_class_name_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_htmlelement_set_class_name_finish(void *context);

/*
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns a code representing the type of the underlying object as defined above
 * 
 * 
 * @param handle Pointer to the object representing this htmlelement.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param ret_value A code representing the type of the underlying object as defined above. 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_htmlelement_get_html_element_type_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_htmlelement_get_html_element_type_start(javacall_handle handle,
                                                     javacall_int32 invocation_id,
                                                     void **context,
                                                     /* OUT */ javacall_dom_html_element_types* ret_value);

/*
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns a code representing the type of the underlying object as defined above
 * 
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value A code representing the type of the underlying object as defined above. 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_htmlelement_get_html_element_type_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_htmlelement_get_html_element_type_finish(void *context,
                                                      /* OUT */ javacall_dom_html_element_types* ret_value);

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
javacall_dom_htmlelement_clear_references(javacall_handle handle, javacall_int32 count);


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ifndef __JAVACALL_DOM_HTMLELEMENT_H_ */
