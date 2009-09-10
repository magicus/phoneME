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
#ifdef PCSL_ESC_TESTING
#error PCSL_ESC_TESTING must not be defined
#endif


#define PCSL_ESC_CASE_SENSITIVE_MD 0

#define PCSL_ESC_RADIX_MD 16


#define PCSL_ESC_ONLY_IF_CASE_SENSITIVE_MD( something ) ;;
#define PCSL_ESC_EXPR_IF_CASE_SENSITIVE_MD( expr_if, expr_else ) (expr_else)

/*
 * We don't need more digits for radix = 16
 */
#define PCSL_ESC_MOREDIGITS_MD ""

/* may be also used as digits */
#define PCSL_ESC_SHIFT_TOGGLE_MD '_'
#define PCSL_ESC_SHIFT1_MD '-'

/* must not be used as digits */
#define PCSL_ESC_PREV_BLOCK_MD '$'
#define PCSL_ESC_NEW_BLOCK_MD '%'
#define PCSL_ESC_TOGGLE_MD '#'
#define PCSL_ESC_FULL_CODES_MD '='

    
#define PCSL_ESC_BYTES_PER_TUPLE_MD 1
/*
     ( PCSL_ESC_RADIX == 16 ? 1 \
    :( PCSL_ESC_RADIX == 41 ? 2 \
    :( PCSL_ESC_RADIX == 64 ? 3 \
    :( PCSL_ESC_RADIX == 85 ? 4 \
    :0 \
    ))))
*/
#define PCSL_ESC_DIGITS_PER_TUPLE_MD (PCSL_ESC_BYTES_PER_TUPLE+1)

#define PCSL_ESC_FULL_TUPLE_MASK_MD 0xff
/*
     ( PCSL_ESC_RADIX == 16 ? 0xff \
    :( PCSL_ESC_RADIX == 41 ? 0xffff \
    :( PCSL_ESC_RADIX == 64 ? 0xffffff \
    :( PCSL_ESC_RADIX == 85 ? 0xffffffff \
    :0 \
    ))))
*/
/* we can enable (set to 1) or disable (set to 0) this feature */
#define PCSL_ESC_EXTENDED_SHIFT_MD 0

/* if PCSL_ESC_EXTENDED_SHIFT_MD==1, these define special symbols
 * that are considered upper-case of digits */
#define PCSL_ESC_EXTRA_LOWERCASE_MD "0123456789"
#define PCSL_ESC_EXTRA_UPPERCASE_MD " !@#$%.,=+"
#define PCSL_ESC_EXTRA_NEEDXCASE_MD PCSL_ESC_EXTRA_UPPERCASE_MD

#define PCSL_ESC_EXTRA_UPPER_MD(c) (c)
/*
    (esc_extended_shift ? pcsl_esc_mapchar((c),PCSL_ESC_EXTRA_LOWERCASE_MD,PCSL_ESC_EXTRA_UPPERCASE_MD) : (c))
*/
#define PCSL_ESC_EXTRA_LOWER_MD(c) (c)
/*
    (esc_extended_shift ? pcsl_esc_mapchar((c),PCSL_ESC_EXTRA_UPPERCASE_MD,PCSL_ESC_EXTRA_LOWERCASE_MD) : (c))
*/

#define PCSL_ESC_CONVERT_CASE_MD(c) PCSL_ESC_EXTRA_LOWER_MD(c)


#define PCSL_ESC_UPPER_MD(c) \
    PCSL_ESC_EXPR_IF_CASE_SENSITIVE_MD( \
	PCSL_ESC_EXTRA_UPPER_MD(c), \
	'a'<=(c)&&(c)<='z'?(c)+'A'-'a':PCSL_ESC_EXTRA_UPPER_MD(c) \
	)
#define PCSL_ESC_LOWER_MD(c) \
    PCSL_ESC_EXPR_IF_CASE_SENSITIVE_MD( \
	PCSL_ESC_EXTRA_LOWER_MD(c), \
	'A'<=(c)&&(c)<='Z'?(c)+'a'-'A':PCSL_ESC_EXTRA_LOWER_MD(c) \
	)

 

#endif
