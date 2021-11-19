/**
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
 * 
 * These are constant defines both in native and Java layers.
 * NOTE: DO NOT EDIT. THIS FILE IS GENERATED. If you want to 
 * edit it, you need to modify the corresponding XML files.
 *
 * Patent pending.
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
	code &= 0xFFFFL;
	do {
		prev = next++;
		if (code < next->first){
			if (code <= prev->last) code += prev->inc;
			break;
		}
	} while(1);
	return code;
}

javacall_utf16* javautil_str_wcstolow(javacall_utf16* str){
	javacall_utf16*  s = str - 1;
	while (*++s) *s = to_low(*s);
	return str;
}


int javautil_str_wcslen(const javacall_utf16* str){
	const javacall_utf16*  end = str - 1;
	while (*++end) ;
	return end - str;
}

int javautil_str_wcscmp(const javacall_utf16* str1, const javacall_utf16* str2){
	while (*str1 && *str2 && *str1 == *str2) {++str1;++str2;}
	return *str1 - *str2;
}

int javautil_str_wcsncmp(const javacall_utf16* str1, const javacall_utf16* str2, int size){
	++size;
	while (--size && *str1 && *str2 && *str1 == *str2) {++str1;++str2;}
	return (!size) ? 0 : *str1 - *str2;
}

int javautil_str_wcsicmp(const javacall_utf16* str1, const javacall_utf16* str2){
	while (*str1 && *str2 && !wchricmp(*str1,*str2)) {++str1;++str2;}
	return wchricmp(*str1,*str2);
}

int javautil_str_wcsincmp(const javacall_utf16* str1, const javacall_utf16* str2, int size){
	++size;
	while (--size && *str1 && *str2 && !wchricmp(*str1,*str2)) {++str1;++str2;}
	return (!size) ? 0 : wchricmp(*str1,*str2);
}

int javautil_str_strlen(const char* str){
	const char* end = str - 1;
	while (*++end) ;
	return end - str;
}
int javautil_str_strcmp(const char* str1, const char* str2){
	while (*str1 && *str2 && *str1 == *str2)  {++str1;++str2;}
	return *str1 - *str2;
}

int javautil_str_strncmp(const char* str1, const char* str2, int size){
	++size;
	while (--size && *str1 && *str2 && *str1 == *str2)  {++str1;++str2;}
	return (!size) ? 0 : *str1 - *str2;
}

int javautil_str_stricmp(const char* str1, const char* str2){
	while (*str1 && *str2 && !chricmp(*str1,*str2))  {++str1;++str2;}
	return chricmp(*str1,*str2);
}

int javautil_str_strincmp(const char* str1, const char* str2, int size){
	++size;
	while (--size && *str1 && *str2 && !chricmp(*str1,*str2))  {++str1;++str2;}
	return (!size) ? 0 : chricmp(*str1,*str2);
}

char* javautil_str_strtolow(char* str){
	char*  s = str - 1;
	while (*++s) *s = (*s>='A' && *s<='Z') ? *s + ('a'-'A'): *s;
	return str;
}

const javacall_utf16* javautil_str_wcsstr(const javacall_utf16* str, const javacall_utf16* pattern){
	--str;
	while (*++str){
		const javacall_utf16* str1= str-1;
		const javacall_utf16* str2= pattern-1;
		while (*++str1 && *++str2 && *str1 == *str2);
		if (!*str2) return str;
	}
	return 0;
}

const javacall_utf16* javautil_str_wcsstri(const javacall_utf16* str, const javacall_utf16* pattern){
	--str;
	while (*++str){
		const javacall_utf16* str1= str-1;
		const javacall_utf16* str2= pattern-1;
		while (*++str1 && *++str2 && !wchricmp(*str1,*str2));
		if (!*str2) return str;
	}
	return 0;
}

const javacall_utf16* javautil_str_wcsrstr(const javacall_utf16* str, const javacall_utf16* pattern){
	const javacall_utf16* end=str-1;
	while (*++end);
	while (--end>=str){
		const javacall_utf16* str1= end-1;
		const javacall_utf16* str2= pattern-1;
		while (*str1 && *str2 && *str1 == *str2) {++str1;++str2;}
		if (!*str2) return end;
	}
	return 0;
}

const javacall_utf16* javautil_str_wcsrstri(const javacall_utf16* str, const javacall_utf16* pattern){
	const javacall_utf16* end=str-1;
	while (*++end);
	while (--end>=str){
		const javacall_utf16* str1= end-1;
		const javacall_utf16* str2= pattern-1;
		while (*str1 && *str2 && !wchricmp(*str1,*str2)) {++str1;++str2;}
		if (!*str2) return end;
	}
	return 0;
}

const javacall_utf16* javautil_str_wcschr(const javacall_utf16* str, javacall_utf16 wch){
	--str;
	while (*++str && *str != wch);
	return *str ? str : 0;
}

const javacall_utf16* javautil_str_wcschri(const javacall_utf16* str, javacall_utf16 wch){
	--str;
	while (*++str && wchricmp(*str,wch));
	return *str ? str : 0;
}

const javacall_utf16* javautil_str_wcsrchr(const javacall_utf16* str, javacall_utf16 wch){
	const javacall_utf16* res = 0;
	--str;
	while (*++str) if (*str == wch) res=str;
	return res;
}

const javacall_utf16* javautil_str_wcsrchri(const javacall_utf16* str, javacall_utf16 wch){
	const javacall_utf16* res = 0;
	--str;
	while (*++str) if (!wchricmp(*str,wch)) res=str;
	return res;
}

