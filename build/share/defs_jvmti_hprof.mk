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
# @(#)hprof.mk	1.23 06/10/24
#

#
#  Makefile for building the Hprof tool
#

###############################################################################
# Make definitions:

CVM_JVMTI_HPROF_LIBDIR        ?= $(CVM_LIBDIR)
CVM_JVMTI_HPROF_LIB           = \
	$(CVM_JVMTI_HPROF_LIBDIR)/$(LIB_PREFIX)jvmtihprof$(LIB_POSTFIX)

CVM_JVMTI_HPROF_BUILD_TOP     = $(CVM_BUILD_TOP)/jvmti/hprof
CVM_JVMTI_HPROF_OBJDIR        = $(call abs2rel,$(CVM_JVMTI_HPROF_BUILD_TOP)/obj)
CVM_JVMTI_HPROF_FLAGSDIR      = $(CVM_JVMTI_HPROF_BUILD_TOP)/flags

CVM_JVMTI_HPROF_SHAREROOT     = $(CVM_SHAREROOT)/tools/jvmti/hprof
CVM_JVMTI_HPROF_TARGETROOT    = $(CVM_TARGETROOT)/tools/jvmti/hprof
CVM_JVMTI_HPROF_TARGETSHAREROOT = $(CDC_OS_COMPONENT_DIR)/src/$(TARGET_OS)

CVM_JVMTI_HPROF_BUILDDIRS += \
        $(CVM_JVMTI_HPROF_OBJDIR) \
        $(CVM_JVMTI_HPROF_FLAGSDIR)

#
# Search path for include files:
#
CVM_JVMTI_HPROF_INCLUDES  += \
	$(CVM_BUILD_TOP)/generated/javavm/include \
        $(CVM_JVMTI_HPROF_SHAREROOT) \
        $(CVM_JVMTI_HPROF_SHAREROOT)/../crw \
        $(CVM_JVMTI_HPROF_TARGETROOT)

ifeq ($(CVM_STATICLINK_TOOLS), true)
CVM_STATIC_TOOL_LIBS += $(CVM_JVMTI_HPROF_LIB)
endif

ifeq ($(CVM_DEBUG), true)
CVM_DEFINES += -DDEBUG
endif
#
# List of object files to build:
#
CVM_JVMTI_HPROF_SHAREOBJS += \
	debug_malloc.o \
	jvmti_hprof_blocks.o \
	jvmti_hprof_check.o \
	jvmti_hprof_class.o \
	jvmti_hprof_cpu.o \
	jvmti_hprof_error.o \
	jvmti_hprof_event.o \
	jvmti_hprof_frame.o \
	jvmti_hprof_init.o \
	jvmti_hprof_io.o \
	jvmti_hprof_ioname.o \
	jvmti_hprof_listener.o \
	jvmti_hprof_loader.o \
	jvmti_hprof_monitor.o \
	jvmti_hprof_object.o \
	jvmti_hprof_reference.o \
	jvmti_hprof_site.o \
	jvmti_hprof_stack.o \
	jvmti_hprof_string.o \
	jvmti_hprof_table.o \
	jvmti_hprof_tag.o \
	jvmti_hprof_tls.o \
	jvmti_hprof_trace.o \
	jvmti_hprof_tracker.o \
	jvmti_hprof_md.o \
	linker_md.o \
	jvmti_hprof_util.o



CVM_JVMTI_HPROF_OBJECTS0 = $(CVM_JVMTI_HPROF_SHAREOBJS) $(CVM_JVMTI_HPROF_TARGETOBJS)
CVM_JVMTI_HPROF_OBJECTS  = $(patsubst %.o,$(CVM_JVMTI_HPROF_OBJDIR)/%.o,$(CVM_JVMTI_HPROF_OBJECTS0))

CVM_JVMTI_HPROF_SRCDIRS  = \
	$(CVM_JVMTI_HPROF_SHAREROOT) \
	$(CVM_JVMTI_HPROF_TARGETROOT) \
	$(CVM_JVMTI_HPROF_TARGETSHAREROOT)/javavm/runtime \
	$(CVM_JVMTI_HPROF_SHAREROOT)/../crw

CVM_JVMTI_HPROF_FLAGS += \
        CVM_SYMBOLS \
        CVM_OPTIMIZED \
        CVM_DEBUG \
        CVM_DEBUG_CLASSINFO \
        CVM_JVMTI \
        CVM_DYNAMIC_LINKING \

CVM_JVMTI_HPROF_FLAGS0 = $(foreach flag, $(CVM_JVMTI_HPROF_FLAGS), $(flag).$($(flag)))

CVM_JVMTI_HPROF_CLEANUP_ACTION = \
        rm -rf $(CVM_JVMTI_HPROF_BUILD_TOP)

CVM_JVMTI_HPROF_CLEANUP_OBJ_ACTION = \
        rm -rf $(CVM_JVMTI_HPROF_OBJDIR)

