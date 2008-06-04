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
# @(#)jvmti_crw.mk	1.23 06/10/24
#

#
#  Makefile for building the Class Read/Write library
#

###############################################################################
# Make definitions:

CVM_CRW_LIB           = $(LIB_PREFIX)java_crw_demo$(LIB_POSTFIX)
CVM_CRW_JAR           = java_crw_demo.jar

CVM_CRW_BUILD_TOP     = $(CVM_BUILD_TOP)/jvmti/crw
CVM_CRW_OBJDIR        := $(call abs2rel,$(CVM_CRW_BUILD_TOP)/obj)
CVM_CRW_FLAGSDIR      = $(CVM_CRW_BUILD_TOP)/flags
CVM_CRW_CLASSES	      = $(CVM_CRW_BUILD_TOP)/classes

CVM_CRW_LIBDIR        ?= $(CVM_LIBDIR)
CVM_CRW_JARDIR        = $(CVM_LIBDIR)

CVM_CRW_SHAREROOT     = $(CVM_SHAREROOT)/tools/jvmti/crw
CVM_CRW_SHARECLASSESROOT = $(CVM_SHAREDCLASSES_SRCDIR)/com/sun/demo/jvmti/hprof
CVM_CRW_TARGETROOT    = $(CVM_TARGETROOT)/tools/jvmti/crw

CVM_CRW_BUILDDIRS += \
        $(CVM_CRW_OBJDIR) \
        $(CVM_CRW_FLAGSDIR) \
	$(CVM_CRW_CLASSES)

#
# Search path for include files:
#
CVM_CRW_INCLUDES  += \
	$(CVM_BUILD_TOP)/generated/javavm/include \
        $(CVM_CRW_SHAREROOT) \
        $(CVM_CRW_TARGETROOT)

java_crw_demo : ALL_INCLUDE_FLAGS := \
	$(ALL_INCLUDE_FLAGS) $(call makeIncludeFlags,$(CVM_CRW_INCLUDES))

#
# List of object files to build:
#
CVM_CRW_SHAREOBJS += \
	java_crw_demo.o

CVM_CRW_OBJECTS0 = $(CVM_CRW_SHAREOBJS) $(CVM_CRW_TARGETOBJS)
CVM_CRW_OBJECTS  = $(patsubst %.o,$(CVM_CRW_OBJDIR)/%.o,$(CVM_CRW_OBJECTS0))

CVM_CRW_TRACKER0 = Tracker.class
CVM_CRW_TRACKER  = $(patsubst %.class,$(CVM_CRW_CLASSES)/%.class,$(CVM_CRW_TRACKER0))

CVM_CRW_SRCDIRS  = \
	$(CVM_CRW_SHAREROOT) \
	$(CVM_CRW_TARGETROOT)

vpath %.c      $(CVM_CRW_SRCDIRS)
vpath %.S      $(CVM_CRW_SRCDIRS)
vpath %.java   $(CVM_CRW_SHARECLASSESROOT)

CVM_CRW_FLAGS += \
        CVM_SYMBOLS \
        CVM_OPTIMIZED \
        CVM_DEBUG \
        CVM_DEBUG_CLASSINFO \
        CVM_JVMTI \
        CVM_DYNAMIC_LINKING \

CVM_CRW_FLAGS0 = $(foreach flag, $(CVM_CRW_FLAGS), $(flag).$($(flag)))

CVM_CRW_CLEANUP_ACTION = \
        rm -rf $(CVM_CRW_BUILD_TOP)

CVM_CRW_CLEANUP_OBJ_ACTION = \
        rm -rf $(CVM_CRW_OBJDIR)

###############################################################################
# Make rules:

tools:: java_crw_demo
tool-clean: java_crw_demo-clean

java_crw_demo-clean:
	$(CVM_CRW_CLEANUP_ACTION)

ifeq ($(CVM_JVMTI), true)
    crw_build_list = crw_initbuild \
		$(CVM_CRW_LIBDIR)/$(CVM_CRW_LIB) \
		$(CVM_CRW_JARDIR)/$(CVM_CRW_JAR)
else
    crw_build_list =
endif

java_crw_demo: $(crw_build_list)

crw_initbuild: crw_check_cvm crw_checkflags $(CVM_CRW_BUILDDIRS)

# Make sure that CVM is built before building crw.  If not, the issue a
# warning and abort.
crw_check_cvm:
	@if [ ! -f $(CVM_BINDIR)/$(CVM) ]; then \
	    echo "Warning! Need to build CVM before building crw."; \
	    exit 1; \
	else \
	    echo; echo "Building crw library ..."; \
	fi

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

$(CVM_CRW_LIBDIR)/$(CVM_CRW_LIB): $(CVM_CRW_OBJECTS)
	@echo "Linking $@"
	$(SO_LINK_CMD)
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
