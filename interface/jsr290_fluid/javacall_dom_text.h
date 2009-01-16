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
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR breaks this node into two nodes at the specified <code>offset</code>, 
 * keeping both in the tree as siblings. After being split, this node 
 * will contain all the content up to the <code>offset</code> point. A 
 * new node of the same type, which contains all the content at and 
 * after the <code>offset</code> point, is returned. If the original 
 * node had a parent node, the new node is inserted as the next sibling 
 * of the original node. When the <code>offset</code> is equal to the 
 * length of this node, the new node has no data.
 * 
 * @param handle Pointer to the object representing this text.
 * @param invocation_id Invocation identifier which MUST be used in the 
 *                  corresponding javanotify function.
 * @param context The context saved during asynchronous operation.
 * @param offset The 16-bit unit offset at which to split, starting from 
 *   <code>0</code>.
 * @param ret_value Pointer to the object representing 
 *   the new node, of the same type as this node.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_INDEX_SIZE_ERR
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
 *                 JAVACALL_DOM_INDEX_SIZE_ERR
 *                 JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_text_split_text_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_text_split_text_start(javacall_handle handle,
                                   javacall_int32 invocation_id,
                                   void **context,
                                   javacall_int32 offset,
                                   /* OUT */ javacall_handle* ret_value,
                                   /* OUT */ javacall_dom_exceptions* exception_code);

/**
 * Forms request to the native engine and returns with JAVACALL_WOULD_BLOCK code 
 * OR breaks this node into two nodes at the specified <code>offset</code>, 
 * keeping both in the tree as siblings. After being split, this node 
 * will contain all the content up to the <code>offset</code> point. A 
 * new node of the same type, which contains all the content at and 
 * after the <code>offset</code> point, is returned. If the original 
 * node had a parent node, the new node is inserted as the next sibling 
 * of the original node. When the <code>offset</code> is equal to the 
 * length of this node, the new node has no data.
 * 
 * @param context The context saved during asynchronous operation.
 *   <code>0</code>.
 * @param ret_value Pointer to the object representing 
 *   the new node, of the same type as this node.
 * @param exception_code Code of the error if function fails; the following 
 *                       codes are acceptable: 
 *                            JAVACALL_DOM_RUNTIME_ERR
 *                            JAVACALL_DOM_INDEX_SIZE_ERR
 *                            JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 * 
 * @return JAVACALL_OK if all done successfuly,
 *         JAVACALL_FAIL if error or exception occured;
 *             in this case exception_code has to be filled.
 *             JAVACALL_DOM_RUNTIME_ERR stands for an error in native code,
 *             For exception that might be thrown by native engine
 *             corresponding exception code should be set:
 *                 JAVACALL_DOM_INDEX_SIZE_ERR
 *                 JAVACALL_DOM_NO_MODIFICATION_ALLOWED_ERR
 *         JAVACALL_WOULD_BLOCK caller must call the 
 *             javacall_dom_text_split_text_finish function to complete the 
 *             operation,
 *         JAVACALL_NOT_IMPLEMENTED when the stub was called
 */
javacall_result
javacall_dom_text_split_text_finish(void *context,
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
javacall_dom_text_clear_references(javacall_handle handle, javacall_int32 count);


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ifndef __JAVACALL_DOM_TEXT_H_ */
