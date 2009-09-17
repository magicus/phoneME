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
# @(#)defs.mk	1.14 06/10/13
#


#
# defs for solaris-x86 target
#

CVM_TARGETOBJS_SPEED +=	\
    float_arch.o \
    x86_float_cpu.o	\

CVM_TARGETOBJS_OTHER += \
    invokeNative_i386.o \
    x86_double_divmul.o

ifeq ($(CVM_MP_SAFE), true)
CVM_TARGETOBJS_OTHER += \
        x86_membar.o
endif

CVM_SRCDIRS   += \
	$(CVM_TOP)/src/$(TARGET_OS)-$(TARGET_CPU_FAMILY)/javavm/runtime \

CVM_INCLUDE_DIRS  += \
	$(CVM_TOP)/src/$(TARGET_OS)-$(TARGET_CPU_FAMILY)

ifeq ($(CVM_JIT), true)

CVM_SRCDIRS += \
	$(CVM_TOP)/src/$(TARGET_OS)-$(TARGET_CPU_FAMILY)/javavm/runtime/jit

endif

