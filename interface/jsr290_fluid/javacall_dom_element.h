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

#ifndef __JAVACALL_DOM_ELEMENT_H_
#define __JAVACALL_DOM_ELEMENT_H_

/**
 * @file javacall_dom_element.h
 * @ingroup JSR290DOM
 * @brief Javacall DOM interfaces for Element
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
 * OR the name of the element. If <code>Node.localName</code> is different
 * from <code>NULL</code>, this attribute is a qualified name.
 * For example, in: 
 * <pre> &lt;elementExample id="demo"&gt; ... 
 * &lt;/elementExample&gt; , </pre>
 *  <code>tagName</code> has the value
 * <code>"elementExample"</code>. Note that this is case-preserving in
 * XML, as are all of the operations of the DOM. The HTML DOM returns
 * the <code>tagName</code> of an HTML element in the canonical
 * uppercase form, regardless of the case in the source HTML document.
 * 
 * Note: If ret_value_len is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill ret_value_len 
 *       with actual length of the returned string.
 *
 * @param handle Pointer to the object representing this element.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param ret_value a String containing the name of the element
 * @param ret_value_len Number of code_units of the returned string
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context or if length of the returend string is more then 
 *             specified in ret_value_len,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_get_tag_name_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_get_tag_name_start(javacall_handle handle,
                                        javacall_int32 invocation_id,
                                        void **context,
                                        /* OUT */ javacall_utf16** ret_value,
                                        /* INOUT */ javacall_uint32* ret_value_len);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR the name of the element. If <code>Node.localName</code> is different
 * from <code>NULL</code>, this attribute is a qualified name.
 * For example, in: 
 * <pre> &lt;elementExample id="demo"&gt; ... 
 * &lt;/elementExample&gt; , </pre>
 *  <code>tagName</code> has the value
 * <code>"elementExample"</code>. Note that this is case-preserving in
 * XML, as are all of the operations of the DOM. The HTML DOM returns
 * the <code>tagName</code> of an HTML element in the canonical
 * uppercase form, regardless of the case in the source HTML document.
 * 
 * Note: If ret_value_len is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill ret_value_len 
 *       with actual length of the returned string.
 *
 * @param context The context saved during asynchronous operation.
 * @param ret_value a String containing the name of the element
 * @param ret_value_len Number of code_units of the returned string
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if length of the returend string is more then 
 *             specified in ret_value_len,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_get_tag_name_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_get_tag_name_finish(void *context,
                                         /* OUT */ javacall_utf16** ret_value,
                                         /* INOUT */ javacall_uint32* ret_value_len);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns retrieves an attribute value by name.
 * 
 * Note: If ret_value_len is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill ret_value_len 
 *       with actual length of the returned string.
 *
 * @param handle Pointer to the object representing this element.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param name The name of the attribute to retrieve.
 * @param ret_value The <code>Attr</code> value as a string, or the empty string 
 *   if that attribute does not have a specified or default value.
 * @param ret_value_len Number of code_units of the returned string
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context or if length of the returend string is more then 
 *             specified in ret_value_len,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_get_attribute_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_get_attribute_start(javacall_handle handle,
                                         javacall_int32 invocation_id,
                                         void **context,
                                         javacall_const_utf16_string name,
                                         /* OUT */ javacall_utf16** ret_value,
                                         /* INOUT */ javacall_uint32* ret_value_len);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns retrieves an attribute value by name.
 * 
 * Note: If ret_value_len is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill ret_value_len 
 *       with actual length of the returned string.
 *
 * @param context The context saved during asynchronous operation.
 * @param ret_value The <code>Attr</code> value as a string, or the empty string 
 *   if that attribute does not have a specified or default value.
 * @param ret_value_len Number of code_units of the returned string
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if length of the returend string is more then 
 *             specified in ret_value_len,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_get_attribute_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_get_attribute_finish(void *context,
                                          /* OUT */ javacall_utf16** ret_value,
                                          /* INOUT */ javacall_uint32* ret_value_len);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR sets adds a new attribute. If an attribute with that name is already present 
 * in the element, its value is changed to be that of the value 
 * parameter. This value is a simple string; it is not parsed as it is 
 * being set. So any markup (such as syntax to be recognized as an 
 * entity reference) is treated as literal text, and needs to be 
 * appropriately escaped by the implementation when it is written out. 
 * In order to assign an attribute value that contains entity 
 * references, the user must create an <code>Attr</code> node plus any 
 * <code>Text</code> and <code>EntityReference</code> nodes, build the 
 * appropriate subtree, and use <code>setAttributeNode</code> to assign 
 * it as the value of an attribute.
 * <br>To set an attribute with a qualified name and namespace URI, use 
 * the <code>setAttributeNS</code> method.
 * 
 * @param handle Pointer to the object representing this element.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param name The name of the attribute to create or alter.
 * @param value Value to set in string form.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_INVALID_CHARACTER_ERR
 *                            JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_INVALID_CHARACTER_ERR
 *                 JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_set_attribute_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_set_attribute_start(javacall_handle handle,
                                         javacall_int32 invocation_id,
                                         void **context,
                                         javacall_const_utf16_string name,
                                         javacall_const_utf16_string value,
                                         /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR sets adds a new attribute. If an attribute with that name is already present 
 * in the element, its value is changed to be that of the value 
 * parameter. This value is a simple string; it is not parsed as it is 
 * being set. So any markup (such as syntax to be recognized as an 
 * entity reference) is treated as literal text, and needs to be 
 * appropriately escaped by the implementation when it is written out. 
 * In order to assign an attribute value that contains entity 
 * references, the user must create an <code>Attr</code> node plus any 
 * <code>Text</code> and <code>EntityReference</code> nodes, build the 
 * appropriate subtree, and use <code>setAttributeNode</code> to assign 
 * it as the value of an attribute.
 * <br>To set an attribute with a qualified name and namespace URI, use 
 * the <code>setAttributeNS</code> method.
 * 
 * @param context The context saved during asynchronous operation.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_INVALID_CHARACTER_ERR
 *                            JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_INVALID_CHARACTER_ERR
 *                 JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_set_attribute_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_set_attribute_finish(void *context,
                                          /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR removes an attribute by name. If a default value for the removed 
 * attribute is defined in the DTD, a new attribute immediately appears 
 * with the default value as well as the corresponding namespace URI, 
 * local name, and prefix when applicable.
 * <br>If no attribute with this name is found, this method has no effect.
 * <br>To remove an attribute by local name and namespace URI, use the 
 * <code>removeAttributeNS</code> method.
 * 
 * @param handle Pointer to the object representing this element.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param name The name of the attribute to remove.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_remove_attribute_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_remove_attribute_start(javacall_handle handle,
                                            javacall_int32 invocation_id,
                                            void **context,
                                            javacall_const_utf16_string name,
                                            /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR removes an attribute by name. If a default value for the removed 
 * attribute is defined in the DTD, a new attribute immediately appears 
 * with the default value as well as the corresponding namespace URI, 
 * local name, and prefix when applicable.
 * <br>If no attribute with this name is found, this method has no effect.
 * <br>To remove an attribute by local name and namespace URI, use the 
 * <code>removeAttributeNS</code> method.
 * 
 * @param context The context saved during asynchronous operation.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_remove_attribute_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_remove_attribute_finish(void *context,
                                             /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns retrieves an attribute node by name.
 * <br>To retrieve an attribute node by qualified name and namespace URI, 
 * use the <code>getAttributeNodeNS</code> method.
 * 
 * @param handle Pointer to the object representing this element.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param name The name (<code>nodeName</code>) of the attribute to 
 *   retrieve.
 * @param ret_value Pointer to the object representing 
 *   the <code>Attr</code> node with the specified name (
 *   <code>nodeName</code>) or <code>NULL</code> if there is no such 
 *   attribute.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_get_attribute_node_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_get_attribute_node_start(javacall_handle handle,
                                              javacall_int32 invocation_id,
                                              void **context,
                                              javacall_const_utf16_string name,
                                              /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns retrieves an attribute node by name.
 * <br>To retrieve an attribute node by qualified name and namespace URI, 
 * use the <code>getAttributeNodeNS</code> method.
 * 
 * @param context The context saved during asynchronous operation.
 *   retrieve.
 * @param ret_value Pointer to the object representing 
 *   the <code>Attr</code> node with the specified name (
 *   <code>nodeName</code>) or <code>NULL</code> if there is no such 
 *   attribute.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_get_attribute_node_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_get_attribute_node_finish(void *context,
                                               /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR sets adds a new attribute node. If an attribute with that name (
 * <code>nodeName</code>) is already present in the element, it is 
 * replaced by the new one. Replacing an attribute node by itself has no 
 * effect.
 * <br>To add a new attribute node with a qualified name and namespace 
 * URI, use the <code>setAttributeNodeNS</code> method.
 * 
 * @param handle Pointer to the object representing this element.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param new_attr Pointer to the object of
 *   the <code>Attr</code> node to add to the attribute list.
 * @param ret_value Pointer to the object representing 
 *   if the <code>new_attr</code> attribute replaces an existing 
 *   attribute, the replaced <code>Attr</code> node is returned, 
 *   otherwise <code>NULL</code> is returned.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_WRONG_DOCUMENT_ERR
 *                            JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *                            JAVACALL_DOM_INUSE_ATTRIBUTE_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_WRONG_DOCUMENT_ERR
 *                 JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *                 JAVACALL_DOM_INUSE_ATTRIBUTE_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_set_attribute_node_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_set_attribute_node_start(javacall_handle handle,
                                              javacall_int32 invocation_id,
                                              void **context,
                                              javacall_handle new_attr,
                                              /* OUT */ javacall_handle* ret_value,
                                              /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR sets adds a new attribute node. If an attribute with that name (
 * <code>nodeName</code>) is already present in the element, it is 
 * replaced by the new one. Replacing an attribute node by itself has no 
 * effect.
 * <br>To add a new attribute node with a qualified name and namespace 
 * URI, use the <code>setAttributeNodeNS</code> method.
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   if the <code>newAttr</code> attribute replaces an existing 
 *   attribute, the replaced <code>Attr</code> node is returned, 
 *   otherwise <code>NULL</code> is returned.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_WRONG_DOCUMENT_ERR
 *                            JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *                            JAVACALL_DOM_INUSE_ATTRIBUTE_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_WRONG_DOCUMENT_ERR
 *                 JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *                 JAVACALL_DOM_INUSE_ATTRIBUTE_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_set_attribute_node_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_set_attribute_node_finish(void *context,
                                               /* OUT */ javacall_handle* ret_value,
                                               /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR removes the specified attribute node. If a default value for the 
 * removed <code>Attr</code> node is defined in the DTD, a new node 
 * immediately appears with the default value as well as the 
 * corresponding namespace URI, local name, and prefix when applicable. 
 * 
 * @param handle Pointer to the object representing this element.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param old_attr Pointer to the object of
 *   the <code>Attr</code> node to remove from the attribute 
 *   list.
 * @param ret_value Pointer to the object representing 
 *   the <code>Attr</code> node that was removed.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *                            JAVACALL_DOM_NOT_FOUND_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *                 JAVACALL_DOM_NOT_FOUND_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_remove_attribute_node_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_remove_attribute_node_start(javacall_handle handle,
                                                 javacall_int32 invocation_id,
                                                 void **context,
                                                 javacall_handle old_attr,
                                                 /* OUT */ javacall_handle* ret_value,
                                                 /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR removes the specified attribute node. If a default value for the 
 * removed <code>Attr</code> node is defined in the DTD, a new node 
 * immediately appears with the default value as well as the 
 * corresponding namespace URI, local name, and prefix when applicable. 
 * 
 * @param context The context saved during asynchronous operation.
 *   list.
 * @param ret_value Pointer to the object representing 
 *   the <code>Attr</code> node that was removed.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *                            JAVACALL_DOM_NOT_FOUND_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *                 JAVACALL_DOM_NOT_FOUND_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_remove_attribute_node_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_remove_attribute_node_finish(void *context,
                                                  /* OUT */ javacall_handle* ret_value,
                                                  /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns a <code>NodeList</code> of all descendant <code>Elements</code> 
 * with a given tag name, in document order.
 * 
 * @param handle Pointer to the object representing this element.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param name The name of the tag to match on. The special value "*" 
 *   matches all tags.
 * @param ret_value Pointer to the object representing 
 *   a list of matching <code>Element</code> nodes.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_get_elements_by_tag_name_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_get_elements_by_tag_name_start(javacall_handle handle,
                                                    javacall_int32 invocation_id,
                                                    void **context,
                                                    javacall_const_utf16_string name,
                                                    /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns a <code>NodeList</code> of all descendant <code>Elements</code> 
 * with a given tag name, in document order.
 * 
 * @param context The context saved during asynchronous operation.
 *   matches all tags.
 * @param ret_value Pointer to the object representing 
 *   a list of matching <code>Element</code> nodes.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_get_elements_by_tag_name_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_get_elements_by_tag_name_finish(void *context,
                                                     /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns retrieves an attribute value by local name and namespace URI. 
 * <br>Per [<a href='http://www.w3.org/TR/1999/REC-xml-names-19990114/'>XML Namespaces</a>]
 * , applications must use the value <code>NULL</code> as the 
 * <code>namespace_uri</code> parameter for methods if they wish to have 
 * no namespace.
 * 
 * Note: If ret_value_len is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill ret_value_len 
 *       with actual length of the returned string.
 *
 * @param handle Pointer to the object representing this element.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param namespace_uri The namespace URI of the attribute to retrieve.
 * @param local_name The local name of the attribute to retrieve.
 * @param ret_value The <code>Attr</code> value as a string, or the empty string 
 *   if that attribute does not have a specified or default value.
 * @param ret_value_len Number of code_units of the returned string
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_NOT_SUPPORTED_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context or if length of the returend string is more then 
 *             specified in ret_value_len,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_NOT_SUPPORTED_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_get_attribute_ns_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_get_attribute_ns_start(javacall_handle handle,
                                            javacall_int32 invocation_id,
                                            void **context,
                                            javacall_const_utf16_string namespace_uri,
                                            javacall_const_utf16_string local_name,
                                            /* OUT */ javacall_utf16** ret_value,
                                            /* INOUT */ javacall_uint32* ret_value_len,
                                            /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns retrieves an attribute value by local name and namespace URI. 
 * <br>Per [<a href='http://www.w3.org/TR/1999/REC-xml-names-19990114/'>XML Namespaces</a>]
 * , applications must use the value <code>NULL</code> as the 
 * <code>namespaceURI</code> parameter for methods if they wish to have 
 * no namespace.
 * 
 * Note: If ret_value_len is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill ret_value_len 
 *       with actual length of the returned string.
 *
 * @param context The context saved during asynchronous operation.
 * @param ret_value The <code>Attr</code> value as a string, or the empty string 
 *   if that attribute does not have a specified or default value.
 * @param ret_value_len Number of code_units of the returned string
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_NOT_SUPPORTED_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_OUT_OF_MEMORY if length of the returend string is more then 
 *             specified in ret_value_len,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_NOT_SUPPORTED_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_get_attribute_ns_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_get_attribute_ns_finish(void *context,
                                             /* OUT */ javacall_utf16** ret_value,
                                             /* INOUT */ javacall_uint32* ret_value_len,
                                             /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR sets adds a new attribute. If an attribute with the same local name and 
 * namespace URI is already present on the element, its prefix is 
 * changed to be the prefix part of the <code>qualified_name</code>, and 
 * its value is changed to be the <code>value</code> parameter. This 
 * value is a simple string; it is not parsed as it is being set. So any 
 * markup (such as syntax to be recognized as an entity reference) is 
 * treated as literal text, and needs to be appropriately escaped by the 
 * implementation when it is written out. In order to assign an 
 * attribute value that contains entity references, the user must create 
 * an <code>Attr</code> node plus any <code>Text</code> and 
 * <code>EntityReference</code> nodes, build the appropriate subtree, 
 * and use <code>setAttributeNodeNS</code> or 
 * <code>setAttributeNode</code> to assign it as the value of an 
 * attribute.
 * <br>Per [<a href='http://www.w3.org/TR/1999/REC-xml-names-19990114/'>XML Namespaces</a>]
 * , applications must use the value <code>NULL</code> as the 
 * <code>namespace_uri</code> parameter for methods if they wish to have 
 * no namespace.
 * 
 * @param handle Pointer to the object representing this element.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param namespace_uri The namespace URI of the attribute to create or 
 *   alter.
 * @param qualified_name The qualified name of the attribute to create or 
 *   alter.
 * @param value The value to set in string form.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_INVALID_CHARACTER_ERR
 *                            JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *                            JAVACALL_DOM_NAMESPACE_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_INVALID_CHARACTER_ERR
 *                 JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *                 JAVACALL_DOM_NAMESPACE_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_set_attribute_ns_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_set_attribute_ns_start(javacall_handle handle,
                                            javacall_int32 invocation_id,
                                            void **context,
                                            javacall_const_utf16_string namespace_uri,
                                            javacall_const_utf16_string qualified_name,
                                            javacall_const_utf16_string value,
                                            /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR sets adds a new attribute. If an attribute with the same local name and 
 * namespace URI is already present on the element, its prefix is 
 * changed to be the prefix part of the <code>qualifiedName</code>, and 
 * its value is changed to be the <code>value</code> parameter. This 
 * value is a simple string; it is not parsed as it is being set. So any 
 * markup (such as syntax to be recognized as an entity reference) is 
 * treated as literal text, and needs to be appropriately escaped by the 
 * implementation when it is written out. In order to assign an 
 * attribute value that contains entity references, the user must create 
 * an <code>Attr</code> node plus any <code>Text</code> and 
 * <code>EntityReference</code> nodes, build the appropriate subtree, 
 * and use <code>setAttributeNodeNS</code> or 
 * <code>setAttributeNode</code> to assign it as the value of an 
 * attribute.
 * <br>Per [<a href='http://www.w3.org/TR/1999/REC-xml-names-19990114/'>XML Namespaces</a>]
 * , applications must use the value <code>NULL</code> as the 
 * <code>namespaceURI</code> parameter for methods if they wish to have 
 * no namespace.
 * 
 * @param context The context saved during asynchronous operation.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_INVALID_CHARACTER_ERR
 *                            JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *                            JAVACALL_DOM_NAMESPACE_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_INVALID_CHARACTER_ERR
 *                 JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *                 JAVACALL_DOM_NAMESPACE_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_set_attribute_ns_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_set_attribute_ns_finish(void *context,
                                             /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR removes an attribute by local name and namespace URI.If a default 
 * value for the removed attribute is defined in the DTD, a new 
 * attribute immediately appears with the default value as well as the 
 * corresponding namespace URI, local name, and prefix when applicable. 
 * <br>If no attribute with this local name and namespace URI is found, 
 * this method has no effect.
 * <br>Per [<a href='http://www.w3.org/TR/1999/REC-xml-names-19990114/'>XML Namespaces</a>]
 * , applications must use the value <code>NULL</code> as the 
 * <code>namespace_uri</code> parameter for methods if they wish to have 
 * no namespace.
 * 
 * @param handle Pointer to the object representing this element.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param namespace_uri The namespace URI of the attribute to remove.
 * @param local_name The local name of the attribute to remove.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *                            JAVACALL_DOM_NOT_SUPPORTED_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *                 JAVACALL_DOM_NOT_SUPPORTED_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_remove_attribute_ns_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_remove_attribute_ns_start(javacall_handle handle,
                                               javacall_int32 invocation_id,
                                               void **context,
                                               javacall_const_utf16_string namespace_uri,
                                               javacall_const_utf16_string local_name,
                                               /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR removes an attribute by local name and namespace URI.If a default 
 * value for the removed attribute is defined in the DTD, a new 
 * attribute immediately appears with the default value as well as the 
 * corresponding namespace URI, local name, and prefix when applicable. 
 * <br>If no attribute with this local name and namespace URI is found, 
 * this method has no effect.
 * <br>Per [<a href='http://www.w3.org/TR/1999/REC-xml-names-19990114/'>XML Namespaces</a>]
 * , applications must use the value <code>NULL</code> as the 
 * <code>namespaceURI</code> parameter for methods if they wish to have 
 * no namespace.
 * 
 * @param context The context saved during asynchronous operation.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *                            JAVACALL_DOM_NOT_SUPPORTED_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *                 JAVACALL_DOM_NOT_SUPPORTED_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_remove_attribute_ns_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_remove_attribute_ns_finish(void *context,
                                                /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns retrieves an <code>Attr</code> node by local name and namespace URI.
 * <br>Per [<a href='http://www.w3.org/TR/1999/REC-xml-names-19990114/'>XML Namespaces</a>]
 * , applications must use the value <code>NULL</code> as the 
 * <code>namespace_uri</code> parameter for methods if they wish to have 
 * no namespace. 
 * 
 * @param handle Pointer to the object representing this element.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param namespace_uri The namespace URI of the attribute to retrieve.
 * @param local_name The local name of the attribute to retrieve.
 * @param ret_value Pointer to the object representing 
 *   the <code>Attr</code> node with the specified attribute local 
 *   name and namespace URI or <code>NULL</code> if there is no such 
 *   attribute.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_NOT_SUPPORTED_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_NOT_SUPPORTED_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_get_attribute_node_ns_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_get_attribute_node_ns_start(javacall_handle handle,
                                                 javacall_int32 invocation_id,
                                                 void **context,
                                                 javacall_const_utf16_string namespace_uri,
                                                 javacall_const_utf16_string local_name,
                                                 /* OUT */ javacall_handle* ret_value,
                                                 /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns retrieves an <code>Attr</code> node by local name and namespace URI.
 * <br>Per [<a href='http://www.w3.org/TR/1999/REC-xml-names-19990114/'>XML Namespaces</a>]
 * , applications must use the value <code>NULL</code> as the 
 * <code>namespaceURI</code> parameter for methods if they wish to have 
 * no namespace. 
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   the <code>Attr</code> node with the specified attribute local 
 *   name and namespace URI or <code>NULL</code> if there is no such 
 *   attribute.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_NOT_SUPPORTED_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_NOT_SUPPORTED_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_get_attribute_node_ns_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_get_attribute_node_ns_finish(void *context,
                                                  /* OUT */ javacall_handle* ret_value,
                                                  /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR sets adds a new attribute. If an attribute with that local name and that 
 * namespace URI is already present in the element, it is replaced by 
 * the new one. Replacing an attribute node by itself has no effect.
 * <br>Per [<a href='http://www.w3.org/TR/1999/REC-xml-names-19990114/'>XML Namespaces</a>]
 * , applications must use the value <code>NULL</code> as the 
 * <code>namespaceURI</code> parameter for methods if they wish to have 
 * no namespace.
 * 
 * @param handle Pointer to the object representing this element.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param new_attr Pointer to the object of
 *   the <code>Attr</code> node to add to the attribute list.
 * @param ret_value Pointer to the object representing 
 *   if the <code>new_attr</code> attribute replaces an existing 
 *   attribute with the same local name and namespace URI, the replaced 
 *   <code>Attr</code> node is returned, otherwise <code>NULL</code> is 
 *   returned.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_WRONG_DOCUMENT_ERR
 *                            JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *                            JAVACALL_DOM_INUSE_ATTRIBUTE_ERR
 *                            JAVACALL_DOM_NOT_SUPPORTED_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_WRONG_DOCUMENT_ERR
 *                 JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *                 JAVACALL_DOM_INUSE_ATTRIBUTE_ERR
 *                 JAVACALL_DOM_NOT_SUPPORTED_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_set_attribute_node_ns_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_set_attribute_node_ns_start(javacall_handle handle,
                                                 javacall_int32 invocation_id,
                                                 void **context,
                                                 javacall_handle new_attr,
                                                 /* OUT */ javacall_handle* ret_value,
                                                 /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR sets adds a new attribute. If an attribute with that local name and that 
 * namespace URI is already present in the element, it is replaced by 
 * the new one. Replacing an attribute node by itself has no effect.
 * <br>Per [<a href='http://www.w3.org/TR/1999/REC-xml-names-19990114/'>XML Namespaces</a>]
 * , applications must use the value <code>NULL</code> as the 
 * <code>namespaceURI</code> parameter for methods if they wish to have 
 * no namespace.
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   if the <code>newAttr</code> attribute replaces an existing 
 *   attribute with the same local name and namespace URI, the replaced 
 *   <code>Attr</code> node is returned, otherwise <code>NULL</code> is 
 *   returned.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_WRONG_DOCUMENT_ERR
 *                            JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *                            JAVACALL_DOM_INUSE_ATTRIBUTE_ERR
 *                            JAVACALL_DOM_NOT_SUPPORTED_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_WRONG_DOCUMENT_ERR
 *                 JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *                 JAVACALL_DOM_INUSE_ATTRIBUTE_ERR
 *                 JAVACALL_DOM_NOT_SUPPORTED_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_set_attribute_node_ns_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_set_attribute_node_ns_finish(void *context,
                                                  /* OUT */ javacall_handle* ret_value,
                                                  /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns a <code>NodeList</code> of all the descendant 
 * <code>Elements</code> with a given local name and namespace URI in 
 * document order.
 * 
 * @param handle Pointer to the object representing this element.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param namespace_uri The namespace URI of the elements to match on. The 
 *   special value "*" matches all namespaces.
 * @param local_name The local name of the elements to match on. The 
 *   special value "*" matches all local names.
 * @param ret_value Pointer to the object representing 
 *   a new <code>NodeList</code> object containing all the matched 
 *   <code>Elements</code>.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_NOT_SUPPORTED_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_NOT_SUPPORTED_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_get_elements_by_tag_name_ns_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_get_elements_by_tag_name_ns_start(javacall_handle handle,
                                                       javacall_int32 invocation_id,
                                                       void **context,
                                                       javacall_const_utf16_string namespace_uri,
                                                       javacall_const_utf16_string local_name,
                                                       /* OUT */ javacall_handle* ret_value,
                                                       /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns a <code>NodeList</code> of all the descendant 
 * <code>Elements</code> with a given local name and namespace URI in 
 * document order.
 * 
 * @param context The context saved during asynchronous operation.
 *   special value "*" matches all local names.
 * @param ret_value Pointer to the object representing 
 *   a new <code>NodeList</code> object containing all the matched 
 *   <code>Elements</code>.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_NOT_SUPPORTED_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_NOT_SUPPORTED_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_get_elements_by_tag_name_ns_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_get_elements_by_tag_name_ns_finish(void *context,
                                                        /* OUT */ javacall_handle* ret_value,
                                                        /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns <code>true</code> when an attribute with a given name is 
 * specified on this element or has a default value, <code>false</code> 
 * otherwise.
 * 
 * @param handle Pointer to the object representing this element.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param name The name of the attribute to look for.
 * @param ret_value <code>true</code> if an attribute with the given name is 
 *   specified on this element or has a default value, <code>false</code>
 *   otherwise.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_has_attribute_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_has_attribute_start(javacall_handle handle,
                                         javacall_int32 invocation_id,
                                         void **context,
                                         javacall_const_utf16_string name,
                                         /* OUT */ javacall_bool* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns <code>true</code> when an attribute with a given name is 
 * specified on this element or has a default value, <code>false</code> 
 * otherwise.
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value <code>true</code> if an attribute with the given name is 
 *   specified on this element or has a default value, <code>false</code>
 *   otherwise.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_has_attribute_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_has_attribute_finish(void *context,
                                          /* OUT */ javacall_bool* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns <code>true</code> when an attribute with a given local name and 
 * namespace URI is specified on this element or has a default value, 
 * <code>false</code> otherwise.
 * <br>Per [<a href='http://www.w3.org/TR/1999/REC-xml-names-19990114/'>XML Namespaces</a>]
 * , applications must use the value <code>NULL</code> as the 
 * <code>namespace_uri</code> parameter for methods if they wish to have 
 * no namespace.
 * 
 * @param handle Pointer to the object representing this element.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param namespace_uri The namespace URI of the attribute to look for.
 * @param local_name The local name of the attribute to look for.
 * @param ret_value <code>true</code> if an attribute with the given local name 
 *   and namespace URI is specified or has a default value on this 
 *   element, <code>false</code> otherwise.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_has_attribute_ns_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_has_attribute_ns_start(javacall_handle handle,
                                            javacall_int32 invocation_id,
                                            void **context,
                                            javacall_const_utf16_string namespace_uri,
                                            javacall_const_utf16_string local_name,
                                            /* OUT */ javacall_bool* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns <code>true</code> when an attribute with a given local name and 
 * namespace URI is specified on this element or has a default value, 
 * <code>false</code> otherwise.
 * <br>Per [<a href='http://www.w3.org/TR/1999/REC-xml-names-19990114/'>XML Namespaces</a>]
 * , applications must use the value <code>NULL</code> as the 
 * <code>namespaceURI</code> parameter for methods if they wish to have 
 * no namespace.
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value <code>true</code> if an attribute with the given local name 
 *   and namespace URI is specified or has a default value on this 
 *   element, <code>false</code> otherwise.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_has_attribute_ns_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_has_attribute_ns_finish(void *context,
                                             /* OUT */ javacall_bool* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR sets  If the parameter <code>is_id</code> is <code>true</code>, this method 
 * declares the specified attribute to be a user-determined ID attribute
 * . This affects the value of <code>Attr.is_id</code> and the behavior 
 * of <code>Document.getElementById</code>.
 * Use the value <code>false</code> for the parameter 
 * <code>is_id</code> to undeclare an attribute for being a 
 * user-determined ID attribute. 
 * <br> To specify an attribute by local name and namespace URI, use the 
 * <code>setIdAttributeNS</code> method. 
 * 
 * @param handle Pointer to the object representing this element.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param name The name of the attribute.
 * @param is_id Whether the attribute is a of type ID.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *                            JAVACALL_DOM_NOT_FOUND_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *                 JAVACALL_DOM_NOT_FOUND_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_set_id_attribute_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_set_id_attribute_start(javacall_handle handle,
                                            javacall_int32 invocation_id,
                                            void **context,
                                            javacall_const_utf16_string name,
                                            javacall_bool is_id,
                                            /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR sets  If the parameter <code>isId</code> is <code>true</code>, this method 
 * declares the specified attribute to be a user-determined ID attribute
 * . This affects the value of <code>Attr.isId</code> and the behavior 
 * of <code>Document.getElementById</code>.
 * Use the value <code>false</code> for the parameter 
 * <code>isId</code> to undeclare an attribute for being a 
 * user-determined ID attribute. 
 * <br> To specify an attribute by local name and namespace URI, use the 
 * <code>setIdAttributeNS</code> method. 
 * 
 * @param context The context saved during asynchronous operation.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *                            JAVACALL_DOM_NOT_FOUND_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *                 JAVACALL_DOM_NOT_FOUND_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_set_id_attribute_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_set_id_attribute_finish(void *context,
                                             /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR sets  If the parameter <code>is_id</code> is <code>true</code>, this method 
 * declares the specified attribute to be a user-determined ID attribute
 * . This affects the value of <code>Attr.is_id</code> and the behavior 
 * of <code>Document.getElementById</code>.
 * Use the value <code>false</code> for the parameter 
 * <code>is_id</code> to undeclare an attribute for being a 
 * user-determined ID attribute. 
 * 
 * @param handle Pointer to the object representing this element.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param namespace_uri The namespace URI of the attribute.
 * @param local_name The local name of the attribute.
 * @param is_id Whether the attribute is a of type ID.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *                            JAVACALL_DOM_NOT_FOUND_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *                 JAVACALL_DOM_NOT_FOUND_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_set_id_attribute_ns_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_set_id_attribute_ns_start(javacall_handle handle,
                                               javacall_int32 invocation_id,
                                               void **context,
                                               javacall_const_utf16_string namespace_uri,
                                               javacall_const_utf16_string local_name,
                                               javacall_bool is_id,
                                               /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR sets  If the parameter <code>isId</code> is <code>true</code>, this method 
 * declares the specified attribute to be a user-determined ID attribute
 * . This affects the value of <code>Attr.isId</code> and the behavior 
 * of <code>Document.getElementById</code>.
 * Use the value <code>false</code> for the parameter 
 * <code>isId</code> to undeclare an attribute for being a 
 * user-determined ID attribute. 
 * 
 * @param context The context saved during asynchronous operation.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *                            JAVACALL_DOM_NOT_FOUND_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *                 JAVACALL_DOM_NOT_FOUND_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_set_id_attribute_ns_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_set_id_attribute_ns_finish(void *context,
                                                /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR sets  If the parameter <code>is_id</code> is <code>true</code>, this method 
 * declares the specified attribute to be a user-determined ID attribute
 * . This affects the value of <code>Attr.is_id</code> and the behavior 
 * of <code>Document.getElementById</code>.
 * Use the value <code>false</code> for the parameter 
 * <code>is_id</code> to undeclare an attribute for being a 
 * user-determined ID attribute. 
 * 
 * @param handle Pointer to the object representing this element.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param id_attr Pointer to the object of
 *   the attribute node.
 * @param is_id Whether the attribute is a of type ID.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *                            JAVACALL_DOM_NOT_FOUND_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *                 JAVACALL_DOM_NOT_FOUND_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_set_id_attribute_node_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_set_id_attribute_node_start(javacall_handle handle,
                                                 javacall_int32 invocation_id,
                                                 void **context,
                                                 javacall_handle id_attr,
                                                 javacall_bool is_id,
                                                 /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR sets  If the parameter <code>isId</code> is <code>true</code>, this method 
 * declares the specified attribute to be a user-determined ID attribute
 * . This affects the value of <code>Attr.isId</code> and the behavior 
 * of <code>Document.getElementById</code>.
 * Use the value <code>false</code> for the parameter 
 * <code>isId</code> to undeclare an attribute for being a 
 * user-determined ID attribute. 
 * 
 * @param context The context saved during asynchronous operation.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *                            JAVACALL_DOM_NOT_FOUND_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *                 JAVACALL_DOM_NOT_FOUND_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_set_id_attribute_node_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_set_id_attribute_node_finish(void *context,
                                                  /* OUT */ javacall_dom_exceptions* exception_code);

/*
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR checks if this Element is HTMLElement
 * 
 * 
 * @param handle Pointer to the object representing this element.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param ret_value <code>true</code> if the Element is HTMLElement,
 * <code>false</code> otherwise. 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_is_html_element_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_is_html_element_start(javacall_handle handle,
                                           javacall_int32 invocation_id,
                                           void **context,
                                           /* OUT */ javacall_bool* ret_value);

/*
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR checks if this Element is HTMLElement
 * 
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value <code>true</code> if the Element is HTMLElement,
 * <code>false</code> otherwise. 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_is_html_element_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_is_html_element_finish(void *context,
                                            /* OUT */ javacall_bool* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns retrieves the number of child elements.
 *
 * 
 * @param handle Pointer to the object representing this element.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param ret_value the current number of element nodes that are immediate children
 * of this element. <code>0</code> if this element has no child elements.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_get_child_element_count_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_get_child_element_count_start(javacall_handle handle,
                                                   javacall_int32 invocation_id,
                                                   void **context,
                                                   /* OUT */ javacall_int32* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns retrieves the number of child elements.
 *
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value the current number of element nodes that are immediate children
 * of this element. <code>0</code> if this element has no child elements.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_get_child_element_count_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_get_child_element_count_finish(void *context,
                                                    /* OUT */ javacall_int32* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns retrieves the first child element.
 * 
 * 
 * @param handle Pointer to the object representing this element.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   the first child element node of this element.
 * <code>NULL</code> if this element has no child elements.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_get_first_element_child_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_get_first_element_child_start(javacall_handle handle,
                                                   javacall_int32 invocation_id,
                                                   void **context,
                                                   /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns retrieves the first child element.
 * 
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   the first child element node of this element.
 * <code>NULL</code> if this element has no child elements.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_get_first_element_child_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_get_first_element_child_finish(void *context,
                                                    /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns retrieves the last child element.
 *
 * 
 * @param handle Pointer to the object representing this element.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   the last child element node of this element.
 * <code>NULL</code> if this element has no child elements.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_get_last_element_child_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_get_last_element_child_start(javacall_handle handle,
                                                  javacall_int32 invocation_id,
                                                  void **context,
                                                  /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns retrieves the last child element.
 *
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   the last child element node of this element.
 * <code>NULL</code> if this element has no child elements.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_get_last_element_child_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_get_last_element_child_finish(void *context,
                                                   /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns retrieves the next sibling element.
 * 
 * 
 * @param handle Pointer to the object representing this element.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   the next sibling element node of this element.
 * <code>NULL</code> if this element has no element sibling nodes
 * that come after this one in the document tree.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_get_next_element_sibling_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_get_next_element_sibling_start(javacall_handle handle,
                                                    javacall_int32 invocation_id,
                                                    void **context,
                                                    /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns retrieves the next sibling element.
 * 
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   the next sibling element node of this element.
 * <code>NULL</code> if this element has no element sibling nodes
 * that come after this one in the document tree.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_get_next_element_sibling_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_get_next_element_sibling_finish(void *context,
                                                     /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns retrieves the previous sibling element.
 * 
 * 
 * @param handle Pointer to the object representing this element.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   the previous sibling element node of this element.
 * <code>NULL</code> if this element has no element sibling nodes
 * that come before this one in the document tree.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_get_previous_element_sibling_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_get_previous_element_sibling_start(javacall_handle handle,
                                                        javacall_int32 invocation_id,
                                                        void **context,
                                                        /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns retrieves the previous sibling element.
 * 
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   the previous sibling element node of this element.
 * <code>NULL</code> if this element has no element sibling nodes
 * that come before this one in the document tree.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_get_previous_element_sibling_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_get_previous_element_sibling_finish(void *context,
                                                         /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the value of the <code>contentDocument</code> attribute.
 *
 * <p>The value of the <code>contentDocument</code> attribute of an object
 * that implements the <code>EmbeddingElement</code> interface 
 * <span class="rfc2119">must</span> be the child document's 
 * <code>Document</code> object or <code>NULL</code> if there is no such
 * object.</p>
 *
 * 
 * @param handle Pointer to the object representing this element.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   the value of the <code>contentDocument</code> attribute. If
 * none is available, then the value is <code>NULL</code>.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_get_content_document_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_get_content_document_start(javacall_handle handle,
                                                javacall_int32 invocation_id,
                                                void **context,
                                                /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the value of the <code>contentDocument</code> attribute.
 *
 * <p>The value of the <code>contentDocument</code> attribute of an object
 * that implements the <code>EmbeddingElement</code> interface 
 * <span class="rfc2119">must</span> be the child document's 
 * <code>Document</code> object or <code>NULL</code> if there is no such
 * object.</p>
 *
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   the value of the <code>contentDocument</code> attribute. If
 * none is available, then the value is <code>NULL</code>.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_get_content_document_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_get_content_document_finish(void *context,
                                                 /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the value of the <code>contentWindow</code> attribute.
 *
 * <p>The value of the <code>contentWindow</code> attribute of an object
 * that implements the <code>EmbeddingElement</code> interface
 * <span class="rfc2119">must</span> be the child document's 
 * <code>Window</code> object or <code>NULL</code> if there is no such
 * object.</p>
 *
 * 
 * @param handle Pointer to the object representing this element.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   the value of the <code>contentWindow</code> attribute.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_get_content_window_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_get_content_window_start(javacall_handle handle,
                                              javacall_int32 invocation_id,
                                              void **context,
                                              /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the value of the <code>contentWindow</code> attribute.
 *
 * <p>The value of the <code>contentWindow</code> attribute of an object
 * that implements the <code>EmbeddingElement</code> interface
 * <span class="rfc2119">must</span> be the child document's 
 * <code>Window</code> object or <code>NULL</code> if there is no such
 * object.</p>
 *
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   the value of the <code>contentWindow</code> attribute.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_element_get_content_window_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_get_content_window_finish(void *context,
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
javacall_dom_element_clear_references(javacall_handle handle, javacall_int32 count);


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ifndef __JAVACALL_DOM_ELEMENT_H_ */
