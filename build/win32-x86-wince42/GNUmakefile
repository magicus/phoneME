#
# Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

CVM_DEFINES 		+= -D_i386_ -D_386 -DWINCE41 -DUNDER_CE=410 -D_WIN32_WCE=410
ASM_ARCH_FLAGS		= -c
CC_ARCH_FLAGS   	=
CC_ARCH_FLAGS_FDLIB	= /Op
CC_ARCH_FLAGS_LOOP 	=
LINK_ARCH_LIBS		= corelibc.lib
LINK_ARCH_FLAGS		= \
	/subsystem:windowsce,4.20 /MACHINE:IX86 \
	/base:0x00100000
LINKEXE_ARCH_LIBS	= /defaultlib:corelibc.lib
LINKEXE_ARCH_FLAGS	= \
	/subsystem:windowsce,4.20 /MACHINE:IX86 \
	/base:0x00010000 /stack:0x200000,0x40000

CVM_JIT_USE_FP_HARDWARE = false

# Disable CVM_JIT_REGISTER_LOCALS for x86 for now.
CVM_JIT_REGISTER_LOCALS ?= false

# Description of the VC win32 platform.
# Windows path format
# Can be overridden on the make command line or exported from shell
WIN_SDK_DIR     ?= "C:\Program Files\Windows CE Tools"
WIN_VC_PATH     ?= "C:\Program Files\Microsoft eMbedded C++ 4.0"
WIN_PLATFORM    ?= 'POCKET PC 2003'
WIN_PLATFORM_OS ?= 'WCE420'

# Conversion Platform SDK Dir paths to the 8.3 no-spaces format
WIN_PSD0        = $(WIN_SDK_DIR)\\$(WIN_PLATFORM_OS)\\$(WIN_PLATFORM)
WIN_PSD         = $(shell cygpath -u $(shell cygpath -ms $(WIN_PSD0)))

# Platform description
# Can be overridden on the make command line or exported from shell
PLATFORM        ?= $(shell basename $(WIN_PSD))
PLATFORM_OS     ?= $(shell basename $(shell dirname $(WIN_PSD)))

# Must use DOS path for SDK_DIR with no quotes.
# Can be overridden on the make command line or exported from shell
SDK_DIR         ?= $(shell cygpath -ms $(WIN_SDK_DIR))

# Must use /cygdrive path for VC_PATH with no quotes
# Can be overridden on the make command line or exported from shell
VC_PATH         ?= $(shell cygpath -u $(shell cygpath -ms $(WIN_VC_PATH)))

PLATFORM_INCLUDE_DIRS   = include/Emulator mfc/include atl/include
PLATFORM_LIB_DIRS       = lib/Emulator mfc/lib/x86 atl/lib/x86

WIN32_PLATFORM = wince

include ../share/top.mk
