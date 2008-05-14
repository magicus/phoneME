/*
 *
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

/**
 * @file javacall_dom_node.h
 * @brief DOM level 2 interface
 */


#ifndef __JAVACALL_DOM_NODE_H
#define __JAVACALL_DOM_NODE_H

#include <javacall_defs.h>

#ifdef __cplusplus
extern "C" {
#endif/*__cplusplus*/


/**
 *
 */
javacall_result 
javacall_dom_node_name(javacall_handle handle,
                       javacall_utf16_string /* OUT */ nodeName,
                       int nodeNameMaxLen);

/**
 *
 */
javacall_result
javacall_dom_node_value(javacall_handle handle,
                        javacall_utf16_string /* OUT */ nodeValue,
                        int nodeValueMaxLen);
                        
/**
 *
 */
javacall_result
javacall_dom_set_node_value(javacall_handle handle,
                            javacall_utf16_string nodeValue,
                            int valueLength);
                        
/**
 *
 */
short javacall_dom_node_type(javacall_handle handle);
                        
/**
 *
 */
long javacall_dom_parent_node(javacall_handle handle);
                        
/**
 *
 */
long javacall_dom_child_nodes(javacall_handle handle);
                        
/**
 *
 */
long javacall_dom_first_child(javacall_handle handle);
                        
/**
 *
 */
long javacall_dom_last_child(javacall_handle handle);
                        
/**
 *
 */
long javacall_dom_previous_sibling(javacall_handle handle);
                        
/**
 *
 */
long javacall_dom_next_sibling(javacall_handle handle);
                        
/**
 *
 */
long javacall_dom_attributes(javacall_handle handle);
                        
/**
 *
 */
long javacall_dom_owner_document(javacall_handle handle);
                        
/**
 *
 */
javacall_result 
javacall_dom_insert_before(javacall_handle handle,
                           javacall_handle newHandle, 
                           javacall_handle refHandle);
                        
/**
 *
 */
long javacall_dom_replace_child(javacall_handle handle,
                                javacall_handle newHandle, 
                                javacall_handle oldHandle);
                        
/**
 *
 */
long javacall_dom_remove_child(javacall_handle handle, 
                               javacall_handle oldHandle);
                        
/**
 *
 */
long javacall_dom_append_child(javacall_handle handle, 
                               javacall_handle newHandle);
                        
/**
 *
 */
javacall_bool javacall_dom_has_child_nodes(javacall_handle handle);
                        
/**
 *
 */
long javacall_dom_clone_node(javacall_handle handle, javacall_bool deep);
                        
/**
 *
 */
javacall_result javacall_dom_normalize(javacall_handle handle);
                        
/**
 *
 */
javacall_bool javacall_dom_is_supported(javacall_handle handle,
                               javacall_utf16_string feature,
                               javacall_utf16_string version);
                        
/**
 *
 */
javacall_result
javacall_dom_namespace_URI(javacall_handle handle,
                           javacall_utf16_string /* OUT */ uri,
                           int uriMaxLen);
                        
/**
 *
 */
javacall_result
javacall_dom_prefix(javacall_handle handle,
                    javacall_utf16_string /* OUT */ prefix,
                    int prefixMaxLen);
                        
/**
 *
 */
javacall_result
javacall_dom_set_prefix(javacall_handle handle, 
                        javacall_utf16_string prefix);
                        
/**
 *
 */
javacall_result
javacall_dom_local_name(javacall_handle handle,
                        javacall_utf16_string /* OUT */ name,
                        int nameMaxLen);
                        
/**
 *
 */
javacall_bool javacall_dom_has_attributes(javacall_handle handle);
                        
/**
 *
 */
javacall_result
javacall_dom_text_content(javacall_handle handle,
                           javacall_utf16_string /* OUT */ text,
                           int textMaxLen);
                        
/**
 *
 */
javacall_result
javacall_dom_set_text_content(javacall_handle handle,
                              javacall_utf16_string text,
                              int valueLength);




#ifdef __cplusplus
}
#endif/*__cplusplus*/

/** @} */
#endif
