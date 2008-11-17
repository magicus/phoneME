/*
 * @(#)memory.h	1.16 06/10/10
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

#ifndef _INCLUDED_PORTING_MEMORY_H
#define _INCLUDED_PORTING_MEMORY_H

#include "javavm/include/porting/defs.h"
#include "javavm/include/porting/ansi/stddef.h"
#include "javavm/include/porting/vm-defs.h"

#include CVM_HDR_MEMORY_H

/*
 * The platform must define the following for aligned memory allocation.
 */
void* CVMmemalignAlloc(size_t alignment, size_t size);
void  CVMmemalignFree(void* memalignAllocedSpace);


#ifndef CVM_USE_MMAP_APIS
#define CVM_USE_MMAP_APIS  0 /* 1 to enable, 0 to disable */
#endif

#if CVM_USE_MMAP_APIS

/* The platform may choose to provide implementations for the following
   memory mapping functionality:

       CVMmemInit, CVMmemPageSize,
       CVMmemMap, CVMmemUnmap, CVMmemCommit, CVMmemDecommit

   If so, the platform should also define the CVM_USE_MMAP_APIS macro to
   be a non-zero value to enable their use in shared code.
 */

/* Purpose: Initialized the memory mapping subsystem. 
   Returns: TRUE if initialization is successful.
*/
CVMBool CVMmemInit();

/* Purpose: Returns the machine minimum page size that can be mapped,
            unmapped, committed, or decommitted.  The size is in units of
	    bytes. */
size_t CVMmemPageSize();

/* Purpose: Reserves a memory address range of the requested size for later use.
   Returns: The starting address of the reserved memory range if successful.
            The actual size of the reserved memory range will be set in
	    *mappedSize in units of bytes.
            Else if failed to map, NULL is returned and *mappedSize is set
	    to 0.
   Note: memory sizes must be in increments of the page size as returned
         by CVMmemPageSize() in units of bytes.
*/
void *CVMmemMap(size_t requestedSize, size_t *mappedSize);

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
void *CVMmemUnmap(void *addr, size_t requestedSize, size_t *unmappedSize);

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
void *CVMmemCommit(void *addr, size_t requestedSize, size_t *committedSize);


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
void *CVMmemDecommit(void *addr, size_t requestedSize,
		     size_t *decommittedSize);

#endif /* CVM_USE_MMAP_APIS */

/* The platform may choose to provide implementations for the following by
   defining the macros to platform specific functions.  If the platform
   does not define these macros, they will default to use the standard
   memmove implementation.  The macros and the prototypes of the functions
   they point to are as follows:

   void CVMmemmoveByte(void *s1, const void *s2, size_t n);
   void CVMmemmoveBoolean(void *s1, const void *s2, size_t n);
   void CVMmemmoveShort(void *s1, const void *s2, size_t n);
   void CVMmemmoveChar(void *s1, const void *s2, size_t n);
   void CVMmemmoveInt(void *s1, const void *s2, size_t n);
   void CVMmemmoveFloat(void *s1, const void *s2, size_t n);
   void CVMmemmoveRef(void *s1, const void *s2, size_t n);
   void CVMmemmoveLong(void *s1, const void *s2, size_t n);
   void CVMmemmoveDouble(void *s1, const void *s2, size_t n);

   NOTE: The return type does not strictly have to be void (as is the
         case with the ansi implementation of memmove which returns
         a void * instead).  It is OK to have the actual implementation
         return a non void value because this value is not used in CVM
         and will be ignored.
*/

/* Provide defaults if the target platform does not provide their
   own memmove functions: */

#ifndef CVMmemmoveByte
#define CVMmemmoveByte          memmove
#endif
#ifndef CVMmemmoveBoolean
#define CVMmemmoveBoolean       memmove
#endif
#ifndef CVMmemmoveShort
#define CVMmemmoveShort         memmove
#endif
#ifndef CVMmemmoveChar
#define CVMmemmoveChar          memmove
#endif
#ifndef CVMmemmoveInt
#define CVMmemmoveInt           memmove
#endif
#ifndef CVMmemmoveFloat
#define CVMmemmoveFloat         memmove
#endif
#ifndef CVMmemmoveRef
#define CVMmemmoveRef           memmove
#endif

/*
 * CVMDprivateDefaultNoBarrierArrayCopy() in directmem.h 
 * does not fit for a 64-bit port, because
 * sizeof(jlong) and sizeof(jdouble) in
 * CVMmemmove## accessor (dstElements_, srcElements_,(len) * sizeof(jType));
 * does not reflect that two java words are concerned.
 *
 * So have 64-bit specific versions of CVMmemmove{Long,Double} that
 * double the length argument for the memmove.
 */
#ifdef CVM_64
#ifndef CVMmemmoveLong
#define CVMmemmoveLong(s1,s2,len)    memmove(s1,s2,(len)*2)
#endif
#ifndef CVMmemmoveDouble
#define CVMmemmoveDouble(s1,s2,len)  memmove(s1,s2,(len)*2)
#endif
#else
#ifndef CVMmemmoveLong
#define CVMmemmoveLong          memmove
#endif
#ifndef CVMmemmoveDouble
#define CVMmemmoveDouble        memmove
#endif
#endif

#ifndef CVMmemmoveForGC
#define CVMmemmoveForGC(s, d, n) \
{                                \
    CVMUint32* dEnd = (d) + (n); \
    do {                         \
        *(d)++ = *(s)++;         \
    } while (d != dEnd);         \
}
#endif

#endif /* _INCLUDED_PORTING_MEMORY_H */
