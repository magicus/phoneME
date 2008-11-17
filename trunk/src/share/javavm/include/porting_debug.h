/*
 * @(#)porting_debug.h	1.19 06/10/10
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.  
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
 */

#ifndef _INCLUDED_PORTING_DEBUG_H
#define _INCLUDED_PORTING_DEBUG_H

#if defined(CVM_PORTING_DEBUG_STUBS) && !defined(CVM_PORTING_DEBUG_IMPL)

#include "javavm/include/porting/ansi/stddef.h"
#include "javavm/include/porting/vm-defs.h"

/*
 * This file allows a way to redirect interfaces through
 * debugging stubs.  Add stubs as needed.
 */

void * CVMCmemsetStub(void * s, int c, size_t n);
void * CVMCmemcpyStub(void * d, const void * s, size_t n);
void * CVMCmemmoveStub(void * d, const void * s, size_t n);
int CVMCstrcmpStub(const char * s1, const char * s2);
int CVMCstrncmpStub(const char * s1, const char * s2, size_t n);
size_t CVMCstrlenStub(const char* s);
char * CVMCstrcpyStub(char * dst, const char * src);
char * CVMCstrncpyStub(char * dst, const char * src, size_t n);
char * CVMCstrdupStub(const char * str);
char * CVMCstrcatStub(char * dst, const char * src);
char * CVMCstrncatStub(char * dst, const char * src, size_t n);
char * CVMCstrstrStub(const char * s1, const char * s2);
char * CVMCstrchrStub(const char * s, int c);
char * CVMCstrrchrStub(const char * s, int c);
long CVMCstrtolStub(const char * str, char ** endptr, int base);
void * CVMCmallocStub(size_t size);
void * CVMCcallocStub(size_t nelem, size_t elsize);
void CVMCfreeStub(void * ptr);
void * CVMCreallocStub(void * ptr, size_t size);
int CVMCisspaceStub(int c);
int CVMCisalnumStub(int c);
int CVMCisprintStub(int c);
void CVMCqsortStub(void * array, size_t numElements, size_t elementSize,
		   int (* compareFunc)(const void *, const void *));

#undef memset
#define memset CVMCmemsetStub
#undef memcpy
#define memcpy CVMCmemcpyStub
#undef memmove
#define memmove CVMCmemmoveStub
#undef strcmp
#define strcmp CVMCstrcmpStub
#undef strncmp
#define strncmp CVMCstrncmpStub
#undef strlen
#define strlen CVMCstrlenStub
#undef strcpy
#define strcpy CVMCstrcpyStub
#undef strncpy
#define strncpy CVMCstrncpyStub
#undef strdup
#define strdup CVMCstrdupStub
#undef strcat
#define strcat CVMCstrcatStub
#undef strncat
#define strncat CVMCstrncatStub
#undef strstr
#define strstr CVMCstrstrStub
#undef strchr
#define strchr CVMCstrchrStub
#undef strrchr
#define strrchr CVMCstrrchrStub
#undef strtol
#define strtol CVMCstrtolStub
#undef malloc
#define malloc CVMCmallocStub
#undef calloc
#define calloc CVMCcallocStub
#undef free
#define free CVMCfreeStub
#undef realloc
#define realloc CVMCreallocStub
#undef isspace
#define isspace CVMCisspaceStub
#undef isalnum
#define isalnum CVMCisalnumStub
#undef isprint
#define isprint CVMCisprintStub
#undef qsort
#define qsort CVMCqsortStub

#include "javavm/include/porting/ansi/setjmp.h"

#undef longjmp
#define longjmp CVMlongjmpStub
void CVMlongjmpStub(jmp_buf env, int val);

#include "javavm/include/porting/ansi/time.h"

#undef mktime
#define mktime CVMmktimeStub
time_t CVMmktimeStub(struct tm * timeptr);

#include "javavm/include/porting/doubleword.h"

#undef CVMlongDiv
#define CVMlongDiv(op1, op2)	\
	(CVMlongDivStub((op1), (op2)))
CVMJavaLong CVMlongDivStub(CVMJavaLong op1, CVMJavaLong op2);

/* 
 * The following jvm2long, long2jvm, jvm2double, double2jvm and
 * CVMmemCopy64 are used in conjunction with the 'raw' field of struct
 * JavaVal32.  Because the raw field is of type CVMAddr, the type of
 * location has to be changed accordingly.
 *
 * CVMAddr is 8 byte on 64 bit platforms and 4 byte on 32 bit
 * platforms.
 */
#undef CVMjvm2Long
#define CVMjvm2Long(location)	\
	CVMjvm2LongStub((location))
CVMJavaLong CVMjvm2LongStub(const CVMAddr location[2]);

#undef CVMlong2Jvm
#define CVMlong2Jvm(location, val)	\
	CVMlong2JvmStub((location), (val))
void CVMlong2JvmStub(CVMAddr location[2], CVMJavaLong val);

#undef CVMjvm2Double
#define CVMjvm2Double(location)	\
	CVMjvm2DoubleStub((location))
CVMJavaDouble CVMjvm2DoubleStub(const CVMAddr location[2]);

#undef CVMdouble2Jvm
#define CVMdouble2Jvm(location, val)	\
	CVMdouble2JvmStub((location), (val))
void CVMdouble2JvmStub(CVMAddr location[2], CVMJavaDouble val);

#undef CVMmemCopy64
#define CVMmemCopy64(to, from)	\
	CVMmemCopy64Stub((to), (from))
void CVMmemCopy64Stub(CVMAddr to[2], const CVMAddr from[2]);

#endif

#endif /* _INCLUDED_PORTING_DEBUG_H */
