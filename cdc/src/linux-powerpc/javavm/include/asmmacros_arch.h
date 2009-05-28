/*
 * @(#)asmmacros_arch.h	1.8 06/10/10
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
 * This file defines a few asm macros whose implementations differ on
 * different platforms.
 */

#ifndef _INCLUDED_ASMMACROS_ARCH_H
#define _INCLUDED_ASMMACROS_ARCH_H

#define SYM_NAME(x) x
#define HA16(x) x@ha
#define LO16(x) x@l
#define _SE_ ;  /* statement end char */

#define ENTRY(x)		\
	.align 2;		\
	.globl x;		\
	.type x, @function;	\
	x:

#define SET_SIZE(x)		\
	.size	x, (.-x)

#undef r0
#undef sp
#undef r2
#undef r3
#undef r4
#undef r5
#undef r6
#undef r7
#undef r8
#undef r9
#undef r10
#undef r11
#undef r12
#undef r13
#undef r14
#undef r15
#undef r16
#undef r17
#undef r18
#undef r19
#undef r20
#undef r21
#undef r22
#undef r23
#undef r24
#undef r25
#undef r26
#undef r27
#undef r28
#undef r29
#undef r30
#undef r31

#undef f0
#undef f1
#undef f2
#undef f3
#undef f4
#undef f5
#undef f6
#undef f7
#undef f8
#undef f9
#undef f10
#undef f11
#undef f12
#undef f13
#undef f14
#undef f15
#undef f16
#undef f17
#undef f18
#undef f19
#undef f20
#undef f21
#undef f22
#undef f23
#undef f24
#undef f25
#undef f26
#undef f27
#undef f28
#undef f29
#undef f30
#undef f31

#define r0	0
#define sp	1
#define toc	2
#define r3	3
#define r4	4
#define r5	5
#define r6	6
#define r7	7
#define r8	8
#define r9	9
#define r10	10
#define r11	11
#define r12	12
#define r13	13
#define r14	14
#define r15	15
#define r16	16
#define r17	17
#define r18	18
#define r19	19
#define r20	20
#define r21	21
#define r22	22
#define r23	23
#define r24	24
#define r25	25
#define r26	26
#define r27	27
#define r28	28
#define r29	29
#define r30	30
#define r31	31

#define f0	0
#define f1	1
#define f2	2
#define f3	3
#define f4	4
#define f5	5
#define f6	6
#define f7	7
#define f8	8
#define f9	9
#define f10	10
#define f11	11
#define f12	12
#define f13	13
#define f14	14
#define f15	15
#define f16	16
#define f17	17
#define f18	18
#define f19	19
#define f20	20
#define f21	21
#define f22	22
#define f23	23
#define f24	24
#define f25	25
#define f26	26
#define f27	27
#define f28	28
#define f29	29
#define f30	30
#define f31	31

#endif /* _INCLUDED_ASMMACROS_ARCH_H */
