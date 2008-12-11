#
# @(#)defs.mk	1.59 06/10/24
#
# Portions Copyright  2000-2008 Sun Microsystems, Inc. All Rights
# Reserved.  Use is subject to license terms.
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

#
# defs for linux-arm target
#

# WARNING: code scheduling does not appear to be working, so don't
# set CVM_JIT_CODE_SCHED=true.

# enable/disable all IAI optimizations
CVM_IAI_OPT_ALL ?= true
CVM_FLAGS	+= CVM_IAI_OPT_ALL
CVM_IAI_OPT_ALL_CLEANUP_ACTION	= $(CVM_DEFAULT_CLEANUP_ACTION)
ifeq ($(CVM_IAI_OPT_ALL), true)
CVM_DEFINES += -DCVM_IAI_OPT_ALL
else
override CVM_JIT_CODE_SCHED = false
endif

# enable/disable code scheduling
ifneq ($(CVM_JIT),true)
override CVM_JIT_CODE_SCHED = false
else
CVM_JIT_CODE_SCHED ?= false
endif

CVM_JIT_CODE_SCHED_CLEANUP_ACTION = $(CVM_DEFAULT_CLEANUP_ACTION)
CVM_FLAGS	+= CVM_JIT_CODE_SCHED
ifeq ($(CVM_JIT_CODE_SCHED), true)
CVM_DEFINES 	+= -DCVM_JIT_CODE_SCHED
endif

# for enabling the use of hard float (not recommended if no fpu)
CVM_FORCE_HARD_FLOAT	?= false
CVM_FLAGS += CVM_FORCE_HARD_FLOAT
CVM_FORCE_HARD_FLOAT_CLEANUP_ACTION = $(CVM_DEFAULT_CLEANUP_ACTION)

SO_CFLAGS       += -fpic
ASM_ARCH_FLAGS  += -traditional

CVM_TARGETOBJS_SPEED += \
	arm_float_cpu.o

CVM_TARGETOBJS_OTHER += \
	invokeNative_arm.o \
	atomic_arm.o

ifneq ($(CVM_JIT), true)
CVM_TARGETOBJS_OTHER += \
	memory_asm_cpu.o
endif

# segvhandler_arch only needed for the JIT and MEM_MGR
ifeq ($(findstring true,$(CVM_JIT)$(CVM_USE_MEM_MGR)), true)
CVM_TARGETOBJS_SPACE += segvhandler_arch.o
endif

CVM_SRCDIRS   += \
	$(CVM_TOP)/src/$(TARGET_OS)-$(TARGET_CPU_FAMILY)/javavm/runtime

CVM_INCLUDE_DIRS  += \
	$(CVM_TOP)/src/$(TARGET_OS)-$(TARGET_CPU_FAMILY)

ifneq ($(CVM_FORCE_HARD_FLOAT), true)
ifeq ($(USE_GCC2), true)
	CC_ARCH_FLAGS   += -msoft-float
	ASM_ARCH_FLAGS  += -msoft-float
	LINK_ARCH_FLAGS += -msoft-float
	LINK_ARCH_LIBS  += -lfloat
	CVM_TARGETOBJS_OTHER += _fixunsdfsi.o
endif
endif

# Our source needs to know if AAPCS calling conventions are used
USE_AAPCS ?= false
CVM_FLAGS += USE_AAPCS
USE_AAPCS_CLEANUP_ACTION = $(CVM_DEFAULT_CLEANUP_ACTION)
ifeq ($(USE_AAPCS),true)
CVM_DEFINES += -DAAPCS 
endif

#
# JIT related settings
#

ifeq ($(CVM_JIT), true)

CVM_TARGETOBJS_SPACE += \

CVM_TARGETOBJS_OTHER += \
	flushcache_arch.o

endif

ifeq ($(CVM_JIT_CODE_SCHED), true)
CVM_TARGETOBJS_SPEED += jitcodesched.o
endif
