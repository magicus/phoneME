#
# @(#)defs_personal_peer_based.mk	1.20 06/10/10
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
# Include target awt implementation specific makefiles. This is where
# AWT_PEERSET will be overriden if desired.
#
-include $(CDC_OS_COMPONENT_DIR)/build/$(TARGET_OS)/defs_personal_$(AWT_IMPLEMENTATION).mk
-include $(CDC_DEVICE_COMPONENT_DIR)/build/$(TARGET_OS)-$(TARGET_CPU_FAMILY)-$(TARGET_DEVICE)/defs_personal_$(AWT_IMPLEMENTATION).mk

#
# Default peerset is qt
#
AWT_PEERSET ?= qt

#
# Include target awt peerset specific makefiles.
#
-include $(CDC_OS_COMPONENT_DIR)/build/$(TARGET_OS)/defs_personal_$(AWT_PEERSET).mk
-include $(CDC_OSCPU_COMPONENT_DIR)/build/$(TARGET_OS)-$(TARGET_CPU_FAMILY)/defs_personal_$(AWT_PEERSET).mk
-include $(CDC_DEVICE_COMPONENT_DIR)/build/$(TARGET_OS)-$(TARGET_CPU_FAMILY)-$(TARGET_DEVICE)/defs_personal_$(AWT_PEERSET).mk

#
# Include shared awt peerset specific makefile
#
include $(CDC_DIR)/build/share/defs_personal_$(AWT_PEERSET).mk

#
# Contains the peerset and optionally the version. The peerset specific
# makefile typically sets the PEERSET_NAME
# 
AWT_PEERSET_NAME ?= $(AWT_PEERSET)


# Use the AWT_PEERSET to build the lib name instead of the AWT_IMPLEMENTATION.
AWT_LIB_NAME = $(AWT_PEERSET)awt

#
# All the build flags we need to keep track of in case they are toggled.
#
CVM_FLAGS += \
	AWT_PEERSET \

CVM_DEFINES += -D$(AWT_PEERSET)

#
# Wipe out objects and classes when AWT_PEERSET changes.
#
AWT_PEERSET_CLEANUP_ACTION = \
	$(AWT_IMPLEMENTATION_CLEANUP_ACTION)

#
# Personal Profile shared native directories
#

PROFILE_SRCDIRS_NATIVE += \
	$(CVM_SHAREROOT)/personal/native/awt/$(AWT_PEERSET_NAME)

PROFILE_INCLUDE_DIRS += \
	$(CVM_SHAREROOT)/personal/native/awt/$(AWT_PEERSET_NAME)

AWT_LIB_OBJS += \
	PeerBasedToolkit.o


CLASSLIB_CLASSES += \
	java.awt.LightweightDispatcher \
	sun.awt.EmbeddedFrame \
	sun.awt.PeerBasedToolkit \
	sun.awt.peer.ButtonPeer \
	sun.awt.peer.CanvasPeer \
        sun.awt.peer.CheckboxMenuItemPeer \
        sun.awt.peer.CheckboxPeer \
        sun.awt.peer.ChoicePeer \
	sun.awt.peer.DialogPeer \
        sun.awt.peer.FileDialogPeer \
        sun.awt.peer.LabelPeer \
        sun.awt.peer.ListPeer \
        sun.awt.peer.MenuBarPeer \
        sun.awt.peer.MenuItemPeer \
        sun.awt.peer.MenuPeer \
        sun.awt.peer.PopupMenuPeer \
        sun.awt.peer.ScrollPanePeer \
        sun.awt.peer.ScrollbarPeer \
        sun.awt.peer.TextAreaPeer \
        sun.awt.peer.TextComponentPeer \
        sun.awt.peer.TextFieldPeer \
        sun.awt.peer.ComponentPeer \
        sun.awt.peer.ContainerPeer \
        sun.awt.peer.FontPeer \
        sun.awt.peer.FramePeer \
        sun.awt.peer.LightweightPeer \
        sun.awt.peer.MenuComponentPeer \
        sun.awt.peer.PanelPeer \
        sun.awt.peer.WindowPeer \
        sun.awt.PlatformFont \
