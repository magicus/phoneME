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
# @(#)winsim.mk	1.12 06/10/10
#

# FIXME: We should move things to the corresponding defs.mk and remove winsim.mk.

SDK_DIR = C:/Program Files/Microsoft Visual Studio
VC_DIR  = $(call WIN2POSIX,$(SDK_DIR))
PLATFORM                = VC98
PLATFORM_SDK_DIR        = $(SDK_DIR)/VC98
PLATFORM_TOOLS_DIR      = $(VC_DIR)/VC98/Bin
COMMON_TOOLS_DIR        = $(VC_DIR)/Common/MSDev98/Bin
PLATFORM_LIB_DIRS       = Lib

include ../win32/host_defs.mk

ifeq ($(CVM_DLL),true)
CPPFLAGS += -D__DLL__
MT_FLAGS = $(MT_DLL_FLAGS)
else
CPPFLAGS += -D__EXE__
MT_FLAGS = $(MT_EXE_FLAGS)

CVM_TARGETOBJS_SPACE += ansi_java_md.o java_md.o
endif

CCFLAGS += $(MT_FLAGS)
# Shared strings
CCFLAGS += /GF
# No stack checks
CCFLAGS += /Gs
# Pack structures to 4-byte boundary
CCFLAGS += /Zp4

ASM_ARCH_FLAGS = -c
