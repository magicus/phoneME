/*
 * @(#)defs.h	1.24 06/10/10
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

#ifndef _INCLUDED_PORTING_DEFS_H
#define _INCLUDED_PORTING_DEFS_H

/*
 * This part of the porting layer defines CVM primitive types.
 *
 *
 * CVMInt8, CVMInt16, CVMInt32, CVMInt64
 * CVMUint8, CVMUint16, CVMUint32
 *
 * Signed and unsigned integers of specified bit width.
 * CVMInt32 represents a 32-bit signed integer.
 *
 * The 64-bit types can have an arbitrary representation,
 * such as a struct, but the other types must have a
 * native representation that is supported by the compiler.
 *
 * CVMfloat32, CVMfloat64
 *
 * Single and double-precision IEEE floating point types.
 * These can have an arbitrary representation.
 *
 *
 * CVMSize
 *
 * Equivalent to ansi C size_t.
 *
 * ANSI header file locations
 *
 * CVM_HDR_ANSI_LIMITS_H   (ANSI <limits.h>)
 * CVM_HDR_ANSI_STDIO_H    (ANSI <stdio.h>)
 * CVM_HDR_ANSI_STDARG_H   (ANSI <stdarg.h>)
 * CVM_HDR_ANSI_STDDEF_H   (ANSI <stddef.h>)
 * CVM_HDR_ANSI_STDLIB_H   (ANSI <stdlib.h>)
 * CVM_HDR_ANSI_STRING_H   (ANSI <string.h>)
 * CVM_HDR_ANSI_TIME_H     (ANSI <time.h>)
 * CVM_HDR_ANSI_CTYPE_H    (ANSI <ctype.h>)
 * CVM_HDR_ANSI_ASSERT_H   (ANSI <assert.h>)
 * CVM_HDR_ANSI_ERRNO_H    (ANSI <errno.h>)
 * CVM_HDR_ANSI_SETJMP_H   (ANSI <setjmp.h>)
 *
 * VM header file locations
 *
 * CVM_HDR_INT_H           (included by porting/int.h)
 * CVM_HDR_FLOAT_H         (included by porting/float.h)
 * CVM_HDR_DOUBLEWORD_H    (included by porting/doubleword.h)
 * CVM_HDR_JNI_H           (included by porting/jni.h)
 * CVM_HDR_GLOBALS_H       (included by porting/globals.h)
 * CVM_HDR_THREADS_H       (included by porting/threads.h)
 * CVM_HDR_SYNC_H          (included by porting/sync.h)
 * CVM_HDR_LINKER_H        (included by porting/linker.h)
 * CVM_HDR_MEMORY_H        (included by porting/memory.h)
 * CVM_HDR_PATH_H          (included by porting/path.h)
 * CVM_HDR_IO_H            (included by porting/io.h)
 * CVM_HDR_NET_H           (included by porting/net.h)
 * CVM_HDR_TIME_H          (included by porting/time.h)
 * CVM_HDR_ENDIANNESS_H    (included by porting/enddianness.h)
 * CVM_HDR_SYSTEM_H        (included by porting/system.h)
 * CVM_HDR_TIMEZONE_H      (included by porting/timezone.h)
 * CVM_HDR_JIT_JIT_H       (included by porting/jit.h)
 *
 * Advanced platform features
 *
 * These features are not required by default, but enabling some VM
 * features may result in their needing to be defined.
 *
 * CVM_ADV_SPINLOCK
 *
 * Defined if the platform supports CVMspinlockYield and CVMvolatileStore.
 *
 * CVM_ADV_SCHEDLOCK
 *
 * Defined if the a scheduler lock (CVMschedLock) is supported.
 *
 * CVM_ADV_MUTEX_SET_OWNER
 *
 * Defined if setting the owner of a mutex (CVMmutexSetOwner)
 * is supported.
 *
 * CVM_ADV_ATOMIC_CMPANDSWAP
 *
 * Defined if atomic compare-and-swap (CVMatomicCompareAndSwap)
 * is supported.
 *
 * CVM_ADV_ATOMIC_SWAP
 *
 * Defined if atomic swap (CVMatomicSwap) is supported.
 *
 *
 * Process model
 *
 *   CVM_HAVE_PROCESS_MODEL
 *
 *   Defining this macro specifies that the platform (porting layer)
 *   implements a "process" model.  The VM will not attempt to wait
 *   for daemon threads before shutting down, nor will it attempt to
 *   free up resources.
 *
 *   If this macro is undefined, the VM will wait for all VM threads
 *   to exit before shutting down, and will try to release any
 *   resources that were allocated.
 */

#include "javavm/include/defs_md.h"

#endif /* _INCLUDED_PORTING_DEFS_H */
