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
# @(#)rules_nb_profiler.mk	1.0 09/04/24
#

#
#  Makefile for building the netbeans profiler agent
#
###############################################################################
# Make rules:

tools:: nb_profiler
tool-clean: nb_profiler-clean

nb_profiler-clean:
	$(CVM_NB_PROFILER_CLEANUP_ACTION)

nb_profiler_build_list =
ifeq ($(CVM_JVMTI), true)
    nb_profiler_build_list = nb_profiler_initbuild \
                       $(CVM_NB_PROFILER_LIB) jfluid_libs
endif

nb_profiler: $(nb_profiler_build_list)

nb_profiler : ALL_INCLUDE_FLAGS := \
	$(ALL_INCLUDE_FLAGS) $(call makeIncludeFlags,$(CVM_NB_PROFILER_INCLUDES))

nb_profiler_initbuild: nb_profiler_check_cvm nb_profiler_checkflags $(CVM_NB_PROFILER_BUILDDIRS)

# Make sure that CVM is built before building hprof.  If not, the issue a
# warning and abort.
nb_profiler_check_cvm:
ifneq ($(CVM_STATICLINK_TOOLS), true)
	@if [ ! -f $(CVM_BINDIR)/$(CVM) ]; then \
	    echo "Warning! Need to build CVM before building nb_profiler agent."; \
	    exit 1; \
	else \
	    echo; echo "Building netbeans profiler agent ..."; \
	fi
else
	    echo; echo "Building netbeans profiler agent ...";
endif
# Make sure all of the build flags files are up to date. If not, then do
# the requested cleanup action.
nb_profiler_checkflags: $(CVM_NB_PROFILER_FLAGSDIR)
	@for filename in $(CVM_NB_PROFILER_FLAGS0); do \
		if [ ! -f $(CVM_NB_PROFILER_FLAGSDIR)/$${filename} ]; then \
			echo "NB profiler flag $${filename} changed. Cleaning up."; \
			rm -f $(CVM_NB_PROFILER_FLAGSDIR)/$${filename%.*}.*; \
			touch $(CVM_NB_PROFILER_FLAGSDIR)/$${filename}; \
			$(CVM_NB_PROFILER_CLEANUP_OBJ_ACTION); \
		fi \
	done

vpath %.c      $(CVM_NB_PROFILER_SRCDIRS)
vpath %.S      $(CVM_NB_PROFILER_SRCDIRS)

$(CVM_NB_PROFILER_BUILDDIRS):
	@echo ... mkdir $@
	@if [ ! -d $@ ]; then mkdir -p $@; fi

$(CVM_NB_PROFILER_LIB): $(CVM_NB_PROFILER_OBJECTS)
	@echo "Linking $@"
ifeq ($(CVM_STATICLINK_TOOLS), true)
	$(STATIC_LIB_LINK_CMD)
else
	$(call SO_LINK_CMD, $^, $(CVM_JVMTI_LINKLIBS))
endif
	@echo "Done Linking $@"

profiler_module:
ifneq ($(STATIC_APP),true)
	@echo copying profiler patched modules
	-$(AT)rm -rf $(CVM_LIBDIR)/profiler/modules
	-$(AT)mkdir -p $(CVM_LIBDIR)/profiler/modules
	$(AT)cp -r $(CVM_NB_PROFILER_SHAREROOT)/modules $(CVM_LIBDIR)/profiler
	-$(AT)find $(CVM_LIBDIR)/profiler/modules -depth -name .svn -exec rm -rf {} \;
endif

jfluid_libs: $(CVM_LIBDIR)/profiler/lib/jfluid-server.jar $(CVM_LIBDIR)/profiler/lib/jfluid-server-cvm.jar

$(CVM_LIBDIR)/profiler/lib/jfluid-%.jar: $(CVM_NB_PROFILER_SHAREROOT)/profiler/lib/jfluid-%.jar
	@echo copying jfluid libraries
	-$(AT)mkdir -p $(CVM_LIBDIR)/profiler/lib
	$(AT)cp $< $(CVM_LIBDIR)/profiler/lib
	-$(AT)find $(CVM_LIBDIR)/profiler -depth -name .svn -exec rm -rf {} \;
	$(AT)unzip -l $@ | fgrep server | awk '{print $$4}' | sed -e 's|/|.|g' | sed -e 's|.class||' > $(CVM_LIBDIR)/profiler/lib/MIDPPermittedClasses.txt
	$(AT)$(CVM_JAR) uf $@ -C $(CVM_LIBDIR)/profiler/lib MIDPPermittedClasses.txt
	$(AT)-rm -f $(CVM_LIBDIR)/profiler/lib/MIDPPermittedClasses.txt

# The following are used to build the .o files needed for $(CVM_NB_PROFILER_OBJECTS):

#####################################
# include all of the dependency files
#####################################
files := $(foreach file, $(wildcard $(CVM_NB_PROFILER_OBJDIR)/*.d), $(file))
ifneq ($(strip $(files)),)
    include $(files)
endif

$(CVM_NB_PROFILER_OBJDIR)/%.o: %.c
	@echo "... $@"
	$(SO_CC_CMD)
	$(GENERATEMAKEFILES_CMD)

$(CVM_NB_PROFILER_OBJDIR)/%.o: %.S
	@echo "... $@"
	$(SO_ASM_CMD)

# post process...
#$ unzip -l jfluid-server-cvm.jar | fgrep server | awk '{print $4}' | sed -e 's|/|.|g' | sed 's|.class||' > MIDPPermittedClasses.txt