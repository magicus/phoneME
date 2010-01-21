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
# @(#)GNUmakefile	1.2 06/10/10
#
ASM_ARCH_FLAGS		= -CPU StrongARM1 -32 -ignore 0274
CC_ARCH_FLAGS   	=
CC_ARCH_FLAGS_FDLIB	= /Op
# NOTE: we may not need /Od now that /Op is being used. Need to confirm.
CC_ARCH_FLAGS_FDLIB	+= /Od
CC_ARCH_FLAGS_LOOP 	=
LINK_ARCH_LIBS		= 
LINK_ARCH_FLAGS		= /subsystem:windowsce,5.01
LINKEXE_STACK		= /stack:0x010000
LINKDLL_BASE		= /base:0x00100000

# Description of the VC win32 platform.
# The following are all for the benefit of win32/wince_defs.mk
PLATFORM		= Windows Mobile 5.0 Pocket PC SDK
PLATFORM_OS		= wce500
PLATFORM_INCLUDE_DIRS	= Include/Armv4i
PLATFORM_LIB_DIRS	= Lib/Armv4i 

# assume the JIT is enabled for this device unless told otherwise
CVM_JIT ?= true

# Some Windows Mobile devices don't allow the code cache to reside in
# a read/write/execute segment of the executable. Uncomment this
# definition if you find that the JVM crashes soon after the JIT
# begins to compile code.
# WINCE_DISABLE_STATIC_CODECACHE=true

# package boot classes into rt.jar for non-ROMized build
ifneq ($(CVM_PRELOAD_LIB),true)
CVM_CREATE_RTJAR 	?= true
endif

WIN32_PLATFORM = wince
PCSL_TARGET = javacall_arm

include ../share/top.mk
