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

#ifndef __JAVACALL_DOM_HTMLDOCUMENT_H_
#define __JAVACALL_DOM_HTMLDOCUMENT_H_

/**
 * @file javacall_dom_htmldocument.h
 * @ingroup JSR290DOM
 * @brief Javacall DOM interfaces for HTMLDocument
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
 * OR with [<a href="http://www.w3.org/TR/1999/REC-html401-19991224">HTML
 * 4.01</a>] documents, this method returns the (possibly empty) 
 * collection of elements whose <code>name</code> value is given by 
 * <code>element_name</code>. In 
 * [<a href="http://www.w3.org/TR/2002/REC-xhtml1-20020801">XHTML 1.0</a>] 
 * documents, this method only returns the 
 * (possibly empty) collection of form controls with matching name. This 
 * method is case sensitive. The argument <code>element_name</code> 
 * <span class="rfc2119">may</span> be NULL in which case, an empty
 * collection is returned.
 *
 * 
 * @param handle Pointer to the object representing this htmldocument.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param element_name The <code>name</code> attribute value for an 
 * element.
 *
 * @param ret_value Pointer to the object representing 
 *   the matching elements.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_OUT_OF_MEMORY if function fails to allocate memory for the 
 *             context,
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_htmldocument_get_elements_by_name_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_htmldocument_get_elements_by_name_start(javacall_handle handle,
                                                     javacall_int32 invocation_id,
                                                     void **context,
                                                     javacall_const_utf16_string element_name,
                                                     /* OUT */ javacall_handle* ret_value);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR with [<a href="http://www.w3.org/TR/1999/REC-html401-19991224">HTML
 * 4.01</a>] documents, this method returns the (possibly empty) 
 * collection of elements whose <code>name</code> value is given by 
 * <code>elementName</code>. In 
 * [<a href="http://www.w3.org/TR/2002/REC-xhtml1-20020801">XHTML 1.0</a>] 
 * documents, this method only returns the 
 * (possibly empty) collection of form controls with matching name. This 
 * method is case sensitive. The argument <code>elementName</code> 
 * <span class="rfc2119">may</span> be NULL in which case, an empty
 * collection is returned.
 *
 * 
 * @param context The context saved during asynchronous operation.
 * element.
 *
 * @param ret_value Pointer to the object representing 
 *   the matching elements.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error in native code occured
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_htmldocument_get_elements_by_name_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_htmldocument_get_elements_by_name_finish(void *context,
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
javacall_dom_htmldocument_clear_references(javacall_handle handle, javacall_int32 count);


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ifndef __JAVACALL_DOM_HTMLDOCUMENT_H_ */
