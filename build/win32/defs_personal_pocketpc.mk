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
# @(#)defs_personal_pocketpc.mk	1.11 06/10/24
#
#  Makefile definitions specific to the Personal Profile for PocketPC.
#

##### wceCompat library

CVM_INCLUDE_DIRS  += $(WCECOMPAT_LIB_SRC_DIR) \
	$(CVM_TOP)/src/win32/javavm/runtime

# by default we don't want PocketPC menu style
POCKETPC_MENUS = true
CVM_FLAGS += POCKETPC_MENUS

# Replace *_SUFFIX with *_POSTFIX? See definition in host_defs.mk
LIB_SUFFIX              = .dll
LIB_LINK_SUFFIX         = .lib

WCECOMPAT_LIB_NAME      = wcecompat
WCECOMPAT_LIB_OBJS      = wceCompat.o
WCECOMPAT_LIB_RESOURCES = wceCompat.RES
WCECOMPAT_LIB_PATHNAME  = $(CVM_BINDIR)/$(LIB_PREFIX)$(WCECOMPAT_LIB_NAME)$(LIB_SUFFIX)
WCECOMPAT_LINK_PATHNAME = $(CVM_BINDIR)/$(LIB_PREFIX)$(WCECOMPAT_LIB_NAME)$(LIB_LINK_SUFFIX)

WCECOMPAT_LIB_LIBS            = 
WCECOMPAT_LIB_SRC_DIR         = $(CVM_TOP)/src/win32/personal/native/compat
WCECOMPAT_LIB_OBJS_FILES      = $(patsubst %.o,$(CVM_OBJDIR)/%.o,$(WCECOMPAT_LIB_OBJS))
WCECOMPAT_LIB_RESOURCES_FILES = $(patsubst %.RES,$(CVM_OBJDIR)/%.RES,$(WCECOMPAT_LIB_RESOURCES))
WCECOMPAT_LIB_CONTENTS        = $(WCECOMPAT_LIB_OBJS_FILES) $(WCECOMPAT_LIB_RESOURCES_FILES)

#
# Add the library to the library dependencies
#
ifeq ($(CVM_STATICLINK_LIBS), true)
CVM_OBJECTS += $(WCECOMPAT_LIB_OBJS_FILES)
BUILTIN_LIBS += $(WCECOMPAT_LIB_NAME)
endif

# wceCompat CC rules
WCECOMPAT_LIB_CC_FLAGS      = -I$(WCECOMPAT_LIB_SRC_DIR) -DINSIDE_WCE_COMPAT -D_WIN32_WCE=300 

ifeq ($(POCKETPC_MENUS), true)
WCECOMPAT_LIB_CC_FLAGS     += -DPOCKETPC_MENUS
endif

ifeq ($(CVM_PRELOAD_LIB), true)
WCECOMPAT_LIB_CC_FLAGS     += -DCVM_PRELOAD_LIB
endif

ifeq ($(CVM_STATICLINK_LIBS), true)
WCECOMPAT_LIB_CC_FLAGS     += -DCVM_STATICLINK_LIBS
endif

WCECOMPAT_LIB_CC_RULE_SPACE = $(AT)$(TARGET_CC) $(CFLAGS_SPACE) $(WCECOMPAT_LIB_CC_FLAGS) /Fo$@ $<
WCECOMPAT_LIB_CC_RULE       = $(WCECOMPAT_LIB_CC_RULE_SPACE)

# wceCompat RC rules for resource files
WCECOMPAT_LIB_RC_FLAGS = 
WCECOMPAT_LIB_RC_RULE  = $(AT)$(RC) $(WCECOMPAT_LIB_RC_FLAGS) /Fo$@ $<

# wceCompat LINK rules
ifeq ($(POCKETPC_MENUS), true)
WCECOMPAT_LIB_LINKLIBS  = aygshell.lib
else
WCECOMPAT_LIB_LINKLIBS  = commctrl.lib
endif

WCECOMPAT_LIB_LINKFLAGS = /map /mapinfo:exports  

ifeq ($(CVM_DEBUG), true)
WCECOMPAT_LIB_LINKFLAGS += /debug
endif

ifneq ($(CVM_STATICLINK_LIBS), true)
WCECOMPAT_LIB_LINKFLAGS += /dll
endif

WCECOMPAT_LIB_LINKFLAGS += $(LINK_ARCH_FLAGS) $(WCECOMPAT_LIB_LINKLIBS)

WCECOMPAT_LIB_LINK_RULE = $(AT)$(TARGET_LD) $(WCECOMPAT_LIB_LINKFLAGS) /out:$@ $^

# link in the library file to the pocketpc awt library
ifneq ($(CVM_STATICLINK_LIBS), true)
AWT_LIB_LIBS += $(WCECOMPAT_LINK_PATHNAME)
JPEG_LIB_LIBS += $(WCECOMPAT_LINK_PATHNAME)
endif

#
# If we are not statically linking the profile, then we must link the profile
# shared libraries with the CVM executable if we are romizing.
#
ifeq ($(CVM_PRELOAD_LIB), true)
ifneq ($(CVM_STATICLINK_LIBS), true)
LINKLIBS += $(CVM_BINDIR)/lib$(AWT_LIB_NAME)$(DEBUG_POSTFIX).lib \
            $(CVM_BINDIR)/lib$(JPEG_LIB_NAME)$(DEBUG_POSTFIX).lib
endif
endif

ifeq ($(CVM_DEBUG), true)
WCECOMPAT_LIB_CC_FLAGS += /Zi
endif

# cvm needs the WinCE compability library for getenv().
#LINKLIBS += $(WCECOMPAT_LINK_PATHNAME)

# This might not be needed, but it's important to remember to clean this up
CVM_DEFAULT_CLEANUP_ACTION += rm -rf $(CVM_BINDIR)/$(LIB_PREFIX)$(WCECOMPAT_LIB_NAME)*
POCKETPC_MENUS_CLEANUP_ACTION = rm -rf \
			$(CVM_OBJDIR)/PPCFramePeer.o \
			$(CVM_OBJDIR)/PPCMenuBarPeer.o \
			$(CVM_OBJDIR)/PPCMenuPeer.o \
			$(CVM_OBJDIR)/PPCMenuItemPeer.o \
			$(CVM_OBJDIR)/PPCPopupMenuPeer.o

