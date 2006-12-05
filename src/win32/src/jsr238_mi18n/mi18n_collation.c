/*
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

#ifdef __cplusplus
extern "C" {
#endif

#include "javacall_defs.h" 
#include "javacall_mi18n_collation.h"
#include "windows.h"
#include "stdlib.h"

/*
Implementation of low-level porting API for JSR 238 (MI18N).
*/

// platform specific locale operation part
int mi18n_enum_locales();
LCID mi18n_get_locale_id(int index);
javacall_result mi18n_get_locale_name(char* pBuff, int bufLen, int index);

/**
 *  The index for neutral locale. It has an value of the number of platform supported locales.
 */
static int local_neutral_id;

/**
 * Gets the number of supported locales for string collation.
 *
 * @return the number of supported locales or 0 if something is wrong
 */
int javacall_mi18n_get_collation_locales_count() {
    local_neutral_id = mi18n_enum_locales();
    return local_neutral_id + 1;
}

/**
 * Gets a locale name for string collation for the index.
 *
 * @param loc    buffer for the locale
 * @param len    buffer length
 * @param index  index of the locale
 * @return JAVACALL_OK on success, JAVACALL_FAIL otherwise
 */
javacall_result javacall_mi18n_get_collation_locale(char* loc, int len, int index) {
    if (index == local_neutral_id) {
        memset(loc, 0, len);
        return JAVACALL_OK;
    }
    return mi18n_get_locale_name(loc, len, index);
}

/**
 * Compare two strings.
 *
 * @param s1            first unicode string to compare
 * @param len1          length of the the first string
 * @param s2            second unicode string to compare
 * @param len2          length of the second string
 * @param locale_index  index of the locale
 * @param level         level of collation
 * @return negative if s1 belongs before s2, 0 if the strings are equal, positive if s1 belongs after s2
 */
int javacall_mi18n_compare_strings(char* s1, int len1, char* s2, 
                                   int len2, int locale_index, int level) {
    LCID locID;
    int res;
    DWORD dwFlags = 0;

    if (locale_index == local_neutral_id) {
        locID = LOCALE_NEUTRAL;
    } else {
        locID = mi18n_get_locale_id(locale_index);
    }

    if (COLLATION_LEVEL1 == level) {
        dwFlags = NORM_IGNORECASE|NORM_IGNORENONSPACE|NORM_IGNOREWIDTH;
    } else if (COLLATION_LEVEL2 == level) {
        dwFlags = NORM_IGNORECASE|NORM_IGNOREWIDTH;
    } 
    res = CompareStringW(locID, dwFlags, (LPWSTR)s1, (DWORD)len1, (LPWSTR)s2, (DWORD)len2);
    if (CSTR_LESS_THAN == res) {
        return -1;
    } else if (CSTR_EQUAL == res) {
        return 0;
    } else {
        return 1;
    }
}

#ifdef __cplusplus
}
#endif
