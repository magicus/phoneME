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
# @(#)defs.mk	1.66 06/10/10
#
# defs for Darwin target
#

CVM_TARGETROOT	= $(CVM_TOP)/src/$(TARGET_OS)

#
# Platform specific defines
#
CVM_DEFINES += -D_GNU_SOURCE

#
# Platform specific source directories
#
CVM_SRCDIRS   += $(CVM_TOP)/src/portlibs/ansi_c
CVM_SRCDIRS   += $(CVM_TOP)/src/portlibs/posix
CVM_SRCDIRS   += $(CVM_TOP)/src/portlibs/dlfcn
CVM_SRCDIRS   += $(CVM_TOP)/src/portlibs/realpath
CVM_SRCDIRS   += \
	$(CVM_TARGETROOT)/javavm/runtime \
	$(CVM_TARGETROOT)/bin \
	$(CVM_TARGETROOT)/native/java/lang \
	$(CVM_TARGETROOT)/native/java/io \
	$(CVM_TARGETROOT)/native/java/net \
        $(CVM_TARGETROOT)/native/java/util \


CVM_INCLUDE_DIRS  += \
	$(CVM_TOP)/src \
	$(CVM_TARGETROOT) \
	$(CVM_TARGETROOT)/native/java/net \
	$(CVM_TARGETROOT)/native/common \

#
# Platform specific objects
#

CVM_TARGETOBJS_SPACE += \
	java_md.o \
	ansi_java_md.o \
	canonicalize_md.o \
	posix_sync_md.o \
	posix_threads_md.o \
	io_md.o \
	posix_io_md.o \
	posix_net_md.o \
	net_md.o \
	time_md.o \
	io_util.o \
	sync_md.o \
	system_md.o \
	threads_md.o \
	globals_md.o \
	java_props_md.o

ifeq ($(CVM_DYNAMIC_LINKING), true)
	CVM_TARGETOBJS_SPACE += linker_md.o
endif

###########################################
# Overrides of stuff setup in share/defs.mk
###########################################

#
# Open source (gtk, qt, etc) are usually stored by fink under /sw, so include
# /sw/include and /sw/lib
#
CVM_FINK_DIR	     = /sw
CVM_FINK_INCLUDE_DIR = $(CVM_FINK_DIR)/include
CVM_FINK_LIB_DIR     = $(CVM_FINK_DIR)/lib

# We need to turn off Apple's precompiled headers support.
CCFLAGS   	+= -no-cpp-precomp
# point to the dlcompat headers
CPPFLAGS	+= -isystem $(CVM_FINK_INCLUDE_DIR)

#
# Link options for darwin are a bit different than for other versions of unix.
# gcc doesn't like -Wl,-export-dynamic, but is does accept --export-dynamic.
# We also have to include --export-all and tell it where the dlcomp lib is.
#
LINKFLAGS	:= $(subst -Wl$(comma)-export-dynamic,,$(LINKFLAGS))
LINKFLAGS	+= --export-all --export-dynamic -Wl,-L$(CVM_FINK_LIB_DIR)

#
# Fixup SO_LINKFLAGS..
#
SO_LINKFLAGS	:= $(subst -shared,,$(SO_LINKFLAGS))
SO_LINKFLAGS	+= -bundle -undefined warning -flat_namespace
