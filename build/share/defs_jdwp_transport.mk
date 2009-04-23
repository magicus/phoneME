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
# @(#)defs_jdwp_transport.mk	1.8 06/10/10
#

#
#  Makefile for building the jdwp tool
#

-include $(CDC_OS_COMPONENT_DIR)/build/$(TARGET_OS)/defs_jdwp_transport_$(CVM_JDWP_TRANSPORT).mk
-include $(CDC_DIR)/build/share/defs_jdwp_transport_$(CVM_JDWP_TRANSPORT).mk

###############################################################################
# Make definitions:

CVM_JDWP_DT_BUILD_TOP     = $(CVM_BUILD_TOP)/jdwp-dt
CVM_JDWP_DT_OBJDIR        := $(call abs2rel,$(CVM_JDWP_DT_BUILD_TOP)/obj)
CVM_JDWP_DT_LIBDIR        ?= $(CVM_LIBDIR)
CVM_JDWP_DT_BUILDDIRS     = $(CVM_JDWP_DT_OBJDIR)

CVM_JDWP_DT_LIB = \
	$(CVM_JDWP_DT_LIBDIR)/$(LIB_PREFIX)dt_$(CVM_JDWP_TRANSPORT)$(LIB_POSTFIX)


#
# Search path for include files:
#
CVM_JDWP_DT_INCLUDE_DIRS  += \
	$(CVM_JDWP_INCLUDE_DIRS) \
        $(CVM_JDWP_SHAREROOT)/transport/$(CVM_JDWP_TRANSPORT)

ifeq ($(CVM_STATICLINK_TOOLS), true)
CVM_STATIC_TOOL_LIBS += $(CVM_JDWP_DT_LIB)
endif

CVM_JDWP_DT_OBJECTS0 = $(CVM_JDWP_DT_SHAREOBJS) $(CVM_JDWP_DT_TARGETOBJS)
CVM_JDWP_DT_OBJECTS  = $(patsubst %.o,$(CVM_JDWP_DT_OBJDIR)/%.o,$(CVM_JDWP_DT_OBJECTS0))

