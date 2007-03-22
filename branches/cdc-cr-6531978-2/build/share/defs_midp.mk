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

CVM_MIDP_BUILDDIR	= $(CDC_DIST_DIR)/midp

ifeq ($(USE_MIDP),true)

# Include target specific makefiles first
-include ../$(TARGET_CPU_FAMILY)/defs_midp.mk
-include ../$(TARGET_OS)/defs_midp.mk


ifeq ($(AWT_IMPLEMENTATION), gci)
    MIDP_PLATFORM = linux_gci
else
    MIDP_PLATFORM = linux_fb_gcc
endif


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
ifeq ($(wildcard $(PCSL_DIR)/donuts),)
$(error PCSL_DIR must point to the PCSL directory: $(PCSL_DIR))
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
TARGET_VM		= cdc_vm
MIDP_DIR		?= $(COMPONENTS_DIR)/midp
MIDP_DEFS_JCC_MK	= $(MIDP_DIR)/build/common/cdc_vm/defs_cdc.mk
ifeq ($(wildcard $(MIDP_DEFS_JCC_MK)),)
$(error MIDP_DIR must point to the MIDP directory: $(MIDP_DIR))
endif

MIDP_MAKEFILE_DIR 	?= build/$(MIDP_PLATFORM)
MIDP_OUTPUT_DIR		?= $(CVM_MIDP_BUILDDIR)/midp_$(MIDP_PLATFORM)
export MIDP_OUTPUT_DIR
USE_SSL			?= false
USE_RESTRICTED_CRYPTO	?= false
VERIFY_BUILD_ENV	?= 
#CONFIGURATION_OVERRIDE	?= MEASURE_STARTUP=true 
USE_QT_FB		?= false
USE_DIRECTFB		?= false
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
MIDP_SHARED_LIB		?= $(CVM_LIBDIR_ABS)/libmidp$(LIB_POSTFIX)
else
MIDP_CLASSES_ZIP	?= $(MIDP_OUTPUT_DIR)/classes.zip
endif

RUNMIDLET		?= $(MIDP_OUTPUT_DIR)/bin/$(TARGET_CPU)/runMidlet
MIDP_OBJECTS		?= $(MIDP_OUTPUT_DIR)/obj$(DEBUG_POSTFIX)/$(TARGET_CPU)/*.o
ifeq ($(CVM_PRELOAD_LIB), true)
CVM_OBJECTS		+= $(MIDP_OBJECTS)
MIDP_LIBS 		?= \
        -L$(PCSL_OUTPUT_DIR)/$(PCSL_TARGET)/lib -lpcsl_file \
        -lpcsl_memory -lpcsl_network -lpcsl_print -lpcsl_string
LINKLIBS 		+= $(MIDP_LIBS)
endif

-include $(MIDP_DEFS_JCC_MK)
ifeq ($(CVM_PRELOAD_LIB), true)
# Add MIDP classes to JCC input list so they can be romized.
CVM_JCC_CL_INPUT	+= -cl:midp $(MIDP_CLASSES_ZIP)

# Add MIDP CNI classes to CVM_CNI_CLASSES
CVM_CNI_CLASSES += $(MIDP_CNI_CLASSES)

endif

endif

