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
# @(#)defs.mk	1.2 06/10/10
#

# Description of the VC 2005 win32 platform.
# The following are all for the benefit of win32/defs.mk
VS8_DIR			?= C:/Program Files/Microsoft Visual Studio 8
VS8_PATH		= $(call WIN2POSIX,$(VS8_DIR))
VC_DIR			= $(VS8_DIR)/VC
VC_PATH			= $(VS8_PATH)/VC
PLATFORM_SDK_DIR	?= C:/Program Files/Microsoft Visual Studio 8/VC/PlatformSDK
PLATFORM_SDK_PATH	= $(call WIN2POSIX,$(PLATFORM_SDK_DIR))
PLATFORM_TOOLS_PATH	= $(VC_PATH)/bin
COMMON_TOOLS_PATH	= $(PLATFORM_SDK_PATH)/Bin
TARGET_CC		= cl.exe
TARGET_LINK		= link.exe

LIB     := $(VC_DIR)/lib;$(PLATFORM_SDK_DIR)/lib
INCLUDE := $(VC_DIR)/include;$(PLATFORM_SDK_DIR)/include
PATH    := $(PLATFORM_TOOLS_PATH):$(VS8_PATH)/Common7/IDE:$(PATH)

export LIB
export INCLUDE
export PATH
export USE_VS2005=true

# get some vc specific defs
include ../win32/vc_defs.mk
