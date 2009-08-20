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

#ifndef __JAVACALL_DOM_NAMEDNODEMAP_H_
#define __JAVACALL_DOM_NAMEDNODEMAP_H_

/**
 * @file javacall_dom_namednodemap.h
 * @ingroup JSR290DOM
 * @brief Javacall DOM interfaces for NamedNodeMap
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
 * OR returns retrieves a node specified by name.
 * 
 * @param handle Pointer to the object representing this namednodemap.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param name The <code>nodeName</code> of a node to retrieve.
 * @param ret_value Pointer to the object representing 
 *   a <code>Node</code> (of any type) with the specified 
 *   <code>nodeName</code>, or <code>NULL</code> if it does not identify 
 *   any node in this map.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_namednodemap_get_named_item_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_namednodemap_get_named_item_start(javacall_handle handle,
                                               javacall_int32 invocation_id,
                                               void **context,
                                               javacall_const_utf16_string name,
                                               /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns retrieves a node specified by name.
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   a <code>Node</code> (of any type) with the specified 
 *   <code>nodeName</code>, or <code>NULL</code> if it does not identify 
 *   any node in this map.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_namednodemap_get_named_item_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_namednodemap_get_named_item_finish(void *context,
                                                /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR sets adds a node using its <code>nodeName</code> attribute. If a node with 
 * that name is already present in this map, it is replaced by the new 
 * one.
 * <br>As the <code>nodeName</code> attribute is used to derive the name 
 * which the node must be stored under, multiple nodes of certain types 
 * (those that have a "special" string value) cannot be stored as the 
 * names would clash. This is seen as preferable to allowing nodes to be 
 * aliased.
 * 
 * @param handle Pointer to the object representing this namednodemap.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param arg Pointer to the object of
 *   a node to store in this map. The node will later be 
 *   accessible using the value of its <code>nodeName</code> attribute.
 * @param ret_value Pointer to the object representing 
 *   if the new <code>Node</code> replaces an existing node the 
 *   replaced <code>Node</code> is returned, otherwise <code>NULL</code> 
 *   is returned.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_WRONG_DOCUMENT_ERR
 *                            JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *                            JAVACALL_DOM_INUSE_ATTRIBUTE_ERR
 *                            JAVACALL_DOM_HIERARCHY_REQUEST_ERR
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
 *                 JAVACALL_DOM_HIERARCHY_REQUEST_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_namednodemap_set_named_item_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_namednodemap_set_named_item_start(javacall_handle handle,
                                               javacall_int32 invocation_id,
                                               void **context,
                                               javacall_handle arg,
                                               /* OUT */ javacall_handle* ret_value,
                                               /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR sets adds a node using its <code>nodeName</code> attribute. If a node with 
 * that name is already present in this map, it is replaced by the new 
 * one.
 * <br>As the <code>nodeName</code> attribute is used to derive the name 
 * which the node must be stored under, multiple nodes of certain types 
 * (those that have a "special" string value) cannot be stored as the 
 * names would clash. This is seen as preferable to allowing nodes to be 
 * aliased.
 * 
 * @param context The context saved during asynchronous operation.
 *   accessible using the value of its <code>nodeName</code> attribute.
 * @param ret_value Pointer to the object representing 
 *   if the new <code>Node</code> replaces an existing node the 
 *   replaced <code>Node</code> is returned, otherwise <code>NULL</code> 
 *   is returned.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_WRONG_DOCUMENT_ERR
 *                            JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *                            JAVACALL_DOM_INUSE_ATTRIBUTE_ERR
 *                            JAVACALL_DOM_HIERARCHY_REQUEST_ERR
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
 *                 JAVACALL_DOM_HIERARCHY_REQUEST_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_namednodemap_set_named_item_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_namednodemap_set_named_item_finish(void *context,
                                                /* OUT */ javacall_handle* ret_value,
                                                /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR removes a node specified by name. When this map contains the attributes 
 * attached to an element, if the removed attribute is known to have a 
 * default value, an attribute immediately appears containing the 
 * default value as well as the corresponding namespace URI, local name, 
 * and prefix when applicable.
 * 
 * @param handle Pointer to the object representing this namednodemap.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param name The <code>nodeName</code> of the node to remove.
 * @param ret_value Pointer to the object representing 
 *   the node removed from this map if a node with such a name 
 *   exists.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_NOT_FOUND_ERR
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
 *                 JAVACALL_DOM_NOT_FOUND_ERR
 *                 JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_namednodemap_remove_named_item_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_namednodemap_remove_named_item_start(javacall_handle handle,
                                                  javacall_int32 invocation_id,
                                                  void **context,
                                                  javacall_const_utf16_string name,
                                                  /* OUT */ javacall_handle* ret_value,
                                                  /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR removes a node specified by name. When this map contains the attributes 
 * attached to an element, if the removed attribute is known to have a 
 * default value, an attribute immediately appears containing the 
 * default value as well as the corresponding namespace URI, local name, 
 * and prefix when applicable.
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   the node removed from this map if a node with such a name 
 *   exists.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_NOT_FOUND_ERR
 *                            JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_NOT_FOUND_ERR
 *                 JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_namednodemap_remove_named_item_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_namednodemap_remove_named_item_finish(void *context,
                                                   /* OUT */ javacall_handle* ret_value,
                                                   /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the <code>index</code>th item in the map. If <code>index</code> 
 * is greater than or equal to the number of nodes in this map, this 
 * returns <code>NULL</code>.
 * 
 * @param handle Pointer to the object representing this namednodemap.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param index Index into this map.
 * @param ret_value Pointer to the object representing 
 *   the node at the <code>index</code>th position in the map, or 
 *   <code>NULL</code> if that is not a valid index.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_namednodemap_item_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_namednodemap_item_start(javacall_handle handle,
                                     javacall_int32 invocation_id,
                                     void **context,
                                     javacall_int32 index,
                                     /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the <code>index</code>th item in the map. If <code>index</code> 
 * is greater than or equal to the number of nodes in this map, this 
 * returns <code>NULL</code>.
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   the node at the <code>index</code>th position in the map, or 
 *   <code>NULL</code> if that is not a valid index.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_namednodemap_item_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_namednodemap_item_finish(void *context,
                                      /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the number of nodes in this map. The range of valid child node indices 
 * is <code>0</code> to <code>length-1</code> inclusive. 
 * 
 * @param handle Pointer to the object representing this namednodemap.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param ret_value The number of nodes in this map
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_namednodemap_get_length_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_namednodemap_get_length_start(javacall_handle handle,
                                           javacall_int32 invocation_id,
                                           void **context,
                                           /* OUT */ javacall_int32* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the number of nodes in this map. The range of valid child node indices 
 * is <code>0</code> to <code>length-1</code> inclusive. 
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value The number of nodes in this map
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_namednodemap_get_length_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_namednodemap_get_length_finish(void *context,
                                            /* OUT */ javacall_int32* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns retrieves a node specified by local name and namespace URI. 
 * 
 * @param handle Pointer to the object representing this namednodemap.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param namespace_uri The namespace URI of the node to retrieve.
 * @param local_name The local name of the node to retrieve.
 * @param ret_value Pointer to the object representing 
 *   a <code>Node</code> (of any type) with the specified local 
 *   name and namespace URI, or <code>NULL</code> if they do not 
 *   identify any node in this map.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_namednodemap_get_named_item_ns_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_namednodemap_get_named_item_ns_start(javacall_handle handle,
                                                  javacall_int32 invocation_id,
                                                  void **context,
                                                  javacall_const_utf16_string namespace_uri,
                                                  javacall_const_utf16_string local_name,
                                                  /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns retrieves a node specified by local name and namespace URI. 
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   a <code>Node</code> (of any type) with the specified local 
 *   name and namespace URI, or <code>NULL</code> if they do not 
 *   identify any node in this map.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_namednodemap_get_named_item_ns_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_namednodemap_get_named_item_ns_finish(void *context,
                                                   /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR sets adds a node using its <code>namespaceURI</code> and 
 * <code>localName</code>. If a node with that namespace URI and that 
 * local name is already present in this map, it is replaced by the new 
 * one.
 * 
 * @param handle Pointer to the object representing this namednodemap.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param arg Pointer to the object of
 *   a node to store in this map. The node will later be 
 *   accessible using the value of its <code>namespaceURI</code> and 
 *   <code>localName</code> attributes.
 * @param ret_value Pointer to the object representing 
 *   if the new <code>Node</code> replaces an existing node the 
 *   replaced <code>Node</code> is returned, otherwise <code>NULL</code> 
 *   is returned.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_WRONG_DOCUMENT_ERR
 *                            JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *                            JAVACALL_DOM_INUSE_ATTRIBUTE_ERR
 *                            JAVACALL_DOM_HIERARCHY_REQUEST_ERR
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
 *                 JAVACALL_DOM_HIERARCHY_REQUEST_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_namednodemap_set_named_item_ns_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_namednodemap_set_named_item_ns_start(javacall_handle handle,
                                                  javacall_int32 invocation_id,
                                                  void **context,
                                                  javacall_handle arg,
                                                  /* OUT */ javacall_handle* ret_value,
                                                  /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR sets adds a node using its <code>namespaceURI</code> and 
 * <code>localName</code>. If a node with that namespace URI and that 
 * local name is already present in this map, it is replaced by the new 
 * one.
 * 
 * @param context The context saved during asynchronous operation.
 *   accessible using the value of its <code>namespaceURI</code> and 
 *   <code>localName</code> attributes.
 * @param ret_value Pointer to the object representing 
 *   if the new <code>Node</code> replaces an existing node the 
 *   replaced <code>Node</code> is returned, otherwise <code>NULL</code> 
 *   is returned.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_WRONG_DOCUMENT_ERR
 *                            JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *                            JAVACALL_DOM_INUSE_ATTRIBUTE_ERR
 *                            JAVACALL_DOM_HIERARCHY_REQUEST_ERR
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
 *                 JAVACALL_DOM_HIERARCHY_REQUEST_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_namednodemap_set_named_item_ns_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_namednodemap_set_named_item_ns_finish(void *context,
                                                   /* OUT */ javacall_handle* ret_value,
                                                   /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR removes a node specified by local name and namespace URI. A removed 
 * attribute may be known to have a default value when this map contains 
 * the attributes attached to an element, as returned by the attributes 
 * attribute of the <code>Node</code> interface. If so, an attribute 
 * immediately appears containing the default value as well as the 
 * corresponding namespace URI, local name, and prefix when applicable.
 * 
 * @param handle Pointer to the object representing this namednodemap.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param namespace_uri The namespace URI of the node to remove.
 * @param local_name The local name of the node to remove.
 * @param ret_value Pointer to the object representing 
 *   the node removed from this map if a node with such a local 
 *   name and namespace URI exists.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_NOT_FOUND_ERR
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
 *                 JAVACALL_DOM_NOT_FOUND_ERR
 *                 JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_namednodemap_remove_named_item_ns_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_namednodemap_remove_named_item_ns_start(javacall_handle handle,
                                                     javacall_int32 invocation_id,
                                                     void **context,
                                                     javacall_const_utf16_string namespace_uri,
                                                     javacall_const_utf16_string local_name,
                                                     /* OUT */ javacall_handle* ret_value,
                                                     /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR removes a node specified by local name and namespace URI. A removed 
 * attribute may be known to have a default value when this map contains 
 * the attributes attached to an element, as returned by the attributes 
 * attribute of the <code>Node</code> interface. If so, an attribute 
 * immediately appears containing the default value as well as the 
 * corresponding namespace URI, local name, and prefix when applicable.
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   the node removed from this map if a node with such a local 
 *   name and namespace URI exists.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_NOT_FOUND_ERR
 *                            JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_NOT_FOUND_ERR
 *                 JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_namednodemap_remove_named_item_ns_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_namednodemap_remove_named_item_ns_finish(void *context,
                                                      /* OUT */ javacall_handle* ret_value,
                                                      /* OUT */ javacall_dom_exceptions* exception_code);

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
javacall_dom_namednodemap_clear_references(javacall_handle handle, javacall_int32 count);


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ifndef __JAVACALL_DOM_NAMEDNODEMAP_H_ */
