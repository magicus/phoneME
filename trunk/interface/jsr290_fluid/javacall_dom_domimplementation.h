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

#ifndef __JAVACALL_DOM_DOMIMPLEMENTATION_H_
#define __JAVACALL_DOM_DOMIMPLEMENTATION_H_

/**
 * @file javacall_dom_domimplementation.h
 * @ingroup JSR290DOM
 * @brief Javacall DOM interfaces for DOMImplementation
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
 * OR test if the DOM implementation implements a specific feature.
 * 
 * @param handle Pointer to the object representing this domimplementation.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param feature The name of the feature to test (case-insensitive). The 
 *   values used by DOM features are defined throughout the DOM Level 2 
 *   specifications and listed in the  section. The name must be an XML 
 *   name. To avoid possible conflicts, as a convention, names referring 
 *   to features defined outside the DOM specification should be made 
 *   unique by reversing the name of the Internet domain name of the 
 *   person (or the organization that the person belongs to) who defines 
 *   the feature, component by component, and using this as a prefix. 
 *   For instance, the W3C SVG Working Group defines the feature 
 *   "org.w3c.dom.svg".
 * @param version This is the version number of the feature to test. In 
 *   Level 2, the string can be either "2.0" or "1.0". If the version is 
 *   not specified (parameter is either NULL or the empty string, as per 
 *   http://www.w3.org/TR/DOM-Level-3-Events/events.html#Conformance), 
 *   supporting any version of the feature causes the 
 *   method to return <code>true</code>.
 * @param ret_value <code>true</code> if the feature is implemented in the 
 *   specified version, <code>false</code> otherwise.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_domimplementation_has_feature_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_domimplementation_has_feature_start(javacall_handle handle,
                                                 javacall_int32 invocation_id,
                                                 void **context,
                                                 javacall_const_utf16_string feature,
                                                 javacall_const_utf16_string version,
                                                 /* OUT */ javacall_bool* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR test if the DOM implementation implements a specific feature.
 * 
 * @param context The context saved during asynchronous operation.
 *   Level 2, the string can be either "2.0" or "1.0". If the version is 
 *   not specified (parameter is either NULL or the empty string, as per 
 *   http://www.w3.org/TR/DOM-Level-3-Events/events.html#Conformance), 
 *   supporting any version of the feature causes the 
 *   method to return <code>true</code>.
 * @param ret_value <code>true</code> if the feature is implemented in the 
 *   specified version, <code>false</code> otherwise.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_domimplementation_has_feature_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_domimplementation_has_feature_finish(void *context,
                                                  /* OUT */ javacall_bool* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR creates an empty <code>DocumentType</code> node. Entity declarations 
 * and notations are not made available. Entity reference expansions and 
 * default attribute additions do not occur. It is expected that a 
 * future version of the DOM will provide a way for populating a 
 * <code>DocumentType</code>.
 * 
 * @param handle Pointer to the object representing this domimplementation.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param qualified_name The qualified name of the document type to be 
 *   created. 
 * @param public_id The external subset public identifier.
 * @param system_id The external subset system identifier.
 * @param ret_value Pointer to the object representing 
 *   a new <code>DocumentType</code> node with 
 *   <code>Node.ownerDocument</code> set to <code>NULL</code>.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_INVALID_CHARACTER_ERR
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
 *                 JAVACALL_DOM_NAMESPACE_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_domimplementation_create_document_type_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_domimplementation_create_document_type_start(javacall_handle handle,
                                                          javacall_int32 invocation_id,
                                                          void **context,
                                                          javacall_const_utf16_string qualified_name,
                                                          javacall_const_utf16_string public_id,
                                                          javacall_const_utf16_string system_id,
                                                          /* OUT */ javacall_handle* ret_value,
                                                          /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR creates an empty <code>DocumentType</code> node. Entity declarations 
 * and notations are not made available. Entity reference expansions and 
 * default attribute additions do not occur. It is expected that a 
 * future version of the DOM will provide a way for populating a 
 * <code>DocumentType</code>.
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   a new <code>DocumentType</code> node with 
 *   <code>Node.ownerDocument</code> set to <code>NULL</code>.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_INVALID_CHARACTER_ERR
 *                            JAVACALL_DOM_NAMESPACE_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_INVALID_CHARACTER_ERR
 *                 JAVACALL_DOM_NAMESPACE_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_domimplementation_create_document_type_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_domimplementation_create_document_type_finish(void *context,
                                                           /* OUT */ javacall_handle* ret_value,
                                                           /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR creates an XML <code>Document</code> object of the specified type with 
 * its document element. 
 * 
 * @param handle Pointer to the object representing this domimplementation.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param namespace_uri The namespace URI of the document element to create.
 * @param qualified_name The qualified name of the document element to be 
 *   created.
 * @param doctype Pointer to the object of
 *   the type of document to be created or <code>NULL</code>.
 *   When <code>doctype</code> is not <code>NULL</code>, its 
 *   <code>Node.ownerDocument</code> attribute is set to the document 
 *   being created.
 * @param ret_value Pointer to the object representing 
 *   a new <code>Document</code> object.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_INVALID_CHARACTER_ERR
 *                            JAVACALL_DOM_NAMESPACE_ERR
 *                            JAVACALL_DOM_WRONG_DOCUMENT_ERR
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
 *                 JAVACALL_DOM_WRONG_DOCUMENT_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_domimplementation_create_document_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_domimplementation_create_document_start(javacall_handle handle,
                                                     javacall_int32 invocation_id,
                                                     void **context,
                                                     javacall_const_utf16_string namespace_uri,
                                                     javacall_const_utf16_string qualified_name,
                                                     javacall_handle doctype,
                                                     /* OUT */ javacall_handle* ret_value,
                                                     /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR creates an XML <code>Document</code> object of the specified type with 
 * its document element. 
 * 
 * @param context The context saved during asynchronous operation.
 *   When <code>doctype</code> is not <code>NULL</code>, its 
 *   <code>Node.ownerDocument</code> attribute is set to the document 
 *   being created.
 * @param ret_value Pointer to the object representing 
 *   a new <code>Document</code> object.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_INVALID_CHARACTER_ERR
 *                            JAVACALL_DOM_NAMESPACE_ERR
 *                            JAVACALL_DOM_WRONG_DOCUMENT_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_INVALID_CHARACTER_ERR
 *                 JAVACALL_DOM_NAMESPACE_ERR
 *                 JAVACALL_DOM_WRONG_DOCUMENT_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_domimplementation_create_document_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_domimplementation_create_document_finish(void *context,
                                                      /* OUT */ javacall_handle* ret_value,
                                                      /* OUT */ javacall_dom_exceptions* exception_code);

/*
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns handle to the new instance of DOM implementaiton
 * 
 * 
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   new instance of DOM implementaiton. 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_domimplementation_create_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_domimplementation_create_start(javacall_int32 invocation_id,
                                            void **context,
                                            /* OUT */ javacall_handle* ret_value);

/*
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR returns handle to the new instance of DOM implementaiton
 * 
 * 
 * @param context The context saved during asynchronous operation.
 * @param ret_value Pointer to the object representing 
 *   new instance of DOM implementaiton. 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_domimplementation_create_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_domimplementation_create_finish(void *context,
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
javacall_dom_domimplementation_clear_references(javacall_handle handle, javacall_int32 count);


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ifndef __JAVACALL_DOM_DOMIMPLEMENTATION_H_ */
