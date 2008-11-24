/*
 * @(#)memory_md.c	1.3 06/10/10
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
 * Implementation of primitive memory allocation.
 *
 * The only thing machine dependent about this allocator is how it
 * initially finds all of the possible memory, and how it implements
 * mapChunk() and unmapChunk().
 *
 * This is all pretty simple stuff.  It is not likely to be banged on
 * frequently enough to be a performance issue, unless the underlying
 * primitives are.  Implementing things:
 *
 * HPI function      linux
 * --------------------------------------------------------------------
 * CVMmemMap()	     mmap(... MAP_NORESERVE ...)
 * CVMmemUnmap()     munmap()
 * CVMmemCommit()    mmap(...)
 * CVMmemDecommit()  mmap(... MAP_NORESERVE ...)
 */

#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "javavm/include/porting/memory.h"
#include "javavm/include/utils.h"

#if CVM_USE_MMAP_APIS

#ifndef MAP_ANONYMOUS
static int devZeroFD;
#endif

#define PROT_ALL (PROT_READ|PROT_WRITE|PROT_EXEC)

#ifndef MAP_NORESERVE
#define MAP_NORESERVE 0
#endif

#ifndef MAP_FAILED
#define MAP_FAILED ((caddr_t)-1)
#endif

static size_t memGrainSize;

/* #define DEBUG_MMAP to turn on tracing and debugging code below. */
#undef DEBUG_MMAP


#ifdef CVM_DEBUG_ASSERTS
/*
 * Mem size rounding is expected to be done by the caller.  We need to
 * assert here that the sizes passed to this level is rounded properly
 * to a multiple of memGrainSize.
 */
static long
roundUpToGrain(long value)
{
    return (value + memGrainSize - 1) & ~(memGrainSize - 1);
}

static long
roundDownToGrain(long value)
{
    return value & ~(memGrainSize - 1);
}
#endif /* CVM_DEBUG_ASSERTS */


/*
 * Map a chunk of memory.  Return the address of the base if successful,
 * 0 otherwise.  We do not care where the mapped memory is, and can't
 * even express a preference in the current HPI.  If any platforms
 * require us to manage addresses of mapped chunks explicitly, that
 * must be done below the HPI.
 */
static char *
mapChunk(long length)
{
    char *ret;

#if defined(MAP_ANONYMOUS)
     ret = (char *) mmap(0, length, PROT_ALL,
                         MAP_NORESERVE | MAP_PRIVATE | MAP_ANONYMOUS,
                         -1, (off_t) 0);
#else
    ret = (char *) mmap(0, length, PROT_ALL, MAP_NORESERVE|MAP_PRIVATE,
		   devZeroFD, (off_t) 0);
#endif
    return (ret == MAP_FAILED ? NULL : ret);
}

/*
 * Map a chunk of memory at a specific address and reserve swap space
 * for it.  This is currently only used to remap space previously mapped
 * MAP_NORESERVE, reserving swap and getting native error handling.  We
 * assume that all alignment and rounding has been done by the caller.
 * Return 1 if successful and 0 otherwise.
 */
static char *
mapChunkReserve(char *addr, long length)
{
    char *ret;
#if defined(MAP_ANONYMOUS)
     ret = (char *) mmap(addr, length, PROT_ALL,
                         MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS,
                         -1, (off_t) 0);
#else
    ret = (char *) mmap(addr, length, PROT_ALL, MAP_FIXED|MAP_PRIVATE,
                        devZeroFD, (off_t) 0);
#endif
    return (ret == MAP_FAILED ? NULL : ret);
}

/*
 * Map a chunk of memory at a specific address and reserve swap space
 * for it.  This is currently only used to remap space previously mapped
 * MAP_RESERVE, unreserving swap and getting native error handling.  We
 * assume that all alignment and rounding has been done by the caller.
 * Return 1 if successful and 0 otherwise.
 */
static char *
mapChunkNoreserve(char *addr, long length)
{
    char *ret;

#if defined(MAP_ANONYMOUS)
    ret = (char *) mmap(addr, length, PROT_ALL,
                        MAP_FIXED | MAP_PRIVATE |
                        MAP_NORESERVE | MAP_ANONYMOUS,
                        -1, (off_t) 0);
#else
    ret = (char *) mmap(addr, length, PROT_ALL,
			MAP_FIXED|MAP_PRIVATE|MAP_NORESERVE,
                        devZeroFD, (off_t) 0);
#endif
    return (ret == MAP_FAILED ? NULL : ret);
}

/*
 * Unmap a chunk of memory.  Return 1 if successful, 0 otherwise.  We
 * currently don't do any alignment or rounding, assuming that we only
 * will unmap chunks that have previously been returned by mapChunk().
 */
static CVMBool
unmapChunk(void *addr, long length)
{
    return (munmap(addr, length) == 0);
}

/* HPI Functions: */

/* Purpose: Initialized the memory mapping subsystem. 
   Returns: TRUE if initialization is successful.
*/
CVMBool CVMmemInit(void)
{
    /*
     * Set system-specific variables used by mem allocator
     */
    CVMassert(memGrainSize == 0);
    memGrainSize = (size_t) sysconf(_SC_PAGESIZE);

#if !defined(MAP_ANONYMOUS)
    devZeroFD = open("/dev/zero", O_RDWR);
    if (devZeroFD == -1) {
	return CVM_FALSE;
    }
#endif /* !MAP_ANONYMOUS*/
    return CVM_TRUE;
}

/* Purpose: Returns the machine minimum page size that can be mapped,
            unmapped, committed, or decommitted.  The size is in units of
	    bytes. */
size_t CVMmemPageSize()
{
    CVMassert(memGrainSize != 0);
    return memGrainSize;
}

/* Purpose: Reserves a memory address range of the requested size for later use.
   Returns: The starting address of the reserved memory range if successful.
            The actual size of the reserved memory range will be set in
	    *mappedSize in units of bytes.
            Else if failed to map, NULL is returned and *mappedSize is set
	    to 0.
   Note: memory sizes must be in increments of the page size as returned
         by CVMmemPageSize() in units of bytes.
*/
void *CVMmemMap(size_t requestedSize, size_t *mappedSize)
{
    void *mappedAddr = NULL;

    CVMassert(requestedSize == roundUpToGrain(requestedSize));
    if (requestedSize != 0) {
	mappedAddr = mapChunk(requestedSize);  /* Returns 0 on failure */
    }
    *mappedSize = (mappedAddr != NULL) ? requestedSize : 0;

#ifdef DEBUG_MMAP
    if (mappedAddr != NULL) {
	CVMconsolePrintf(
	    "CVMmemMap: 0x%x bytes at 0x%x (request: 0x%x bytes)\n",
	    *mappedSize, mappedAddr, requestedSize);
    } else {
	CVMconsolePrintf("CVMmemMap failed: (request: 0x%x bytes)\n",
			 requestedSize);
    }
#endif
    return mappedAddr;
}

/* Purpose: Relinquishes a range of memory which was previously reserved using
            CVMmemMap().  The memory range may be a subset of the range
	    returned by CVMmemMap().  The range to relinquish is specified by
	    addr as the starting address of the range, and requestedSize as the
	    size of the range in units of bytes.
   Returns: The starting address of the unmmapped range is returned.
            The actual size of the unmapped range is set in *unmappedSize
	    in units of bytes.
	    Else, if failed to unmap, NULL is returned and *unmappedSize is
	    set to 0.
   Note: memory sizes must be in increments of the page size as returned
         by CVMmemPageSize() in units of bytes.
*/
void *CVMmemUnmap(void *requestedAddr, size_t requestedSize,
		  size_t *unmappedSize)
{
    void *unmappedAddr = NULL;
    CVMBool success = CVM_FALSE;

    CVMassert(requestedAddr != NULL);
    CVMassert(requestedSize == roundUpToGrain(requestedSize));

    if (requestedSize != 0) {
	success = unmapChunk(requestedAddr, requestedSize);
    }
    if (success) {
	unmappedAddr = requestedAddr;
	*unmappedSize = requestedSize;
    } else {
	*unmappedSize = 0;
    }

#ifdef DEBUG_MMAP
    if (success) {
        CVMconsolePrintf(
	    "CVMmemUnmap: 0x%x bytes at 0x%x (request: 0x%x bytes at 0x%x)\n",
            *unmappedSize, unmappedAddr, requestedSize, requestedAddr);
    } else {
	CVMconsolePrintf("CVMmemUnmap failed: (request: 0x%x bytes at 0x%x)\n",
	     requestedSize, requestedAddr);
    }
#endif
    return unmappedAddr;
}

/*
 * Commit/decommit backing store to a range of virtual memory.  This range
 * needs not be identical to a mapped range, but must be a subset of one.
 *
 * We do not validate that commitment requests cover already-mapped
 * memory, although in principle we could.  The size asked for here
 * is what the upper level has asked for.  We need to assert that it is
 * rounded properly.
 *
 * When you commit, you commit to the entire page (or whatever quantum
 * your O/S requires) containing the pointer, and return the beginning of
 * that page.  When you decommit, you decommit starting at the next page
 * *up* from that containing the pointer, except that decommitting from
 * a pointer to the beginning of the page operates on that page.
 */

/* Purpose: Commits a range of memory which was previously reserved using
            CVMmemMap().  The memory range may be a subset of the range
	    returned by CVMmemMap().  The range to commit is specified by
            addr as the starting address of the range, and requestedSize as the
	    size of the range in units of bytes.
   Returns: The starting address of the committed range is returned.
            The actual size of the committed range is set in *committedSize
	    in units of bytes.
	    Else, if failed to commit, NULL is returned and *committedSize is
	    set to 0.
   Note: memory sizes must be in increments of the page size as returned
         by CVMmemPageSize() in units of bytes.
	 Committed memory must be uncommitted using CVMmemDecommit() before it
	 is unmmapped with CVMmemUnmap().  If this order is not adhered to,
	 then the state of the committed memory will be undefined.
*/
void *CVMmemCommit(void *requestedAddr, size_t requestedSize,
		   size_t *committedSize)
{
    void *committedAddr = NULL;

    CVMassert(requestedSize == roundUpToGrain(requestedSize));
    CVMassert(requestedAddr ==
	      (void *)roundDownToGrain((CVMAddr)requestedAddr));

    if (requestedSize != 0) {
	committedAddr = mapChunkReserve(requestedAddr, requestedSize);
    }
    *committedSize = (committedAddr != NULL) ?
	(CVMassert(committedAddr == requestedAddr), requestedSize) : 0;

#ifdef DEBUG_MMAP
    if (committedAddr != NULL) {
	CVMconsolePrintf(
	    "CVMmemCommit: 0x%x bytes at 0x%x (request: 0x%x bytes at 0x%x)\n",
	    *committedSize, committedAddr, requestedSize, requestedAddr);
    } else {
	CVMconsolePrintf(
	    "CVMmemCommit failed: (request: 0x%x bytes at 0x%x)\n",
	    requestedSize, requestedAddr);
    }
#endif

    return committedAddr;
}

/* Purpose: Decommits a range of memory which was previously committed using
            CVMmemCommit().  The memory range may be a subset of the range
	    returned by CVMmemCommit().  The range to decommit is specified by
            addr as the starting address of the range, and requestedSize as the
	    size of the range in units of bytes.
   Returns: The starting address of the decommitted range is returned.
            The actual size of the decommitted range is set in *decommittedSize
	    in units of bytes.
	    Else, if failed to decommit, NULL is returned and *decommittedSize
	    is set to 0.
   Note: memory sizes must be in increments of the page size as returned
         by CVMmemPageSize() in units of bytes.
	 Committed memory must be uncommitted using CVMmemDecommit() before it
	 is unmmapped with CVMmemUnmap().  If this order is not adhered to,
	 then the state of the committed memory will be undefined.
*/
void *CVMmemDecommit(void *requestedAddr, size_t requestedSize,
		     size_t *decommittedSize)
{
    void *decommittedAddr = NULL;

    CVMassert(requestedSize == roundDownToGrain(requestedSize));
    CVMassert(requestedAddr == (void *)roundUpToGrain((CVMAddr)requestedAddr));

    if (requestedSize != 0) {
	decommittedAddr = mapChunkNoreserve(requestedAddr, requestedSize);
    }
    *decommittedSize = (decommittedAddr != NULL) ? requestedSize : 0;

#ifdef DEBUG_MMAP
    if (decommittedAddr != NULL) {
	CVMconsolePrintf(
            "CVMmemDecommit: 0x%x bytes at 0x%x (request: 0x%x bytes at 0x%x)\n",
            *decommittedSize, decommittedAddr, requestedSize, requestedAddr);
    } else {
	CVMconsolePrintf(
	    "CVMmemDecommit failed: (request: 0x%x bytes at 0x%x)\n",
	    requestedSize, requestedAddr);
    }
#endif

    return decommittedAddr;
}

#endif /* CVM_USE_MMAP_APIS */
