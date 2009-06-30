
/*
 *   
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

#ifndef _MIDP_STRING_H_
#define _MIDP_STRING_H_

#include <midp_global_status.h>

#include <midp_constants_data.h>
#include <pcsl_string.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Horizontal Tab - Unicode character 0x09. */
#define  HT   0x09

/** SPace - Unicode character 0x20. */
#define  SP   0x20

/**
 * Looks for a white space (SPACE and TAB).
 */
#define IS_WHITE_SPACE(c) ((*(c)==SP) || (*(c)==HT))

/**
 * Looks for a white space (SPACE and TAB).
 */
#define IS_CHAR_WHITE_SPACE(c) (((c)==SP) || ((c)==HT))

/*=========================================================================
 * MidpString handling utilities
 *=======================================================================*/
/** Represents a native VM Unicode string */
typedef struct _MidpString {
    /**
     * Length of the string, or NULL_LEN for a null string, or 
     * OUT_OF_MEM_LEN if an out of memory error during allocation, or
     * IO_ERROR_LEN if an error reading the string from storage.
     */
    jint len; 
    /** Data of the string as Java platform Unicode characters. */
    jchar* data;
} MidpString;

/** Null string constant. Do not free. */
extern const MidpString NULL_MIDP_STRING;
/** Empty string constant. Do not free. */
extern const MidpString EMPTY_MIDP_STRING;

/**
 * Returns an index of first occurrence of the character in the string.
 * 
 * @param line   MidpString to look in.
 * @param c      character to look for
 * @return index
 */
int midpStringIndexOf(MidpString line, jchar c);

/**
 * Removes white spaces from the end of a MidpString.
 * <p>
 * Trims by changing the length of the string.
 * 
 * @param line   MidpString to handle
 * @return Handled MidpString
 */
MidpString midpStringTrimFromEnd(MidpString line);

/**
 * Removes white spaces from the both ends of a MidpString.
 * <p>
 * Memory allocation done here, even if the result is the same as the input.
 * <p>
 * This function should not be used directly,
 * use the midpNewString macro.
 * 
 * @param line   MidpString to handle
 * @param filename provided by the midpStringTrim macro
 * @param line provided by the midpStringTrim macro
 *
 * @return new string
 */
MidpString midpStringTrimImpl(MidpString in, char* filename, int line);

/**
 * Converts MidpString representation of decimal integer to integer.
 * 
 * @param str    MidpString representation of integer
 * @return integer value of str
 */
int midpIntegerParseInt(MidpString str);

/**
 * Converts integer to MidpString in decimal format.
 * Memory allocation done here.
 * 
 * @param inputInt integer to convert
 * @return MidpString representation of the inputInt
 */
MidpString midpStringValueOfInt(int inputInt);

/**
 * Concatenates 2 Unicode strings into 1 string. Free the results data
 * field with midpFree.
 * This function should not be used directly,
 * use the midpStringCat macro.
 * 
 * @param str1 first string
 * @param str2 second string
 * @param filename provided by the midpStringCat macro
 * @param line provided by the midpStringCat macro
 *
 * @return result of first + second, len is OUT_OF_MEM_STR_LEN if out of memory
 */
MidpString midpStringCatImpl(MidpString str1, MidpString str2, char* filename,
                             int line);

/**
 * Compares 2 Unicode strings.
 * 
 * @param str1 first string
 * @param str2 second string
 *
 * @return 0 if str1 = str2, < 0 if str1 < str2, > 0 if str1 > str2
 */
int midpStringCmp(MidpString str1, MidpString str2);

/**
 * Check to see if 2 Unicode strings are equal.
 * 
 * @param str1 first string
 * @param str2 second string
 *
 * @return true if equal false if not
 */
int midpStringEquals(MidpString str1, MidpString str2);

/**
 * Check to see if one string ends with another.
 * 
 * @param str1 first string
 * @param str2 second string
 *
 * @return 1 if str1 ends with str2, else 0
 */
int midpStringEndsWith(MidpString str1, MidpString str2);

/**
 * Convert a jchar string to a C string.
 * This function should not be used directly,
 * use the midpJcharsToChars macro.
 *
 * @param in jchar string
 * @param filename provided by the midpJcharsToChars macro
 * @param line provided by the midpJcharsToChars macro
 *
 * @return C string or NULL
 */
char* midpJcharsToCharsImpl(MidpString in, char* filename, int line);

/**
 * Convert a C string to a jchar string.
 * This function should not be used directly,
 * use the midpCharsToJchars macro.
 *
 * @param in C string
 * @param filename provided by the midpCharsToJchars macro
 * @param line provided by the midpCharsToJchars macro
 *
 * @return jchar string
 */
MidpString midpCharsToJcharsImpl(char* in, char* filename, int line);

/**
 * Duplicates a Unicode string. Free the results data
 * field with midpFree.
 * This function should not be used directly,
 * use the midpStringDup macro.
 * 
 * @param in input string
 * @param filename provided by the midpStringDup macro
 * @param line provided by the midpStringDup macro
 *
 * @return copy of the input string
 */
MidpString midpStringDupImpl(MidpString in, char* filename, int line);

/**
 * Returns a new string that is a substring of this string. The
 * substring begins at the specified <code>beginIndex</code> and
 * extends to the character at index <code>endIndex - 1</code>.
 * Thus the length of the substring is <code>endIndex-beginIndex</code>.
 * <p>
 * Examples:
 * <blockquote><pre>
 * "hamburger".substring(4, 8) returns "urge"
 * "smiles".substring(1, 5) returns "mile"
 * </pre></blockquote>
 *
 * This function should not be used directly,
 * use the midpStringDup macro.
 *
 * @param in input string
 * @param beginIndex the beginning index, inclusive.
 * @param endIndex the ending index, exclusive.
 * @param filename provided by the midpSubString macro
 * @param line provided by the midpSubString macro
 *
 * @return     the specified substring, or a string with a length of
 *             OUT_OF_MEM_LEN.
 */
MidpString midpSubStringImpl(MidpString in, int beginIndex, int endIndex,
    char* filename, int line);

/**
 * Returns the index within this string of the last occurrence of the
 * specified character. That is, the index returned is the largest
 * value <i>k</i> such that:
 * <blockquote><pre>
 * string.data[<i>k</i>] == ch
 * </pre></blockquote>
 * is true.
 * The String is searched backwards starting at the last character.
 *
 * @param   str  Unicode string to search
 * @param   ch   character to search for
 * @return  the index of the last occurrence of the character in the
 *          character sequence represented by this object, or
 *          <code>-1</code> if the character does not occur.
 */
int midpStringLastIndexOf(MidpString str, jchar ch);

/**
 * Frees a MIDP string.
 *
 * @param str string to free
 */
void midpFreeString(MidpString str);

/**
 * Frees a string list.
 *
 * @param pStrings point to an array of suites
 * @param numberOfStrings number of elements in pStrings
 */
void freeStringList(MidpString* pStrings, int numberOfStrings);

void free_pcsl_string_list(pcsl_string* pStrings, int numberOfStrings);

/**
 * Allocate an array of pcsl_string's.
 * Initialize the strings with PCSL_STRING_NULL.
 *
 * @param numberOfStrings desired array size
 * @return address of the array, or NULL if out-of-memory error happened
 */
pcsl_string* alloc_pcsl_string_list(int numberOfStrings);

/**
 * Create pcsl_string from the specified MidpString.
 * The caller is responsible for freeing the created string when done.
 *
 * @param midpString pointer to the MidpString instance
 * @param pcslString pointer to the pcsl_string instance
 * @return status of the operation
 */
pcsl_string_status midp_string_to_pcsl_string(const MidpString * midpString,
					      pcsl_string * pcslString);

/**
 * Create MidpString from the specified pcsl_string.
 * The caller is responsible for freeing the created string when done.
 *
 * @param pcslString pointer to the pcsl_string instance
 * @param midpString pointer to the MidpString instance
 * @return status of the operation
 */
pcsl_string_status midp_string_from_pcsl_string(const pcsl_string * pcslString,
						MidpString * midpString);


pcsl_string_status midp_string_from_pcsl_string_hack(const pcsl_string * pcslString,
						MidpString * midpString);

/**
 * Allocate a buffer of jchars with the text from the specified pcsl_string.
 * The caller is responsible for freeing the buffer with midpFree when done.
 *
 * @param pcslString pointer to the pcsl_string instance
 * @param pBuf receives the allocated buffer
 * @param pBufLen receives the size (in jchars) of the buffer
 * @return status of the operation
 */
pcsl_string_status text_buffer_from_pcsl_string(const pcsl_string * pcslString,
						jchar * * pBuf, int * pBufLen);

/** Convenient macro to free and reset the fields of a MidpString. */
#define MIDP_FREE_STRING(X) \
{ \
   MidpString* temp = &(X); \
   midpFreeString(*temp); \
   temp->len = NULL_LEN; \
   temp->data = NULL; \
}

/** Wrapper Macros so string leaks can be traced. */

#if REPORT_LEVEL <= LOG_WARNING

#  define midpStringCat(x, y) midpStringCatImpl((x), (y), __FILE__, __LINE__)
#  define midpCharsToJchars(x) midpCharsToJcharsImpl((x), __FILE__, __LINE__)
#  define midpJcharsToChars(x) midpJcharsToCharsImpl((x), __FILE__, __LINE__)
#  define midpStringDup(x)  midpStringDupImpl((x), __FILE__, __LINE__)
#  define midpSubString(x, y, z)  midpSubStringImpl((x), (y), (z), __FILE__, \
                                 __LINE__)
#  define midpStringTrim(x)  midpStringTrimImpl((x), __FILE__, __LINE__)

#else

#  define midpStringCat(x, y) midpStringCatImpl((x), (y), NULL, 0)
#  define midpCharsToJchars(x) midpCharsToJcharsImpl((x), NULL, 0)
#  define midpJcharsToChars(x) midpJcharsToCharsImpl((x), NULL, 0)
#  define midpStringDup(x)  midpStringDupImpl((x), NULL, 0)
#  define midpSubString(x, y, z)  midpSubStringImpl((x), (y), (z), NULL, 0)
#  define midpStringTrim(x)  midpStringTrimImpl((x), NULL, 0)

#endif

#ifdef __cplusplus
}
#endif

#endif /* _MIDP_STRING_H_ */

