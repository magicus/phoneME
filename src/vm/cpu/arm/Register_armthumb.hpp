/*
 *
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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
 */

    // position and order are relevant!
    r0, r1, r2 , r3 , r4 , r5 , r6 , r7 ,
    r8, r9, r10, r11, r12, r13, r14, r15,

#if ENABLE_ARM_VFP
    // Single-precision floating point registers.
    s0, s1, s2, s3, s4, s5, s6, s7,
    s8, s9, s10, s11, s12, s13, s14, s15,
    s16, s17, s18, s19, s20, s21, s22, s23,
    s24, s25, s26, s27, s28, s29, s30, s31,
#endif
    number_of_registers,

#if ENABLE_ARM_VFP
    // Double-precision floating point registers.
    d0  = s0,
    d1  = s2,
    d2  = s4,
    d3  = s6,
    d4  = s8,
    d5  = s10,
    d6  = s12,
    d7  = s14,
    d8  = s16,
    d9  = s18,
    d10 = s20,
    d11 = s22,
    d12 = s24,
    d13 = s26,
    d14 = s28,
    d15 = s30,
#endif

    // for platform-independant code
    return_register = r0,
    stack_lock_register = r1,

    // for instruction assembly only
    sbz =  r0,
    sbo = r15,

    no_reg			=  -1,
    first_register		=  r0,
#if ENABLE_ARM_VFP
    last_register		= s31,
    number_of_float_registers	=  32,
#else
    last_register		= r15,
    number_of_float_registers	=   0,
#endif
    number_of_gp_registers	=  16,

    first_allocatable_register	= r0,
    method_return_type		= r2,

    _force_32bit_Register	= 0x10000000,	// for ADS compiler
