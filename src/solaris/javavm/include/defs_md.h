/*
 * @(#)defs_md.h	1.29 06/10/10
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

#ifndef _SOLARIS_DEFS_MD_H
#define _SOLARIS_DEFS_MD_H

/*
 * Solaris uses types common to gcc
 */

#ifndef _ASM
#include "portlibs/gcc_32_bit/defs.h"
#endif
#include "javavm/include/defs_arch.h"

#define CVM_HDR_ANSI_LIMITS_H	<limits.h>
#define CVM_HDR_ANSI_STDIO_H	<stdio.h>
#define CVM_HDR_ANSI_STDDEF_H	<stddef.h>
#define CVM_HDR_ANSI_STRING_H	<string.h>
#define CVM_HDR_ANSI_STDLIB_H	<stdlib.h>
#define CVM_HDR_ANSI_TIME_H	<time.h>
#define CVM_HDR_ANSI_SETJMP_H	<setjmp.h>
#define CVM_HDR_ANSI_CTYPE_H	<ctype.h>
#define CVM_HDR_ANSI_ASSERT_H	<assert.h>

#define CVM_HDR_ANSI_ERRNO_H	"javavm/include/errno_md.h"
#define CVM_HDR_ANSI_STDARG_H	"javavm/include/stdarg_md.h"
#define CVM_HDR_ANSI_STDINT_H	"javavm/include/stdint_md.h"

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

#define CVM_ADV_MUTEX_SET_OWNER
#undef CVM_ADV_THREAD_BOOST
#undef SOLARIS_BOOST_MUTEX

#define CVM_HAVE_PROCESS_MODEL

#endif /* _SOLARIS_DEFS_MD_H */
