/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */

#ifndef _INCLUDED_PORTING_MEMORY_H
#define _INCLUDED_PORTING_MEMORY_H

#include "javavm/include/porting/defs.h"
#include "javavm/include/porting/ansi/stddef.h"
#include "javavm/include/porting/vm-defs.h"
/*
 * The platform must define the following for aligned memory allocation.
 */
void* CVMmemalignAlloc(size_t alignment, size_t size);
void  CVMmemalignFree(void* memalignAllocedSpace);

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

#include CVM_HDR_MEMORY_H

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

#endif /* _INCLUDED_PORTING_MEMORY_H */
