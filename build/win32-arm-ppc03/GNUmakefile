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
ASM_ARCH_FLAGS		= -CPU StrongARM1 -32
CC_ARCH_FLAGS   	= /MC
CC_ARCH_FLAGS_FDLIB	= /Op
# NOTE: we may not need /Od now that /Op is being used. Need to confirm.
CC_ARCH_FLAGS_FDLIB	+= /Od
CC_ARCH_FLAGS_LOOP 	=
LINK_ARCH_LIBS		= 
LINK_ARCH_FLAGS		= /subsystem:windowsce,3.00 /MACHINE:ARM
LINKEXE_STACK		= /stack:0x010000
LINKDLL_BASE		= /base:0x00100000

# Description of the VC win32 platform.
# The following are all for the benefit of win32/wince_defs.mk
PLATFORM		= POCKET PC 2003
PLATFORM_OS		= wce420
PLATFORM_INCLUDE_DIRS	= Include/Armv4 Mfc/include Atl/include
PLATFORM_LIB_DIRS	= Lib/Armv4 Mfc/lib/armv4 Atl/lib/armv4

# assume the JIT is enabled for this device unless told otherwise
CVM_JIT ?= true

WIN32_PLATFORM = wince

include ../share/top.mk
