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

#ifndef _PCSL_STRING_H_
#define _PCSL_STRING_H_

#include <java_types.h>
#include <pcsl_string_status.h>
#include <pcsl_string_md.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup string String Manipulation Interface
 */

/**
 * @file
 * @ingroup string
 */

/**
 * @addtogroup string
 * @brief Interface for handling string operations.
 *
 * @{
 *
 * This file defines the API for common string operations, like
 * conversion, comparison, concatenation etc.
 * In order to accomodate different implementations, the data type
 * pcsl_string that represents strings is opaque, i.e. the calling routine need
 * have no knowledge of its actual definition. The calling routines just pass
 * these data structures around.
 * <p>
 * This flexibility in PCSL string library interface allows to select
 * the best reprentation for a particular platform. For example, for Win32
 * platforms it would be reasonable to have UTF-16 representation as we have
 * wide-versions of Win32 API functions and thus we can avoid conversion to UTF-8
 * at all. On platforms, that don't support 16-bit characters it might be
 * reasonable to select UTF8 internal representation to avoid repeated
 * conversions of the same PCSL string to UTF8. It is also possible to have an
 * internal PCSL string representation that caches different encodings of the
 * same string to achieve maximum performance.
 * <p>
 * The interface is designed to fully support Unicode 4.0 standard.
 * In particular, the implementation is required to support strings with
 * arbitrary unicode characters, including supplementary characters
 * (see http://www.unicode.org/glossary/#supplementary_character) that
 * cannot be encoded with UCS2.
 */

/**
 * CONCURRENCY: not multithread safe
 */

/**
 * Opaque type that represents strings in PCSL.
 * The definition of this type is implementation-dependent.
 * It is required that filling a pcsl_string with zeroes
 * sets the string to PCSL_STRING_NULL, for example,
 * <pre>
 * pcsl_string my_pcsl_string;
 * memset(&my_pcsl_string, 0, sizeof(my_pcsl_string));
 * </pre>
 * is equivalent to
 * <pre>
 * pcsl_string my_pcsl_string = PCSL_STRING_NULL_INITIALIZER;
 * </pre>
 */
typedef pcsl_string_md pcsl_string;

/**
 * Define a pcsl_string that corresponds to ASCII literal.
 *
 * For an ASCII literal definition:
 * <pre>
 *  const char * const literal_name = "literal_value";
 * </pre>
 * the corresponding pcsl_string definition will be:
 * <pre>
 * PCSL_DEFINE_ASCII_STRING_LITERAL_START(literal_name)
 *   {'l','i','t','e','r','a','l','_','v','a','l','u','e','\\0'}
 * PCSL_DEFINE_ASCII_STRING_LITERAL_END(literal_name);
 * </pre>
 * NOTE: the string must be explicitly terminated with '\\0'.
 * The literal must not have extra trailing zeroes.
 */
#define PCSL_DEFINE_ASCII_STRING_LITERAL_START(literal_name) \
  PCSL_DEFINE_ASCII_STRING_LITERAL_START_MD(literal_name)

/** End of ASCII literal string macro. */
#define PCSL_DEFINE_ASCII_STRING_LITERAL_END(literal_name) \
  PCSL_DEFINE_ASCII_STRING_LITERAL_END_MD(literal_name)

/**
 * Obtain the length of a string literal created with
 * one of PCSL_DEFINE_..._STRING_LITERAL_START/END macros.
 * The behavoir is undefined when this macro is applied not to a literal
 * created with PCSL_DEFINE_... macros (in particular, depending on
 * implementation, there may be or not be a compile-time error).
 */
#define PCSL_STRING_LITERAL_LENGTH(literal_name) \
  PCSL_STRING_LITERAL_LENGTH_MD(literal_name)

/**
 * Define a static pcsl_string that corresponds to ASCII literal.
 *
 * For an ASCII literal definition:
 * <pre>
 *  static const char * const literal_name = "literal_value";
 * </pre>
 * the corresponding pcsl_string definition will be:
 * <pre>
 * PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(literal_name)
 *   {'l','i','t','e','r','a','l','_','v','a','l','u','e','\\0'}
 * PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(literal_name);
 * </pre>
 * NOTE: the string must be explicitly terminated with '\\0'.
 * The literal must not have extra trailing zeroes.
 */
#define PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(literal_name) \
  PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START_MD(literal_name)

/** End of static ASCII literal string macro. */
#define PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(literal_name) \
  PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END_MD(literal_name)

/**
 * NULL initializer for pcsl_string variables.
 * <p>
 * For a plain char* initialization:
 * <pre>
 *  char * str = NULL;
 * </pre>
 * the corresponding pcsl_string initialization will be:
 * <pre>
 *  pcsl_string str = PCSL_STRING_NULL_INITIALIZER;
 * </pre>
 */
#define PCSL_STRING_NULL_INITIALIZER PCSL_STRING_NULL_INITIALIZER_MD

/**
 * This macro us used to determine size of the result buffer for passing it to
 * unicode_to_escaped_ascii function.
 * Namely it multiplies given length by 5 and adds one (the worst-case is:
 * one escape character plus 4 radix16 digits per digit, plus one more escape character).
 */
#define PCSL_STRING_ESCAPED_BUFFER_SIZE(length) ((length) * 5 + 1)

/**
 * Returns whether the string system is active.
 * The string subsystem is active after it is initialized and
 * before it is finalized.
 *
 * @return PCSL_TRUE if the string system is active,
 *         PCSL_FALSE otherwise
 */
jboolean pcsl_string_is_active(void);

/**
 * Performs platform-specific initialization of the string system.
 * This function must be invoked before any other functions of
 * the string subsystem.
 * Has no effect if the string system is active.
 *
 * @return initialization status
 */
pcsl_string_status pcsl_string_initialize(void);

/**
 * Performs platform-specific finalization of the string system.
 * No functions of the string subsystem should be used after
 * the string system is finalized.
 * Has no effect if the string system is not active.
 *
 * @return finalization status
 */
pcsl_string_status pcsl_string_finalize();

/**
 * Returns the number of abstract characters in the specified string,
 * not including the terminating zero character.
 * Returns a negative value if str is NULL or doesn't represent
 * a valid abstract character sequence.
 * See Unicode Glossary at http://www.unicode.org/glossary/.
 *
 * @param str string
 * @return number of abstract characters in the string
 */
jsize pcsl_string_length(const pcsl_string * str);

/**
 * Returns the number of 16-bit units in the UTF-16 representation
 * of the specified string, not including the terminating zero character.
 * Returns a negative value if str is NULL or doesn't have
 * a valid UTF-16 representation.
 *
 * @param str string
 * @return length of UTF-16 representation of the string
 */
jsize pcsl_string_utf16_length(const pcsl_string * str);

/**
 * Returns the number of bytes in the UTF-8 representation
 * of the specified string, not including the terminating zero character.
 * Returns a negative value if str is NULL or doesn't have
 * a valid UTF-8 representation.
 *
 * @param str string
 * @return length of UTF-8 representation of the string
 */
jsize pcsl_string_utf8_length(const pcsl_string * string);

/**
 * Converts the string to UTF-8.
 * If the buffer length is not sufficient to hold both the UTF-8 data and 
 * the terminating zero character, the conversion is not performed and
 * the function returns PCSL_BUFFER_OVERFLOW.
 * If converted_length is not NULL and the specified string has a valid UTF-8
 * representation, the number of bytes in the UTF-8 representation
 * of the string (not including the terminating zero character) is written 
 * to converted_length.
 *
 * @param str           string to convert
 * @param buffer        buffer to store the result of conversion
 * @param buffer_length length of buffer (number of bytes)
 * @param converted_length
 *            storage for the number of bytes in UTF-8 representation
 *            of the string, not including the terminating zero character
 * @return status of the operation
 */
pcsl_string_status pcsl_string_convert_to_utf8(const pcsl_string * string,
				    jbyte * buffer,
				    jsize buffer_length,
				    jsize * converted_length);

/**
 * Converts the string to UTF-16.
 * If the buffer length is not sufficient to hold both the UTF-16 data and 
 * the terminating zero character, the conversion is not performed and
 * the function returns PCSL_BUFFER_OVERFLOW.
 * If converted_length is not NULL and the specified string has a valid UTF-8
 * representation, the number of 16-bit units in the UTF-16 representation
 * of the string (not including the terminating zero character) is written 
 * to converted_length.
 *
 * @param str           string to convert
 * @param buffer        buffer to store the result of conversion
 * @param buffer_length length of buffer (number of 16-bit units)
 * @param converted_length
 *            storage for the number of 16-bit units in UTF-16 representation
 *            of the string, not including the terminating zero character
 * @return status of the operation
 */
pcsl_string_status pcsl_string_convert_to_utf16(const pcsl_string * str,
				     jchar * buffer,
				     jsize buffer_length,
				     jsize * converted_length);

/**
 * Creates a new string from the specified array of UTF-8 characters
 * and saves a pointer to that string into the specified storage 'string'.
 * Fails if either 'buffer' or 'string' is NULL.
 * Trailing zero characters in the specified array are stripped,
 * then a single zero character is appended at the end.
 * <p>
 * Use pcsl_string_free() to dispose of the created string when done.
 *
 * @param buffer        array of UTF-8 characters
 * @param buffer_length number of bytes in the array
 * @param string        storage for the created string
 * @return status of the operation
 */
pcsl_string_status pcsl_string_convert_from_utf8(const jbyte * buffer,
				      jsize buffer_length,
				      pcsl_string * string);

/**
 * Creates a new string from the specified array of UTF-16 characters
 * and saves a pointer to that string into the specified storage 'string'.
 * Fails if either 'buffer' or 'string' is NULL.
 * Trailing zero characters in the specified array are stripped,
 * then a single zero character is appended at the end.
 * <p>
 * Use pcsl_string_free() to dispose of the created string when done.
 *
 * @param buffer        array of UTF-16 characters
 * @param buffer_length number of 16-bit units in the array
 * @param string        storage for the created string
 * @return status of the operation
 */
pcsl_string_status pcsl_string_convert_from_utf16(const jchar * buffer,
				       jsize buffer_length,
				       pcsl_string * string);

/**
 * Tests if two strings are equal.
 * <p>
 * Returns PCSL_FALSE if either of the two strings is NULL.
 *
 * @param str1 first string to compare
 * @param str2 second string to compare
 * @return PCSL_TRUE if both strings are not NULL and are equal,
 *         PCSL_FALSE otherwise
 */
jboolean pcsl_string_equals(const pcsl_string * str1, const pcsl_string * str2);

/**
 * Compares two strings. The result of comparison is an integer less than,
 * equal to, or greater than zero if str1 is found, respectively, to be
 * less than, to match, or be greater than str2.
 *
 * @param str1 first string to compare
 * @param str2 second string to compare
 * @param comparison storage for the result of comparison
 * @return status of operation comparison
 */
pcsl_string_status pcsl_string_compare(const pcsl_string * str1, const pcsl_string * str2,
			    jint * comparison);

/**
 * Creates a new string that is a concatenation of the two specified strings.
 * <p>
 * The terminating zero character at the end of the first string is stripped
 * before the concatenation.
 * <p>
 * Use pcsl_string_free() to dispose of the created string when done.
 *
 * @param str1 first string to concatenate
 * @param str2 second string to concatenate
 * @param dst  storage for the created concatenation
 * @return status of the operation
 */
pcsl_string_status pcsl_string_cat(const pcsl_string * str1,
			const pcsl_string * str2,
			pcsl_string * str);

/**
 * Creates a new string that is a duplicate of this string.
 * <p>
 * Use pcsl_string_free() to dispose of the created string when done.
 *
 * @param str         string
 * @param dst         storage for the created duplicate
 * @return status of the operation
 */
pcsl_string_status pcsl_string_dup(const pcsl_string * src,
			pcsl_string * dst);

/**
 * Append a string to an existing destination string.
 * All buffers obtained from the string must be released before the string
 * is changed by this function.
 *
 * @param dst the destination string, that grows
 * @param src the source string whose text gets appended to the destination string
 * @return status code
 */
pcsl_string_status pcsl_string_append(pcsl_string* dst, const pcsl_string* src);

/**
 * Append a character to an existing destination string.
 * All buffers obtained from the string must be released before the string
 * is changed by this function.
 *
 * @param dst the destination string, that grows
 * @param newchar the character that gets appended to the destination string
 * @return status code
 */
pcsl_string_status pcsl_string_append_char(pcsl_string* dst, const jchar newchar);

/**
 * Append text in a jchar array to an existing destination string.
 * All buffers obtained from the string must be released before the string
 * is changed by this function.
 *
 * @param dst the destination string, that grows
 * @param newtext the character array that gets appended to the destination string
 * @param textsize the size of the text in the character array
 * @return status code
 */
pcsl_string_status pcsl_string_append_buf(pcsl_string* dst,
                                          const jchar* newtext,
                                          const jint textsize);

/**
 * Set string buffer capacity to the specified value.
 * All buffers obtained from a string must be released before the string
 * is changed by this function.
 *
 * This function is a hint that allows an efficient implementation
 * of concatenation of multiple strings using the append functions
 * (see pcsl_string_append, pcsl_string_append_buf, pcsl_string_append_char).
 * Before appending, invoke this function to specify the final string length.
 *
 * Nevertheless, an implementation may choose to implement this function
 * as a nothing-doer. There is no guarantee that the append functions
 * will not cause buffer reallocation.
 *
 * This function does not fail: if the resize request fails, it leaves
 * the string unchanged.
 *
 * @param str the string whose buffers's capacity is desired to be changed
 * @param size the desired new string buffer capacity
 *
 */
void pcsl_string_predict_size(pcsl_string* str, jint size);

/** deprecated name; use pcsl_string_predict_size instead */
#define pcsl_string_set_capacity(x,y) pcsl_string_predict_size(y,x)

/**
 * Creates a new string that is a substring of this string.
 * The substring represents a subsequence of the abstract character sequence
 * specified by the given string.
 * The subsequence begins with the abstract character at the specified
 * begin_index and extends to the abstract character at the index end_index-1.
 * Trailing zero characters in the subsequence are stripped,
 * then a single zero character is appended at the end.
 * <p>
 * Operation fails if the string is NULL, or indices are not within the bounds:
 * 0 <= begin_index <= end_index <= pcsl_string_length(str).
 * <p>
 * Use pcsl_string_free() to dispose of the created string when done.
 *
 * @param str         string to look in
 * @param begin_index the beginning index, inclusive
 * @param end_index   the ending index, exclusive
 * @param dst         storage for the created substring
 * @return status of the operation
 */
pcsl_string_status pcsl_string_substring(const pcsl_string * str,
			      jint begin_index, jint end_index,
			      pcsl_string * dst);

/**
 * Tests if this string starts with the specified prefix
 *
 * @param str    string to look in
 * @param prefix prefix to look for
 * @return PCSL_TRUE if the string and the prefix are not NULL and
 *         the starts with the specified prefix, PCSL_FALSE otherwise
 */
jboolean pcsl_string_starts_with(const pcsl_string * str,
				 const pcsl_string * prefix);

/**
 * Tests if this string ends with the specified suffix.
 *
 * @param str    string to look in
 * @param suffix suffix to look for
 * @return PCSL_TRUE if the string and the suffix are not NULL and
 *         the ends with the specified suffix, PCSL_FALSE otherwise
 */
jboolean pcsl_string_ends_with(const pcsl_string * str,
			       const pcsl_string * suffix);

/**
 * Returns an index of the first occurrence of the abstract character
 * in the abstract character sequence specified by the given string.
 * The abstract character is specified by the Unicode code point.
 * <p>
 * Returns a negative value if the specified integer value
 * is not in the Unicode codespace,
 * or the specified integer value is a surrogate code point,
 * or the string is NULL, or the character doesn't occur.
 * Otherwise,
 * the returned index must be 0 <= index <= pcsl_string_length(str)-1.
 *
 * @param str   string to look in
 * @param c     Unicode code point to look for
 * @return index of the first occurence of the character
 */
jint pcsl_string_index_of(const pcsl_string * str, jint c);

/**
 * Returns an index of the first occurrence of the abstract character
 * in the abstract character sequence specified by the given string,
 * starting the search at the specified from_index.
 * The abstract character is specified by the Unicode code point.
 * <p>
 * Returns a negative value if the specified integer value
 * is not in the Unicode codespace,
 * or the specified integer value is a surrogate code point,
 * or the string is NULL, or the character doesn't occur.
 * Otherwise,
 * the returned index must be from_index <= index <= pcsl_string_length(str)-1.
 * <p>
 *
 * @param str   string to look in
 * @param c     Unicode code point to look for
 * @return index of the first occurence of the character in the string
 */
jint pcsl_string_index_of_from(const pcsl_string * str, jint c,
			       jint from_index);

/**
 * Returns an index of the last occurrence of the abstract character
 * in the abstract character sequence specified by the given string.
 * The abstract character is specified by the Unicode code point.
 * <p>
 * Returns a negative value if the specified integer value is not in the Unicode codespace,
 * or the specified integer value is a surrogate code point,
 * or the string is NULL, or the character doesn't occur.
 * Otherwise,
 * the returned index must be 0 <= index <= pcsl_string_length(str)-1.
 *
 * @param  str   string to look in
 * @param  c     Unicode code point to look for
 * @return index of the last occurence of the character in the string
 */
jint pcsl_string_last_index_of(const pcsl_string * str, jint c);

/**
 * Returns an index of the last occurrence of the abstract character
 * in the abstract character sequence specified by the given string,
 * searching backward starting at the specified from_index.
 * The abstract character is specified by the Unicode code point.
 * <p>
 * Returns a negative value if the specified integer value is not in the Unicode codespace,
 * or the specified integer value is a surrogate code point,
 * or the string is NULL, or the character doesn't occur.
 * Otherwise,
 * the returned index must be 0 <= index <= from_index.
 *
 * @param  str   string to look in
 * @param  c     Unicode code point to look for
 * @return index of the last occurence of the character in the string
 */
jint pcsl_string_last_index_of_from(const pcsl_string * str, jint c,
				    jint from_index);

/**
 * Removes white spaces from both sides of the string.
 *
 * @param  str string to handle
 * @param  dst storage for the trimmed string
 * @return status of trimming
 */
pcsl_string_status pcsl_string_trim(const pcsl_string * str,
				  pcsl_string * dst);

/**
 * Removes white spaces from the end of the string.
 *
 * @param  str string to handle
 * @param  dst storage for the trimmed string
 * @return status of trimming
 */
pcsl_string_status pcsl_string_trim_from_end(const pcsl_string * str,
				  pcsl_string * dst);

/**
 * Parses the string argument as a signed decimal integer. The
 * characters in the string must all be decimal digits, except that
 * the first character may be an ASCII minus sign to indicate
 * a negative value or plus to indicate positive value.
 * The value must fit into 32-bit signed integer.
 * <p>
 * The parsed integer value is written to the memory location specified by
 * 'value'. Fails if 'value' is NULL.
 * <p>
 * If parsing fails, nothing is written and the return value indicates the
 * failure status.
 *
 * @param str    the string to be parsed
 * @param value  storage for the parsed value
 * @return parsing status
 */
pcsl_string_status pcsl_string_convert_to_jint(const pcsl_string * str, jint * value);

/**
 * Creates a new string representing the specified 32-bit signed integer value.
 * The integer value is converted to signed decimal representation.
 * <p>
 * An address of the created string is written to the memory location specified
 * by 'str'. Fails if 'str' is NULL.
 * <p>
 * Use pcsl_string_free() to dispose of the created string when done.
 *
 * @param   value the value to be converted
 * @param   str   storage for the created string
 * @return  conversion status
 */
pcsl_string_status pcsl_string_convert_from_jint(jint value, pcsl_string * str);

/**
 * Parses the string argument as a signed decimal integer. The
 * characters in the string must all be decimal digits, except that
 * the first character may be an ASCII minus sign to indicate
 * a negative value. The value must fit into 64-bit signed integer.
 * <p>
 * The parsed integer value is written to the memory location specified by
 * 'value'. Fails if 'value' is NULL.
 * <p>
 * If parsing fails, nothing is written and the return value indicates the
 * failure status.
 *
 * @param str    the string to be parsed
 * @param value  storage for the parsed value
 * @return parsing status
 */
pcsl_string_status pcsl_string_convert_to_jlong(const pcsl_string * str, jlong * value);

/**
 * Creates a new string representing the specified 64-bit signed integer value.
 * The integer value is converted to signed decimal representation.
 * <p>
 * An address of the created string is written to the memory location specified
 * by 'str'. Fails if 'str' is NULL.
 * <p>
 * Use pcsl_string_free() to dispose of the created string when done.
 *
 * @param   value the value to be converted
 * @param   str   storage for the created string
 * @return  conversion status
 */
pcsl_string_status pcsl_string_convert_from_jlong(jlong value, pcsl_string * str);

/**
 * Frees the specified string.
 * Sets the freed pcsl_string to PCSL_STRING_NULL.
 *
 * @param   str string to free
 * @return  status
 */
pcsl_string_status pcsl_string_free(pcsl_string * str);

/**
 * Returns UTF-8 representation for the specified string.
 * Returns NULL in case of failure.
 * <p>
 * If not NULL, the returned pointer points to an internal buffer that contains
 * UTF-8 representation of the string. You can safely read up to
 * <code>pcsl_string_utf8_length(str)</code> bytes from this buffer,
 * plus one terminating zero byte.
 * <p>
 * Do not write into the returned buffer, honor the 'const' modifier.
 * The implementation is not required to synchronize the buffer contents with
 * the actual string value.
 * <p>
 * To release the buffer, invoke <code>pcsl_string_release_utf8_data()</code>
 * when you are done and pass this pointer as an argument.
 * <p>
 * All buffers obtained from a string must be released before the string
 * itself is destroyed with <code>pcsl_string_free()</code>.
 *
 * @param str the string
 * @return UTF-8 representation for the specified string
 */
const jbyte * pcsl_string_get_utf8_data(const pcsl_string * str);

/**
 * Releases the internal buffer that pointed to by <code>buf</code>.
 * The pointer must have been returned by a previous call to
 * <code>pcsl_string_get_data_utf8()</code>, otherwise the behavior is not
 * specified.
 * <p>
 * Does nothing if <code>buf</code> is NULL.
 *
 * @param buf pointer to the buffer
 * @param str the string from which this buffer was obtained
 */
void pcsl_string_release_utf8_data(const jbyte * buf, const pcsl_string * str);

/**
 * Returns UTF-16 representation for the specified string.
 * Returns NULL in case of failure.
 * <p>
 * If not NULL, the returned pointer points to an internal buffer that contains
 * UTF-16 representation of the string. You can safely read up to
 * <code>pcsl_string_utf16_length(str)</code> 16-bit units from this buffer,
 * plus one terminating zero 16-bit unit.
 * <p>
 * Do not write into the returned buffer, honor the 'const' modifier.
 * The implementation is not required to synchronize the buffer contents with
 * the actual string value.
 * <p>
 * To release the buffer, invoke <code>pcsl_string_release_utf16_data()</code>
 * when you are done and pass this pointer as an argument.
 * <p>
 * All buffers obtained from a string must be released before the string
 * itself is destroyed with <code>pcsl_string_free()</code>.
 *
 * @param str the string
 * @return UTF-8 representation for the specified string
 */
const jchar * pcsl_string_get_utf16_data(const pcsl_string * str);

/**
 * Releases the internal buffer that pointed to by <code>buf</code>.
 * The pointer must have been returned by a previous call to
 * <code>pcsl_string_get_data_utf16()</code>, otherwise the behavior is not
 * specified.
 * <p>
 * Does nothing if <code>buf</code> is NULL.
 *
 * @param buf pointer to the buffer
 * @param str the string from which this buffer was obtained
 */
void pcsl_string_release_utf16_data(const jchar * buf, const pcsl_string * str);

/**
 * Compares the given string with PCSL_STRING_NULL.
 *
 * @param str the string to compare
 * @return PCSL_TRUE if str is not NULL and is equal to PCSL_STRING_NULL,
 * PCSL_FALSE otherwise
 */
jboolean pcsl_string_is_null(const pcsl_string * str);

/** Zero-terminated empty string constant. */
extern const pcsl_string PCSL_STRING_EMPTY;

/** NULL string constant. */
extern const pcsl_string PCSL_STRING_NULL;


/**
 * Converts the UTF-8 to UTF-16.
 * If the buffer length is not sufficient, the conversion is not performed and
 * the function returns PCSL_BUFFER_OVERFLOW.
 * If converted_length is not NULL, the number of 16-bit units in
 * the UTF-16 representation of the string is written to it.
 * If buffer is NULL, the conversion is performed, but its result is
 * not written; PCSL_STRING_BUFFER_OVERFLOW is NOT returned if buffer is NULL.
 * After encountering a sequence of bytes that does not correspond to any
 * Unicode character, conversion continues but PCSL_STRING_EILSEQ will
 * be returned rather than PCSL_STRING_OK.
 * The error status PCSL_STRING_BUFFER_OVERFLOW takes priority over
 * PCSL_STRING_EILSEQ, that is, when both the buffer is too small and
 * there's a broken character, PCSL_STRING_BUFFER_OVERFLOW is returned.
 *
 * @param str           UTF-8 string representation
 * @param str_length    number of bytes in UTF-8 string representation
 * @param buffer        buffer to store the result of conversion
 * @param buffer_length length of buffer (number of 16-bit units)
 * @param converted_length
 *            storage for the number of 16-bit units in UTF-16 representation
 *            of the string
 * @return status of the operation: PCSL_STRING_EINVAL if str is NULL;
 * PCSL_STRING_BUFFER_OVERFLOW if buffer is not NULL and its size is not enough;
 * PCSL_STRING_EILSEQ if not all bytes represent Unicode characters;
 * PCSL_STRING_OK is everything is ok.
 */
pcsl_string_status pcsl_utf8_convert_to_utf16(const jbyte * str, jsize str_length,
			      jchar * buffer, jsize buffer_length,
			      jsize * converted_length);

/**
 * Converts the UTF-16 to UTF-8.
 * If the buffer length is not sufficient, the conversion is not performed and
 * the function returns PCSL_BUFFER_OVERFLOW.
 * If converted_length is not NULL, the number of bytes in
 * the UTF-8 representation of the string is written to it.
 * If buffer is NULL, the conversion is performed, but its result is
 * not written; PCSL_STRING_BUFFER_OVERFLOW is NOT returned if buffer is NULL.
 * After encountering a sequence of 16-bit units that does not correspond to any
 * Unicode character, conversion continues but PCSL_STRING_EILSEQ will
 * be returned rather than PCSL_STRING_OK.
 * The error status PCSL_STRING_BUFFER_OVERFLOW takes priority over
 * PCSL_STRING_EILSEQ, that is, when both the buffer is too small and
 * there's a broken character, PCSL_STRING_BUFFER_OVERFLOW is returned.
 *
 * @param str           UTF-16 string representation
 * @param str_length    number of 16-bit units in UTF-16 string representation
 * @param buffer        buffer to store the result of conversion
 * @param buffer_length length of buffer (number of bytes)
 * @param converted_length
 *            storage for the number of bytes in UTF-8 representation
 *            of the string
 * @return status of the operation: PCSL_STRING_EINVAL if str is NULL;
 * PCSL_STRING_BUFFER_OVERFLOW if buffer is not NULL and its size is not enough;
 * PCSL_STRING_EILSEQ if not all 16-bit units represent Unicode characters;
 * PCSL_STRING_OK is everything is ok.
 */
pcsl_string_status pcsl_utf16_convert_to_utf8(const jchar * str, jsize str_length,
			      jbyte * buffer, jsize buffer_length,
			      jsize * converted_length);

/**
 * Converts the Unicode code point to UTF-16 code unit.
 * See Unicode Glossary at http://www.unicode.org/glossary/.
 *
 * @param code_point  Unicode code point
 * @param code_unit   Storage for UTF-16 code unit
 * @param unit_length Storage for the number of 16-bit units
 *                    in the UTF-16 code unit
 * @return status of the conversion
 */
pcsl_string_status pcsl_code_point_to_utf16_code_unit(jint code_point,
				      jchar code_unit[2],
				      jsize * unit_length);

/**
 * Convert a Unicode string into a form that can be safely stored on
 * an ANSI-compatible file system. All characters that are not
 * [A-Za-z0-9] are converted into %uuuu, where uuuu is the hex
 * representation of the character's unicode value. Note even
 * though "_" is allowed it is converted because we use it for
 * for internal purposes. Potential file separators are converted
 * so the storage layer does not have deal with sub-directory hierarchies.
 *
 * @param str_data buffer with a string that may contain any unicode character
 * @param str_len length of the string pointed to by str_data
 * @param pBuffer a buffer that is at least 5 times the size of str after
 *        the offset, must not be the memory as str
 * @param offset where to start putting the characters
 *
 * @return number of characters put in pBuffer
 */
int pcsl_utf16_to_escaped_ascii(const jchar* str_data, const int str_len,
                             jchar* pBuffer, int offset);

/** @} */   //End of group

#ifdef __cplusplus
}
#endif

#endif /* _PCSL_STRING_H_ */


