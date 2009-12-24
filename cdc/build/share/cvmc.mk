
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
# @(#)cvmc.mk	1.6 06/10/10
#

#
#  Makefile for building the cvmc tool
#

###############################################################################
# Make definitions:
CVMC			= cvmc
CVM_CVMC_BUILD_TOP	= $(CVM_BUILD_TOP)
CVM_CVMC_OBJDIR		= $(CVM_OBJDIR)
CVM_CVMC_BINDIR		= $(CVM_BINDIR)

CVM_CVMC_SHAREROOT	= $(CVM_SHAREROOT)/tools/cvmc

CVM_CVMC_BUILDDIRS += \
	$(CVM_CVMC_OBJDIR)

# List of object files to build:
CVM_CVMC_SHAREOBJS += \
	cvmc.o

CVM_CVMC_OBJECTS0 = $(CVM_CVMC_SHAREOBJS)
CVM_CVMC_OBJECTS  += $(patsubst %.o,$(CVM_CVMC_OBJDIR)/%.o,$(CVM_CVMC_OBJECTS0))

CVM_CVMC_SRCDIRS = \
	$(CVM_CVMC_SHAREROOT)

vpath %.c	$(CVM_CVMC_SRCDIRS)

CVM_CVMC_CLEANUP_ACTION = \
	rm -rf $(CVM_CVMC_OBJDIR)/cvmc.o

CVM_CVMC_CLEANUP_OBJ_ACTION = \
	rm -rf $(CVM_CVMC_OBJDIR)/cvmc.o

#################################################################################
# Make rules:

tools:: cvmc
tool-clean: cvmc-clean

cvmc-clean:
	$(CVM_CVMC_CLEANUP_ACTION)

ifeq ($(CVM_MTASK), true)
    cvmc_build_list = $(CVM_CVMC_BINDIR)/$(CVMC)
else
    cvmc_build_list =
endif

cvmc: $(cvmc_build_list)

$(CVM_CVMC_BINDIR)/$(CVMC): $(CVM_CVMC_OBJECTS)
	@echo "Linking $@"
	$(call LINK_CMD, $^, $(CVM_CVMC_LINKLIBS))
	@echo "Done Linking $@"

$(CVM_CVMC_OBJDIR)/%.o: %.c
	@echo "...$@"
	$(SO_CC_CMD)

