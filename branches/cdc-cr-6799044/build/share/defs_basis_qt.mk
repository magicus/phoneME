# @(#)defs_basis_qt.mk	1.13 06/10/16
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


# Include target specific makefiles first
-include $(CDC_OS_COMPONENT_DIR)/build/$(TARGET_OS)/defs_basis_$(AWT_IMPLEMENTATION).mk
-include $(CDC_DEVICE_COMPONENT_DIR)/build/$(TARGET_OS)-$(TARGET_CPU_FAMILY)-$(TARGET_DEVICE)/defs_basis_$(AWT_IMPLEMENTATION).mk

# setup qt tools, includes, and libs
include $(CDC_DIR)/build/share/defs_qt.mk

TOOLKIT_CLASS = java.awt.QtToolkit

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
	java.awt.QtGraphics \
	java.awt.QtGraphicsConfiguration \
	java.awt.QtDefaultGraphicsConfiguration \
	java.awt.QtARGBGraphicsConfiguration \
	java.awt.QtGraphicsDevice \
	java.awt.QtGraphicsEnvironment \
	java.awt.QtFontMetrics \
	java.awt.QtImage \
	java.awt.QtOffscreenImage \
	java.awt.QtRobotHelper \
	java.awt.QtSubimage \
	java.awt.QtToolkit \
	java.awt.QtVolatileImage \
	sun.awt.NullGraphics 


AWT_LIB_OBJS += \
	QtFontMetrics.o \
	QtGraphics.o \
	QtGraphicsConfiguration.o \
	QtGraphicsDevice.o \
	QtGraphicsEnv.o \
	QtImage.o \
	QtRobotHelper.o \
	QtToolkit.o \
	Window.o \
	QtBackEnd.o \
    QtApplication.o \
    QtSync.o \
    QtORB.o \
    QpObject.o \
    QpRobot.o \
    QtScreenFactory.o \

#	QtDefaultGraphicsConfiguration.o \
#	cursors.o

# Define the restrictions for this AWT implementation

Frame.setSize.isRestricted := true
Frame.setResizable.isRestricted := true
Frame.setLocation.isRestricted := true
#fix for bug 6272320
#Frame.setState.isRestricted := true
Frame.setTitle.isRestricted := true
AlphaComposite.SRC_OVER.isRestricted := true


AWT_LIB_LIBS += -lstdc++
ifeq ($(QTEMBEDDED), true)
AWT_LIB_LIBS += -lm
#AWT_LIB_OBJS += QPatchedPixmap.o
endif

ifeq ($(AWT_QT_DEBUG), true)
AWT_LIB_OBJS += QtDebugBackEnd.o
endif
