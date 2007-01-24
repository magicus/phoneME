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

#ifndef __javacall_mi18n_collation_h
#define __javacall_mi18n_collation_h
/**
 * @defgroup JSR238 JSR238 Mobile Internationalization API (MI18N)
 * @ingroup MSA
 * 
 * Porting interface for native implementation Mobile Internationalization API.
 * 
 * @{
 */
/** @} */

/**
 * @file javacall_mi18n_collation.h
 * @ingroup JSR238
 * @brief JSR238 Mobile Internationalization API (MI18N)
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "javacall_defs.h" 

#define NEUTRAL_LOCALE_INDEX 0
#define COMPARE_ERROR -1000

/**
 * @defgroup jsrMandatoryCollation Low-level collation porting API
 * @ingroup JSR238
 * 
 * Porting interface for native implementation of string comparison functionality.
 * 
 * @{
 */

/**
 * Constant for the comparison level that takes all differences between characters into account.
 */
#define COLLATION_IDENTICAL 15

/**
 * Constant for the primary collation level. For European languages this 
 * level honours differences between alphabetical characters at the language level.
 */
#define COLLATION_LEVEL1 1

/**
 * Constant for the secondary collation level. For European languages this
 * level honours differences in normal and accented versions of the same character.
 */
#define COLLATION_LEVEL2 2

/**
 * Constant for the tertiary collation level. For European languages this 
 * level honours differences in character case.
 */
#define COLLATION_LEVEL3 3

/**
 * Gets the number of supported locales for string collation.
 * @param count_out pointer to integer that recieves
 *					the number of supported locales
 * @return JAVACALL_OK if all done successfuly, 
 *         error code otherwise
 */
javacall_result javacall_mi18n_get_collation_locales_count(/*OUT*/int* count_out);


/**
 * Gets a locale name for collation for the index.
 *
 * @param locale_index  index of the locale.
 * @param locale_name_out  buffer for the locale.
 * @param plen	pointer to integer initially containing the buffer length
 *				and receiving length of result string in wchars including terminating zero, 
 * @return JAVACALL_OK if all done successfuly, 
 *         error code otherwise
 * @note Zero index (neutral locale) must return name - empty string
 */
javacall_result javacall_mi18n_get_collation_locale_name(int locale_index, javacall_utf16* locale_name_out,
														 /*IN|OUT*/int* plen);


/**
 * Gets locale index used for collation formatting by the given miroedition.locale name.
 *
 * @param locale    utf16 string containing requested locale name or null for neutral locale
 * @param index_out	pointer to integer receiving index of requested locale,
 * @return JAVACALL_OK if all done successfuly, 
 *         error code otherwise
 * @note Neutral (empty string) locale must be supported. It must have index 0
 */
javacall_result javacall_mi18n_get_collation_locale_index(const javacall_utf16* locale, /*OUT*/int* index_out);



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
 *         error code otherwise
 */
javacall_result javacall_mi18n_compare_strings(int locale_index, 
								   const javacall_utf16* s1, int len1,
								   const javacall_utf16* s2, int len2,
								   int level,
								   /*OUT*/int* result_out);

/** @} */


#ifdef __cplusplus
}
#endif

#endif //__javacall_mi18n_collation_h
