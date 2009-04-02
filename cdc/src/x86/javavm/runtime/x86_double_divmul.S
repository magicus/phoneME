/*
 * @(#)x86_double_divmul.S	1.6	06/10/10
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

#include "javavm/include/asmmacros_cpu.h"

 # These functions scale their operands such that rounding occurs at
 # the "right" place. This avoids the "double rounding" problem
 # that can otherwise occur.
 # Then, the result is scaled back and rounded using a store and
 # load of the return value.
 #
	.text
	ALIGN16
	VARIABLE(small_factor)
	.long 0x0,0x80000000,0x03ff,5,6
	ALIGN16
	VARIABLE(large_factor)
	.long 0x0,0x80000000,0x7bff

	.text
	ENTRY(doubleMul)

	# set up stack frame
	pushl %ebp
	movl %esp,%ebp
	subl $8,%esp

	# fetch and scale one operand
	fldl 8(%ebp)
	fldt small_factor
	fmulp %st,%st(1)
	# fetch second operand and do the multiplication
	fldl 16(%ebp)
	fmulp %st,%st(1)
	# scale result
	fldt large_factor
	fmulp %st,%st(1)
	# round result
	fstpl -8(%ebp)
	fldl -8(%ebp)

	movl %ebp,%esp
	popl %ebp
	ret

	.align 4
	SET_SIZE(doubleMul)

	ENTRY(doubleDiv)

	# set up stack frame
	pushl %ebp
	movl %esp,%ebp
	subl $8,%esp

	# fetch operands and scale one operand
	fldl 16(%ebp)
	fldl 8(%ebp)
	fldt small_factor
	fmulp %st,%st(1)
	# do the division
	fdivp %st,%st(1)
	# scale result
	fldt large_factor
	fmulp %st,%st(1)
	# round result
	fstpl -8(%ebp)
	fldl -8(%ebp)

	movl %ebp,%esp
	popl %ebp
	ret

	.align 4
	SET_SIZE(doubleDiv)
