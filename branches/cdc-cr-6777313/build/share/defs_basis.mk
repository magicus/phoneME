#
# @(#)defs_basis.mk	1.126 06/10/10
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
-include $(CDC_CPU_COMPONENT_DIR)/build/$(TARGET_CPU_FAMILY)/defs_basis.mk
-include $(CDC_OS_COMPONENT_DIR)/build/$(TARGET_OS)/defs_basis.mk
-include $(CDC_DEVICE_COMPONENT_DIR)/build/$(TARGET_OS)-$(TARGET_CPU_FAMILY)-$(TARGET_DEVICE)/defs_basis.mk

#
# Assume we are building a qt based implementation of AWT for
# basis by default. The target makefiles may change this. Don't
# override anything that defs_personal.mk may have set.  Wait until after we
# check the target-specific makefiles to set this
#
AWT_IMPLEMENTATION ?= qt
AWT_IMPLEMENTATION_DIR ?= $(CDC_DIR)

#
# Include awt implementation makefiles for the profile.
#
include $(AWT_IMPLEMENTATION_DIR)/build/share/defs_$(J2ME_CLASSLIB)_$(AWT_IMPLEMENTATION).mk

#
# JPEG_LIB_LIBS. Don't override it if the target defs_basis.mk set it.
#
JPEG_LIB_LIBS ?= -lm

#
# Determine native library name for the awt implementation.
# This is assumed to the the AWT implementation name followed by "awt"
# (e.g. libmicrowindowsawt.so). Don't override anything that may have
# been setup by defs_personal.mk, which will use $(AWT_PEERSET)awt instead.
#
AWT_LIB_NAME	?= $(AWT_IMPLEMENTATION)awt

JPEG_LIB_NAME	?= awtjpeg

#
# Shared libraries, by default, go into CVM_LIBDIR. This may have
# been changed, so use AWT_LIBDIR instead.
#
AWT_LIBDIR	?= $(CVM_LIBDIR)

#
# Full path names of the shared libraries we build. Do not
# override any previous setting, which could vary based
# on target OS or architecture.
#
ifneq ($(CVM_STATICLINK_LIBS), true)
AWT_LIB_PATHNAME  = $(AWT_LIBDIR)/$(LIB_PREFIX)$(AWT_LIB_NAME)$(LIB_POSTFIX)
JPEG_LIB_PATHNAME = $(AWT_LIBDIR)/$(LIB_PREFIX)$(JPEG_LIB_NAME)$(LIB_POSTFIX)
endif

#
# Make the build depend on the following shared libraries
#
ifneq ($(CVM_STATICLINK_LIBS), true)
CLASSLIB_DEPS += $(AWT_LIB_PATHNAME) $(JPEG_LIB_PATHNAME)
endif

#
# If we are statically linking the profile, then we must link the profile
# objects with the CVM executable.
#
ifeq ($(CVM_STATICLINK_LIBS), true)
CVM_OBJECTS += $(patsubst %.o,$(CVM_OBJDIR)/%.o,$(AWT_LIB_OBJS))
CVM_OBJECTS += $(patsubst %.o,$(CVM_OBJDIR)/%.o,$(JPEG_LIB_OBJS))
endif

#
# If statically linking the profile libraries, then list the profile
# libraries that are built in, so loadLibrary() is not called on them.
#
ifeq ($(CVM_STATICLINK_LIBS), true)
BUILTIN_LIBS += $(AWT_LIB_NAME) $(JPEG_LIB_NAME)
endif

# by default we don't want to exclude the xlet runner
EXCLUDE_XLET_RUNNER ?= false

#
# All the build flags we need to keep track of in case they are toggled.
#
CVM_FLAGS += \
	EXCLUDE_XLET_RUNNER \
	AWT_IMPLEMENTATION \

#
# Wipe out objects and classes when AWT_IMPLEMENTATION changes.
#
AWT_IMPLEMENTATION_CLEANUP_ACTION = \
	rm -rf *_classes/* lib/* $(CVM_OBJDIR)
ifeq ($(CVM_PRELOAD_LIB),true)
AWT_IMPLEMENTATION_CLEANUP_ACTION += \
	btclasses*
endif

EXCLUDE_XLET_RUNNER_CLEANUP_ACTION = \
	$(CVM_JAVAC_DEBUG_CLEANUP_ACTION)

#
# Basis Profile shared native directories
#

PROFILE_SRCDIRS_NATIVE += \
	$(CVM_SHAREROOT)/basis/native/image/gif \
	$(CVM_SHAREROOT)/basis/native/image/jpeg \
	$(CVM_SHAREROOT)/basis/native/image/jpeg/lib 

PROFILE_INCLUDE_DIRS += \
	$(CVM_SHAREROOT)/basis/native/image/gif \
	$(CVM_SHAREROOT)/basis/native/image/jpeg/lib 

#
# Basis shared class directories

PROFILE_SRCDIRS += \
	$(CVM_SHAREROOT)/basis/classes/common

#
# Shared Native code
#
AWT_LIB_OBJS += \
	gifdecoder.o \


JPEG_LIB_OBJS += \
	jpegdecoder.o 

ifneq ($(EXCLUDE_JPEG_LIB),true)
JPEG_LIB_OBJS += \
	jcapimin.o \
	jcmaster.o \
	jdcoefct.o \
	jdphuff.o \
	jidctfst.o \
	jcapistd.o \
	jcomapi.o \
	jdcolor.o\
	jdpostct.o \
	jidctint.o \
	jccoefct.o \
	jcparam.o \
	jddctmgr.o \
	jdsample.o \
	jidctred.o \
	jccolor.o \
	jcphuff.o \
	jdhuff.o \
	jdtrans.o \
	jmemmgr.o \
	jcdctmgr.o \
	jcprepct.o \
	jdinput.o \
	jerror.o \
	jmemnobs.o \
	jchuff.o \
	jcsample.o \
	jdmainct.o \
	jfdctflt.o \
	jcinit.o \
	jctrans.o \
	jdmarker.o \
	jfdctfst.o \
	jquant1.o \
	jcmainct.o \
	jdapimin.o \
	jdmaster.o \
	jfdctint.o \
	jquant2.o \
	jcmarker.o \
	jdapistd.o \
	jdmerge.o \
	jidctflt.o \
	jutils.o
endif

#
# Security File
#
CVM_POLICY_SRC  ?= $(CVM_TOP)/src/share/basis/lib/security/java.policy

#
# Shared class files, regardless of OS or AWT implementation.
#
CLASSLIB_CLASSES += \
		java.beans.Beans \
		java.beans.PropertyChangeEvent \
		java.beans.PropertyChangeListener \
		java.beans.PropertyChangeSupport \
		java.beans.PropertyVetoException \
		java.beans.VetoableChangeListener \
		java.beans.VetoableChangeSupport \
		java.beans.Visibility \
		java.rmi.AccessException \
		java.rmi.AlreadyBoundException \
		java.rmi.NotBoundException \
		java.rmi.Remote \
		java.rmi.RemoteException \
		java.rmi.UnexpectedException \
		java.rmi.registry.Registry \
		javax.microedition.xlet.UnavailableContainerException \
		javax.microedition.xlet.Xlet \
		javax.microedition.xlet.XletContext \
		javax.microedition.xlet.XletStateChangeException \
		javax.microedition.xlet.ixc.IxcRegistry \
		javax.microedition.xlet.ixc.StubException \
		javax.microedition.xlet.ixc.IxcPermission \
		sun.misc.Cache \
		sun.misc.Ref \
		sun.misc.WeakCache \
		sun.net.www.content.image.gif \
		sun.net.www.content.image.jpeg \
		sun.net.www.content.image.png \
		sun.io.FileFileIO \
		sun.io.FileIO \
		sun.io.FileIOFactory \
		sun.io.ObjectInputStreamDelegate \
		sun.io.ObjectOutputStreamDelegate \
		sun.io.ObjectStreamPermissionConstants \
		java.awt.AlphaComposite \
		java.awt.AWTError \
		java.awt.AWTEvent \
		java.awt.AWTEventMulticaster \
		java.awt.AWTException \
		java.awt.AWTPermission \
		java.awt.ActiveEvent \
		java.awt.Adjustable \
		java.awt.AttributeValue \
		java.awt.BasicStroke \
		java.awt.BorderLayout \
		java.awt.CardLayout \
		java.awt.Color \
		java.awt.Component \
		java.awt.Composite \
		java.awt.Conditional \
		java.awt.Container \
		java.awt.Cursor \
		java.awt.Dimension \
		java.awt.EventDispatchThread \
		java.awt.EventQueue \
		java.awt.EventQueueListener \
		java.awt.FlowLayout \
		java.awt.Font \
		java.awt.FontMetrics \
		java.awt.Frame \
		java.awt.Graphics \
		java.awt.Graphics2D \
		java.awt.GraphicsConfiguration \
		java.awt.GraphicsDevice \
		java.awt.GraphicsEnvironment \
		java.awt.GridBagConstraints \
		java.awt.GridBagLayout \
		java.awt.GridLayout \
		java.awt.HeadlessException \
		java.awt.IllegalComponentStateException \
		java.awt.Image \
		java.awt.ImageCapabilities \
		java.awt.Insets \
		java.awt.ItemSelectable \
		java.awt.LayoutManager \
		java.awt.LayoutManager2 \
		java.awt.MediaTracker \
		java.awt.Point \
		java.awt.Polygon \
		java.awt.Rectangle \
		java.awt.SequencedEvent \
		java.awt.Shape \
		java.awt.Stroke \
		java.awt.SystemColor \
		java.awt.Toolkit \
		java.awt.Transparency \
		java.awt.Window \
		java.awt.color.ColorSpace \
		java.awt.event.AWTEventListener \
		java.awt.event.AWTEventListenerProxy \
		java.awt.event.ActionEvent \
		java.awt.event.ActionListener \
		java.awt.event.AdjustmentEvent \
		java.awt.event.AdjustmentListener \
		java.awt.event.ComponentAdapter \
		java.awt.event.ComponentEvent \
		java.awt.event.ComponentListener \
		java.awt.event.ContainerAdapter \
		java.awt.event.ContainerEvent \
		java.awt.event.ContainerListener \
		java.awt.event.FocusAdapter \
		java.awt.event.FocusEvent \
		java.awt.event.FocusListener \
		java.awt.event.InputEvent \
		java.awt.event.InvocationEvent \
		java.awt.event.InputMethodEvent \
		java.awt.event.InputMethodListener \
		java.awt.event.ItemEvent \
		java.awt.event.ItemListener \
		java.awt.event.KeyAdapter \
		java.awt.event.KeyEvent \
		java.awt.event.KeyListener \
		java.awt.event.MouseAdapter \
		java.awt.event.MouseEvent \
		java.awt.event.MouseListener \
		java.awt.event.MouseWheelEvent \
		java.awt.event.MouseWheelListener \
		java.awt.event.MouseMotionAdapter \
		java.awt.event.MouseMotionListener \
		java.awt.event.PaintEvent \
		java.awt.event.TextEvent \
		java.awt.event.TextListener \
		java.awt.event.WindowAdapter \
		java.awt.event.WindowEvent \
		java.awt.event.WindowListener \
                java.awt.font.TextAttribute \
                java.awt.font.TextHitInfo \
                java.awt.im.InputContext \
                java.awt.im.InputMethodHighlight \
                java.awt.im.InputMethodRequests \
                java.awt.im.InputSubset \
		java.awt.image.AreaAveragingScaleFilter \
		java.awt.image.BufferedImage \
		java.awt.image.ColorModel \
		java.awt.image.DataBuffer \
		java.awt.image.CropImageFilter \
		java.awt.image.DirectColorModel \
		java.awt.image.FilteredImageSource \
		java.awt.image.ImageConsumer \
		java.awt.image.ImageFilter \
		java.awt.image.ImageObserver \
		java.awt.image.ImageProducer \
		java.awt.image.IndexColorModel \
		java.awt.image.MemoryImageSource \
		java.awt.image.PixelGrabber \
		java.awt.image.RGBImageFilter \
		java.awt.image.RasterFormatException \
		java.awt.image.ReplicateScaleFilter \
		java.awt.image.VolatileImage \
		sun.awt.image.ByteArrayImageSource \
		sun.awt.image.FileImageSource \
		sun.awt.image.GifImageDecoder \
		sun.awt.image.ImageConsumerQueue \
		sun.awt.image.ImageDecoder \
		sun.awt.image.ImageFetchable \
		sun.awt.image.ImageFetcher \
		sun.awt.image.ImageFormatException \
		sun.awt.image.ImageProductionMonitor \
		sun.awt.image.ImageWatched \
		sun.awt.image.InputStreamImageSource \
		sun.awt.image.JPEGImageDecoder \
		sun.awt.image.PNGImageDecoder \
		sun.awt.image.URLImageSource \
		sun.awt.image.XbmImageDecoder \
		sun.awt.image.BufferedImagePeer \
		sun.awt.ConstrainableGraphics \
		sun.awt.AppContext \
		sun.awt.AWTSecurityManager \
		sun.awt.SunToolkit \
		sun.awt.IdentityWeakHashMap \
		sun.awt.Robot \
		sun.awt.RobotHelper \
		sun.awt.im.InputContext \
	       	sun.awt.im.InputMethod \
	   	sun.awt.im.InputMethodAdapter \
	   	sun.awt.im.InputMethodContext \
        \
        java.awt.FocusTraversalPolicy \
        java.awt.ContainerOrderFocusTraversalPolicy \
        java.awt.DefaultFocusTraversalPolicy \
        java.awt.KeyEventDispatcher \
        java.awt.KeyEventPostProcessor \
        java.awt.KeyboardFocusManager \
        java.awt.DefaultKeyboardFocusManager \
        java.awt.AWTKeyStroke \
        java.awt.event.WindowFocusListener \

# JUMP specific basis classes
ifeq ($(USE_JUMP), true)
CLASSLIB_CLASSES += \
		sun.mtask.xlet.XletFrame \
		sun.mtask.xlet.PXletRunner \
		sun.mtask.xlet.PXletStateQueue \
		sun.mtask.xlet.PXletManager \
		sun.mtask.xlet.PXletContextImpl 
else 
CLASSLIB_CLASSES += \
                com.sun.xlet.ixc.ConstantPool \
                com.sun.xlet.ixc.ImportRegistry \
                com.sun.xlet.ixc.IxcRegistryImpl \
                com.sun.xlet.ixc.RegistryKey \
                com.sun.xlet.ixc.TypeInfo \
                com.sun.xlet.ixc.Worker \
                com.sun.xlet.ixc.WrappedRemote \
                com.sun.xlet.ixc.IxcClassLoader 
endif

ifeq ($(EXCLUDE_XLET_RUNNER), false)
ifeq ($(USE_JUMP), false)
CLASSLIB_CLASSES += \
		com.sun.xlet.ToplevelFrame \
		com.sun.xlet.XletClassLoader \
		com.sun.xlet.XletContainer \
		com.sun.xlet.XletContextImpl \
		com.sun.xlet.XletLifecycleHandler \
		com.sun.xlet.XletManager \
		com.sun.xlet.XletRunner \
		com.sun.xlet.XletSecurity \
		com.sun.xlet.XletStateQueue 
#CVM_DEMO_CLASSES += \
#		IXCDemo.IXCMain 
endif
endif

#
# Converters, include the defaults. The rest will be sorted as we go!
#

CLASSLIB_CLASSES += \
		sun.io.ByteToCharCp1252 \
		sun.io.CharToByteCp1252 \
		sun.io.ByteToCharSingleByte \
		sun.io.CharToByteSingleByte \

# Define system properties from makefile settings
# "isRestricted" values that are set to 'true' in the
# defs_$(J2ME_CLASSLIB)_$(AWT_IMPLEMENTATION).mk file 
# are written out to the system properties.
# By default we assume all to be false.

SYSTEM_PROPERTIES += awt.toolkit=$(TOOLKIT_CLASS)

ifeq ($(AlphaComposite.SRC_OVER.isRestricted), true)
SYSTEM_PROPERTIES += java.awt.AlphaComposite.SRC_OVER.isRestricted=$(AlphaComposite.SRC_OVER.isRestricted)
endif

ifeq ($(Component.setCursor.isRestricted), true)
SYSTEM_PROPERTIES += java.awt.Component.setCursor.isRestricted=$(Component.setCursor.isRestricted)
endif

ifeq ($(Frame.setSize.isRestricted), true)
SYSTEM_PROPERTIES += java.awt.Frame.setSize.isRestricted=$(Frame.setSize.isRestricted)
endif

ifeq ($(Frame.setResizable.isRestricted), true)
SYSTEM_PROPERTIES += java.awt.Frame.setResizable.isRestricted=$(Frame.setResizable.isRestricted)
endif

ifeq ($(Frame.setLocation.isRestricted), true)
SYSTEM_PROPERTIES += java.awt.Frame.setLocation.isRestricted=$(Frame.setLocation.isRestricted)
endif

# fix for bug 6272320
#ifeq ($(Frame.setState.isRestricted), true)
#SYSTEM_PROPERTIES += java.awt.Frame.setState.isRestricted=$(Frame.setState.isRestricted)
#endif

ifeq ($(Frame.setTitle.isRestricted), true)
SYSTEM_PROPERTIES += java.awt.Frame.setTitle.isRestricted=$(Frame.setTitle.isRestricted)
endif

ifeq ($(Frame.setUndecorated.isRestricted), true)
SYSTEM_PROPERTIES += java.awt.Frame.setUndecorated.isRestricted=$(Frame.setUndecorated.isRestricted)
endif

ifeq ($(event.MouseEvent.isRestricted), true)
SYSTEM_PROPERTIES += java.awt.event.MouseEvent.isRestricted=$(event.MouseEvent.isRestricted)
SYSTEM_PROPERTIES += java.awt.event.MouseEvent.supportLevel=$(event.MouseEvent.supportLevel)
endif

ifeq ($(event.KeyEvent.isRestricted), true)
SYSTEM_PROPERTIES += java.awt.event.KeyEvent.isRestricted=$(event.KeyEvent.isRestricted)
SYSTEM_PROPERTIES += java.awt.event.KeyEvent.supportMask=$(event.KeyEvent.supportMask)
endif

ifneq ($(GRAPHICS_ENV_CLASS),)
SYSTEM_PROPERTIES += java.awt.graphicsenv=$(GRAPHICS_ENV_CLASS)
endif

#
# Demo stuff
#

CVM_DEMOCLASSES_SRCDIRS += $(CVM_SHAREROOT)/basis/demo

CVM_DEMO_CLASSES += \
    basis.Bean \
    basis.Builder \
    basis.DemoButton \
    basis.DemoButtonListener \
    basis.DemoFrame \
    basis.DemoXlet \
    basis.demos.BeanDemo \
    basis.demos.BorderLayoutDemo \
    basis.demos.CardLayoutDemo \
    basis.demos.Demo \
    basis.demos.DrawDemo \
    basis.demos.EventDemo \
    basis.demos.FlowLayoutDemo \
    basis.demos.FontDemo \
    basis.demos.FrameDemo \
    basis.demos.GameDemo \
    basis.demos.GraphicsDemo \
    basis.demos.GridBagLayoutDemo \
    basis.demos.GridLayoutDemo \
    basis.demos.ImageDemo \
    basis.demos.LayoutDemo \
    basis.demos.MiscDemo \
    basis.demos.NullLayoutDemo \
	IXCDemo.shared.Plane \
	IXCDemo.shared.PlaneListener \
	IXCDemo.shared.Position \
	IXCDemo.ixcXlets.clientXlet.PlaneClient \
	IXCDemo.ixcXlets.serverXlet.PlaneImpl \
	IXCDemo.ixcXlets.serverXlet.PlaneServer 

ifneq ($(USE_JUMP), true)
CVM_DEMO_CLASSES += \
	IXCDemo.IXCMain 
endif


#
# Unit Tests
#
CVM_TESTCLASSES_SRCDIRS += \
   $(CVM_TOP)/test/share/basis \

# Include profile specific gunit defs, if it exists
ifeq ($(CVM_GUNIT_TESTS), true)
-include $(CDC_DIR)/build/share/defs_$(J2ME_CLASSLIB)_gunit.mk
endif

CVM_TEST_CLASSES += \
	tests.volatileImage.TestVolatileGC \
	tests.volatileImage.TestVolatileComponent \
	tests.fullScreenMode.TestFull \

#
# Basis profile sits on top of foundation so we need to include 
# foundation definitions.
#
include $(CDC_DIR)/build/share/defs_foundation.mk
