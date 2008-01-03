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

#include <pcsl_memory.h>
#include <donuts.h>
#include <string.h>

/*
 * Test simple memory allocation 
 * This test checks to see that NULL is not returned from a pcsl_mem_malloc
 * call.  (Note:  NULL may be a valid return in a very full/limited
 * memory system)
 */
void testAllocation() {
    void * buffer = pcsl_mem_malloc(1000);

    assertTrue("failed to allocate a 1000 byte buffer", 
	       (buffer != NULL));

    pcsl_mem_free(buffer);
}

/*
 * Test the pcsl_mem_malloc method and pcsl_mem_get_free_heap method
 * This test ensures that after a pcsl_mem_malloc call for 1000 bytes
 * that the heap size available is reduced by 1000 bytes.
 * note: acutally, the heap pcsl_memory.c impl adds 4 guard bytes, so
 * we have to take that into account...
 */
void testMalloc() {
    int spcBefore;
    int spcAfter;
    void * buffer;

    spcBefore = pcsl_mem_get_free_heap();

    buffer = pcsl_mem_malloc(1000);
    assertTrue("failed to allocate a 1000 byte buffer", 
	       (buffer != NULL));

    spcAfter = pcsl_mem_get_free_heap();

    if (spcAfter != -1) {
        assertTrue("pcsl_mem_malloc & heap_size_available mis-match",
		   (spcBefore - 1004 == spcAfter ||
		    spcBefore - 1000 == spcAfter));
    }

    spcBefore = pcsl_mem_get_free_heap();

    pcsl_mem_free(buffer);

    spcAfter = pcsl_mem_get_free_heap();

    if (spcAfter != -1) {
        assertTrue("pcsl_mem_free & heap_size_available mis-match",
		   (spcBefore + 1004 == spcAfter ||
		    spcBefore + 1000 == spcAfter));
    }
}

/*
 * Test the pcsl_mem_allocate_chunk method
 * Sanity check test to see if pcsl_mem_allocate_chunk works OK
 */
void testAllocateChunk() {
    void * buffer = pcsl_mem_allocate_chunk(1000, 1100, 1);

    assertTrue("failed to allocate a 1000 byte buffer with pcsl_mem_allocate_chunk", 
	       (buffer != NULL));

    pcsl_mem_free_chunk(buffer);
}

/*
 * Test the pcsl_mem_allocate_chunk and pcsl_mem_adjust_chunk methods
 */
void testModifyChunkSize() {

    void * buffer = pcsl_mem_allocate_chunk(100, 200, 1);
    int rv;

    /* see if we can make the chunk larger than maxsize */
    rv = pcsl_mem_adjust_chunk(buffer, 300);
    assertTrue("pcsl_mem_adjust_chunk returned buffer size > max_size",
	       (rv == 0));

    /* OK...now try to increase the size to something reasonable */
    rv = pcsl_mem_adjust_chunk(buffer, 160);
    assertTrue("pcsl_mem_adjust_chunk returned incorrect previous buf size",
	       (rv == 100));

    /* OK...now decrease the size to 100, check to see old size was 160 */
    rv = pcsl_mem_adjust_chunk(buffer, 100);
    assertTrue("pcsl_mem_adjust_chunk returned incorrect previous buf size",
	       (rv == 160));

    pcsl_mem_free_chunk(buffer);
}

/*
 * Test that a repeated pcsl_mem_initialize call is ignored
 */
void testDoubleInit() {
    void *buffer = pcsl_mem_malloc(1000);
    int i, rv;
    int *arr;

    arr = (int*) buffer;

    for (i = 0; i < 100; i++) {
        *(arr + i) = 727;
    } 

    rv = pcsl_mem_initialize(NULL, 4000);
    assertTrue("second pcsl_mem_initialize call fails",
	       rv == 0);
    
    assertTrue("data corrupt after pcsl_mem_initialize call",
	       *arr == 727 &&
	       *(arr + 50) == 727);
    pcsl_mem_free(buffer);
}

/*
 * Test the fact that global heap size never changes
 */
void testHeapSize() {
     void *buffer;
     int rv, rv2, rv3;

     rv = pcsl_mem_get_total_heap();
   
     buffer = pcsl_mem_malloc(1000);

     rv2 = pcsl_mem_get_total_heap();

     pcsl_mem_free(buffer);

     rv3 = pcsl_mem_get_total_heap();

     assertTrue("pcsl total heap size should never change, but did",
	        (rv == rv2) && (rv2 == rv3));
}  

/*
 * Test the Calloc method.
 * Try allocating space for some structs and ensure that the
 * fields are initialized to zero
 */
void testCalloc() {

    struct point {
        int x;
        int y;      
    };

    struct point *start;
    struct point *pt;

    start = pcsl_mem_calloc(sizeof(struct point), 10);
    
    pt = start;
    
    assertTrue("pcsl_mem_calloc failed to zero allocated bytes",
	       (pt->x == 0) && (pt->y ==0));
    
    pt += 9;
    
    assertTrue("pcsl_mem_calloc failed to zero allocated bytes",
	       (pt->x == 0) && (pt->y ==0));
    
    pcsl_mem_free(start);
}

/*
 * Test pcsl_mem_realloc, and use it to free memory
 *
 * This test ensures that after a pcsl_mem_malloc call for 1000 bytes
 * and a pcsl_mem_realloc call for 1500 bytes that heap size available 
 * is reduced by 1500 bytes.
 * note: acutally, the heap pcsl_memory.c impl adds 4 guard bytes, so
 * we have to take that into account in our error checking...
 */
void testRealloc() {
    int spcBefore;
    int spcAfter;

    void * buffer;

    spcBefore = pcsl_mem_get_free_heap();

    buffer = pcsl_mem_malloc(1000);
    assertTrue("failed to allocate a 1000 byte buffer", 
	       (buffer != NULL));

    spcAfter = pcsl_mem_get_free_heap();

    if (spcAfter != -1) {
        assertTrue("pcsl_mem_malloc & heap_size_available mis-match",
		   (spcBefore - 1004 == spcAfter ||
		    spcBefore - 1000 == spcAfter));
    }

    spcBefore = spcAfter;

    buffer = pcsl_mem_realloc(buffer, 1500);

    spcAfter = pcsl_mem_get_free_heap();

    if (spcAfter != -1) {
        assertTrue("pcsl_mem_realloc & heap_size_available mis-match",
		   (spcBefore - 504 == spcAfter ||
		    spcBefore - 500 == spcAfter));
    }
    spcBefore = spcAfter;

    pcsl_mem_realloc(buffer, 0);

    spcAfter = pcsl_mem_get_free_heap();

    if (spcAfter != -1) {
        assertTrue("pcsl_mem_realloc (free) & heap_size_available mis-match",
		   (spcBefore + 1504 == spcAfter ||
		    spcBefore + 1500 == spcAfter));
    }
}

/*
 * test pcsl_mem_strdup function works properly and allocated memory for copied string
 */
void testStrdup() {

    int spcBefore, spcAfter;
    char* str1 = "a 20 character string";
    char* str2;

    spcBefore = pcsl_mem_get_free_heap();

    str2 = (char*)pcsl_mem_strdup(str1);

    spcAfter = pcsl_mem_get_free_heap();

    assertTrue("pcsl_mem_strdup failed:  str1 and str2 not equal!",
	       strncmp(str1, str2, 20) == 0);

    if (spcAfter != -1) {
        assertTrue("pcsl_mem_strdup did not allocate any new memory", 
		   (spcAfter + 20) <= spcBefore);
    }
    pcsl_mem_free(str2);
}

/*
 * Unit test framework entry point for this set of unit tests.
 *
 */
void testMem_runTests() {
  pcsl_mem_initialize(NULL, 5000);

  testAllocation();
  testMalloc();
  testAllocateChunk();
  /*
   * comment out this test. The implementation changes the
   * specified max size, to align with a page
   * need revisit: what the new max size is. So we can't
   * compare against it.
   */
  /*
  testModifyChunkSize();
  */
  testDoubleInit();
  testHeapSize();
  testCalloc();
  testRealloc();
  testStrdup();

  pcsl_mem_finalize();
}
