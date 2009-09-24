#
# @(#)defs_personal.mk	1.88 06/10/10
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
# Assume we are building a peer based implementation of AWT for
# personal by default. The target makefiles may change this.
#
AWT_IMPLEMENTATION ?= peer_based

#
# Do OS and Device defs_personal.mk first so AWT_IMPLEMENTATION can be changed.
#
-include $(CDC_OS_COMPONENT_DIR)/build/$(TARGET_OS)/defs_personal.mk
-include $(CDC_DEVICE_COMPONENT_DIR)/build/$(TARGET_OS)-$(TARGET_CPU_FAMILY)-$(TARGET_DEVICE)/defs_personal.mk

#
# Personal Profile class directories
#
# Make sure that Personal Profile finds it's classes before
# those of Personal Basis, foundation, and cdc.
#
PROFILE_SRCDIRS += \
	$(CVM_SHAREROOT)/personal/classes/awt/$(AWT_IMPLEMENTATION) \
	$(CVM_SHAREROOT)/personal/classes/common

#
# Personal Profile shared native directories
#
PROFILE_SRCDIRS_NATIVE += \
	$(CVM_SHAREROOT)/personal/native/sun/awt/common \
	$(CVM_TARGETROOT)/personal/native/sun/audio

#
# Shared Native code
#
AWT_LIB_OBJS += \
	audioDevice.o \
	embeddedframe.o \

#
# Security File
#
CVM_POLICY_SRC  ?= $(CVM_TOP)/src/share/personal/lib/security/java.policy

#
# Shared class files, regardless of OS or toolkit.
#
CLASSLIB_CLASSES += \
        java.applet.Applet \
        java.applet.AppletContext \
        java.applet.AppletStub \
        java.applet.AudioClip \
        java.math.BigDecimal \
        sun.misc.BASE64Decoder \
        sun.misc.BASE64Encoder \
        sun.misc.CEFormatException \
        sun.misc.CEStreamExhausted \
        sun.misc.CharacterDecoder \
        sun.misc.CharacterEncoder \
        sun.misc.HexDumpEncoder \
        sun.misc.MessageUtils \
        sun.misc.Queue \
        sun.misc.REException \
        sun.misc.Regexp \
        sun.misc.RegexpPool \
        sun.misc.RegexpTarget \
        com.sun.util.DefaultPTimer \
	com.sun.util.PTimer \
	com.sun.util.PTimerScheduleFailedException \
	com.sun.util.PTimerSpec \
	com.sun.util.PTimerWentOffEvent \
	com.sun.util.PTimerWentOffListener \
        sun.applet.AppletAudioClip \
        sun.applet.AppletClassLoader \
        sun.applet.AppletEvent \
        sun.applet.AppletEventMulticaster \
        sun.applet.AppletIOException \
        sun.applet.AppletIllegalArgumentException \
        sun.applet.AppletImageRef \
        sun.applet.AppletListener \
        sun.applet.AppletMessageHandler \
        sun.applet.AppletObjectInputStream \
        sun.applet.AppletPanel \
        sun.applet.AppletProps \
        sun.applet.AppletResourceLoader \
        sun.applet.AppletSecurity \
        sun.applet.AppletSecurityException \
        sun.applet.AppletThreadGroup \
        sun.applet.AppletViewer \
        sun.applet.AppletViewerFactory \
        sun.applet.AppletViewerPanel \
        sun.applet.Main \
        sun.applet.resources.MsgAppletViewer \
        sun.applet.resources.MsgAppletViewer_ja \
        sun.applet.resources.MsgAppletViewer_zh \
        sun.audio.AudioData \
        sun.audio.AudioDataStream \
        sun.audio.AudioDevice \
        sun.audio.AudioPlayer \
        sun.audio.AudioStream \
        sun.audio.AudioTranslatorStream \
        sun.audio.ContinuousAudioDataStream \
        sun.audio.InvalidAudioFormatException \
        sun.audio.NativeAudioStream \
        sun.net.TelnetInputStream \
        sun.net.TelnetOutputStream \
        sun.net.TelnetProtocolException \
        sun.net.TransferProtocolClient \
        sun.net.ftp.FtpClient \
        sun.net.ftp.FtpInputStream \
        sun.net.ftp.FtpLoginException \
        sun.net.ftp.FtpProtocolException \
        java.awt.Button \
	java.awt.Canvas \
        java.awt.Checkbox \
        java.awt.CheckboxGroup \
        java.awt.CheckboxMenuItem \
        java.awt.Choice \
	java.awt.Dialog \
	java.awt.Event \
        java.awt.FileDialog \
        java.awt.Frame \
        java.awt.Label \
        java.awt.List \
        java.awt.Menu \
        java.awt.MenuBar \
        java.awt.MenuItem \
        java.awt.MenuShortcut \
        java.awt.Panel \
        java.awt.PopupMenu \
        java.awt.ScrollPane \
        java.awt.Scrollbar \
        java.awt.TextArea \
        java.awt.TextComponent \
        java.awt.TextField \
        java.awt.datatransfer.Clipboard \
        java.awt.datatransfer.ClipboardOwner \
        java.awt.datatransfer.DataFlavor \
        java.awt.datatransfer.MimeType \
        java.awt.datatransfer.MimeTypeParameterList \
        java.awt.datatransfer.MimeTypeParseException \
        java.awt.datatransfer.StringSelection \
        java.awt.datatransfer.Transferable \
        java.awt.datatransfer.UnsupportedFlavorException \
        sun.awt.AppContext \
        sun.awt.AWTSecurityManager \
        sun.awt.AWTFinalizeable \
        sun.awt.AWTFinalizer \
        sun.awt.CharsetString \
        sun.awt.DrawingSurface \
        sun.awt.DrawingSurfaceInfo \
        sun.awt.FontDescriptor \
        sun.awt.PhysicalDrawingSurface \
        sun.awt.ScreenUpdater \
        sun.awt.SunToolkit \
        sun.awt.UpdateClient \

#
# Converters, defaults included in basis. The rest will be sorted as we go!
#

ifeq ($(ALL_CONVERTERS), true)
CLASSLIB_CLASSES += \
        sun.io.ByteToCharBig5 \
        sun.io.ByteToCharCp037 \
        sun.io.ByteToCharCp1006 \
        sun.io.ByteToCharCp1025 \
        sun.io.ByteToCharCp1026 \
        sun.io.ByteToCharCp1046 \
        sun.io.ByteToCharCp1097 \
        sun.io.ByteToCharCp1098 \
        sun.io.ByteToCharCp1112 \
        sun.io.ByteToCharCp1122 \
        sun.io.ByteToCharCp1123 \
        sun.io.ByteToCharCp1124 \
        sun.io.ByteToCharCp1140 \
        sun.io.ByteToCharCp1141 \
        sun.io.ByteToCharCp1142 \
        sun.io.ByteToCharCp1143 \
        sun.io.ByteToCharCp1144 \
        sun.io.ByteToCharCp1145 \
        sun.io.ByteToCharCp1146 \
        sun.io.ByteToCharCp1147 \
        sun.io.ByteToCharCp1148 \
        sun.io.ByteToCharCp1149 \
        sun.io.ByteToCharCp1250 \
        sun.io.ByteToCharCp1251 \
        sun.io.ByteToCharCp1253 \
        sun.io.ByteToCharCp1254 \
        sun.io.ByteToCharCp1255 \
        sun.io.ByteToCharCp1256 \
        sun.io.ByteToCharCp1257 \
        sun.io.ByteToCharCp1258 \
        sun.io.ByteToCharCp1381 \
        sun.io.ByteToCharCp1383 \
        sun.io.ByteToCharCp273 \
        sun.io.ByteToCharCp277 \
        sun.io.ByteToCharCp278 \
        sun.io.ByteToCharCp280 \
        sun.io.ByteToCharCp284 \
        sun.io.ByteToCharCp285 \
        sun.io.ByteToCharCp297 \
        sun.io.ByteToCharCp33722 \
        sun.io.ByteToCharCp420 \
        sun.io.ByteToCharCp424 \
        sun.io.ByteToCharCp437 \
        sun.io.ByteToCharCp500 \
        sun.io.ByteToCharCp737 \
        sun.io.ByteToCharCp775 \
        sun.io.ByteToCharCp838 \
        sun.io.ByteToCharCp850 \
        sun.io.ByteToCharCp852 \
        sun.io.ByteToCharCp855 \
        sun.io.ByteToCharCp856 \
        sun.io.ByteToCharCp857 \
        sun.io.ByteToCharCp858 \
        sun.io.ByteToCharCp860 \
        sun.io.ByteToCharCp861 \
        sun.io.ByteToCharCp862 \
        sun.io.ByteToCharCp863 \
        sun.io.ByteToCharCp864 \
        sun.io.ByteToCharCp865 \
        sun.io.ByteToCharCp866 \
        sun.io.ByteToCharCp868 \
        sun.io.ByteToCharCp869 \
        sun.io.ByteToCharCp870 \
        sun.io.ByteToCharCp871 \
        sun.io.ByteToCharCp874 \
        sun.io.ByteToCharCp875 \
        sun.io.ByteToCharCp918 \
        sun.io.ByteToCharCp921 \
        sun.io.ByteToCharCp922 \
        sun.io.ByteToCharCp923 \
        sun.io.ByteToCharCp930 \
        sun.io.ByteToCharCp933 \
        sun.io.ByteToCharCp935 \
        sun.io.ByteToCharCp937 \
        sun.io.ByteToCharCp939 \
        sun.io.ByteToCharCp942 \
        sun.io.ByteToCharCp942C \
        sun.io.ByteToCharCp943 \
        sun.io.ByteToCharCp943C \
        sun.io.ByteToCharCp948 \
        sun.io.ByteToCharCp949 \
        sun.io.ByteToCharCp949C \
        sun.io.ByteToCharCp950 \
        sun.io.ByteToCharCp964 \
        sun.io.ByteToCharCp970 \
        sun.io.ByteToCharDBCS_ASCII \
        sun.io.ByteToCharDBCS_EBCDIC \
        sun.io.ByteToCharDoubleByte \
        sun.io.ByteToCharEUC \
        sun.io.ByteToCharEUC_CN \
        sun.io.ByteToCharEUC_JP \
        sun.io.ByteToCharEUC_KR \
        sun.io.ByteToCharEUC_TW \
        sun.io.ByteToCharGBK \
        sun.io.ByteToCharISO2022 \
        sun.io.ByteToCharISO2022CN \
        sun.io.ByteToCharISO2022JP \
        sun.io.ByteToCharISO2022KR \
        sun.io.ByteToCharISO8859_15_FDIS \
        sun.io.ByteToCharISO8859_2 \
        sun.io.ByteToCharISO8859_3 \
        sun.io.ByteToCharISO8859_4 \
        sun.io.ByteToCharISO8859_5 \
        sun.io.ByteToCharISO8859_6 \
        sun.io.ByteToCharISO8859_7 \
        sun.io.ByteToCharISO8859_8 \
        sun.io.ByteToCharISO8859_9 \
        sun.io.ByteToCharJIS0201 \
        sun.io.ByteToCharJIS0208 \
        sun.io.ByteToCharJIS0212 \
        sun.io.ByteToCharJISAutoDetect \
        sun.io.ByteToCharJohab \
        sun.io.ByteToCharKOI8_R \
        sun.io.ByteToCharMS874 \
        sun.io.ByteToCharMS932 \
        sun.io.ByteToCharMS936 \
        sun.io.ByteToCharMS950 \
        sun.io.ByteToCharMacArabic \
        sun.io.ByteToCharMacCentralEurope \
        sun.io.ByteToCharMacCroatian \
        sun.io.ByteToCharMacCyrillic \
        sun.io.ByteToCharMacDingbat \
        sun.io.ByteToCharMacGreek \
        sun.io.ByteToCharMacHebrew \
        sun.io.ByteToCharMacIceland \
        sun.io.ByteToCharMacRoman \
        sun.io.ByteToCharMacRomania \
        sun.io.ByteToCharMacSymbol \
        sun.io.ByteToCharMacThai \
        sun.io.ByteToCharMacTurkish \
        sun.io.ByteToCharMacUkraine \
        sun.io.ByteToCharSJIS \
        sun.io.ByteToCharTIS620 \
        sun.io.CharToByteBig5 \
        sun.io.CharToByteCp037 \
        sun.io.CharToByteCp1006 \
        sun.io.CharToByteCp1025 \
        sun.io.CharToByteCp1026 \
        sun.io.CharToByteCp1046 \
        sun.io.CharToByteCp1097 \
        sun.io.CharToByteCp1098 \
        sun.io.CharToByteCp1112 \
        sun.io.CharToByteCp1122 \
        sun.io.CharToByteCp1123 \
        sun.io.CharToByteCp1124 \
        sun.io.CharToByteCp1140 \
        sun.io.CharToByteCp1141 \
        sun.io.CharToByteCp1142 \
        sun.io.CharToByteCp1143 \
        sun.io.CharToByteCp1144 \
        sun.io.CharToByteCp1145 \
        sun.io.CharToByteCp1146 \
        sun.io.CharToByteCp1147 \
        sun.io.CharToByteCp1148 \
        sun.io.CharToByteCp1149 \
        sun.io.CharToByteCp1250 \
        sun.io.CharToByteCp1251 \
        sun.io.CharToByteCp1253 \
        sun.io.CharToByteCp1254 \
        sun.io.CharToByteCp1255 \
        sun.io.CharToByteCp1256 \
        sun.io.CharToByteCp1257 \
        sun.io.CharToByteCp1258 \
        sun.io.CharToByteCp1381 \
        sun.io.CharToByteCp1383 \
        sun.io.CharToByteCp273 \
        sun.io.CharToByteCp277 \
        sun.io.CharToByteCp278 \
        sun.io.CharToByteCp280 \
        sun.io.CharToByteCp284 \
        sun.io.CharToByteCp285 \
        sun.io.CharToByteCp297 \
        sun.io.CharToByteCp33722 \
        sun.io.CharToByteCp420 \
        sun.io.CharToByteCp424 \
        sun.io.CharToByteCp437 \
        sun.io.CharToByteCp500 \
        sun.io.CharToByteCp737 \
        sun.io.CharToByteCp775 \
        sun.io.CharToByteCp838 \
        sun.io.CharToByteCp850 \
        sun.io.CharToByteCp852 \
        sun.io.CharToByteCp855 \
        sun.io.CharToByteCp856 \
        sun.io.CharToByteCp857 \
        sun.io.CharToByteCp858 \
        sun.io.CharToByteCp860 \
        sun.io.CharToByteCp861 \
        sun.io.CharToByteCp862 \
        sun.io.CharToByteCp863 \
        sun.io.CharToByteCp864 \
        sun.io.CharToByteCp865 \
        sun.io.CharToByteCp866 \
        sun.io.CharToByteCp868 \
        sun.io.CharToByteCp869 \
        sun.io.CharToByteCp870 \
        sun.io.CharToByteCp871 \
        sun.io.CharToByteCp874 \
        sun.io.CharToByteCp875 \
        sun.io.CharToByteCp918 \
        sun.io.CharToByteCp921 \
        sun.io.CharToByteCp922 \
        sun.io.CharToByteCp923 \
        sun.io.CharToByteCp930 \
        sun.io.CharToByteCp933 \
        sun.io.CharToByteCp935 \
        sun.io.CharToByteCp937 \
        sun.io.CharToByteCp939 \
        sun.io.CharToByteCp942 \
        sun.io.CharToByteCp942C \
        sun.io.CharToByteCp943 \
        sun.io.CharToByteCp943C \
        sun.io.CharToByteCp948 \
        sun.io.CharToByteCp949 \
        sun.io.CharToByteCp949C \
        sun.io.CharToByteCp950 \
        sun.io.CharToByteCp964 \
        sun.io.CharToByteCp970 \
        sun.io.CharToByteDBCS_ASCII \
        sun.io.CharToByteDBCS_EBCDIC \
        sun.io.CharToByteDoubleByte \
        sun.io.CharToByteEUC \
        sun.io.CharToByteEUC_CN \
        sun.io.CharToByteEUC_JP \
        sun.io.CharToByteEUC_KR \
        sun.io.CharToByteEUC_TW \
        sun.io.CharToByteGBK \
        sun.io.CharToByteISO2022 \
        sun.io.CharToByteISO2022CN_CNS \
        sun.io.CharToByteISO2022CN_GB \
        sun.io.CharToByteISO2022JP \
        sun.io.CharToByteISO2022KR \
        sun.io.CharToByteISO8859_15_FDIS \
        sun.io.CharToByteISO8859_2 \
        sun.io.CharToByteISO8859_3 \
        sun.io.CharToByteISO8859_4 \
        sun.io.CharToByteISO8859_5 \
        sun.io.CharToByteISO8859_6 \
        sun.io.CharToByteISO8859_7 \
        sun.io.CharToByteISO8859_8 \
        sun.io.CharToByteISO8859_9 \
        sun.io.CharToByteJIS0201 \
        sun.io.CharToByteJIS0208 \
        sun.io.CharToByteJIS0212 \
        sun.io.CharToByteJohab \
        sun.io.CharToByteKOI8_R \
        sun.io.CharToByteMS874 \
        sun.io.CharToByteMS932 \
        sun.io.CharToByteMS936 \
        sun.io.CharToByteMS950 \
        sun.io.CharToByteMacArabic \
        sun.io.CharToByteMacCentralEurope \
        sun.io.CharToByteMacCroatian \
        sun.io.CharToByteMacCyrillic \
        sun.io.CharToByteMacDingbat \
        sun.io.CharToByteMacGreek \
        sun.io.CharToByteMacHebrew \
        sun.io.CharToByteMacIceland \
        sun.io.CharToByteMacRoman \
        sun.io.CharToByteMacRomania \
        sun.io.CharToByteMacSymbol \
        sun.io.CharToByteMacThai \
        sun.io.CharToByteMacTurkish \
        sun.io.CharToByteMacUkraine \
        sun.io.CharToByteSJIS \
        sun.io.CharToByteTIS620 \

endif

#
# Converters for Japanese local support
# Not added to CVM_FLAGS yet since we do not officically support this.
# It is only intended for demo or testing purposes.
#

ifeq ($(JA_CONVERTERS), true)
CLASSLIB_CLASSES += \
   sun.io.CharToByteEUC \
   sun.io.CharToByteEUC_JP \
   sun.io.CharToByteJIS0208 \
   sun.io.CharToByteJIS0201 \
   sun.io.CharToByteJIS0212 \
   sun.io.CharToByte ISO2022JP \
   sun.io.CharToByte Johab \
   sun.io.CharToByte SJIS \
   sun.io.ByteToCharEUC_JP \
   sun.io.ByteToCharJIS0208 \
   sun.io.ByteToCharJIS0201 \
   sun.io.ByteToCharJIS0212 \
   sun.io.ByteToCharISO2022JP \
   sun.io.ByteToCharJISAutoDetect \
   sun.io.ByteToCharJohab \
   sun.io.ByteToCharSJIS
endif

#
# Personal profile sits on top of Basis so we need to include 
# Basis definitions.
#
include $(CDC_DIR)/build/share/defs_basis.mk

# Define system properties from makefile settings.
# "isRestricted" values that are set to 'true' in the
# defs_$(J2ME_CLASSLIB)_$(AWT_IMPLEMENTATION).mk file
# are written out to the system properties.
# By default we assume all to be false.

# Note: this needs to be done after including defs_basis.mk,
# as defs_$(J2ME_CLASSLIB)_$(AWT_IMPLEMENTATION).mk is 
# included from defs_basis.mk.

# Only write out properties that are personal specific.
# defs_basis.mk takes care of the rest.

#fix for bug 6272320
#ifeq ($(TextField.setEchoChar.isRestricted), true)
#SYSTEM_PROPERTIES += java.awt.TextField.setEchoChar.isRestricted=$(TextField.setEchoChar.isRestricted)
#endif

ifeq ($(Dialog.setSize.isRestricted), true)
SYSTEM_PROPERTIES += java.awt.Dialog.setSize.isRestricted=$(Dialog.setSize.isRestricted)
endif

ifeq ($(Dialog.setResizable.isRestricted), true)
SYSTEM_PROPERTIES += java.awt.Dialog.setResizable.isRestricted=$(Dialog.setResizable.isRestricted)
endif

ifeq ($(Dialog.setLocation.isRestricted), true)
SYSTEM_PROPERTIES += java.awt.Dialog.setLocation.isRestricted=$(Dialog.setLocation.isRestricted)
endif

ifeq ($(Dialog.setTitle.isRestricted), true)
SYSTEM_PROPERTIES += java.awt.Dialog.setTitle.isRestricted=$(Dialog.setTitle.isRestricted)
endif

ifeq ($(Dialog.setUndecorated.isRestricted), true)
SYSTEM_PROPERTIES += java.awt.Dialog.setUndecorated.isRestricted=$(Dialog.setUndecorated.isRestricted)
endif

#
# PP demo classes
#
# NOTE: Has to appear after including defs_basis.mk 
#       so that the correct manifest file is used
CVM_DEMOCLASSES_SRCDIRS += $(CVM_SHAREROOT)/personal/demo
CVM_DEMO_CLASSES += \
    personal.DemoApplet \
    personal.DemoFrame \
    personal.DemoXlet \
    personal.demos.WidgetDemo \

