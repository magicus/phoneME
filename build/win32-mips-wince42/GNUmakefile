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
# @(#)GNUmakefile	1.14 06/10/10
#
#
# platform specific architecture flags
#
CVM_DEFINES +=	-DMIPSIV -DMIPSIV_FP -DMIPSFP -DMIPS64 -D_MIPS64 \
		-DWINCE41 -DUNDER_CE=410 -D_WIN32_WCE=410
ASM_ARCH_FLAGS		= /QMmips4 /QMn32 /QMFPE-
CC_ARCH_FLAGS   	= /QMmips4 /QMn32 /QMFPE- /MC
CC_ARCH_FLAGS_FDLIB	= /Op
CC_ARCH_FLAGS_LOOP 	=
LINK_ARCH_LIBS		= 
LINK_ARCH_FLAGS		= \
	/subsystem:windowsce,4.20 /MACHINE:MIPSFPU \
	/base:0x00100000 /entry:_DllMainCRTStartup
LINKEXE_ARCH_LIBS	=
LINKEXE_ARCH_FLAGS	= \
	/subsystem:windowsce,4.20 /MACHINE:MIPSFPU /stack:0x010000

# Description of the VC win32 platform.
# The following are all for the benefit of win32/wince_defs.mk
PLATFORM		= wince42
PLATFORM_OS		= wce420
PLATFORM_INCLUDE_DIRS	= Include/Mipsiv_fp mfc/include atl/include
PLATFORM_LIB_DIRS	= lib/mipsiv_fp mfc/lib/mips atl/lib/mips

# assume the JIT is enabled for this device unless told otherwise
CVM_JIT ?= true
# assume floating point hardware on this device unless told otherwise
CVM_JIT_USE_FP_HARDWARE ?= true

WIN32_PLATFORM = wince

include ../share/top.mk
