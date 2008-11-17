
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

# Is there a better, centralized location to put the ant tool location?
CVM_JUMP_BUILDDIR	= $(CDC_DIST_DIR)/jump

ifeq ($(USE_JUMP),true)
#
# JUMP defs
#

export JAVA_HOME	= $(JDK_HOME)

#JUMP's binary bundle pattern file name
BINARYBUNDLE_PATTERN_FILENAME=.binary-pattern
# .jar and .zip files to compile jump classes against
JUMP_BOOTCLASSES0 = $(patsubst $(CVM_BUILD_TOP)/%,%,$(CVM_BUILDTIME_CLASSESZIP) $(LIB_CLASSESJAR) $(JSROP_JARS))
JUMP_BOOTCLASSES = $(patsubst $(CVM_BUILD_TOP_ABS)/%,%,$(JUMP_BOOTCLASSES0))
JUMP_ANT_OPTIONS += -Djump.boot.cp=$(subst $(space),$(comma),$(JUMP_BOOTCLASSES)) \
		    -Ddist.dir=$(call POSIX2HOST,$(CVM_JUMP_BUILDDIR)) 	\
		    -Dcdc.dir=$(call POSIX2HOST,$(CDC_DIST_DIR)) \
		    -Dbinary.pattern.file=$(BINARYBUNDLE_PATTERN_FILENAME)   

ifeq ($(USE_MIDP), true)
JUMP_ANT_OPTIONS         += -Dmidp_output_dir=$(subst $(CDC_DIST_DIR)/,,$(MIDP_OUTPUT_DIR))
endif

ifneq ($(JUMP_BUILD_PROPS_FILE),)
JUMP_ANT_OPTIONS += -Duser.build.properties=$(JUMP_BUILD_PROPS_FILE) 
endif

# The default JUMP component location
JUMP_DIR		?= $(COMPONENTS_DIR)/jump
ifeq ($(wildcard $(JUMP_DIR)/build/build.xml),)
$(error JUMP_DIR must point to the JUMP directory: $(JUMP_DIR))
endif
JUMP_OUTPUT_DIR         = $(CVM_JUMP_BUILDDIR)/lib
JUMP_SRCDIR             = $(JUMP_DIR)/src
JUMP_SCRIPTS_DIR        = $(JUMP_DIR)/tools/scripts

JUMP_JSROP_JARS         = :$(subst $(space),$(PS),$(patsubst $(CVM_BUILD_TOP)%,\$$PHONEME_DIST%,$(JSROP_JARS)))

#
# JUMP_DEPENDENCIES defines what needs to be built for jump
#
JUMP_DEPENDENCIES   = $(CVM_BUILDTIME_CLASSESZIP)

ifneq ($(CVM_PRELOAD_LIB), true)
JUMP_DEPENDENCIES   += $(LIB_CLASSESJAR)
endif

JUMP_API_CLASSESZIP             := \
    $(addprefix $(JUMP_OUTPUT_DIR)/,\
        shared-jump-api.jar client-jump-api.jar executive-jump-api.jar)
JUMP_IMPL_CLASSESZIP            := \
    $(addprefix $(JUMP_OUTPUT_DIR)/,\
        shared-jump-impl.jar client-jump-impl.jar executive-jump-impl.jar)
JUMP_SHARED_BOOTCLASSESZIP      := $(JUMP_OUTPUT_DIR)/jump.jar
JUMP_EXECUTIVE_BOOTCLASSESZIP   := $(JUMP_OUTPUT_DIR)/executive-jump.jar

JUMP_SRCDIRS           += \
	$(JUMP_SRCDIR)/share/api/native \
	$(JUMP_SRCDIR)/share/impl/isolate/native \
	$(JUMP_SRCDIR)/share/impl/os/native

#
# Add as necessary
#	$(JUMP_SRCDIR)/share/impl/<component>/native \

JUMP_INCLUDE_DIRS  += \
	$(JUMP_SRCDIR)/share/api/native/include \
	$(JUMP_SRCDIR)/share/impl/os/native/include \

# Add as necessary
#	$(JUMP_SRCDIR)/share/impl/<component>/native/include \
#

#
# Any shared native code goes here.
# 
JUMP_OBJECTS            += \
	jump_os_impl.o \
	jump_messaging.o \
	jump_isolate_impl.o \

#
# Any native code for the stand-alone jump native library goes here
# 
JUMP_NATIVE_LIBRARY_OBJECTS            += \
	jump_messaging.o

JUMP_NATIVE_LIBRARY_PATHNAME = $(JUMP_OUTPUT_DIR)/$(LIB_PREFIX)jumpmesg$(LIB_POSTFIX)

#
# Make sure this shared library gets built
#
CLASSLIB_DEPS += $(JUMP_NATIVE_LIBRARY_PATHNAME)

# 
# For the binary bundle 
#
# The file $(BINARYBUNDLE_PATTERN_FILENAME) includes an additonal pattern
# that a jump build needs to include to the binary bundle.
# See jump's build/build-impl-contentstore.xml, "generate-pattern-file" for 
# how this pattern file is generated.
#
BINARY_BUNDLE_PATTERNS += \
       $(CVM_BUILD_TOP)/`cat $(CVM_JUMP_BUILDDIR)/generated/$(BINARYBUNDLE_PATTERN_FILENAME)`

#
# Get any platform specific dependencies of any kind.
#
-include $(CDC_CPU_COMPONENT_DIR)/build/$(TARGET_CPU_FAMILY)/defs_jump.mk
-include $(CDC_OS_COMPONENT_DIR)/build/$(TARGET_OS)/defs_jump.mk
-include $(CDC_OSCPU_COMPONENT_DIR)/build/$(TARGET_OS)-$(TARGET_CPU_FAMILY)/defs_jump.mk
-include $(CDC_DEVICE_COMPONENT_DIR)/build/$(TARGET_OS)-$(TARGET_CPU_FAMILY)-$(TARGET_DEVICE)/defs_jump.mk

#
# Finally modify CVM variables w/ all the JUMP items
#
JUMP_NATIVE_LIB_OBJS     = $(patsubst %.o,$(CVM_OBJDIR)/%.o,$(JUMP_NATIVE_LIBRARY_OBJECTS))
CVM_CVMC_OBJECTS        += $(JUMP_NATIVE_LIB_OBJS)
CVM_OBJECTS             += $(patsubst %.o,$(CVM_OBJDIR)/%.o,$(JUMP_OBJECTS))
CVM_SRCDIRS             += $(JUMP_SRCDIRS)
CVM_INCLUDE_DIRS        += $(JUMP_INCLUDE_DIRS)

MIDP_CLASSESZIP_DEPS += $(JUMP_API_CLASSESZIP)

#
# In case we build any libraries that we want the cvm binary to use
#
#LINKLIBS 		+= -L$(PCSL_OUTPUT_DIR)/$(PCSL_TARGET)/lib -lpcsl_file -lpcsl_memory -lpcsl_network -lpcsl_print -lpcsl_string

# Add JUMP classes to JCC input list so they can be romized.
ifeq ($(CVM_PRELOAD_LIB), true)
CVM_JCC_INPUT		+= $(JUMP_API_CLASSESZIP)
CVM_JCC_INPUT		+= $(JUMP_IMPL_CLASSESZIP)

CVM_CNI_CLASSES +=

endif

endif

