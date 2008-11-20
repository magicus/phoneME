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
# @(#)GNUmakefile	1.16 06/10/10
#
# GNUmakefile for solaris-sparc-sun target
#

#
# platform specific architecture flags
#
ASM_ARCH_FLAGS		=
CC_ARCH_FLAGS		= -mv8plus
CC_ARCH_FLAGS_FDLIB	=
# Don't inline the interpreter loop helper functions. This helps
# reduce register pressure on sparc and improve generated code. This
# seems strange given the abundance of registers on sparc, but spec
# ran faster when doing this.
CC_ARCH_FLAGS_LOOP	= -fno-inline
LINK_ARCH_FLAGS 	= 
LINK_ARCH_LIBS		=

# assume the JIT is enabled for this device unless told otherwise
CVM_JIT ?= true
# assume floating point hardware on this device, unless told otherwise
CVM_JIT_USE_FP_HARDWARE ?= true

# assume support for "cas" on this device, unless told otherwise.
CVM_HAS_CAS ?= true

include ../share/top.mk
