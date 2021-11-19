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

#ifndef __JAVACALL_DOM_DOCUMENT_H_
#define __JAVACALL_DOM_DOCUMENT_H_

/**
 * @file javacall_dom_document.h
 * @ingroup JSR290DOM
 * @brief Javacall DOM interfaces for Document
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
 * OR the Document Type Declaration (see <code>DocumentType</code>) 
 * associated with this document. For XML documents without a document
 * type declaration this returns <code>NULL</code>. For HTML documents,
 * a <code>DocumentType</code> object may be returned, independently of
 * the presence or absence of document type declaration in the HTML
 * document.
 * <br>This provides direct access to the <code>DocumentType</code> node, 
 * child node of this <code>Document</code>. This node can be set at 
 * document creation time and later changed through the use of child 
 * nodes manipulation methods, such as <code>Node.insertBefore</code>, 
 * or <code>Node.replaceChild</code>. Note, however, that while some 
 * implementations may instantiate different types of 
 * <code>Document</code> objects supporting additional features than the 
 * "Core", such as "HTML" [<a href='http://www.w3.org/TR/2003/REC-DOM-Level-2-HTML-20030109'>DOM Level 2 HTML</a>]
 * , based on the <code>DocumentType</code> specified at creation time, 
 * changing it afterwards is very unlikely to result in a change of the 
 * features supported.
 * 
 * @param handle Pointer to the object representing this document.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   the Document Type Declaration associated with this document, or <code>NULL</code>
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_document_get_doctype_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_document_get_doctype_start(javacall_handle handle,
                                        javacall_int32 invocation_id,
                                        void **context,
                                        /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR the Document Type Declaration (see <code>DocumentType</code>) 
 * associated with this document. For XML documents without a document
 * type declaration this returns <code>NULL</code>. For HTML documents,
 * a <code>DocumentType</code> object may be returned, independently of
 * the presence or absence of document type declaration in the HTML
 * document.
 * <br>This provides direct access to the <code>DocumentType</code> node, 
 * child node of this <code>Document</code>. This node can be set at 
 * document creation time and later changed through the use of child 
 * nodes manipulation methods, such as <code>Node.insertBefore</code>, 
 * or <code>Node.replaceChild</code>. Note, however, that while some 
 * implementations may instantiate different types of 
 * <code>Document</code> objects supporting additional features than the 
 * "Core", such as "HTML" [<a href='http://www.w3.org/TR/2003/REC-DOM-Level-2-HTML-20030109'>DOM Level 2 HTML</a>]
 * , based on the <code>DocumentType</code> specified at creation time, 
 * changing it afterwards is very unlikely to result in a change of the 
 * features supported.
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   the Document Type Declaration associated with this document, or <code>NULL</code>
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_document_get_doctype_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_document_get_doctype_finish(void *context,
                                         /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the <code>DOMImplementation</code> object that handles this document. A 
 * DOM application may use objects from multiple implementations.
 * 
 * @param handle Pointer to the object representing this document.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   the <code>DOMImplementation</code> object that handles this document
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_document_get_implementation_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_document_get_implementation_start(javacall_handle handle,
                                               javacall_int32 invocation_id,
                                               void **context,
                                               /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the <code>DOMImplementation</code> object that handles this document. A 
 * DOM application may use objects from multiple implementations.
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   the <code>DOMImplementation</code> object that handles this document
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_document_get_implementation_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_document_get_implementation_finish(void *context,
                                                /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns this is a convenience attribute that allows direct access to the child 
 * node that is the root element of the document. 
 * 
 * @param handle Pointer to the object representing this document.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   the child node that is the root element of the document
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_document_get_document_element_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_document_get_document_element_start(javacall_handle handle,
                                                 javacall_int32 invocation_id,
                                                 void **context,
                                                 /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns this is a convenience attribute that allows direct access to the child 
 * node that is the root element of the document. 
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   the child node that is the root element of the document
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_document_get_document_element_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_document_get_document_element_finish(void *context,
                                                  /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR creates an element of the type specified. Note that the instance 
 * returned implements the <code>Element</code> interface, so attributes 
 * can be specified directly on the returned object.
 * <br>In addition, if there are known attributes with default values, 
 * <code>Attr</code> nodes representing them are automatically created 
 * and attached to the element.
 * <br>To create an element with a qualified name and namespace URI, use 
 * the <code>createElementNS</code> method.
 * 
 * @param handle Pointer to the object representing this document.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param tag_name The name of the element type to instantiate. For XML, 
 *   this is case-sensitive. otherwise it depends on the 
 *   case-sensitivity of the markup language in use. In that case, the 
 *   name is mapped to the canonical form of that markup by the DOM 
 *   implementation.
 * @param ret_value Pointer to the object representing 
 *   a new <code>Element</code> object with the 
 *   <code>nodeName</code> attribute set to <code>tag_name</code>, and 
 *   <code>localName</code>, <code>prefix</code>, and 
 *   <code>namespaceURI</code> set to <code>NULL</code>.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_INVALID_CHARACTER_ERR
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
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_document_create_element_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_document_create_element_start(javacall_handle handle,
                                           javacall_int32 invocation_id,
                                           void **context,
                                           javacall_const_utf16_string tag_name,
                                           /* OUT */ javacall_handle* ret_value,
                                           /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR creates an element of the type specified. Note that the instance 
 * returned implements the <code>Element</code> interface, so attributes 
 * can be specified directly on the returned object.
 * <br>In addition, if there are known attributes with default values, 
 * <code>Attr</code> nodes representing them are automatically created 
 * and attached to the element.
 * <br>To create an element with a qualified name and namespace URI, use 
 * the <code>createElementNS</code> method.
 * 
 * @param context The context saved during asynchronous operation.
 *   this is case-sensitive. otherwise it depends on the 
 *   case-sensitivity of the markup language in use. In that case, the 
 *   name is mapped to the canonical form of that markup by the DOM 
 *   implementation.
 * @param ret_value Pointer to the object representing 
 *   a new <code>Element</code> object with the 
 *   <code>nodeName</code> attribute set to <code>tagName</code>, and 
 *   <code>localName</code>, <code>prefix</code>, and 
 *   <code>namespaceURI</code> set to <code>NULL</code>.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_INVALID_CHARACTER_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_INVALID_CHARACTER_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_document_create_element_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_document_create_element_finish(void *context,
                                            /* OUT */ javacall_handle* ret_value,
                                            /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR creates an empty <code>DocumentFragment</code> object. 
 * 
 * @param handle Pointer to the object representing this document.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   a new <code>DocumentFragment</code>.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_document_create_document_fragment_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_document_create_document_fragment_start(javacall_handle handle,
                                                     javacall_int32 invocation_id,
                                                     void **context,
                                                     /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR creates an empty <code>DocumentFragment</code> object. 
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   a new <code>DocumentFragment</code>.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_document_create_document_fragment_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_document_create_document_fragment_finish(void *context,
                                                      /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR creates a <code>Text</code> node given the specified string.
 * 
 * @param handle Pointer to the object representing this document.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param data The data for the node.
 * @param ret_value Pointer to the object representing 
 *   the new <code>Text</code> object.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_document_create_text_node_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_document_create_text_node_start(javacall_handle handle,
                                             javacall_int32 invocation_id,
                                             void **context,
                                             javacall_const_utf16_string data,
                                             /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR creates a <code>Text</code> node given the specified string.
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   the new <code>Text</code> object.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_document_create_text_node_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_document_create_text_node_finish(void *context,
                                              /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR creates a <code>Comment</code> node given the specified string.
 * 
 * @param handle Pointer to the object representing this document.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param data The data for the node.
 * @param ret_value Pointer to the object representing 
 *   the new <code>Comment</code> object.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_document_create_comment_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_document_create_comment_start(javacall_handle handle,
                                           javacall_int32 invocation_id,
                                           void **context,
                                           javacall_const_utf16_string data,
                                           /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR creates a <code>Comment</code> node given the specified string.
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   the new <code>Comment</code> object.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_document_create_comment_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_document_create_comment_finish(void *context,
                                            /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR creates a <code>CDATASection</code> node whose value is the specified 
 * string.
 * 
 * @param handle Pointer to the object representing this document.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param data The data for the <code>CDATASection</code> contents.
 * @param ret_value Pointer to the object representing 
 *   the new <code>CDATASection</code> object.
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
 *             javacall_dom_document_create_cdata_section_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_document_create_cdata_section_start(javacall_handle handle,
                                                 javacall_int32 invocation_id,
                                                 void **context,
                                                 javacall_const_utf16_string data,
                                                 /* OUT */ javacall_handle* ret_value,
                                                 /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR creates a <code>CDATASection</code> node whose value is the specified 
 * string.
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   the new <code>CDATASection</code> object.
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
 *             javacall_dom_document_create_cdata_section_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_document_create_cdata_section_finish(void *context,
                                                  /* OUT */ javacall_handle* ret_value,
                                                  /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR creates a <code>ProcessingInstruction</code> node given the specified 
 * name and data strings.
 * 
 * @param handle Pointer to the object representing this document.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param target The target part of the processing instruction. Unlike 
 *   <code>Document.createElementNS</code> or 
 *   <code>Document.createAttributeNS</code>, no namespace well-formed 
 *   checking is done on the target name.
 * @param data The data for the node.
 * @param ret_value Pointer to the object representing 
 *   the new <code>ProcessingInstruction</code> object.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_INVALID_CHARACTER_ERR
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
 *                 JAVACALL_DOM_INVALID_CHARACTER_ERR
 *                 JAVACALL_DOM_NOT_SUPPORTED_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_document_create_processing_instruction_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_document_create_processing_instruction_start(javacall_handle handle,
                                                          javacall_int32 invocation_id,
                                                          void **context,
                                                          javacall_const_utf16_string target,
                                                          javacall_const_utf16_string data,
                                                          /* OUT */ javacall_handle* ret_value,
                                                          /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR creates a <code>ProcessingInstruction</code> node given the specified 
 * name and data strings.
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   the new <code>ProcessingInstruction</code> object.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_INVALID_CHARACTER_ERR
 *                            JAVACALL_DOM_NOT_SUPPORTED_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_INVALID_CHARACTER_ERR
 *                 JAVACALL_DOM_NOT_SUPPORTED_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_document_create_processing_instruction_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_document_create_processing_instruction_finish(void *context,
                                                           /* OUT */ javacall_handle* ret_value,
                                                           /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR creates an <code>Attr</code> of the given name. Note that the 
 * <code>Attr</code> instance can then be set on an <code>Element</code> 
 * using the <code>setAttributeNode</code> method. 
 * <br>To create an attribute with a qualified name and namespace URI, use 
 * the <code>createAttributeNS</code> method.
 * 
 * @param handle Pointer to the object representing this document.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param name The name of the attribute.
 * @param ret_value Pointer to the object representing 
 *   a new <code>Attr</code> object with the <code>nodeName</code> 
 *   attribute set to <code>name</code>, and <code>localName</code>, 
 *   <code>prefix</code>, and <code>namespaceURI</code> set to 
 *   <code>NULL</code>. The value of the attribute is the empty string.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_INVALID_CHARACTER_ERR
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
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_document_create_attribute_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_document_create_attribute_start(javacall_handle handle,
                                             javacall_int32 invocation_id,
                                             void **context,
                                             javacall_const_utf16_string name,
                                             /* OUT */ javacall_handle* ret_value,
                                             /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR creates an <code>Attr</code> of the given name. Note that the 
 * <code>Attr</code> instance can then be set on an <code>Element</code> 
 * using the <code>setAttributeNode</code> method. 
 * <br>To create an attribute with a qualified name and namespace URI, use 
 * the <code>createAttributeNS</code> method.
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   a new <code>Attr</code> object with the <code>nodeName</code> 
 *   attribute set to <code>name</code>, and <code>localName</code>, 
 *   <code>prefix</code>, and <code>namespaceURI</code> set to 
 *   <code>NULL</code>. The value of the attribute is the empty string.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_INVALID_CHARACTER_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_INVALID_CHARACTER_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_document_create_attribute_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_document_create_attribute_finish(void *context,
                                              /* OUT */ javacall_handle* ret_value,
                                              /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR creates an <code>EntityReference</code> object. In addition, if the 
 * referenced entity is known, the child list of the 
 * <code>EntityReference</code> node is made the same as that of the 
 * corresponding <code>Entity</code> node. 
 * <p ><b>Note:</b> If any descendant of the <code>Entity</code> node has 
 * an unbound namespace prefix, the corresponding descendant of the 
 * created <code>EntityReference</code> node is also unbound; (its 
 * <code>namespaceURI</code> is <code>NULL</code>). The DOM Level 2 and 
 * 3 do not support any mechanism to resolve namespace prefixes in this 
 * case.
 * 
 * @param handle Pointer to the object representing this document.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param name The name of the entity to reference. Unlike 
 *   <code>Document.createElementNS</code> or 
 *   <code>Document.createAttributeNS</code>, no namespace well-formed 
 *   checking is done on the entity name.
 * @param ret_value Pointer to the object representing 
 *   the new <code>EntityReference</code> object.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_INVALID_CHARACTER_ERR
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
 *                 JAVACALL_DOM_INVALID_CHARACTER_ERR
 *                 JAVACALL_DOM_NOT_SUPPORTED_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_document_create_entity_reference_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_document_create_entity_reference_start(javacall_handle handle,
                                                    javacall_int32 invocation_id,
                                                    void **context,
                                                    javacall_const_utf16_string name,
                                                    /* OUT */ javacall_handle* ret_value,
                                                    /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR creates an <code>EntityReference</code> object. In addition, if the 
 * referenced entity is known, the child list of the 
 * <code>EntityReference</code> node is made the same as that of the 
 * corresponding <code>Entity</code> node. 
 * <p ><b>Note:</b> If any descendant of the <code>Entity</code> node has 
 * an unbound namespace prefix, the corresponding descendant of the 
 * created <code>EntityReference</code> node is also unbound; (its 
 * <code>namespaceURI</code> is <code>NULL</code>). The DOM Level 2 and 
 * 3 do not support any mechanism to resolve namespace prefixes in this 
 * case.
 * 
 * @param context The context saved during asynchronous operation.
 *   <code>Document.createElementNS</code> or 
 *   <code>Document.createAttributeNS</code>, no namespace well-formed 
 *   checking is done on the entity name.
 * @param ret_value Pointer to the object representing 
 *   the new <code>EntityReference</code> object.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_INVALID_CHARACTER_ERR
 *                            JAVACALL_DOM_NOT_SUPPORTED_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_INVALID_CHARACTER_ERR
 *                 JAVACALL_DOM_NOT_SUPPORTED_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_document_create_entity_reference_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_document_create_entity_reference_finish(void *context,
                                                     /* OUT */ javacall_handle* ret_value,
                                                     /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns a <code>NodeList</code> of all the <code>Elements</code> with a 
 * given tag name in the order in which they are encountered in a 
 * preorder traversal of the <code>Document</code> tree. 
 * 
 * @param handle Pointer to the object representing this document.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param tagname The name of the tag to match on. The special value "*" 
 *   matches all tags. For XML, the <code>tagname</code> parameter is 
 *   case-sensitive, otherwise it depends on the case-sensitivity of the 
 *   markup language in use.
 * @param ret_value Pointer to the object representing 
 *   a new <code>NodeList</code> object containing all the matched 
 *   <code>Elements</code>.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_document_get_elements_by_tag_name_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_document_get_elements_by_tag_name_start(javacall_handle handle,
                                                     javacall_int32 invocation_id,
                                                     void **context,
                                                     javacall_const_utf16_string tagname,
                                                     /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns a <code>NodeList</code> of all the <code>Elements</code> with a 
 * given tag name in the order in which they are encountered in a 
 * preorder traversal of the <code>Document</code> tree. 
 * 
 * @param context The context saved during asynchronous operation.
 *   matches all tags. For XML, the <code>tagname</code> parameter is 
 *   case-sensitive, otherwise it depends on the case-sensitivity of the 
 *   markup language in use.
 * @param ret_value Pointer to the object representing 
 *   a new <code>NodeList</code> object containing all the matched 
 *   <code>Elements</code>.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_document_get_elements_by_tag_name_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_document_get_elements_by_tag_name_finish(void *context,
                                                      /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR imports a node from another document to this document, without altering 
 * or removing the source node from the original document; this method 
 * creates a new copy of the source node. The returned node has no 
 * parent; (<code>parentNode</code> is <code>NULL</code>).
 * <br>For all nodes, importing a node creates a node object owned by the 
 * importing document, with attribute values identical to the source 
 * node's <code>nodeName</code> and <code>nodeType</code>, plus the 
 * attributes related to namespaces (<code>prefix</code>, 
 * <code>localName</code>, and <code>namespaceURI</code>). As in the 
 * <code>cloneNode</code> operation, the source node is not altered.
 * <br>Additional information is copied as appropriate to the 
 * <code>nodeType</code>, attempting to mirror the behavior expected if 
 * a fragment of XML or HTML source was copied from one document to 
 * another, recognizing that the two documents may have different DTDs 
 * in the XML case. The following list describes the specifics for each 
 * type of node. 
 * <dl>
 * <dt>ATTRIBUTE_NODE</dt>
 * <dd>The <code>ownerElement</code> attribute 
 * is set to <code>NULL</code> and the <code>specified</code> flag is 
 * set to <code>true</code> on the generated <code>Attr</code>. The 
 * descendants of the source <code>Attr</code> are recursively imported 
 * and the resulting nodes reassembled to form the corresponding subtree.
 * Note that the <code>deep</code> parameter has no effect on 
 * <code>Attr</code> nodes; they always carry their children with them 
 * when imported.</dd>
 * <dt>DOCUMENT_FRAGMENT_NODE</dt>
 * <dd>If the <code>deep</code> option 
 * was set to <code>true</code>, the descendants of the source  
 * <code>DocumentFragment</code> are recursively imported and the 
 * resulting nodes reassembled under the imported 
 * <code>DocumentFragment</code> to form the corresponding subtree. 
 * Otherwise, this simply generates an empty 
 * <code>DocumentFragment</code>.</dd>
 * <dt>DOCUMENT_NODE</dt>
 * <dd><code>Document</code> 
 * nodes cannot be imported.</dd>
 * <dt>DOCUMENT_TYPE_NODE</dt>
 * <dd><code>DocumentType</code> 
 * nodes cannot be imported.</dd>
 * <dt>ELEMENT_NODE</dt>
 * <dd><em>Specified</em> attribute nodes of the source element are imported, and the generated 
 * <code>Attr</code> nodes are attached to the generated 
 * <code>Element</code>. Default attributes are <em>not</em> copied, though if the document being imported into defines default 
 * attributes for this element name, those are assigned. If the 
 * <code>importNode</code> <code>deep</code> parameter was set to 
 * <code>true</code>, the descendants of the source element are 
 * recursively imported and the resulting nodes reassembled to form the 
 * corresponding subtree.</dd>
 * <dt>ENTITY_NODE</dt>
 * <dd><code>Entity</code> nodes can be 
 * imported, however in the current release of the DOM the 
 * <code>DocumentType</code> is readonly. Ability to add these imported 
 * nodes to a <code>DocumentType</code> will be considered for addition 
 * to a future release of the DOM.On import, the <code>publicId</code>, 
 * <code>systemId</code>, and <code>notationName</code> attributes are 
 * copied. If a <code>deep</code> import is requested, the descendants 
 * of the the source <code>Entity</code> are recursively imported and 
 * the resulting nodes reassembled to form the corresponding subtree.</dd>
 * <dt>
 * ENTITY_REFERENCE_NODE</dt>
 * <dd>Only the <code>EntityReference</code> itself is 
 * copied, even if a <code>deep</code> import is requested, since the 
 * source and destination documents might have defined the entity 
 * differently. If the document being imported into provides a 
 * definition for this entity name, its value is assigned.</dd>
 * <dt>NOTATION_NODE</dt>
 * <dd>
 * <code>Notation</code> nodes can be imported, however in the current 
 * release of the DOM the <code>DocumentType</code> is readonly. Ability 
 * to add these imported nodes to a <code>DocumentType</code> will be 
 * considered for addition to a future release of the DOM.On import, the 
 * <code>publicId</code> and <code>systemId</code> attributes are copied.
 * Note that the <code>deep</code> parameter has no effect on this type
 * of nodes since they cannot have any children.</dd>
 * <dt>
 * PROCESSING_INSTRUCTION_NODE</dt>
 * <dd>The imported node copies its 
 * <code>target</code> and <code>data</code> values from those of the 
 * source node. Note that the <code>deep</code> parameter has no effect 
 * on this type of nodes since they cannot have any children.</dd>
 * <dt>TEXT_NODE, CDATA_SECTION_NODE, COMMENT_NODE</dt>
 * <dd>These three 
 * types of nodes inheriting from <code>CharacterData</code> copy their 
 * <code>data</code> and <code>length</code> attributes from those of 
 * the source node.Note that the <code>deep</code> parameter has no effect 
 * on this type of nodes since they cannot have any children.</dd>
 * </dl> 
 * 
 * @param handle Pointer to the object representing this document.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param imported_node Pointer to the object of
 *   the node to import.
 * @param deep If <code>true</code>, recursively import the subtree under 
 *   the specified node; if <code>false</code>, import only the node 
 *   itself, as explained above. This has no effect on nodes that cannot 
 *   have any children, and on <code>Attr</code>, and 
 *   <code>EntityReference</code> nodes.
 * @param ret_value Pointer to the object representing 
 *   the imported node that belongs to this <code>Document</code>.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_NOT_SUPPORTED_ERR
 *                            JAVACALL_DOM_INVALID_CHARACTER_ERR
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
 *                 JAVACALL_DOM_INVALID_CHARACTER_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_document_import_node_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_document_import_node_start(javacall_handle handle,
                                        javacall_int32 invocation_id,
                                        void **context,
                                        javacall_handle imported_node,
                                        javacall_bool deep,
                                        /* OUT */ javacall_handle* ret_value,
                                        /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR imports a node from another document to this document, without altering 
 * or removing the source node from the original document; this method 
 * creates a new copy of the source node. The returned node has no 
 * parent; (<code>parentNode</code> is <code>NULL</code>).
 * <br>For all nodes, importing a node creates a node object owned by the 
 * importing document, with attribute values identical to the source 
 * node's <code>nodeName</code> and <code>nodeType</code>, plus the 
 * attributes related to namespaces (<code>prefix</code>, 
 * <code>localName</code>, and <code>namespaceURI</code>). As in the 
 * <code>cloneNode</code> operation, the source node is not altered.
 * <br>Additional information is copied as appropriate to the 
 * <code>nodeType</code>, attempting to mirror the behavior expected if 
 * a fragment of XML or HTML source was copied from one document to 
 * another, recognizing that the two documents may have different DTDs 
 * in the XML case. The following list describes the specifics for each 
 * type of node. 
 * <dl>
 * <dt>ATTRIBUTE_NODE</dt>
 * <dd>The <code>ownerElement</code> attribute 
 * is set to <code>NULL</code> and the <code>specified</code> flag is 
 * set to <code>true</code> on the generated <code>Attr</code>. The 
 * descendants of the source <code>Attr</code> are recursively imported 
 * and the resulting nodes reassembled to form the corresponding subtree.
 * Note that the <code>deep</code> parameter has no effect on 
 * <code>Attr</code> nodes; they always carry their children with them 
 * when imported.</dd>
 * <dt>DOCUMENT_FRAGMENT_NODE</dt>
 * <dd>If the <code>deep</code> option 
 * was set to <code>true</code>, the descendants of the source  
 * <code>DocumentFragment</code> are recursively imported and the 
 * resulting nodes reassembled under the imported 
 * <code>DocumentFragment</code> to form the corresponding subtree. 
 * Otherwise, this simply generates an empty 
 * <code>DocumentFragment</code>.</dd>
 * <dt>DOCUMENT_NODE</dt>
 * <dd><code>Document</code> 
 * nodes cannot be imported.</dd>
 * <dt>DOCUMENT_TYPE_NODE</dt>
 * <dd><code>DocumentType</code> 
 * nodes cannot be imported.</dd>
 * <dt>ELEMENT_NODE</dt>
 * <dd><em>Specified</em> attribute nodes of the source element are imported, and the generated 
 * <code>Attr</code> nodes are attached to the generated 
 * <code>Element</code>. Default attributes are <em>not</em> copied, though if the document being imported into defines default 
 * attributes for this element name, those are assigned. If the 
 * <code>importNode</code> <code>deep</code> parameter was set to 
 * <code>true</code>, the descendants of the source element are 
 * recursively imported and the resulting nodes reassembled to form the 
 * corresponding subtree.</dd>
 * <dt>ENTITY_NODE</dt>
 * <dd><code>Entity</code> nodes can be 
 * imported, however in the current release of the DOM the 
 * <code>DocumentType</code> is readonly. Ability to add these imported 
 * nodes to a <code>DocumentType</code> will be considered for addition 
 * to a future release of the DOM.On import, the <code>publicId</code>, 
 * <code>systemId</code>, and <code>notationName</code> attributes are 
 * copied. If a <code>deep</code> import is requested, the descendants 
 * of the the source <code>Entity</code> are recursively imported and 
 * the resulting nodes reassembled to form the corresponding subtree.</dd>
 * <dt>
 * ENTITY_REFERENCE_NODE</dt>
 * <dd>Only the <code>EntityReference</code> itself is 
 * copied, even if a <code>deep</code> import is requested, since the 
 * source and destination documents might have defined the entity 
 * differently. If the document being imported into provides a 
 * definition for this entity name, its value is assigned.</dd>
 * <dt>NOTATION_NODE</dt>
 * <dd>
 * <code>Notation</code> nodes can be imported, however in the current 
 * release of the DOM the <code>DocumentType</code> is readonly. Ability 
 * to add these imported nodes to a <code>DocumentType</code> will be 
 * considered for addition to a future release of the DOM.On import, the 
 * <code>publicId</code> and <code>systemId</code> attributes are copied.
 * Note that the <code>deep</code> parameter has no effect on this type
 * of nodes since they cannot have any children.</dd>
 * <dt>
 * PROCESSING_INSTRUCTION_NODE</dt>
 * <dd>The imported node copies its 
 * <code>target</code> and <code>data</code> values from those of the 
 * source node. Note that the <code>deep</code> parameter has no effect 
 * on this type of nodes since they cannot have any children.</dd>
 * <dt>TEXT_NODE, CDATA_SECTION_NODE, COMMENT_NODE</dt>
 * <dd>These three 
 * types of nodes inheriting from <code>CharacterData</code> copy their 
 * <code>data</code> and <code>length</code> attributes from those of 
 * the source node.Note that the <code>deep</code> parameter has no effect 
 * on this type of nodes since they cannot have any children.</dd>
 * </dl> 
 * 
 * @param context The context saved during asynchronous operation.
 *   the specified node; if <code>false</code>, import only the node 
 *   itself, as explained above. This has no effect on nodes that cannot 
 *   have any children, and on <code>Attr</code>, and 
 *   <code>EntityReference</code> nodes.
 * @param ret_value Pointer to the object representing 
 *   the imported node that belongs to this <code>Document</code>.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_NOT_SUPPORTED_ERR
 *                            JAVACALL_DOM_INVALID_CHARACTER_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_NOT_SUPPORTED_ERR
 *                 JAVACALL_DOM_INVALID_CHARACTER_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_document_import_node_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_document_import_node_finish(void *context,
                                         /* OUT */ javacall_handle* ret_value,
                                         /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR creates an element of the given qualified name and namespace URI. 
 * <br>Per [<a href='http://www.w3.org/TR/1999/REC-xml-names-19990114/'>XML Namespaces</a>]
 * , applications must use the value <code>NULL</code> as the 
 * namespace_uri parameter for methods if they wish to have no namespace.
 * 
 * @param handle Pointer to the object representing this document.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param namespace_uri The namespace URI of the element to create.
 * @param qualified_name The qualified name of the element type to 
 *   instantiate.
 * @param ret_value Pointer to the object representing 
 *   a new <code>Element</code> object with the following 
 *   attributes:
 * <table border='1' cellpadding='3'>
 * <tr>
 * <th>Attribute</th>
 * <th>Value</th>
 * </tr>
 * <tr>
 * <td valign='top' rowspan='1' colspan='1'><code>Node.nodeName</code></td>
 * <td valign='top' rowspan='1' colspan='1'>
 *   <code>qualified_name</code></td>
 * </tr>
 * <tr>
 * <td valign='top' rowspan='1' colspan='1'><code>Node.namespace_uri</code></td>
 * <td valign='top' rowspan='1' colspan='1'>
 *   <code>namespace_uri</code></td>
 * </tr>
 * <tr>
 * <td valign='top' rowspan='1' colspan='1'><code>Node.prefix</code></td>
 * <td valign='top' rowspan='1' colspan='1'>prefix, extracted 
 *   from <code>qualified_name</code>, or <code>NULL</code> if there is 
 *   no prefix</td>
 * </tr>
 * <tr>
 * <td valign='top' rowspan='1' colspan='1'><code>Node.localName</code></td>
 * <td valign='top' rowspan='1' colspan='1'>local name, extracted from 
 *   <code>qualified_name</code></td>
 * </tr>
 * <tr>
 * <td valign='top' rowspan='1' colspan='1'><code>Element.tagName</code></td>
 * <td valign='top' rowspan='1' colspan='1'>
 *   <code>qualified_name</code></td>
 * </tr>
 * </table>
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_INVALID_CHARACTER_ERR
 *                            JAVACALL_DOM_NAMESPACE_ERR
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
 *                 JAVACALL_DOM_INVALID_CHARACTER_ERR
 *                 JAVACALL_DOM_NAMESPACE_ERR
 *                 JAVACALL_DOM_NOT_SUPPORTED_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_document_create_element_ns_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_document_create_element_ns_start(javacall_handle handle,
                                              javacall_int32 invocation_id,
                                              void **context,
                                              javacall_const_utf16_string namespace_uri,
                                              javacall_const_utf16_string qualified_name,
                                              /* OUT */ javacall_handle* ret_value,
                                              /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR creates an element of the given qualified name and namespace URI. 
 * <br>Per [<a href='http://www.w3.org/TR/1999/REC-xml-names-19990114/'>XML Namespaces</a>]
 * , applications must use the value <code>NULL</code> as the 
 * namespaceURI parameter for methods if they wish to have no namespace.
 * 
 * @param context The context saved during asynchronous operation.
 *   instantiate.
 * @param ret_value Pointer to the object representing 
 *   a new <code>Element</code> object with the following 
 *   attributes:
 * <table border='1' cellpadding='3'>
 * <tr>
 * <th>Attribute</th>
 * <th>Value</th>
 * </tr>
 * <tr>
 * <td valign='top' rowspan='1' colspan='1'><code>Node.nodeName</code></td>
 * <td valign='top' rowspan='1' colspan='1'>
 *   <code>qualifiedName</code></td>
 * </tr>
 * <tr>
 * <td valign='top' rowspan='1' colspan='1'><code>Node.namespaceURI</code></td>
 * <td valign='top' rowspan='1' colspan='1'>
 *   <code>namespaceURI</code></td>
 * </tr>
 * <tr>
 * <td valign='top' rowspan='1' colspan='1'><code>Node.prefix</code></td>
 * <td valign='top' rowspan='1' colspan='1'>prefix, extracted 
 *   from <code>qualifiedName</code>, or <code>NULL</code> if there is 
 *   no prefix</td>
 * </tr>
 * <tr>
 * <td valign='top' rowspan='1' colspan='1'><code>Node.localName</code></td>
 * <td valign='top' rowspan='1' colspan='1'>local name, extracted from 
 *   <code>qualifiedName</code></td>
 * </tr>
 * <tr>
 * <td valign='top' rowspan='1' colspan='1'><code>Element.tagName</code></td>
 * <td valign='top' rowspan='1' colspan='1'>
 *   <code>qualifiedName</code></td>
 * </tr>
 * </table>
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_INVALID_CHARACTER_ERR
 *                            JAVACALL_DOM_NAMESPACE_ERR
 *                            JAVACALL_DOM_NOT_SUPPORTED_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_INVALID_CHARACTER_ERR
 *                 JAVACALL_DOM_NAMESPACE_ERR
 *                 JAVACALL_DOM_NOT_SUPPORTED_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_document_create_element_ns_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_document_create_element_ns_finish(void *context,
                                               /* OUT */ javacall_handle* ret_value,
                                               /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR creates an attribute of the given qualified name and namespace URI. 
 * <br>Per [<a href='http://www.w3.org/TR/1999/REC-xml-names-19990114/'>XML Namespaces</a>]
 * , applications must use the value <code>NULL</code> as the 
 * <code>namespace_uri</code> parameter for methods if they wish to have 
 * no namespace.
 * 
 * @param handle Pointer to the object representing this document.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param namespace_uri The namespace URI of the attribute to create.
 * @param qualified_name The qualified name of the attribute to instantiate.
 * @param ret_value Pointer to the object representing 
 *   a new <code>Attr</code> object with the following attributes:
 * <table border='1' cellpadding='3'>
 * <tr>
 * <th>
 *   Attribute</th>
 * <th>Value</th>
 * </tr>
 * <tr>
 * <td valign='top' rowspan='1' colspan='1'><code>Node.nodeName</code></td>
 * <td valign='top' rowspan='1' colspan='1'>qualified_name</td>
 * </tr>
 * <tr>
 * <td valign='top' rowspan='1' colspan='1'>
 *   <code>Node.namespace_uri</code></td>
 * <td valign='top' rowspan='1' colspan='1'><code>namespace_uri</code></td>
 * </tr>
 * <tr>
 * <td valign='top' rowspan='1' colspan='1'>
 *   <code>Node.prefix</code></td>
 * <td valign='top' rowspan='1' colspan='1'>prefix, extracted from 
 *   <code>qualified_name</code>, or <code>NULL</code> if there is no 
 *   prefix</td>
 * </tr>
 * <tr>
 * <td valign='top' rowspan='1' colspan='1'><code>Node.localName</code></td>
 * <td valign='top' rowspan='1' colspan='1'>local name, extracted from 
 *   <code>qualified_name</code></td>
 * </tr>
 * <tr>
 * <td valign='top' rowspan='1' colspan='1'><code>Attr.name</code></td>
 * <td valign='top' rowspan='1' colspan='1'>
 *   <code>qualified_name</code></td>
 * </tr>
 * <tr>
 * <td valign='top' rowspan='1' colspan='1'><code>Node.nodeValue</code></td>
 * <td valign='top' rowspan='1' colspan='1'>the empty 
 *   string</td>
 * </tr>
 * </table>
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_INVALID_CHARACTER_ERR
 *                            JAVACALL_DOM_NAMESPACE_ERR
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
 *                 JAVACALL_DOM_INVALID_CHARACTER_ERR
 *                 JAVACALL_DOM_NAMESPACE_ERR
 *                 JAVACALL_DOM_NOT_SUPPORTED_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_document_create_attribute_ns_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_document_create_attribute_ns_start(javacall_handle handle,
                                                javacall_int32 invocation_id,
                                                void **context,
                                                javacall_const_utf16_string namespace_uri,
                                                javacall_const_utf16_string qualified_name,
                                                /* OUT */ javacall_handle* ret_value,
                                                /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR creates an attribute of the given qualified name and namespace URI. 
 * <br>Per [<a href='http://www.w3.org/TR/1999/REC-xml-names-19990114/'>XML Namespaces</a>]
 * , applications must use the value <code>NULL</code> as the 
 * <code>namespaceURI</code> parameter for methods if they wish to have 
 * no namespace.
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   a new <code>Attr</code> object with the following attributes:
 * <table border='1' cellpadding='3'>
 * <tr>
 * <th>
 *   Attribute</th>
 * <th>Value</th>
 * </tr>
 * <tr>
 * <td valign='top' rowspan='1' colspan='1'><code>Node.nodeName</code></td>
 * <td valign='top' rowspan='1' colspan='1'>qualifiedName</td>
 * </tr>
 * <tr>
 * <td valign='top' rowspan='1' colspan='1'>
 *   <code>Node.namespaceURI</code></td>
 * <td valign='top' rowspan='1' colspan='1'><code>namespaceURI</code></td>
 * </tr>
 * <tr>
 * <td valign='top' rowspan='1' colspan='1'>
 *   <code>Node.prefix</code></td>
 * <td valign='top' rowspan='1' colspan='1'>prefix, extracted from 
 *   <code>qualifiedName</code>, or <code>NULL</code> if there is no 
 *   prefix</td>
 * </tr>
 * <tr>
 * <td valign='top' rowspan='1' colspan='1'><code>Node.localName</code></td>
 * <td valign='top' rowspan='1' colspan='1'>local name, extracted from 
 *   <code>qualifiedName</code></td>
 * </tr>
 * <tr>
 * <td valign='top' rowspan='1' colspan='1'><code>Attr.name</code></td>
 * <td valign='top' rowspan='1' colspan='1'>
 *   <code>qualifiedName</code></td>
 * </tr>
 * <tr>
 * <td valign='top' rowspan='1' colspan='1'><code>Node.nodeValue</code></td>
 * <td valign='top' rowspan='1' colspan='1'>the empty 
 *   string</td>
 * </tr>
 * </table>
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_INVALID_CHARACTER_ERR
 *                            JAVACALL_DOM_NAMESPACE_ERR
 *                            JAVACALL_DOM_NOT_SUPPORTED_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_INVALID_CHARACTER_ERR
 *                 JAVACALL_DOM_NAMESPACE_ERR
 *                 JAVACALL_DOM_NOT_SUPPORTED_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_document_create_attribute_ns_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_document_create_attribute_ns_finish(void *context,
                                                 /* OUT */ javacall_handle* ret_value,
                                                 /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns a <code>NodeList</code> of all the <code>Elements</code> with a 
 * given local name and namespace URI in document order.
 * 
 * @param handle Pointer to the object representing this document.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param namespace_uri The namespace URI of the elements to match on. The 
 *   special value <code>"*"</code> matches all namespaces.
 * @param local_name The local name of the elements to match on. The 
 *   special value <code>"*"</code> matches all local names.
 * @param ret_value Pointer to the object representing 
 *   a new <code>NodeList</code> object containing all the matched 
 *   <code>Elements</code>.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_document_get_elements_by_tag_name_ns_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_document_get_elements_by_tag_name_ns_start(javacall_handle handle,
                                                        javacall_int32 invocation_id,
                                                        void **context,
                                                        javacall_const_utf16_string namespace_uri,
                                                        javacall_const_utf16_string local_name,
                                                        /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns a <code>NodeList</code> of all the <code>Elements</code> with a 
 * given local name and namespace URI in document order.
 * 
 * @param context The context saved during asynchronous operation.
 *   special value <code>"*"</code> matches all local names.
 * @param ret_value Pointer to the object representing 
 *   a new <code>NodeList</code> object containing all the matched 
 *   <code>Elements</code>.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_document_get_elements_by_tag_name_ns_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_document_get_elements_by_tag_name_ns_finish(void *context,
                                                         /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the <code>Element</code>that has an ID attribute with the 
 *given value. If no such element exists, this returns <code>NULL</code>.
 * If more than one element has an ID attribute with that value, what 
 * is returned is undefined.
 * <br>The DOM implementation is expected to use the attribute 
 * <code>Attr.isId</code> to determine if an attribute is of type ID. 
 * <p ><b>Note:</b> Attributes with the name "ID" or "id" are not of type 
 * ID unless so defined. 
 * Implementations that do not know whether attributes are of type ID or 
 * not are expected to return <code>NULL</code>.
 * 
 * @param handle Pointer to the object representing this document.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param element_id The unique <code>id</code> value for an element.
 * @param ret_value Pointer to the object representing 
 *   the matching element or <code>NULL</code> if there is none.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_document_get_element_by_id_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_document_get_element_by_id_start(javacall_handle handle,
                                              javacall_int32 invocation_id,
                                              void **context,
                                              javacall_const_utf16_string element_id,
                                              /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the <code>Element</code>that has an ID attribute with the 
 *given value. If no such element exists, this returns <code>NULL</code>.
 * If more than one element has an ID attribute with that value, what 
 * is returned is undefined.
 * <br>The DOM implementation is expected to use the attribute 
 * <code>Attr.isId</code> to determine if an attribute is of type ID. 
 * <p ><b>Note:</b> Attributes with the name "ID" or "id" are not of type 
 * ID unless so defined. 
 * Implementations that do not know whether attributes are of type ID or 
 * not are expected to return <code>NULL</code>.
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   the matching element or <code>NULL</code> if there is none.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_document_get_element_by_id_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_document_get_element_by_id_finish(void *context,
                                               /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR  Attempts to adopt a node from another document to this document. If 
 * supported, it changes the <code>ownerDocument</code> of the source 
 * node, its children, as well as the attached attribute nodes if there 
 * are any. If the source node has a parent it is first removed from the 
 * child list of its parent. This effectively allows moving a subtree 
 * from one document to another (unlike <code>importNode()</code> which 
 * create a copy of the source node instead of moving it). When it 
 * fails, applications should use <code>Document.importNode()</code> 
 * instead. Note that if the adopted node is already part of this 
 * document (i.e. the source and target document are the same), this 
 * method still has the effect of removing the source node from the 
 * child list of its parent, if any. The following list describes the 
 * specifics for each type of node. 
 * <dl>
 * <dt>ATTRIBUTE_NODE</dt>
 * <dd>The 
 * <code>ownerElement</code> attribute is set to <code>NULL</code>> and 
 * the <code>specified</code> flag is set to <code>true</code> on the 
 * adopted <code>Attr</code>. The descendants of the source 
 * <code>Attr</code> are recursively adopted.</dd>
 * <dt>DOCUMENT_FRAGMENT_NODE</dt>
 * <dd>The 
 * descendants of the source node are recursively adopted.</dd>
 * <dt>DOCUMENT_NODE</dt>
 * <dd>
 * <code>Document</code> nodes cannot be adopted.</dd>
 * <dt>DOCUMENT_TYPE_NODE</dt>
 * <dd>
 * <code>DocumentType</code> nodes cannot be adopted.</dd>
 * <dt>ELEMENT_NODE</dt>
 * <dd><em>Specified</em> attribute nodes of the source element are adopted. Default attributes 
 * are discarded, though if the document being adopted into defines 
 * default attributes for this element name, those are assigned. The 
 * descendants of the source element are recursively adopted.</dd>
 * <dt>ENTITY_NODE</dt>
 * <dd>
 * <code>Entity</code> nodes cannot be adopted.</dd>
 * <dt>ENTITY_REFERENCE_NODE</dt>
 * <dd>Only 
 * the <code>EntityReference</code> node itself is adopted, the 
 * descendants are discarded, since the source and destination documents 
 * might have defined the entity differently. If the document being 
 * imported into provides a definition for this entity name, its value 
 * is assigned.</dd>
 * <dt>NOTATION_NODE</dt>
 * <dd><code>Notation</code> nodes cannot be 
 * adopted.</dd>
 * <dt>PROCESSING_INSTRUCTION_NODE, TEXT_NODE, CDATA_SECTION_NODE, 
 * COMMENT_NODE</dt>
 * <dd>These nodes can all be adopted. No specifics.</dd>
 * </dl> 
 * <p ><b>Note:</b>  Since it does not create new nodes unlike the 
 * <code>Document.importNode()</code> method, this method does not raise 
 * an <code>INVALID_CHARACTER_ERR</code> exception.
 * 
 * @param handle Pointer to the object representing this document.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param source Pointer to the object of
 *   the node to move into this document.
 * @param ret_value Pointer to the object representing 
 *   the adopted node, or <code>NULL</code> if this operation 
 *   fails, such as when the source node comes from a different 
 *   implementation.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_NOT_SUPPORTED_ERR
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
 *                 JAVACALL_DOM_NOT_SUPPORTED_ERR
 *                 JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_document_adopt_node_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_document_adopt_node_start(javacall_handle handle,
                                       javacall_int32 invocation_id,
                                       void **context,
                                       javacall_handle source,
                                       /* OUT */ javacall_handle* ret_value,
                                       /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR  Attempts to adopt a node from another document to this document. If 
 * supported, it changes the <code>ownerDocument</code> of the source 
 * node, its children, as well as the attached attribute nodes if there 
 * are any. If the source node has a parent it is first removed from the 
 * child list of its parent. This effectively allows moving a subtree 
 * from one document to another (unlike <code>importNode()</code> which 
 * create a copy of the source node instead of moving it). When it 
 * fails, applications should use <code>Document.importNode()</code> 
 * instead. Note that if the adopted node is already part of this 
 * document (i.e. the source and target document are the same), this 
 * method still has the effect of removing the source node from the 
 * child list of its parent, if any. The following list describes the 
 * specifics for each type of node. 
 * <dl>
 * <dt>ATTRIBUTE_NODE</dt>
 * <dd>The 
 * <code>ownerElement</code> attribute is set to <code>NULL</code>> and 
 * the <code>specified</code> flag is set to <code>true</code> on the 
 * adopted <code>Attr</code>. The descendants of the source 
 * <code>Attr</code> are recursively adopted.</dd>
 * <dt>DOCUMENT_FRAGMENT_NODE</dt>
 * <dd>The 
 * descendants of the source node are recursively adopted.</dd>
 * <dt>DOCUMENT_NODE</dt>
 * <dd>
 * <code>Document</code> nodes cannot be adopted.</dd>
 * <dt>DOCUMENT_TYPE_NODE</dt>
 * <dd>
 * <code>DocumentType</code> nodes cannot be adopted.</dd>
 * <dt>ELEMENT_NODE</dt>
 * <dd><em>Specified</em> attribute nodes of the source element are adopted. Default attributes 
 * are discarded, though if the document being adopted into defines 
 * default attributes for this element name, those are assigned. The 
 * descendants of the source element are recursively adopted.</dd>
 * <dt>ENTITY_NODE</dt>
 * <dd>
 * <code>Entity</code> nodes cannot be adopted.</dd>
 * <dt>ENTITY_REFERENCE_NODE</dt>
 * <dd>Only 
 * the <code>EntityReference</code> node itself is adopted, the 
 * descendants are discarded, since the source and destination documents 
 * might have defined the entity differently. If the document being 
 * imported into provides a definition for this entity name, its value 
 * is assigned.</dd>
 * <dt>NOTATION_NODE</dt>
 * <dd><code>Notation</code> nodes cannot be 
 * adopted.</dd>
 * <dt>PROCESSING_INSTRUCTION_NODE, TEXT_NODE, CDATA_SECTION_NODE, 
 * COMMENT_NODE</dt>
 * <dd>These nodes can all be adopted. No specifics.</dd>
 * </dl> 
 * <p ><b>Note:</b>  Since it does not create new nodes unlike the 
 * <code>Document.importNode()</code> method, this method does not raise 
 * an <code>INVALID_CHARACTER_ERR</code> exception.
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   the adopted node, or <code>NULL</code> if this operation 
 *   fails, such as when the source node comes from a different 
 *   implementation.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_NOT_SUPPORTED_ERR
 *                            JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_NOT_SUPPORTED_ERR
 *                 JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_document_adopt_node_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_document_adopt_node_finish(void *context,
                                        /* OUT */ javacall_handle* ret_value,
                                        /* OUT */ javacall_dom_exceptions* exception_code);

/*
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR checks if this Document is HTMLDocument
 * 
 * 
 * @param handle Pointer to the object representing this document.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param ret_value <code>true</code> if the Document is HTMLDocument,
 * <code>false</code> otherwise. 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_document_is_html_document_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_document_is_html_document_start(javacall_handle handle,
                                             javacall_int32 invocation_id,
                                             void **context,
                                             /* OUT */ javacall_bool* ret_value);

/*
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR checks if this Document is HTMLDocument
 * 
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value <code>true</code> if the Document is HTMLDocument,
 * <code>false</code> otherwise. 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_document_is_html_document_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_document_is_html_document_finish(void *context,
                                              /* OUT */ javacall_bool* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR create an Event.
 *
 * 
 * @param handle Pointer to the object representing this document.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param event_type  The <code>event_type</code> parameter specifies the 
 *   name of the DOM Events interface to be supported by the created 
 *   event object, e.g. <code>"Event"</code>, <code>"MouseEvent"</code>, 
 *   <code>"MutationEvent"</code> and so on. If the <code>Event</code> 
 *   is to be dispatched via the <code>EventTarget.dispatchEvent()</code>
 *   method the appropriate event init method must be called after 
 *   creation in order to initialize the <code>Event</code>'s values.  
 *   As an example, a user wishing to synthesize some kind of 
 *   <code>UIEvent</code> would invoke 
 *   <code>DocumentEvent.createEvent("UIEvent")</code>. The 
 *   <code>UIEvent.initUIEventNS()</code> method could then be called on 
 *   the newly created <code>UIEvent</code> object to set the specific 
 *   type of user interface event to be dispatched, DOMActivate for 
 *   example, and set its context information, e.g. 
 *   <code>UIEvent.detail</code> in this example. 
 * <p><b>Note:</b>For backward compatibility reason, "UIEvents", 
 *   "MouseEvents", "MutationEvents", and "HTMLEvents" feature names are 
 *   valid values for the parameter <code>event_type</code> and represent 
 *   respectively the interfaces "UIEvent", "MouseEvent", 
 *   "MutationEvent", and "Event". 
 * <p><b>Note:</b>  JSR 280 follows the DOM 3 rule for <code>Event.type</code>
 * and considers it to be case-sensitive. This differs from DOM 2, which
 * considers it to be case-insensitive.
 * @param ret_value Pointer to the object representing 
 *    The newly created event object. 
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
 *             javacall_dom_document_create_event_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_document_create_event_start(javacall_handle handle,
                                         javacall_int32 invocation_id,
                                         void **context,
                                         javacall_const_utf16_string event_type,
                                         /* OUT */ javacall_handle* ret_value,
                                         /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR create an Event.
 *
 * 
 * @param context The context saved during asynchronous operation.
 *   name of the DOM Events interface to be supported by the created 
 *   event object, e.g. <code>"Event"</code>, <code>"MouseEvent"</code>, 
 *   <code>"MutationEvent"</code> and so on. If the <code>Event</code> 
 *   is to be dispatched via the <code>EventTarget.dispatchEvent()</code>
 *   method the appropriate event init method must be called after 
 *   creation in order to initialize the <code>Event</code>'s values.  
 *   As an example, a user wishing to synthesize some kind of 
 *   <code>UIEvent</code> would invoke 
 *   <code>DocumentEvent.createEvent("UIEvent")</code>. The 
 *   <code>UIEvent.initUIEventNS()</code> method could then be called on 
 *   the newly created <code>UIEvent</code> object to set the specific 
 *   type of user interface event to be dispatched, DOMActivate for 
 *   example, and set its context information, e.g. 
 *   <code>UIEvent.detail</code> in this example. 
 * <p><b>Note:</b>For backward compatibility reason, "UIEvents", 
 *   "MouseEvents", "MutationEvents", and "HTMLEvents" feature names are 
 *   valid values for the parameter <code>eventType</code> and represent 
 *   respectively the interfaces "UIEvent", "MouseEvent", 
 *   "MutationEvent", and "Event". 
 * <p><b>Note:</b>  JSR 280 follows the DOM 3 rule for <code>Event.type</code>
 * and considers it to be case-sensitive. This differs from DOM 2, which
 * considers it to be case-insensitive.
 * @param ret_value Pointer to the object representing 
 *    The newly created event object. 
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
 *             javacall_dom_document_create_event_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_document_create_event_finish(void *context,
                                          /* OUT */ javacall_handle* ret_value,
                                          /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the default <code>AbstractView</code> for this <code>Document</code>, 
 * or <code>NULL</code> if none available.
 * 
 * @param handle Pointer to the object representing this document.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_document_get_default_view_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_document_get_default_view_start(javacall_handle handle,
                                             javacall_int32 invocation_id,
                                             void **context,
                                             /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns the default <code>AbstractView</code> for this <code>Document</code>, 
 * or <code>NULL</code> if none available.
 * 
 * @param context The context saved during asynchronous operation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_document_get_default_view_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_document_get_default_view_finish(void *context,
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
javacall_dom_document_clear_references(javacall_handle handle, javacall_int32 count);


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ifndef __JAVACALL_DOM_DOCUMENT_H_ */
