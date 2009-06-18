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
# @(#)rules_jvmti_hprof.mk	1.23 06/10/24
#

#
#  Makefile for building the Hprof tool
#
###############################################################################
# Make rules:

tools:: jvmti_hprof
tool-clean: jvmti_hprof-clean

jvmti_hprof-clean:
	$(CVM_JVMTI_HPROF_CLEANUP_ACTION)

jvmti_hprof_build_list =
ifeq ($(CVM_JVMTI), true)
    jvmti_hprof_build_list = jvmti_hprof_initbuild \
                       $(CVM_JVMTI_HPROF_LIB) \
                       $(CVM_LIBDIR)/jvm.hprof.txt
endif

jvmti_hprof : ALL_INCLUDE_FLAGS := \
	$(ALL_INCLUDE_FLAGS) $(call makeIncludeFlags,$(CVM_JVMTI_HPROF_INCLUDES))
jvmti_hprof : CVM_DEFINES += -DSKIP_NPT -DHPROF_LOGGING

jvmti_hprof: $(jvmti_hprof_build_list)

jvmti_hprof_initbuild: jvmti_hprof_check_cvm jvmti_hprof_checkflags $(CVM_JVMTI_HPROF_BUILDDIRS)

# Make sure that CVM is built before building hprof.  If not, the issue a
# warning and abort.
jvmti_hprof_check_cvm:
ifneq ($(CVM_STATICLINK_TOOLS), true)
	@if [ ! -f $(CVM_BINDIR)/$(CVM) ]; then \
	    echo "Warning! Need to build CVM before building hprof."; \
	    exit 1; \
	else \
	    echo; echo "Building hprof tool ..."; \
	fi
else
	    echo; echo "Building hprof tool ...";
endif
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

vpath %.c      $(CVM_JVMTI_HPROF_SRCDIRS)
vpath %.S      $(CVM_JVMTI_HPROF_SRCDIRS)

$(CVM_JVMTI_HPROF_BUILDDIRS):
	@echo ... mkdir $@
	@if [ ! -d $@ ]; then mkdir -p $@; fi

$(CVM_JVMTI_HPROF_LIB): $(CVM_JVMTI_HPROF_OBJECTS)
	@echo "Linking $@"
ifeq ($(CVM_STATICLINK_TOOLS), true)
	$(STATIC_LIB_LINK_CMD)
else
	$(call SO_LINK_CMD, $^, $(CVM_JVMTI_LINKLIBS))
endif
	@echo "Done Linking $@"

ifeq ($(CVM_JVMTI), true)
ifeq ($(CVM_JVMPI), false)
$(CVM_LIBDIR)/jvm.hprof.txt:
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
