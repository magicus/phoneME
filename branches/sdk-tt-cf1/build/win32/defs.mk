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
# @(#)defs.mk	1.42 06/10/24
#
# defs for windows target
#

#
# We rely on many things setup in build/share/defs.mk. However, since
# it is meant primarily unix build environments and GNU tools, we have
# to override a lot of values.
#

# Miscellaneous options we override
CVM		= cvmi.dll
CVMLIB		= cvm.o
override GENERATEMAKEFILES = false  
TARGET_CCC = $(TARGET_CC)

WIN_LINKLIBS += wininet.lib

WIN32_PLATFORM ?= win32
JAVACALL_TARGET ?= $(WIN32_PLATFORM)_$(TARGET_CPU_FAMILY)

#
# Platform source directory
#
CVM_TARGETROOT	= $(CVM_TOP)/src/win32

#
# Platform specific defines
#
CVM_DEFINES      += -DHAVE_64_BIT_IO

ifneq ($(CVM_DEBUG_ASSERTS), true)
	CVM_DEFINES += -DNDEBUG
endif

#
# Platform specific source directories
#
CVM_SRCDIRS   += $(CVM_TOP)/src/portlibs/ansi_c
CVM_SRCDIRS   += \
	$(CVM_TARGETROOT)/javavm/runtime \
	$(CVM_TARGETROOT)/bin \
	$(CVM_TARGETROOT)/native/java/lang \
	$(CVM_TARGETROOT)/native/java/io \
	$(CVM_TARGETROOT)/native/java/net \
	$(CVM_TARGETROOT)/native/java/util \
	$(CVM_TARGETROOT)/native/sun/security/provider \

CVM_INCLUDE_DIRS  += \
	$(CVM_TOP)/src \
	$(CVM_TARGETROOT) \
        $(CVM_TARGETROOT)/native/java/net \
        $(CVM_TARGETROOT)/native/java/util \
        $(CVM_TARGETROOT)/native/common \

#
# Platform specific objects
#
CVM_TARGETOBJS_SPACE += \
	io_sockets.o \
	io_md.o \
	net_md.o \
	time_md.o \
	io_util.o \
	sync_md.o \
	system_md.o \
	threads_md.o \
	globals_md.o \
	java_props_md.o \
	tchar.o \
        memory_md.o

ifeq ($(CVM_DYNAMIC_LINKING), true)
	CVM_TARGETOBJS_SPACE += linker_md.o
endif
ifeq ($(CVM_DEBUG), true) 
	AFXLIB = uafxwced.lib
	# verbose heap debugging
	# CVM_DEFINES += -D_CRTDBG_MAP_ALLOC
else
	AFXLIB = uafxwce.lib
endif

ifeq ($(CVM_JIT), true)
CVM_SRCDIRS   += \
        $(CVM_TARGETROOT)/javavm/runtime/jit
CVM_TARGETOBJS_SPACE += \
        jit_md.o
endif

# Is CVM a DLL (not an EXE)?
CVM_DLL = true
include ../win32/host_defs.mk
