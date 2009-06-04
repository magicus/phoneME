#
# @(#)defs_gci.mk	1.1 06/10/10
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

#
# Translate Basis AWT definitions to GCI ones
#
ifeq ($(AWT_IMPLEMENTATION), gci)
   USE_GCI = true
endif

ifeq ($(USE_GCI), true)

ifeq ($(J2ME_CLASSLIB), basis)
#
# Define AWT_IMPLEMENTATION to be GCI-based
#
AWT_IMPLEMENTATION ?= gci
ifneq ($(AWT_IMPLEMENTATION), gci)
    $(warning AWT_IMPLEMENTATION should be set to gci when USE_GCI=true. \
              Any other value set will be overridden.) 
    override AWT_IMPLEMENTATION = gci
endif

#
## Allow user to set either GCI_DIR or AWT_IMPLEMENTATION_DIR
#
ifdef GCI_DIR
    AWT_IMPLEMENTATION_DIR = $(GCI_DIR)
else
    ifdef AWT_IMPLEMENTATION_DIR
        GCI_DIR = $(AWT_IMPLEMENTATION_DIR)
    else
        GCI_DIR = $(COMPONENTS_DIR)/gci
        AWT_IMPLEMENTATION_DIR = $(GCI_DIR)
    endif
endif 

endif # J2ME_CLASSLIB

GCI_DIR                ?= $(COMPONENTS_DIR)/gci

include $(GCI_DIR)/build/share/defs.mk

GCI_LIB_NAME ?= gci
GCI_LIBDIR ?= $(CVM_LIBDIR)

ifeq ($(CVM_STATICLINK_LIBS), true)
  CVM_OBJECTS += $(patsubst %.o,$(CVM_OBJDIR)/%.o,$(GCI_LIB_OBJS))
  BUILTIN_LIBS += $(GCI_LIB_NAME)
else
  GCI_LIB_PATHNAME  = $(GCI_LIBDIR)/$(LIB_PREFIX)$(GCI_LIB_NAME)$(LIB_POSTFIX)
  CLASSLIB_DEPS += $(GCI_LIB_PATHNAME)
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
CVM_INCLUDE_DIRS        += $(GCI_INCLUDE_DIRS)

endif # USE_GCI
