/*
 * @(#)defs_md.h	1.22 06/10/10
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

#ifndef _LINUX_DEFS_MD_H
#define _LINUX_DEFS_MD_H

/*
 * Linux uses types common to gcc
 */

#ifndef _ASM
#include "portlibs/gcc_32_bit/defs.h"
#endif
#include "javavm/include/defs_arch.h"

#define CVM_HDR_ANSI_LIMITS_H	<limits.h>
#define CVM_HDR_ANSI_STDIO_H	<stdio.h>
#define CVM_HDR_ANSI_STDDEF_H	<stddef.h>
#define CVM_HDR_ANSI_STRING_H	<string.h>
#define CVM_HDR_ANSI_STDLIB_H	"javavm/include/stdlib_md.h"
#define CVM_HDR_ANSI_TIME_H	<time.h>
#define CVM_HDR_ANSI_SETJMP_H	<setjmp.h>
#define CVM_HDR_ANSI_CTYPE_H	<ctype.h>
#define CVM_HDR_ANSI_ASSERT_H	<assert.h>
#define CVM_HDR_ANSI_STDINT_H   <stdint.h>

#define CVM_HDR_ANSI_ERRNO_H	"javavm/include/errno_md.h"
#define CVM_HDR_ANSI_STDARG_H	"javavm/include/stdarg_md.h"

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

#define CVM_HAVE_PROCESS_MODEL

/*
 DMRLNX: pthread.h and semaphore.h probably should be included elsewhere.
*/
#ifndef _ASM
#include <pthread.h>
#include <semaphore.h>
#endif

#define thread_t                pthread_t

#define mutex_t                 pthread_mutex_t
#define mutex_init              pthread_mutex_init
#define mutex_lock              pthread_mutex_lock
#define mutex_trylock           pthread_mutex_trylock
#define mutex_unlock            pthread_mutex_unlock
#define mutex_destroy           pthread_mutex_destroy

#define cond_t                  pthread_cond_t
#define cond_destroy            pthread_cond_destroy
#define cond_wait               pthread_cond_wait
#define cond_timedwait          pthread_cond_timedwait
#define cond_signal             pthread_cond_signal
#define cond_broadcast          pthread_cond_broadcast

#define thread_key_t            pthread_key_t
#define thr_setspecific         pthread_setspecific
#define thr_keycreate           pthread_key_create

#define thr_sigsetmask          pthread_sigmask
#define thr_self                pthread_self
#define thr_yield               sched_yield
#define thr_kill                pthread_kill
#define thr_exit                pthread_exit

#endif /* _LINUX_DEFS_MD_H */
