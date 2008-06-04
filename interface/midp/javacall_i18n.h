/*
 *
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
 
#ifndef __JAVACALL_I18N_H_
#define __JAVACALL_I18N_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <javacall_defs.h>

/**
 * Check if native platform has resources for string data
 *
 * @return JAVACALL_TRUE if resource data is exist or JAVACALL_FALSE if not
 */
javacall_bool javacall_i18n_check_native_resouce();

/**
 * Get string data from native platform resource data
 *
 * @param index     Index of string
 * @param str       Return of string. It should be pointer to constant memory.
 * @param length    Length of str
 *
 * @return JAVACALL_OK or JAVACALL_FAIL
 */
javacall_result javacall_i18n_get_native_string(int index, 
        javacall_utf16** str, int* length);

#ifdef __cplusplus
}
#endif

#endif
