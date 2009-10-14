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
 * @file
 *
 * Interface for UTF8 string handling.
 */

#ifndef _JAVAUTIL_STRING_H_
#define _JAVAUTIL_STRING_H_

#include <string.h>
#include "javacall_defs.h"

#ifdef __cplusplus
extern "C" {
#endif 

#define COLON    0x3A    /* Colon ':' - Unicode character 0x3A */
#define CR       0x0D    /* Carriage Return - Unicode character 0x0D */
#define LF       0x0A    /* Line Feed - Unicode character 0x0A */
#define SPACE    0x20    /* Space - Unicode character 0x20 */
#define HTAB     0x09    /* Horizontal TAB - Unicode character 0x09 */
#define POUND    0x23    /* '#' - Unicode character 0x23 */

/**
 * looks for first occurrence of <param>c</param> within <param>str</param>
 *
 * @param str string to search in
 * @param c character to look for
 * @param index index of the first occurence of <param>c</param>
 * @return <code>JAVACALL_OK</code> on success,
 *         <code>JAVACALL_FAIL</code> or any other negative value otherwise.
 */
javacall_result javautil_string_index_of(const char* str, 
                                         char c, /* OUT */ int* index);

/**
 * Looks for the last occurence of <param>c</param> within <param>str</param>
 *
 * @param str string to search in
 * @param c character to look for
 * @param index index of the first occurence of <param>c</param>
 * @return <code>JAVACALL_OK</code> on success,
 *         <code>JAVACALL_FAIL</code> or any other negative value otherwise.
 */
javacall_result javautil_string_last_index_of(const char* str, char c,
                                              /* OUT */ int* index);

/**
 * Check to see if two strings are equal.
 *
 * @param str1 first string
 * @param str2 second string
 *
 * @return <code>JAVACALL_TRUE</code> if equal,
 *         <code>JAVACALL_FALSE</code> otherwise.
 */
javacall_bool javautil_string_equals(const char* str1, 
                                     const char* str2);

/**
 * Returns a new string that is a substring of this string. The
 * substring begins at the specified <code>beginIndex</code> and
 * extends to the character at index <code>endIndex - 1</code>.
 * Thus the length of the substring is <code>endIndex-beginIndex</code>.
 *
 * @param src input string
 * @param begin the beginning index, inclusive.
 * @param end the ending index, exclusive.
 * @param dest the output string, will contain the specified substring
 * @return <code>JAVACALL_OK</code> on success,
 *         <code>JAVACALL_FAIL</code> or any other negative value otherwise.
 */
javacall_result javautil_string_substring(const char* src, int begin, int end,
                                          /*OUT*/ char** dest);

/**
 * Remove white spaces from the end of a string
 *
 * @param str string to trim
 * @return <code>JAVACALL_OK</code> on success,
 *         <code>JAVACALL_FAIL</code> or any other negative value otherwise.
 */
javacall_result javautil_string_trim(char* str);

/**
 * Converts a given string representation of decimal integer to integer.
 *
 * @param str string representation of integer
 * @param number the integer value of str
 * @return <code>JAVACALL_OK</code> on success,
 *         <code>JAVACALL_FAIL</code> or any other negative value otherwise.
 */
javacall_result javautil_string_parse_int(const char* str, int* number);

/*
 * Copies source javacall_utf16_string string into newly allocated memory buffer
 * @param src source javacall_utf16_string
 * @return pointer to newly allocated memory containing copy of source string
 */
javacall_utf16_string javautil_wcsdup(javacall_const_utf16_string src) ;

/**
 * Copies at most n wide characters from the wide-character string
 * pointed to by src, including the terminating L'\0' character, to the array
 * pointed to by dest.  Exactly n wide characters are written at dest.  If the length
 * wcslen(src) is greater or equal to n, the string pointed to by dest will not
 * be L'\0' terminated.
 *
 * The strings may not overlap.
 * 
 * @param dst destination buffer. At least nchars wide-characters long.
 * @param src copied string
 * @param nchars max number of copied wide-characters
 * @return number of copied wide-characters
 */
size_t javautil_wcsncpy(javacall_utf16 * dst, javacall_const_utf16_string src, size_t nchars);

/**
 * Compare characters of two strings without regard to case.
 *     javautil_strnicmp,
 *     javautil_stricmp,
 *     javautil_wcsnicmp,
 *     javautil_wcsicmp
 *
 * @param string1, string2 null-terminated strings to compare
 * @param nchars the number of characters to compare
 * @return integer value indicates the relationship between the substrings as follows.
 *   < 0   string1 less than string2
 *   0     string1 identical to string2
 *   > 0   string1 greater than string2
 */
int javautil_strnicmp(const char* string1, const char* string2, size_t nchars);
int javautil_stricmp(const char* string1, const char* string2);
int javautil_wcsnicmp(javacall_const_utf16_string string1, javacall_const_utf16_string string2, size_t nchars);
	
/**
* Compare characters of two strings.
*     javautil_wcsncmp
*
* @param string1, string2 null-terminated strings to compare
* @param nchars the number of characters to compare
* @return integer value indicates the relationship between the substrings as follows.
*   < 0   string1 less than string2
*   0     string1 identical to string2
*   > 0   string1 greater than string2
*/	
int javautil_wcsncmp(javacall_const_utf16_string str1, javacall_const_utf16_string str2, size_t nchars);	

/**
 * Returns a new string that is a concatenation of two input strings.
 * Memory allocated within this function by javacall_malloc() and should be freed
 * by javacall_free()
 *
 * @param prefix the beginning/prefix string
 * @param suffix the ending/suffix string
 * @return concatenated string on success,
 *         NULL or any other negative value otherwise.
 */
char* javautil_string_strcat(const char* prefix, const char* suffix);

/**
 * Skip leading blanks
 * 
 * @param s input string
 * 
 * @return a pointer to the first non blank character inside "s"
 */
char* javautil_string_skip_leading_blanks(char* s);

/**
 * Skip trailing blanks
 * 
 * @param s input string
 */
void javautil_string_skip_trailing_blanks(char * s);

/**
 * Skip blanks in the beginning and at the end of the string
 * 
 * @param s string to be stripped of whitespaces
 */
void javautil_string_strip(char * s);

/**
 * Duplicates a string
 * 
 * @param s input string
 * @return a newly allocated string with the same content as s
 */
char* javautil_string_duplicate(const char *s);

#ifdef __cplusplus
}
#endif

#endif
