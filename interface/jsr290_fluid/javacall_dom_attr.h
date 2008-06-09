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

#ifndef __JAVACALL_DOM_ATTR_H_
#define __JAVACALL_DOM_ATTR_H_

/**
 * @file javacall_dom_attr.h
 * @ingroup JSR290DOM
 * @brief Javacall DOM interfaces for Attr
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
 * Returns returns the name of this attribute. If <code>Node.localName</code> is 
 * different from <code>NULL</code>, this attribute is a qualified name.
 * 
 * Note: If retValueLen is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill retValueLen 
 *       with actual length of the returned string.
 *
 * @param handle Pointer to the object representing this attr.
 * @param retValue the attribute name
 * @param retValueLen Length of the returned string
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_OUT_OF_MEMORY if length of the returend string is more then 
 *                                specified in retValueLen,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_attr_get_name(javacall_handle handle,
                           /* OUT */ javacall_utf16_string retValue,
                           /* INOUT */ javacall_uint32* retValueLen);

/**
 * Returns if this attribute was explicitly given a value in the original 
 * document, this is <code>true</code>; otherwise, it is 
 * <code>false</code>. Note that the implementation is in charge of this 
 * attribute, not the user. If the user changes the value of the 
 * attribute (even if it ends up having the same value as the default 
 * value) then the <code>specified</code> flag is automatically flipped 
 * to <code>true</code>. To re-specify the attribute as the default 
 * value from the DTD, the user must delete the attribute. The 
 * implementation will then make a new attribute available with 
 * <code>specified</code> set to <code>false</code> and the default 
 * value (if one exists).
 * <br>In summary:  If the attribute has an assigned value in the document 
 * then <code>specified</code> is <code>true</code>, and the value is 
 * the assigned value.  If the attribute has no assigned value in the 
 * document and has a default value in the DTD, then 
 * <code>specified</code> is <code>false</code>, and the value is the 
 * default value in the DTD. If the attribute has no assigned value in 
 * the document and has a value of #IMPLIED in the DTD, then the 
 * attribute does not appear in the structure model of the document. If 
 * the <code>ownerElement</code> attribute is <code>NULL</code> (i.e. 
 * because it was just created or was set to <code>NULL</code> by the 
 * various removal and cloning operations) <code>specified</code> is 
 * <code>true</code>. 
 * 
 * @param handle Pointer to the object representing this attr.
 * @param retValue <code>true</code> if this attribute was explicitly specified, otherwise <code>false</code>
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_attr_get_specified(javacall_handle handle,
                                /* OUT */ javacall_bool* retValue);

/**
 * Returns returns the value of this attribute. 
 * On retrieval, the value of the attribute is returned as a string. 
 * Character and general entity references are replaced with their 
 * values. See also the method <code>getAttribute</code> on the 
 * <code>Element</code> interface.
 * <br>On setting, this creates a <code>Text</code> node with the unparsed 
 * contents of the string. I.e. any characters that an XML processor 
 * would recognize as markup are instead treated as literal text. See 
 * also the method <code>Element.setAttribute</code>.
 * <br> Some specialized implementations, such as some [<a href='http://www.w3.org/TR/2003/REC-SVG11-20030114/'>SVG 1.1</a>] 
 * implementations, may do normalization automatically, even after 
 * mutation; in such case, the value on retrieval may differ from the 
 * value on setting.
 * 
 * Note: If retValueLen is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill retValueLen 
 *       with actual length of the returned string.
 *
 * @param handle Pointer to the object representing this attr.
 * @param retValue a String containing the value of this attribute
 * @param retValueLen Length of the returned string
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_OUT_OF_MEMORY if length of the returend string is more then 
 *                                specified in retValueLen,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_attr_get_value(javacall_handle handle,
                            /* OUT */ javacall_utf16_string retValue,
                            /* INOUT */ javacall_uint32* retValueLen);

/**
 * Sets sets the value of this attribute. 
 * On setting, this creates a <code>Text</code> node with the unparsed 
 * contents of the string. I.e. any characters that an XML processor 
 * would recognize as markup are instead treated as literal text. See 
 * also the method <code>Element.setAttribute</code>.
 * <br> Some specialized implementations, such as some [<a href='http://www.w3.org/TR/2003/REC-SVG11-20030114/'>SVG 1.1</a>] 
 * implementations, may do normalization automatically, even after 
 * mutation; in such case, the value on retrieval may differ from the 
 * value on setting.
 * 
 * @param handle Pointer to the object representing this attr.
 * @param value a String containing the value of this attribute
 * @param exceptionCode Code of the error if function fails; 
 *                      see javacall_dom_exceptions 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error occured; in this case exceptionCode has to be 
                                  filled,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_attr_set_value(javacall_handle handle,
                            javacall_const_utf16_string value,
                            /* OUT */ javacall_dom_exceptions* exceptionCode);

/**
 * Returns the <code>Element</code> node this attribute is attached to or 
 * <code>NULL</code> if this attribute is not in use.
 * 
 * @param handle Pointer to the object representing this attr.
 * @param retValue Pointer to the object representing 
 *   the <code>Element</code> node this attribute is attached to, or <code>NULL</code>
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_attr_get_owner_element(javacall_handle handle,
                                    /* OUT */ javacall_handle* retValue);

/**
 * Returns whether this attribute is known to be of type ID or not. 
 * In other words, whether this attribute 
 * contains an identifier for its owner element or not. When it is and 
 * its value is unique, the <code>ownerElement</code> of this attribute 
 * can be retrieved using the method <code>Document.getElementById</code>.
 * <p>Note: The JSR 280 DOM subset does not support XML schema or
 * <code>Document.normalizeDocument()</code>, and thus supports only 
 * a subset of the DOM 3 mechanisms for identifying ID attributes:
 * <ul>
 * <li> the use of the methods <code>Element.setIdAttribute()</code>, 
 * <code>Element.setIdAttributeNS()</code>, or 
 * <code>Element.setIdAttributeNode()</code>, i.e. it is an 
 * user-determined ID attribute.</li>
 * </ul>
 * 
 * @param handle Pointer to the object representing this attr.
 * @param retValue <code>true</code> if the attribute is of type ID, otherwise <code>false</code>
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_attr_is_id(javacall_handle handle,
                        /* OUT */ javacall_bool* retValue);

/** 
 * Deletes object representing this attr
 * 
 * @param handle Pointer to the object representing this attr.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_attr_finalize(javacall_handle handle);


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ifndef __JAVACALL_DOM_ATTR_H_ */
