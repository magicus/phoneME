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
# @(#)rules_jdwp_transport.mk	1.8 06/10/10
#

#
###############################################################################
# Make rules:

tools:: jdwp-dt

ifeq ($(CVM_JVMTI), true)
    jdwp_dt_build_list = $(CVM_JDWP_DT_BUILDDIRS) $(CVM_JDWP_DT_LIB)
else
    jdwp_dt_build_list =
endif

jdwp-dt: $(jdwp_dt_build_list)

jdwp-dt : ALL_INCLUDE_FLAGS := \
	$(ALL_INCLUDE_FLAGS) $(call makeIncludeFlags,$(CVM_JDWP_DT_INCLUDE_DIRS))
jdwp-dt: CVM_DEFINES += $(CVM_JDWP_DEFINES)

$(CVM_JDWP_DT_BUILDDIRS):
	@echo ... mkdir $@
	@if [ ! -d $@ ]; then mkdir -p $@; fi

$(CVM_JDWP_DT_LIB): $(CVM_JDWP_DT_OBJECTS)
	@echo "Linking $@"
ifeq ($(CVM_STATICLINK_TOOLS), true)
	$(call STATIC_LIB_LINK_CMD, $(CVM_JDWP_DT_LINKLIBS))
else
	$(call SO_LINK_CMD, $^, $(CVM_JDWP_DT_LINKLIBS))
endif
	@echo "Done Linking $@"

# The following are used to build the .o files needed for $(CVM_JDWP_OBJECTS):

#####################################
# include all of the dependency files
#####################################
files := $(foreach file, $(wildcard $(CVM_JDWP_DT_OBJDIR)/*.d), $(file))
ifneq ($(strip $(files)),)
    include $(files)
endif

# jdwpgen

$(CVM_JDWP_DT_OBJDIR)/%.o: %.c
	@echo "... $@"
	$(SO_CC_CMD)
	$(GENERATEMAKEFILES_CMD)

$(CVM_JDWP_DT_OBJDIR)/%.o: %.S
	@echo "... $@"
	$(SO_ASM_CMD)
