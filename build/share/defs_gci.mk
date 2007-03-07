#
# @(#)defs_gci.mk	1.1 06/10/10
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

ifeq ($(CVM_INCLUDE_GCI),true)

GCI_DIR                ?= $(COMPONENTS_DIR)/gci

include $(GCI_DIR)/build/share/defs.mk

GCI_LIB_NAME ?= gci
GCI_LIBDIR ?= $(CVM_LIBDIR)

ifneq ($(CVM_STATICLINK_LIBS), true)
GCI_LIB_PATHNAME  = $(GCI_LIBDIR)/$(LIB_PREFIX)$(GCI_LIB_NAME)$(LIB_POSTFIX)
endif

ifneq ($(CVM_STATICLINK_LIBS), true)
CLASSLIB_DEPS += $(GCI_LIB_PATHNAME)
endif

ifeq ($(CVM_STATICLINK_LIBS), true)
CVM_OBJECTS += $(patsubst %.o,$(CVM_OBJDIR)/%.o,$(GCI_LIB_OBJS))
endif


ifeq ($(CVM_STATICLINK_LIBS), true)
BUILTIN_LIBS += $(GCI_LIB_NAME)
endif


CVM_FLAGS += GCI_IMPLEMENTATION

GCI_IMPLEMENTATION_CLEANUP_ACTION = \
        rm -rf *_classes/* lib/* $(CVM_OBJDIR)
ifeq ($(CVM_PRELOAD_LIB),true)
GCI_IMPLEMENTATION_CLEANUP_ACTION += \
        btclasses*
endif


#
# Finally, modify CVM variables wth GCI items
#
PROFILE_SRCDIRS         += $(GCI_SRCDIRS)
CVM_SRCDIRS             += $(GCI_SRCDIRS_NATIVE)
CVM_INCLUDES            += $(GCI_INCLUDES)
CVM_INCLUDE_DIRS        += $(GCI_INCLUDE_DIRS)

endif
