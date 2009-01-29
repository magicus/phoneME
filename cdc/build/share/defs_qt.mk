#  @(#)defs_qt.mk	1.15 06/10/19
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
# Variables that can be set before including defs_qt.mk:
#
#   QTEMBEDDED             - true is using qte. default is false
#   QT_KEYPAD_MODE         - true if running qte on a keypad. default is false
#   QTOPIA                 - true if using qtopia. default is $(QTEMBEDDED)
#   QT_NEED_THREAD_SUPPORT - true if using qt thread support (qt-mt).
#                            default is !$(QTEMBEDDED)
#   QT_VERSION             - 2 is using qt2.3.2 (default for QTEMBEDDED=true)
#                            3 is using qt3.3.1 (default for QTEMBEDDED=false)
#

-include $(CDC_OS_COMPONENT_DIR)/build/$(TARGET_OS)/defs_qt.mk
-include $(CDC_DEVICE_COMPONENT_DIR)/build/$(TARGET_OS)-$(TARGET_CPU_FAMILY)-$(TARGET_DEVICE)/defs_qt.mk

QTEMBEDDED	?= false
QTOPIA		?= $(QTEMBEDDED)
ifeq ($(QTEMBEDDED), true)
QT_NEED_THREAD_SUPPORT  ?= false
QT_VERSION              ?= 2
else
QT_NEED_THREAD_SUPPORT  ?= true
QT_VERSION              ?= 3
endif

# Can't include thread support when embedded
ifeq ($(QT_NEED_THREAD_SUPPORT), true)
ifeq ($(QTEMBEDDED), true)
$(error Cannot specify QT_NEED_THREAD_SUPPORT=true and QTEMBEDDED=true.)
endif
endif

# Can't have qtopia without being embedded
ifeq ($(QTOPIA), true)
ifeq ($(QTEMBEDDED), false)
$(error Cannot specify QTOPIA=true and QTEMBEDDED=false.)
endif
endif

#
# Specify the location of the qt moc tool and the qt target dir. Don't
# override anything that the device defs_qt.mk setup.
#

ifneq ($(QT_TARGET_DIR),)
ifeq ($(wildcard $(QT_TARGET_DIR)),)
$(error QT_TARGET_DIR does not exist: $(QT_TARGET_DIR))
endif
QT_TARGET_LIB_DIR	= $(QT_TARGET_DIR)/lib
QT_TARGET_INCLUDE_DIR	= $(QT_TARGET_DIR)/include
endif


ifneq ($(CVM_USE_NATIVE_TOOLS), true)
    # this is a cross compilation

    # We usually need X11 libraries for QT. If not, this will be ignored
    X11_LIB_DIR   ?= $(CVM_TOOLS_DIR)/lib/$(CVM_TARGET)/usr/X11R6/lib

    # find qt lib and include directories
    QT_TARGET_LIB_DIR ?= \
	$(CVM_TOOLS_DIR)/lib/$(CVM_TARGET)/usr/lib/qt$(QT_VERSION)/lib
    QT_TARGET_INCLUDE_DIR ?= \
	$(CVM_TOOLS_DIR)/lib/$(CVM_TARGET)/usr/lib/qt$(QT_VERSION)/include

    # find moc tool
    ifeq ($(MOC),)
        # MOC isn't set, so look in default location
        MOC = $(CVM_TOOLS_DIR)/$(CVM_HOST)/qt/bin/moc
        ifeq ($(wildcard $(MOC)),)
            # moc not in default location, so use whatever is on the path
            MOC = moc
        endif
    endif
else  # CVM_USE_NATIVE_TOOLS
    # This is a native compilation

    # We usually need X11 libraries for QT. If not, this will be ignored
    X11_LIB_DIR	?= /usr/X11R6/lib

    # figure out which version of QT we want
    ifeq ($(QTEMBEDDED), true)
        QT_SUBDIR_NAME ?= qte
    else
        QT_SUBDIR_NAME ?= qt$(QT_VERSION)
    endif

    # For native builds, pick up moc from QT_TARGET_DIR if set
    ifneq ($(QT_TARGET_DIR),)
        MOC			?= $(QT_TARGET_DIR)/bin/moc
    endif

    # For historical purposes, check for commerical version installation
    ifneq ($(wildcard /usr/lib/$(QT_SUBDIR_NAME)-commercial/lib),)
	QT_TARGET_LIB_DIR     ?= /usr/lib/$(QT_SUBDIR_NAME)-commercial/lib
	QT_TARGET_INCLUDE_DIR ?= /usr/lib/$(QT_SUBDIR_NAME)-commercial/include
        MOC		      ?= /usr/lib/$(QT_SUBDIR_NAME)-commercial/bin/moc
    endif

    # Check for typical /usr/lib/<qt> installation
    ifneq ($(wildcard /usr/lib/$(QT_SUBDIR_NAME)/lib),)
	QT_TARGET_LIB_DIR	?= /usr/lib/$(QT_SUBDIR_NAME)/lib
	QT_TARGET_INCLUDE_DIR	?= /usr/lib/$(QT_SUBDIR_NAME)/include
        MOC			?= /usr/lib/$(QT_SUBDIR_NAME)/bin/moc
    endif

    # Check in /usr/share/<qt>. This is where Ubuntu installs
    ifneq ($(wildcard /usr/share/$(QT_SUBDIR_NAME)/lib),)
	QT_TARGET_LIB_DIR	?= /usr/share/$(QT_SUBDIR_NAME)/lib
	QT_TARGET_INCLUDE_DIR	?= /usr/include/$(QT_SUBDIR_NAME)
        MOC			?= /usr/share/$(QT_SUBDIR_NAME)/bin/moc
    endif

    # Check in QTDIR if set
    ifneq ($(QTDIR),)
    ifneq ($(wildcard $(QTDIR)/lib),)
	QT_TARGET_LIB_DIR	?= $(QTDIR)/lib
	QT_TARGET_INCLUDE_DIR	?= $(QTDIR)/include
	MOC			?= $(QTDIR)/bin/moc
    endif
    endif

    # As a last resort, check in /usr/lib and /usr/include
    QT_TARGET_LIB_DIR		?= /usr/lib
    QT_TARGET_INCLUDE_DIR	?= /usr/include
    MOC				?= moc

endif  # CVM_USE_NATIVE_TOOLS

ifeq ($(wildcard $(QT_TARGET_LIB_DIR)),)
$(error QT_TARGET_LIB_DIR does not exist: $(QT_TARGET_LIB_DIR))
endif
ifeq ($(wildcard $(QT_TARGET_INCLUDE_DIR)),)
$(error QT_TARGET_INCLUDE_DIR does not exist: $(QT_TARGET_INCLUDE_DIR))
endif

#
# Get the CPP includes needed for qt. Note that some distributions
# put some qt files in /usr/include and the rest in /usr/include/qt.
#
PROFILE_INCLUDE_DIRS += \
	$(QT_TARGET_INCLUDE_DIR) \
	$(QT_TARGET_INCLUDE_DIR)/qt

# Specify the appropriate QT_LIBRARY
ifeq ($(QTEMBEDDED), true)
QT_LIBRARY	?= qte
CVM_DEFINES	+= -DQWS
else
ifeq ($(QT_NEED_THREAD_SUPPORT), true)
QT_LIBRARY	?= qt-mt
CVM_DEFINES += -DQT_THREAD_SUPPORT
else
QT_LIBRARY	?= qt
endif
endif

#
# If it is not QTEMBEDDED, treat it as a UNIX build and set the 
# X11 libs based on the QT version used.
#
ifneq ($(QTEMBEDDED), true)
# personal and basis needs to have access to the X11 headers.
  PROFILE_INCLUDE_DIRS += \
    $(X11_LIB_DIR)/../include \

  ifeq ($(QT_VERSION), 3)
    X11_LIBS ?= -L$(X11_LIB_DIR) -lXext -lX11 -lSM -lICE -lXft -lXrender -lXrandr -lXinerama
  else
    X11_LIBS ?= -L$(X11_LIB_DIR) -lXext -lX11 -lSM -lICE -lXft -lXrender 
  endif # QT_VERSION

  # figure out, how to link with qt
  ifeq ($(wildcard $(QT_TARGET_LIB_DIR)/lib$(QT_LIBRARY).a), $(QT_TARGET_LIB_DIR)/lib$(QT_LIBRARY).a)
    QT_STATIC_LINK ?= true
  else
    QT_STATIC_LINK ?= false
  endif 

  QT_LIB_LIBS ?= -lstdc++ $(X11_LIBS)
  # Unset QT_LIB_LIBS to nothing if we are going to dynamically link or
  # CVM_PRELOAD_LIB is false.
  ifneq ($(QT_STATIC_LINK), true)
  ifneq ($(CVM_PRELOAD_LIB), true)
    QT_LIB_LIBS = 
  endif # CVM_PRELOAD_LIB
  endif # QT_STATIC_LINK
endif # QTEMBEDDED

# Add all QT related libraries to AWT_LIB_LIBS
AWT_LIB_LIBS ?= $(QT_LIB_LIBS) -L$(QT_TARGET_LIB_DIR) -l$(QT_LIBRARY) -lstdc++

# stuff related to running on Qtopia
ifeq ($(QTOPIA), true)
CVM_DEFINES += -DQTOPIA
AWT_LIB_LIBS += -lqpe
endif

# running qte on a keypad
ifeq ($(QT_KEYPAD_MODE), true)
CVM_DEFINES += -DQT_KEYPAD_MODE
endif
