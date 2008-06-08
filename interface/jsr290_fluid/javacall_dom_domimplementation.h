/*
* Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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

#include <javacall_dom.h>

/**
 * Test if the DOM implementation implements a specific feature.
 * 
 * @param handle Pointer to the object representing this domimplementation.
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
 * @param retValue <code>true</code> if the feature is implemented in the 
 *   specified version, <code>false</code> otherwise.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_domimplementation_has_feature(javacall_handle handle,
                                           javacall_const_utf16_string feature,
                                           javacall_const_utf16_string version,
                                           /* OUT */ javacall_bool* retValue);

/**
 * Creates an empty <code>DocumentType</code> node. Entity declarations 
 * and notations are not made available. Entity reference expansions and 
 * default attribute additions do not occur. It is expected that a 
 * future version of the DOM will provide a way for populating a 
 * <code>DocumentType</code>.
 * 
 * Note: If retValueLen is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill retValueLen 
 *       with actual length of the returned string.
 *
 * @param handle Pointer to the object representing this domimplementation.
 * @param qualifiedName The qualified name of the document type to be 
 *   created. 
 * @param publicId The external subset public identifier.
 * @param systemId The external subset system identifier.
 * @param retValue Pointer to the object representing 
 *   a new <code>DocumentType</code> node with 
 *   <code>Node.ownerDocument</code> set to <code>NULL</code>.
 * @param exceptionCode Code of the error if function fails; 
 *                      see javacall_dom_exceptions 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error occured; in this case exceptionCode has to be 
                                  filled,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_domimplementation_create_document_type(javacall_handle handle,
                                                    javacall_const_utf16_string qualifiedName,
                                                    javacall_const_utf16_string publicId,
                                                    javacall_const_utf16_string systemId,
                                                    /* OUT */ javacall_handle* retValue,
                                                    /* OUT */ javacall_dom_exceptions* exceptionCode);

/**
 * Creates an XML <code>Document</code> object of the specified type with 
 * its document element. 
 * 
 * Note: If retValueLen is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill retValueLen 
 *       with actual length of the returned string.
 *
 * @param handle Pointer to the object representing this domimplementation.
 * @param namespaceURI The namespace URI of the document element to create.
 * @param qualifiedName The qualified name of the document element to be 
 *   created.
 * @param doctype Pointer to the object of
 *   the type of document to be created or <code>NULL</code>.
 *   When <code>doctype</code> is not <code>NULL</code>, its 
 *   <code>Node.ownerDocument</code> attribute is set to the document 
 *   being created.
 * @param retValue Pointer to the object representing 
 *   a new <code>Document</code> object.
 * @param exceptionCode Code of the error if function fails; 
 *                      see javacall_dom_exceptions 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error occured; in this case exceptionCode has to be 
                                  filled,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_domimplementation_create_document(javacall_handle handle,
                                               javacall_const_utf16_string namespaceURI,
                                               javacall_const_utf16_string qualifiedName,
                                               javacall_handle doctype,
                                               /* OUT */ javacall_handle* retValue,
                                               /* OUT */ javacall_dom_exceptions* exceptionCode);

/**
 * Returns  This method returns a specialized object which implements the 
 * specialized APIs of the specified feature and version, as specified 
 * in <a href="http://www.w3.org/TR/DOM-Level-3-Core/core.html#DOMFeatures"
 * >DOM Features</a>. The specialized object may also be obtained by using 
 * binding-specific casting methods but is not necessarily expected to, 
 * as discussed in 
 * <a href="http://www.w3.org/TR/DOM-Level-3-Core/core.html#Embedded-DOM"
 * >Mixed DOM Implementations</a>.
 * This method also allow the implementation to 
 * provide specialized objects which do not support the 
 * <code>DOMImplementation</code> interface. 
 * <p><b>Note:</b> when using the methods that take a feature and a 
 * version as parameters, applications can use NULL or empty string 
 * for the version parameter if they don't wish to specify a particular
 * version for the specified feature.
 * 
 * @param handle Pointer to the object representing this domimplementation.
 * @param feature  The name of the feature requested. Note that any plus 
 *   sign "+" prepended to the name of the feature will be ignored since 
 *   it is not significant in the context of this method. 
 * @param version  This is the version number of the feature to test. 
 * @param retValue Pointer to the object representing 
 *    Returns an object which implements the specialized APIs of 
 *   the specified feature and version, if any, or <code>NULL</code> if 
 *   there is no object which implements interfaces associated with that 
 *   feature. If the <code>DOMObject</code> returned by this method 
 *   implements the <code>DOMImplementation</code> interface, it must 
 *   delegate to the primary core <code>DOMImplementation</code> and not 
 *   return results inconsistent with the primary core 
 *   <code>DOMImplementation</code> such as <code>hasFeature</code>, 
 *   <code>getFeature</code>, etc. 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_domimplementation_get_feature(javacall_handle handle,
                                           javacall_const_utf16_string feature,
                                           javacall_const_utf16_string version,
                                           /* OUT */ javacall_handle* retValue);

/** 
 * Deletes object representing this domimplementation
 * 
 * @param handle Pointer to the object representing this domimplementation.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_domimplementation_finalize(javacall_handle handle);

