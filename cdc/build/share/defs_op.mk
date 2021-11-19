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

SUBSYSTEM_MAKE_FILE      = subsystem.gmk
SUBSYSTEM_DEFS_FILE      = subsystem_defs.gmk
JSR_INIT_PACKAGE         = com.sun.cdc.config
JSR_INIT_CLASS           = Initializer

JSROP_NUMBERS = 75 82 120 135 172 177 179 180 184 205 211 226 229 234 238 239 256 280
#JSROP_NUMBERS = 75 82 120 135 172 177 179 180 184 205 211 229 234 238 239 256 280

# Defintion for path separator used in JSRs
PATHSEP        ?= $(PS)

# Directory which JSRs *.so files are put to
JSROP_LIB_DIR   = $(CVM_LIBDIR)

# Directory which JSRs *.jar files are put to
ifeq ($(CVM_CREATE_RTJAR),true)
JSROP_JAR_DIR	= $(CVM_RTJARS_DIR)
else
JSROP_JAR_DIR	= $(JSROP_LIB_DIR)
endif

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
JSROP_BUILD_JARS = $(filter-out $(JSROP_JAR_DIR)/jsr205.jar,$(foreach jsr_number,$(INCLUDED_JSROP_NUMBERS),\
           $(JSROP_JAR_DIR)/jsr$(jsr_number).jar))

# JSROP_AGENT_JARS - list (space sepd) of jars that should be romized with MIDP's class loader.
# These jars contain "agent" classes which are used for access via reflection.
# Must not have any public API. Invisible for midlets
JSROP_AGENT_JARS =

# Variable which is passed to MIDP and blocks JSRs building from MIDP; looks like:
# USE_JSR_75=false USE_JSR_82=false USE_JSR_120=false ...
MIDP_JSROP_USE_FLAGS = $(foreach jsr_number,$(JSROP_NUMBERS),USE_JSR_$(jsr_number)=false)
MIDP_JSROP_USE_FLAGS += USE_ABSTRACTIONS=false

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

# The list of JSR jar files we want to hide.
JSROP_HIDE_JARS = $(subst $(space),$(PS),$(filter-out $(JSROP_JAR_DIR)/jsr205.jar,$(foreach jsr_number,$(HIDE_JSROP_NUMBERS),$(JSROP_JAR_DIR)/jsr$(jsr_number).jar)))

# Jump API classpath
JSROP_JUMP_API = $(subst $(space),$(PS),$(JUMP_API_CLASSESZIP))

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
export ABSTRACTIONS_DIR ?= $(COMPONENTS_DIR)/abstractions

ifeq ($(PROJECT_ABSTRACTIONS_DIR),)
JSROP_ABSTR_DIR = ABSTRACTIONS_DIR
else
JSROP_ABSTR_DIR = PROJECT_ABSTRACTIONS_DIR
endif
ABSTRACTIONS_MAKE_FILE = $($(JSROP_ABSTR_DIR))/build/$(SUBSYSTEM_MAKE_FILE)
ifeq ($(wildcard $(ABSTRACTIONS_MAKE_FILE)),)
$(error $(JSROP_ABSTR_DIR) must point to a directory containing JSROP abstractions sources)
endif
include $(ABSTRACTIONS_MAKE_FILE)

JSROP_JARS=$(ABSTRACTIONS_JAR) $(JSROP_BUILD_JARS)
# abstractions required javacall types
CVM_INCLUDE_JAVACALL=true
endif

# Include JSR 75
ifeq ($(USE_JSR_75), true)
export JSR_75_DIR ?= $(COMPONENTS_DIR)/jsr75
JSR_75_MAKE_FILE = $(JSR_75_DIR)/build/cdc_share/$(SUBSYSTEM_DEFS_FILE)
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
JSR_120_DEFS_FILE = $(JSR_120_DIR)/build/cdc_share/$(SUBSYSTEM_DEFS_FILE)
ifeq ($(wildcard $(JSR_120_DEFS_FILE)),)
$(error JSR_120_DIR must point to a directory containing JSR 120 sources)
endif
include $(JSR_120_DEFS_FILE)
endif

# Include JSR 135
ifeq ($(USE_JSR_135), true)
export JSR_135_DIR ?= $(COMPONENTS_DIR)/jsr135
ifeq ($(PROJECT_JSR_135_DIR),)
JSROP_JSR135_DIR = JSR_135_DIR
else
JSROP_JSR135_DIR = PROJECT_JSR_135_DIR
endif
JSR_135_MAKE_FILE = $($(JSROP_JSR135_DIR))/build/cdc_share/$(SUBSYSTEM_MAKE_FILE)
ifeq ($(wildcard $(JSR_135_MAKE_FILE)),)
$(error $(JSROP_JSR135_DIR) must point to a directory containing JSR 135 sources)
endif
include $(JSR_135_MAKE_FILE)
endif

# Include JSR 172
ifeq ($(USE_JSR_172), true)
export JSR_172_DIR ?= $(COMPONENTS_DIR)/jsr172
JSR_172_DEFS_FILE = $(JSR_172_DIR)/build/$(SUBSYSTEM_DEFS_FILE)
ifeq ($(wildcard $(JSR_172_DEFS_FILE)),)
$(error JSR_172_DIR must point to a directory containing JSR 172 sources)
endif
include $(JSR_172_DEFS_FILE)
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
JSR_205_DEFS_FILE = $(JSR_205_DIR)/build/$(SUBSYSTEM_DEFS_FILE)
ifeq ($(wildcard $(JSR_205_DEFS_FILE)),)
$(error JSR_205_DIR must point to a directory containing JSR 205 sources)
endif
include $(JSR_205_DEFS_FILE)
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

# Include JSR 226
ifeq ($(USE_JSR_226), true)
export JSR_226_DIR ?= $(COMPONENTS_DIR)/jsr226
export JSR_226_PLATFORM_RELEASE ?= cdc_midp
JSR_226_MAKE_FILE = $(JSR_226_DIR)/perseus2/platform.releases/$(JSR_226_PLATFORM_RELEASE)/resources/build/$(SUBSYSTEM_MAKE_FILE) 
ifeq ($(wildcard $(JSR_226_MAKE_FILE)),)
$(error JSR_226_DIR must point to a directory containing JSR 226 sources)
endif
include $(JSR_226_MAKE_FILE)
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
JSR_234_MAKE_FILE = $(JSR_234_DIR)/build/cdc_share/$(SUBSYSTEM_MAKE_FILE)
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
ifeq ($(PROJECT_JSR_239_DIR),)
JSR_239_MAKE_FILE = $(JSR_239_DIR)/build/cdc_share/$(SUBSYSTEM_MAKE_FILE)
else
JSR_239_MAKE_FILE = $(PROJECT_JSR_239_DIR)/build/cdc_share/$(SUBSYSTEM_MAKE_FILE)
endif
ifeq ($(wildcard $(JSR_239_MAKE_FILE)),)
$(error JSR_239_DIR must point to a directory containing JSR 239 sources)
endif
include $(JSR_239_MAKE_FILE)
endif

# Include JSR 256
ifeq ($(USE_JSR_256), true)
export JSR_256_DIR ?= $(COMPONENTS_DIR)/jsr256
JSR_256_DEFS_FILE = $(JSR_256_DIR)/build/cdc_share/$(SUBSYSTEM_DEFS_FILE)
ifeq ($(wildcard $(JSR_256_DEFS_FILE)),)
$(error JSR_256_DIR must point to a directory containing JSR 256 sources)
endif
include $(JSR_256_DEFS_FILE)
endif

# Include JSR 280
ifeq ($(USE_JSR_280), true)
export JSR_280_DIR ?= $(COMPONENTS_DIR)/jsr280
JSR_280_DEFS_FILE = $(JSR_280_DIR)/build/cdc_share/$(SUBSYSTEM_DEFS_FILE)
ifeq ($(wildcard $(JSR_280_DEFS_FILE)),)
$(error JSR_280_DIR must point to a directory containing JSR 280 sources)
endif
include $(JSR_280_DEFS_FILE)
endif

ifeq ($(USE_XMLPARSER),true)
export XMLPARSER_DIR ?= $(COMPONENTS_DIR)/xmlparser
XMLPARSER_DEFS_FILE = $(XMLPARSER_DIR)/build/cdc_share/$(SUBSYSTEM_DEFS_FILE)
ifeq ($(wildcard $(XMLPARSER_DEFS_FILE)),)
$(error XMLPARSER_DIR must point to a directory containing xmlparser sources)
endif
include $(XMLPARSER_DEFS_FILE)
JSROP_JARS += $(XMLPARSER_JAR)
endif

# Include API Extensions
ifeq ($(USE_API_EXTENSIONS), true)
ifneq ($(PROJECT_API_EXTENSIONS_DIR),)
JSROP_API_EXT_DIR = PROJECT_API_EXTENSIONS_DIR
else
JSROP_API_EXT_DIR = API_EXTENSIONS_DIR
endif
API_EXTENSIONS_MAKE_FILE = $($(JSROP_API_EXT_DIR))/build/$(SUBSYSTEM_MAKE_FILE)
ifeq ($(wildcard $(API_EXTENSIONS_MAKE_FILE)),)
$(error $(JSROP_API_EXT_DIR) must point to a directory containing API Extensions sources)
endif
include $(API_EXTENSIONS_MAKE_FILE)
JSROP_JARS += $(API_EXTENSIONS_JAR)
endif

ifeq ($(CVM_INCLUDE_JAVACALL), true)
JAVACALL_TARGET?=$(TARGET_OS)_$(TARGET_CPU_FAMILY)
JAVACALL_FLAGS = $(JSROP_OP_FLAGS) USE_API_EXTENSIONS=$(USE_API_EXTENSIONS)
ifeq ($(USE_JAVACALL_EVENTS), true)
JAVACALL_FLAGS += USE_COMMON=true
ifeq ($(USE_JUMP), true)
JUMP_ANT_OPTIONS += -Djavacall.events.used=true
JUMP_SRCDIRS += \
	$(JUMP_SRCDIR)/share/impl/eventqueue/native
JUMP_OBJECTS += \
	jump_eventqueue_impl.o
JUMP_DEPENDENCIES += $(JAVACALL_LIBRARY)
endif
endif
# Check javacall makefile and include it
export JAVACALL_DIR ?= $(COMPONENTS_DIR)/javacall
ifeq ($(PROJECT_JAVACALL_DIR),)
JSROP_JC_DIR = JAVACALL_DIR
else
JSROP_JC_DIR = PROJECT_JAVACALL_DIR
endif

JAVACALL_MAKE_FILE ?= $($(JSROP_JC_DIR))/configuration/phoneMEAdvanced/$(JAVACALL_TARGET)/module.gmk
ifeq ($(wildcard $(JAVACALL_MAKE_FILE)),)
$(warning $(JAVACALL_MAKE_FILE) was not found)
$(error $(JSROP_JC_DIR) must point to a directory containing javacall implementation sources)
endif
include $(JAVACALL_MAKE_FILE)
endif

# The list of all used JSR jar files
JSROP_JARS_LIST = $(subst $(space),$(PS),$(JSROP_JARS) $(JSROP_EXTRA_JARS))

#Variable containing all JSROP components output dirs
JSROP_OUTPUT_DIRS = $(foreach jsr_number,$(JSROP_NUMBERS),\
          $(JSR_$(jsr_number)_BUILD_DIR)) $(JAVACALL_BUILD_DIR) \
          $(ABSTRACTIONS_BUILD_DIR)

CVM_INCLUDE_DIRS+= $(JSROP_INCLUDE_DIRS)

ifneq ($(JAVACALL_LINKLIBS),)
LINKCVM_LIBS    += $(call POSIX2HOST, $(JAVACALL_LINKLIBS))
endif

ifeq ($(CVM_PRELOAD_LIB), true)
CVM_JCC_INPUT   += $(JSROP_JARS)
CVM_CNI_CLASSES += $(JSROP_CNI_CLASSES)
JSROP_EXTRA_SEARCHPATH = $(CVM_JCC_INPUT)
else
JSROP_EXTRA_SEARCHPATH = $(CVM_BUILDTIME_CLASSESZIP) $(LIB_CLASSESJAR) \
                         $(JCE_JARFILE_BUILD)
endif

ifeq ($(CVM_STATICLINK_LIBS), true)
CVM_OBJECTS     += $(JSROP_NATIVE_OBJS)
else
CLASSLIB_DEPS   += $(JSROP_NATIVE_LIBS)
endif

ifeq ($(CVM_DUAL_STACK), true)
# JSR_CDCRESTRICTED_CLASSLIST is a list of JSROP classes 
# that are hidden from CDC applications.
JSR_CDCRESTRICTED_CLASSLIST = $(CVM_LIBDIR)/JSRRestrictedClasses.txt
# JSR_CDCRESTRICTED_CLASSLIST is a list of JSROP classes
# that are accessible from midlets. JSR_CDCRESTRICTED_CLASSLIST and
# JSR_CDCRESTRICTED_CLASSLIST don't always contain the same classes
# depending on which JSRs are hidden from CDC.
JSR_MIDPPERMITTED_CLASSLIST = $(CVM_BUILD_TOP)/.jsrmidppermittedclasses
# CVM_MIDPCLASSLIST_FILES are the files that contain MIDP permitted
# classes.
CVM_MIDPCLASSLIST_FILES += $(JSR_MIDPPERMITTED_CLASSLIST)
endif

ifneq ($(CVM_PRELOAD_LIB),true)
# Not romized, so add JSROP_JARS to the bootclasspath
ifneq ($(CVM_CREATE_RTJAR), true)
CVM_JARFILES += $(patsubst $(JSROP_JAR_DIR)/%,"%"$(comma),$(JSROP_JARS))
else
CVM_RTJARS_LIST += $(JSROP_JARS)
endif
endif

# Include JDBC, which can be downloaded using the following URL:
#    http://java.sun.com/products/jdbc/download.html#cdcfp
ifeq ($(USE_JDBC), true)
ifeq ($(J2ME_CLASSLIB),cdc)
$(error USE_JDBC=true requires at least J2ME_CLASSLIB=foundation)
endif
export JDBC_DIR ?= $(COMPONENTS_DIR)/jdbc
JDBC_DEFS_FILE = $(JDBC_DIR)/build/share/defs_jdbc_pkg.mk
JDBC_RULES_FILE = $(JDBC_DIR)/build/share/rules_jdbc_pkg.mk
ifeq ($(wildcard $(JDBC_DEFS_FILE)),)
$(error JDBC_DIR must point to a directory containing JDBC sources: $(JDBC_DIR))
endif
include $(JDBC_DEFS_FILE)
# fixup JDBC_PACKAGE_SRCPATH. $(JDBC_DIR)/build/share/defs_jdbc_pkg.mk uses it.
JDBC_PACKAGE_SRCPATH = $(JDBC_DIR)/src/share/jdbc/classes
endif

# Include RMI, which can be downloaded using the following URL:
#    http://www.sun.com/software/communitysource/j2me/rmiop/download.xml
ifeq ($(USE_RMI), true)
ifeq ($(J2ME_CLASSLIB),cdc)
$(error USE_RMI=true requires at least J2ME_CLASSLIB=foundation)
endif
export RMI_DIR ?= $(COMPONENTS_DIR)/rmi
RMI_DEFS_FILE = $(RMI_DIR)/build/share/defs_rmi_pkg.mk
RMI_RULES_FILE = $(RMI_DIR)/build/share/rules_rmi_pkg.mk
ifeq ($(wildcard $(RMI_DEFS_FILE)),)
$(error RMI_DIR must point to a directory containing RMI sources: $(RMI_DIR))
endif
include $(RMI_DEFS_FILE)
# fixup RMI_SRCDIR. $(RMI_DIR)/build/share/defs_rmi_pkg.mk uses it.
RMI_SRCDIR = $(RMI_DIR)/src/share/rmi/classes
endif
