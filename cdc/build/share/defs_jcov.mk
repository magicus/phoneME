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
# @(#)defs_jcov.mk	1.24 06/10/24
#
#  Makefile for building the Jcov tool
#

###############################################################################
# Make definitions:

CVM_JCOV_LIBDIR        ?= $(CVM_LIBDIR)
CVM_JCOV_LIB           = \
	$(CVM_JCOV_LIBDIR)/$(LIB_PREFIX)jcov$(LIB_POSTFIX)

CVM_JCOV_BUILD_TOP      = $(CVM_BUILD_TOP)/jcov
CVM_JCOV_OBJDIR         := $(call abs2rel,$(CVM_JCOV_BUILD_TOP)/obj)
CVM_JCOV_LIBDIR         ?= $(CVM_LIBDIR)
CVM_JCOV_FLAGSDIR       = $(CVM_JCOV_BUILD_TOP)/flags

CVM_JCOV_SHAREROOT  = $(CVM_SHAREROOT)/tools/jcov
CVM_JCOV_TARGETROOT = $(CVM_TARGETROOT)/tools/jcov
CVM_JCOV_TARGETSHAREROOT = $(CDC_OS_COMPONENT_DIR)/src/$(TARGET_OS)

CVM_JCOV_BUILDDIRS += \
        $(CVM_JCOV_OBJDIR) \
        $(CVM_JCOV_FLAGSDIR)

#
# Search path for include files:
#
CVM_JCOV_INCLUDE_DIRS  += \
        $(CVM_JCOV_SHAREROOT) \
        $(CVM_JCOV_TARGETROOT)

jcov : ALL_INCLUDE_FLAGS := \
	$(ALL_INCLUDE_FLAGS) $(call makeIncludeFlags,$(CVM_JCOV_INCLUDE_DIRS))

#
# List of object files to build:
#
CVM_JCOV_SHAREOBJS += \
        jcov.o \
        jcov_crt.o \
        jcov_error.o \
        jcov_events.o \
        jcov_file.o \
        jcov_hash.o \
        jcov_htables.o \
        jcov_java.o \
        jcov_setup.o \
        jcov_util.o \
	jcov_md.o

CVM_JCOV_OBJECTS0 = $(CVM_JCOV_SHAREOBJS) $(CVM_JCOV_TARGETOBJS)
CVM_JCOV_OBJECTS  = $(patsubst %.o,$(CVM_JCOV_OBJDIR)/%.o,$(CVM_JCOV_OBJECTS0))

CVM_JCOV_SRCDIRS  = \
	$(CVM_JCOV_SHAREROOT) \
	$(CVM_JCOV_TARGETROOT) \
	$(CVM_JCOV_TARGETSHAREROOT)/javavm/runtime

vpath %.c      $(CVM_JCOV_SRCDIRS)
vpath %.S      $(CVM_JCOV_SRCDIRS)

CVM_JCOV_FLAGS += \
        CVM_SYMBOLS \
        CVM_OPTIMIZED \
        CVM_DEBUG \
        CVM_DEBUG_CLASSINFO \
        CVM_JVMPI \
        CVM_JVMPI_TRACE_INSTRUCTION \
        CVM_DYNAMIC_LINKING \

CVM_JCOV_FLAGS0 = $(foreach flag, $(CVM_JCOV_FLAGS), $(flag).$($(flag)))

CVM_JCOV_CLEANUP_ACTION = \
        rm -rf $(CVM_JCOV_BUILD_TOP)

CVM_JCOV_CLEANUP_OBJ_ACTION = \
        rm -rf $(CVM_JCOV_OBJDIR)

