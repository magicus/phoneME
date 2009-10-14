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
 * Implementation of UTF8 string handling.
 */

#include <string.h>
#include "javautil_string.h"
#include "javacall_memory.h"
#include "javacall_string.h"
#include "javautil_unicode.h"

/**
 * looks for first occurrence of <param>c</param> within <param>str</param>
 *
 * @param str string to search in
 * @param c character to look for
 * @param index index of the first occurence of <param>c</param>
 * @return <code>JAVACALL_OK</code> on success,
 *         <code>JAVACALL_FAIL</code> or any other negative value otherwise.
 */
javacall_result javautil_string_index_of(const char* str, char c, /* OUT */ int* index) {
    int i=0;
    int len = strlen(str);

    *index = -1;

    if (len <= 0) {
        return JAVACALL_FAIL;
    }

    while ((i < len) && (*(str+i) != c)) {
        i++;
    }

    if (i == len) {
        return JAVACALL_FAIL;
    }

    *index = i;
    return JAVACALL_OK;
}

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
                                              /* OUT */ int* index) {
    int i;
    int len = strlen(str);

    *index = -1;
    i = len-1;

    if (len <= 0) {
        return JAVACALL_FAIL;
    }

    while ((i >= 0) && (*(str+i)  != c)) {
        i--;
    }

    if (i == -1) {
        return JAVACALL_FAIL;
    }

    *index = i;
    return JAVACALL_OK;
}

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
                                     const char* str2) {

    if (strcmp(str1, str2) == 0) {
        return JAVACALL_TRUE;
    }

    return JAVACALL_FALSE;
}

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
                                          /*OUT*/ char** dest) {

    char* result = NULL;
    int srcLen = strlen(src);
    int destLen;

    *dest = NULL;

    if ((src == NULL) || (srcLen < 0) || (begin < 0) ||
        (end > srcLen) || (begin >= end)) {
        return JAVACALL_FAIL;
    }

    if (srcLen == 0) {
        *dest = '\0';
        return JAVACALL_OK;
    }

    destLen = end - begin;
    // SHOULD USE pcsl_mem_malloc() ?
    result = (char*)javacall_malloc((destLen+1)*sizeof(char));
    if (result == NULL) {
        return JAVACALL_OUT_OF_MEMORY;
    }

    memcpy(result, src+begin, destLen);
    result[destLen] = '\0';
    *dest = result;

    return JAVACALL_OK;
}

/**
 * Remove white spaces from the end of a string
 *
 * @param str string to trim
 * @return <code>JAVACALL_OK</code> on success,
 *         <code>JAVACALL_FAIL</code> or any other negative value otherwise.
 */
javacall_result javautil_string_trim(char* str) {
    int len = strlen(str);

    if (len <= 0) {
        return JAVACALL_FAIL;
    }

    while (((*(str+len-1) == SPACE) || (*(str+len-1) == HTAB)) && (len > 0)) {
        *(str+len-1) = 0x00;
        len--;
    }

    return JAVACALL_OK;
}

/**
 * Converts a given string representation of decimal integer to integer.
 *
 * @param str string representation of integer
 * @param number the integer value of str
 * @return <code>JAVACALL_OK</code> on success,
 *         <code>JAVACALL_FAIL</code> or any other negative value otherwise.
 */
javacall_result javautil_string_parse_int(const char* str, int* number) {
    int res = 0;
    int td = 1;
    int len = strlen(str);
    char* p = (char*)str;

    *number = -1;

    if (len <= 0) {
        return JAVACALL_FAIL;
    }

    p += len-1;

    while (p >= str) {

        if ((*p >= '0') && (*p <= '9')) { /* range between 0 to 9 */
            res += ((*p)-'0')*td;
            td*=10;
        } else {
            return JAVACALL_FAIL;
        }
        p--;
    }

    *number = res;
    return JAVACALL_OK;
}

#define ISALFA(c) ('A' <= (c & ~0x20) && (c & ~0x20) <= 'Z')

/**
 * Compare characters of two strings without regard to case.
 *
 * @param string1, string2 null-terminated strings to compare
 * @param nchars the number of characters to compare
 * The return value indicates the relationship between the substrings as follows.
 *   < 0   string1 substring less than string2 substring
 *   0     string1 substring identical to string2 substring
 *   > 0   string1 substring greater than string2 substring
 */
int javautil_strnicmp(const char* string1, const char* string2, size_t nchars)
{
    unsigned char ch1, ch2;
    do {
        if (nchars-- == 0) {
            return 0;
        }
        ch1 = (unsigned char) *string1++;
        ch2 = (unsigned char) *string2++;

        if (ch1 != ch2) {
            if (((ch1 ^ ch2) != 0x20) || !ISALFA(ch1))  {
                break;
            }
        }
    }
    while (ch1 && ch2);
    return ch1 - ch2;
}

int javautil_stricmp(const char* string1, const char* string2)
{
    unsigned char ch1, ch2;
    do {
        ch1 = (unsigned char) *string1++;
        ch2 = (unsigned char) *string2++;

        if (ch1 != ch2) {
            if (((ch1 ^ ch2) != 0x20) || !ISALFA(ch1))  {
                break;
            }
        }
    }
    while (ch1 && ch2);
    return ch1 - ch2;
}

javacall_utf16_string javautil_wcsdup(javacall_const_utf16_string src) {
    javacall_int32 length = 0;
    javacall_utf16_string result;
    if( src == NULL ) {
        return NULL;
    }
    if (JAVACALL_OK != javautil_unicode_utf16_ulength (src, &length)) {
        length = 0;
    }
    result = javacall_malloc(((unsigned int)length + 1) * sizeof(javacall_utf16));
    memcpy(result, src, ((unsigned int)length + 1) * sizeof(javacall_utf16));
    return result;
}

size_t javautil_wcsncpy(javacall_utf16 * dst, javacall_const_utf16_string src, size_t nchars)
{
    int count = nchars;
    while( count-- ){
        *dst++ = *src;
        if( !*src++ ) 
            return nchars - count;
    }
    return nchars;
}

int javautil_wcsncmp(const javacall_const_utf16_string string1, const javacall_const_utf16_string string2, size_t nchars){
	javacall_utf16_string str1 = string1;
	javacall_utf16_string str2 = string2;
	++nchars;
	while (--nchars && *str1 && *str2 && *str1 == *str2) {++str1;++str2;}
	return (!nchars) ? 0 : *str1 - *str2;
}


int javautil_wcsnicmp(javacall_const_utf16_string string1, javacall_const_utf16_string string2, size_t nchars)
{
#define BUFFER_SIZE 0x20
    javacall_utf16 buff1[ BUFFER_SIZE ], buff2[ BUFFER_SIZE ];

    while( nchars ){
        size_t len = BUFFER_SIZE, count;
        const javacall_utf16 * p1 = buff1, * p2 = buff2;
        if( len > nchars ) len = nchars;
        javacall_towlower( buff1, javautil_wcsncpy( buff1, string1, len ) );
        javacall_towlower( buff2, javautil_wcsncpy( buff2, string2, len ) );

        for( count = len; count--; ){
            javacall_utf16 ch = *p1++;
            if( ch != *p2++ ) // strings are different
                return ch - *--p2;
            if( !ch ) // strings are equal (zero char is reached)
                return 0;
        }

        nchars -= len; 
        string1 += len; 
        string2 += len;
    }
#undef BUFFER_SIZE
    return 0;
}

/**
 * Returns a new string that is a concatenation of two input strings.
 * Memory allocated within this function by javacall_malloc() and should be freed
 * by javacall_free()
 *
 * @param prefix the beginning/prefix string
 * @param suffix the ending/suffix string
 * @return concatenated string on success, NULL otherwise.
 */
char* javautil_string_strcat(const char* prefix, const char* suffix) {
    char *joined_string = NULL;
    int len1 = 0;
    int len2 = 0;

    if ((prefix == NULL) || (suffix == NULL)) {
        return NULL;
    }

    len1 = strlen(prefix);
    len2 = strlen(suffix);

    joined_string = javacall_malloc((len1+len2+1));
    if (joined_string == NULL) {
        return NULL;
    }
    

    memcpy(joined_string, prefix, len1);
    memcpy(joined_string+len1, suffix, len2);
    joined_string[len1+len2] = '\0';

    return joined_string;
}

/**
 * Skip leading blanks
 * 
 * @param s input string
 * 
 * @return a pointer to the first non blank character inside "s"
 */
char* javautil_string_skip_leading_blanks(char* s) {

    if (s == NULL) {
        return NULL;
    }

    while (*s == ' ' || *s == '\t') {
        s++;
    }

    return s;
}

/**
 * Skip trailing blanks
 * 
 * @param s input string
 */
void javautil_string_skip_trailing_blanks(char * s) {
    int i;

    if (s == NULL) {
        return;
    }

    i = strlen(s) - 1;

    while (i >= 0 && (s[i] == ' ' || s[i] == '\t')) {
        i--;
    }

    s[i+1] = '\0';
}

/**
 * Skip blanks in the beginning and at the end of the string
 * 
 * @param s string to be stripped of whitespaces
 */
void javautil_string_strip(char* s) {
    char* pf;   /*forward pointer*/
    int i, length;

    /*check arguments*/
    if (s == NULL) {
        return;
    }

    pf = s;

    /*skip leading blanks*/
    while (*pf == ' ' || *pf == '\t') {
        pf++;
    }

    /*skip trailing blanks*/
    length = strlen(pf) - 1;

    while (length >= 0 && (pf[length] == ' ' || pf[length] == '\t')) {
        length--;
    }

    length++;

    for (i = 0; i < length; i++) {
        s[i] = pf[i];
    }

    s[i] = '\0';
}

/**
 * Duplicates a string
 * 
 * @param s input string
 * @return a newly allocated string with the same content as s
 */
char* javautil_string_duplicate(const char *s) {
    int len;
    char *new_s;

    if (NULL == s){    
        return NULL;
    }
        
    len = strlen(s);
    new_s = javacall_malloc(len+1);
    if (NULL==new_s){
        return NULL;
    }

    strcpy(new_s, s);
    return new_s;
}
