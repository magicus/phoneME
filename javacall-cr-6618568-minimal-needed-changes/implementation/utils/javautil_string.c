/*
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

#include "javautil_string.h"
#include "javacall_memory.h"
#include <string.h>

#define MAX_STR_LEN	1024

#ifndef min
  #define min(x,y)        (x > y ? y : x)
#endif

/** Horizontal Tab - Unicode character 0x09. */
#define  HT   0x09

/**
 * Looks for a white space (SPACE and TAB).
 */
#define IS_CONTENT_A_WHITE_SPACE(c) ((*(c)==SP) || (*(c)==HT))

/**
 * Looks for a white space (SPACE and TAB).
 */
//#define IS_CHAR_A_WHITE_SPACE(c) (((c)==SP) || ((c)==HT))

/** SPace - Unicode character 0x20. */
#define  SP   0x20

/**
 * Verify that pointer is not NULL
 */
#define CONTENT_NOT_NULL(c) (*(c))

/**
 * Check if pointer is a space
 */
#define IS_SPACE(c)   (CONTENT_NOT_NULL(c) && (IS_CONTENT_A_WHITE_SPACE(c)))

/**
 * looks for first occurrence of <param>c</param> within <param>str</param>
 *
 * @param str string to search in
 * @param c character to look for
 * @param index index of the first occurence of <param>c</param>
 * @return <code>JAVACALL_OK</code> on success,
 *         <code>JAVACALL_FAIL</code> or any other negative value otherwise.
 */
javacall_result javautil_string_index_of(char* str, char c, /* OUT */ int* index) {
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
javacall_result javautil_string_last_index_of(char* str, char c,
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
javacall_bool javautil_string_equals(char* str1, char* str2) {

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
javacall_result javautil_string_substring(char* src, int begin, int end,
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
javacall_result javautil_string_parse_int(char* str, int* number) {
    int res = 0;
    int td = 1;
    int len = strlen(str);
    char* p = str;

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

#define ISALFA(c) ((((c) > 0x40) && ((c) < 0x5B)) || (((c) > 0x60) && ((c) < 0x7B)))

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
int javautil_strnicmp(const char* string1, const char* string2, unsigned int nchars)
{
    unsigned char ch1, ch2;
    do
    {
        if (nchars-- == 0) {
            return 0;
        }
        ch1 = (unsigned char) *string1++;
        ch2 = (unsigned char) *string2++;

        if (ch1 != ch2)
        {
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
    do
    {
        ch1 = (unsigned char) *string1++;
        ch2 = (unsigned char) *string2++;

        if (ch1 != ch2)
        {
            if (((ch1 ^ ch2) != 0x20) || !ISALFA(ch1))  {
                break;
            }
        }
    }
    while (ch1 && ch2);
    return ch1 - ch2;
}

int javautil_wcsnicmp(const unsigned short* string1, const unsigned short* string2, unsigned int nchars)
{
    unsigned short ch1, ch2;
    do
    {
        if (nchars-- == 0) {
            return 0;
        }
        ch1 = *string1++;
        ch2 = *string2++;

        if (ch1 != ch2)
        {
            if (((ch1 ^ ch2) != 0x20) || !ISALFA(ch1))  {
                break;
            }
        }
    }
    while (ch1 && ch2);
    return ch1 - ch2;
}

/**
 * Returns a new string that is a concantenation of two input strings.
 * Memory allocated within this function by javacall_malloc() and should be freed
 * by javacall_free()
 *
 * @param prefix the beginning/prefix string
 * @param suffix the ending/suffix string
 * @return <code>Concantinated string</code> on success,
 *         <code>NULL</code> or any other negative value otherwise.
 */
char* javautil_string_concatenate(const char* prefix, const char* suffix) {
    char *joined_string = NULL;
    int len1 = 0;
    int len2 = 0;

    if((prefix == NULL) || (suffix == NULL)) {
        return NULL;
    }

    len1 = strlen(prefix);
    len2 = strlen(suffix);

    joined_string = javacall_malloc((len1+len2+1));
    if(joined_string == NULL) {
        return NULL;
    }
    memset(joined_string, 0, (len1+len2+1));

    memcpy(joined_string,prefix,len1);
    memcpy(joined_string+len1,suffix,len2);

    return joined_string;
}

/**
 * Convert a string to lower-case
 * 
 * @param s input string
 * @return pointer to statically allocated string holding the "s" string converted
 *		   to lower case.
 */
char* javautil_str_tolwc(char * s) {
    static char l[MAX_STR_LEN+1];
    int i ;

    if(s==NULL) return NULL ;
    memset(l, 0, MAX_STR_LEN+1);
    i=0 ;
    while(s[i] && i<MAX_STR_LEN) {
        l[i] = (char)tolower((int)s[i]);
        i++ ;
    }
    l[MAX_STR_LEN]=(char)0;
    return l ;
}



/**
 * Convert a string to upper-case
 * 
 * @param s input string
 * @return pointer to statically allocated string holding the "s" string converted
 *		   to upper case.
 */
char* javautil_str_toupc(char * s) {
    static char l[MAX_STR_LEN+1];
    int i ;

    if(s==NULL) return NULL ;
    memset(l, 0, MAX_STR_LEN+1);
    i=0 ;
    while(s[i] && i<MAX_STR_LEN) {
        l[i] = (char)toupper((int)s[i]);
        i++ ;
    }
    l[MAX_STR_LEN]=(char)0;
    return l ;
}



/**
 * Skip leading blanks
 * 
 * @param s input string
 * @return a pointer to the first non blank character inside "s"
 */
char* javautil_str_skip_leading_blanks(char * s) {
    char * skip = s;
    if(s==NULL) {
        return NULL ;
    }

    //while(isspace((int)*skip) && *skip)
    while(IS_SPACE(skip)) {
        skip++;
    }
    return skip ;
} 



/**
 * Skip trailing blanks
 * 
 * @param s input string
 * @return a pointer to a STATICALLY ALLOCATED string containg the same string as "s"
 *			but without the trailing spaces.
 */
char* javautil_str_skip_trailing_blanks(char * s) {
    static char l[MAX_STR_LEN+1];
    char * last ;
    int len, copy_len;

    if(s==NULL) return NULL ;
    memset(l, 0, MAX_STR_LEN+1);
    len = strlen(s);
    copy_len = min(len,MAX_STR_LEN-1);
    strncpy(l, s, copy_len+1);
    l[copy_len] = '\0';
    last = l + copy_len;
    while(last > l) {
        //if(!isspace((int)*(last-1)))
        if(!IS_SPACE((last-1))) {
            break ;
        }
        last -- ;
    }
    *last = (char)0;
    return l ;
}



/**
 * Skip blanks in the beginning and at the end of the string
 * 
 * @param s input string
 * @return a pointer to a STATICALLY ALLOCATED string containg the same string as "s"
 *			but without the leading and trailing spaces.
 */
char* javautil_str_strip(char * s) {
    static char l[MAX_STR_LEN+1];
    char * last ;
    int len, copy_len;

    if(s==NULL) return NULL ;

    /* skip leading blanks */
    //while(isspace((int)*s) && *s) s++;
    while(IS_SPACE(s)) {
        s++;
    }

    /* skip trailing blanks */
    memset(l, 0, MAX_STR_LEN+1);
    len = strlen(s);
    copy_len = min(len,MAX_STR_LEN-1);
    strncpy(l, s, copy_len+1);
    last = l + copy_len;
    while(last > l) {
        //if(!isspace((int)*(last-1)))
        if(!IS_SPACE((last-1))) {
            break ;
        }
        last -- ;
    }
    *last = (char)0;

    return(char*)l ;
}

char* javautil_str_duplicate(char *s) {
    int len;
    char *new_s;

    if(NULL == s)
        return NULL;
    len = strlen(s);
    new_s = javacall_malloc(len+1);
    if(NULL==new_s)
        return NULL;
    strcpy(new_s, s);
    return new_s;
}

#ifdef __cplusplus
}
#endif

