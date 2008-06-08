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
 * Returns the name of the element. If <code>Node.localName</code> is different
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
 * Note: If retValueLen is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill retValueLen 
 *       with actual length of the returned string.
 *
 * @param handle Pointer to the object representing this element.
 * @param retValue a String containing the name of the element
 * @param retValueLen Length of the returned string
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_OUT_OF_MEMORY if length of the returend string is more then 
 *                                specified in retValueLen,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_get_tag_name(javacall_handle handle,
                                  /* OUT */ javacall_utf16_string retValue,
                                  /* INOUT */ javacall_uint32* retValueLen);

/**
 * Returns retrieves an attribute value by name.
 * 
 * Note: If retValueLen is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill retValueLen 
 *       with actual length of the returned string.
 *
 * @param handle Pointer to the object representing this element.
 * @param name The name of the attribute to retrieve.
 * @param retValue The <code>Attr</code> value as a string, or the empty string 
 *   if that attribute does not have a specified or default value.
 * @param retValueLen Length of the returned string
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_OUT_OF_MEMORY if length of the returend string is more then 
 *                                specified in retValueLen,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_get_attribute(javacall_handle handle,
                                   javacall_const_utf16_string name,
                                   /* OUT */ javacall_utf16_string retValue,
                                   /* INOUT */ javacall_uint32* retValueLen);

/**
 * Sets adds a new attribute. If an attribute with that name is already present 
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
 * @param name The name of the attribute to create or alter.
 * @param value Value to set in string form.
 * @param exceptionCode Code of the error if function fails; 
 *                      see javacall_dom_exceptions 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error occured; in this case exceptionCode has to be 
                                  filled,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_set_attribute(javacall_handle handle,
                                   javacall_const_utf16_string name,
                                   javacall_const_utf16_string value,
                                   /* OUT */ javacall_dom_exceptions* exceptionCode);

/**
 * Removes an attribute by name. If a default value for the removed 
 * attribute is defined in the DTD, a new attribute immediately appears 
 * with the default value as well as the corresponding namespace URI, 
 * local name, and prefix when applicable.
 * <br>If no attribute with this name is found, this method has no effect.
 * <br>To remove an attribute by local name and namespace URI, use the 
 * <code>removeAttributeNS</code> method.
 * 
 * @param handle Pointer to the object representing this element.
 * @param name The name of the attribute to remove.
 * @param exceptionCode Code of the error if function fails; 
 *                      see javacall_dom_exceptions 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error occured; in this case exceptionCode has to be 
                                  filled,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_remove_attribute(javacall_handle handle,
                                      javacall_const_utf16_string name,
                                      /* OUT */ javacall_dom_exceptions* exceptionCode);

/**
 * Returns retrieves an attribute node by name.
 * <br>To retrieve an attribute node by qualified name and namespace URI, 
 * use the <code>getAttributeNodeNS</code> method.
 * 
 * @param handle Pointer to the object representing this element.
 * @param name The name (<code>nodeName</code>) of the attribute to 
 *   retrieve.
 * @param retValue Pointer to the object representing 
 *   the <code>Attr</code> node with the specified name (
 *   <code>nodeName</code>) or <code>NULL</code> if there is no such 
 *   attribute.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_get_attribute_node(javacall_handle handle,
                                        javacall_const_utf16_string name,
                                        /* OUT */ javacall_handle* retValue);

/**
 * Sets adds a new attribute node. If an attribute with that name (
 * <code>nodeName</code>) is already present in the element, it is 
 * replaced by the new one. Replacing an attribute node by itself has no 
 * effect.
 * <br>To add a new attribute node with a qualified name and namespace 
 * URI, use the <code>setAttributeNodeNS</code> method.
 * 
 * @param handle Pointer to the object representing this element.
 * @param newAttr Pointer to the object of
 *   the <code>Attr</code> node to add to the attribute list.
 * @param retValue Pointer to the object representing 
 *   if the <code>newAttr</code> attribute replaces an existing 
 *   attribute, the replaced <code>Attr</code> node is returned, 
 *   otherwise <code>NULL</code> is returned.
 * @param exceptionCode Code of the error if function fails; 
 *                      see javacall_dom_exceptions 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error occured; in this case exceptionCode has to be 
                                  filled,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_set_attribute_node(javacall_handle handle,
                                        javacall_handle newAttr,
                                        /* OUT */ javacall_handle* retValue,
                                        /* OUT */ javacall_dom_exceptions* exceptionCode);

/**
 * Removes the specified attribute node. If a default value for the 
 * removed <code>Attr</code> node is defined in the DTD, a new node 
 * immediately appears with the default value as well as the 
 * corresponding namespace URI, local name, and prefix when applicable. 
 * 
 * @param handle Pointer to the object representing this element.
 * @param oldAttr Pointer to the object of
 *   the <code>Attr</code> node to remove from the attribute 
 *   list.
 * @param retValue Pointer to the object representing 
 *   the <code>Attr</code> node that was removed.
 * @param exceptionCode Code of the error if function fails; 
 *                      see javacall_dom_exceptions 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error occured; in this case exceptionCode has to be 
                                  filled,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_remove_attribute_node(javacall_handle handle,
                                           javacall_handle oldAttr,
                                           /* OUT */ javacall_handle* retValue,
                                           /* OUT */ javacall_dom_exceptions* exceptionCode);

/**
 * Returns returns a <code>NodeList</code> of all descendant <code>Elements</code> 
 * with a given tag name, in document order.
 * 
 * @param handle Pointer to the object representing this element.
 * @param name The name of the tag to match on. The special value "*" 
 *   matches all tags.
 * @param retValue Pointer to the object representing 
 *   a list of matching <code>Element</code> nodes.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_get_elements_by_tag_name(javacall_handle handle,
                                              javacall_const_utf16_string name,
                                              /* OUT */ javacall_handle* retValue);

/**
 * Returns retrieves an attribute value by local name and namespace URI. 
 * <br>Per [<a href='http://www.w3.org/TR/1999/REC-xml-names-19990114/'>XML Namespaces</a>]
 * , applications must use the value <code>NULL</code> as the 
 * <code>namespaceURI</code> parameter for methods if they wish to have 
 * no namespace.
 * 
 * Note: If retValueLen is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill retValueLen 
 *       with actual length of the returned string.
 *
 * @param handle Pointer to the object representing this element.
 * @param namespaceURI The namespace URI of the attribute to retrieve.
 * @param localName The local name of the attribute to retrieve.
 * @param retValue The <code>Attr</code> value as a string, or the empty string 
 *   if that attribute does not have a specified or default value.
 * @param retValueLen Length of the returned string
 * @param exceptionCode Code of the error if function fails; 
 *                      see javacall_dom_exceptions 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_OUT_OF_MEMORY if length of the returend string is more then 
 *                                specified in retValueLen,
 *         JAVACALL_FAIL if error occured; in this case exceptionCode has to be 
 *                                filled,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_get_attribute_ns(javacall_handle handle,
                                      javacall_const_utf16_string namespaceURI,
                                      javacall_const_utf16_string localName,
                                      /* OUT */ javacall_utf16_string retValue,
                                      /* INOUT */ javacall_uint32* retValueLen,
                                      /* OUT */ javacall_dom_exceptions* exceptionCode);

/**
 * Sets adds a new attribute. If an attribute with the same local name and 
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
 * @param handle Pointer to the object representing this element.
 * @param namespaceURI The namespace URI of the attribute to create or 
 *   alter.
 * @param qualifiedName The qualified name of the attribute to create or 
 *   alter.
 * @param value The value to set in string form.
 * @param exceptionCode Code of the error if function fails; 
 *                      see javacall_dom_exceptions 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error occured; in this case exceptionCode has to be 
                                  filled,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_set_attribute_ns(javacall_handle handle,
                                      javacall_const_utf16_string namespaceURI,
                                      javacall_const_utf16_string qualifiedName,
                                      javacall_const_utf16_string value,
                                      /* OUT */ javacall_dom_exceptions* exceptionCode);

/**
 * Removes an attribute by local name and namespace URI.If a default 
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
 * @param handle Pointer to the object representing this element.
 * @param namespaceURI The namespace URI of the attribute to remove.
 * @param localName The local name of the attribute to remove.
 * @param exceptionCode Code of the error if function fails; 
 *                      see javacall_dom_exceptions 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error occured; in this case exceptionCode has to be 
                                  filled,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_remove_attribute_ns(javacall_handle handle,
                                         javacall_const_utf16_string namespaceURI,
                                         javacall_const_utf16_string localName,
                                         /* OUT */ javacall_dom_exceptions* exceptionCode);

/**
 * Returns retrieves an <code>Attr</code> node by local name and namespace URI.
 * <br>Per [<a href='http://www.w3.org/TR/1999/REC-xml-names-19990114/'>XML Namespaces</a>]
 * , applications must use the value <code>NULL</code> as the 
 * <code>namespaceURI</code> parameter for methods if they wish to have 
 * no namespace. 
 * 
 * @param handle Pointer to the object representing this element.
 * @param namespaceURI The namespace URI of the attribute to retrieve.
 * @param localName The local name of the attribute to retrieve.
 * @param retValue Pointer to the object representing 
 *   the <code>Attr</code> node with the specified attribute local 
 *   name and namespace URI or <code>NULL</code> if there is no such 
 *   attribute.
 * @param exceptionCode Code of the error if function fails; 
 *                      see javacall_dom_exceptions 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error occured; in this case exceptionCode has to be 
                                  filled,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_get_attribute_node_ns(javacall_handle handle,
                                           javacall_const_utf16_string namespaceURI,
                                           javacall_const_utf16_string localName,
                                           /* OUT */ javacall_handle* retValue,
                                           /* OUT */ javacall_dom_exceptions* exceptionCode);

/**
 * Sets adds a new attribute. If an attribute with that local name and that 
 * namespace URI is already present in the element, it is replaced by 
 * the new one. Replacing an attribute node by itself has no effect.
 * <br>Per [<a href='http://www.w3.org/TR/1999/REC-xml-names-19990114/'>XML Namespaces</a>]
 * , applications must use the value <code>NULL</code> as the 
 * <code>namespaceURI</code> parameter for methods if they wish to have 
 * no namespace.
 * 
 * @param handle Pointer to the object representing this element.
 * @param newAttr Pointer to the object of
 *   the <code>Attr</code> node to add to the attribute list.
 * @param retValue Pointer to the object representing 
 *   if the <code>newAttr</code> attribute replaces an existing 
 *   attribute with the same local name and namespace URI, the replaced 
 *   <code>Attr</code> node is returned, otherwise <code>NULL</code> is 
 *   returned.
 * @param exceptionCode Code of the error if function fails; 
 *                      see javacall_dom_exceptions 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error occured; in this case exceptionCode has to be 
                                  filled,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_set_attribute_node_ns(javacall_handle handle,
                                           javacall_handle newAttr,
                                           /* OUT */ javacall_handle* retValue,
                                           /* OUT */ javacall_dom_exceptions* exceptionCode);

/**
 * Returns returns a <code>NodeList</code> of all the descendant 
 * <code>Elements</code> with a given local name and namespace URI in 
 * document order.
 * 
 * @param handle Pointer to the object representing this element.
 * @param namespaceURI The namespace URI of the elements to match on. The 
 *   special value "*" matches all namespaces.
 * @param localName The local name of the elements to match on. The 
 *   special value "*" matches all local names.
 * @param retValue Pointer to the object representing 
 *   a new <code>NodeList</code> object containing all the matched 
 *   <code>Elements</code>.
 * @param exceptionCode Code of the error if function fails; 
 *                      see javacall_dom_exceptions 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error occured; in this case exceptionCode has to be 
                                  filled,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_get_elements_by_tag_name_ns(javacall_handle handle,
                                                 javacall_const_utf16_string namespaceURI,
                                                 javacall_const_utf16_string localName,
                                                 /* OUT */ javacall_handle* retValue,
                                                 /* OUT */ javacall_dom_exceptions* exceptionCode);

/**
 * Returns <code>true</code> when an attribute with a given name is 
 * specified on this element or has a default value, <code>false</code> 
 * otherwise.
 * 
 * @param handle Pointer to the object representing this element.
 * @param name The name of the attribute to look for.
 * @param retValue <code>true</code> if an attribute with the given name is 
 *   specified on this element or has a default value, <code>false</code>
 *    otherwise.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_has_attribute(javacall_handle handle,
                                   javacall_const_utf16_string name,
                                   /* OUT */ javacall_bool* retValue);

/**
 * Returns <code>true</code> when an attribute with a given local name and 
 * namespace URI is specified on this element or has a default value, 
 * <code>false</code> otherwise.
 * <br>Per [<a href='http://www.w3.org/TR/1999/REC-xml-names-19990114/'>XML Namespaces</a>]
 * , applications must use the value <code>NULL</code> as the 
 * <code>namespaceURI</code> parameter for methods if they wish to have 
 * no namespace.
 * 
 * @param handle Pointer to the object representing this element.
 * @param namespaceURI The namespace URI of the attribute to look for.
 * @param localName The local name of the attribute to look for.
 * @param retValue <code>true</code> if an attribute with the given local name 
 *   and namespace URI is specified or has a default value on this 
 *   element, <code>false</code> otherwise.
 * @param exceptionCode Code of the error if function fails; 
 *                      see javacall_dom_exceptions 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error occured; in this case exceptionCode has to be 
                                  filled,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_has_attribute_ns(javacall_handle handle,
                                      javacall_const_utf16_string namespaceURI,
                                      javacall_const_utf16_string localName,
                                      /* OUT */ javacall_bool* retValue,
                                      /* OUT */ javacall_dom_exceptions* exceptionCode);

/**
 * Sets  If the parameter <code>isId</code> is <code>true</code>, this method 
 * declares the specified attribute to be a user-determined ID attribute
 * . This affects the value of <code>Attr.isId</code> and the behavior 
 * of <code>Document.getElementById</code>.
 * Use the value <code>false</code> for the parameter 
 * <code>isId</code> to undeclare an attribute for being a 
 * user-determined ID attribute. 
 * <br> To specify an attribute by local name and namespace URI, use the 
 * <code>setIdAttributeNS</code> method. 
 * 
 * @param handle Pointer to the object representing this element.
 * @param name The name of the attribute.
 * @param isId Whether the attribute is a of type ID.
 * @param exceptionCode Code of the error if function fails; 
 *                      see javacall_dom_exceptions 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error occured; in this case exceptionCode has to be 
                                  filled,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_set_id_attribute(javacall_handle handle,
                                      javacall_const_utf16_string name,
                                      javacall_bool isId,
                                      /* OUT */ javacall_dom_exceptions* exceptionCode);

/**
 * Sets  If the parameter <code>isId</code> is <code>true</code>, this method 
 * declares the specified attribute to be a user-determined ID attribute
 * . This affects the value of <code>Attr.isId</code> and the behavior 
 * of <code>Document.getElementById</code>.
 * Use the value <code>false</code> for the parameter 
 * <code>isId</code> to undeclare an attribute for being a 
 * user-determined ID attribute. 
 * 
 * @param handle Pointer to the object representing this element.
 * @param namespaceURI The namespace URI of the attribute.
 * @param localName The local name of the attribute.
 * @param isId Whether the attribute is a of type ID.
 * @param exceptionCode Code of the error if function fails; 
 *                      see javacall_dom_exceptions 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error occured; in this case exceptionCode has to be 
                                  filled,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_set_id_attribute_ns(javacall_handle handle,
                                         javacall_const_utf16_string namespaceURI,
                                         javacall_const_utf16_string localName,
                                         javacall_bool isId,
                                         /* OUT */ javacall_dom_exceptions* exceptionCode);

/**
 * Sets  If the parameter <code>isId</code> is <code>true</code>, this method 
 * declares the specified attribute to be a user-determined ID attribute
 * . This affects the value of <code>Attr.isId</code> and the behavior 
 * of <code>Document.getElementById</code>.
 * Use the value <code>false</code> for the parameter 
 * <code>isId</code> to undeclare an attribute for being a 
 * user-determined ID attribute. 
 * 
 * @param handle Pointer to the object representing this element.
 * @param idAttr Pointer to the object of
 *   the attribute node.
 * @param isId Whether the attribute is a of type ID.
 * @param exceptionCode Code of the error if function fails; 
 *                      see javacall_dom_exceptions 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error occured; in this case exceptionCode has to be 
                                  filled,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_set_id_attribute_node(javacall_handle handle,
                                           javacall_handle idAttr,
                                           javacall_bool isId,
                                           /* OUT */ javacall_dom_exceptions* exceptionCode);

/** 
 * Deletes object representing this element
 * 
 * @param handle Pointer to the object representing this element.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_element_finalize(javacall_handle handle);

