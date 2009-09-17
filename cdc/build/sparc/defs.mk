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
# @(#)defs.mk	1.13 06/10/10
#
# defs for sparc targets
#

#
# CAS (compare and swap) support
#
CVM_HAS_CAS	?= true
CVM_FLAGS	+= CVM_HAS_CAS
CVM_HAS_CAS_CLEANUP_ACTION	= $(CVM_DEFAULT_CLEANUP_ACTION)
ifeq ($(CVM_HAS_CAS), true)
CVM_DEFINES	+= -DCVM_HAS_CAS
# We must assemble with v8plus in order to get "cas" support.
ASM_ARCH_FLAGS  += -Wa,-xarch=v8plus
endif

#
# cpu specific objects
#

CVM_SRCDIRS   += \
	$(CVM_TOP)/src/$(TARGET_CPU_FAMILY)/javavm/runtime

CVM_INCLUDE_DIRS  += \
	$(CVM_TOP)/src/$(TARGET_CPU_FAMILY) \

#
# JIT related settings
#
ifeq ($(CVM_JIT), true)

CVM_JCS_CPU_INCLUDES_FILE =

CVM_JCS_CPU_DEFS_FILE     =

CVM_JCS_CPU_RULES_FILE    = \
    $(CVM_TOP)/src/sparc/javavm/runtime/jit/jitgrammarrules.jcs

ifeq ($(CVM_JIT_USE_FP_HARDWARE), true)
CVM_JCS_CPU_RULES_FILE    += \
    $(CVM_TOP)/src/sparc/javavm/runtime/jit/jitfloatgrammarrules.jcs
endif

include  $(CDC_DIR)/build/portlibs/defs_jit_risc.mk
endif
