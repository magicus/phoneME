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

int pcsl_esc_num2digit(unsigned int n);
int pcsl_esc_digit2num(unsigned int c);
pcsl_string_status pcsl_esc_append_encoded_tuple(pcsl_string* str, unsigned int num, unsigned int maxnum);
int pcsl_esc_extract_encoded_tuple(unsigned int *pnum, const jchar**pptext);

void pcsl_esc_init();
void pcsl_esc_attach_buf(const jchar* in, jsize len, pcsl_string* out);
void pcsl_esc_attach_string(const pcsl_string* data, pcsl_string*dst);
void pcsl_esc_extract_attached(const int offset, const pcsl_string *src, pcsl_string* dst);

/**
 * True if the file system is case-sensitive
 */
#define PCSL_ESC_CASE_SENSITIVE PCSL_ESC_CASE_SENSITIVE_MD

/**
 * Radix used to convert groups of bytes (tuples) into ascii characters allowed in file names.
 */
#define PCSL_ESC_RADIX PCSL_ESC_RADIX_MD

/**
 * PCSL_ESC_ONLY_IF_CASE_SENSITIVE( something ) executes something only if PCSL_ESC_CASE_SENSITIVE is true.
 * 
 */
#define PCSL_ESC_ONLY_IF_CASE_SENSITIVE PCSL_ESC_ONLY_IF_CASE_SENSITIVE_MD



#endif

