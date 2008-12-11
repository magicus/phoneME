#  @(#)defs_personal_qt.mk	1.25 06/10/16
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

# setup qt tools, includes, and libs
include $(CDC_DIR)/build/share/defs_qt.mk

#
# QtEmbedded support if needed
#
ifeq ($(QTEMBEDDED), true)
Frame.setState.isRestricted := true
Component.setCursor.isRestricted := true
endif

#
# Directory that moc files are generated into.
#
AWT_PEERSET_NAME    = $(AWT_PEERSET)
MOC_OUTPUT_DIR		= $(CVM_DERIVEDROOT)/personal/$(AWT_PEERSET_NAME)/moc
CVM_BUILDDIRS		+= $(MOC_OUTPUT_DIR)
CVM_SRCDIRS		+= $(MOC_OUTPUT_DIR)

#
# These are exported as system properties by defs_basis.mk
#
TOOLKIT_CLASS = sun.awt.qt.QtToolkit
GRAPHICS_ENV_CLASS = sun.awt.qt.QtGraphicsEnvironment


CLASSLIB_CLASSES += \
	sun.awt.qt.QtButtonPeer \
	sun.awt.qt.QtCanvasPeer \
	sun.awt.qt.QtChoicePeer \
	sun.awt.qt.QtCheckboxMenuItemPeer \
	sun.awt.qt.QtCheckboxPeer \
	sun.awt.qt.QtClipboard \
	sun.awt.qt.QtComponentPeer \
	sun.awt.qt.QtDialogPeer \
	sun.awt.qt.QtDrawableImage \
	sun.awt.qt.QtGraphics \
	sun.awt.qt.QtGraphicsConfiguration \
	sun.awt.qt.QtGraphicsDevice \
	sun.awt.qt.QtGraphicsEnvironment \
	sun.awt.qt.QtImage \
	sun.awt.qt.QtImageRepresentation \
	sun.awt.qt.QtSubImage \
	sun.awt.qt.QtFileDialogPeer \
	sun.awt.qt.QtFontPeer \
	sun.awt.qt.QtFramePeer \
	sun.awt.qt.QtLabelPeer \
	sun.awt.qt.QtListPeer \
	sun.awt.qt.QtMenuBarPeer \
	sun.awt.qt.QtMenuContainer \
	sun.awt.qt.QtMenuItemPeer \
	sun.awt.qt.QtMenuPeer \
	sun.awt.qt.QtPanelPeer \
	sun.awt.qt.QtPopupMenuPeer \
	sun.awt.qt.QtRobotHelper \
	sun.awt.qt.QtScrollPanePeer \
	sun.awt.qt.QtScrollbarPeer \
	sun.awt.qt.QtTextAreaPeer \
	sun.awt.qt.QtTextFieldPeer \
	sun.awt.qt.QtTextComponentPeer \
	sun.awt.qt.QtToolkit \
	sun.awt.qt.QtWindowPeer \
    sun.awt.qt.QtImageDecoder \
    sun.awt.qt.QtImageDecoderFactory \
    sun.awt.qt.QtVolatileImage \
	sun.awt.image.ByteArrayImageSource \
        sun.awt.image.FileImageSource \
        sun.awt.image.GifImageDecoder \
        sun.awt.image.Image \
        sun.awt.image.ImageConsumerQueue \
        sun.awt.image.ImageDecoder \
        sun.awt.image.ImageDecoderFactory \
        sun.awt.image.ImageFetchable \
        sun.awt.image.ImageFetcher \
        sun.awt.image.ImageFormatException \
        sun.awt.image.ImageProductionMonitor \
        sun.awt.image.ImageRepresentation \
        sun.awt.image.ImageWatched \
        sun.awt.image.InputStreamImageSource \
        sun.awt.image.JPEGImageDecoder \
        sun.awt.image.OffScreenImageSource \
        sun.awt.image.PNGImageDecoder \
        sun.awt.image.URLImageSource \
        sun.awt.image.XbmImageDecoder \
        sun.awt.NullGraphics \


AWT_LIB_OBJS += \
    \
    QtSync.o \
    QtORB.o \
    \
    QpObject.o \
	QpWidget.o \
	QpWidgetFactory.o \
    QpFrame.o \
    QpMenuBar.o \
    QpPopupMenu.o \
    QpPushButton.o \
    QpComboBox.o \
    QpCheckBox.o \
    QpRadioButton.o \
    QpLabel.o \
    QpListBox.o \
    QpScrollView.o \
    QpScrollBar.o \
    QpLineEdit.o \
    QpFileDialog.o \
    QpFontManager.o \
    QpClipboard.o \
    QpRobot.o \
    \
    QtApplication.o \
    \
	awt.o \
	QtClipboard.o \
	QtClipboard_moc.o \
	QtComponentPeer.o \
	QtToolkit.o \
	QtDisposer.o \
	QtPanelPeer.o \
	QtFramePeer.o \
	QtWindowPeer.o \
	QtGraphics.o \
	KeyCodes.o \
	QtImageRepresentation.o \
	QtImageDecoder.o \
	QtFontPeer.o \
	OffScreenImageSource.o \
	QtKeyboardFocusManager.o \
	QtMenuComponentPeer.o \
	QtMenuBarPeer.o \
	QtMenuItemPeer.o \
	QtMenuItemPeer_moc.o \
	QtMenuPeer.o \
	QtMenuPeer_moc.o \
	QtCheckboxMenuItemPeer.o \
	QtCheckboxMenuItemPeer_moc.o \
	QtPopupMenuPeer.o \
	QtPopupMenuPeer_moc.o \
    \
    \
	QtButtonPeer.o \
	QtButtonPeer_moc.o \
	QtCanvasPeer.o \
	QtChoicePeer.o \
	QtChoicePeer_moc.o \
	QtCheckboxPeer.o \
	QtCheckboxPeer_moc.o \
	QtDialogPeer.o \
	QtLabelPeer.o \
	QtListPeer.o \
	QtListPeer_moc.o \
	QtScrollPanePeer.o \
	QtScrollPanePeer_moc.o \
	QtScrollbarPeer.o \
	QtScrollbarPeer_moc.o \
	QtTextComponentPeer.o \
	QtFileDialogPeer.o \
	QtFileDialogPeer_moc.o \
	Qt$(QT_VERSION)TextFieldPeer.o \
	Qt$(QT_VERSION)TextFieldPeer_moc.o \
	Qt$(QT_VERSION)TextAreaPeer.o \
	Qt$(QT_VERSION)TextAreaPeer_moc.o \
	QtRobotHelper.o \
	QxFileDialog.o \
	QxFileDialog_moc.o \
    QxLineEdit.o \

ifeq ($(QT_VERSION), 2)
AWT_LIB_OBJS += \
    QpMultiLineEdit.o \
    QxMultiLineEdit.o \

else
AWT_LIB_OBJS += \
    QpTextEdit.o \
    QxTextEdit.o \

endif

PROFILE_SRCDIRS_NATIVE += \
    $(CVM_SHAREROOT)/personal/native/awt/$(AWT_PEERSET_NAME)/wproxy \

PROFILE_INCLUDE_DIRS += \
    $(CVM_SHAREROOT)/personal/native/awt/$(AWT_PEERSET_NAME)/wproxy \

# Define restrictions for this AWT implementation.

AlphaComposite.SRC_OVER.isRestricted := true
#fix for bug 6172320
#TextField.setEchoChar.isRestricted := true
