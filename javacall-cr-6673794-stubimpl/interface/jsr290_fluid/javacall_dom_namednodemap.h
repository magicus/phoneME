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
javacall_dom_namednodemap_get_named_item(javacall_handle handle,
                                         javacall_const_utf16_string name,
                                         /* OUT */ javacall_handle* retValue);

javacall_result
javacall_dom_namednodemap_set_named_item(javacall_handle handle,
                                         javacall_handle arg,
                                         /* OUT */ javacall_handle* retValue,
                                         /* OUT */ javacall_utf8_string exceptionValue);

javacall_result
javacall_dom_namednodemap_remove_named_item(javacall_handle handle,
                                            javacall_const_utf16_string name,
                                            /* OUT */ javacall_handle* retValue,
                                            /* OUT */ javacall_utf8_string exceptionValue);

javacall_result
javacall_dom_namednodemap_item(javacall_handle handle,
                               javacall_int32 index,
                               /* OUT */ javacall_handle* retValue);

javacall_result
javacall_dom_namednodemap_get_length(javacall_handle handle,
                                     /* OUT */ javacall_int32* retValue);

javacall_result
javacall_dom_namednodemap_get_named_item_ns(javacall_handle handle,
                                            javacall_const_utf16_string namespaceURI,
                                            javacall_const_utf16_string localName,
                                            /* OUT */ javacall_handle* retValue);

javacall_result
javacall_dom_namednodemap_set_named_item_ns(javacall_handle handle,
                                            javacall_handle arg,
                                            /* OUT */ javacall_handle* retValue,
                                            /* OUT */ javacall_utf8_string exceptionValue);

javacall_result
javacall_dom_namednodemap_remove_named_item_ns(javacall_handle handle,
                                               javacall_const_utf16_string namespaceURI,
                                               javacall_const_utf16_string localName,
                                               /* OUT */ javacall_handle* retValue,
                                               /* OUT */ javacall_utf8_string exceptionValue);

