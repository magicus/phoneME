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

#include <java_types.h>
#include <pcsl_string_status.h>

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
