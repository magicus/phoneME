/*
 *   
 *
 * Portions Copyright  2000-2008 Sun Microsystems, Inc. All Rights
 * Reserved.  Use is subject to license terms.
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
 *!c<
 * Copyright 2006 Intel Corporation. All rights reserved.
 *!c>
 */

/** \file OS_linux.hpp: OS-specific declarations.
 */

#ifndef  __USE_GNU
#define __USE_GNU
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>
#include <signal.h>
#include <sched.h>
#include <errno.h>

#ifdef SOLARIS
  // otherwise mmap/munmap has bad signature
#ifndef  _XPG4_2
#define _XPG4_2 1
#endif
#endif

#include <sys/mman.h>

#if ( ENABLE_NPCE || ( ENABLE_INTERNAL_CODE_OPTIMIZER ) ) && ARM_EXECUTABLE
enum {
  not_found = -1,
};
#define ucontext asm_ucontext
extern "C" {
  extern address gp_compiler_throw_NullPointerException_ptr;
  extern address gp_compiler_throw_ArrayIndexOutOfBoundsException_ptr;
  extern address gp_compiler_throw_NullPointerException_10_ptr;
  extern address gp_compiler_throw_ArrayIndexOutOfBoundsException_10_ptr;
}
#include <asm/ucontext.h>
#else
#include <ucontext.h>
#endif

#if ARM && defined(__NetBSD__)
#include <machine/sysarch.h>
#endif

#if ENABLE_TIMER_THREAD
#include <pthread.h>
#include <semaphore.h>
#endif

#define NEED_XSCALE_PMU_CYCLE_COUNTER \
        (ENABLE_XSCALE_PMU_CYCLE_COUNTER && ARM_EXECUTABLE)

#if NEED_XSCALE_PMU_CYCLE_COUNTER
#include <sys/ioctl.h>

#define PMU_GET_INS_COUNTER 21
#define PMU_GET_CPU_CLK 22
#endif
 
#undef __USE_GNU

#define SUSPENDSIG SIGUSR1
