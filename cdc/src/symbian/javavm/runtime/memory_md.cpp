/*
 * @(#)memory_md.cpp	1.6 06/10/10
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

extern "C" {
#include "javavm/include/porting/memory.h"
#include "javavm/include/porting/sync.h"
#include "javavm/include/porting/system.h"
#include "javavm/include/porting/ansi/stdlib.h"
#include "javavm/include/porting/ansi/string.h"
}

#include <assert.h>
#include <e32std.h>
#include <string.h>
#include <stdio.h>

typedef void * malloc_t(size_t n);
typedef void * calloc_t(size_t, size_t);
typedef char * strdup_t(const char *);

static calloc_t calloc0, mt_calloc;
static malloc_t malloc0, mt_malloc;
static strdup_t strdup0, mt_strdup;
static int init = 0;
static const struct {
    malloc_t *m;
    calloc_t *c;
    strdup_t *s;
} mem_vector0 = {
    &malloc0,
    &calloc0,
    &strdup0
}, mt_mem_vector = {
    &mt_malloc,
    &mt_calloc,
    &mt_strdup
}, *mem_vector = &mem_vector0;

static RCriticalSection cs;

static void
lock()
{
    cs.Wait();
}

static void
unlock()
{
    cs.Signal();
}

static RHeap *heap;

#define MAX_MB 128
static void
malloc_init()
{
    if (!init) {
	if (cs.CreateLocal() != KErrNone) {
	    CVMsystemPanic("Could not initialize malloc\n");
	}

#if !defined(CVM_HW)
	heap = User::ChunkHeap(NULL, 1 * 1024 * 1024, MAX_MB * 1024 * 1024);
#else
	/* For hardware execution, the heap needs to be executable. */
	static RChunk rc;
	if (rc.CreateLocalCode(1 * 1024 * 1024, MAX_MB * 1024 * 1024) !=
	    KErrNone) {
	    heap = NULL;
	} else {
	    heap = User::ChunkHeap(rc, 1 * 1024 * 1024);
	}
#endif
	if (heap == NULL) {
	    fprintf(stderr, "Could not initialize %dMB malloc heap\n",
		MAX_MB);
	    CVMsystemPanic("Could not initialize malloc\n");
	} else {
	    fprintf(stderr, "Initialized %dMB malloc heap OK\n",
		MAX_MB);
	}
	// Should we have each attached thread do an Open?
	heap->Open();	// shared access
	mem_vector = &mt_mem_vector;
	init = 1;
    }
}

RHeap *
SYMBIANgetGlobalHeap()
{
    if (!init) {
	malloc_init();
    }
    return heap;
}

static void *
malloc0(size_t n)
{
    assert(!init);
    malloc_init();
    return (*mem_vector->m)(n);
}

static void *
mt_malloc(size_t n)
{
    void *r;
    lock();
    r = heap->Alloc(n);
    unlock();
    return r;
}

void *
CVMmalloc(size_t n)
{
    return (*mem_vector->m)(n);
}

static void *
calloc0(size_t nelem, size_t elsize)
{
    assert(!init);
    malloc_init();
    return (*mem_vector->c)(nelem, elsize);
}

static void *
mt_calloc(size_t nelem, size_t elsize)
{
    void *r;
    size_t n = nelem * elsize;
    lock();
    r = heap->Alloc(n);
    unlock();
    if (r != NULL) {
	memset(r, 0, n);
    }
    return r;
}

void *
CVMcalloc(size_t nelem, size_t elsize)
{
    return (*mem_vector->c)(nelem, elsize);
}

void *
CVMrealloc(void *ptr, size_t n)
{
    void *r;
    lock();
    r = heap->ReAlloc(ptr, n);
    unlock();
    return r;
}

void
CVMfree(void *p)
{
    lock();
    heap->Free(p);
    unlock();
}

static char *
strdup0(const char *s)
{
    assert(!init);
    malloc_init();
    return (*mem_vector->s)(s);
}

static char *
mt_strdup(const char *s)
{
    char *r = (char *)mt_malloc(strlen(s) + 1);
    if (r != NULL) {
	strcpy(r, s);
    }
    return r;
}

char *
CVMstrdup(const char *s)
{
    return (*mem_vector->s)(s);
}


#include "memory_impl.h"

void *operator new (size_t n, CVMHeapID dummy)
{
    return CVMmalloc(n);
}
