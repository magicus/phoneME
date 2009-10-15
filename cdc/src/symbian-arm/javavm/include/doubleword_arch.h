/*
 * @(#)doubleword_arch.h	1.4 06/10/10
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

#ifndef _LINUX_DOUBLEWORD_ARCH_H
#define _LINUX_DOUBLEWORD_ARCH_H

#define CAN_DO_UNALIGNED_DOUBLE_ACCESS
#define CAN_DO_UNALIGNED_INT64_ACCESS
#define HAVE_DOUBLE_BITS_CONVERSION
#define COPY_64_AS_INT64
#undef COPY_64_AS_DOUBLE
#define NAN_CHECK_d2i
#define NAN_CHECK_d2l
#define BOUNDS_CHECK_d2l

#undef USE_NATIVE_FREM
#undef USE_ANSI_FMOD
#undef USE_NATIVE_FCOMPARE
#undef USE_ANSI_FCOMPARE

#ifdef CVM_JIT
#ifndef CVM_ARM_FLOAT_RESULT_IN_FLOAT_REGISTER		
/* Following are called from C functions. */
extern CVMJavaDouble
CVMCCMruntimeDAdd_C(CVMJavaDouble value1, CVMJavaDouble value2);
extern CVMJavaDouble
CVMCCMruntimeDSub_C(CVMJavaDouble value1, CVMJavaDouble value2);
extern CVMJavaDouble
CVMCCMruntimeDMul_C(CVMJavaDouble value1, CVMJavaDouble value2);
extern CVMJavaDouble
CVMCCMruntimeDDiv_C(CVMJavaDouble value1, CVMJavaDouble value2);

#define CVMdoubleAdd    CVMCCMruntimeDAdd_C
#define CVMdoubleSub    CVMCCMruntimeDSub_C
#define CVMdoubleMul    CVMCCMruntimeDMul_C
#define CVMdoubleDiv    CVMCCMruntimeDDiv_C
#endif

#endif

#include "portlibs/ansi_c/doubleword.h"
#if defined(__GNUC__)
#include "portlibs/gcc_32_bit/doubleword.h"
#endif

#endif /* _LINUX_DOUBLEWORD_ARCH_H */
