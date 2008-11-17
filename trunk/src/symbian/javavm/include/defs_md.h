/*
 * @(#)defs_md.h	1.10 06/10/10
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
 * This file declares basic types. These are C types with the width
 * encoded in a one word name, and a mapping of Java basic types to C types.
 *
 */

#ifndef _SYMBIAN_DEFS_MD_H
#define _SYMBIAN_DEFS_MD_H

/*
 * Linux uses types common to gcc
 */

#ifndef _ASM
#include <e32def.h>
#if defined(__GCC32__) || defined(__ARMCC__)
#include "portlibs/gcc_32_bit/defs.h"
#else
typedef TInt8	CVMInt8;
typedef TInt16	CVMInt16;
typedef TInt32	CVMInt32;
typedef TUint32	CVMSize;
typedef TUint32	CVMAddr;

typedef TUint8	CVMUint8;
typedef TUint16	CVMUint16;
typedef TUint32	CVMUint32;
#if defined(__WINS__)
typedef signed __int64 CVMInt64;
typedef unsigned __int64 CVMUint64;
#endif

typedef TReal32	CVMfloat32;
typedef TReal64	CVMfloat64;
#endif
#endif
#include "javavm/include/defs_arch.h"

#ifdef __VC32__
#define CVM_HDR_ANSI_STDIO_H	"javavm/include/ansi/ansi_stdio.h"
#define CVM_HDR_ANSI_TIME_H	"javavm/include/ansi/ansi_time.h"
#endif
#define CVM_HDR_ANSI_STDARG_H	"javavm/include/stdarg_md.h"
#define CVM_HDR_ANSI_STDLIB_H	"javavm/include/ansi/stdlib.h"
#define CVM_HDR_ANSI_STRING_H	"javavm/include/ansi/string.h"

#define CVM_HDR_INT_H		"javavm/include/int_md.h"
#define CVM_HDR_FLOAT_H		"javavm/include/float_md.h"
#define CVM_HDR_DOUBLEWORD_H	"javavm/include/doubleword_md.h"
#define CVM_HDR_JNI_H		"javavm/include/jni_md.h"
#define CVM_HDR_GLOBALS_H	"javavm/include/globals_md.h"
#define CVM_HDR_THREADS_H	"javavm/include/threads_md.h"
#define CVM_HDR_SYNC_H		"javavm/include/sync_md.h"
#define CVM_HDR_LINKER_H	"javavm/include/defs_md.h" /* no-op */
#define CVM_HDR_MEMORY_H        "javavm/include/memory_md.h"
#define CVM_HDR_PATH_H		"javavm/include/path_md.h"
#define CVM_HDR_IO_H		"javavm/include/io_md.h"
#define CVM_HDR_NET_H		"javavm/include/net_md.h"
#define CVM_HDR_TIME_H		"javavm/include/time_md.h"
#define CVM_HDR_ENDIANNESS_H	"javavm/include/endianness_md.h"
#define CVM_HDR_SYSTEM_H	"javavm/include/defs_md.h" /* no-op */
#define CVM_HDR_TIMEZONE_H	"javavm/include/defs_md.h" /* no-op */

#define CVM_HDR_JIT_JIT_H	"javavm/include/jit/jit_arch.h"

#define  CVM_ADV_MUTEX_SET_OWNER

#define CVM_HAVE_PROCESS_MODEL

#endif /* _SYMBIAN_DEFS_MD_H */
