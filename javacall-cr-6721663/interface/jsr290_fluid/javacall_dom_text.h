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

#ifndef __JAVACALL_DOM_TEXT_H_
#define __JAVACALL_DOM_TEXT_H_

/**
 * @file javacall_dom_text.h
 * @ingroup JSR290DOM
 * @brief Javacall DOM interfaces for Text
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
 * Breaks this node into two nodes at the specified <code>offset</code>, 
 * keeping both in the tree as siblings. After being split, this node 
 * will contain all the content up to the <code>offset</code> point. A 
 * new node of the same type, which contains all the content at and 
 * after the <code>offset</code> point, is returned. If the original 
 * node had a parent node, the new node is inserted as the next sibling 
 * of the original node. When the <code>offset</code> is equal to the 
 * length of this node, the new node has no data.
 * 
 * @param handle Pointer to the object representing this text.
 * @param offset The 16-bit unit offset at which to split, starting from 
 *   <code>0</code>.
 * @param retValue Pointer to the object representing 
 *   the new node, of the same type as this node.
 * @param exceptionCode Code of the error if function fails; 
 *                      see javacall_dom_exceptions 
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error occured; in this case exceptionCode has to be 
 *                                filled,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_text_split_text(javacall_handle handle,
                             javacall_int32 offset,
                             /* OUT */ javacall_handle* retValue,
                             /* OUT */ javacall_dom_exceptions* exceptionCode);

/** 
 * Deletes object representing this text
 * 
 * @param handle Pointer to the object representing this text.
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_text_finalize(javacall_handle handle);


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ifndef __JAVACALL_DOM_TEXT_H_ */
