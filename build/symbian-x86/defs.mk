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
# @(#)defs.mk	1.15 06/10/24
#
# defs for symbian-x86 target
#

CVM_DEFINES	+= -D_X86_ -Dx86
# We need this to build invokeNative_i386.s.
TARGET_AS	= gcc

# FIXME: The wins udeb build doesn't seem to work. 'make clean' is required if 
# you change any build flags.
#SYMBIAN_BLD	= urel

SYMBIAN_LINK_LIBS = ESTLIB.LIB ESOCK.LIB INSOCK.LIB EUSER.LIB CHARCONV.LIB \
	ESTW32.LIB EFSRV.LIB CVMEXT.LIB
ifeq ($(findstring S60, $(SYMBIAN_DEVICE)), S60)
SYMBIAN_LINK_LIBS += ECRT0.LIB
else
SYMBIAN_LINK_LIBS += WCRT0.LIB WSERV.LIB
endif
ifneq ($(CVM_DLL),true)
SYMBIAN_LINK_LIBS += EEXE.LIB
endif

SYMBIAN_DEFDIR	= BWINS

CVM_TARGETOBJS_SPEED +=	\
    float_arch.o \

ifeq ($(CVM_MP_SAFE), true)
CVM_TARGETOBJS_OTHER += \
        x86_membar.o
endif

CVM_SRCDIRS   += \
	$(CVM_TOP)/src/$(TARGET_OS)-$(TARGET_CPU_FAMILY)/javavm/runtime \

CVM_INCLUDE_DIRS  += \
	$(CVM_TOP)/src/$(TARGET_OS)-$(TARGET_CPU_FAMILY)

CVM_TARGETOBJS_SPEED +=	\
	x86_float_cpu2.o \

CVM_SRCDIRS   += \
	$(CVM_TOP)/src/win32-x86/javavm/runtime

CVM_INCLUDE_DIRS  += \
	$(CVM_TOP)/src/win32-x86
