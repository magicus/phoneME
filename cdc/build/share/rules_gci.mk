#
# @(#)rules_gci.mk	1.1 06/10/10
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

ifeq ($(USE_GCI), true)

# print our configuration
printconfig::
	@echo "GCI_DIR            = $(GCI_DIR)"
	@echo "GCI_LIB_LIBS       = $(GCI_LIB_LIBS)"

#
# Include GCI component rules.
#
include $(GCI_DIR)/build/share/rules.mk

#
# Build gci shared library if not statically linked
#
ifneq ($(CVM_STATICLINK_LIBS), true)
GCI_LIB_OBJECTS += $(patsubst %.o,$(CVM_OBJDIR)/%.o,$(GCI_LIB_OBJS))
$(GCI_LIB_PATHNAME) :: $(GCI_LIB_OBJECTS)
	@echo "Linking $@"
	$(call SO_LINK_CMD, $^, $(GCI_LIB_LIBS))
endif

#
# If we are preloading or statically linking the profile, then we must link
# against the libraries that the profile requires (like X11 or QT libraries).
#
ifeq ($(CVM_STATICLINK_LIBS), true)
LINKCVM_LIBS += $(GCI_LIB_LIBS)
endif

#
# If we are not statically linking the profile, then we must link the profile
# shared libraries with the CVM executable if we are romizing.
#
ifeq ($(CVM_PRELOAD_LIB), true)
ifneq ($(CVM_STATICLINK_LIBS), true)
LINKCVM_LIBS += \
        -L$(CVM_LIBDIR) \
        -l$(GCI_LIB_NAME)$(DEBUG_POSTFIX)
endif
endif

endif # USE_GCI
