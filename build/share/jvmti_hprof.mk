#
# Copyright  2007 Sun Microsystems, Inc. All Rights Reserved.  
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

CVM_JVMTI_HPROF_LIB           = $(LIB_PREFIX)jvmtihprof$(LIB_POSTFIX)

CVM_JVMTI_HPROF_LIBDIR        = $(CVM_LIBDIR)

CVM_JVMTI_HPROF_BUILD_TOP     = $(CVM_BUILD_TOP)/jvmti/hprof
CVM_JVMTI_HPROF_OBJDIR        = $(CVM_JVMTI_HPROF_BUILD_TOP)/obj
CVM_JVMTI_HPROF_FLAGSDIR      = $(CVM_JVMTI_HPROF_BUILD_TOP)/flags

CVM_JVMTI_HPROF_SHAREROOT     = $(CVM_SHAREROOT)/tools/jvmti/hprof
CVM_JVMTI_HPROF_TARGETROOT    = $(CVM_TARGETROOT)/tools/jvmti/hprof

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

jvmti_hprof : CVM_INCLUDE_DIRS += $(CVM_JVMTI_HPROF_INCLUDES)
jvmti_hprof : CVM_DEFINES += -DSKIP_NPT -DHPROF_LOGGING

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
	jvmti_hprof_util.o



CVM_JVMTI_HPROF_OBJECTS0 = $(CVM_JVMTI_HPROF_SHAREOBJS) $(CVM_JVMTI_HPROF_TARGETOBJS)
CVM_JVMTI_HPROF_OBJECTS  = $(patsubst %.o,$(CVM_JVMTI_HPROF_OBJDIR)/%.o,$(CVM_JVMTI_HPROF_OBJECTS0))

CVM_JVMTI_HPROF_SRCDIRS  = \
	$(CVM_JVMTI_HPROF_SHAREROOT) \
	$(CVM_JVMTI_HPROF_TARGETROOT) \
	$(CVM_JVMTI_HPROF_SHAREROOT)/../crw

vpath %.c      $(CVM_JVMTI_HPROF_SRCDIRS)
vpath %.S      $(CVM_JVMTI_HPROF_SRCDIRS)

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

###############################################################################
# Make rules:

tools:: jvmti_hprof
tool-clean: jvmti_hprof-clean

jvmti_hprof-clean:
	$(CVM_JVMTI_HPROF_CLEANUP_ACTION)

ifeq ($(CVM_JVMTI), true)
    jvmti_hprof_build_list = jvmti_hprof_initbuild \
                       $(CVM_JVMTI_HPROF_LIBDIR)/$(CVM_JVMTI_HPROF_LIB) \
                       $(CVM_LIBDIR)/jvm.hprof.txt
else
    jvmti_hprof_build_list =
endif

jvmti_hprof: $(jvmti_hprof_build_list)

jvmti_hprof_initbuild: jvmti_hprof_check_cvm jvmti_hprof_checkflags $(CVM_JVMTI_HPROF_BUILDDIRS)

# Make sure that CVM is built before building hprof.  If not, the issue a
# warning and abort.
jvmti_hprof_check_cvm:
	@if [ ! -f $(CVM_BINDIR)/$(CVM) ]; then \
	    echo "Warning! Need to build CVM before building hprof."; \
	    exit 1; \
	else \
	    echo; echo "Building hprof tool ..."; \
	fi

# Make sure all of the build flags files are up to date. If not, then do
# the requested cleanup action.
jvmti_hprof_checkflags: $(CVM_JVMTI_HPROF_FLAGSDIR)
	@for filename in $(CVM_JVMTI_HPROF_FLAGS0); do \
		if [ ! -f $(CVM_JVMTI_HPROF_FLAGSDIR)/$${filename} ]; then \
			echo "Hprof flag $${filename} changed. Cleaning up."; \
			rm -f $(CVM_JVMTI_HPROF_FLAGSDIR)/$${filename%.*}.*; \
			touch $(CVM_JVMTI_HPROF_FLAGSDIR)/$${filename}; \
			$(CVM_JVMTI_HPROF_CLEANUP_OBJ_ACTION); \
		fi \
	done

$(CVM_JVMTI_HPROF_BUILDDIRS):
	@echo ... mkdir $@
	@if [ ! -d $@ ]; then mkdir -p $@; fi

$(CVM_JVMTI_HPROF_LIBDIR)/$(CVM_JVMTI_HPROF_LIB): $(CVM_JVMTI_HPROF_OBJECTS)
	@echo "Linking $@"
	$(SO_LINK_CMD)
	@echo "Done Linking $@"

ifeq ($(CVM_JVMTI), true)
ifeq ($(CVM_JVMPI), false)
$(CVM_JVMTI_HPROF_LIBDIR)/jvm.hprof.txt:
	@echo "Copying $@"
	@if [ ! -d $@ ]; then cp $(CVM_JVMTI_HPROF_SHAREROOT)/jvm.hprof.txt $@; fi
	@echo "Done Copying $@"
endif
endif

# The following are used to build the .o files needed for $(CVM_JVMTI_HPROF_OBJECTS):

#####################################
# include all of the dependency files
#####################################
files := $(foreach file, $(wildcard $(CVM_JVMTI_HPROF_OBJDIR)/*.d), $(file))
ifneq ($(strip $(files)),)
    include $(files)
endif

$(CVM_JVMTI_HPROF_OBJDIR)/%.o: %.c
	@echo "... $@"
	$(SO_CC_CMD)
	$(GENERATEMAKEFILES_CMD)

$(CVM_JVMTI_HPROF_OBJDIR)/%.o: %.S
	@echo "... $@"
	$(SO_ASM_CMD)
