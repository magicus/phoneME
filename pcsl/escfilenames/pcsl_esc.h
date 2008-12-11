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

#ifndef _PCSL_ESC_H_
#define _PCSL_ESC_H_

#include <pcsl_esc_md.h>
#include <pcsl_string.h>

#ifdef __cplusplus
extern "C" {
#endif

char pcsl_esc_mapchar(char x, char* from, char* to);
int pcsl_esc_num2digit(unsigned int n);
int pcsl_esc_digit2num(unsigned int c);
pcsl_string_status pcsl_esc_append_encoded_tuple(pcsl_string* str, unsigned int num, unsigned int maxnum);
int pcsl_esc_extract_encoded_tuple(unsigned int *pnum, const jchar**pptext);

void pcsl_esc_init();
pcsl_string_status pcsl_esc_attach_buf(const jchar* in, jsize len, pcsl_string* out);
pcsl_string_status pcsl_esc_attach_string(const pcsl_string* data, pcsl_string*dst);
pcsl_string_status pcsl_esc_extract_attached(const int offset, const pcsl_string *src, pcsl_string* dst);

/**
 * True if the file system is case-sensitive
 */
#define PCSL_ESC_CASE_SENSITIVE PCSL_ESC_CASE_SENSITIVE_MD

/**
 * Radix used to convert groups of bytes (tuples) into ascii characters allowed in file names.
 */
#define PCSL_ESC_RADIX PCSL_ESC_RADIX_MD

/**
 * PCSL_ESC_ONLY_IF_CASE_SENSITIVE( something ) executes something
 * only if PCSL_ESC_CASE_SENSITIVE is true.
 * 
 */
#define PCSL_ESC_ONLY_IF_CASE_SENSITIVE(something) PCSL_ESC_ONLY_IF_CASE_SENSITIVE_MD(something)


#define PCSL_ESC_EXPR_IF_CASE_SENSITIVE(expr_if,expr_else) PCSL_ESC_EXPR_IF_CASE_SENSITIVE_MD(expr_if,expr_else)

/* special symbols used as digits */
#define PCSL_ESC_MOREDIGITS PCSL_ESC_MOREDIGITS_MD

/**
 * A character that has a special meaning in encoded text.
 * Toggle shift mode, for all subsequent characters.
 * This character MAY be also used as digit.
 */
#define PCSL_ESC_SHIFT_TOGGLE PCSL_ESC_SHIFT_TOGGLE_MD

/**
 * A character that has a special meaning in encoded text.
 * Toggle shift mode, for only one character.
 * This character MAY be also used as digit.
 */
#define PCSL_ESC_SHIFT1 PCSL_ESC_SHIFT1_MD

/**
 * A character that has a special meaning in encoded text.
 * Begin radix-N-encoded utf-16, the utf-16 characters will share the same
 * most significant byte; re-use the previous value of the most-significant
 * byte (that is, the one before the last value).
 * This character MUST NOT be used as digit.
 * The previous-most-significant-byte value is kept in a variable, and initially
 * this variable is set to 0.
 * Note that after encountering this character, the values of the variables
 * that keep the previous-most-significant-byte and last-most-significant-byte
 * values are exchanged.
 */
#define PCSL_ESC_PREV_BLOCK PCSL_ESC_PREV_BLOCK_MD

/**
 * A character that has a special meaning in encoded text.
 * Begin radix-N-encoded utf-16, the utf-16 characters will share the same
 * most significant byte; the value of the most-significant byte is the
 * first (most significant) byte in the first subsequent tuple.
 * This character MUST NOT be used as digit.
 * Note that after encountering this character, the values of the variables
 * that keep the previous-most-significant-byte and last-most-significant-byte
 * values are set, correspondingly, to last-most-significant-byte and 
 * the-new-most-significant-byte.
 */
#define PCSL_ESC_NEW_BLOCK PCSL_ESC_NEW_BLOCK_MD

/**
 * A character that has a special meaning in encoded text.
 * Begin radix-N-encoded utf-16, the utf-16 characters will share the same
 * most significant byte; re-use the last value of the most-significant byte.
 * This character MUST NOT be used as digit.
 * The last-most-significant-byte value is kept in a variable, and initially
 * this variable is set to 0.
 */
#define PCSL_ESC_TOGGLE PCSL_ESC_TOGGLE_MD

/**
 * A character that has a special meaning in encoded text.
 * Begin radix-N-encoded utf-16, for each utf-16 character all 16 bits will
 * be specified.
 * This character MUST NOT be used as digit.
 * Note after each utf-16 character, the values of the variables
 * that keep the previous-most-significant-byte and last-most-significant-byte
 * values are set, correspondingly, to last-most-significant-byte and 
 * the-new-most-significant-byte.
 */
#define PCSL_ESC_FULL_CODES PCSL_ESC_FULL_CODES_MD

/**
 * The number of bytes in a tuple.
 * Bytes are grouped into tuples, and each tuple is converted, separately,
 * into a sequence of radix-N digits. N is chosen so that
 * PCSL_ESC_BYTES_PER_TUPLE bytes become PCSL_ESC_BYTES_PER_TUPLE-1 digits.
 * Tuples are converted as unsigned numbers.
 * Note that a tuple does not need to be a full tuple, but anyway,
 * N bytes are encoded with N+1 digits, so at most PCSL_ESC_BYTES_PER_TUPLE
 * bytes are encoded as one tuple with at most PCSL_ESC_DIGITS_PER_TUPLE digits.
 */
#define PCSL_ESC_BYTES_PER_TUPLE PCSL_ESC_BYTES_PER_TUPLE_MD

/**
 * The number of digits enough to encode a tuple as an unsigned number.
 * Each full tuple contains PCSL_ESC_BYTES_PER_TUPLE bytes.
 * Note that a tuple does not need to be a full tuple, but anyway,
 * N bytes are encoded with N+1 digits, so at most PCSL_ESC_BYTES_PER_TUPLE
 * bytes are encoded as one tuple with at most PCSL_ESC_DIGITS_PER_TUPLE digits.
 */
#define PCSL_ESC_DIGITS_PER_TUPLE PCSL_ESC_DIGITS_PER_TUPLE_MD

/**
 * This value is both a mask for a full tuple, and the maximal unsigned
 * value that may be encoded with one tuple.
 * If the full tuple contains N bytes, the 8*N least significant bits
 * of this value are ones, and all other bits are zeroes.
 */
#define PCSL_ESC_FULL_TUPLE_MASK PCSL_ESC_FULL_TUPLE_MASK_MD

/**
 * Convert a character to the (upper- or lower-) case, preferrable in file names
 * on the target operating system.
 * May be a nothing-doer if the conversion is not necessary.
 * Note that one can use it for non-letter symbols, for example, one can
 * consider ',' (comma) as the upper-case of '1', and ' ' (space) as the
 * upper-case of '2', so that "Hello, world" will be encoded as
 * "-hello_12_world" (assuming that '-' and '_' mean inverting the case
 * for one character and all subsequent characters, correspondingly).
 * If the character x is no of type goes_escaped, then the character code
 * returned as PCSL_ESC_CONVERT_CASE(x) MUST NOT have any special meaning,
 * tht is, be one of PCSL_ESC_SHIFT_TOGGLE, PCSL_ESC_SHIFT1, PCSL_ESC_PREV_BLOCK, PCSL_ESC_NEW_BLOCK,
 * PCSL_ESC_TOGGLE, PCSL_ESC_FULL_CODES.
 */
#define PCSL_ESC_CONVERT_CASE(c) PCSL_ESC_CONVERT_CASE_MD(c)

/**
 * Convert a symbol to uppercase.
 * This function need not be restricted to letters only,
 * allowing special characters not allowed in the platform file names
 * to be encoded as "the other case" of characters that
 * are allowed in file names.
 */
#define PCSL_ESC_UPPER(c) PCSL_ESC_UPPER_MD(c)

/**
 * Convert a symbol to lowercase.
 * This function need not be restricted to letters only,
 * allowing special characters not allowed in the platform file names
 * to be encoded as "the other case" of characters that
 * are allowed in file names.
 */
#define PCSL_ESC_LOWER(c) PCSL_ESC_LOWER_MD(c)

/**
 * True if some non-alphabetical characters are considered uppercase
 * of some other non-alphabetical characters.
 * This feature allows us, for example, to treat !@#$ as upper-case of 1234.
 */
#define PCSL_ESC_EXTENDED_SHIFT PCSL_ESC_EXTENDED_SHIFT_MD

/**
 * Non-alphabetic characters considered upper-case
 */
#define PCSL_ESC_EXTRA_UPPERCASE PCSL_ESC_EXTRA_UPPERCASE_MD

/**
 * Non-alphabetic characters considered lower-case
 */
#define PCSL_ESC_EXTRA_LOWERCASE PCSL_ESC_EXTRA_LOWERCASE_MD

/**
 * Either PCSL_ESC_EXTRA_LOWERCASE or PCSL_ESC_EXTRA_UPPERCASE.
 * These non-alphabetic characters are not allowed if file names
 * and are encoded as "shifted" versions of allowed characters.
 */
#define PCSL_ESC_EXTRA_NEEDXCASE PCSL_ESC_EXTRA_NEEDXCASE_MD

#ifdef __cplusplus
}
#endif

#endif

