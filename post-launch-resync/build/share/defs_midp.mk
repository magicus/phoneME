#
# @(#)defs_midp.mk	1.8 06/10/25
# 
# Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

CDC_CUR_DIR		= $(shell pwd)
export CDC_DIR		= $(CDC_CUR_DIR)/../../
export CDC_DIST_DIR	= $(CDC_CUR_DIR)/$(CVM_BUILD_SUBDIR_NAME)
CVM_MIDP_BUILDDIR	= $(CDC_DIST_DIR)/midp

ifeq ($(CVM_INCLUDE_MIDP),true)

# Include target specific makefiles first
-include ../$(TARGET_CPU_FAMILY)/defs_midp.mk
-include ../$(TARGET_OS)/defs_midp.mk

# MDIP requires Foundation.
ifeq ($(J2ME_CLASSLIB), cdc)
J2ME_CLASSLIB		= foundation
endif

#
# Target tools directory for compiling both PCSL and MIDP.
# GNU_TOOLS_DIR is set by target specific defs_midp.mk.
#
ifeq ($(CVM_USE_NATIVE_TOOLS), false)
GNU_TOOLS_DIR		?=$(CVM_TARGET_TOOLS_DIR)/../$(TARGET_CPU_FAMILY)-$(TARGET_DEVICE)-$(TARGET_OS)
export GNU_TOOLS_DIR
GNU_TOOLS_BINDIR	?= $(CVM_TARGET_TOOLS_PREFIX)
endif

#
# PCSL defs
#
TARGET_CPU		?= $(TARGET_CPU_FAMILY)
PCSL_DIR		?= $(CVM_TOP)/../pcsl
ifeq ($(wildcard $(PCSL_DIR)/donuts),)
$(error PCSL_DIR must point to a PCSL directory)
endif
PCSL_OUTPUT_DIR	?= $(CVM_MIDP_BUILDDIR)/pcsl_fb
export PCSL_OUTPUT_DIR
PCSL_TARGET		?= $(TARGET_OS)_$(TARGET_CPU)
PCSL_PLATFORM		?= $(PCSL_TARGET)_gcc
NETWORK_MODULE		?= bsd/generic
PCSL_MAKE_OPTIONS 	?=

#
# MIDP defs
#
export JDK_DIR		= $(JDK_HOME)
TARGET_VM		= cdc_hi
MIDP_DIR		?= $(CVM_TOP)/../midp
ifeq ($(wildcard $(MIDP_DIR)/src),)
$(error MIDP_DIR must point to a MIDP directory)
endif
MIDP_MAKEFILE_DIR 	?= build/linux_fb_gcc
MIDP_OUTPUT_DIR	?= $(CVM_MIDP_BUILDDIR)/midp_fb
export MIDP_OUTPUT_DIR

USE_SSL			?= false
USE_RESTRICTED_CRYPTO	?= false
VERIFY_BUILD_ENV	?= 
#CONFIGURATION_OVERRIDE	?= MEASURE_STARTUP=true 
USE_QT_FB		?= false
USE_DIRECTFB		?= false
# The MIDP makefiles should be fixed to not require CLDC_DIST_DIR for CDC build.
export CLDC_DIST_DIR	= $(CDC_DIST_DIR)
USE_CONFIGURATOR	?= true
ifeq ($(CVM_TERSEOUTPUT), false)
USE_VERBOSE_MAKE	?= true
endif

ifeq ($(CVM_DEBUG), true)
USE_DEBUG		= true
endif

MIDP_CLASSESZIP_DEPS	=
MIDP_CLASSESZIP		?= $(MIDP_OUTPUT_DIR)/classes.zip

RUNMIDLET		?= $(MIDP_OUTPUT_DIR)/bin/$(TARGET_CPU)/runMidlet$(DEBUG_POSTFIX)
MIDP_OBJECTS		?= $(MIDP_OUTPUT_DIR)/obj$(DEBUG_POSTFIX)/$(TARGET_CPU)/*.o
ifeq ($(CVM_PRELOAD_LIB), true)
CVM_OBJECTS		+= $(MIDP_OBJECTS)
MIDP_LIBS 		?= \
        -L$(PCSL_OUTPUT_DIR)/$(PCSL_TARGET)/lib -lpcsl_file \
        -lpcsl_memory -lpcsl_network -lpcsl_print -lpcsl_string
LINKLIBS 		+= $(MIDP_LIBS)
endif

# Add MIDP classes to JCC input list so they can be romized.
ifeq ($(CVM_PRELOAD_LIB), true)
CVM_JCC_INPUT		+= $(MIDP_CLASSESZIP)

CVM_CNI_CLASSES += \
	com.sun.cdc.i18n.j2me.Conv \
	com.sun.midp.appmanager.MIDletSuiteInfo \
	com.sun.midp.chameleon.input.InputModeFactory \
	com.sun.midp.chameleon.input.NativeInputMode \
	com.sun.midp.chameleon.layers.SoftButtonLayer \
	com.sun.midp.chameleon.skins.resources.LoadedSkinData \
	com.sun.midp.chameleon.skins.resources.LoadedSkinProperties \
	com.sun.midp.chameleon.skins.resources.LoadedSkinResources \
	com.sun.midp.chameleon.skins.resources.SkinResources \
	com.sun.midp.crypto.MD2 \
	com.sun.midp.crypto.MD5 \
	com.sun.midp.crypto.SHA \
	com.sun.midp.events.EventQueue \
	com.sun.midp.events.NativeEventMonitor \
	com.sun.midp.installer.JarReader \
	com.sun.midp.installer.OtaNotifier \
	com.sun.midp.io.j2me.push.PushRegistryImpl \
	com.sun.midp.io.j2me.storage.File \
	com.sun.midp.io.j2me.storage.RandomAccessStream \
	com.sun.midp.l10n.LocalizedStringsBase \
	com.sun.midp.lcdui.DisplayDeviceAccess \
	com.sun.midp.log.Logging \
	com.sun.midp.log.LoggingBase \
	com.sun.midp.main.CDCInit \
	com.sun.midp.main.CommandState \
	com.sun.midp.main.Configuration \
	com.sun.midp.main.MIDletAppImageGenerator \
	com.sun.midp.main.MIDletProxyList \
	com.sun.midp.main.MIDletSuiteLoader \
	com.sun.midp.main.MIDletSuiteUtils \
	com.sun.midp.main.MIDletSuiteVerifier \
	com.sun.midp.midlet.MIDletPeer \
	com.sun.midp.midletsuite.InstallInfo \
	com.sun.midp.midletsuite.MIDletSuiteImpl \
	com.sun.midp.midletsuite.MIDletSuiteInfo \
	com.sun.midp.midletsuite.MIDletSuiteStorage \
	com.sun.midp.midletsuite.SuiteProperties \
	com.sun.midp.midletsuite.SuiteSettings \
	com.sun.midp.pause.PauseSystem \
	'com.sun.midp.pause.PauseSystem$$MIDPSystem' \
	com.sun.midp.rms.RecordStoreFactory \
	com.sun.midp.rms.RecordStoreFile \
	com.sun.midp.rms.RecordStoreUtil \
	javax.microedition.lcdui.Display \
	javax.microedition.lcdui.Font \
	javax.microedition.lcdui.game.GameCanvas \
	javax.microedition.lcdui.Graphics \
	javax.microedition.lcdui.Image \
	javax.microedition.lcdui.ImageDataFactory \
	javax.microedition.lcdui.KeyConverter
endif

endif

