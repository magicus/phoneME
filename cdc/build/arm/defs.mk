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
# @(#)defs.mk	1.26 06/10/13
#

#
# defs for arm targets
#

CVM_TARGETOBJS_SPEED +=	\

CVM_TARGETOBJS_OTHER += \

CVM_SRCDIRS   += \
	$(CVM_TOP)/src/$(TARGET_CPU_FAMILY)/javavm/runtime

CVM_INCLUDE_DIRS  += \
	$(CVM_TOP)/src/$(TARGET_CPU_FAMILY)

#
# JIT related settings
#
ifeq ($(CVM_JIT), true)

CVM_TARGETOBJS_SPEED +=	\
    ccmintrinsics_cpu.o

CVM_JCS_CPU_INCLUDES_FILE = \
    $(CVM_TOP)/src/arm/javavm/runtime/jit/jitgrammarincludes.jcs

CVM_JCS_CPU_DEFS_FILE     = \
    $(CVM_TOP)/src/arm/javavm/runtime/jit/jitgrammardefs.jcs

CVM_JCS_CPU_RULES_FILE    = \
    $(CVM_TOP)/src/arm/javavm/runtime/jit/jitgrammarrules.jcs

# Copy ccm assembler code to the codecache so it is reachable
# with a branch instruction.
CVM_JIT_COPY_CCMCODE_TO_CODECACHE ?= true

CVM_JIT_PMI ?= false
ifeq ($(CVM_JIT_PMI), true)
ifneq ($(CVM_JIT_COPY_CCMCODE_TO_CODECACHE), true)
$(error cannot specify CVM_JIT_PMI=true with CVM_JIT_COPY_CCMCODE_TO_CODECACHE=false)
endif
endif

ifeq ($(CVM_JIT_USE_FP_HARDWARE), true)
CVM_JCS_CPU_RULES_FILE    += \
    $(CVM_TOP)/src/arm/javavm/runtime/jit/jitfloatgrammarrules.jcs
endif

include  $(CDC_DIR)/build/portlibs/defs_jit_risc.mk

endif

#
# Hardware settings.
#

ifdef HW_DIR
  ifeq ($(wildcard $(HW_DIR)/build/subsystem.gmk),)
    $(error HW_DIR must point to a HW directory)
  endif
  include $(HW_DIR)/build/subsystem.gmk
endif
