#
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
# Common makefile defs
#

empty:=
comma:= ,
space:= $(empty) $(empty)

#
# Setup HOST_OS, HOST_CPU_FAMILY, and HOST_DEVICE. These are used
# to assist in locating the proper tools to use.
#

UNAME_OS	?= $(shell uname -s)
UNAME_OS	:= $(UNAME_OS)

# Solaris host support
ifeq ($(UNAME_OS), SunOS)
HOST_OS		?= solaris
HOST_CPU_FAMILY ?= $(shell uname -p)
ifeq ($(HOST_CPU_FAMILY), sparc)
HOST_DEVICE	?= sun
else
HOST_DEVICE	?= pc
endif
endif

# Linux  host support
ifeq ($(UNAME_OS), Linux)
HOST_OS 	?= linux
HOST_CPU_FAMILY ?= $(shell uname -m)
ifeq ($(wildcard /etc/redhat-release), /etc/redhat-release)
	HOST_DEVICE ?= redhat
endif
ifeq ($(wildcard /etc/SuSE-release), /etc/SuSE-release)
	HOST_DEVICE ?= SuSE
endif
LSB_RELEASE = $(shell lsb_release -i -s 2> /dev/null)
ifneq ($(LSB_RELEASE),)
	HOST_DEVICE ?= $(LSB_RELEASE)
endif

ifeq ($(HOST_DEVICE),)
	HOST_DEVICE ?= generic
endif
endif

# Darwin (MacOS X) host support
ifeq ($(UNAME_OS), Darwin)
HOST_OS 	?= darwin
HOST_CPU_FAMILY ?= $(shell uname -p)
ifeq ($(HOST_CPU_FAMILY), powerpc)
HOST_DEVICE	?= mac
else
HOST_DEVICE	?= pc
endif
endif

# Windows host support
ifeq ($(findstring CYGWIN, $(UNAME_OS)), CYGWIN)
HOST_OS 	?= win32
HOST_CPU_FAMILY ?= $(shell uname -m)
HOST_DEVICE	?= cygwin
USE_CYGWIN	?= true
endif

ifeq ($(UNAME_OS), Interix)
HOST_OS		?= win32
HOST_CPU_FAMILY ?= $(shell uname -m)
HOST_DEVICE	?= $(UNAME_OS)
TOOL_WHICH	?= PATH="$(PATH)" whence "$(1)"
USE_INTERIX	?= true
endif

HOST_CPU_FAMILY := $(HOST_CPU_FAMILY)

ifeq ($(HOST_OS),)
$(error Invalid host. "$(UNAME_OS)" not recognized.)
endif

TOOL_WHICH	?= PATH="$(PATH)" which "$(1)"

#
# By default we prefer SHELL=sh. Some of the makefile commands require
# an sh compatible shell. csh won't work. Most versions of sh, ksh, tcsh,
# and bash will work fine. On some hosts we default to something other than
# sh. See comments below to find out when and why.
#
# The "-e" option is added to the SHELL command so shell commands will
# exit if there is an error. If this isn't done, then sometimes the 
# makefile continues to execute after a shell command fails.
#
# Note that gnumake also adds "-c" to the command, so you end up with
# a command that looks like this:
#
#   sh -e -c "<command>"
#
# Some versions of sh don't support this and require you to use "-ec"
# instead of "-e -c", but there is no way to get gnumake to do this.
# In this case you should override SHELL to use some other shell, or just
# drop the -e argument.
# 
# ksh is not the default shell because it is not installed on all 
# systems, and some versions of it have problems with the long commands
# that the makefiles produce.
#
SHELL	= sh -e

#
# Use ksh on solaris, since the solaris sh doesn't support -e as a
# separate parameter.
#
ifeq ($(HOST_OS), solaris)
SHELL	= ksh -e
endif

#
# Use bash on win32, since the Cygwin sh doesn't work for us.
#
ifeq ($(HOST_DEVICE), cygwin)
SHELL	= bash -e
endif

ifeq ($(HOST_DEVICE), Interix)
SHELL	= ksh -e
endif

#
# Setup host and target platform names. Note that the naming coventions
# for each is different with respect to the order of the cpu, os, 
# and device parts.
#
CVM_HOST 	?= $(HOST_CPU_FAMILY)-$(HOST_DEVICE)-$(HOST_OS)
CVM_TARGET	= $(TARGET_OS)-$(TARGET_CPU_FAMILY)-$(TARGET_DEVICE)

# EVAL_SUPPORTED is true if this version of gnumake supports
# the eval function. 
$(eval EVAL_SUPPORTED=true)

# Set overriding values:

# Figure out if this is a CDC 1.0 source base or not
ifeq ($(wildcard $(CDC_DIR)/build/share/defs_zoneinfo.mk),)
	override CDC_10 = true
else
	override CDC_10 = false
endif

CVM_JVMDI               ?= false
# For backwards compatibility of sorts, we migrate CVM_JVMDI to CVM_JVMTI
CVM_JVMTI               ?= $(CVM_JVMDI)
CVM_JVMTI_ROM           ?= $(CVM_JVMTI)
CVM_JVMTI_IOVEC         ?= false

# We need to check this here because the CVM_JVMTI option overrides many
# others that follows through CVM_DEBUG:
ifeq ($(CVM_JVMTI), true)
	override CVM_DEBUG_CLASSINFO = true
ifeq ($(CVM_JVMTI_ROM), true)
	override CVM_JAVAC_DEBUG = true
endif 
	override CVM_AGENTLIB = true
	override CVM_XRUN = true
        override CVM_THREAD_SUSPENSION = true
endif

ifeq ($(CVM_JVMTI_IOVEC), true)
ifneq ($(findstring true,$(CVM_JVMTI)$(CVM_JVMPI)), true)
$(error CVM_JVMTI must be set to 'true' if CVM_JVMTI_IOVEC is 'true')
endif
endif

ifeq ($(CVM_JVMTI_ROM), true)
ifneq ($(CVM_JVMTI), true)
$(error CVM_JVMTI must be set to 'true' if CVM_JVMTI_ROM is 'true')
endif
	override CVM_JAVAC_DEBUG=true
endif

ifeq ($(CVM_CLASSLIB_JCOV), true)
        override CVM_JVMPI = true
        override CVM_JVMPI_TRACE_INSTRUCTION = true
endif

ifeq ($(CVM_JVMPI), true)
        override CVM_DEBUG_CLASSINFO = true
	override CVM_XRUN = true
        override CVM_THREAD_SUSPENSION = true
endif

# Set default options.
# NOTE: These options are officially supported and are documented in
#       docs/guide/build.html:

# CVM_DEBUG must be true or false, or the build will fail.
CVM_DEBUG		?= false
ifneq ($(CVM_DEBUG),true)
ifneq ($(CVM_DEBUG),false)
$(error CVM_DEBUG must be true or false: CVM_DEBUG=$(CVM_DEBUG))
endif
endif

CVM_TRACE		?= $(CVM_DEBUG)
ifeq ($(CVM_TRACE),true)
override CVM_DEBUG_DUMPSTACK	= true
endif
ifeq ($(CVM_VERIFY_HEAP),true)
override CVM_DEBUG_ASSERTS = true
endif

CVM_DEBUG_ASSERTS	?= $(CVM_DEBUG)
CVM_DEBUG_CLASSINFO	?= $(CVM_DEBUG)
CVM_DEBUG_DUMPSTACK	?= $(CVM_DEBUG)
CVM_DEBUG_STACKTRACES	?= true
CVM_INSPECTOR		?= $(CVM_DEBUG)
CVM_JAVAC_DEBUG		?= $(CVM_DEBUG)
CVM_VERIFY_HEAP		?= false
CVM_JIT                 ?= false
CVM_JVMTI_ROM		?= false
CVM_JVMPI               ?= false
CVM_JVMPI_TRACE_INSTRUCTION ?= $(CVM_JVMPI)
CVM_THREAD_SUSPENSION   ?= false
CVM_GPROF		?= false
CVM_GPROF_NO_CALLGRAPH  ?= true
CVM_GCOV		?= false
CVM_USE_CVM_MEMALIGN    ?= false

# CVM_USE_NATIVE_TOOLS is no longer settable by the user. Note that we
# don't give a warning if it was set in a file, because this does
# in fact happen when doing a make clean. It gets set when
# .previous.build.flags is included.
ifneq ($(CVM_USE_NATIVE_TOOLS),)
    ifneq ($(origin CVM_USE_NATIVE_TOOLS),file)
    $(warning Setting CVM_USE_NATIVE_TOOLS is no longer supported. \
	      Any value set will be overridden.)
    endif
endif
override CVM_USE_NATIVE_TOOLS = true

ifeq ($(CVM_DEBUG), true)
CVM_OPTIMIZED		?= false
else
CVM_OPTIMIZED		?= true
endif

CVM_SYMBOLS             ?= $(CVM_DEBUG)
CVM_PRODUCT             ?= premium

ifeq ($(CVM_PRELOAD_LIB), true)
	override CVM_CREATE_RTJAR = false
endif

# CVM_TERSEOUTPUT is now deprecated in favor of USE_VERBOSE_MAKE.
# They have opposite meanings. We look at CVM_TERSEOUTPUT here to set
# USE_VERBOSE_MAKE properly for backwards compatibility. This is the
# only place where CVM_TERSEOUTPUT can be checked
ifeq ($(CVM_TERSEOUTPUT),false)
USE_VERBOSE_MAKE	= true
else
USE_VERBOSE_MAKE	= false
endif

# %begin lvm
CVM_LVM                 ?= false
# %end lvm

CVM_CSTACKANALYSIS	?= false
CVM_TIMESTAMPING	?= true
CVM_INCLUDE_COMMCONNECTION ?= false
USE_MIDP	?= false
USE_JUMP	?= false
USE_GCI         ?= false

# Some makefiles still reference CVM_INCLUDE_MIDP and CVM_INCLUDE_JUMP,
# so give them proper values until they are cleaned up.
CVM_INCLUDE_MIDP ?= $(USE_MIDP)
CVM_INCLUDE_JUMP ?= $(USE_JUMP)

ifeq ($(USE_MIDP), true)
  override CVM_KNI        = true
  override CVM_DUAL_STACK = true
else
  CVM_KNI                 ?= false
  ifeq ($(USE_JUMP), true)
    override CVM_DUAL_STACK = true
  else
    CVM_DUAL_STACK          ?= false
  endif
endif

# We must use the split verifier for cldc/midp classes 
ifeq ($(CVM_DUAL_STACK), true)
    override CVM_SPLIT_VERIFY = true
else
    CVM_SPLIT_VERIFY ?= true
endif

# We must use J2ME_CLASSLIB=foundation in order to support CVM_DUAL_STACK=true
ifeq ($(CVM_DUAL_STACK), true)
ifeq ($(J2ME_CLASSLIB), cdc)
$(error Must use J2ME_CLASSLIB=foundation because CVM_DUAL_STACK=true requires foundation)
endif
endif


CVM_JIT_REGISTER_LOCALS	?= true
CVM_JIT_USE_FP_HARDWARE ?= false

# NOTE: These options are not officially supported:
# NOTE: CVM_INTERPRETER_LOOP can be set to "Split", "Aligned", or "Standard"

CVM_CLASSLOADING	?= true
CVM_REFLECT		?= true
CVM_SERIALIZATION	?= true
CVM_DYNAMIC_LINKING	?= true
CVM_TEST_GC             ?= false
CVM_TEST_GENERATION_GC  ?= false
CVM_INSTRUCTION_COUNTING?= false
CVM_NO_CODE_COMPACTION	?= false
CVM_INTERPRETER_LOOP    ?= Standard
CVM_XRUN		?= false
CVM_AGENTLIB		?= false
CVM_CLASSLIB_JCOV       ?= false
CVM_EMBEDDED_HOOK	?= false

CVM_TRACE_JIT           ?= $(CVM_TRACE)
CVM_JIT_ESTIMATE_COMPILATION_SPEED ?= false
CVM_CCM_COLLECT_STATS   ?= false
CVM_JIT_PROFILE  	?= false
CVM_JIT_DEBUG           ?= false

# mTASK
CVM_MTASK                ?= false

# AOT
CVM_AOT			?= false

# By default build  in the $(CVM_TARGET) directory
CVM_BUILD_SUBDIR  ?= false 

CVM_USE_MEM_MGR		?= false
CVM_MP_SAFE		?= false

CVM_CREATE_RTJAR	?= false

# AOT is only supported for Romized build.
ifeq ($(CVM_AOT), true)
	override CVM_PRELOAD_LIB = true
endif

ifneq ($(CVM_CLASSLOADING), true)
        override CVM_PRELOAD_LIB = true
endif

ifdef CVM_ALLOW_UNRESOLVED
    $(error Internal flag CVM_ALLOW_UNRESOLVED should not be set)
endif
ifdef CVM_PRELOAD_FULL_CLOSURE
    $(error Internal flag CVM_PRELOAD_FULL_CLOSURE should not be set)
endif
ifdef CVM_PRELOAD_ALL
    $(error Internal flag CVM_PRELOAD_ALL should not be set)
endif
ifdef CVM_PRELOAD_SET
    ifdef CVM_PRELOAD_TEST
        $(error Do not set both CVM_PRELOAD_SET and CVM_PRELOAD_TEST)
    endif
    ifdef CVM_PRELOAD_LIB
        $(error Do not set both CVM_PRELOAD_SET and CVM_PRELOAD_LIB)
    endif
    CVM_FLAGS += CVM_PRELOAD_SET
endif

ifdef CVM_PRELOAD_TEST
CVM_PRELOAD_LIB = $(CVM_PRELOAD_TEST)
endif

ifdef CVM_PRELOAD_LIB
    CVM_FLAGS += CVM_PRELOAD_LIB
    ifeq ($(CVM_PRELOAD_LIB), false)
        ifeq ($(CVM_PRELOAD_TEST), true)
            $(error CVM_PRELOAD_LIB=false is incompatible with \
                CVM_PRELOAD_TEST=true)
        else
            CVM_PRELOAD_SET = minfull
        endif
    else
        ifeq ($(CVM_PRELOAD_TEST), true)
            CVM_PRELOAD_SET = libtestfull
        else
            CVM_PRELOAD_SET = libfull
        endif
    endif
endif

CVM_PRELOAD_SET        ?= minfull
CVM_PRELOAD_ALL         = false

ifeq ($(patsubst %full,true,$(CVM_PRELOAD_SET)), true)
    CVM_PRELOAD_FULL_CLOSURE = true
    ifeq ($(patsubst lib%,true,$(CVM_PRELOAD_SET)), true)
        CVM_PRELOAD_ALL = true
    endif
else
    CVM_PRELOAD_FULL_CLOSURE = false
endif

ifeq ($(CVM_PRELOAD_SET), libtestfull)
    CVM_PRELOAD_TEST = true
endif

CVM_STATICLINK_LIBS   = $(CVM_PRELOAD_ALL)
override CVM_PRELOAD_LIB = $(CVM_PRELOAD_ALL)

ifeq ($(CVM_PRELOAD_ALL), true)
	override CVM_CREATE_RTJAR = false
endif

ifeq ($(CVM_AOT), true)
override CVM_JIT         = true
endif

# Turn all JIT tracing off if we don't have the jit:
ifneq ($(CVM_JIT), true)
override CVM_TRACE_JIT          = false
override CVM_JIT_COLLECT_STATS  = false
override CVM_JIT_ESTIMATE_COMPILATION_SPEED = false
override CVM_CCM_COLLECT_STATS  = false
override CVM_JIT_PROFILE        = false
override CVM_JIT_DEBUG          = false
endif

#
# prefix and postfix for shared libraries. These can be overriden
# by platform makefiles if they need to be different.
#
ifeq ($(CVM_DEBUG), true)
DEBUG_POSTFIX = _g
endif
LIB_PREFIX = lib
LIB_POSTFIX = $(DEBUG_POSTFIX).so

#
# All build directories relative to CVM_BUILD_TOP
#
CVM_BUILD_TOP := $(CDC_DEVICE_COMPONENT_DIR)/build/$(CVM_TARGET)/$(CVM_BUILD_SUBDIR_NAME)
CVM_LIBDIR    := $(CVM_BUILD_TOP)/lib

CVM_BUILD_TOP_ABS := $(call ABSPATH,$(CVM_BUILD_TOP))
CVM_LIBDIR_ABS    := $(CVM_BUILD_TOP_ABS)/lib

PROFILE_DIR       ?= $(CDC_DIR)

# locate the tools component
export TOOLS_DIR ?= $(COMPONENTS_DIR)/tools
ifeq ($(wildcard $(TOOLS_DIR)/tools.gmk),)
$(error TOOLS_DIR must point to the shared tools directory: $(TOOLS_DIR))
endif

# include tools component makefile. Don't do this until after HOST_OS is setup
TOOLS_OUTPUT_DIR=$(CVM_BUILD_TOP)/tools.output
include $(TOOLS_DIR)/tools.gmk

# Optional Package names
ifneq ($(strip $(OPT_PKGS)),)
  ifeq ($(OPT_PKGS), all)
    OPT_PKGS_DEFS_FILES := $(wildcard ../share/defs_*_pkg.mk)
    OPT_PKGS_LIST  := $(patsubst ../share/defs_%_pkg.mk,%,\
                        $(OPT_PKGS_DEFS_FILES))
    OPT_PKGS_NAME  := _$(subst $(space),_,$(strip $(OPT_PKGS_LIST)))
  else
    OPT_PKGS_LIST  := $(subst $(comma),$(space),$(OPT_PKGS))
    OPT_PKGS_NAME  := $(subst $(space),,_$(subst $(comma),_,$(OPT_PKGS)))
    OPT_PKGS_DEFS_FILES := $(foreach PKG,$(OPT_PKGS_LIST),\
      $(firstword $(value $(shell echo ${patsubst %,%_DIR,${PKG}} | \
      tr '[:lower:]' '[:upper:]')) ../..)/build/share/defs_$(PKG)_pkg.mk)
  endif
  OPT_PKGS_RULES_FILES := $(subst /defs_,/rules_,$(OPT_PKGS_DEFS_FILES))
  OPT_PKGS_ID_FILES := $(subst /defs_,/id_,$(OPT_PKGS_DEFS_FILES))
  # Add optional packages to the J2ME_PRODUCT_NAME
  J2ME_PRODUCT_NAME		+= $(OPT_PKGS_NAME)
endif

#
# Include id makefiles.
#
include $(PROFILE_DIR)/build/share/id_$(J2ME_CLASSLIB).mk
ifneq ($(J2ME_PLATFORM),)
-include $(PROFILE_DIR)/build/share/id_$(J2ME_PLATFORM).mk
endif 
# This is for identifying binary products, like Personal Profile for Zaurus
-include $(CDC_DEVICE_COMPONENT_DIR)/build/$(TARGET_OS)-$(TARGET_CPU_FAMILY)-$(TARGET_DEVICE)/id_$(J2ME_CLASSLIB).mk

ifeq ($(CVM_CREATE_RTJAR),true)
CVM_RT_JAR_NAME		= "rt.jar"
CVM_RT_JAR		= $(CVM_LIBDIR_ABS)/rt.jar
CVM_RTJARS_DIR		= $(CVM_BUILD_TOP_ABS)/rtjars
endif

#
# The version values referenced here are setup in the profile id.mk files.
#
J2ME_BUILD_RELEASE		= $(J2ME_BUILD_VERSION)
ifneq ($(J2ME_BUILD_RELEASE),)
 J2ME_BUILD_VERSION_STRING      = $(J2ME_BUILD_RELEASE)-$(J2ME_BUILD_ID)
else
 J2ME_BUILD_VERSION_STRING      = $(J2ME_BUILD_ID)
endif
ifneq ($(CVM_BUILD_VERSION),)
 ifeq ($(CVM_DONT_ADD_BUILD_ID), true)
  CVM_BUILD_VERSION_STRING	= $(CVM_BUILD_VERSION)
 else
  CVM_BUILD_RELEASE		= $(CVM_BUILD_VERSION)
  CVM_BUILD_VERSION_STRING	= $(CVM_BUILD_RELEASE)-$(CVM_BUILD_ID)
 endif
else
 CVM_BUILD_VERSION_STRING	= $(CVM_BUILD_ID)
endif

#
# System property settings (be sure to put values between double quotations)
# 
# Specifications related to the system properties are found at:
# - "J2SE 1.3 API spec (System.getProperties())" at:
#   http://java.sun.com/j2se/1.3/docs/api/java/lang/System.html#getProperties()
# - "JavaTM Product Versioning Specification"
#   http://java.sun.com/j2se/1.3/docs/guide/versioning/spec/VersioningTOC.html
#
#   used in src/share/native/java/lang/System.c
CVM_PROP_JAVA_VERSION		= "$(J2ME_BUILD_VERSION_STRING)"
CVM_PROP_JAVA_VENDOR		= "Sun Microsystems Inc."
CVM_PROP_JAVA_VENDOR_URL	= "http://java.sun.com/"
CVM_PROP_JAVA_VENDOR_URL_BUG	= "http://java.sun.com/cgi-bin/bugreport.cgi"
CVM_PROP_JAVA_SPEC_NAME		= "$(J2ME_PROFILE_NAME) Specification"
CVM_PROP_JAVA_SPEC_VERSION	= "$(J2ME_PROFILE_SPEC_VERSION)"
CVM_PROP_JAVA_SPEC_VENDOR	= "Sun Microsystems Inc."
CVM_PROP_JAVA_CLASS_VERSION	= "47.0"
CVM_PROP_COM_SUN_PACKAGE_SPEC_VERSION = "1.4.2"
#   used in src/share/javavm/runtime/jvm.c
CVM_PROP_JAVA_VM_NAME		= "$(CVM_BUILD_NAME)"
CVM_PROP_JAVA_VM_VERSION 	= "$(CVM_BUILD_VERSION_STRING)"
CVM_PROP_SUN_MISC_PRODUCT	= "$(J2ME_PRODUCT_NAME)"
ifeq ($(CVM_JIT), true)
CVM_PROP_JAVA_VM_INFO		= "mixed mode"
else
CVM_PROP_JAVA_VM_INFO		= "interpreter loop"
endif
CVM_PROP_JAVA_VM_VENDOR		= "Sun Microsystems Inc."
CVM_PROP_JAVA_VM_SPEC_NAME	= "Java Virtual Machine Specification"
CVM_PROP_JAVA_VM_SPEC_VERSION	= "1.0"
CVM_PROP_JAVA_VM_SPEC_VENDOR	= "Sun Microsystems Inc."

#   used in src/$(CVM_TARGET)/javavm/runtime/java_props_md.c
CVM_CLASSLIB_JAR_NAME	       ?= "$(J2ME_CLASSLIB)$(OPT_PKGS_NAME).jar"

#   used in src/share/javavm/runtime/utils.c
ifneq ($(CVM_PRELOAD_LIB),true)
ifneq ($(CVM_CREATE_RTJAR), true)
CVM_JARFILES	+= CVM_CLASSLIB_JAR_NAME,
else
CVM_JARFILES	+= $(CVM_RT_JAR_NAME),
CVM_RTJARS_LIST += $(LIB_CLASSESJAR)
endif
endif

ifneq ($(OPT_PKGS_ID_FILES),)
-include $(OPT_PKGS_ID_FILES)
endif

#   list of property settings that are included in $(CVM_BUILD_DEFS_H) file
CVM_BUILD_DEF_VARS += \
	CVM_PROP_JAVA_VERSION \
	CVM_PROP_JAVA_VENDOR \
	CVM_PROP_JAVA_VENDOR_URL \
	CVM_PROP_JAVA_VENDOR_URL_BUG \
	\
	CVM_PROP_JAVA_SPEC_NAME \
	CVM_PROP_JAVA_SPEC_VERSION \
	CVM_PROP_JAVA_SPEC_VENDOR \
	\
	CVM_PROP_JAVA_CLASS_VERSION \
	CVM_PROP_COM_SUN_PACKAGE_SPEC_VERSION \
	\
	CVM_PROP_JAVA_VM_NAME \
	CVM_PROP_JAVA_VM_VERSION \
	CVM_PROP_SUN_MISC_PRODUCT \
	CVM_PROP_JAVA_VM_INFO \
	CVM_PROP_JAVA_VM_VENDOR \
	\
	CVM_PROP_JAVA_VM_SPEC_NAME \
	CVM_PROP_JAVA_VM_SPEC_VERSION \
	CVM_PROP_JAVA_VM_SPEC_VENDOR \
	\
	CVM_CLASSLIB_JAR_NAME \
	CVM_JARFILES

#
# The directory and jar files which the library classes are going to
# be put into. Add in the optional package name.
#
LIB_CLASSESDIR	= $(CVM_BUILD_TOP)/$(J2ME_CLASSLIB)$(OPT_PKGS_NAME)_classes
ifneq ($(CVM_CREATE_RTJAR),true)
LIB_CLASSESJAR ?= $(CVM_LIBDIR_ABS)/$(J2ME_CLASSLIB)$(OPT_PKGS_NAME).jar
else
LIB_CLASSESJAR = $(CVM_RTJARS_DIR)/$(J2ME_CLASSLIB)$(OPT_PKGS_NAME).jar
endif

export CVM_RESOURCES_DIR ?= $(CVM_BUILD_TOP)/resources
export CVM_RESOURCES_JAR_FILENAME ?= resources.jar
export CVM_RESOURCES_JAR ?= $(CVM_LIBDIR_ABS)/$(CVM_RESOURCES_JAR_FILENAME)

#
# command line flags
#
ifeq ($(CVM_OPTIMIZED), true)
	CVM_DEFINES   += -DCVM_OPTIMIZED
endif
ifeq ($(CVM_DEBUG), true)
	CVM_DEFINES   += -DCVM_DEBUG
endif
ifeq ($(CVM_INSPECTOR), true)
	CVM_DEFINES      += -DCVM_INSPECTOR
	override CVM_DEBUG_DUMPSTACK = true
endif
ifeq ($(CVM_CSTACKANALYSIS), true)
        CVM_DEFINES   += -DCVM_CSTACKANALYSIS
endif
ifeq ($(CVM_JAVAC_DEBUG), true)
	JAVAC_OPTIONS += -g
else
	JAVAC_OPTIONS += -g:none
endif
ifeq ($(CVM_CLASSLIB_JCOV), true)
        CVM_DEFINES   += -DCVM_CLASSLIB_JCOV
	JAVAC_OPTIONS += -Xjcov
endif
ifeq ($(CVM_EMBEDDED_HOOK), true)
        CVM_DEFINES   += -DCVM_EMBEDDED_HOOK
endif
ifeq ($(CVM_DEBUG_CLASSINFO), true)
	CVM_DEFINES      += -DCVM_DEBUG_CLASSINFO
endif
ifeq ($(CVM_DEBUG_STACKTRACES), true)
	CVM_DEFINES      += -DCVM_DEBUG_STACKTRACES
endif
ifeq ($(CVM_DEBUG_DUMPSTACK), true)
	CVM_DEFINES      += -DCVM_DEBUG_DUMPSTACK
endif
ifeq ($(CVM_DEBUG_ASSERTS), true)
	CVM_DEFINES      += -DCVM_DEBUG_ASSERTS
else
	CVM_DEFINES	 += -DNDEBUG
endif
ifeq ($(CVM_VERIFY_HEAP), true)
	CVM_DEFINES      += -DCVM_VERIFY_HEAP
endif
ifeq ($(CVM_CLASSLOADING), true)
	CVM_DEFINES      += -DCVM_CLASSLOADING
	CVM_DYNAMIC_LINKING = true
endif

# If reflection is explicitly stated to be false by the user, don't
# allow serialization into the build to override that later
ifneq ($(CVM_REFLECT), true)
	override CVM_SERIALIZATION = false
endif
ifeq ($(CVM_SERIALIZATION), true)
	CVM_DEFINES      += -DCVM_SERIALIZATION
        override CVM_REFLECT = true
endif
ifeq ($(CVM_REFLECT), true)
	CVM_DEFINES      += -DCVM_REFLECT
endif
ifeq ($(CVM_XRUN), true)
	CVM_DEFINES      += -DCVM_XRUN
endif
ifeq ($(CVM_AGENTLIB), true)
	CVM_DEFINES      += -DCVM_AGENTLIB
endif
ifeq ($(CVM_JVMTI), true)
	CVM_DEFINES      += -DCVM_JVMTI
	CVM_DYNAMIC_LINKING = true
endif
ifeq ($(CVM_JVMPI), true)
        CVM_DEFINES      += -DCVM_JVMPI
	CVM_DYNAMIC_LINKING = true
else
        override CVM_JVMPI_TRACE_INSTRUCTION = false
endif
ifeq ($(CVM_JVMTI_IOVEC), true)
	CVM_DEFINES      += -DCVM_JVMTI_IOVEC
endif
ifeq ($(CVM_JVMPI_TRACE_INSTRUCTION), true)
        override CVM_NO_CODE_COMPACTION = true
        CVM_DEFINES      += -DCVM_JVMPI_TRACE_INSTRUCTION
endif
# NOTE: The CVM_THREAD_SUSPENSION option must only be checked after the JVMTI
# and JVMPI have been checked because those options can override it.
ifeq ($(CVM_THREAD_SUSPENSION), true)
	CVM_DEFINES      += -DCVM_THREAD_SUSPENSION
endif

ifeq ($(CVM_INSTRUCTION_COUNTING), true)
	CVM_DEFINES      += -DCVM_INSTRUCTION_COUNTING
endif
# make sure we check CVM_DYNAMIC_LINKING after checking CVM_JVMTI, CVM_JVMPI,
# and CVM_CLASSLOADING.
ifeq ($(CVM_DYNAMIC_LINKING), true)
	CVM_DEFINES      += -DCVM_DYNAMIC_LINKING
endif
ifeq ($(CVM_JIT), true)
	CVM_DEFINES   += -DCVM_JIT
endif
ifeq ($(CVM_JIT_USE_FP_HARDWARE), true)
	CVM_DEFINES   += -DCVM_JIT_USE_FP_HARDWARE
endif

ifeq ($(CVM_DUAL_STACK), true)
	CVM_DEFINES   += -DCVM_DUAL_STACK
endif
ifeq ($(CVM_SPLIT_VERIFY), true)
	CVM_DEFINES   += -DCVM_SPLIT_VERIFY
endif
ifeq ($(CVM_JIT_REGISTER_LOCALS), true)
	CVM_DEFINES   += -DCVM_JIT_REGISTER_LOCALS
endif

ifeq ($(CVM_USE_MEM_MGR), true)
	CVM_DEFINES   += -DCVM_USE_MEM_MGR
endif

ifeq ($(CVM_MP_SAFE), true)
	CVM_DEFINES   += -DCVM_MP_SAFE
endif

# %begin lvm
ifeq ($(CVM_LVM), true)
	CVM_DEFINES   += -DCVM_LVM -DCVM_REMOTE_EXCEPTIONS_SUPPORTED
endif
# %end lvm
ifeq ($(CVM_AOT), true)
	CVM_DEFINES   += -DCVM_AOT
endif
ifeq ($(CVM_TEST_GC), true)
        CVM_DEFINES   += -DCVM_TEST_GC
endif

# if CVM_INTERPRETER_LOOP is not defined to any supported option,
# use the default:
override CVM_INTERPRETER_LOOP := $(subst Loop,,$(CVM_INTERPRETER_LOOP))
ifneq ($(CVM_INTERPRETER_LOOP), Split)
ifneq ($(CVM_INTERPRETER_LOOP), Aligned)
ifneq ($(CVM_INTERPRETER_LOOP), Standard)
ifeq ($(CVM_INTERPRETER_LOOP), )
	CVM_INTERPRETER_LOOP = Standard
else
	invalid value for CVM_INTERPRETER_LOOP
endif
endif
endif
endif

ifeq ($(CVM_TIMESTAMPING), true)
	CVM_DEFINES += -DCVM_TIMESTAMPING
endif

ifeq ($(CVM_TRACE), true)
        CVM_DEFINES   += -DCVM_TRACE
        CVM_TRACE_ENABLED := true
endif
ifeq ($(CVM_PROFILE_METHOD), true)
        CVM_DEFINES   += -DCVM_PROFILE_METHOD
        CVM_TRACE_ENABLED := true
endif
ifeq ($(CVM_PROFILE_CALL), true)
        CVM_DEFINES   += -DCVM_PROFILE_CALL
        CVM_TRACE_ENABLED := true
endif

ifeq ($(CVM_TRACE_JIT), true)
        CVM_DEFINES   += -DCVM_TRACE_JIT
endif

ifeq ($(CVM_JIT_COLLECT_STATS), true)
        CVM_DEFINES   += -DCVM_JIT_COLLECT_STATS
endif

ifeq ($(CVM_JIT_ESTIMATE_COMPILATION_SPEED), true)
        CVM_DEFINES   += -DCVM_JIT_ESTIMATE_COMPILATION_SPEED
endif

ifeq ($(CVM_CCM_COLLECT_STATS), true)
        CVM_DEFINES   += -DCVM_CCM_COLLECT_STATS
endif

ifeq ($(CVM_JIT_PROFILE), true)
        CVM_DEFINES   += -DCVM_JIT_PROFILE
endif

ifeq ($(CVM_GPROF), true)
	CVM_DEFINES   += -DCVM_GPROF
endif

ifeq ($(CVM_JIT_DEBUG), true)
        CVM_DEFINES   += -DCVM_JIT_DEBUG
        CVM_DEFINES   += -DCVM_DEBUG_JIT_TRACE_CODEGEN_RULE_EXECUTION
endif

ifeq ($(CVM_PRELOAD_LIB), true)
	CVM_DEFINES   += -DCVM_PRELOAD_LIB
	CVM_BUILD_LIB_CLASSESJAR = false
else
	CVM_BUILD_LIB_CLASSESJAR = true
endif

ifeq ($(CVM_STATICLINK_LIBS), true)
	CVM_DEFINES   += -DCVM_STATICLINK_LIBS
endif

ifeq ($(CVM_STATICLINK_TOOLS), true)
	CVM_DEFINES   += -DCVM_STATICLINK_TOOLS
endif

# Keep ant quiet unless a verbose build is requested. Note, you can set
# CVM_ANT_OPTIONS=-v or CVM_ANT_OPTIONS=-d on the command line to make
# ant much more verbose.
ifneq ($(USE_VERBOSE_MAKE), true)
CVM_ANT_OPTIONS		+= -q
endif

# Set USE_VERBOSE_JAVAC=true to get a verbose dump on how javac is compiling
# the Java classes.
ifeq ($(USE_VERBOSE_JAVAC), true)
JAVAC_OPTIONS      	+= -verbose
endif

ifneq ($(CVM_DEBUG), true)
CVM_ANT_OPTIONS         += -Ddebug=false
endif

ifeq ($(CDC_10),true)
CVM_DEFINES += -DCDC_10
endif

CVM_DEFINES += -DJ2ME_CLASSLIB=$(J2ME_CLASSLIB)
CVM_DEFINES += -DTARGET_CPU_FAMILY=$(TARGET_CPU_FAMILY)

# The check for CVM_TRACE_ENABLED must come at the bottom because it is
# set based on other build options above.
ifeq ($(CVM_TRACE_ENABLED), true)
        CVM_DEFINES   += -DCVM_TRACE_ENABLED
endif

#
# All the build flags we need to keep track of in case they are toggled.
#
# WARNING: None of the cleanup actions should try to delete flags
# or the generated directory or problems will occur.
#
CVM_FLAGS += \
	CVM_HOST \
	CVM_SYMBOLS \
	CVM_OPTIMIZED \
	CVM_DEBUG \
	CVM_JAVAC_DEBUG \
	CVM_DEBUG_CLASSINFO \
	CVM_DEBUG_STACKTRACES \
	CVM_DEBUG_DUMPSTACK \
	CVM_INSPECTOR \
	CVM_DEBUG_ASSERTS \
	CVM_VERIFY_HEAP \
	CVM_CLASSLOADING \
	CVM_INSTRUCTION_COUNTING \
	CVM_GCCHOICE \
	CVM_NO_CODE_COMPACTION \
	CVM_XRUN \
	CVM_AGENTLIB \
	CVM_JVMTI \
	CVM_JVMTI_ROM \
	CVM_JVMTI_IOVEC \
	CVM_JVMPI \
	CVM_JVMPI_TRACE_INSTRUCTION \
	CVM_THREAD_SUSPENSION \
	CVM_CLASSLIB_JCOV \
	CVM_EMBEDDED_HOOK \
	CVM_REFLECT \
	CVM_SERIALIZATION \
	CVM_STATICLINK_LIBS \
	CVM_STATICLINK_TOOLS \
	CVM_DYNAMIC_LINKING \
	CVM_TEST_GC \
	CVM_TEST_GENERATION_GC \
	CVM_TIMESTAMPING \
	CVM_INCLUDE_COMMCONNECTION \
	USE_MIDP \
	USE_JUMP \
	USE_GCI \
	USE_CDC_COM \
	CVM_DUAL_STACK \
	CVM_SPLIT_VERIFY \
	CVM_KNI \
	CVM_JIT_REGISTER_LOCALS \
	CVM_INTERPRETER_LOOP \
	CVM_JIT \
	CVM_JIT_USE_FP_HARDWARE \
	J2ME_CLASSLIB	\
	CVM_CSTACKANALYSIS \
	CVM_TRACE \
	CVM_TRACE_JIT \
	CVM_JIT_COLLECT_STATS \
	CVM_JIT_ESTIMATE_COMPILATION_SPEED \
	CVM_CCM_COLLECT_STATS \
	CVM_JIT_PROFILE \
	CVM_JIT_DEBUG \
	OPT_PKGS \
        CVM_PRODUCT \
	CVM_GPROF \
	CVM_GPROF_NO_CALLGRAPH \
	CVM_GCOV \
	CVM_USE_CVM_MEMALIGN \
	CVM_USE_MEM_MGR \
	CVM_MP_SAFE \
	CVM_USE_NATIVE_TOOLS \
	CVM_MTASK \
	CVM_AOT \
	CVM_CREATE_RTJAR

# %begin lvm
CVM_FLAGS += \
	CVM_LVM
# %end lvm

CVM_DEFAULT_CLEANUP_ACTION 	= \
	rm -rf $(CVM_OBJDIR)      \
	rm -rf $(CVM_MIDP_BUILDDIR)
CVM_HOST_CLEANUP_ACTION 	= \
	rm -rf $(CVM_JCS_BUILDDIR)
CVM_JAVAC_DEBUG_CLEANUP_ACTION 	= \
	rm -rf $(CVM_BUILD_TOP)/.*classes \
	       $(CVM_BUILD_TOP)/*_classes $(CVM_BUILD_TOP)/.*.list \
	       $(CVM_BUILDTIME_CLASSESDIR) $(CVM_BUILDTIME_CLASSESZIP) \
	       $(CVM_TEST_CLASSESDIR) $(CVM_TEST_CLASSESZIP) \
	       $(CVM_DEMO_CLASSESDIR) $(CVM_DEMO_CLASSESJAR)
CVM_DEBUG_CLASSINFO_CLEANUP_ACTION  = \
	rm -rf $(CVM_OBJDIR) $(CVM_ROMJAVA_CPATTERN)*
CVM_CLASSLOADING_CLEANUP_ACTION     = \
	rm -rf $(CVM_OBJDIR) $(CVM_ROMJAVA_CPATTERN)* \
	       $(CVM_BUILDTIME_CLASSESZIP) \
		.buildtimeclasses
CVM_INSTRUCTION_COUNTING_CLEANUP_ACTION = \
        rm -f $(CVM_OBJDIR)/*opcodes.o $(CVM_OBJDIR)/executejava*.o \
	     $(CVM_OBJDIR)/jni_impl.o
CVM_GCCHOICE_CLEANUP_ACTION 	= \
	mkdir -p $(CVM_DERIVEDROOT)/javavm/include/; \
	echo \\\#include \"javavm/include/gc/$(CVM_GCCHOICE)/gc_config.h\" \
	 	 > $(CVM_DERIVEDROOT)/javavm/include/gc_config.h
CVM_NO_CODE_COMPACTION_CLEANUP_ACTION = \
	rm -rf $(CVM_ROMJAVA_CPATTERN)*

CVM_REFLECT_CLEANUP_ACTION = \
	$(CVM_JAVAC_DEBUG_CLEANUP_ACTION) \
	$(CVM_OBJDIR)/jni_impl.o $(CVM_OBJDIR)/jvm.o $(CVM_OBJDIR)/reflect.o \
	$(CVM_ROMJAVA_CPATTERN)*

CVM_SERIALIZATION_CLEANUP_ACTION = \
	$(CVM_JAVAC_DEBUG_CLEANUP_ACTION) \
	$(CVM_OBJDIR)/jvm.o $(CVM_ROMJAVA_CPATTERN)*

CVM_PRELOAD_SET_CLEANUP_ACTION = \
	rm -rf $(CVM_ROMJAVA_CPATTERN)* \
	$(DEFAULTLOCALELIST_JAVA) \
	$(CVM_BUILDTIME_CLASSESDIR) \
	$(CVM_BUILDTIME_CLASSESZIP) .buildtimeclasses \
	$(LIB_CLASSESJAR) $(LIB_CLASSESDIR)

CVM_STATICLINK_LIBS_CLEANUP_ACTION = \
	rm -rf $(CVM_LIBDIR) $(CVM_BINDIR)

CVM_STATICLINK_TOOLS_CLEANUP_ACTION = \
	rm -rf $(CVM_LIBDIR) $(CVM_BINDIR)

CVM_DYNAMIC_LINKING_CLEANUP_ACTION = \
	rm -f $(CVM_OBJDIR)/jvm.o $(CVM_OBJDIR)/jni_impl.o \
		$(CVM_OBJDIR)/linker_md.o $(CVM_OBJDIR)/common_exceptions.o

CVM_INTERPRETER_LOOP_CLEANUP_ACTION = \
        rm -f $(CVM_OBJDIR)/executejava*.o

CVM_CSTACKANALYSIS_CLEANUP_ACTION = \
	rm -rf $(CVM_OBJDIR)/*.asm $(CVM_OBJDIR)/*.o

CVM_JIT_CLEANUP_ACTION = \
	$(CVM_DEFAULT_CLEANUP_ACTION)   \
	rm -rf $(CVM_ROMJAVA_CPATTERN)* \
	       $(CVM_DERIVEDROOT)/javavm/runtime/jit/* \
	       $(CVM_DERIVEDROOT)/javavm/include/jit/* \
	       $(CVM_BUILDTIME_CLASSESZIP) \
	       .buildtimeclasses \
	       $(CVM_BUILDTIME_CLASSESDIR) \
	       $(CVM_TEST_CLASSESDIR)

# %begin lvm
CVM_LVM_CLEANUP_ACTION = \
	rm -rf $(CVM_OBJDIR) $(CVM_ROMJAVA_CPATTERN)* \
	       $(CVM_BUILDTIME_CLASSESZIP) \
	       .buildtimeclasses \
	       $(CVM_BUILDTIME_CLASSESDIR)/sun/misc/*LogicalVM*.class \
	       $(CVM_BUILDTIME_CLASSESDIR)/sun/misc/*Isolate*.class \
	       $(CVM_TEST_CLASSESDIR)/lvmtest \
	       $(CVM_TEST_CLASSESZIP)
# %end lvm

CVM_DEBUG_ASSERTS_CLEANUP_ACTION = \
	rm -rf $(BUILDFLAGS_JAVA) \
		$(CVM_JAVAC_DEBUG_CLEANUP_ACTION) \
		$(CVM_DEFAULT_CLEANUP_ACTION) 

CVM_DEBUG_CLEANUP_ACTION 		= $(CVM_DEFAULT_CLEANUP_ACTION)
CVM_OPTIMIZED_CLEANUP_ACTION 		= $(CVM_DEFAULT_CLEANUP_ACTION)
CVM_SYMBOLS_CLEANUP_ACTION 		= $(CVM_DEFAULT_CLEANUP_ACTION)
CVM_DEBUG_STACKTRACES_CLEANUP_ACTION 	= $(CVM_DEFAULT_CLEANUP_ACTION)
CVM_DEBUG_DUMPSTACK_CLEANUP_ACTION 	= $(CVM_DEFAULT_CLEANUP_ACTION)
CVM_INSPECTOR_CLEANUP_ACTION	 	= \
	$(CVM_JAVAC_DEBUG_CLEANUP_ACTION) \
	$(CVM_DEBUG_CLASSINFO_CLEANUP_ACTION)
CVM_VERIFY_HEAP_CLEANUP_ACTION 		= $(CVM_DEFAULT_CLEANUP_ACTION)
CVM_XRUN_CLEANUP_ACTION			= $(CVM_DEFAULT_CLEANUP_ACTION)
CVM_AGENTLIB_CLEANUP_ACTION		= $(CVM_DEFAULT_CLEANUP_ACTION)
CVM_JVMTI_CLEANUP_ACTION		= $(CVM_DEFAULT_CLEANUP_ACTION)
CVM_JVMTI_ROM_CLEANUP_ACTION		= $(CVM_DEFAULT_CLEANUP_ACTION)
CVM_JVMTI_IOVEC_CLEANUP_ACTION		= $(CVM_DEFAULT_CLEANUP_ACTION)
CVM_JVMPI_CLEANUP_ACTION                = \
        $(CVM_DEFAULT_CLEANUP_ACTION)     \
        $(CVM_DEBUG_CLASSINFO_CLEANUP_ACTION)
CVM_JVMPI_TRACE_INSTRUCTION_CLEANUP_ACTION = $(CVM_JVMPI_CLEANUP_ACTION)
CVM_THREAD_SUSPENSION_CLEANUP_ACTION	= $(CVM_DEFAULT_CLEANUP_ACTION)
CVM_TEST_GC_CLEANUP_ACTION              = $(CVM_DEFAULT_CLEANUP_ACTION)
CVM_TEST_GENERATION_GC_CLEANUP_ACTION   = $(CVM_DEFAULT_CLEANUP_ACTION)

CVM_TIMESTAMPING_CLEANUP_ACTION         = \
	rm -rf $(CVM_OBJDIR) $(CVM_ROMJAVA_CPATTERN)* \
	       $(CVM_OBJDIR)/libromjava.a \
	       $(CVM_BUILDTIME_CLASSESZIP) \
	       .buildtimeclasses \
	       $(CVM_BUILDTIME_CLASSESDIR)/sun/misc/TimeStamps.class \
	       $(CVM_TEST_CLASSESDIR)/TimeStampsTest.class

CVM_CLASSLIB_JCOV_CLEANUP_ACTION		= \
	$(CVM_DEFAULT_CLEANUP_ACTION) 	   	  \
	$(CVM_JAVAC_DEBUG_CLEANUP_ACTION)

CVM_EMBEDDED_HOOK_CLEANUP_ACTION		= \
	$(CVM_DEFAULT_CLEANUP_ACTION) 	   	  \
	$(CVM_JAVAC_DEBUG_CLEANUP_ACTION)

CVM_TRACE_CLEANUP_ACTION               = $(CVM_DEFAULT_CLEANUP_ACTION)
CVM_TRACE_JIT_CLEANUP_ACTION           = $(CVM_DEFAULT_CLEANUP_ACTION)
CVM_INCLUDE_COMMCONNECTION_CLEANUP_ACTION        = \
	$(CVM_DEFAULT_CLEANUP_ACTION)    \
	$(CVM_JAVAC_DEBUG_CLEANUP_ACTION)
USE_MIDP_CLEANUP_ACTION        = \
	$(CVM_DEFAULT_CLEANUP_ACTION)    \
	$(CVM_JAVAC_DEBUG_CLEANUP_ACTION)
USE_JUMP_CLEANUP_ACTION        = \
	$(CVM_DEFAULT_CLEANUP_ACTION)    \
	$(CVM_JAVAC_DEBUG_CLEANUP_ACTION)
USE_GCI_CLEANUP_ACTION         = \
        $(CVM_DEFAULT_CLEANUP_ACTION)    \
        $(CVM_JAVAC_DEBUG_CLEANUP_ACTION)
CVM_DUAL_STACK_CLEANUP_ACTION          = $(CVM_DEFAULT_CLEANUP_ACTION)
CVM_SPLIT_VERIFY_CLEANUP_ACTION        = $(CVM_DEFAULT_CLEANUP_ACTION)
CVM_KNI_CLEANUP_ACTION                 = $(CVM_DEFAULT_CLEANUP_ACTION)
CVM_JIT_REGISTER_LOCALS_CLEANUP_ACTION = $(CVM_JIT_CLEANUP_ACTION)
CVM_JIT_COLLECT_STATS_CLEANUP_ACTION   = $(CVM_JIT_CLEANUP_ACTION)
CVM_JIT_ESTIMATE_COMPILATION_SPEED_CLEANUP_ACTION = $(CVM_JIT_CLEANUP_ACTION)
CVM_CCM_COLLECT_STATS_CLEANUP_ACTION   = $(CVM_JIT_CLEANUP_ACTION)
CVM_JIT_PROFILE_CLEANUP_ACTION	       = $(CVM_JIT_CLEANUP_ACTION)
CVM_JIT_DEBUG_CLEANUP_ACTION = \
        rm -rf $(CVM_OBJDIR)/jit* $(CVM_OBJDIR)/ccm*  \
	       $(CVM_DERIVEDROOT)/javavm/runtime/jit/* \
	       $(CVM_DERIVEDROOT)/javavm/include/jit/*
CVM_JIT_USE_FP_HARDWARE_CLEANUP_ACTION = $(CVM_JIT_CLEANUP_ACTION)

OPT_PKGS_CLEANUP_ACTION                = $(J2ME_CLASSLIB_CLEANUP_ACTION)
CVM_PRODUCT_CLEANUP_ACTION             = rm -f $(CVM_OBJDIR)/jvm.o \
                                         rm -f $(INSTALLDIR)/src.zip
CVM_GPROF_CLEANUP_ACTION 	       = $(CVM_DEFAULT_CLEANUP_ACTION)
CVM_GPROF_NO_CALLGRAPH_CLEANUP_ACTION  = $(CVM_DEFAULT_CLEANUP_ACTION)
CVM_GCOV_CLEANUP_ACTION 	       = $(CVM_DEFAULT_CLEANUP_ACTION)
CVM_USE_NATIVE_TOOLS_CLEANUP_ACTION    = $(CVM_DEFAULT_CLEANUP_ACTION)
CVM_USE_CVM_MEMALIGN_CLEANUP_ACTION    = $(CVM_DEFAULT_CLEANUP_ACTION)
CVM_USE_MEM_MGR_CLEANUP_ACTION         = $(CVM_DEFAULT_CLEANUP_ACTION)
CVM_MP_SAFE_CLEANUP_ACTION	       = $(CVM_DEFAULT_CLEANUP_ACTION)
CVM_MTASK_CLEANUP_ACTION               = $(CVM_DEFAULT_CLEANUP_ACTION)
CVM_AOT_CLEANUP_ACTION                 = $(CVM_DEFAULT_CLEANUP_ACTION)
CVM_CREATE_RTJAR_CLEANUP_ACTION	       = rm -rf $(CVM_RT_JAR)

#
# Wipe out objects and classes when J2ME_CLASSLIB changes.
#
J2ME_CLASSLIB_CLEANUP_ACTION            = \
	$(CVM_JAVAC_DEBUG_CLEANUP_ACTION) ; \
	$(CVM_DEFAULT_CLEANUP_ACTION) ; \
	rm -rf $(CVM_LIBDIR)

# generate header dependency files
GENERATEMAKEFILES = true

# Use the original generational GC as default.
# Other possible GC choices are:
#	semispace
# 	marksweep
#       generational-seg
#
CVM_GCCHOICE   ?= generational

#
# And by default, do not use a segmented heap for generational GC
#
CVM_GC_SEGMENTED_HEAP = false

ifeq ($(CVM_TEST_GC), true)
        override CVM_GCCHOICE   = semispace
	override CVM_GC_SEGMENTED_HEAP = false
endif

#
# There should be a GC-specific makefile.
# This special-case will do for now.
#
ifeq ($(CVM_GCCHOICE), generational)
    CVM_SHAREOBJS_SPEED += \
        gen_semispace.o \
	gen_markcompact.o
    override CVM_GC_SEGMENTED_HEAP=false
endif

ifeq ($(CVM_GCCHOICE), generational-seg)
    CVM_SHAREOBJS_SPEED += \
	gen_segment.o \
        gen_eden.o \
        gen_edenspill.o \
	gen_markcompact.o
    override CVM_GC_SEGMENTED_HEAP=true
endif

ifeq ($(CVM_GC_SEGMENTED_HEAP), true)
    CVM_DEFINES   += -DCVM_SEGMENTED_HEAP
endif

ifeq ($(CVM_USE_CVM_MEMALIGN), true)
    CVM_SHAREOBJS_SPACE += \
        memory_aligned.o
    CVM_DEFINES   += -DCVM_USE_CVM_MEMALIGN
endif

#
# We will be including the TimeStampsTest when the 
# the CVM is build with CVM_TIMESTAMPING=true
#
ifeq ($(CVM_TIMESTAMPING), true)
    CVM_BUILDTIME_CLASSES += \
        sun.misc.TimeStamps
    CVM_SHAREOBJS_SPACE += \
	TimeStamps.o \
	timestamp.o
    CVM_TEST_CLASSES += \
	TimeStampsTest
endif

#
# Object and data files needed for dual stack support
#
ifeq ($(CVM_DUAL_STACK), true)
ifeq ($(USE_MIDP), true)
    # The MIDP version include all CLDC classes plus 8 additional
    # javax/micro/microeditional/io/* classes.
    CVM_MIDPDIR           = $(CVM_TOP)/src/share/lib/dualstack/midp
else
    CVM_MIDPDIR           = $(CVM_TOP)/src/share/lib/dualstack/cldc
endif
    CVM_SHAREOBJS_SPACE  += \
	MemberFilter.o      \
	$(CVM_ROM_MEMBER_FILTER)
    CVM_MIDPFILTERCONFIG  = $(CVM_LIBDIR)/MIDPFilterConfig.txt
    CVM_MIDPCLASSLISTNAME = MIDPPermittedClasses.txt
    CVM_MIDPCLASSLIST     = $(CVM_LIBDIR)/$(CVM_MIDPCLASSLISTNAME)
    CVM_MIDPCLASSLIST_FILES += $(CVM_MIDPDIR)/MIDPPermittedClasses.txt
    CVM_MIDPFILTERCONFIGINPUT = $(CVM_MIDPDIR)/MIDPFilterConfig.txt

#
# JavaAPILister related defs for generating dualstack
# filter
#
ifneq ($(CVM_MIDPFILTERINPUT),)
# CVM_MIDPFILTERINPUT should be jar files that contains classes.
# The jar files are the input of JavaAPILister. The output of  
# JavaAPILister are:
# - romjavaMemberFilterData.c (ROMized member filter data)
# - a list of visible class and their members (mout)
# - a list of visible classes (cout)
CVM_JCC_APILISTER_OPTIONS	+= -listapi:include=java/*,include=javax/*,input=$(CVM_MIDPFILTERINPUT),mout=$(CVM_MIDPFILTERCONFIG),cout=$(CVM_MIDPCLASSLIST)
else
# if CVM_MIDPFILTERINPUT is not set, we use the 'mout' output file
# as the input. 'mout' is generated by JavaAPILister when input
# jar files are given. 'mout' contains a list of visible classes 
# and their members. JavaAPILister parse the input list and creates
# romjavaMemberFilterData.c, which contains the ROMized member 
# filter data.
CVM_JCC_APILISTER_OPTIONS	+= -listapi:minput=$(CVM_MIDPFILTERCONFIGINPUT)
endif

# MIDP package checker class
MIDP_PKG_CHECKER = MIDPPkgChecker.java
GENERATED_CLASSES + = sun.misc.MIDPPkgChecker

CVM_BUILDTIME_CLASSES_min += \
	sun.misc.MIDPPkgChecker

# The rom.config file used to generate the MIDPPkgChecker class
ROMGEN_INCLUDE_PATHS += \
	$(CDC_DIR)/build/share
ROMGEN_CFG_FILES += rom.config \
	cdc_rom.cfg
endif

#
# Object needed for split (CLDC) verifier support
#
ifeq ($(CVM_SPLIT_VERIFY), true)
    CVM_SHAREOBJS_SPACE += \
	split_verify.o
endif

#
# Stuff needed for KNI support
#
ifeq ($(CVM_KNI), true)
    CVM_SHAREOBJS_SPACE += \
	kni_impl.o \
	sni_impl.o
    CVM_TEST_CLASSES += KNITest
ifeq ($(CVM_PRELOAD_TEST),true)
    CVM_SHAREOBJS_SPACE += \
	KNITest.o
    CVM_SRCDIRS += $(CVM_TESTCLASSES_SRCDIR)
    CVM_CNI_CLASSES += KNITest
endif
endif

#
# Directories
#
CVM_JCSDIR               = $(CVM_BUILD_TOP)/jcs
CVM_OBJDIR               := $(call abs2rel,$(CVM_BUILD_TOP)/obj)
CVM_BINDIR               = $(CVM_BUILD_TOP)/bin
CVM_DERIVEDROOT          = $(CVM_BUILD_TOP)/generated
CVM_BUILDTIME_CLASSESDIR = $(CVM_BUILD_TOP)/btclasses
CVM_JSR_CLASSESDIR 	 = $(CVM_BUILD_TOP)/jsrclasses
CVM_TEST_CLASSESDIR      = $(CVM_BUILD_TOP)/testclasses
CVM_DEMO_CLASSESDIR	 = $(CVM_BUILD_TOP)/democlasses
CVM_SHAREROOT  		 = $(CVM_TOP)/src/share

# Full path for current build directory
CDC_CUR_DIR	:= $(call ABSPATH,.)
# directory where cdc build is located.
export CDC_DIST_DIR := $(CVM_BUILD_TOP_ABS)
# Directory where javadocs, source bundles, and binary bundle get installed.
INSTALLDIR	:= $(CVM_TOP_ABS)/install

#
# Full path name for Binary Bundle
#
TM = (TM)
BUNDLE_PRODUCT_NAME0 = $(subst $(space),_,$(J2ME_PROFILE_NAME))
BUNDLE_PRODUCT_NAME1 = $(subst /,_,$(BUNDLE_PRODUCT_NAME0))
BUNDLE_PRODUCT_NAME  = $(subst $(TM),$(empty),$(BUNDLE_PRODUCT_NAME1))

BUNDLE_VERSION	= $(subst -,_,$(J2ME_BUILD_VERSION_STRING))
BUNDLE_TARGET	= $(subst -,_,$(CVM_TARGET))

BINARY_BUNDLE_NAME	= \
	$(BUNDLE_PRODUCT_NAME)-$(BUNDLE_VERSION)-$(BUNDLE_TARGET)-bin
BINARY_BUNDLE_DIRNAME	= $(BINARY_BUNDLE_NAME)

DEVICE_BUNDLE_NAME	= \
	$(BUNDLE_PRODUCT_NAME)-$(BUNDLE_VERSION)-$(BUNDLE_TARGET)-dev
DEVICE_BUNDLE_DIRNAME	= $(DEVICE_BUNDLE_NAME)

JAVAME_LEGAL_DIR ?= $(COMPONENTS_DIR)/legal

# Add the svn revison number to BINARY_BUNDLE_NAME and BINARY_BUNDLE_DIRNAME
# if requested.
BINARY_BUNDLE_APPEND_REVISION ?= true
ifeq ($(BINARY_BUNDLE_APPEND_REVISION),true)
ifneq ($(wildcard .svn),.svn)
REVISION_NUMBER = UNKNOWN
else
REVISION_NUMBER = \
     $(shell svn info | grep "Revision:" | sed -e 's/Revision: \(.*\)/\1/')
endif
# We need to set BINARY_BUNDLE_DIRNAME first since it usually depends on
# BINARY_BUNDLE_NAME, and we don't want the revision number in there twice.
override BINARY_BUNDLE_DIRNAME := $(BINARY_BUNDLE_DIRNAME)-rev$(REVISION_NUMBER)
override BINARY_BUNDLE_NAME := $(BINARY_BUNDLE_NAME)-rev$(REVISION_NUMBER)
override DEVICE_BUNDLE_DIRNAME := $(DEVICE_BUNDLE_DIRNAME)-rev$(REVISION_NUMBER)
override DEVICE_BUNDLE_NAME := $(DEVICE_BUNDLE_NAME)-rev$(REVISION_NUMBER)
endif


#
# Java source directories. 
#

CVM_TESTCLASSES_SRCDIR    = $(CVM_SHAREROOT)/javavm/test
CVM_CLDCCLASSES_SRCDIR    = $(CVM_SHAREROOT)/classes/cldc
CVM_VMIMPLCLASSES_SRCDIR  = $(CVM_SHAREROOT)/javavm/classes
CVM_SHAREDCLASSES_SRCDIR  = $(CVM_SHAREROOT)/classes
CVM_TARGETCLASSES_SRCDIR  = $(CVM_TARGETROOT)/classes

CVM_BUILDTIME_CLASSESZIP = $(CVM_BUILD_TOP_ABS)/btclasses.zip
CVM_TEST_CLASSESZIP      = $(CVM_BUILD_TOP_ABS)/testclasses.zip
CVM_DEMO_CLASSESJAR	 = $(CVM_BUILD_TOP_ABS)/democlasses.jar

# Security properties and policy files
DO_SECURITY_PROVIDER_FILTERING = false
CVM_PROPS_SRC   = $(CVM_TOP)/src/share/lib/security/java.security
CVM_PROPS_BUILD = $(CVM_LIBDIR)/security/java.security
CVM_POLICY_BUILD  = $(CVM_LIBDIR)/security/java.policy

# Build option record file to generate
CVM_BUILD_DEFS_H = $(CVM_DERIVEDROOT)/javavm/include/build_defs.h
CVM_BUILD_DEFS_MK= $(CVM_DERIVEDROOT)/build_defs.mk

# sun.misc.DefaultLoacleList.java
DEFAULTLOCALELIST_JAVA = \
    $(CVM_DERIVEDROOT)/classes/sun/misc/DefaultLocaleList.java

ifeq ($(CVM_TEST_GC), true)
include $(CDC_DIR)/build/share/testgc.mk
endif

ifeq ($(CVM_TEST_GENERATION_GC), true)
    CVM_SRCDIRS += \
	$(CVM_SHAREROOT)/javavm/test/GenerationGCTest/Csrc 
    CVM_INCLUDE_DIRS  += \
	$(CVM_SHAREROOT)/javavm/test/GenerationGCTest/Include 
    CVM_SHAREOBJS_SPACE += \
	BarrierTest.o
endif

CVM_SRCDIRS    += \
	$(CVM_SHAREROOT)/javavm/runtime \
	$(CVM_DERIVEDROOT)/javavm/runtime \
	$(CVM_SHAREROOT)/javavm/runtime/gc/$(CVM_GCCHOICE) \
	$(CVM_SHAREROOT)/javavm/native/sun/io \
	$(CVM_SHAREROOT)/javavm/native/sun/misc \
	$(CVM_SHAREROOT)/javavm/native/java/lang \
	$(CVM_SHAREROOT)/javavm/native/java/lang/reflect \
	$(CVM_SHAREROOT)/javavm/native/java/security \
	$(CVM_SHAREROOT)/javavm/native/java/util \
	$(CVM_SHAREROOT)/native/common \
	$(CVM_SHAREROOT)/native/java/lang \
	$(CVM_SHAREROOT)/native/java/lang/ref \
	$(CVM_SHAREROOT)/native/java/lang/reflect \
	$(CVM_SHAREROOT)/native/java/lang/fdlibm/src \
	$(CVM_SHAREROOT)/native/java/util \
	$(CVM_SHAREROOT)/native/java/io \
	$(CVM_SHAREROOT)/native/java/net \
	$(CVM_SHAREROOT)/native/java/util/zip \
	$(CVM_SHAREROOT)/native/java/util/zip/zlib-1.1.3 \
	$(CVM_SHAREROOT)/native/sun/misc \

ifeq ($(CVM_LVM), true)
CVM_SRCDIRS += \
	$(CVM_SHAREROOT)/javavm/runtime/lvm
endif

ifeq ($(CVM_JIT), true)
CVM_SRCDIRS += \
	$(CVM_DERIVEDROOT)/javavm/runtime/jit \
	$(CVM_SHAREROOT)/javavm/runtime/jit
endif

# This combines all the native sourcepaths for the vpath search:
# NOTE: PROFILE_SRCDIRS_NATIVE is for profile specific native source files.
#       CVM_SRCDIR is for VM specific and base configuration native source
#       files.
CVM_ALL_NATIVE_SRCDIRS = \
    $(PROFILE_SRCDIRS_NATIVE) $(CVM_SRCDIRS)


# This is for compatibility with the rmi makefiles,
# which still use PROFILE_SRCDIR
# NOTE: PROFILE_SRCDIR is for Java source files.
PROFILE_SRCDIR = $(PROFILE_SRCDIRS)

#
# some build directories that need to be created.
#
CVM_BUILDDIRS  += \
	$(CVM_OBJDIR) \
	$(CVM_BINDIR) \
	$(CVM_DERIVEDROOT)/javavm/runtime \
        $(CVM_DERIVEDROOT)/javavm/runtime/opcodeconsts \
	$(CVM_DERIVEDROOT)/javavm/include \
	$(CVM_DERIVEDROOT)/classes \
	$(CVM_DERIVEDROOT)/classes/sun/misc \
	$(CVM_DERIVEDROOT)/jni \
	$(CVM_DERIVEDROOT)/cni \
	$(CVM_DERIVEDROOT)/offsets \
	$(CVM_DERIVEDROOT)/flags \
	$(CVM_BUILDTIME_CLASSESDIR) \
	$(CVM_TEST_CLASSESDIR) \
	$(CVM_DEMO_CLASSESDIR) \
	$(CVM_LIBDIR) \
	$(CVM_LIBDIR)/security \
	$(CVM_MISC_TOOLS_CLASSPATH)

ifneq ($(CVM_PRELOAD_LIB), true)
CVM_BUILDDIRS  += \
	$(LIB_CLASSESDIR)
endif

ifeq ($(CVM_CREATE_RTJAR),true)
CVM_BUILDDIRS  += \
	$(CVM_RTJARS_DIR)
endif

ifeq ($(CVM_JIT), true)
CVM_BUILDDIRS  += \
	$(CVM_DERIVEDROOT)/javavm/runtime/jit \
	$(CVM_DERIVEDROOT)/javavm/include/jit \
	$(CVM_JCSDIR)
endif

#
# C include directories
#
CVM_INCLUDE_DIRS  += \
	$(CVM_SHAREROOT) \
	$(CVM_BUILD_TOP) \
	. \

#
# These are for the convenience of external code like
# JDK native method libraries that like to #include
# "jni.h" and "java_lang_String.h", etc.  We should
# only need these for those .c files, but gnumake
# doesn't support target-specific macros.
#
CVM_INCLUDE_DIRS  += \
	$(CVM_SHAREROOT)/javavm/export \
	$(CVM_SHAREROOT)/native/common \
	$(CVM_SHAREROOT)/native/java/lang \
	$(CVM_SHAREROOT)/native/java/lang/fdlibm/include \
	$(CVM_SHAREROOT)/native/java/net \
	$(CVM_SHAREROOT)/native/java/io \
	$(CVM_SHAREROOT)/native/java/util/zip \
	$(CVM_SHAREROOT)/native/java/util/zip/zlib-1.1.3 \
	$(CVM_DERIVEDROOT)/jni \

ifneq ($(KBENCH_JAR),)
CVM_TEST_JARFILES += $(KBENCH_JAR)
endif

#
# Classes to build
#
CVM_TEST_CLASSES += \
	ClassLoaderTest \
	HelloWorld \
	EllisGC_ST \
	ExceptionTest \
	StaticFieldTest \
	MTGC \
	MPStress \
	FastSync \
	InterruptTest \
	ThreadsAndSync \
	ThreadSuspend \
	DaemonThreadTest \
	Test \
	ManyFieldsAndMethods \
	ExceptionThrowingOpcodesTest \
	UncaughtExceptionTest \
	StringSignatureTest \
	InterfaceTest \
	DebuggerTest \
	setTime \
	TimeZoneTest \
	CVMadddec \
	ShowSysProps \
	ConvertBoundTest \
	ConvertStressTest \
	SurrogateTest \
	TestLongShiftSpeed \
	TestSync \
	Scopetest \
	scopetest.a.C2 \
	scopetest.b.C1 \
	cvmtest.TypeidRefcountHelper

ifneq ($(CDC_10),true)
CVM_TEST_CLASSES += \
	ChainedExceptionTest
endif

#
# The following tests make up a LOT of classes. Make sure we are not
# preloading them.
#
ifneq ($(CVM_PRELOAD_TEST), true)
CVM_TEST_CLASSES += \
	ClassLink \
	ClassisSubclassOf
endif

# Note: this test removed now that it is known to work. If you want to
# add it back in you must change the CVM_DYNAMIC_LINKING_CLEANUP_ACTION
# above.
# ifeq ($(CVM_DYNAMIC_LINKING), true)
# 	CVM_TEST_CLASSES += DynLinkTest
# endif

ifeq ($(CVM_REFLECT), true)
	CVM_TEST_CLASSES += \
		ReflectionTest \
		ReflectionTestSamePackageHelper \
		cvmtest.ReflectionTestOtherPackageHelper \
		ReflectionClinitTest \
		ReflectionSecurity \
		ReflectionStackOverflowTest
endif

#
# Classes with CNI native methods
#
CVM_CNI_CLASSES += sun.io.ByteToCharISO8859_1 \
		   sun.io.CharToByteISO8859_1 \
		   sun.misc.CVM \
		   'sun.misc.CVM$$Preloader' \
		   java.security.AccessController \
		   java.lang.reflect.Constructor \
		   java.lang.reflect.Field \
		   java.lang.reflect.Method \
		   java.lang.String \
		   java.util.Vector \
		   java.lang.StringBuffer

ifeq ($(CVM_JVMPI), true)
CVM_CNI_CLASSES += sun.misc.CVMJVMPI
CVM_BUILDTIME_CLASSES += \
	sun.misc.CVMJVMPI
endif

ifeq ($(CVM_JVMTI), true)
CVM_CNI_CLASSES += sun.misc.CVMJVMTI
CVM_BUILDTIME_CLASSES += \
	sun.misc.CVMJVMTI
endif

ifeq ($(CVM_INSPECTOR), true)
CVM_TEST_CLASSES += \
	cvmsh

ifneq ($(J2ME_CLASSLIB), cdc)
CVM_TEST_CLASSES += \
	cvmclient
endif

CVM_BUILDTIME_CLASSES += \
	sun.misc.VMInspector

CVM_SHAREOBJS_SPACE += \
	VMInspector.o \
	inspector.o

endif

#
# Classes that the VM needs to have field offsets for.
#
CVM_OFFSETS_CLASSES += \
	java.lang.String \
	java.lang.Throwable \
	java.lang.StackTraceElement \
	java.lang.Class \
	java.lang.Thread \
	java.lang.ThreadGroup \
	java.lang.Boolean \
	java.lang.Byte \
	java.lang.Character \
	java.lang.Short \
	java.lang.Integer \
	java.lang.Long \
	java.lang.Float \
	java.lang.Double \
	java.lang.ref.Reference \
	sun.io.ByteToCharConverter \
	sun.io.CharToByteConverter \
	sun.io.CharToByteISO8859_1 \
	java.lang.StringBuffer \
	java.lang.AssertionStatusDirectives

CVM_OFFSETS_CLASSES += \
	java.net.URLClassLoader

ifeq ($(CVM_CLASSLOADING), true)
CVM_OFFSETS_CLASSES += \
	java.lang.ClassLoader 
endif

ifeq ($(CVM_REFLECT), true)
	CVM_OFFSETS_CLASSES += \
		java.lang.reflect.AccessibleObject \
		java.lang.reflect.Constructor \
		java.lang.reflect.Field \
		java.lang.reflect.Method
endif

#
# The objects that make up zlib-1.1.3
#
ZLIBOBJS = \
	CRC32.o \
	ZipFile.o \
	ZipEntry.o \
	zadler32.o \
	zcrc32.o \
	deflate.o \
	trees.o \
	zutil.o \
	inflate.o \
	infblock.o \
	inftrees.o \
	infcodes.o \
	infutil.o \
	inffast.o \
	zip_util.o

#
# The objects that make up the 'fdlibm' math library
#
MATHOBJS = \
	e_acos.o \
	e_asin.o \
	e_atan2.o \
	e_exp.o \
	e_fmod.o \
	e_log.o \
	e_pow.o \
	e_rem_pio2.o \
	e_remainder.o \
	e_sqrt.o \
	k_cos.o \
	k_rem_pio2.o \
	k_sin.o \
	k_tan.o \
	s_atan.o \
	s_ceil.o \
	s_copysign.o \
	s_cos.o \
	s_fabs.o \
	s_floor.o \
	s_isnan.o \
	s_rint.o \
	s_scalbn.o \
	s_signgam.o \
	s_sin.o \
	s_tan.o \
	w_acos.o \
	w_asin.o \
	w_atan2.o \
	w_exp.o \
	w_fmod.o \
	w_log.o \
	w_pow.o \
	w_remainder.o \
	w_sqrt.o

#
# The following fdlibm files aren't used, so we don't bother build them.
# They are kept here as a referenced.
#
X_MATHOBJS += \
	e_acosh.o \
	e_atanh.o \
	e_cosh.o \
	e_gamma.o \
	e_gamma_r.o \
	e_hypot.o \
	e_j0.o \
	e_j1.o \
	e_jn.o \
	e_lgamma.o \
	e_lgamma_r.o \
	e_log10.o \
	e_scalb.o \
	e_sinh.o \
	k_standard.o \
	s_asinh.o \
	s_cbrt.o \
	s_erf.o \
	s_expm1.o \
	s_finite.o \
	s_frexp.o \
	s_ilogb.o \
	s_ldexp.o \
	s_log1p.o \
	s_logb.o \
	s_matherr.o \
	s_modf.o \
	s_nextafter.o \
	s_significand.o \
	s_tanh.o \
	w_acosh.o \
	w_atanh.o \
	w_cosh.o \
	w_gamma.o \
	w_gamma_r.o \
	w_hypot.o \
	w_j0.o \
	w_j1.o \
	w_jn.o \
	w_lgamma.o \
	w_lgamma_r.o \
	w_log10.o \
	w_scalb.o \
	w_sinh.o \

#
# Compile JIT.o unconditionally. Source does nothing if
# CVM_JIT undefined.
#
CVM_SHAREOBJS_SPACE += \
	JIT.o	

#
# JIT-specific objects
#
ifeq ($(CVM_JIT), true)

CVM_SHAREOBJS_SPACE += \
	jitcompile.o \
	jitirnode.o \
	jitirlist.o \
	jitirrange.o \
	jitirblock.o \
	jitirdump.o \
	jitstackmap.o \
        jitcodebuffer.o \
        jitconstantpool.o \
        jitintrinsic.o \
        jitopcodemap.o \
	jitpcmap.o \
	jitutils.o \
	jitmemory.o \
	jitset.o \
	jitcomments.o \
	jitstats.o \
	jitdebug.o

CVM_SHAREOBJS_SPEED += \
	jitir.o \
	jitopt.o \
	jit_common.o \
        ccm_runtime.o \
        ccmintrinsics.o

#
# JIT-specific tests
#
CVM_TEST_CLASSES += \
	MicroBench \
	CompilerTest \
	jittest.simple \
	jittest.multiJoin \
	assign \
	runRunAll \
	runNamedTest \
	MethodCall \
	Fib \
	ExerciseOpcodes \
	DoResolveAndClinit \
	MTSimpleSync

ifneq ($(KBENCH_JAR),)
CVM_TEST_CLASSES += RunKBench 
endif

endif

# %begin lvm
#
# Logical VM-specific objects
#
ifeq ($(CVM_LVM), true)
CVM_SHAREOBJS_SPACE += \
	lvm.o \
	LogicalVM.o

#
# Logical VM-specific tests
#
CVM_TEST_CLASSES += \
	lvmtest.LVMLauncher \
	lvmtest.PlainLauncher \
	lvmsh

endif
# %end lvm

#
# Objects to build
#
ifeq ($(CVM_CLASSLOADING), true)
CVM_SHAREOBJS_SPACE += \
	classlink.o \
	classverify.o \
	constantpool.o \
	mangle.o \
	quicken.o \
	verifycode.o
endif

CVM_SHAREOBJS_SPEED += \
	gc_common.o \
	gc_impl.o \
	gc_stat.o \
	indirectmem.o \
	interpreter.o \
	named_sys_monitor.o \
	objsync.o \
	stackmaps.o \
	sync.o

CVM_SHAREOBJS_SPACE += \
	basictypes.o \
	bcattr.o \
	bcutils.o \
	classinitialize.o \
	classcreate.o \
	classload.o \
	classlookup.o \
	classtable.o \
	classes.o \
	common_exceptions.o \
	cstates.o \
	float_fdlibm.o \
	globals.o \
	globalroots.o \
	jni_impl.o \
	jni_util.o \
	jvm.o \
	loadercache.o \
	localroots.o \
	opcodelen.o \
	opcodes.o \
	gen_opcodes.o \
	packages.o \
	preloader.o \
	reflect.o \
	stacks.o \
	stackwalk.o \
	stringintern.o \
	typeid.o \
	utils.o \
        porting_debug.o \
	verifyformat.o \
	weakrefs.o \
	\
	Object.o \
	Class.o \
	ClassLoader.o \
	ByteToCharISO8859_1.o \
        CharToByteISO8859_1.o \
	CVM.o \
	DatagramPacket.o \
	Finalizer.o \
	Float.o \
	GC.o \
	Double.o \
	Launcher.o \
	Package.o \
	Runtime.o \
	Shutdown.o \
	System.o \
	SecurityManager.o \
	TimeZone.o \
	Thread.o \
	Throwable.o \
	StrictMath.o \
	Array.o \
	Field.o \
	Method.o \
	Proxy.o \
	Constructor.o \
	FileInputStream.o \
	ObjectInputStream.o \
	ObjectStreamClass.o \
	ObjectOutputStream.o \
	InetAddress.o \
	AccessController.o \
	ResourceBundle.o \
	String.o \
	Inflater.o \
	Vector.o \
	StringBuffer.o

ifneq ($(USE_JAVASE),true)
CVM_SHAREOBJS_SPACE += \
	FileDescriptor.o \
	FileOutputStream.o
endif

ifeq ($(CDC_10),true)
CVM_SHAREOBJS_SPACE += \
	Character.o
else
CVM_SHAREOBJS_SPACE += \
	javaAssertions.o \
	Inet4Address.o \
	Inet6Address.o \
	net_util.o \
	CharacterData.o \
	CharacterDataLatin1.o
endif

#
# Interpreter loop objects. We may compile these slightly differently
# in order to prevent inlining of helper functions.
#
ifeq ($(CVM_INTERPRETER_LOOP), Split)
CVM_SHAREOBJS_LOOP += \
	executejava_split1.o \
	executejava_split2.o
endif
ifeq ($(CVM_INTERPRETER_LOOP), Aligned)
CVM_SHAREOBJS_LOOP += \
	executejava_aligned.o
endif
ifeq ($(CVM_INTERPRETER_LOOP), Standard)
CVM_SHAREOBJS_LOOP += \
	executejava_standard.o
endif

ifeq ($(CVM_JVMTI), true)
CVM_SHAREOBJS_SPACE += \
	executejava_standard_jvmti.o \
	jvmtiCapabilities.o \
	jvmtiEnv.o \
	jvmtiExport.o \
	jvmti_jni.o \
	jvmtiDumper.o \
	CVMJVMTI.o  \
	bag.o
endif

ifeq ($(CVM_JVMPI), true)
CVM_SHAREOBJS_SPACE += \
	jvmpi.o \
	CVMJVMPI.o
endif

ifeq ($(CVM_XRUN), true)
CVM_SHAREOBJS_SPACE += \
	xrun.o 
endif

ifeq ($(CVM_AGENTLIB), true)
CVM_SHAREOBJS_SPACE += \
	agentlib.o
endif

ifeq ($(CVM_USE_MEM_MGR), true)
CVM_SHAREOBJS_SPACE += \
	mem_mgr.o
endif

# Include support for a specific profiler specified with
# CVM_JVMPI_PROFILER=<profiler>.  Note that this is not an
# officially supported feature of CDC.
ifeq ($(CVM_JVMPI), true)
ifneq ($(CVM_JVMPI_PROFILER),)
CVM_DEFINES         += -DCVM_JVMPI_PROFILER=$(CVM_JVMPI_PROFILER)
CVM_SHAREOBJS_SPACE += jvmpi_$(CVM_JVMPI_PROFILER).o 
endif
endif

CVM_SHAREOBJS_SPACE += $(ZLIBOBJS)

# Note: this test removed now that it is known to work. If you want to
# add it back in you must change the CVM_DYNAMIC_LINKING_CLEANUP_ACTION
# above.
# ifeq ($(CVM_DYNAMIC_LINKING), true)
# 	# Pick up the native code for the dynamic linking test
# 	CVM_SHAREOBJS_SPACE += DynLinkTest.o
# endif

#
# After including the profile defs files, our object lists are complete.
# Create the full path object list.
#
CVM_OBJECTS_SPEED0 = $(CVM_SHAREOBJS_SPEED) $(CVM_TARGETOBJS_SPEED)
CVM_OBJECTS_SPACE0 = $(CVM_SHAREOBJS_SPACE) $(CVM_TARGETOBJS_SPACE)
CVM_OBJECTS_LOOP0  = $(CVM_SHAREOBJS_LOOP)  $(CVM_TARGETOBJS_LOOP)
CVM_OBJECTS_OTHER0 = $(CVM_SHAREOBJS_OTHER) $(CVM_TARGETOBJS_OTHER)

CVM_OBJECTS_SPEED  = $(patsubst %.o,$(CVM_OBJDIR)/%.o,$(CVM_OBJECTS_SPEED0))
CVM_OBJECTS_SPACE  = $(patsubst %.o,$(CVM_OBJDIR)/%.o,$(CVM_OBJECTS_SPACE0))
CVM_OBJECTS_LOOP   = $(patsubst %.o,$(CVM_OBJDIR)/%.o,$(CVM_OBJECTS_LOOP0))
CVM_OBJECTS_OTHER  = $(patsubst %.o,$(CVM_OBJDIR)/%.o,$(CVM_OBJECTS_OTHER0))
CVM_OBJECTS  += $(CVM_OBJECTS_SPEED) $(CVM_OBJECTS_SPACE) \
	        $(CVM_OBJECTS_LOOP) $(CVM_OBJECTS_OTHER)

CVM_FDLIB_FILES = $(patsubst %.o,$(CVM_OBJDIR)/%.o,$(MATHOBJS))

##################################################################
# Miscellaneous options. Some platforms may want to override these.
##################################################################

# file separator character
CVM_FILESEP	= /

ifeq ($(CDC_10),true)
# Some platforms don't have a tzmappings file. They should override
# CVM_TZDATAFILE be empty in this case.
CVM_TZDIR      = $(CDC_OS_COMPONENT_DIR)/src/$(TARGET_OS)/lib
CVM_TZDATAFILE = $(CVM_LIBDIR)/tzmappings
endif

# mime content properties file
CVM_MIMEDIR	 = $(CDC_OS_COMPONENT_DIR)/src/$(TARGET_OS)/lib
CVM_MIMEDATAFILE = $(CVM_LIBDIR)/content-types.properties

# Name of the cvm binary
ifeq ($(CVM_BUILD_SO),true)
CVM             = $(LIB_PREFIX)cvm$(LIB_POSTFIX)
else
CVM             = cvm
endif

##############################################################
# Locate the tools.
##############################################################

# See if the user specified something other than the default
# for target tool locations.
ifneq ($(CVM_TARGET_TOOLS_PREFIX)$(TARGET_CC),)
    USER_SPECIFIED_LOCATION = true
endif
ifneq ($(CVM_TOOLS_DIR)$(CVM_TARGET_TOOLS_DIR),)
    # don't count if imported from the shell
    ifneq ($(origin CVM_TOOLS_DIR),environment)
        ifneq ($(origin CVM_TARGET_TOOLS_DIR),environment)
            USER_SPECIFIED_LOCATION = true
        endif
    endif
endif

# If the user specified the target tools location, then don't use native tools.
ifeq ($(USER_SPECIFIED_LOCATION),true)
    override CVM_USE_NATIVE_TOOLS := false
endif

# Locate the target gnu tools:
#
# The goal is to set CVM_TARGET_TOOLS_PREFIX to a value that when 
# prepended to "gcc", will specify the full path of gcc. Note that
# this is the only path we officially support on the comand line.
# CVM_TOOLS_DIR, CVM_TARGET_TOOLS_DIR, and CVM_USE_NATIVE_TOOLS
# should not be set on the command line.
#
# Unless we were specifically told to use native tools, the gnu
# cross compiler is located by using CVM_TOOLS_DIR, CVM_HOST,
# TARGET_CPU_FAMILY, TARGET_DEVICE, and TARGET_OS.
# 
# CVM_TOOLS_DIR and CVM_HOST can be overriden, but TARGET_CPU_FAMILY,
# TARGET_CPU_FAMILY, and TARGET_OS cannot.
# 
# CVM_TARGET_TOOLS_PREFIX can be overriden, in which case none of
# the other variables normally used to compute it matter.
#
CVM_TOOLS_DIR ?= /usr/tools
CVM_TARGET_TOOLS_DIR	?= $(CVM_TOOLS_DIR)/$(CVM_HOST)/gnu/bin
CVM_TARGET_TOOLS_PREFIX ?= \
    $(CVM_TARGET_TOOLS_DIR)/$(TARGET_CPU_FAMILY)-$(TARGET_DEVICE)-$(TARGET_OS)-

# Unless the user told us where the tools are, check if a cross compiler
# exists in the default location. Use native tools if not.
ifneq ($(USER_SPECIFIED_LOCATION), true)
GCC_PATH 		:= $(CVM_TARGET_TOOLS_PREFIX)gcc
ifneq ($(wildcard $(GCC_PATH)),$(GCC_PATH))
CVM_TARGET_TOOLS_PREFIX :=
CVM_TARGET_TOOLS_DIR    :=
else
override CVM_USE_NATIVE_TOOLS    := false
endif
endif

#
# Locate the host tools:
#

# Find where the native gcc is located (HOST_CC).
#
ifeq ($(CVM_USE_NATIVE_TOOLS), true)
TEMP_HOST_CC		:= $(CC)
TEMP_HOST_CCC		:= $(CXX)
else
CVM_HOST_TOOLS_DIR 	:= $(CVM_TARGET_TOOLS_DIR)
CVM_HOST_TOOLS_PREFIX	:= $(CVM_HOST_TOOLS_DIR)/
TEMP_HOST_CC 		:= $(CVM_HOST_TOOLS_PREFIX)gcc
TEMP_HOST_CCC 		:= $(CVM_HOST_TOOLS_PREFIX)g++
ifneq ($(TEMP_HOST_CC), $(shell ls $(TEMP_HOST_CC) 2>&1))
CVM_HOST_TOOLS_PREFIX   :=
TEMP_HOST_CC		:= $(CC)
TEMP_HOST_CCC		:= $(CXX)
endif
endif

# Using TEMP variables above allows HOST_CC and HOST_CC to be set in the
# GNUmakefile and not get overwritten by the above := assignments.
HOST_CC 	?= $(TEMP_HOST_CC)$(GCC_VERSION)
HOST_CCC	?= $(TEMP_HOST_CCC)$(GCC_VERSION)

#
# Locate the JDK tools:
#
# Look in JDK_HOME. If java doesn't exists there, then just assume
# the tools are on the path. The user can override either 
# JDK_HOME pr JDK_VERSION, or specify CVM_JAVA_TOOLS_PREFIX.
#
ifeq ($(CDC_10),true)
JDK_VERSION	?= jdk1.3.1
else
JDK_VERSION	?= jdk1.4.2
endif

ifneq ($(JDK_HOME),)
# If user set JDK_HOME, then always use it
CVM_JAVA_TOOLS_PREFIX	?= $(JDK_HOME)/bin/
else
# If user did not set JDK_HOME, then look in the default location. If
# nothing in the default location, then assume java tools are on the path
# or user set CVM_JAVA_TOOLS_PREFIX.
JDK_HOME	= $(CVM_TOOLS_DIR)/$(CVM_HOST)/java/$(JDK_VERSION)
ifneq ($(wildcard $(JDK_HOME)/bin/java*),)
CVM_JAVA_TOOLS_PREFIX	?= $(JDK_HOME)/bin/
endif
endif

CVM_JAVAC		?= $(CVM_JAVA_TOOLS_PREFIX)javac
CVM_JAVAH		?= $(CVM_JAVA_TOOLS_PREFIX)javah
CVM_JAVA		?= $(CVM_JAVA_TOOLS_PREFIX)java
CVM_JAVADOC		?= $(CVM_JAVA_TOOLS_PREFIX)javadoc
CVM_JAR			?= $(CVM_JAVA_TOOLS_PREFIX)jar

JAVAC_OPTIONS +=  -encoding iso8859-1
ifeq ($(CDC_10),true)
JAVAC_SOURCE_TARGET_OPTIONS ?= -target 1.3
else
JAVAC_SOURCE_TARGET_OPTIONS ?= -source 1.4 -target 1.4
endif
JAVAC_OPTIONS += $(JAVAC_SOURCE_TARGET_OPTIONS)

#
# Location of source for scripts and Java source files used during the build
#
CVM_MISC_TOOLS_SRCDIR    = $(CVM_SHAREROOT)/tools
CVM_MISC_TOOLS_CLASSPATH = $(CVM_BUILD_TOP)/classes.tools

#
# Specify all the host and target tools. 
#
ifneq ($(CVM_USE_NATIVE_TOOLS), true)
TARGET_CC	?= $(CVM_TARGET_TOOLS_PREFIX)gcc
TARGET_CCC	?= $(CVM_TARGET_TOOLS_PREFIX)g++
else
TARGET_CC	?= $(HOST_CC)
TARGET_CCC	?= $(HOST_CCC)
endif

TARGET_AS	?= $(TARGET_CC)
TARGET_LD	?= $(TARGET_CC)
TARGET_AR	?= $(CVM_TARGET_TOOLS_PREFIX)ar
TARGET_RANLIB	?= $(CVM_TARGET_TOOLS_PREFIX)ranlib
TARGET_AR_CREATE ?= $(TARGET_AR) rc $(1)
TARGET_AR_UPDATE ?= $(TARGET_RANLIB) $(1)

# NOTE: We already set HOST_CC above.
ifneq ($(origin LEX),default)
FLEX		= $(LEX)
else
FLEX		?= $(CVM_HOST_TOOLS_PREFIX)flex
endif
BISON		?= $(CVM_HOST_TOOLS_PREFIX)bison
ZIP             ?= zip
UNZIP           ?= unzip
CVM_ANT         ?= ant

#######################################################################
# Build tool options:
#
# The defaults below works on most platforms that use unix based
# and used gcc. Overrides to these values should be done in
# platform specific makefiles, not on the command line.
########################################################################

#
# Compiler and linker flags
#

GET_GCC_VERSION := $(shell $(TARGET_CC) -dumpversion  2> /dev/null)
GET_GCC_VERSION := $(subst ., ,$(GET_GCC_VERSION))
GCC_MAJOR_VERSION := $(firstword $(GET_GCC_VERSION))
GCC_MINOR_VERSION := $(word 2, $(GET_GCC_VERSION))

# don't enable extra gcc warnings if using gcc version 2.x
# enable -fwrapv for 3.4 and later

GCC_BOUNDARIES := $(shell awk \
    'END { \
             if (M < 3) { \
                 printf "v2.x"; \
             } else if (M * 1000 + N >= 3004) { \
                 printf "v3.4.x"; \
             } \
         }' \
    M=$(GCC_MAJOR_VERSION) N=$(GCC_MINOR_VERSION) < /dev/null)

ifeq ($(GCC_BOUNDARIES), v2.x)
USE_EXTRA_GCC_WARNINGS ?= false
USE_FWRAPV = false
endif
ifeq ($(GCC_BOUNDARIES), v3.4.x)
USE_FWRAPV = true
endif

USE_EXTRA_GCC_WARNINGS ?= true
ifeq ($(USE_EXTRA_GCC_WARNINGS),true)
EXTRA_CC_WARNINGS ?= -W -Wno-unused-parameter -Wno-sign-compare
endif

ifeq ($(USE_FWRAPV), true)
GCC_VERSION_SPECIFIC_FLAGS += -fwrapv
endif

# for creating gnumake .d files
CCDEPEND   	= -MM

# CVM_EXTRA_XXX_FLAGS usually come from the make command line
ASM_FLAGS	= -c -fno-common $(ASM_ARCH_FLAGS) $(CVM_EXTRA_ASM_FLAGS)
CCFLAGS     	= -c -fno-common -Wall \
			$(EXTRA_CC_WARNINGS) \
			-fno-strict-aliasing \
			$(GCC_VERSION_SPECIFIC_FLAGS) \
			$(CC_ARCH_FLAGS) \
			$(CVM_EXTRA_CC_FLAGS)
CCCFLAGS 	= -fno-rtti
ifeq ($(CVM_OPTIMIZED), true)
CCFLAGS_SPEED_OPTIONS ?= -O4 
CCFLAGS_SPACE_OPTIONS ?= -O2 
endif
CCFLAGS_SPEED	= $(CCFLAGS_SPEED_OPTIONS) $(CCFLAGS)
CCFLAGS_SPACE	= $(CCFLAGS_SPACE_OPTIONS) $(CCFLAGS)

CCFLAGS_LOOP	= $(CCFLAGS_SPEED) $(CC_ARCH_FLAGS_LOOP)
CCFLAGS_FDLIB 	= $(CCFLAGS_SPEED) $(CC_ARCH_FLAGS_FDLIB)

ifeq ($(CVM_SYMBOLS), true)
CCFLAGS		+= -g
ASM_FLAGS	+= -g
endif

ifeq ($(CVM_GPROF), true)
LINKFLAGS 	+= -pg
ifeq ($(CVM_GPROF_NO_CALLGRAPH), true)
CCFLAGS   	+= -g
else
CCFLAGS   	+= -pg -g
endif
endif

ifeq ($(CVM_GCOV), true)
CCFLAGS   	+= -fprofile-arcs -ftest-coverage
endif

# Note, ALL_INCLUDE_FLAGS flags is setup in rules.mk so
# abs2rel only needs to be called on it once.
CPPFLAGS 	+= $(CVM_DEFINES) $(ALL_INCLUDE_FLAGS)
CFLAGS_SPEED   	= $(CFLAGS) $(CCFLAGS_SPEED) $(CPPFLAGS)
CFLAGS_SPACE   	= $(CFLAGS) $(CCFLAGS_SPACE) $(CPPFLAGS)
CFLAGS_LOOP    	= $(CFLAGS) $(CCFLAGS_LOOP)  $(CPPFLAGS)
CFLAGS_FDLIB   	= $(CFLAGS) $(CCFLAGS_FDLIB) $(CPPFLAGS)
CFLAGS_JCS	= 

LINKFLAGS       += -g -Wl,-export-dynamic $(LINK_ARCH_FLAGS)
LINKLIBS     	+= -lpthread -ldl $(LINK_ARCH_LIBS)
LINKLIBS_JCS    +=

SO_CCFLAGS   	= $(CCFLAGS_SPEED)
SO_CFLAGS	= $(CFLAGS) $(SO_CCFLAGS) $(CPPFLAGS)
SO_LINKFLAGS 	= $(LINKFLAGS) -shared

#
# commands for running the tools
#
ASM_CMD 	= $(AT)$(TARGET_AS) $(ASM_FLAGS) -D_ASM $(CPPFLAGS) \
			-o $@ $(call abs2rel,$<)

# compileCCC(flags, objfile, srcfiles)
compileCCC      = $(AT)$(TARGET_CCC) $(1) -o $(2) $(call abs2rel,$(3))
CCC_CMD_SPEED   = $(call compileCCC,$(CFLAGS_SPEED) $(CCCFLAGS),$@,$<)
CCC_CMD_SPACE   = $(call compileCCC,$(CFLAGS_SPACE) $(CCCFLAGS),$@,$<)

# compileCC(flags, objfile, srcfiles)
compileCC       = $(AT)$(TARGET_CC) $(1) -o $(2) $(call abs2rel,$(3))
CC_CMD_SPEED    = $(call compileCC,$(CFLAGS_SPEED),$@,$<)
CC_CMD_SPACE	= $(call compileCC,$(CFLAGS_SPACE),$@,$<)
CC_CMD_LOOP	= $(call compileCC,$(CFLAGS_LOOP) ,$@,$<)
CC_CMD_FDLIB	= $(call compileCC,$(CFLAGS_FDLIB),$@,$<)

# LINK_CMD(objFiles, extraLibs)
LINK_CMD	= $(AT)$(TARGET_LD) $(LINKFLAGS) -o $@ $(1) $(LINKLIBS) $(2)
SO_ASM_CMD 	= $(ASM_CMD)
SO_CC_CMD	= $(call compileCC,$(SO_CFLAGS),$@,$<)

# SO_LINK_CMD(objFiles, extraLibs)
SO_LINK_CMD 	= $(AT)$(TARGET_LD) $(SO_LINKFLAGS) -o $@ $(1) $(2)
JAVAC_CMD	= $(CVM_JAVAC) $(JAVAC_OPTIONS)
JAR_CMD		= $(CVM_JAR)
JAVA_CMD	= $(CVM_JAVA)

# CVM_LINK_CMD(objFiles, extraLibs)
ifeq ($(CVM_BUILD_SO),true)
CVM_LINK_CMD   = $(call SO_LINK_CMD, $(1), $(2))
else
CVM_LINK_CMD   = $(call LINK_CMD, $(1), $(2))
endif


#
# Standard classpath for libclasses compilation
#
JAVA_CLASSPATH += $(LIB_CLASSESDIR)

#
# Include target makfiles last.
#
# NOTE: the target defs.mk were switched to come after the
# shared defs.mk because the platform-specific object file list
# currently needs to look at a flag (dynamic linking) whose default is
# set in the global flags. We should consider doing the separation
# of the defs from the building of the object file lists.
#
-include $(CDC_CPU_COMPONENT_DIR)/build/$(TARGET_CPU_FAMILY)/defs.mk
-include $(CDC_OS_COMPONENT_DIR)/build/$(TARGET_OS)/defs.mk
-include $(CDC_OSCPU_COMPONENT_DIR)/build/$(TARGET_OS)-$(TARGET_CPU_FAMILY)/defs.mk
-include $(CDC_DEVICE_COMPONENT_DIR)/build/$(TARGET_OS)-$(TARGET_CPU_FAMILY)-$(TARGET_DEVICE)/defs.mk

# Root directory for unittests reports
REPORTS_DIR ?= $(call POSIX2HOST,$(CDC_DIST_DIR)/reports)
