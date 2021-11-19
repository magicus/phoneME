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
# @(#)rules_jvmti_crw.mk	1.23 06/10/24
#

#
#  Makefile for building the Class Read/Write library
#
###############################################################################
# Make rules:

vpath %.c      $(CVM_CRW_SRCDIRS)
vpath %.S      $(CVM_CRW_SRCDIRS)
vpath %.java   $(CVM_CRW_SHARECLASSESROOT)

tools:: java_crw_demo
tool-clean: java_crw_demo-clean

java_crw_demo-clean:
	$(CVM_CRW_CLEANUP_ACTION)

crw_build_list =

ifeq ($(CVM_JVMTI), true)
    crw_build_list = crw_initbuild \
		$(CVM_CRW_LIB) \
		$(CVM_CRW_JARDIR)/$(CVM_CRW_JAR)
endif

java_crw_demo : ALL_INCLUDE_FLAGS := \
	$(ALL_INCLUDE_FLAGS) $(call makeIncludeFlags,$(CVM_CRW_INCLUDES))

java_crw_demo: $(crw_build_list)

crw_initbuild: crw_check_cvm crw_checkflags $(CVM_CRW_BUILDDIRS)

# Make sure that CVM is built before building crw.  If not, the issue a
# warning and abort.
crw_check_cvm:
ifneq ($(CVM_STATICLINK_TOOLS), true)
	@if [ ! -f $(CVM_BINDIR)/$(CVM) ]; then \
	    echo "Warning! Need to build CVM before building crw."; \
	    exit 1; \
	else \
	    echo; echo "Building crw library ..."; \
	fi
else
	    echo; echo "Building crw library ...";
endif

# Make sure all of the build flags files are up to date. If not, then do
# the requested cleanup action.
crw_checkflags: $(CVM_CRW_FLAGSDIR)
	@for filename in $(CVM_CRW_FLAGS0); do \
		if [ ! -f $(CVM_CRW_FLAGSDIR)/$${filename} ]; then \
			echo "crw flag $${filename} changed. Cleaning up."; \
			rm -f $(CVM_CRW_FLAGSDIR)/$${filename%.*}.*; \
			touch $(CVM_CRW_FLAGSDIR)/$${filename}; \
			$(CVM_CRW_CLEANUP_OBJ_ACTION); \
		fi \
	done

$(CVM_CRW_BUILDDIRS):
	@echo ... mkdir $@
	@if [ ! -d $@ ]; then mkdir -p $@; fi

$(CVM_CRW_LIB): $(CVM_CRW_OBJECTS)
	@echo "Linking $@"
ifeq ($(CVM_STATICLINK_TOOLS), true)
	$(STATIC_LIB_LINK_CMD)
else
	$(call SO_LINK_CMD, $^,)
endif
	@echo "Done Linking $@"

# The following are used to build the .o files needed for $(CVM_CRW_OBJECTS):

#####################################
# include all of the dependency files
#####################################
files := $(foreach file, $(wildcard $(CVM_CRW_OBJDIR)/*.d), $(file))
ifneq ($(strip $(files)),)
    include $(files)
endif

$(CVM_CRW_JARDIR)/$(CVM_CRW_JAR): $(CVM_CRW_TRACKER)
	@echo "... $@"
	$(AT)$(CVM_JAR) cf $@ -C $(CVM_CRW_CLASSES) com/
	$(AT)unzip -l $@ | fgrep .class | awk '{print $$4}' | sed -e 's|/|.|g' | sed -e 's|.class||' > $(CVM_CRW_BUILD_TOP)/MIDPPermittedClasses.txt
	$(AT)$(CVM_JAR) uf $@ -C $(CVM_CRW_BUILD_TOP) MIDPPermittedClasses.txt
	$(AT)-rm -f $(CVM_CRW_BUILD_TOP)/MIDPPermittedClasses.txt

$(CVM_CRW_CLASSES)/%.class: %.java
	@echo "Compiling crw classes..."
	$(AT)$(CVM_JAVAC) -d $(CVM_CRW_CLASSES) \
		-sourcepath $(CVM_CRW_SHARECLASSESROOT) \
		$<

$(CVM_CRW_OBJDIR)/%.o: %.c
	@echo "... $@"
	$(SO_CC_CMD)
	$(GENERATEMAKEFILES_CMD)

$(CVM_CRW_OBJDIR)/%.o: %.S
	@echo "... $@"
	$(SO_ASM_CMD)
