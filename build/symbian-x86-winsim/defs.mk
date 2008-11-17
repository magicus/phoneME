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
# @(#)defs.mk	1.8 06/10/24
#
# get some vc6 specific defs

SYMBIAN_MAKEFILE_PLATFORM = vc6
SYMBIAN_PLATFORM ?= wins

CVM_PROVIDE_TARGET_RULES = true

include ../symbian/winsim.mk

CVMEXT_LIB	= $(CVM_OBJDIR)/CVMEXT.LIB
#CLASSLIB_DEPS 	+= $(CVMEXT_LIB)
ifeq ($(SYMBIAN_PLATFORM),wins)
LL_LIB = $(CVM_BUILD_SUBDIR_NAME)/obj/ll.lib
CLASSLIB_DEPS += $(LL_LIB)
SYMBIAN_LINK_LIBS += LL.LIB
LL_OBJS = $(CVM_BUILD_SUBDIR_NAME)/.lst
endif

# Use CodeWarrior linker when building for winscw.
ifeq ($(SYMBIAN_PLATFORM), winscw)
TARGET_AR	= mwldsym2 -library
TARGET_AR_CREATE = $(TARGET_AR) -o $(1)
endif
