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



/**
 * @file
 *
 * win32 implemenation for public keystore handling functions
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "javacall_security.h" 
#include "javacall_memory.h" 


int permissions_load_domain_list(char** array) {
    return javacall_permissions_load_domain_list((javacall_utf8_string*)array);
}
int permissions_load_group_list(char** array) {
    return javacall_permissions_load_group_list((javacall_utf8_string*)array);
}

int permissions_load_group_permissions(char** array, char* group_name) {
    return javacall_permissions_load_group_permissions((javacall_utf8_string*)array,
                                            (javacall_utf8_string)group_name);
}

int permissions_get_default_value(char* domain_name, char* group_name) {
    return javacall_permissions_get_default_value((javacall_utf8_string)domain_name,
                                      (javacall_utf8_string) group_name);
}

int permissions_get_max_value(char* domain_name, char* group_name) {
    return javacall_permissions_get_max_value((javacall_utf8_string)domain_name,
                                  (javacall_utf8_string)group_name);
}

int permissions_load_group_messages(char** array, char* group_name) {
    return javacall_permissions_load_group_messages((javacall_utf8_string*)array,
                                        (javacall_utf8_string)group_name);
}

void permissions_dealloc(void* array) {
    javacall_free(array);
}

void permissions_loading_finished() {
    javacall_permissions_loading_finished();
}

#ifdef __cplusplus
}
#endif


