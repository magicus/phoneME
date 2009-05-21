/*
 * @(#)porting_debug.c	1.17 06/10/10
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

/*
 * This file contains debug wrappers for porting layer functions.
 * This allows setting breakpoints even in interfaces that are
 * implemented by macros.
 *
 * Some Exceptions:
 *
 * - interfaces that take a variable number of arguments
 * - setjmp
 *
 * This file is compiled only with the CVM_DEBUG option on.
 */
#define CVM_PORTING_DEBUG_IMPL
#include "javavm/include/clib.h"

#ifdef CVM_PORTING_DEBUG_STUBS

void* 
CVMCmemsetStub(void* s, int c, size_t n)
{
    return memset(s, c, n);
}

void* 
CVMCmemcpyStub(void* d, const void* s, size_t n)
{
    return memcpy(d, s, n);
}

void* 
CVMCmemmoveStub(void* d, const void* s, size_t n)
{
    return memmove(d, s, n);
}

int
CVMCstrcmpStub(const char* s1, const char* s2)
{
    return strcmp(s1, s2);
}

int
CVMCstrncmpStub(const char* s1, const char* s2, size_t n)
{
    return strncmp(s1, s2, n);
}

size_t
CVMCstrlenStub(const char* s)
{
    return strlen(s);
}

char*    
CVMCstrcpyStub(char* dst, const char* src)
{
    return strcpy(dst, src);
}

char*    
CVMCstrncpyStub(char* dst, const char* src, size_t n)
{
    return strncpy(dst, src, n);
}

char*    
CVMCstrdupStub(const char* str)
{
    return strdup(str);
}

char*    
CVMCstrcatStub(char* dst, const char* src)
{
    return strcat(dst, src);
}

char*    
CVMCstrncatStub(char* dst, const char* src, size_t n)
{
    return strncat(dst, src, n);
}

char*
CVMCstrstrStub(const char* s1, const char* s2)
{
    return strstr(s1, s2);
}

char*
CVMCstrchrStub(const char* s, int c)
{
    return strchr(s, c);
}

char*
CVMCstrrchrStub(const char* s, int c)
{
    return strrchr(s, c);
}

long
CVMCstrtolStub(const char *str, char **endptr, int base)
{
    return strtol(str, endptr, base);
}

void* 
CVMCmallocStub(size_t size)
{
    return malloc(size);
}

void* 
CVMCcallocStub(size_t nelem, size_t elsize)
{
    return calloc(nelem, elsize);
}

void
CVMCfreeStub(void* ptr)
{
    free(ptr);
}

void* 
CVMCreallocStub(void* ptr, size_t size)
{
    return realloc(ptr, size);
}

int CVMCisspaceStub(int c)
{
    return isspace(c);
}

int CVMCisalnumStub(int c)
{
    return isalnum(c);
}

int CVMCisprintStub(int c)
{
    return isprint(c);
}

void CVMCqsortStub(void* array, size_t numElements, size_t elementSize,
		   int (*compareFunc)(const void *, const void *))
{
   qsort(array, numElements, elementSize, compareFunc);
}

#include "javavm/include/porting/ansi/setjmp.h"

void CVMlongjmpStub(jmp_buf env, int val)
{
    longjmp(env, val);
}

#include "javavm/include/porting/ansi/time.h"

time_t CVMmktimeStub(struct tm *timeptr)
{
    return mktime(timeptr);
}

#include "javavm/include/porting/doubleword.h"

CVMJavaLong
CVMlongDivStub(CVMJavaLong op1, CVMJavaLong op2)
{
    return CVMlongDiv(op1, op2);
}

CVMJavaLong
CVMjvm2LongStub(const CVMAddr location[2])
{
    return CVMjvm2Long(location);
}

void
CVMlong2JvmStub(CVMAddr location[2], CVMJavaLong val)
{
    CVMlong2Jvm(location, val);
}

CVMJavaDouble
CVMjvm2DoubleStub(const CVMAddr location[2])
{
    return CVMjvm2Double(location);
}

void
CVMdouble2JvmStub(CVMAddr location[2], CVMJavaDouble val)
{
    CVMdouble2Jvm(location, val);
}

void
CVMmemCopy64Stub(CVMAddr to[2], const CVMAddr from[2])
{
    CVMmemCopy64(to, from);
}

#include "javavm/include/porting/float.h"
#include "javavm/include/porting/int.h"

#include "javavm/include/porting/sync.h"

typedef void (*CVMsyncHook)(void *);

static CVMsyncHook CVMmutexInitHook;
static CVMsyncHook CVMmutexDestroyHook;
static CVMsyncHook CVMcondvarInitHook;
static CVMsyncHook CVMcondvarDestroyHook;

void
CVMsetMutexHooks(CVMsyncHook init, CVMsyncHook destroy)
{
    CVMmutexInitHook = init;
    CVMmutexDestroyHook = destroy;
}

void
CVMsetCondvarHooks(CVMsyncHook init, CVMsyncHook destroy)
{
    CVMcondvarInitHook = init;
    CVMcondvarDestroyHook = destroy;
}

CVMBool
CVMmutexInitStub(CVMMutex* m)
{
    CVMBool result = CVMmutexInit(m);
    if (result && CVMmutexInitHook != NULL) {
	(*CVMmutexInitHook)(m);
    }
    return result;
}

void
CVMmutexDestroyStub(CVMMutex* m)
{
    CVMmutexDestroy(m);
    if (CVMmutexDestroyHook != NULL) {
	(*CVMmutexDestroyHook)(m);
    }
}

CVMBool
CVMcondvarInitStub(CVMCondVar *c, CVMMutex* m)
{
    CVMBool result = CVMcondvarInit(c, m);
    if (result && CVMcondvarInitHook != NULL) {
	(*CVMcondvarInitHook)(c);
    }
    return result;
}

void
CVMcondvarDestroyStub(CVMCondVar *c)
{
    CVMcondvarDestroy(c);
    if (CVMcondvarDestroyHook != NULL) {
	(*CVMcondvarDestroyHook)(c);
    }
}

#endif /* CVM_DEBUG */
