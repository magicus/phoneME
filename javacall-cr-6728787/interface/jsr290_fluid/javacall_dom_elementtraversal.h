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

#ifndef __JAVACALL_DOM_ELEMENTTRAVERSAL_H_
#define __JAVACALL_DOM_ELEMENTTRAVERSAL_H_

/**
 * @file javacall_dom_elementtraversal.h
 * @ingroup JSR290DOM
 * @brief Javacall DOM interfaces for ElementTraversal
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
 * Returns retrieves the number of child elements.
 *
 * 
 * @param handle Pointer to the object representing this elementtraversal.
 * @param ret_value the current number of element nodes that are immediate children
 * of this element. <code>0</code> if this element has no child elements.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_elementtraversal_get_child_element_count(javacall_handle handle,
                                                      /* OUT */ javacall_int32* ret_value);

/**
 * Returns retrieves the first child element.
 * 
 * 
 * @param handle Pointer to the object representing this elementtraversal.
 * @param ret_value Pointer to the object representing 
 *   the first child element node of this element.
 * <code>NULL</code> if this element has no child elements.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_elementtraversal_get_first_element_child(javacall_handle handle,
                                                      /* OUT */ javacall_handle* ret_value);

/**
 * Returns retrieves the last child element.
 *
 * 
 * @param handle Pointer to the object representing this elementtraversal.
 * @param ret_value Pointer to the object representing 
 *   the last child element node of this element.
 * <code>NULL</code> if this element has no child elements.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_elementtraversal_get_last_element_child(javacall_handle handle,
                                                     /* OUT */ javacall_handle* ret_value);

/**
 * Returns retrieves the next sibling element.
 * 
 * 
 * @param handle Pointer to the object representing this elementtraversal.
 * @param ret_value Pointer to the object representing 
 *   the next sibling element node of this element.
 * <code>NULL</code> if this element has no element sibling nodes
 * that come after this one in the document tree.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_elementtraversal_get_next_element_sibling(javacall_handle handle,
                                                       /* OUT */ javacall_handle* ret_value);

/**
 * Returns retrieves the previous sibling element.
 * 
 * 
 * @param handle Pointer to the object representing this elementtraversal.
 * @param ret_value Pointer to the object representing 
 *   the previous sibling element node of this element.
 * <code>NULL</code> if this element has no element sibling nodes
 * that come before this one in the document tree.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_elementtraversal_get_previous_element_sibling(javacall_handle handle,
                                                           /* OUT */ javacall_handle* ret_value);

/** 
 * Deletes object representing this elementtraversal
 * 
 * @param handle Pointer to the object representing this elementtraversal.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_elementtraversal_finalize(javacall_handle handle);


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ifndef __JAVACALL_DOM_ELEMENTTRAVERSAL_H_ */
