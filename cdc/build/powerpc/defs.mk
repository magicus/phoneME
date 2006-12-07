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
# @(#)defs.mk	1.19 06/10/13
#
# defs for powerpc targets
#

#
# All the cpu specific build flags we need to keep track of in
# case they are toggled.
#

# WARNING: using fmadd results in a non-compliant vm. Some floating
# point tck tests will fail! Therefore, this support if off by defualt.
CVM_JIT_USE_FMADD	?= false

CVM_FLAGS += \
	CVM_JIT_USE_FMADD \

CVM_JIT_USE_FMADD_CLEANUP_ACTION	= $(CVM_JIT_CLEANUP_ACTION)

#
# cpu specific defines
#
ifeq ($(CVM_JIT_USE_FMADD), true)
CVM_DEFINES	+= -DCVM_JIT_USE_FMADD
endif

#
# cpu specific objects
#

CVM_SRCDIRS   += \
	$(CVM_TOP)/src/$(TARGET_CPU_FAMILY)/javavm/runtime

CVM_INCLUDES  += \
	-I$(CVM_TOP)/src/$(TARGET_CPU_FAMILY) \

#
# JIT related settings
#
ifeq ($(CVM_JIT), true)

CVM_JCS_CPU_RULES_FILE    = \
    $(CVM_TOP)/src/powerpc/javavm/runtime/jit/jitgrammarrules.jcs

ifeq ($(CVM_JIT_USE_FP_HARDWARE), true)
CVM_JCS_CPU_RULES_FILE    += \
    $(CVM_TOP)/src/powerpc/javavm/runtime/jit/jitfloatgrammarrules.jcs
# WARNING: using fmadd results in a non-compliant vm. Some floating
# point tck tests will fail. Therefore, this support if off by defualt.
ifeq ($(CVM_JIT_USE_FMADD), true)
CVM_JCS_CPU_RULES_FILE    += \
    $(CVM_TOP)/src/powerpc/javavm/runtime/jit/jitfmaddgrammarrules.jcs
endif
endif

# Copy ccm assembler code to the codecache so it is reachable
# with a branch instruction.
CVM_JIT_COPY_CCMCODE_TO_CODECACHE ?= true

CVM_JIT_PATCHED_METHOD_INVOCATIONS = true
ifeq ($(CVM_JIT_PATCHED_METHOD_INVOCATIONS), true)
ifneq ($(CVM_JIT_COPY_CCMCODE_TO_CODECACHE), true)
$(error cannot specify CVM_JIT_PATCHED_METHOD_INVOCATIONS=true with CVM_JIT_COPY_CCMCODE_TO_CODECACHE=false)
endif
endif

include  ../portlibs/defs_jit_risc.mk

endif
