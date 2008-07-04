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

#ifndef _PCSL_ESC_MD_H_
#define _PCSL_ESC_MD_H_

#ifndef _PCSL_ESC_H_
# error "Never include <pcsl_esc_md.h> directly; use <pcsl_esc.h> instead."
#endif

/* let Donuts know that all radix values may be tested */
#define PCSL_ESC_TESTING


extern int esc_case_sensitive;
#define PCSL_ESC_CASE_SENSITIVE_MD esc_case_sensitive

extern int esc_radix;
#define PCSL_ESC_RADIX_MD esc_radix


#define PCSL_ESC_ONLY_IF_CASE_SENSITIVE_MD( something ) if(esc_case_sensitive) { something; };;
/*
#if PCSL_ESC_CASE_SENSITIVE
#define PCSL_ESC_ONLY_IF_CASE_SENSITIVE( something ) something;;
#else
#define PCSL_ESC_ONLY_IF_CASE_SENSITIVE( something ) ;;
#endif
*/

/* special symbols used as digits */
#define PCSL_ESC_MOREDIGITS "`~!"  "^&*()-_+[]{}\\|;:',<>"
//                      `~!@#$%^&*()-_=+[]{}\|;:'",.<>/?

/**
 * Toggle shift mode, for all subsequent characters.
 * This character MAY be also used as digit.
 */
#define PCSL_ESC_SHIFT_TOGGLE '_'

/**
 * Toggle shift mode, for only one character.
 * This character MAY be also used as digit.
 */
#define PCSL_ESC_SHIFT1 '-'

/**
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
#define PCSL_ESC_PREV_BLOCK '$'

/**
 * Begin radix-N-encoded utf-16, the utf-16 characters will share the same
 * most significant byte; the value of the most-significant byte is the
 * first (most significant) byte in the first subsequent tuple.
 * This character MUST NOT be used as digit.
 * Note that after encountering this character, the values of the variables
 * that keep the previous-most-significant-byte and last-most-significant-byte
 * values are set, correspondingly, to last-most-significant-byte and 
 * the-new-most-significant-byte.
 */
#define PCSL_ESC_NEW_BLOCK '%'

/**
 * Begin radix-N-encoded utf-16, the utf-16 characters will share the same
 * most significant byte; re-use the last value of the most-significant byte.
 * This character MUST NOT be used as digit.
 * The last-most-significant-byte value is kept in a variable, and initially
 * this variable is set to 0.
 */
#define PCSL_ESC_TOGGLE '#'

/**
 * Begin radix-N-encoded utf-16, for each utf-16 character all 16 bits will
 * be specified.
 * This character MUST NOT be used as digit.
 * Note after each utf-16 character, the values of the variables
 * that keep the previous-most-significant-byte and last-most-significant-byte
 * values are set, correspondingly, to last-most-significant-byte and 
 * the-new-most-significant-byte.
 */
#define PCSL_ESC_FULL_CODES '='

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
#define PCSL_ESC_BYTES_PER_TUPLE \
     ( PCSL_ESC_RADIX == 16 ? 1 \
    :( PCSL_ESC_RADIX == 41 ? 2 \
    :( PCSL_ESC_RADIX == 64 ? 3 \
    :( PCSL_ESC_RADIX == 85 ? 4 \
    :0 \
    ))))

/**
 * The number of digits enough to encode a tuple as an unsigned number.
 * Each full tuple contains PCSL_ESC_BYTES_PER_TUPLE bytes.
 * Note that a tuple does not need to be a full tuple, but anyway,
 * N bytes are encoded with N+1 digits, so at most PCSL_ESC_BYTES_PER_TUPLE
 * bytes are encoded as one tuple with at most PCSL_ESC_DIGITS_PER_TUPLE digits.
 */
#define PCSL_ESC_DIGITS_PER_TUPLE (PCSL_ESC_BYTES_PER_TUPLE+1)

/**
 * This value is both a mask for a full tuple, and the maximal unsigned
 * value that may be encoded with one tuple.
 * If the full tuple contains N bytes, the 8*N least significant bits
 * of this value are ones, and all other bits are zeroes.
 */
#define PCSL_ESC_FULL_TUPLE_MASK \
     ( PCSL_ESC_RADIX == 16 ? 0xff \
    :( PCSL_ESC_RADIX == 41 ? 0xffff \
    :( PCSL_ESC_RADIX == 64 ? 0xffffff \
    :( PCSL_ESC_RADIX == 85 ? 0xffffffff \
    :0 \
    ))))

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
#define PCSL_ESC_CONVERT_CASE(c) c

/**
 * Convert a symbol to uppercase.
 * This function need not be restricted to letters only,
 */
#define PCSL_ESC_UPPER(c) ('a'<=(c)&&(c)<='z'?(c)+'A'-'a':(c))
#define PCSL_ESC_LOWER(c) ('A'<=(c)&&(c)<='Z'?(c)+'a'-'A':(c))

#endif
