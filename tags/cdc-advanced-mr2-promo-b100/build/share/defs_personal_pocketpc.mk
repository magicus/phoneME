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
# @(#)defs_personal_pocketpc.mk	1.7 06/10/10
#
#  Makefile definitions specific to the Personal Profile for PocketPC.
#

AWT_PEERSET_SRC_DIR = $(CVM_SHAREROOT)/personal/native/awt/$(AWT_PEERSET)

PROFILE_SRCDIRS_NATIVE += AWT_PEERSET_SRC_DIR

TOOLKIT_CLASS = sun.awt.pocketpc.PPCToolkit
GRAPHICS_ENV_CLASS = sun.awt.pocketpc.PPCGraphicsEnvironment

AWT_LIB_LIBS += aygshell.lib 

CLASSLIB_CLASSES += \
	sun.awt.pocketpc.PPCObjectPeer \
	sun.awt.pocketpc.PPCCheckboxPeer \
	sun.awt.pocketpc.PPCCheckboxMenuItemPeer \
	sun.awt.pocketpc.PPCChoicePeer \
	sun.awt.pocketpc.PPCClipboard \
	sun.awt.pocketpc.PPCColor \
	sun.awt.pocketpc.PPCComponentPeer \
	sun.awt.pocketpc.PPCCanvasPeer \
	sun.awt.pocketpc.PPCWindowPeer \
	sun.awt.pocketpc.PPCFramePeer \
	sun.awt.pocketpc.PPCGraphics \
	sun.awt.pocketpc.PPCDrawableImage \
	sun.awt.pocketpc.PPCImage \
	sun.awt.pocketpc.PPCImageRepresentation \
	sun.awt.pocketpc.PPCButtonPeer \
	sun.awt.pocketpc.PPCLabelPeer \
	sun.awt.pocketpc.PPCSubImage \
	sun.awt.pocketpc.PPCDefaultFontCharset \
	sun.awt.pocketpc.PPCDialogPeer \
	sun.awt.pocketpc.PPCFontMetrics \
	sun.awt.pocketpc.PPCFontPeer \
	sun.awt.pocketpc.PPCListPeer \
	sun.awt.pocketpc.PPCMenuBarPeer \
	sun.awt.pocketpc.PPCMenuContainer \
	sun.awt.pocketpc.PPCMenuItemPeer \
	sun.awt.pocketpc.PPCMenuPeer \
	sun.awt.pocketpc.PPCPopupMenuPeer \
	sun.awt.pocketpc.PPCScrollbarPeer \
	sun.awt.pocketpc.PPCScrollPanePeer \
	sun.awt.pocketpc.PPCTextAreaPeer \
	sun.awt.pocketpc.PPCTextFieldPeer \
	sun.awt.pocketpc.PPCTextComponentPeer \
	sun.awt.pocketpc.PPCToolkit \
	sun.awt.pocketpc.PPCGraphicsConfiguration \
	sun.awt.pocketpc.PPCGraphicsDevice \
	sun.awt.pocketpc.PPCGraphicsEnvironment \
	sun.awt.pocketpc.PPCRobotHelper \
	sun.awt.pocketpc.ShutdownHook \
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
    sun.awt.NullGraphics \
    sun.awt.pocketpc.PPCFileDialogPeer
#	sun.awt.pocketpc.PPCPanelPeer \



AWT_LIB_OBJS += \
	awt.o \
	CmdIDList.o \
	Hashtable.o \
	PPCImageRepresentation.o \
	ObjectList.o \
	OffScreenImageSource.o \
	PPCBrush.o \
	PPCButtonPeer.o \
	PPCCanvasPeer.o \
	PPCChoicePeer.o \
	PPCClipboard.o \
	PPCColor.o \
	PPCComponentPeer.o \
	PPCDialogPeer.o \
	PPCFontPeer.o \
	PPCFramePeer.o \
	PPCGraphics.o \
	PPCImageRepresentation.o \
	PPCLabelPeer.o \
	PPCListPeer.o \
	PPCObjectPeer.o \
	PPCPen.o \
	PPCScrollPanePeer.o \
	PPCScrollbarPeer.o \
	PPCTextAreaPeer.o \
	PPCTextComponentPeer.o \
	PPCTextFieldPeer.o \
	PPCToolkit.o \
	PPCUnicode.o \
	PPCWindowPeer.o \
	PPCCheckboxPeer.o \
	ThreadLocking.o \
	PPCMenuItemPeer.o \
	PPCPopupMenuPeer.o \
	PPCMenuPeer.o \
	PPCMenuBarPeer.o \
	PPCRobotHelper.o \
	PPCKeyboardFocusManager.o \
        PPCFileDialogPeer.o

# Define restrictions for this AWT implementation.

AlphaComposite.SRC_OVER.isRestricted := true

# Cannot resize or iconify frame in Pocket PC, so restrict it appropriately
Frame.setState.isRestricted := true

# Cannot change the cursor in Pocket PC, so restrict it appropriately
Component.setCursor.isRestricted := true

##### Resource files to be linked into the executable
RC_FLAGS = 
RC_RULE  = $(AT)$(RC) $(RC_FLAGS) /Fo$@ $<
AWT_LIB_OBJECTS += $(CVM_OBJDIR)/awt.RES

