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
# @(#)GNUmakefile	1.15 06/10/10
#
CVM_DEFINES 		+= -D_i386_ -D_386
ASM_ARCH_FLAGS		= -c
CC_ARCH_FLAGS   	=
CC_ARCH_FLAGS_FDLIB	= /Op
CC_ARCH_FLAGS_LOOP 	=
LINK_ARCH_LIBS		= corelibc.lib
LINK_ARCH_FLAGS		= /subsystem:windowsce,3.00 /MACHINE:IX86
LINKEXE_STACK		= /stack:0x010000
LINKDLL_BASE		= /base:0x00100000


CVM_JIT_USE_FP_HARDWARE = false

# Disable CVM_JIT_REGISTER_LOCALS for x86 for now.
CVM_JIT_REGISTER_LOCALS ?= false

# Description of the VC win32 platform.
# The following are all for the benefit of win32/wince_defs.mk
PLATFORM		= Pocket PC 2002
PLATFORM_OS		= wce300
PLATFORM_INCLUDE_DIRS	= include mfc/include atl/include
PLATFORM_LIB_DIRS	= lib/x86 mfc/lib/x86 atl/lib/x86

WIN32_PLATFORM = wince

include ../share/top.mk
