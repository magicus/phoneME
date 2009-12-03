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
# @(#)rules_jcov.mk	1.24 06/10/24
#
#  Makefile for building the Jcov tool
#

###############################################################################
# Make rules:

tools:: jcov
tool-clean: jcov-clean

jcov-clean:
	$(CVM_JCOV_CLEANUP_ACTION)

ifeq ($(CVM_JVMPI), true)
    jcov_build_list = jcov_initbuild $(CVM_JCOV_LIB)
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

$(CVM_JCOV_LIB): $(CVM_JCOV_OBJECTS)
	@echo "Linking $@"
	$(call SO_LINK_CMD, $^, $(CVM_JCOV_LINKLIBS))
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
