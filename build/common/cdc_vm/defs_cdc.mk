#
#
#
# Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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
######################################################################

# Classes that have CNI methods.
MIDP_CNI_CLASSES += \
        com.sun.cdc.i18n.j2me.Conv \
        com.sun.midp.appmanager.MIDletSuiteInfo \
        com.sun.midp.chameleon.input.InputModeFactory \
        com.sun.midp.chameleon.input.NativeInputMode \
        com.sun.midp.chameleon.input.VirtualKeyboardInputMode \
        com.sun.midp.chameleon.layers.SoftButtonLayer \
        com.sun.midp.chameleon.skins.resources.LoadedSkinData \
        com.sun.midp.chameleon.skins.resources.LoadedSkinProperties \
        com.sun.midp.chameleon.skins.resources.LoadedSkinResources \
        com.sun.midp.chameleon.skins.resources.SkinResourcesImpl \
        com.sun.midp.crypto.MD2 \
        com.sun.midp.crypto.MD5 \
        com.sun.midp.crypto.SHA \
        com.sun.midp.crypto.PRand \
        com.sun.midp.events.EventQueue \
        com.sun.midp.events.NativeEventMonitor \
        com.sun.midp.jarutil.JarReader \
        com.sun.midp.installer.OtaNotifier \
        com.sun.midp.io.NetworkConnectionBase \
        com.sun.midp.io.j2me.push.PushRegistryImpl \
        com.sun.midp.io.j2me.storage.File \
        com.sun.midp.io.j2me.storage.RandomAccessStream \
        com.sun.midp.l10n.LocalizedStringsBase \
        com.sun.midp.lcdui.DisplayDeviceAccess \
        com.sun.midp.lcdui.DisplayDevice \
        com.sun.midp.lcdui.DisplayDeviceContainer \
        com.sun.midp.log.Logging \
        com.sun.midp.log.LoggingBase \
        com.sun.midp.main.CDCInit \
        com.sun.midp.main.Configuration \
        com.sun.midp.main.NativeForegroundState \
        com.sun.midp.midletsuite.InstallInfo \
        com.sun.midp.midletsuite.MIDletSuiteImpl \
        com.sun.midp.midletsuite.MIDletSuiteStorage \
        com.sun.midp.midletsuite.SuiteProperties \
        com.sun.midp.midletsuite.SuiteSettings \
        com.sun.midp.rms.RecordStoreFactory \
        com.sun.midp.rms.RecordStoreFile \
        com.sun.midp.rms.RecordStoreUtil \
        com.sun.midp.rms.RecordStoreSharedDBHeader \
        javax.microedition.lcdui.Display \
        javax.microedition.lcdui.Font \
        javax.microedition.lcdui.game.GameCanvas \
        javax.microedition.lcdui.Graphics \
        javax.microedition.lcdui.ImageData \
        javax.microedition.lcdui.ImageDataFactory \
        javax.microedition.lcdui.KeyConverter \
        javax.microedition.lcdui.SuiteImageCacheImpl \
        com.sun.midp.util.ResourceHandler \
        com.sun.midp.security.Permissions

ifeq ($(USE_PISCES), true)
MIDP_CNI_CLASSES += \
        com.sun.pisces.Configuration \
        com.sun.pisces.GraphicsSurfaceDestination \
        com.sun.pisces.NativeFinalizer \
        com.sun.pisces.RendererNativeFinalizer \
        com.sun.pisces.SurfaceNativeFinalizer \
        com.sun.pisces.PiscesFinalizer \
        com.sun.pisces.Transform6 \
        com.sun.pisces.PiscesRenderer \
        com.sun.pisces.NativeSurface \
        com.sun.pisces.AbstractSurface \
        com.sun.pisces.GraphicsSurface
endif

ifeq ($(CVM_INCLUDE_JUMP), true)
MIDP_CNI_CLASSES += \
        com.sun.midp.jump.JumpInit \
        com.sun.midp.jump.isolate.MIDletContainer
endif

# The MIDP rom.config file
ROMGEN_INCLUDE_PATHS += $(MIDP_DIR)/build/common/config

# Patterns to be included in the binary bundle.
MIDP_BINARY_BUNDLE_PATTERNS += \
	$(MIDP_CLASSES_ZIP) \
	$(MIDP_OUTPUT_DIR)/lib/* \
	$(MIDP_OUTPUT_DIR)/bin/$(TARGET_CPU)/* \
	$(MIDP_OUTPUT_DIR)/appdb/*

MIDP_BINARY_BUNDLE_PATTERNS := \
        $(patsubst $(CDC_DIST_DIR)%,$(BINARY_BUNDLE_DIRNAME)%,$(MIDP_BINARY_BUNDLE_PATTERNS))

BINARY_BUNDLE_PATTERNS += $(MIDP_BINARY_BUNDLE_PATTERNS)


