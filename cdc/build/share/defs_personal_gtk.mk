#
# @(#)defs_personal_gtk.mk	1.19 06/10/10
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
# Specify the location of the gtk-config. Don't
# override anything that the device defs_personal_qt.mk setup.
#
ifeq ($(CVM_USE_NATIVE_TOOLS), true)
GTK_CONFIG	?= gtk-config
else
GTK_CONFIG	?= $(CVM_TOOLS_DIR)/lib/$(CVM_TARGET)/usr/bin/gtk-config
endif

#
# Setup default AWT_LIB_LIBS. Don't override anything that the device 
# defs_personal.mk setup.
#
AWT_LIB_LIBS ?= `$(GTK_CONFIG) --libs` -lgthread

#
# These are exported as system properties by defs_basis.mk
#
TOOLKIT_CLASS	   = sun.awt.gtk.GToolkit
GRAPHICS_ENV_CLASS = sun.awt.gtk.GdkGraphicsEnvironment

#
# Set the GTK default font property if the makefile variable has been set
#
ifdef GTK_DEFAULT_FONT
  SYSTEM_PROPERTIES += j2me.pp.gtk.default.font=$(GTK_DEFAULT_FONT)
endif

#
# Get the gtk include directories.
#
PROFILE_INCLUDE_DIRS += $(patsubst -I%,%,$(shell $(GTK_CONFIG) --cflags))

CLASSLIB_CLASSES += \
	sun.awt.gtk.GButtonPeer \
	sun.awt.gtk.GCanvasPeer \
	sun.awt.gtk.GChoicePeer \
	sun.awt.gtk.GCheckboxMenuItemPeer \
	sun.awt.gtk.GCheckboxPeer \
	sun.awt.gtk.GComponentPeer \
	sun.awt.gtk.GDialogPeer \
	sun.awt.gtk.GdkDrawableImage \
	sun.awt.gtk.GdkGraphics \
	sun.awt.gtk.GdkGraphicsConfiguration \
	sun.awt.gtk.GdkGraphicsDevice \
	sun.awt.gtk.GdkGraphicsEnvironment \
        sun.awt.gtk.GdkImage \
	sun.awt.gtk.GdkImageRepresentation \
	sun.awt.gtk.GdkSubImage \
	sun.awt.gtk.GtkClipboard \
	sun.awt.gtk.GFileDialogPeer \
	sun.awt.gtk.GFontPeer \
	sun.awt.gtk.GFramePeer \
	sun.awt.gtk.GLabelPeer \
	sun.awt.gtk.GListPeer \
	sun.awt.gtk.GMenuBarPeer \
	sun.awt.gtk.GMenuContainer \
	sun.awt.gtk.GMenuItemPeer \
	sun.awt.gtk.GMenuPeer \
	sun.awt.gtk.GPanelPeer \
	sun.awt.gtk.GPlatformFont \
	sun.awt.gtk.GPopupMenuPeer \
	sun.awt.gtk.GScrollPanePeer \
	sun.awt.gtk.GScrollbarPeer \
	sun.awt.gtk.GTextAreaPeer \
	sun.awt.gtk.GTextFieldPeer \
	sun.awt.gtk.GTextComponentPeer \
	sun.awt.gtk.GToolkit \
	sun.awt.gtk.GWindowPeer \
	sun.awt.image.ByteArrayImageSource \
        sun.awt.image.FileImageSource \
        sun.awt.image.GifImageDecoder \
        sun.awt.image.Image \
        sun.awt.image.ImageConsumerQueue \
        sun.awt.image.ImageDecoder \
        sun.awt.image.ImageFetchable \
        sun.awt.image.ImageFetcher \
        sun.awt.image.ImageFormatException \
        sun.awt.image.ImageProductionMonitor \
        sun.awt.image.ImageRepresentation \
        sun.awt.image.ImageWatched \
        sun.awt.image.InputStreamImageSource \
        sun.awt.image.JPEGImageDecoder \
        sun.awt.image.OffScreenImageSource \
        sun.awt.image.PixelStore \
        sun.awt.image.PixelStore32 \
        sun.awt.image.PixelStore8 \
        sun.awt.image.PNGImageDecoder \
        sun.awt.image.URLImageSource \
        sun.awt.image.XbmImageDecoder \
        sun.awt.NullGraphics


AWT_LIB_OBJS += \
	GComponentPeer.o \
	GFramePeer.o \
	GPanelPeer.o \
	GToolkit.o \
	GWindowPeer.o \
	GdkGraphics.o \
	KeyCodes.o \
	ThreadLocking.o \
	GdkImageRepresentation.o \
	GtkClipboard.o \
	GFontPeer.o \
	OffScreenImageSource.o \
	GButtonPeer.o \
	GCanvasPeer.o \
	GChoicePeer.o \
	GCheckboxMenuItemPeer.o \
	GCheckboxPeer.o \
	GDialogPeer.o \
	GFileDialogPeer.o \
	GLabelPeer.o \
	GListPeer.o \
	GMenuPeer.o \
	GMenuBarPeer.o \
	GMenuItemPeer.o \
	GPopupMenuPeer.o \
	GPlatformFont.o \
	GScrollPanePeer.o \
	GScrollbarPeer.o \
	GTextAreaPeer.o \
	GTextFieldPeer.o \
	GTextComponentPeer.o \

# Define restrictions for this AWT implementation.

AlphaComposite.SRC_OVER.isRestricted := true
