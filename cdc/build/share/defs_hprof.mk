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
# @(#)defs_hprof.mk	1.23 06/10/24
#

#
#  Makefile for building the Hprof tool
#

###############################################################################
# Make definitions:

CVM_HPROF_LIBDIR        ?= $(CVM_LIBDIR)
CVM_HPROF_LIB           = \
	$(CVM_HPROF_LIBDIR)/$(LIB_PREFIX)hprof$(LIB_POSTFIX)

CVM_HPROF_BUILD_TOP     = $(CVM_BUILD_TOP)/hprof
CVM_HPROF_OBJDIR        := $(call abs2rel,$(CVM_HPROF_BUILD_TOP)/obj)
CVM_HPROF_LIBDIR        ?= $(CVM_LIBDIR)
CVM_HPROF_FLAGSDIR      = $(CVM_HPROF_BUILD_TOP)/flags

CVM_HPROF_SHAREROOT     = $(CVM_SHAREROOT)/tools/hprof
CVM_HPROF_TARGETROOT    = $(CVM_TARGETROOT)/tools/hprof
CVM_HPROF_TARGETSHAREROOT = $(CDC_OS_COMPONENT_DIR)/src/$(TARGET_OS)

CVM_HPROF_BUILDDIRS += \
        $(CVM_HPROF_OBJDIR) \
        $(CVM_HPROF_FLAGSDIR)


#
# Search path for include files:
#
CVM_HPROF_INCLUDE_DIRS  += \
        $(CVM_HPROF_SHAREROOT) \
        $(CVM_HPROF_TARGETROOT)

hprof : ALL_INCLUDE_FLAGS := \
	$(ALL_INCLUDE_FLAGS) $(call makeIncludeFlags,$(CVM_HPROF_INCLUDE_DIRS))

#
# List of object files to build:
#
CVM_HPROF_SHAREOBJS += \
        hprof.o \
        hprof_class.o \
        hprof_cpu.o \
        hprof_gc.o \
        hprof_hash.o \
        hprof_heapdump.o \
        hprof_io.o \
        hprof_jni.o \
        hprof_listener.o \
        hprof_method.o \
        hprof_monitor.o \
        hprof_name.o \
        hprof_object.o \
        hprof_setup.o \
        hprof_site.o \
        hprof_thread.o \
        hprof_trace.o \
	hprof_md.o

CVM_HPROF_OBJECTS0 = $(CVM_HPROF_SHAREOBJS) $(CVM_HPROF_TARGETOBJS)
CVM_HPROF_OBJECTS  = $(patsubst %.o,$(CVM_HPROF_OBJDIR)/%.o,$(CVM_HPROF_OBJECTS0))

CVM_HPROF_SRCDIRS  = \
	$(CVM_HPROF_SHAREROOT) \
	$(CVM_HPROF_TARGETROOT) \
	$(CVM_HPROF__TARGETSHAREROOT)/javavm/runtime


vpath %.c      $(CVM_HPROF_SRCDIRS)
vpath %.S      $(CVM_HPROF_SRCDIRS)

CVM_HPROF_FLAGS += \
        CVM_SYMBOLS \
        CVM_OPTIMIZED \
        CVM_DEBUG \
        CVM_DEBUG_CLASSINFO \
        CVM_JVMPI \
        CVM_JVMPI_TRACE_INSTRUCTION \
        CVM_DYNAMIC_LINKING \

CVM_HPROF_FLAGS0 = $(foreach flag, $(CVM_HPROF_FLAGS), $(flag).$($(flag)))

CVM_HPROF_CLEANUP_ACTION = \
        rm -rf $(CVM_HPROF_BUILD_TOP)

CVM_HPROF_CLEANUP_OBJ_ACTION = \
        rm -rf $(CVM_HPROF_OBJDIR)

