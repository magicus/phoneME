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
 * @ingroup stack
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
 *
 * @return the number of supported locales or 0 if something is wrong.
 */
int javacall_mi18n_get_collation_locales_count();

/**
 * Gets a locale name for string collation for the index.
 *
 * @param loc    buffer for the locale.
 * @param len    buffer length
 * @param index  index of the locale.
 * @return JAVACALL_OK if all done successfuly, 
 *         JAVACALL_FAIL otherwise
 */
javacall_result javacall_mi18n_get_collation_locale(char* loc, int len, int index);

/**
 * Compare two strings.
 *
 * @param s1            first unicode string to compare.
 * @param len1          length of the the first string.
 * @param s2            second unicode string to compare.
 * @param len2          length of the second string.
 * @param locale_index  index of the locale.
 * @param level         level of collation.
 * @return negative if s1 belongs before s2, 0 if the strings are equal, 
 *         positive if s1 belongs after s2.
 */
int javacall_mi18n_compare_strings(char* s1, int len1, char* s2, 
                                   int len2, int locale_index, int level);

/** @} */


#ifdef __cplusplus
}
#endif

#endif //__javacall_mi18n_collation_h
