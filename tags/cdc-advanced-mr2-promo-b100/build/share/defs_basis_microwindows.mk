#
# @(#)defs_basis_microwindows.mk	1.20 06/10/10
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

# Include target specific makefiles first
-include $(CDC_OS_COMPONENT_DIR)/build/$(TARGET_OS)/defs_basis_$(AWT_IMPLEMENTATION).mk
-include $(CDC_DEVICE_COMPONENT_DIR)/build/$(TARGET_OS)-$(TARGET_CPU_FAMILY)-$(TARGET_DEVICE)/defs_basis_$(AWT_IMPLEMENTATION).mk

ifeq ($(CVM_USE_NATIVE_TOOLS), true)
# checks /usr/local/microwin/src/engine directory first
ifneq              ($(wildcard /usr/local/microwin/src/engine),)
MICROWIN        ?= /usr/local/microwin/src/engine
FREETYPE_CONFIG ?= freetype-config
else
# checks $(CVM_TOOLS_DIR)/lib/$(CVM_TARGET)/usr/microwin directory next
ifneq             ($(wildcard $(CVM_TOOLS_DIR)/lib/$(CVM_TARGET)/usr/microwin),)
MICROWIN        ?= $(CVM_TOOLS_DIR)/lib/$(CVM_TARGET)/usr/microwin
FREETYPE_CONFIG ?= $(MICROWIN)/freetype-config
else
ifdef MICROWIN
FREETYPE_CONFIG ?= freetype-config
else
# we stop the build if both the above two checks fail and MICROWIN is not defined
$(error Microwindows tools not found when CVM_USE_NATIVE_TOOLS=true.)
endif
endif
endif
else
# this is a cross compile
MICROWIN        ?= $(CVM_TOOLS_DIR)/lib/$(CVM_TARGET)/usr/microwin
FREETYPE_CONFIG ?= $(MICROWIN)/freetype-config
endif

PROFILE_INCLUDE_DIRS += $(MICROWIN)
AWT_LIB_LIBS ?= -L$(MICROWIN) -lmwengine `$(FREETYPE_CONFIG) --libs` -L/usr/X11R6/lib -lX11 -lpthread

#
# These are exported as system properties by defs_basis.mk
#
TOOLKIT_CLASS = java.awt.MWToolkit

#
# Fonts
#
CVM_FONTSDIR	  = $(CVM_LIBDIR)/fonts
CVM_FONTS_SRCDIR  = $(CVM_TOP)/src/share/basis/lib/microwindows/fonts
CVM_BUILDDIRS 	  += $(CVM_FONTSDIR)

#
# microwindows native directories
#
PROFILE_SRCDIRS_NATIVE += \
	$(CVM_SHAREROOT)/basis/native/awt/$(AWT_IMPLEMENTATION)
PROFILE_INCLUDE_DIRS  += \
	$(CVM_SHAREROOT)/basis/native/awt/$(AWT_IMPLEMENTATION)

#
# microwindows shared class directories
#
PROFILE_SRCDIRS += \
	$(CVM_SHAREROOT)/basis/classes/awt/$(AWT_IMPLEMENTATION) \

CLASSLIB_CLASSES += \
	java.awt.LightweightDispatcher \
	java.awt.MWGraphics \
	java.awt.MWGraphicsConfiguration \
	java.awt.MWDefaultGraphicsConfiguration \
	java.awt.MWARGBGraphicsConfiguration \
	java.awt.MWGraphicsDevice \
	java.awt.MWGraphicsEnvironment \
	java.awt.MWFontMetrics \
	java.awt.MWImage \
	java.awt.MWOffscreenImage \
	java.awt.MWSubimage \
	java.awt.MWToolkit \
	sun.awt.NullGraphics


AWT_LIB_OBJS += \
	MWFontMetrics.o \
	MWGraphics.o \
	MWGraphicsConfiguration.o \
	MWDefaultGraphicsConfiguration.o \
	MWGraphicsDevice.o \
	MWGraphicsEnv.o \
	MWImage.o \
	MWToolkit.o \
	Window.o \
#	cursors.o

# Define the restrictions for this AWT implementation

Frame.setSize.isRestricted := true
Frame.setResizable.isRestricted := true
Frame.setLocation.isRestricted := true
Frame.setState.isRestricted := true
Frame.setTitle.isRestricted := true

