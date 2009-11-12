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
# defs for Windows Mobile 5 target
#

# Must use DOS path for SDK_DIR with no quotes.
# Can be overridden on the make command line or exported from shell
SDK_DIR         ?= C:/Program Files/Windows CE Tools

ifndef VS8_DIR
VS8_REG_KEY	:= 'HKLM\Software\Microsoft\VisualStudio\8.0\Setup\VS'
VS8_REG_VALUE	:= ProductDir
VS8_REG_DIR	:= $(call WIN32_QUERY_REG,$(VS8_REG_KEY),$(VS8_REG_VALUE))
ifdef VS8_REG_DIR
VS8_DIR		:= $(subst \,/,$(VS8_REG_DIR))<EOL>
VS8_DIR		:= $(subst /<EOL>,<EOL>,$(VS8_DIR))
VS8_DIR		:= $(subst <EOL>,,$(VS8_DIR))
endif
endif

VS8_DIR         ?= C:/Program Files/Microsoft Visual Studio 8
VS8_PATH         = $(call WIN2POSIX,$(VS8_DIR))
VC_DIR           = $(VS8_DIR)/VC
VC_PATH          = $(VS8_PATH)/VC

PLATFORM_TOOLS_PATH	= $(VC_PATH)/ce/bin/x86_arm
COMMON_TOOLS_PATH0	= $(VS8_PATH)/Common7/Tools/Bin:$(VS8_PATH)/Common7/IDE
COMMON_TOOLS_PATH	= $(VC_PATH)/bin:$(COMMON_TOOLS_PATH0)

include ../win32/wince50_defs.mk

INCLUDE0 := $(INCLUDE)
INCLUDE := $(INCLUDE);$(VC_DIR)/ce/include
INCLUDE := $(INCLUDE);$(VC_DIR)/ce/atlmfc/include

LIB0 := $(LIB)
LIB := $(LIB);$(VC_DIR)/ce/lib/armv4i
LIB := $(LIB);$(VC_DIR)/ce/atlmfc/lib/armv4i

CVM_DEFINES +=  -DPOCKETPC
CC_ARCH_FLAGS  += /GS-
TARGET_CC      = CL.EXE 

#####
##### FIXME: Adding this here to force dependency in PCSL makefiles to build
#####  with Microsoft Visual Studio 2005 (which is needed for Windows Mobile
#####  5.0 & 6.0 builds).  Need to change to eventually unify how Makefiles
#####  deal with Compiler variables in both CDC and CLDC based builds.
export USE_VS2005=true
export VS2005_CE_ARM_LIB := '$(LIB0)'
export VS2005_CE_ARM_INCLUDE := $(INCLUDE0)
export VS2005_CE_ARM_PATH := "$(PLATFORM_TOOLS_PATH)"
export VS2005_COMMON_PATH := '$(VS8_PATH)/VC/bin'

