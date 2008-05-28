/*
* Copyright  2000-2008 Sun Microsystems, Inc. All Rights
* Reserved.  Use is subject to license terms.
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
javacall_dom_document_get_doctype(javacall_handle handle,
                                  /* OUT */ javacall_handle* retValue);

javacall_result
javacall_dom_document_get_implementation(javacall_handle handle,
                                         /* OUT */ javacall_handle* retValue);

javacall_result
javacall_dom_document_get_document_element(javacall_handle handle,
                                           /* OUT */ javacall_handle* retValue);

javacall_result
javacall_dom_document_create_element(javacall_handle handle,
                                     javacall_const_utf16_string tagName,
                                     /* OUT */ javacall_handle* retValue,
                                     /* OUT */ javacall_utf8_string exceptionValue);

javacall_result
javacall_dom_document_create_document_fragment(javacall_handle handle,
                                               /* OUT */ javacall_handle* retValue);

javacall_result
javacall_dom_document_create_text_node(javacall_handle handle,
                                       javacall_const_utf16_string data,
                                       /* OUT */ javacall_handle* retValue);

javacall_result
javacall_dom_document_create_comment(javacall_handle handle,
                                     javacall_const_utf16_string data,
                                     /* OUT */ javacall_handle* retValue);

javacall_result
javacall_dom_document_create_cdata_section(javacall_handle handle,
                                           javacall_const_utf16_string data,
                                           /* OUT */ javacall_handle* retValue,
                                           /* OUT */ javacall_utf8_string exceptionValue);

javacall_result
javacall_dom_document_create_processing_instruction(javacall_handle handle,
                                                    javacall_const_utf16_string target,
                                                    javacall_const_utf16_string data,
                                                    /* OUT */ javacall_handle* retValue,
                                                    /* OUT */ javacall_utf8_string exceptionValue);

javacall_result
javacall_dom_document_create_attribute(javacall_handle handle,
                                       javacall_const_utf16_string name,
                                       /* OUT */ javacall_handle* retValue,
                                       /* OUT */ javacall_utf8_string exceptionValue);

javacall_result
javacall_dom_document_create_entity_reference(javacall_handle handle,
                                              javacall_const_utf16_string name,
                                              /* OUT */ javacall_handle* retValue,
                                              /* OUT */ javacall_utf8_string exceptionValue);

javacall_result
javacall_dom_document_get_elements_by_tag_name(javacall_handle handle,
                                               javacall_const_utf16_string tagname,
                                               /* OUT */ javacall_handle* retValue);

javacall_result
javacall_dom_document_import_node(javacall_handle handle,
                                  javacall_handle importedNode,
                                  javacall_bool deep,
                                  /* OUT */ javacall_handle* retValue,
                                  /* OUT */ javacall_utf8_string exceptionValue);

javacall_result
javacall_dom_document_create_element_ns(javacall_handle handle,
                                        javacall_const_utf16_string namespaceURI,
                                        javacall_const_utf16_string qualifiedName,
                                        /* OUT */ javacall_handle* retValue,
                                        /* OUT */ javacall_utf8_string exceptionValue);

javacall_result
javacall_dom_document_create_attribute_ns(javacall_handle handle,
                                          javacall_const_utf16_string namespaceURI,
                                          javacall_const_utf16_string qualifiedName,
                                          /* OUT */ javacall_handle* retValue,
                                          /* OUT */ javacall_utf8_string exceptionValue);

javacall_result
javacall_dom_document_get_elements_by_tag_name_ns(javacall_handle handle,
                                                  javacall_const_utf16_string namespaceURI,
                                                  javacall_const_utf16_string localName,
                                                  /* OUT */ javacall_handle* retValue);

javacall_result
javacall_dom_document_get_element_by_id(javacall_handle handle,
                                        javacall_const_utf16_string elementId,
                                        /* OUT */ javacall_handle* retValue);

javacall_result
javacall_dom_document_adopt_node(javacall_handle handle,
                                 javacall_handle source,
                                 /* OUT */ javacall_handle* retValue,
                                 /* OUT */ javacall_utf8_string exceptionValue);

