/*
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
 * This is all pretty simple stuff.  It is not likely to be banged on
 * frequently enough to be a performance issue, unless the underlying
 * primitives are.  Implementing things:
 *
 * HPI function      win32
 * --------------------------------------------------------------------
 * CVMmemMap()	     VirtualAlloc(... MEM_RESERVE, PAGE_READONLY)
 * CVMmemUnmap()     VirtualFree(... MEM_RELEASE)
 * CVMmemCommit()    VirtualAlloc(... MEM_COMMIT, PAGE_READWRITE)
 * CVMmemDecommit()  VirtualFree(... MEM_DECOMMIT)
 */

#include <windows.h>

#include "javavm/include/porting/memory.h"
#include "javavm/include/assert.h"
#include "javavm/include/globals.h"
#include "javavm/include/utils.h"

#define WIN32_VIRTUAL_ALLOC_ALIGNMENT (64 * 1024)

extern void*
CVMmemalignAlloc(size_t alignment, size_t size)
{
    CVMassert(alignment == WIN32_VIRTUAL_ALLOC_ALIGNMENT);
    return VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);
}

extern void
CVMmemalignFree(void* memalignAllocedSpace)
{
    VirtualFree(memalignAllocedSpace, 0, MEM_RELEASE);
}

#if CVM_USE_MMAP_APIS

/* #define DEBUG_MMAP to turn on tracing and debugging code below. */
#undef DEBUG_MMAP

static size_t memGrainSize;

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

/* HPI Functions: */

/* Purpose: Initialized the memory mapping subsystem. 
   Returns: TRUE if initialization is successful.
*/
CVMBool CVMmemInit(void)
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    memGrainSize = si.dwPageSize;
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
	DWORD protectAttr;

	/* For WinCE 5.0 and earlier, calling VirtualAlloc() with PAGE_READONLY
	   will ensure that the virtual address range is reserved from within
	   the 32M process space.  This range is protected from reads and
	   writes by other non-trusted processes.  On WinCE systems where all
	   processes are trusted, other processes can still read and write into
	   our process via WinCE system calls.

	   If instead of PAGE_READONLY, we use PAGE_NOACCESS together with
	   MEM_RESERVE and we request a size greater than 2M, than the address
	   range will be reserved from shared memory.  Note that shared memory
	   can be read and written to by other processes as long as they know
	   the address.  This could open the heap data to security
	   vulnerabilities, and expose the VM to attacks more easily.

	   Note that PAGE_READONLY doesn't actually mean that the page is
	   read only later, nor does PAGE_NOACCESS means that the page is
	   in accessible.  Before the pages are used, they will be committed
	   in CVMmemCommit() using VirtualAlloc(... PAGE_READWRITE).  Hence,
	   we'll have read write access to the committed heap pages.

	   Relevant MSDN references:
	   http://msdn2.microsoft.com/EN-US/library/aa454885.aspx#effective_memory_storage_power_mgmt_wm5_topic2
	   http://msdn2.microsoft.com/en-us/library/aa908768.aspx 
	*/

	protectAttr = PAGE_READONLY;
#ifdef WINCE
	if (CVMglobals.target.useLargeMemoryArea) {
	    protectAttr = PAGE_NOACCESS;
	}
#endif
	/* Returns NULL on failure */
	mappedAddr = VirtualAlloc(0, requestedSize, MEM_RESERVE, protectAttr);
    }
    *mappedSize = (mappedAddr != NULL) ? requestedSize : 0;

#ifdef DEBUG_MMAP
    if (mappedAddr != NULL) {
	CVMconsolePrintf(
	    "CVMmemMap: 0x%x bytes at 0x%x (request: 0x%x bytes)\n",
	    *mappedSize, mappedAddr, requestedSize);
    } else {
	CVMconsolePrintf("CVMmemMap failed: Error %d, (request: 0x%x bytes)\n",
			 GetLastError(), requestedSize);
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
	success = VirtualFree(requestedAddr, 0, MEM_RELEASE);
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
	CVMconsolePrintf("CVMmemUnmap failed: Error %d, (request: 0x%x bytes at 0x%x)\n",
                         GetLastError(), requestedSize, requestedAddr);
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

#define CONST_32MB      (0x02000000)
#define CONST_32MB_MASK (0xFE000000)
#define ROUND_UP_32MB(x) (((DWORD)(x) & CONST_32MB_MASK) + CONST_32MB)

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
    MEMORY_BASIC_INFORMATION mb;

    CVMassert(requestedSize == roundUpToGrain(requestedSize));
    CVMassert(requestedAddr ==
	      (void *)roundDownToGrain((CVMAddr)requestedAddr));

    if (requestedSize != 0) {
        committedAddr = VirtualAlloc(requestedAddr, requestedSize, MEM_COMMIT,
                                     PAGE_READWRITE);
#ifdef WINCE
        if (committedAddr == NULL && ((DWORD)requestedAddr + requestedSize) >=
            ROUND_UP_32MB(requestedAddr)) {
            /* hitting/crossing 32MB boundary, need to break up the commit */
            size_t origSize = requestedSize;
            void *newAddr = requestedAddr;
            size_t pageSize = CVMmemPageSize();

            while(((DWORD)newAddr + origSize) >= ROUND_UP_32MB(newAddr)) {
                size_t newSize = ROUND_UP_32MB(newAddr) - (DWORD)newAddr;
                if (newSize >= pageSize * 2) {
                    /* Sometimes, for whatever reason, if you
                     * allocate right up to the 32MB boundary it returns
                     * INVALID PARAMETER error. So, back off a page 
                     */
                    newSize -= pageSize;
                }
                committedAddr = VirtualAlloc(newAddr, newSize, MEM_COMMIT,
                                             PAGE_READWRITE);
                if (committedAddr == NULL) {
                    break;
                }
                newAddr = (void*)((DWORD)newAddr + newSize);
                origSize -= newSize;
            }

#if _WIN32_WCE < 600
            while(committedAddr != NULL && origSize > 0) {
                /* Some residual pages to commit */
                /* WinCE < 6.0 fails on commits that are too big, where too big
                 * is some unknown value I can't seem to pin down. So just do
                 * it a page at a time.
                 */
                committedAddr = VirtualAlloc(newAddr, pageSize, MEM_COMMIT,
                                             PAGE_READWRITE);
                origSize -= pageSize;
                newAddr = (void *)((DWORD)newAddr + pageSize);
            }
#else
            if (committedAddr != NULL) {
                committedAddr = VirtualAlloc(newAddr, origSize, MEM_COMMIT,
                                             PAGE_READWRITE);
            }
#endif
            if (committedAddr != NULL) {
                committedAddr = requestedAddr;
            }
        }
#endif
    }
    *committedSize = (committedAddr != NULL) ?
	(CVMassert(committedAddr == requestedAddr), requestedSize) : 0;

#ifdef WINCE
    if (committedAddr == NULL) {
        /* Must have had an error committing, attempt to decommit anything we
         * committed
         */
        size_t decommittedSize;
        CVMmemDecommit(requestedAddr, requestedSize, &decommittedSize);
    }
#endif
#ifdef DEBUG_MMAP
    if (committedAddr != NULL) {
	CVMconsolePrintf(
	    "CVMmemCommit: 0x%x bytes at 0x%x (request: 0x%x bytes at 0x%x)\n",
	    *committedSize, committedAddr, requestedSize, requestedAddr);
    } else {
	CVMconsolePrintf(
	    "CVMmemCommit failed: Error %d, (request: 0x%x bytes at 0x%x)\n",
	    GetLastError(), requestedSize, requestedAddr);
        CVMconsolePrintf("CVMmemCommit: State:\n");
        VirtualQuery(requestedAddr, &mb, sizeof(mb));
        CVMconsolePrintf("  Base: 0x%x\n  AllocBase: 0x%x\n  AllocProtect: 0x%x\n  Size: 0x%x\n  State: 0x%x\n  Protect: 0x%x\n  Type: 0x%x\n",
                         (int)mb.BaseAddress, mb.AllocationBase,
                         mb.AllocationProtect, mb.RegionSize, mb.State,
                         mb.Protect, mb.Type);
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
    CVMBool success = CVM_FALSE;

    CVMassert(requestedSize == roundDownToGrain(requestedSize));
    CVMassert(requestedAddr == (void *)roundUpToGrain((CVMAddr)requestedAddr));

    if (requestedSize != 0) {
        success = VirtualFree(requestedAddr, requestedSize, MEM_DECOMMIT);
#ifdef WINCE
        if (!success && ((DWORD)requestedAddr + requestedSize) >=
            ROUND_UP_32MB(requestedAddr)) {
            /* crossing 32MB boundary, need to break up the decommit */
            size_t origSize = requestedSize;
            void *newAddr = requestedAddr;

            while(((DWORD)newAddr + origSize) >= ROUND_UP_32MB(newAddr)) {
                size_t newSize = ROUND_UP_32MB(newAddr) - (DWORD)newAddr;
                success = VirtualFree(newAddr, newSize, MEM_DECOMMIT);
                if (!success) {
                    break;
                }
                newAddr = (void*)((DWORD)newAddr + newSize);
                origSize -= newSize;
            }

            if (success && origSize > 0) {
                /* Some residual pages to decommit */
                success = VirtualFree(newAddr, origSize, MEM_DECOMMIT);
            }
        }
#endif
    }
    if (success) {
	decommittedAddr = requestedAddr;
	*decommittedSize = requestedSize;
    } else {
	*decommittedSize = 0;
    }

#ifdef DEBUG_MMAP
    if (success) {
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
