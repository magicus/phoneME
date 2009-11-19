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

CVM_MIDP_BUILDDIR	= $(CDC_DIST_DIR)/midp

ifeq ($(USE_MIDP),true)

# Include target specific makefiles first
-include $(CDC_CPU_COMPONENT_DIR)/build/$(TARGET_CPU_FAMILY)/defs_midp.mk
-include $(CDC_OS_COMPONENT_DIR)/build/$(TARGET_OS)/defs_midp.mk
-include $(CDC_DEVICE_COMPONENT_DIR)/build/$(TARGET_OS)-$(TARGET_CPU_FAMILY)-$(TARGET_DEVICE)/defs_midp.mk

# Don't use MIDP private memory management implementation by default.
USE_MIDP_MALLOC  ?= false

ifeq ($(USE_GCI), true)
    MIDP_PLATFORM ?= linux_gci
else
    MIDP_PLATFORM ?= linux_fb_gcc
endif

MIDP_TARGET_OS ?= linux

# bootclasspath classes needed to compile midp
VM_BOOTCLASSPATH0	= $(CVM_BUILDTIME_CLASSESZIP) $(LIB_CLASSESJAR)
ifeq ($(USE_JUMP), true)    
    VM_BOOTCLASSPATH0	+= $(JUMP_API_CLASSESZIP)
endif
VM_BOOTCLASSPATH = $(subst $(space),$(PS),$(VM_BOOTCLASSPATH0))

#
# Target tools directory for compiling both PCSL and MIDP.
#
ifeq ($(CVM_USE_NATIVE_TOOLS), false)
GNU_TOOLS_BINDIR	?= $(CVM_TARGET_TOOLS_PREFIX)
endif

#
# PCSL defs
#
TARGET_CPU		?= $(TARGET_CPU_FAMILY)
PCSL_DIR		?= $(COMPONENTS_DIR)/pcsl
export PCSL_DIR
ifeq ($(wildcard $(PCSL_DIR)/donuts),)
$(error PCSL_DIR must point to the PCSL directory: $(PCSL_DIR))
endif
PCSL_OUTPUT_DIR	?= $(CVM_MIDP_BUILDDIR)/pcsl_fb
export PCSL_OUTPUT_DIR
PCSL_TARGET		?= $(TARGET_OS)_$(TARGET_CPU)
PCSL_PLATFORM		?= $(PCSL_TARGET)_gcc
NETWORK_MODULE		?= bsd/generic
PCSL_MAKE_OPTIONS 	?=
MEMORY_MODULE		?= malloc
MEMORY_PORT_MODULE	?= malloc

ifeq ($(findstring javacall,$(PCSL_PLATFORM)),javacall)
PCSL_DEPENDENCIES       += $(JAVACALL_LIBRARY)
PCSL_MAKE_OPTIONS       += JAVACALL_OUTPUT_DIR=$(JAVACALL_BUILD_DIR)
CVM_INCLUDE_JAVACALL     = true
endif

#
# MIDP defs
#
export JDK_DIR		= $(JDK_HOME)
TARGET_VM		= cdc_vm
MIDP_DIR		?= $(COMPONENTS_DIR)/midp
MIDP_DEFS_CDC_MK	= $(MIDP_DIR)/build/common/cdc_vm/defs_cdc.mk
ifeq ($(wildcard $(MIDP_DEFS_CDC_MK)),)
$(error MIDP_DIR must point to the MIDP directory: $(MIDP_DIR))
endif

# Setup JPEG_DIR
ifeq ($(USE_JPEG), true)
export JPEG_DIR ?= $(COMPONENTS_DIR)/jpeg
JPEG_MAKE_FILE = $(JPEG_DIR)/$(SUBSYSTEM_MAKE_FILE)
ifeq ($(wildcard $(JPEG_MAKE_FILE)),)
$(error JPEG_DIR must point to a directory containing jpeg sources: $(JPEG_DIR))
endif
endif

# Locate the midp-com component
ifeq ($(USE_MIDP_COM),true)
PROJECT_MIDP_DIR ?= $(COMPONENTS_DIR)/midp-com
ifeq ($(wildcard $(PROJECT_MIDP_DIR)/build/common/project.gmk),)
$(error PROJECT_MIDP_DIR must point to a directory containing the midp-com sources: $(PROJECT_MIDP_DIR))
endif
else
PROJECT_MIDP_DIR ?= $(MIDP_DIR)
endif

MIDP_MAKEFILE_DIR 	?= $(MIDP_DIR)/build/$(MIDP_PLATFORM)

MIDP_OUTPUT_DIR		?= $(CVM_MIDP_BUILDDIR)/midp_$(MIDP_PLATFORM)
export MIDP_OUTPUT_DIR
USE_SSL			?= false
USE_RESTRICTED_CRYPTO	?= false
VERIFY_BUILD_ENV	?= 
#CONFIGURATION_OVERRIDE	?= MEASURE_STARTUP=true 
USE_QT_FB		?= false
USE_DIRECTFB		?= false
USE_DIRECTDRAW          ?= false
# The MIDP makefiles should be fixed to not require CLDC_DIST_DIR for CDC build.
USE_CONFIGURATOR	?= true

ifeq ($(CVM_DEBUG), true)
USE_DEBUG		= true
endif

MIDP_CLASSESZIP_DEPS	=

# If this is a non-romized build, redirect the location of the 
# midp classes.zip and libmidp.so to the cdc's build dir.
# For the romized build, both the java and native would be folded into
# cvm, but set MIDP_CLASSES_ZIP to default for java compilation
# rule in rules_midp.mk.

ifneq ($(CVM_PRELOAD_LIB), true)
MIDP_CLASSES_ZIP	?= $(CVM_LIBDIR_ABS)/midpclasses.zip
MIDP_PUB_CLASSES_ZIP    ?= $(CVM_LIBDIR_ABS)/midpclassespub.zip
ifneq ($(CVM_CREATE_RTJAR),true)
MIDP_PRIV_CLASSES_ZIP   ?= $(CVM_LIBDIR_ABS)/midpclassespriv.zip
else
MIDP_PRIV_CLASSES_ZIP   ?= $(CVM_RTJARS_DIR)/midpclassespriv.zip
endif
else
MIDP_CLASSES_ZIP	?= $(MIDP_OUTPUT_DIR)/classes.zip
MIDP_PUB_CLASSES_ZIP    ?= $(MIDP_OUTPUT_DIR)/classespub.zip
MIDP_PRIV_CLASSES_ZIP   ?= $(MIDP_OUTPUT_DIR)/classespriv.zip
endif
ifneq ($(CVM_STATICLINK_LIBS), true)
MIDP_SHARED_LIB		?= $(CVM_LIBDIR_ABS)/libmidp$(LIB_POSTFIX)
else
MIDP_OBJ_FILE_LIST	?= $(MIDP_OUTPUT_DIR)/bin/$(TARGET_CPU)/midp_obj_files.lst 
endif
MIDP_NATIVES = $(MIDP_SHARED_LIB) $(MIDP_OBJ_FILE_LIST)

LINKCVM_EXTRA_DEPS += $(MIDP_NATIVES)


#List of MIDP classes
MIDP_CLASSLIST		= $(CVM_BUILD_TOP)/.midpclasslist
CVM_MIDPCLASSLIST_FILES += $(MIDP_CLASSLIST)

ifeq ($(CVM_STATICLINK_LIBS), true)
LINKCVM_EXTRA_OBJECTS += $(shell cat $(MIDP_OBJ_FILE_LIST))
MIDP_LIBS 		?= \
        -L$(PCSL_OUTPUT_DIR)/$(PCSL_TARGET)/lib -lpcsl_file \
        -lpcsl_memory -lpcsl_network -lpcsl_print -lpcsl_string \
        -lpcsl_escfilenames
LINKLIBS 		+= $(MIDP_LIBS)
endif
include $(MIDP_DEFS_CDC_MK)

ifeq ($(CVM_PRELOAD_LIB), true)
# Add MIDP classes to JCC input list so they can be romized.
CVM_JCC_INPUT           += $(MIDP_PRIV_CLASSES_ZIP)
CVM_JCC_CL_MIDP_INPUT	+= -cl:midp:boot $(MIDP_PUB_CLASSES_ZIP) $(JSROP_AGENT_JARS)
# Add MIDP CNI classes to CVM_CNI_CLASSES
CVM_CNI_CLASSES += $(MIDP_CNI_CLASSES)
else
# Not romized, so add MIDP_PRIV_CLASSES_ZIP to the bootclasspath
ifneq ($(CVM_CREATE_RTJAR), true)
CVM_JARFILES += $(patsubst $(CVM_LIBDIR_ABS)/%,"%"$(comma),$(MIDP_PRIV_CLASSES_ZIP))
else
CVM_RTJARS_LIST += $(MIDP_PRIV_CLASSES_ZIP)
endif
endif

# Setup the property containing where midp implementation is located.
# This will be a list all the jar and zip files loaded by the
# MIDPImplemantionClassLoader. We strip out everything but the base name,
# and java code will prepend java.home/lib/ to it. Note, if we are romizing,
# then there are no jar or zip files to load from.
CVM_BUILD_DEF_VARS += CVM_PROP_MIDP_IMPL
ifeq ($(CVM_PRELOAD_LIB), true)
CVM_PROP_MIDP_IMPL = ""
else
CVM_PROP_MIDP_IMPL += \
	"$(patsubst $(CVM_LIBDIR_ABS)/%,%,$(MIDP_PUB_CLASSES_ZIP)) \
	$(patsubst $(JSROP_LIB_DIR)/%,%,$(JSROP_AGENT_JARS))"
endif

endif
