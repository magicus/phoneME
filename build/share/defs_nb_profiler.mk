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
# @(#)nb_profiler.mk	1.0 09/04/24
#

#
#  Makefile for building the netbeans profiler agent
#

###############################################################################
# Make definitions:

CVM_NB_PROFILER_LIBDIR        ?= $(CVM_LIBDIR)
CVM_NB_PROFILER_LIB           = \
	$(CVM_NB_PROFILER_LIBDIR)/$(LIB_PREFIX)profilerinterface$(LIB_POSTFIX)

CVM_NB_PROFILER_BUILD_TOP     = $(CVM_BUILD_TOP)/jvmti/nb_profiler
CVM_NB_PROFILER_OBJDIR        = $(call abs2rel,$(CVM_NB_PROFILER_BUILD_TOP)/obj)
CVM_NB_PROFILER_FLAGSDIR      = $(CVM_NB_PROFILER_BUILD_TOP)/flags

CVM_NB_PROFILER_SHAREROOT     = $(CVM_SHAREROOT)/tools/jvmti/nb_profiler
CVM_NB_PROFILER_TARGETROOT    = $(CVM_TARGETROOT)/tools/jvmti/nb_profiler
CVM_NB_PROFILER_TARGETSHAREROOT = $(CDC_OS_COMPONENT_DIR)/src/$(TARGET_OS)

CVM_NB_PROFILER_BUILDDIRS += \
        $(CVM_NB_PROFILER_OBJDIR) \
        $(CVM_NB_PROFILER_FLAGSDIR)

#
# Search path for include files:
#
CVM_NB_PROFILER_INCLUDES  += \
	$(CVM_BUILD_TOP)/generated/javavm/include \
        $(CVM_NB_PROFILER_SHAREROOT) \
        $(CVM_NB_PROFILER_TARGETROOT)

ifeq ($(CVM_STATICLINK_TOOLS), true)
CVM_STATIC_TOOL_LIBS += $(CVM_NB_PROFILER_LIB)
endif

#
# List of object files to build:
#
CVM_NB_PROFILER_SHAREOBJS += \
	class_file_cache.o \
	attach.o \
	Classes.o \
	Timers.o \
	GC.o \
	Threads.o \
	Stacks.o \
	common_functions.o \
	common_functions_md.o


CVM_NB_PROFILER_OBJECTS0 = $(CVM_NB_PROFILER_SHAREOBJS) $(CVM_NB_PROFILER_TARGETOBJS)
CVM_NB_PROFILER_OBJECTS  = $(patsubst %.o,$(CVM_NB_PROFILER_OBJDIR)/%.o,$(CVM_NB_PROFILER_OBJECTS0))

CVM_NB_PROFILER_SRCDIRS  = \
	$(CVM_NB_PROFILER_SHAREROOT) \
	$(CVM_NB_PROFILER_TARGETROOT) \
	$(CVM_NB_PROFILER_TARGETSHAREROOT)/javavm/runtime

CVM_NB_PROFILER_FLAGS += \
        CVM_SYMBOLS \
        CVM_OPTIMIZED \
        CVM_DEBUG \
        CVM_DEBUG_CLASSINFO \
        CVM_JVMTI \
        CVM_DYNAMIC_LINKING \

CVM_NB_PROFILER_FLAGS0 = $(foreach flag, $(CVM_NB_PROFILER_FLAGS), $(flag).$($(flag)))

CVM_NB_PROFILER_CLEANUP_ACTION = \
        rm -rf $(CVM_NB_PROFILER_BUILD_TOP)

CVM_NB_PROFILER_CLEANUP_OBJ_ACTION = \
        rm -rf $(CVM_NB_PROFILER_OBJDIR)

