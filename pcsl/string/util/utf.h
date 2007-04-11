/*
 *
 *
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

#include <java_types.h>
#include <pcsl_string_status.h>

/**
 * Converts the UTF-8 to UTF-16.
 * If the buffer length is not sufficient, the conversion is not performed and
 * the function returns PCSL_BUFFER_OVERFLOW.
 * If converted_length is not NULL, the number of 16-bit units in
 * the UTF-16 representation of the string is written to it.
 * If buffer is NULL, the conversion is performed, but its result is
 * not written.
 *
 * @param str           UTF-8 string representation
 * @param str_length    number of bytes in UTF-8 string representation
 * @param buffer        buffer to store the result of conversion
 * @param buffer_length length of buffer (number of 16-bit units)
 * @param converted_length
 *            storage for the number of 16-bit units in UTF-16 representation
 *            of the string
 * @return status of the operation
 */
pcsl_string_status utf8_convert_to_utf16(const jbyte * str, jsize str_length,
			      jchar * buffer, jsize buffer_length,
			      jsize * converted_length);

/**
 * Converts the UTF-16 to UTF-8.
 * If the buffer length is not sufficient, the conversion is not performed and
 * the function returns PCSL_BUFFER_OVERFLOW.
 * If converted_length is not NULL, the number of bytes in
 * the UTF-8 representation of the string is written to it.
 * If buffer is NULL, the conversion is performed, but its result is
 * not written.
 *
 * @param str           UTF-16 string representation
 * @param str_length    number of 16-bit units in UTF-16 string representation
 * @param buffer        buffer to store the result of conversion
 * @param buffer_length length of buffer (number of bytes)
 * @param converted_length
 *            storage for the number of bytes in UTF-8 representation
 *            of the string
 * @return status of the operation
 */
pcsl_string_status utf16_convert_to_utf8(const jchar * str, jsize str_length,
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
pcsl_string_status code_point_to_utf16_code_unit(jint code_point,
				      jchar code_unit[2],
				      jsize * unit_length);

/**
 * Returns the number of abstract characters in the string specified
 * by the UTF-16 code unit sequence.
 * Returns -1 if str is NULL or is not a valid UTF-16 code unit sequence.
 * See Unicode Glossary at http://www.unicode.org/glossary/.
 *
 * @param str           UTF-16 code unit sequence
 * @param str_length    number of UTF-16 code units in the sequence
 * @return number of abstract characters in the string
 */
jsize utf16_string_length(jchar * str, jsize str_length);

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
int unicode_to_escaped_ascii(const jchar* str_data, const int str_len,
                             jchar* pBuffer, int offset);

/**
 * Returns whether the specified integer value is a Unicode code point.
 */
#define IS_UNICODE_CODE_POINT(code_point) \
  ((code_point) >= 0x0 && (code_point) <= 0x10ffff)

/**
 * Returns whether the specified integer value is a BMP code point.
 */
#define IS_BMP_CODE_POINT(code_point) \
  ((code_point) >= 0x0 && (code_point) <= 0xffff)

/**
 * Returns whether the specified integer value is a surrogate code point.
 */
#define IS_SURROGATE_CODE_POINT(code_point) \
  ((code_point) >= 0xd800 && (code_point) <= 0xdfff)
