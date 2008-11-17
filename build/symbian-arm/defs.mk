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
# @(#)defs.mk	1.11 06/10/29
#
# defs for symbian-x86 target
#

SYMBIAN_PLATFORM ?= armv5
ifeq ($(SYMBIAN_PLATFORM), armv5)
# Specify armcc --gnu flag for GNU extension.
SYMBIAN_USER_OPTION = OPTION ARMCC --gnu
CVM_DEFINES	+= -D__RVCT__
endif

# By default, Symbian assumes THUMB mode is used for user-side. Specifies
# ALWAYS_BUILD_AS_ARM in MMP file for ARM mode.
SYMBIAN_BUILD_AS_ARM = ALWAYS_BUILD_AS_ARM

# FIXME: Symbian/ARM does not support the spinlock microlock, which means
# we can't use the ARM microlock assembler glue.
CVM_DEFINES	+= -DCVM_JIT_CCM_USE_C_SYNC_HELPER

SYMBIAN_LINK_LIBS += efsrv.lib \
	c32.lib \
	esock.lib \
	insock.lib \
	euser.lib \
	estlib.lib \
	charconv.lib

SYMBIAN_DEFDIR	= BMARM

CVM_TARGETOBJS_SPACE += ansi_java_md.o java_md.o

#CPPFLAGS += -D__SYMBIAN32__ -D__EPOC32__ -D__MARM__ -D__MARM_ARM4__ -D_UNICODE -D__EXE__
ASM_ARCH_FLAGS = -c -traditional

CVM_TARGETOBJS_SPEED +=	\
	arm_float_cpu.o \
	float_arch.o

# For armv5, just build invokeNative_arm normally.  Otherwise, use
# some special rules in symbian-arm/rules.mk to build it.

ifeq ($(SYMBIAN_PLATFORM), armv5)
CVM_TARGETOBJS_OTHER += \
        invokeNative_arm.o
endif

ifeq ($(CVM_MP_SAFE), true)
CVM_TARGETOBJS_OTHER += \
        x86_membar.o
endif

CVM_SRCDIRS   += \
	$(CVM_TOP)/src/$(TARGET_OS)-$(TARGET_CPU_FAMILY)/javavm/runtime \

CVM_INCLUDE_DIRS  += \
	$(CVM_TOP)/src/$(TARGET_OS)-$(TARGET_CPU_FAMILY)

#
# JIT related settings
#
ifeq ($(CVM_JIT), true)

CVM_TARGETOBJS_SPACE += \
	jit_arch.o \

CVM_TARGETOBJS_OTHER += \
	flushcache_arch.o

CVM_SRCDIRS   += \
	$(CVM_TOP)/src/$(TARGET_OS)-$(TARGET_CPU_FAMILY)/javavm/runtime/jit

# Compile ccmcodecachecopy_cpu.S separately and link as a library for 'armv4'
# target. For 'armv5' target, run CPP on the *.S files and add the generated
# files to the MMP project file.
ifneq ($(SYMBIAN_PLATFORM), armv5)
JIT_CPU_O =
CCMCODECACHECOPY_CPU_O =
endif
endif

ifneq ($(SYMBIAN_PLATFORM), armv5)
CVMEXT_LIB = $(CVM_OBJDIR)/cvmext.lib
endif

# Forth building JCS before CVMEXT.LIB since we need to add
# Symbian gcc to the PATH for CCMCODECACHECOPY.LIB.
#CLASSLIB_DEPS += $(CVM_CODEGEN_CFILES) $(CVMEXT_LIB)

ifneq ($(SYMBIAN_PLATFORM), armv5)
SYMBIAN_LINK_LIBS += CVMEXT.LIB \
	"../../../gcc/lib/gcc-lib/arm-epoc-pe/2.9-psion-98r2/libgcc.a"
endif
