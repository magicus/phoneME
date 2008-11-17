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
# @(#)defs.mk	1.30 06/10/24
#
# defs for win32-arm target
#

CVM_DEFINES	+= -DARM -D_ARM
# __RVCT__ indicates ARM syntax rather than GAS syntax
CVM_DEFINES	+= -D__RVCT__
TARGET_CC	= CLARM.EXE
TARGET_AS	= ARMASM.EXE

CCMCODECACHECOPY_CPU_O	= ccmcodecachecopy_arch.o

CVM_TARGETOBJS_OTHER +=	\
	atomic.o \
	invokeNative_sarm.o \

CVM_TARGETOBJS_SPEED +=	\
	arm_float_win32_arch.o

CVM_SRCDIRS   += \
	$(CVM_TOP)/src/$(TARGET_OS)-$(TARGET_CPU_FAMILY)/javavm/runtime

CVM_INCLUDE_DIRS  += \
	$(CVM_TOP)/src/$(TARGET_OS)-$(TARGET_CPU_FAMILY)

ifeq ($(CVM_JIT), true)
CVM_TARGETOBJS_SPACE +=	\
	jit_arch.o
CVM_TARGETOBJS_OTHER += \
        flushcache_arch.o
CVM_SRCDIRS   += \
	$(CVM_TOP)/src/$(TARGET_OS)-$(TARGET_CPU_FAMILY)/javavm/runtime/jit \
	$(CVM_TOP)/src/$(TARGET_OS)/javavm/runtime/jit
else
CVM_TARGETOBJS_OTHER +=	\
	memory_asm_cpu.o \

endif

# command for assembling
ASM_ARCH_FLAGS += -NOTerse -WIdth 132 -list $*.lst $*.i
define ASM_CMD
    $(AT)$(TARGET_CC) -D_ASM $(CPPFLAGS) /nologo -TC -P -EP $(call abs2rel,$<)
    $(AT)$(TARGET_AS) $(ASM_FLAGS) $@
    @rm $*.i $*.lst
endef
