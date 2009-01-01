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
#  @(#)rules_personal_pocketpc.mk	1.5 06/10/10
#
#  Makefile rules specific to the Win32 Personal Profile for PocketPC.
#

$(CVM_OBJDIR)/%.o: $(WCECOMPAT_LIB_SRC_DIR)/%.c
	$(AT)echo "...Compiling wcecompat objs: $@"
	$(WCECOMPAT_LIB_CC_RULE)	

$(CVM_OBJDIR)/%.RES: $(WCECOMPAT_LIB_SRC_DIR)/%.rc
	$(AT)echo "...Compiling wcecompat resource file: $@ "
	$(WCECOMPAT_LIB_RC_RULE)

ifneq ($(CVM_STATICLINK_LIBS), true) 
$(WCECOMPAT_LIB_PATHNAME) :: $(WCECOMPAT_LIB_CONTENTS)
	@echo "Linking $@"
	$(WCECOMPAT_LIB_LINK_RULE)
endif

ifeq ($(CVM_STATICLINK_LIBS), true)
LINKLIBS += $(WCECOMPAT_LIB_LINKLIBS)
endif

#
# libraries depend on wcecompat
#
$(CLASSLIB_DEPS):: $(WCECOMPAT_LIB_PATHNAME)

# Remove the wcecompat files as part of a make clean
clean::
	rm -rf  $(CVM_BINDIR)/$(LIB_PREFIX)$(WCECOMPAT_LIB_NAME)*

