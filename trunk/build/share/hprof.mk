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

CVM_HPROF_BUILD_TOP     = $(CVM_BUILD_TOP)/hprof
CVM_HPROF_OBJDIR        := $(call abs2rel,$(CVM_HPROF_BUILD_TOP)/obj)
CVM_HPROF_LIBDIR        ?= $(CVM_LIBDIR)
CVM_HPROF_FLAGSDIR      = $(CVM_HPROF_BUILD_TOP)/flags

CVM_HPROF_SHAREROOT     = $(CVM_SHAREROOT)/tools/hprof
CVM_HPROF_TARGETROOT    = $(CVM_TARGETROOT)/tools/hprof

CVM_HPROF_BUILDDIRS += \
        $(CVM_HPROF_OBJDIR) \
        $(CVM_HPROF_FLAGSDIR)

CVM_HPROF_LIB = $(LIB_PREFIX)hprof$(LIB_POSTFIX)

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

###############################################################################
# Make rules:

tools:: hprof
tool-clean: hprof-clean

hprof-clean:
	$(CVM_HPROF_CLEANUP_ACTION)

ifeq ($(CVM_JVMPI), true)
    hprof_build_list = hprof_initbuild \
                       $(CVM_HPROF_LIBDIR)/$(CVM_HPROF_LIB) \
                       $(CVM_LIBDIR)/jvm.hprof.txt
else
    hprof_build_list =
endif

hprof: $(hprof_build_list)

hprof_initbuild: hprof_check_cvm hprof_checkflags $(CVM_HPROF_BUILDDIRS)

# Make sure that CVM is built before building hprof.  If not, the issue a
# warning and abort.
hprof_check_cvm:
	@if [ ! -f $(CVM_BINDIR)/$(CVM) ]; then \
	    echo "Warning! Need to build CVM with before building Hprof."; \
	    exit 1; \
	else \
	    echo; echo "Building Hprof tool ..."; \
	fi

# Make sure all of the build flags files are up to date. If not, then do
# the requested cleanup action.
hprof_checkflags: $(CVM_HPROF_FLAGSDIR)
	@for filename in $(CVM_HPROF_FLAGS0); do \
		if [ ! -f $(CVM_HPROF_FLAGSDIR)/$${filename} ]; then \
			echo "Hprof flag $${filename} changed. Cleaning up."; \
			rm -f $(CVM_HPROF_FLAGSDIR)/$${filename%.*}.*; \
			touch $(CVM_HPROF_FLAGSDIR)/$${filename}; \
			$(CVM_HPROF_CLEANUP_OBJ_ACTION); \
		fi \
	done

$(CVM_HPROF_BUILDDIRS):
	@echo ... mkdir $@
	@if [ ! -d $@ ]; then mkdir -p $@; fi

$(CVM_HPROF_LIBDIR)/$(CVM_HPROF_LIB): $(CVM_HPROF_OBJECTS)
	@echo "Linking $@"
	$(call SO_LINK_CMD, $(CVM_HPROF_LINKLIBS))
	@echo "Done Linking $@"

ifeq ($(CVM_JVMPI), true)
ifeq ($(CVM_JVMTI), false)
$(CVM_LIBDIR)/jvm.hprof.txt:
	@echo "Copying $@"
	@if [ ! -d $@ ]; then cp $(CVM_HPROF_SHAREROOT)/jvm.hprof.txt $@; fi
	@echo "Done Copying $@"
endif
endif
# The following are used to build the .o files needed for $(CVM_HPROF_OBJECTS):

#####################################
# include all of the dependency files
#####################################
files := $(foreach file, $(wildcard $(CVM_HPROF_OBJDIR)/*.d), $(file))
ifneq ($(strip $(files)),)
    include $(files)
endif

$(CVM_HPROF_OBJDIR)/%.o: %.c
	@echo "... $@"
	$(SO_CC_CMD)
	$(GENERATEMAKEFILES_CMD)

$(CVM_HPROF_OBJDIR)/%.o: %.S
	@echo "... $@"
	$(SO_ASM_CMD)
