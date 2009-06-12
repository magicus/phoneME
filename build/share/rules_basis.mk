#
# @(#)rules_basis.mk	1.33 06/10/10
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
# Basis profile sits on top of Foundationm so we need to include 
# foundation rules.
#
include $(CDC_DIR)/build/share/rules_foundation.mk

# print our configuration
printconfig::
	@echo "AWT_LIB_LIBS       = $(AWT_LIB_LIBS)"
	@echo "AWT_IMPLEMENTATION = $(AWT_IMPLEMENTATION)"

#
# Include AWT implementation rules.
#
include $(AWT_IMPLEMENTATION_DIR)/build/share/rules_$(J2ME_CLASSLIB)_$(AWT_IMPLEMENTATION).mk

# Include profile specific gunit rules, if it exists
ifeq ($(CVM_GUNIT_TESTS), true)
-include $(CDC_DIR)/build/share/rules_$(J2ME_CLASSLIB)_gunit.mk
endif

testclasses:: basis_test_copy_resources
basis_test_copy_resources:
	@cp $(CVM_TOP)/test/share/basis/tests/volatileImage/*.gif $(CVM_TEST_CLASSESDIR)/tests/volatileImage
	@chmod +w $(CVM_TEST_CLASSESDIR)/tests/volatileImage/*.gif

javadoc-basis:
	@echo ""
	@echo "******************************************"
	@echo "javadoc.zip target not supported for basis"
	@echo "******************************************"
	@echo ""

#
# Build awt shared library if not statically linked
#
ifneq ($(CVM_STATICLINK_LIBS), true)
AWT_LIB_OBJECTS += $(patsubst %.o,$(CVM_OBJDIR)/%.o,$(AWT_LIB_OBJS))
$(AWT_LIB_PATHNAME) :: $(AWT_LIB_OBJECTS)
	@echo "Linking $@"
	$(call SO_LINK_CMD, $^, $(AWT_LIB_LIBS))
endif

#
# Build jpeg shared library if not statically linked
#
ifneq ($(CVM_STATICLINK_LIBS), true)
JPEG_LIB_OBJECTS = $(patsubst %.o,$(CVM_OBJDIR)/%.o,$(JPEG_LIB_OBJS))
$(JPEG_LIB_PATHNAME) :: $(JPEG_LIB_OBJECTS)
	@echo "Linking $@"
	$(call SO_LINK_CMD, $^, $(JPEG_LIB_LIBS))
endif

#
# If we are preloading or statically linking the profile, then we must link
# against the libraries that the profile requires (like X11 or QT libraries).
#
ifeq ($(CVM_STATICLINK_LIBS), true)
LINKLIBS += $(AWT_LIB_LIBS) $(JPEG_LIB_LIBS)
endif

#
# If we are not statically linking the profile, then we must link the profile
# shared libraries with the CVM executable if we are romizing.
#
ifeq ($(CVM_PRELOAD_LIB), true)
ifneq ($(CVM_STATICLINK_LIBS), true)
LINKLIBS += \
	-L$(CVM_LIBDIR) \
	-l$(AWT_LIB_NAME)$(DEBUG_POSTFIX) \
	-l$(JPEG_LIB_NAME)$(DEBUG_POSTFIX)
endif
endif
