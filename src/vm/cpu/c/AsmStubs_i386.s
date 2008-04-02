#
#   
#
# Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
# DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
# 
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License version
# 2 only, as published by the Free Software Foundation.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License version 2 for more details (a copy is
# included at /legal/license.txt).
# 
# You should have received a copy of the GNU General Public License
# version 2 along with this work; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
# 02110-1301 USA
# 
# Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
# Clara, CA 95054 or visit www.sun.com if you need additional
# information or have any questions.
#

.arch i486
.text
.align 16

#ifdef cygwin
#define EXTERN(sym) _##sym
#else
#define EXTERN(sym) sym
#endif

# Modified GCC-generated code for ANSI dmul
        .global EXTERN(JFP_lib_dmul_x86)
EXTERN(JFP_lib_dmul_x86):
        # Loading shifter.
        movl   $0x00000000, 0xfffffff8(%esp)
        movl   $0x7bf00000, 0xfffffffc(%esp)
        # Shift the values to get 64-bit precision.
        fld1        
        fdivl  0xfffffff8(%esp)
        fstpl  0xfffffff0(%esp)
        fldl   0xfffffff0(%esp)
        fmull  0xfffffff0(%esp)
        fmull  0xfffffff0(%esp)
        fmull  0xfffffff0(%esp)
        fmull  0xfffffff0(%esp)
        fmull  0xfffffff0(%esp)
        fmull  0xfffffff0(%esp)
        fmull  0xfffffff0(%esp)
        fmull  0xfffffff0(%esp)
        fmull  0xfffffff0(%esp)
        fmull  0xfffffff0(%esp)
        fmull  0xfffffff0(%esp)
        fmull  0xfffffff0(%esp)
        fmull  0xfffffff0(%esp)
        fmull  0xfffffff0(%esp)
        fmull  0xfffffff0(%esp)
        # Perform operation.
        fmull  0x00000004(%esp)
        fmull  0x0000000c(%esp)
        # Prepare shifting back to 80-bit value.
        fldl   0xfffffff8(%esp)
        fmull  0xfffffff8(%esp)
        fmull  0xfffffff8(%esp)
        fmull  0xfffffff8(%esp)
        fmull  0xfffffff8(%esp)
        fmull  0xfffffff8(%esp)
        fmull  0xfffffff8(%esp)
        fmull  0xfffffff8(%esp)
        fmull  0xfffffff8(%esp)
        fmull  0xfffffff8(%esp)
        fmull  0xfffffff8(%esp)
        fmull  0xfffffff8(%esp)
        fmull  0xfffffff8(%esp)
        fmull  0xfffffff8(%esp)
        fmull  0xfffffff8(%esp)
        fmull  0xfffffff8(%esp)
        # Get 80-bit result.
        fmulp  %st,%st(1)
        # Round 80-bit to 64-bit value.
        fstpl  0xfffffff0(%esp)
        fldl   0xfffffff0(%esp)
        ret

# Modified GCC-generated code for ANSI ddiv
        .global EXTERN(JFP_lib_ddiv_x86)
EXTERN(JFP_lib_ddiv_x86):
        # Loading shifter.
        movl   $0x00000000, 0xfffffff8(%esp)
        movl   $0x7bf00000, 0xfffffffc(%esp)
        # Shift the values to achive 64-bit presicion of the operation.
        fld1        
        fdivl  0xfffffff8(%esp)
        fstpl  0xfffffff0(%esp)
        fldl   0xfffffff0(%esp)
        fmull  0xfffffff0(%esp)
        fmull  0xfffffff0(%esp)
        fmull  0xfffffff0(%esp)
        fmull  0xfffffff0(%esp)
        fmull  0xfffffff0(%esp)
        fmull  0xfffffff0(%esp)
        fmull  0xfffffff0(%esp)
        fmull  0xfffffff0(%esp)
        fmull  0xfffffff0(%esp)
        fmull  0xfffffff0(%esp)
        fmull  0xfffffff0(%esp)
        fmull  0xfffffff0(%esp)
        fmull  0xfffffff0(%esp)
        fmull  0xfffffff0(%esp)
        fmull  0xfffffff0(%esp)
        # Perform 64-bit precision operation.
        fmull  0x00000004(%esp)
        fdivl  0x0000000c(%esp)
        # Prepare shifting back.
        fldl   0xfffffff8(%esp)
        fmull  0xfffffff8(%esp)
        fmull  0xfffffff8(%esp)
        fmull  0xfffffff8(%esp)
        fmull  0xfffffff8(%esp)
        fmull  0xfffffff8(%esp)
        fmull  0xfffffff8(%esp)
        fmull  0xfffffff8(%esp)
        fmull  0xfffffff8(%esp)
        fmull  0xfffffff8(%esp)
        fmull  0xfffffff8(%esp)
        fmull  0xfffffff8(%esp)
        fmull  0xfffffff8(%esp)
        fmull  0xfffffff8(%esp)
        fmull  0xfffffff8(%esp)
        fmull  0xfffffff8(%esp)
        # Get 80-bit result.
        fmulp  %st,%st(1)
        # Round 80-bit to 64-bit value.
        fstpl  0xfffffff0(%esp)
        fldl   0xfffffff0(%esp)
        ret

# mark the end of ASM functions
        .global end_of_the_world
end_of_the_world:
