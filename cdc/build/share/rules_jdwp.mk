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
# @(#)rules_jdwp.mk	1.23 06/10/24
#

###############################################################################
# Make rules:

tools:: jdwp
tool-clean: jdwp-clean

jdwp-clean:
	$(CVM_JDWP_CLEANUP_ACTION)

ifeq ($(CVM_JVMTI), true)
    jdwp_build_list = jdwp_initbuild \
	    $(CVM_JDWP_BUILD_TOP)/JDWPCommands.h \
	    $(CVM_JDWP_LIB) \
	    jdwp-dt
else
    jdwp_build_list =
endif

jdwp: $(jdwp_build_list)

jdwp : ALL_INCLUDE_FLAGS := \
	$(ALL_INCLUDE_FLAGS) $(call makeIncludeFlags,$(CVM_JDWP_INCLUDE_DIRS))
jdwp : CVM_DEFINES += $(CVM_JDWP_DEFINES)

jdwp_initbuild: jdwp_check_cvm jdwp_checkflags $(CVM_JDWP_BUILDDIRS)

# Make sure that CVM is built before building jdwp.  If not, the issue a
# warning and abort.
jdwp_check_cvm:
ifneq ($(CVM_STATICLINK_TOOLS), true)
	@if [ ! -f $(CVM_BINDIR)/$(CVM) ]; then \
	    echo "Warning! Need to build CVM with before building JDWP."; \
	    exit 1; \
	else \
	    echo; echo "Building JDWP tool ..."; \
	fi
else
	echo; echo "Building JDWP tool ...";
endif

# Make sure all of the build flags files are up to date. If not, then do
# the requested cleanup action.
jdwp_checkflags: $(CVM_JDWP_FLAGSDIR)
	@for filename in $(CVM_JDWP_FLAGS0); do \
		if [ ! -f $(CVM_JDWP_FLAGSDIR)/$${filename} ]; then \
			echo "JDWP flag $${filename} changed. Cleaning up."; \
			rm -f $(CVM_JDWP_FLAGSDIR)/$${filename%.*}.*; \
			touch $(CVM_JDWP_FLAGSDIR)/$${filename}; \
			$(CVM_JDWP_CLEANUP_OBJ_ACTION); \
		fi \
	done

vpath %.c      $(CVM_JDWP_SRCDIRS)
vpath %.S      $(CVM_JDWP_SRCDIRS)

$(CVM_JDWP_BUILDDIRS):
	@echo ... mkdir $@
	@if [ ! -d $@ ]; then mkdir -p $@; fi

$(CVM_JDWP_LIB): $(CVM_JDWP_OBJECTS)
	@echo "Linking $@"
ifeq ($(CVM_STATICLINK_TOOLS), true)
	$(call STATIC_LIB_LINK_CMD, $(CVM_JDWP_LINKLIBS))
else
	$(call SO_LINK_CMD, $^, $(CVM_JDWP_LINKLIBS))
endif
	@echo "Done Linking $@"

# The following are used to build the .o files needed for $(CVM_JDWP_OBJECTS):

#####################################
# include all of the dependency files
#####################################
files := $(foreach file, $(wildcard $(CVM_JDWP_OBJDIR)/*.d), $(file))
ifneq ($(strip $(files)),)
    include $(files)
endif

# jdwpgen

JDWPGENPKGDIR = com/sun/tools/jdwpgen
JDWPGENDIR = $(CVM_JDWP_SHAREROOT)/classes/$(JDWPGENPKGDIR)
JDWP_SPEC = $(JDWPGENDIR)/jdwp.spec
JDWPGEN = com.sun.tools.jdwpgen
JDWPGEN_CLASS = $(CVM_JDWP_CLASSES)/$(JDWPGENPKGDIR)/Main.class

$(JDWPGEN_CLASS) : $(CVM_JDWP_SHAREROOT)/classes/$(JDWPGENPKGDIR)/Main.java
	$(CVM_JAVAC) -d $(CVM_JDWP_CLASSES) \
		-sourcepath $(CVM_JDWP_SHAREROOT)/classes \
		$<

$(CVM_JDWP_BUILD_TOP)/JDWPCommands.h : $(JDWPGEN_CLASS)
	$(CVM_JAVA) -Xbootclasspath/p:$(CVM_JDWP_CLASSES) \
		$(JDWPGEN).Main $(JDWP_SPEC) \
	    -include $@

$(CVM_JDWP_OBJDIR)/%.o: %.c
	@echo "... $@"
	$(SO_CC_CMD)
	$(GENERATEMAKEFILES_CMD)

$(CVM_JDWP_OBJDIR)/%.o: %.S
	@echo "... $@"
	$(SO_ASM_CMD)
