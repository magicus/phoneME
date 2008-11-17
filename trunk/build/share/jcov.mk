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
# @(#)jcov.mk	1.24 06/10/24
#
#  Makefile for building the Jcov tool
#

###############################################################################
# Make definitions:

CVM_JCOV_BUILD_TOP      = $(CVM_BUILD_TOP)/jcov
CVM_JCOV_OBJDIR         := $(call abs2rel,$(CVM_JCOV_BUILD_TOP)/obj)
CVM_JCOV_LIBDIR         ?= $(CVM_LIBDIR)
CVM_JCOV_FLAGSDIR       = $(CVM_JCOV_BUILD_TOP)/flags

CVM_JCOV_SHAREROOT  = $(CVM_SHAREROOT)/tools/jcov
CVM_JCOV_TARGETROOT = $(CVM_TARGETROOT)/tools/jcov

CVM_JCOV_BUILDDIRS += \
        $(CVM_JCOV_OBJDIR) \
        $(CVM_JCOV_FLAGSDIR)

CVM_JCOV_LIB = $(LIB_PREFIX)jcov$(LIB_POSTFIX)

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

###############################################################################
# Make rules:

tools:: jcov
tool-clean: jcov-clean

jcov-clean:
	$(CVM_JCOV_CLEANUP_ACTION)

ifeq ($(CVM_JVMPI), true)
    jcov_build_list = jcov_initbuild $(CVM_JCOV_LIBDIR)/$(CVM_JCOV_LIB)
else
    jcov_build_list =
endif

jcov: $(jcov_build_list)

jcov_initbuild: jcov_check_cvm jcov_checkflags $(CVM_JCOV_BUILDDIRS)

# Make sure that CVM is built before building jcov.  If not, the issue a
# warning and abort.
jcov_check_cvm:
	@if [ ! -f $(CVM_BINDIR)/$(CVM) ]; then \
	    echo "Warning! Need to build CVM with before building Jcov."; \
	    exit 1; \
	else \
	    echo; echo "Building Jcov tool ..."; \
	fi

# Make sure all of the build flags files are up to date. If not, then do
# the requested cleanup action.
jcov_checkflags: $(CVM_JCOV_FLAGSDIR)
	@for filename in $(CVM_JCOV_FLAGS0); do \
		if [ ! -f $(CVM_JCOV_FLAGSDIR)/$${filename} ]; then \
			echo "Jcov flag $${filename} changed. Cleaning up."; \
			rm -f $(CVM_JCOV_FLAGSDIR)/$${filename%.*}.*; \
			touch $(CVM_JCOV_FLAGSDIR)/$${filename}; \
			$(CVM_JCOV_CLEANUP_OBJ_ACTION); \
		fi \
	done

$(CVM_JCOV_BUILDDIRS):
	@echo ... mkdir $@
	@if [ ! -d $@ ]; then mkdir -p $@; fi

$(CVM_JCOV_LIBDIR)/$(CVM_JCOV_LIB): $(CVM_JCOV_OBJECTS)
	@echo "Linking $@"
	$(SO_LINK_CMD)
	@echo "Done Linking $@"

# The following are used to build the .o files needed for $(CVM_JCOV_OBJECTS):

#####################################
# include all of the dependency files
#####################################
files := $(foreach file, $(wildcard $(CVM_JCOV_OBJDIR)/*.d), $(file))
ifneq ($(strip $(files)),)
    include $(files)
endif

$(CVM_JCOV_OBJDIR)/%.o: %.c
	@echo "... $@"
	$(SO_CC_CMD)
	$(GENERATEMAKEFILES_CMD)

$(CVM_JCOV_OBJDIR)/%.o: %.S
	@echo "... $@"
	$(SO_ASM_CMD)
