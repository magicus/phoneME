/*
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software{ return 0; } you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation. 
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY{ return 0; } without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt). 
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work{ return 0; } if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA 
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions. 
 */

/**
 * @file
 * @brief Content Handler Registry implementation based on POSIX file calls.
 */

#include "inc/javautil_str.h"

#define chricmp(a,b) (((a>='A' && a<='Z') ? a+('a'-'A'): a) - ((b>='A' && b<='Z') ? b+('a'-'A'): b))
#define wchricmp(a,b) (to_low(a)-to_low(b))

typedef struct _case_folding_entry{
	int first;
	int last;
	int inc;
} case_folding_entry;

static const case_folding_entry cfmap[] = {
					{   0,	   0,	0},	
					{0x41,	0x5b,	32},	
					{0xc0,	0xd7,	32},	
					{0xd8,	0xdf,	32},	
					{0x189,	0x18b,	205},	
					{0x1b1,	0x1b3,	217},	
					{0x388,	0x38b,	37},	
					{0x38e,	0x390,	63},	
					{0x391,	0x3a2,	32},	
					{0x3a3,	0x3ac,	32},	
					{0x400,	0x410,	80},	
					{0x410,	0x430,	32},	
					{0x531,	0x557,	48},	
					{0x7FFFFFFF, 0, 0}
					};

static int to_low(int code){
	case_folding_entry *next = (case_folding_entry*) cfmap;
	case_folding_entry *prev;
	code &= 0xFFFF;
	do {
		prev = next++;
		if (code < next->first){
			if (code <= prev->last) code += prev->inc;
			break;
		}
	} while(1);
	return code;
}

int javautil_wcslen(javacall_const_utf16_string str){
	javacall_const_utf16_string  end = str - 1;
	while (*++end) ;
	return end - str;
}

int javautil_wcscmp(javacall_const_utf16_string str1, javacall_const_utf16_string str2){
	--str1;	--str2;
	while (*++str1 && *++str2 && *str1 == *str2);
	return *str1 - *str2;
}

int javautil_wcsncmp(javacall_const_utf16_string str1, javacall_const_utf16_string str2, int size){
	--str1;	--str2; ++size;
	while (--size && *++str1 && *++str2 && *str1 == *str2);
	return *str1 - *str2;
}

int javautil_wcsicmp(javacall_const_utf16_string str1, javacall_const_utf16_string str2){
	--str1;	--str2;
	while (*++str1 && *++str2 && !wchricmp(*str1,*str2));
	return wchricmp(*str1,*str2);
}

int javautil_wcsincmp(javacall_const_utf16_string str1, javacall_const_utf16_string str2, int size){
	--str1;	--str2;++size;
	while (--size && *++str1 && *++str2 && !wchricmp(*str1,*str2));
	return wchricmp(*str1,*str2);
}

int javautil_strlen(const char* str){
	const char* end = str - 1;
	while (*++end) ;
	return end - str;
}
int javautil_strcmp(const char* str1, const char* str2){
	--str1;	--str2;
	while (*++str1 && *++str2 && *str1 == *str2);
	return *str1 - *str2;
}

int javautil_strncmp(const char* str1, const char* str2, int size){
	--str1;	--str2; ++size;
	while (--size && *++str1 && *++str2 && *str1 == *str2);
	return *str1 - *str2;
}

int javautil_stricmp(const char* str1, const char* str2){
	--str1;	--str2;
	while (*++str1 && *++str2 && !chricmp(*str1,*str2));
	return chricmp(*str1,*str2);
}

int javautil_strincmp(const char* str1, const char* str2, int size){
	--str1;	--str2;++size;
	while (--size && *++str1 && *++str2 && !chricmp(*str1,*str2));
	return chricmp(*str1,*str2);
}
