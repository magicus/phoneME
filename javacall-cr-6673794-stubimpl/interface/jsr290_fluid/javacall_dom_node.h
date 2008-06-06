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

javacall_result
javacall_dom_node_get_node_name(javacall_handle handle,
                                /* OUT */ javacall_utf16_string retValue,
                                /* INOUT */ javacall_uint32* retValueLen);

javacall_result
javacall_dom_node_get_node_value(javacall_handle handle,
                                 /* OUT */ javacall_utf16_string retValue,
                                 /* INOUT */ javacall_uint32* retValueLen,
                                 /* OUT */ javacall_int16 exceptionCode);

javacall_result
javacall_dom_node_set_node_value(javacall_handle handle,
                                 javacall_const_utf16_string nodeValue,
                                 /* OUT */ javacall_int16 exceptionCode);

javacall_result
javacall_dom_node_get_node_type(javacall_handle handle,
                                /* OUT */ javacall_int16* retValue);

javacall_result
javacall_dom_node_get_parent_node(javacall_handle handle,
                                  /* OUT */ javacall_handle* retValue);

javacall_result
javacall_dom_node_get_child_nodes(javacall_handle handle,
                                  /* OUT */ javacall_handle* retValue);

javacall_result
javacall_dom_node_get_first_child(javacall_handle handle,
                                  /* OUT */ javacall_handle* retValue);

javacall_result
javacall_dom_node_get_last_child(javacall_handle handle,
                                 /* OUT */ javacall_handle* retValue);

javacall_result
javacall_dom_node_get_previous_sibling(javacall_handle handle,
                                       /* OUT */ javacall_handle* retValue);

javacall_result
javacall_dom_node_get_next_sibling(javacall_handle handle,
                                   /* OUT */ javacall_handle* retValue);

javacall_result
javacall_dom_node_get_attributes(javacall_handle handle,
                                 /* OUT */ javacall_handle* retValue);

javacall_result
javacall_dom_node_get_owner_document(javacall_handle handle,
                                     /* OUT */ javacall_handle* retValue);

javacall_result
javacall_dom_node_insert_before(javacall_handle handle,
                                javacall_handle newChild,
                                javacall_handle refChild,
                                /* OUT */ javacall_handle* retValue,
                                /* OUT */ javacall_int16 exceptionCode);

javacall_result
javacall_dom_node_replace_child(javacall_handle handle,
                                javacall_handle newChild,
                                javacall_handle oldChild,
                                /* OUT */ javacall_handle* retValue,
                                /* OUT */ javacall_int16 exceptionCode);

javacall_result
javacall_dom_node_remove_child(javacall_handle handle,
                               javacall_handle oldChild,
                               /* OUT */ javacall_handle* retValue,
                               /* OUT */ javacall_int16 exceptionCode);

javacall_result
javacall_dom_node_append_child(javacall_handle handle,
                               javacall_handle newChild,
                               /* OUT */ javacall_handle* retValue,
                               /* OUT */ javacall_int16 exceptionCode);

javacall_result
javacall_dom_node_has_child_nodes(javacall_handle handle,
                                  /* OUT */ javacall_bool* retValue);

javacall_result
javacall_dom_node_clone_node(javacall_handle handle,
                             javacall_bool deep,
                             /* OUT */ javacall_handle* retValue);

javacall_result
javacall_dom_node_normalize(javacall_handle handle);

javacall_result
javacall_dom_node_is_supported(javacall_handle handle,
                               javacall_const_utf16_string feature,
                               javacall_const_utf16_string version,
                               /* OUT */ javacall_bool* retValue);

javacall_result
javacall_dom_node_get_namespace_uri(javacall_handle handle,
                                    /* OUT */ javacall_utf16_string retValue,
                                    /* INOUT */ javacall_uint32* retValueLen);

javacall_result
javacall_dom_node_get_prefix(javacall_handle handle,
                             /* OUT */ javacall_utf16_string retValue,
                             /* INOUT */ javacall_uint32* retValueLen);

javacall_result
javacall_dom_node_set_prefix(javacall_handle handle,
                             javacall_const_utf16_string prefix,
                             /* OUT */ javacall_int16 exceptionCode);

javacall_result
javacall_dom_node_get_local_name(javacall_handle handle,
                                 /* OUT */ javacall_utf16_string retValue,
                                 /* INOUT */ javacall_uint32* retValueLen);

javacall_result
javacall_dom_node_has_attributes(javacall_handle handle,
                                 /* OUT */ javacall_bool* retValue);

javacall_result
javacall_dom_node_get_text_content(javacall_handle handle,
                                   /* OUT */ javacall_utf16_string retValue,
                                   /* INOUT */ javacall_uint32* retValueLen,
                                   /* OUT */ javacall_int16 exceptionCode);

javacall_result
javacall_dom_node_set_text_content(javacall_handle handle,
                                   javacall_const_utf16_string textContent,
                                   /* OUT */ javacall_int16 exceptionCode);

javacall_result
javacall_dom_node_get_feature(javacall_handle handle,
                              javacall_const_utf16_string feature,
                              javacall_const_utf16_string version,
                              /* OUT */ javacall_handle* retValue);

javacall_result
javacall_dom_node_finalize();

