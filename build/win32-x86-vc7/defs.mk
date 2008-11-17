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
# @(#)defs.mk	1.3 06/10/10
# 


# Description of the VC 2003 win32 platform.
# The following are all for the benefit of win32/defs.mk
SDK_DIR			?= C:/Program Files/Microsoft Visual Studio .NET 2003/Vc7
VC_PATH			= $(call WIN2POSIX,$(SDK_DIR))
PLATFORM_SDK_DIR	?= C:/Program Files/Microsoft Platform SDK
PLATFORM_SDK_PATH 	= $(call WIN2POSIX,$(PLATFORM_SDK_DIR))
PLATFORM_TOOLS_PATH	= $(VC_PATH)/Bin
COMMON_TOOLS_PATH	= $(PLATFORM_SDK_PATH)/Bin

LIB     := $(SDK_DIR)/lib;$(PLATFORM_SDK_DIR)/lib
INCLUDE := $(SDK_DIR)/include;$(PLATFORM_SDK_DIR)/include
PATH    := $(PLATFORM_TOOLS_PATH):$(VC_PATH)/IDE:$(PATH)

export LIB
export INCLUDE
export PATH

# get some vc6 specific defs
include ../win32/vc6_defs.mk
