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
 * This file declares basic types. These are C types with the width
 * encoded in a one word name, and a mapping of Java basic types to C types.
 *
 */

#ifndef _WIN32_DEFS_MD_H
#define _WIN32_DEFS_MD_H

#include "javavm/include/defs_arch.h"

#ifndef _ASM
/* <windows.h> is troublesome, so undo some of what it does */
#pragma include_alias(<windows.h>, <javavm/include/win32/windows.h>)
/* work around type conflicts */
#pragma include_alias(<winnt.h>, <javavm/include/win32/winnt.h>)
#endif

#define CVM_HAS_PLATFORM_SPECIFIC_SUBOPTIONS

#ifdef WINCE
#define CVM_HDR_ANSI_ERRNO_H	"javavm/include/ansi/errno.h"
#ifndef _ASM
#pragma include_alias(<errno.h>, <javavm/include/ansi/errno.h>)
#endif
#define CVM_HDR_ANSI_STRING_H	"javavm/include/ansi/string.h"
#define CVM_HDR_ANSI_STDLIB_H	"javavm/include/ansi/ansi_stdlib.h"
#endif

#define CVM_HDR_ANSI_CTYPE_H	<ctype.h>

#define CVM_HDR_ANSI_SETJMP_H   "javavm/include/ansi/ansi_setjmp.h"
#define CVM_HDR_ANSI_STDARG_H	"javavm/include/ansi/stdarg.h"
#define CVM_HDR_ANSI_STDDEF_H	"javavm/include/ansi/stddef.h"
#ifdef WINCE
#define CVM_HDR_ANSI_TIME_H	"javavm/include/ansi/time.h"
#define CVM_HDR_ANSI_ASSERT_H	"javavm/include/ansi/assert.h"
#else
#define CVM_HDR_ANSI_TIME_H	<time.h>
#define CVM_HDR_ANSI_ASSERT_H	<assert.h>
#endif
#define CVM_HDR_ANSI_STDINT_H	"javavm/include/ansi/stdint.h"

#define CVM_HDR_INT_H		"javavm/include/int_md.h"
#define CVM_HDR_FLOAT_H		"javavm/include/float_md.h"
#define CVM_HDR_DOUBLEWORD_H	"javavm/include/doubleword_md.h"
#define CVM_HDR_JNI_H		"javavm/include/jni_md.h"
#define CVM_HDR_GLOBALS_H	"javavm/include/globals_md.h"
#define CVM_HDR_THREADS_H	"javavm/include/threads_md.h"
#define CVM_HDR_SYNC_H		"javavm/include/sync_md.h"
#define CVM_HDR_LINKER_H	"javavm/include/defs_md.h" /* no-op */
#define CVM_HDR_PATH_H		"javavm/include/path_md.h"
#define CVM_HDR_IO_H		"javavm/include/io_md.h"
#define CVM_HDR_NET_H		"javavm/include/net_md.h"
#define CVM_HDR_TIME_H		"javavm/include/time_md.h"
#define CVM_HDR_ENDIANNESS_H	"javavm/include/endianness_md.h"
#define CVM_HDR_SYSTEM_H	"javavm/include/defs_md.h" /* no-op */
#define CVM_HDR_TIMEZONE_H	"javavm/include/defs_md.h" /* no-op */
#define CVM_HDR_MEMORY_H	"javavm/include/memory_md.h"

#define CVM_HDR_JIT_JIT_H	"javavm/include/jit/jit_arch.h"

#undef  CVM_ADV_MUTEX_SET_OWNER
#define CVM_ADV_THREAD_BOOST

#define CVM_HAVE_PROCESS_MODEL

#if _MSC_VER >= 1300
#undef INTERFACE
#endif

/*
 * Win32 uses types common to vc
 */
#ifndef _ASM

typedef signed __int64	CVMInt64;
typedef signed __int32	CVMInt32;
typedef signed __int16	CVMInt16;
typedef signed __int8	CVMInt8;

typedef unsigned __int64 CVMUint64;
typedef unsigned __int32 CVMUint32;
typedef unsigned __int16 CVMUint16;
typedef unsigned __int8	 CVMUint8;

/* 
 * make CVM ready to run on 64 bit platforms
 * To keep the source base common for 32 and 64 bit
 * platforms it is required to introduce a new type
 * CVMAddr, an integer big enough to hold a native pointer.
 * That meens 4 byte (int32) on 32 platforms and 8 byte
 * on 64 bit platforms. This type should be used at places
 * where pointer arithmetic and casting is required. Using
 * this type instead of the original CVMUint32 should
 * not change anything for the platforms linux32/win32 but
 * make the code better portable for 64 bit platforms.
 */
#if defined(CVM_64)
typedef unsigned __int64	CVMAddr;
#else
typedef unsigned __int32	CVMAddr;
#endif

typedef float		CVMfloat32;
typedef double		CVMfloat64;

typedef CVMUint32 CVMSize;

#endif
#define CONST64(x)  (x ## i64)
#define UCONST64(x) ((uint64_t)CONST64(x))

#endif /* _WIN32_DEFS_MD_H */
