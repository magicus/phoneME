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
# @(#)GNUmakefile	1.4 06/10/10
#
CVM_DEFINES 		+= -D_i386_ -D_386
# Fix some annoying C4996 warnings introduced with VC8
CVM_DEFINES 		+= \
	-D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE
ASM_ARCH_FLAGS		= -c
CC_ARCH_FLAGS   	=
CC_ARCH_FLAGS_FDLIB	= /Op
# Looks like we still need /Od to disable optimization for fdlib. (Bug 6255783)
CC_ARCH_FLAGS_FDLIB	+= /Od
CC_ARCH_FLAGS_LOOP 	=
LINK_ARCH_LIBS		=
LINK_ARCH_FLAGS		=

CVM_JIT_USE_FP_HARDWARE = false

# Disable CVM_JIT_REGISTER_LOCALS for x86 for now.
CVM_JIT_REGISTER_LOCALS ?= false

include ../share/top.mk
