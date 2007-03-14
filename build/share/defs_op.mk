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

SUBSYSTEM_MAKE_FILE      = subsystem.gmk
JSR_INIT_PACKAGE         = com.sun.cdc.config
JSR_INIT_CLASS           = Initializer

JSROP_NUMBERS = 75 82 120 135 172 177 179 180 184 205 211 229 234 238 239

# Directory which JSRs *.jar and *.so files are put to
JSROP_LIB_DIR   = $(CVM_LIBDIR)

# Directory where JSRs build subdirectories are created
JSROP_BUILD_DIR = $(CVM_BUILD_TOP)

# Directory which JSRs object files are put to
JSROP_OBJ_DIR   = $(CVM_OBJDIR)

# Make a list of all JSR flags and their settings. It will look something like:
#  USR_JSR_75=true USE_JSR_82=false USE_JSR_120=true ...
JSROP_OP_FLAGS = $(foreach jsr_number,$(JSROP_NUMBERS),\
          USE_JSR_$(jsr_number)=$(USE_JSR_$(jsr_number)))

# Convert JSROP_OP_FLAGS into a list of JSR numbers that are enabled.
# This will give you a list something like:
#   75 120 184 ...
INCLUDED_JSROP_NUMBERS = $(patsubst USE_JSR_%=true,%,\
              $(filter %true, $(JSROP_OP_FLAGS)))

# Create a list of a JSR jar files we want to build.
JSROP_BUILD_JARS = $(foreach jsr_number,$(INCLUDED_JSROP_NUMBERS),\
           $(JSROP_LIB_DIR)/jsr$(jsr_number).jar)

# Variable which is passed to MIDP and blocks JSRs building from MIDP; looks like:
# USE_JSR_75=false USE_JSR_82=false USE_JSR_120=false ...
MIDP_JSROP_USE_FLAGS = $(foreach jsr_number,$(JSROP_NUMBERS),USE_JSR_$(jsr_number)=false)

# Hide all JSROPs from CDC by default
HIDE_ALL_JSRS ?= true

# Create a list of hidden JSR numbers, which should look like:
#   75 120 135 ...
ifeq ($(HIDE_ALL_JSRS),true)
# All included JSRs are hidden from CDC
HIDE_JSROP_NUMBERS = $(INCLUDED_JSROP_NUMBERS)
else
# Make a list of all JSR HIDE_JSR_<#> setting. It will look something like:
#   HIDE_JSR_75=true USE_JSR_82= USE_JSR_135=true ...
HIDE_JSROP_FLAGS = $(foreach jsr_number,$(JSROP_NUMBERS),\
                HIDE_JSR_$(jsr_number)=$(HIDE_JSR_$(jsr_number)))
# Extract the JSR numbers from HIDE_JSROP_FLAGS and form a list
# of hidden JSR numbers.
HIDE_JSROP_NUMBERS = $(patsubst HIDE_JSR_%=true,%,\
                $(filter %true, $(HIDE_JSROP_FLAGS)))
endif

# The list of JAR jar files we want to hide.
JSROP_HIDE_JARS = $(subst $(space),:,$(foreach jsr_number,$(HIDE_JSROP_NUMBERS),$(JSROP_LIB_DIR)/jsr$(jsr_number).jar))

# Jump API classpath
EMPTY =
ONESPACE = $(EMPTY) $(EMPTY)
JSROP_JUMP_API = $(subst $(ONESPACE),$(PS),$(JUMP_API_CLASSESZIP) $(JUMP_IMPL_CLASSESZIP))

# SecOP - CDC/FP Security Optional Package
ifeq ($(USE_SECOP),true)
SECOP_DIR ?= $(COMPONENTS_DIR)/secop
ifeq ($(wildcard $(SECOP_DIR)/build/share/$(SUBSYSTEM_MAKE_FILE)),)
$(error SECOP_DIR must point to the SecOP source directory: $(SECOP_DIR))
endif
include $(SECOP_DIR)/build/share/$(SUBSYSTEM_MAKE_FILE)
endif

# If any JSR is built include JSROP abstractions building
ifneq ($(INCLUDED_JSROP_NUMBERS),)
# Check Jump building
ifneq ($(CVM_INCLUDE_JUMP), true)
$(error JSR optional packages require Jump to be supported. CVM_INCLUDE_JUMP must be true.)
endif

export ABSTRACTIONS_DIR ?= $(COMPONENTS_DIR)/abstractions
ABSTRACTIONS_MAKE_FILE = $(ABSTRACTIONS_DIR)/build/$(SUBSYSTEM_MAKE_FILE)
ifeq ($(wildcard $(ABSTRACTIONS_MAKE_FILE)),)
$(error ABSTRACTIONS_DIR must point to a directory containing JSROP abstractions sources)
endif
include $(ABSTRACTIONS_MAKE_FILE)

JSROP_JARS=$(ABSTRACTIONS_JAR) $(JSROP_BUILD_JARS)
endif

# Include JSR 75
ifeq ($(USE_JSR_75), true)
export JSR_75_DIR ?= $(COMPONENTS_DIR)/jsr75
JSR_75_MAKE_FILE = $(JSR_75_DIR)/build/$(SUBSYSTEM_MAKE_FILE)
ifeq ($(wildcard $(JSR_75_MAKE_FILE)),)
$(error JSR_75_DIR must point to a directory containing JSR 75 sources)
endif
include $(JSR_75_MAKE_FILE)
endif

# Include JSR 82
ifeq ($(USE_JSR_82), true)
export JSR_82_DIR ?= $(COMPONENTS_DIR)/jsr82
JSR_82_MAKE_FILE = $(JSR_82_DIR)/build/$(SUBSYSTEM_MAKE_FILE)
ifeq ($(wildcard $(JSR_82_MAKE_FILE)),)
$(error JSR_82_DIR must point to a directory containing JSR 82 sources)
endif
include $(JSR_82_MAKE_FILE)
endif

# Include JSR 120
ifeq ($(USE_JSR_120), true)
export JSR_120_DIR ?= $(COMPONENTS_DIR)/jsr120
JSR_120_MAKE_FILE = $(JSR_120_DIR)/build/$(SUBSYSTEM_MAKE_FILE)
ifeq ($(wildcard $(JSR_120_MAKE_FILE)),)
$(error JSR_120_DIR must point to a directory containing JSR 120 sources)
endif
include $(JSR_120_MAKE_FILE)
endif

# Include JSR 135
ifeq ($(USE_JSR_135), true)
export JSR_135_DIR ?= $(COMPONENTS_DIR)/jsr135
JSR_135_MAKE_FILE = $(JSR_135_DIR)/build/$(SUBSYSTEM_MAKE_FILE)
ifeq ($(wildcard $(JSR_135_MAKE_FILE)),)
$(error JSR_135_DIR must point to a directory containing JSR 135 sources)
endif
include $(JSR_135_MAKE_FILE)
endif

# Include JSR 172
ifeq ($(USE_JSR_172), true)
export JSR_172_DIR ?= $(COMPONENTS_DIR)/jsr172
JSR_172_MAKE_FILE = $(JSR_172_DIR)/build/$(SUBSYSTEM_MAKE_FILE)
ifeq ($(wildcard $(JSR_172_MAKE_FILE)),)
$(error JSR_172_DIR must point to a directory containing JSR 172 sources)
endif
include $(JSR_172_MAKE_FILE)
endif

# Include JSR 177
ifeq ($(USE_JSR_177), true)
export JSR_177_DIR ?= $(COMPONENTS_DIR)/jsr177
JSR_177_MAKE_FILE = $(JSR_177_DIR)/build/$(SUBSYSTEM_MAKE_FILE)
ifeq ($(wildcard $(JSR_177_MAKE_FILE)),)
$(error JSR_177_DIR must point to a directory containing JSR 177 sources)
endif
include $(JSR_177_MAKE_FILE)
endif

# Include JSR 179
ifeq ($(USE_JSR_179), true)
export JSR_179_DIR ?= $(COMPONENTS_DIR)/jsr179
JSR_179_MAKE_FILE = $(JSR_179_DIR)/build/$(SUBSYSTEM_MAKE_FILE)
ifeq ($(wildcard $(JSR_179_MAKE_FILE)),)
$(error JSR_179_DIR must point to a directory containing JSR 179 sources)
endif
include $(JSR_179_MAKE_FILE)
endif

# Include JSR 180
ifeq ($(USE_JSR_180), true)
export JSR_180_DIR ?= $(COMPONENTS_DIR)/jsr180
JSR_180_MAKE_FILE = $(JSR_180_DIR)/build/$(SUBSYSTEM_MAKE_FILE)
ifeq ($(wildcard $(JSR_180_MAKE_FILE)),)
$(error JSR_180_DIR must point to a directory containing JSR 180 sources)
endif
include $(JSR_180_MAKE_FILE)
endif

# Include JSR 184
ifeq ($(USE_JSR_184), true)
export JSR_184_DIR ?= $(COMPONENTS_DIR)/jsr184
JSR_184_MAKE_FILE = $(JSR_184_DIR)/build/$(SUBSYSTEM_MAKE_FILE)
ifeq ($(wildcard $(JSR_184_MAKE_FILE)),)
$(error JSR_184_DIR must point to a directory containing JSR 184 sources)
endif
include $(JSR_184_MAKE_FILE)
endif

# Include JSR 205
ifeq ($(USE_JSR_205), true)
export JSR_205_DIR ?= $(COMPONENTS_DIR)/jsr205
JSR_205_MAKE_FILE = $(JSR_205_DIR)/build/$(SUBSYSTEM_MAKE_FILE)
ifeq ($(wildcard $(JSR_205_MAKE_FILE)),)
$(error JSR_205_DIR must point to a directory containing JSR 205 sources)
endif
include $(JSR_205_MAKE_FILE)
endif

# Include JSR 211
ifeq ($(USE_JSR_211), true)
export JSR_211_DIR ?= $(COMPONENTS_DIR)/jsr211
JSR_211_MAKE_FILE = $(JSR_211_DIR)/build/$(SUBSYSTEM_MAKE_FILE)
ifeq ($(wildcard $(JSR_211_MAKE_FILE)),)
$(error JSR_211_DIR must point to a directory containing JSR 211 sources)
endif
include $(JSR_211_MAKE_FILE)
endif

# Include JSR 229
ifeq ($(USE_JSR_229), true)
export JSR_229_DIR ?= $(COMPONENTS_DIR)/jsr229
JSR_229_MAKE_FILE = $(JSR_229_DIR)/build/$(SUBSYSTEM_MAKE_FILE)
ifeq ($(wildcard $(JSR_229_MAKE_FILE)),)
$(error JSR_229_DIR must point to a directory containing JSR 229 sources)
endif
include $(JSR_229_MAKE_FILE)
endif

# Include JSR 234
ifeq ($(USE_JSR_234), true)
export JSR_234_DIR ?= $(COMPONENTS_DIR)/jsr234
JSR_234_MAKE_FILE = $(JSR_234_DIR)/build/$(SUBSYSTEM_MAKE_FILE)
ifeq ($(wildcard $(JSR_234_MAKE_FILE)),)
$(error JSR_234_DIR must point to a directory containing JSR 234 sources)
endif
include $(JSR_234_MAKE_FILE)
endif

# Include JSR 238
ifeq ($(USE_JSR_238), true)
export JSR_238_DIR ?= $(COMPONENTS_DIR)/jsr238
JSR_238_MAKE_FILE = $(JSR_238_DIR)/build/$(SUBSYSTEM_MAKE_FILE)
ifeq ($(wildcard $(JSR_238_MAKE_FILE)),)
$(error JSR_238_DIR must point to a directory containing JSR 238 sources)
endif
include $(JSR_238_MAKE_FILE)
endif

# Include JSR 239
ifeq ($(USE_JSR_239), true)
export JSR_239_DIR ?= $(COMPONENTS_DIR)/jsr239
JSR_239_MAKE_FILE = $(JSR_239_DIR)/build/$(SUBSYSTEM_MAKE_FILE)
ifeq ($(wildcard $(JSR_239_MAKE_FILE)),)
$(error JSR_239_DIR must point to a directory containing JSR 239 sources)
endif
include $(JSR_239_MAKE_FILE)
endif

ifeq ($(CVM_INCLUDE_JAVACALL), true)
JAVACALL_TARGET=$(TARGET_OS)_$(TARGET_CPU_FAMILY)
# Check javacall makefile and include it
ifeq ($(JAVACALL_PROJECT_DIR),)
export JAVACALL_DIR ?= $(COMPONENTS_DIR)/javacall
JAVACALL_MAKE_FILE = $(JAVACALL_DIR)/configuration/phoneMEAdvanced/$(JAVACALL_TARGET)/module.gmk
else
JAVACALL_MAKE_FILE = $(JAVACALL_PROJECT_DIR)/configuration/tiburon/$(JAVACALL_TARGET)/module.gmk
endif
ifeq ($(wildcard $(JAVACALL_MAKE_FILE)),)
$(error JAVACALL_DIR must point to a directory containing javacall implementation sources)
endif
include $(JAVACALL_MAKE_FILE)
endif

CVM_INCLUDE_DIRS+= $(JSROP_INCLUDE_DIRS)

ifeq ($(CVM_PRELOAD_LIB), true)
CVM_JCC_INPUT   += $(JSROP_JARS)
CVM_CNI_CLASSES += $(JSROP_CNI_CLASSES)
CVM_OBJECTS     += $(JSROP_NATIVE_OBJS)
ifneq ($(JAVACALL_LINKLIBS),)
LINKLIBS_CVM    += $(JAVACALL_LINKLIBS) -L$(JSROP_LIB_DIR)
endif
else
CLASSLIB_DEPS   += $(JSROP_NATIVE_LIBS)
endif

# CVM_CDCFILTERCONFIG is a list of JSROP classes 
# that are hidden from CDC applications.
ifeq ($(CVM_DUAL_STACK), true)
CVM_CDCFILTERCONFIG = $(CVM_LIBDIR)/CDCRestrictedClasses.txt
else
CVM_CDCFILTERCONFIG =
endif
