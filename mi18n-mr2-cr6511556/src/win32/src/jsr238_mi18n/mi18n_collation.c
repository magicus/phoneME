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

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif

#ifndef WINVER
#define WINVER 0x0500
#endif

#ifndef UNICODE
#define UNICODE
#endif

#include "windows.h"
#include <stdlib.h>
#include "javacall_mi18n_collation.h"
#include "mi18n_locales.h"


#define COMPARATOR_LOCALES_SET  INCLUDE_SORTING_VARIANTS

/*********************************************** API Implementation ****************************************************************/

/**
 * Gets the number of supported locales for string collation.
 * @param count_out pointer to integer that recieves
 *					the number of supported locales
 * @return JAVACALL_OK if all done successfuly, 
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_mi18n_get_collation_locales_count(/*OUT*/int* count_out){
	if (!count_out) return JAVACALL_FAIL;
	*count_out = i18n_get_supported_locales_count(COMPARATOR_LOCALES_SET);
	if (!*count_out) return JAVACALL_FAIL;
    return JAVACALL_OK;
}

/**
 * Gets a locale name for collation for the index.
 *
 * @param locale_index  index of the locale.
 * @param locale_name_out  buffer for the locale.
 * @param plen	pointer to integer initially containing the buffer length
 *				and receiving length of result string in wchars including terminating zero, 
 * @return JAVACALL_OK if all done successfuly, 
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_mi18n_get_collation_locale_name(int locale_index, javacall_utf16* locale_name_out,
														 /*IN|OUT*/int* plen){
    return mi18n_get_locale_name(locale_index, COMPARATOR_LOCALES_SET, locale_name_out, plen);
}

/**
 * Gets locale index used for collation formatting by the given miroedition.locale name.
 *
 * @param locale    utf16 string containing requested locale name or null for neutral locale
 * @param index_out	pointer to integer receiving index of requested locale,
 * @return JAVACALL_OK if all done successfuly, 
 *         JAVACALL_FAIL otherwise
 * @note If neutral (empty string) locale is supported it must have index 0
 */
javacall_result javacall_mi18n_get_collation_locale_index(const javacall_utf16* locale, /*OUT*/int* index_out){
	if (!index_out) return JAVACALL_INVALID_ARGUMENT;
	*index_out = mi18n_get_locale_index(locale,COMPARATOR_LOCALES_SET);
	if (*index_out<0) return JAVACALL_FAIL;
	return JAVACALL_OK;
}

/**
 * Compare two strings.
 *
 * @param locale_index  index of the locale.
 *						NEUTRAL_LOCALE_INDEX (0) for neutral locale
 * @param s1            first utf16 string to compare.
 * @param len1          length of the the first string.
 * @param s2            second utf16 string to compare.
 * @param len2          length of the second string.
 * @param level         level of collation:
 *							1. alphabetic ordering
 *							2. diacritic ordering
 *							3. case ordering
 *							15. identical comparision
 * @param result_out	pointer to integer receiving comparision result: 
						negative if s1 belongs before s2,
						0 if the strings are equal, 
 *						positive if s1 belongs after s2.
 * @return JAVACALL_OK if all done successfuly, 
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_mi18n_compare_strings(int locale_index, 
								   const javacall_utf16* s1, int len1,
								   const javacall_utf16* s2, int len2,
								   int level,
								   /*OUT*/int* result_out)
{
    LCID lcid;
    int res;
    DWORD dwFlags = 0;

	if (!result_out || !s1 || !s2 || len1<0 || len2<0) return JAVACALL_INVALID_ARGUMENT;

    if (locale_index == NEUTRAL_LOCALE_INDEX) {
        lcid = LOCALE_NEUTRAL;
    } else {
        lcid = mi18n_get_locale_id(locale_index,COMPARATOR_LOCALES_SET);
		if (lcid==-1) return JAVACALL_FAIL;
    }

    if (COLLATION_LEVEL1 == level) {
        dwFlags = NORM_IGNORECASE|NORM_IGNORENONSPACE|NORM_IGNOREWIDTH|NORM_IGNORESYMBOLS|SORT_STRINGSORT;
    } else
		if (COLLATION_LEVEL2 == level) {
        dwFlags = NORM_IGNORECASE|NORM_IGNOREWIDTH|NORM_IGNORESYMBOLS|SORT_STRINGSORT;
    } else
		if (COLLATION_LEVEL3 == level) {
		dwFlags = NORM_IGNOREWIDTH|NORM_IGNORESYMBOLS|SORT_STRINGSORT;
	} else 
		if (COLLATION_IDENTICAL == level) {
		dwFlags = SORT_STRINGSORT;
	} else {
		return JAVACALL_INVALID_ARGUMENT;
	}


    res = CompareStringW(lcid, dwFlags, (LPWSTR)s1, (DWORD)len1, (LPWSTR)s2, (DWORD)len2);

	if (!res) return JAVACALL_OK;

	*result_out = res - 2;

	return JAVACALL_OK;

}

#ifdef __cplusplus
}
#endif
