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

#ifndef __JAVACALL_DOM_DOCUMENTTYPE_H_
#define __JAVACALL_DOM_DOCUMENTTYPE_H_

/**
 * @file javacall_dom_documenttype.h
 * @ingroup JSR290DOM
 * @brief Javacall DOM interfaces for DocumentType
 */

#include <javacall_dom.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returns the name of DTD; i.e., the name immediately following the 
 * <code>DOCTYPE</code> keyword.
 * 
 * Note: If retValueLen is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill retValueLen 
 *       with actual length of the returned string.
 *
 * @param handle Pointer to the object representing this documenttype.
 * @param retValue the name of the DTD
 * @param retValueLen Length of the returned string
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_OUT_OF_MEMORY if length of the returend string is more then 
 *                                specified in retValueLen,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_documenttype_get_name(javacall_handle handle,
                                   /* OUT */ javacall_utf16_string retValue,
                                   /* INOUT */ javacall_uint32* retValueLen);

/**
 * Returns a <code>NamedNodeMap</code> containing the general entities, both 
 * external and internal, declared in the DTD. Parameter entities are 
 * not contained. Duplicates are discarded. For example in: 
 * <pre>&lt;!DOCTYPE 
 * ex SYSTEM "ex.dtd" [ &lt;!ENTITY foo "foo"&gt; &lt;!ENTITY bar 
 * "bar"&gt; &lt;!ENTITY bar "bar2"&gt; &lt;!ENTITY % baz "baz"&gt; 
 * ]&gt; &lt;ex/&gt;</pre>
 *  the interface provides access to <code>foo</code> 
 * and the first declaration of <code>bar</code> but not the second 
 * declaration of <code>bar</code> or <code>baz</code>. Every node in 
 * this map also implements the <code>Entity</code> interface.
 * <br>The DOM Level 2 does not support editing entities, therefore 
 * <code>entities</code> cannot be altered in any way.
 * 
 * @param handle Pointer to the object representing this documenttype.
 * @param retValue Pointer to the object representing 
 *   a <code>NamedNodeMap</code> containing the general entities 
 * contained in the DTD
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_documenttype_get_entities(javacall_handle handle,
                                       /* OUT */ javacall_handle* retValue);

/**
 * Returns a <code>NamedNodeMap</code> containing the notations declared in the 
 * DTD. Duplicates are discarded. Every node in this map also implements 
 * the <code>Notation</code> interface.
 * <br>The DOM Level 2 does not support editing notations, therefore 
 * <code>notations</code> cannot be altered in any way.
 * 
 * @param handle Pointer to the object representing this documenttype.
 * @param retValue Pointer to the object representing 
 *   a <code>NamedNodeMap</code> containing the notations declared in the DTD
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_documenttype_get_notations(javacall_handle handle,
                                        /* OUT */ javacall_handle* retValue);

/**
 * Returns the public identifier of the external subset.
 * 
 * Note: If retValueLen is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill retValueLen 
 *       with actual length of the returned string.
 *
 * @param handle Pointer to the object representing this documenttype.
 * @param retValue the public identifier of the external subset
 * @param retValueLen Length of the returned string
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_OUT_OF_MEMORY if length of the returend string is more then 
 *                                specified in retValueLen,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_documenttype_get_public_id(javacall_handle handle,
                                        /* OUT */ javacall_utf16_string retValue,
                                        /* INOUT */ javacall_uint32* retValueLen);

/**
 * Returns the system identifier of the external subset.
 * 
 * Note: If retValueLen is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill retValueLen 
 *       with actual length of the returned string.
 *
 * @param handle Pointer to the object representing this documenttype.
 * @param retValue The system identifier of the external subset
 * @param retValueLen Length of the returned string
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_OUT_OF_MEMORY if length of the returend string is more then 
 *                                specified in retValueLen,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_documenttype_get_system_id(javacall_handle handle,
                                        /* OUT */ javacall_utf16_string retValue,
                                        /* INOUT */ javacall_uint32* retValueLen);

/**
 * Returns the internal subset as a string, or <code>NULL</code> if there is none.
 * <p><b>Note:</b> The actual content returned depends on how much
 * information is available to the implementation. This may vary
 * depending on various parameters, including the XML processor used to
 * build the document.
 * 
 * Note: If retValueLen is less then length of the returned string this function 
 *       has to return with JAVACALL_OUT_OF_MEMORY code and fill retValueLen 
 *       with actual length of the returned string.
 *
 * @param handle Pointer to the object representing this documenttype.
 * @param retValue a String containing a representation of the internal subset
 * @param retValueLen Length of the returned string
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_OUT_OF_MEMORY if length of the returend string is more then 
 *                                specified in retValueLen,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_documenttype_get_internal_subset(javacall_handle handle,
                                              /* OUT */ javacall_utf16_string retValue,
                                              /* INOUT */ javacall_uint32* retValueLen);

/** 
 * Deletes object representing this documenttype
 * 
 * @param handle Pointer to the object representing this documenttype.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_documenttype_finalize(javacall_handle handle);


#ifdef __cplusplus
}
#endif

#endif /* ifndef __JAVACALL_DOM_DOCUMENTTYPE_H_ */
