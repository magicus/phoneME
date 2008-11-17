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
# @(#)defs.mk	1.16 06/10/24
#
# defs for win32-mips target
#

CVM_DEFINES	+= -DMIPS -D_MIPS_ -DWINCE_MIPS
TARGET_CC	= CLMIPS.EXE
TARGET_AS	= MIPSASM.EXE

CVM_TARGETOBJS_OTHER +=	\
	atomic_mips.o \
	invokeNative_mips.o  

CVM_TARGETOBJS_SPEED +=	\
	mips_float_win32_arch.o \

CVM_SRCDIRS   += \
	$(CVM_TOP)/src/$(TARGET_OS)-$(TARGET_CPU_FAMILY)/javavm/runtime

CVM_INCLUDE_DIRS  += \
	$(CVM_TOP)/src/$(TARGET_OS)-$(TARGET_CPU_FAMILY)

ifeq ($(CVM_JIT), true)
CVM_TARGETOBJS_SPACE +=	\
	jit_arch.o
CVM_SRCDIRS   += \
	$(CVM_TOP)/src/$(TARGET_OS)-$(TARGET_CPU_FAMILY)/javavm/runtime/jit
endif

# command for assembling
ASM_ARCH_FLAGS += /nologo /c
define ASM_CMD
    $(AT)$(TARGET_CC) -D_ASM $(CPPFLAGS) /nologo -TC -P -EP $(call abs2rel,$<)
    $(AT)$(TARGET_AS) -D_ASM $(CPPFLAGS) $(ASM_FLAGS) /Fo$(call abs2rel,$<) $@
    @rm $*.i
endef
