# @(#)top.mk	1.38 06/10/26
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
# Topmost makefile shared by all targets
#
#########################################################
# Definitions of shared supported options.
# OS specific options are described in build/<os>/top.mk
#########################################################
#
# J2ME_CLASSLIB default: cdc
#     The class library build target. The choices are cdc and foundation.
#     cdc represents a limited class library that is meant for testing
#     purposes only. foundation represents the full Foundation Profile
#     class library.
#
# JDK_HOME
#     Location of the Java SE SDK tools. Versions 1.4.2 through Java 6 are
#     supported. JDK_HOME should point to the directory containing the SDK's
#     bin directory. If this option is left unset, tools found on the $PATH
#     are used.
#
# OPT_PKGS
#     Indicates that an optional package will be compiled as part of the
#     regular build. The syntax of this flag is as follows: 
#
#       OPT_PKGS=all | <pkg1>[,<pkg2>]
#
#     where <pkg1> is the name of the optional package and a ',' is used to
#     separate multiple package names.  Quotes ("") are needed if spaces exist
#     in the value of OPT_PKGS, but are not necessary otherwise. When OPT_PKGS
#     is set to all, all available optional packages will be part of the
#     compilation.
#
#     Note that OPT_PKGS is typcially used for rmi and jdbc optional packages.
#     Other optional packages use a different mechanism.
#
# CVM_DEBUG default: false
#     Build the debug version of the VM.
#
# CVM_DEBUG_ASSERTS default: $(CVM_DEBUG)
#     Enable asserts. Also is forced to true if CVM_VERIFY_HEAP=true.
#
# CVM_DEBUG_CLASSINFO default: $(CVM_DEBUG)
#     Build the VM with the code necessary to interpret class debugging
#     information in the class files. Also causes preloaded classes to include
#     debugging information if they were compiled with it.
#     CVM_JAVAC_DEBUG=true should also be used to provide class debugging
#     information in the CDC and Foundation class files. Otherwise
#     this option will only benefit application classes that are
#     compiled with the -g option.
#
# CVM_DEBUG_DUMPSTACK default: $(CVM_DEBUG)
#     Include support for the CVMdumpStack and CVMdumpFrame functions.
#     CVMdumpStack is useful for dumping a Java stack from gdb after the
#     VM has crashed.
#
# CVM_DEBUG_STACKTRACES default: true
#     Include code for doing Throwable.printStackTrace and
#     Throwable.fillInStackTrace. If false, then printStackTrace will print
#     a "not supported" message. This is not really just a debug build feature.
#     To reduce the footprint of non-debug builds, set this option to false.
#
# CVM_JAVAC_DEBUG default: $(CVM_DEBUG)
#     Compile classes with debugging information (line numbers, localvariables,
#     etc.) by using the -g option. Otherwise build using -g:none. This will
#     not affect the size of the VM image unless CVM_DEBUG_CLASSINFO is also
#     true. Using this option will increase the size of the profile jar file.
#
# CVM_JIT default: target specific - see GNUmakefile
#     Build a VM with the dynamic compiler.
#
# CVM_AOT default: false
#     Enable runtime support for ahead-of-time compilation of Java methods.
#
# CVM_JIT_USE_FP_HARDWARE default: target specific - see GNUmakefile
#     Enable the JIT to use an FPU. If true, the JIT will emit FP instructions
#     and use FP registers. If false, the JIT will store FP values in general
#     purpose registers and call out to C or assembler helper functions to do
#     FP arithmetic.
#
# CVM_JVMTI default: false
#     Build a VM that supports the new JVMTI debugger/profiler interface.
#     When set true, there can be a significant degradation of performance.
#
# CVM_JVMTI_ROM default: false
#     Build a VM that supports debugging romized system classes. Note that
#     CVM_JVMTI must also be true.
#
# CVM_OPTIMIZED default: !$(CVM_DEBUG)
#     If true, then use various C compiler optimization features. Setting
#     both CVM_DEBUG=true and CVM_OPTIMIZED=true will provide both debug
#     support and optimized code that will run faster, but not as fast
#     as when using CVM_DEBUG=false.
#
# CVM_PRELOAD_SET default: minfull
#     Build a VM with the specified set of classes preloaded.
#     Possible choices:
#        min - minimum required classes to build and run the vm.
#        minfull - same as min but with full transitive closure
#        nullapp - mimumum required classes that avoids any class loading
#                  when running an app that does nothing.
#        nullappfull - same as nullapp but with full transitive closure
#        libfull - All the library classes
#        libtestfull - All the library and test classes (testclasses.zip)
#     Note that "full" implies full transitive closure of the classes being
#     preloaded, meaning that there will be no references from preloaded
#     classes to classes that need to be dynamically loaded.
#
# CVM_SYMBOLS default: $(CVM_DEBUG)
#     Include debugging and symbol information for C code even if the build is
#     optimized.
#
# USE_VERBOSE_MAKE default: false
#     Avoid printing detailed messages that show each build step. Has the
#     opposite meaning as CVM_TERSEOUTPUT, which can be used instead, but
#     is now deprecated.
#
# CVM_TRACE default: $(CVM_DEBUG)
#     Include support for tracing VM events to stderr. The events that are
#     traced are controlled by the -Xtrace option.  Since CVM_TRACE=true
#     slows down the VM a lot, it is useful to build with CVM_TRACE=false
#     and CVM_DEBUG=true to get debugging support without tracing support.
#
# CVM_VERIFY_HEAP default: false
#     Enable Java heap verification code.  Because this can have a dramatically
#     adverse affect on performance, is can be turned off while still enabling
#     other assertion code with CVM_DEBUG_ASSERTS=true.
#
# CVM_INCLUDE_COMMCONNECTION default: false
#     Include GCF CommProtocol support. This feature is not supported
#     on all platforms.
#
# CVM_BUILD_SUBDIR_NAME default: unset
#     Name of subdir to place build into, rather than using the current
#     build directory. Makes it possible to have multiple builds in the
#     same build directory, and makes it easier to remove builds.
#
# CVM_DUAL_STACK default: false
#     Support concurrent CLDC and CDC stacks running on the same VM.
#     This option is most commonly used when supporting a MIDP stack.
#
# CVM_SPLIT_VERIFY default: false
#     Set to true to enable the split verifier, which will make use
#     of StackMap resources in the .class file for quicker verification.
#
#####################################################################
# Deprecated build options.
#####################################################################
#
# CVM_JVMPI default: false
#     Build a VM that supports the legacy JVMPI Java profiler. This option is
#     not supported with CVM_JIT=true. When set true, there will be a significant
#     degradation of performance.
#
# CVM_JVMPI_TRACE_INSTRUCTION default: $(CVM_JVMPI)
#     Build a VM that supports Java bytecode tracing for profiling purposes.
#     Enabling this option imposes a greater runtime burden on the interpreter
#     and may cause it to run a little slower.  Hence, this option is provided
#     in case the user does not need this feature and does not want the
#     additional runtime burden to impact the profile they are sampling.
#
# CVM_PRELOAD_LIB default: unset
#     Build a VM with all the system and profile classes preloaded.
#     Obsolete.  Replaced by CVM_PRELOAD_SET.
#
# CVM_PRELOAD_TEST default: unset
#     Build a VM with the test classes (testclasses.zip) preloaded.
#     Obsolete.  Replaced by CVM_PRELOAD_SET.
#
#####################################################################
# Definitions of limited options. The default values of these options
# are supported. Alternate values should be considered experimental.
#####################################################################
#
# CVM_CSTACKANALYSIS default: false
#     Include stub functions to assist in C stack usage analysis.
#
# CVM_GPROF default: false
#     Enable gprof profiling support.
#
# CVM_GPROF_NO_CALLGRAPH: true
#     When gprof is enabled, this option can be used to control if
#     call graph is generated in the gprof output.
#
# CVM_CCM_COLLECT_STATS default: false
#     Build a VM which collect statistics on the runtime activity of
#     dynamically compiled code, even if the build is optimized.
#
# CVM_CLASSLIB_JCOV default: false
#     Build library classes with -Xjov (JDK 1.4 javac command line option)
#     enabled. Also instruments the VM to simulate loading of classfiles for
#     preloaded classes at startup.
#
# CVM_CLASSLOADING default: true
#     Build a VM that supports class loading. This option also affects how many
#     system classes are preloaded. If true, then the minimal number of classes
#     needed to bootstrap the VM are preloaded and the rest are dynamically
#     loaded. WARNING: setting this option to false is currently broken.
#
# CVM_DYNAMIC_LINKING default: true
#     Support the base functionality in the porting layer for dynamic linking.
#     This will be needed by dynamic classloading as well as debugger and
#     profiler support.
#
# CVM_GCCHOICE default: generational
#     Set to the garbage collection technique. semispace, marksweep and
#     generational-seg are also available.
#
# CVM_JIT_DEBUG default: false
#     Build the JIT with extra debugging support, including support for
#     filtering which methods are compiled, and support for tracing
#     the JCS rules used during compilation.
#
# CVM_JIT_ESTIMATE_COMPILATION_SPEED default: false
#     Build a VM which estimates the theoretical maximum compilation speed
#     of the JIT. The measurement is in KB of byte-code compiled per second.
#
# CVM_JIT_PROFILE default: false
#     Enable profiling of compiled code. Use -Xjit:Xprofile=<filename> to
#     enable profiling and specify the file to dump profile information too.
#     For linux, enabling profiling at runtime generally degrades performance
#     by about 2%. If profiling support is included at build time but not
#     used at runtime, it has no affect on preformance.
#
# CVM_REBUILD default: false
#     Rebuild using the same build flags as last time, preventing the need to
#     retype a bunch of command line options. The main benefit of this is that
#     there is not risk of having a typo that results in a bunch
#     of cleanup actions triggered.
#
#     NOTE: this option will not remember the value of any options that
#     specify where tools are located, such as JDK_HOME and CVM_TOOLS_DIR.
#
# CVM_REFLECT default: true
#     Build a VM that supports the java.lang.reflect package. This does not
#     cause any native function definitions to be eliminated from the
#     build. Instead, their bodies simply throw an
#     UnsupportedOperationException. See the description of CVM_SERIALIZATION
#     for more information.
#
#     NOTE: setting this option false will result in a VM that is not
#     compliant with the J2ME CDC and Foundation specifications.
#
# CVM_SERIALIZATION default: true
#     Build a VM that supports object serialization
#     (java.io.ObjectInputStream, java.io.ObjectOutputStream).
#     Currently, this only eliminates three functions: JVM_AllocateNewObject,
#     JVM_AllocateNewArray, and JVM_LoadClass0.  In addition, serialization
#     depends on reflection, so if CVM_SERIALIZATION is true, CVM_REFLECT will
#     be set to true as well.
#
#     NOTE: setting this option false will result in a vm that is not
#     compliant with the J2ME CDC and Foundation specifications.
#
# CVM_TRACE_JIT default: $(CVM_TRACE)
#     Build a VM with tracing support enabled for all dynamic compiler events,
#     even if the build is optimized. This option is provided to allow
#     building without any other debugging support other than JIT tracing,
#     thus reducing the performance impact. Compiled code will run somewhat
#     slower as a result of the method call tracing that is enabled
#     (estimated 5% slower).
#
# CVM_XRUN default: false
#     Build a VM which supports the -Xrun command line option for loading
#     native libraries. Defaults to true if CVM_JVMPI is true.
#
# CVM_GCOV default: false
#     Enable gcov code coverage support.
#
# CVM_JIT_PMI default: false
#     Support for the dynamic patching of calls to methods within 
#     compiled methods. Not supported on all platforms.
#
# CVM_CREATE_RT_JAR default: false
#     Place all JSR and CDC library classes into the same jar file.
#
# CVM_INSPECTOR default: false
#     Include VM Inspector support in the build.
#
#####################################################################
# Options for locating target tools such as gcc. See defs.mk file
# for more details:
#####################################################################
#
#
# CVM_TOOLS_DIR default:
#   /usr/tools
# CVM_HOST default:
#   "i686-SuSE-linux" on SuSE Linux hosts
# CVM_HOST_TOOLS_DIR default:
#   <empty>
# CVM_TARGET_TOOLS_DIR default:
#   $(CVM_TOOLS_DIR)/$(CVM_HOST)/gnu/bin
# CVM_TARGET_TOOLS_PREFIX default:
#   $(CVM_TARGET_TOOLS_DIR)/$(TARGET_CPU_FAMILY)-$(TARGET_DEVICE)-$(TARGET_OS)-
#
# Options for locating JDK tools such as java, javac, javah, and jar. Normally
# you will override JDK_HOME on the command line. See defs.mk for more
# details. 
#
# JDK_VERSION default:
#   jdk1.4.2
# JDK_HOME default:
#   $(CVM_TOOLS_DIR)/$(CVM_HOST)/java/$(JDK_VERSION)
# CVM_JAVA_TOOLS_PREFIX default:
#   $(JDK_HOME)/bin/
#

# speed up makefile by not using implicit suffixes
.SUFFIXES:

# So JSR makefiles know this is a CVM build
CVM_BUILD = true

#
# Define our platform by setting TARGET_OS, TARGET_CPU_FAMILY, and
# TARGET_DEVICE based on the name of the directory we are building from.
#
CWD_PARTS		:= $(subst -, ,$(notdir $(CURDIR)))
TARGET_OS		:= $(word 1,$(CWD_PARTS))
TARGET_CPU_FAMILY	:= $(word 2,$(CWD_PARTS))
TARGET_DEVICE		:= $(word 3,$(CWD_PARTS))

ABSPATH = $(shell mkdir -p $(1); cd $(1); echo `pwd`)

# COMPONENTS_DIR is the directory that all the components are located in,
# such as midp, pcsl, and jump. It is used for providing default locations
# for directories like MIDP_DIR and JUMP_DIR. It must be an absolute path.
ifndef COMPONENTS_DIR
COMPONENTS_DIR     := $(call ABSPATH, ../../..)
endif

# If CVM_BUILD_SUBDIR_NAME was specified, then it is reasonable to assume
# that CVM_BUILD_SUBDIR should be enabled.  This saves the user from
# having to specify both CVM_BUILD_SUBDIR and CVM_BUILD_SUBDIR_NAME.
# Specifying CVM_BUILD_SUBDIR_NAME automatically implies CVM_BUILD_SUBDIR.
ifneq ($(CVM_BUILD_SUBDIR_NAME),)
    CVM_BUILD_SUBDIR = true
else
    CVM_BUILD_SUBDIR_NAME = .
endif

ifeq ($(CVM_BUILD_SUBDIR), true)
  ifeq ($(CVM_DEBUG), true)
    CVM_BUILD_SUBDIR_NAME=$(J2ME_CLASSLIB)_dbg
  else
    CVM_BUILD_SUBDIR_NAME=$(J2ME_CLASSLIB)
  endif
else
  override CVM_BUILD_SUBDIR_NAME=.
endif

ifeq ($(CVM_BUILD_SUBDIR), true)
# There should no longer be a need to use CVM_BUILD_SUBDIR_UP now that paths
# are abosolute instead of relative. The folowing is left in to make errors
# more obvious.
#CVM_BUILD_SUBDIR_UP = ../
CVM_BUILD_SUBDIR_UP = "Do not use CVM_BUILD_SUBDIR_UP"
endif

# If requested, load build flags from previous build. This must be done
# before any makefile logic that is dependent on the build flags
# that are read in.
CVM_BUILD_FLAGS_FILE=$(CVM_BUILD_TOP)/.previous.build.flags
ifeq ($(CVM_REBUILD),true)
-include $(CVM_BUILD_FLAGS_FILE)
endif

# Include JavaSE setup if necessary:
ifeq ($(USE_JAVASE),true)
PROFILE_DIR ?= $(COMPONENTS_DIR)/cvmjavase
include $(PROFILE_DIR)/build/share/top_javase.mk
endif

# Full path for the cdc component directory
CDC_DIR ?= $(call ABSPATH, ../..)
export CDC_DIR := $(CDC_DIR)

# Note CVM_TOP used to be relative, but is now absolute. CVM_TOP_ABS references
# can be changed to CVM_TOP and CVM_TOP_ABS can then be removed.
CVM_TOP       := $(CDC_DIR)
CVM_TOP_ABS   := $(CDC_DIR)

# Paths to os, cpu, os-cpu, and os-cpu-device ports. They default
# to being in the cdc component, but can be setup to point elsewhere.
export CDC_OS_COMPONENT_DIR      ?= $(CDC_DIR)
export CDC_CPU_COMPONENT_DIR     ?= $(CDC_DIR)
export CDC_OSCPU_COMPONENT_DIR   ?= $(CDC_DIR)
export CDC_DEVICE_COMPONENT_DIR  ?= $(CDC_DIR)

# Locate the cdc-com component
ifeq ($(USE_CDC_COM),true)
CDC_COM_DIR ?= $(COMPONENTS_DIR)/cdc-com
ifeq ($(wildcard $(CDC_COM_DIR)/build/share/top.mk),)
$(error CDC_COM_DIR must point to a directory containing the cdc-com sources: \
        $(CDC_COM_DIR))
endif
endif

# Include any existing platform defs first
ifneq ($(J2ME_PLATFORM),)
include $(CDC_DIR)/build/share/defs_$(J2ME_PLATFORM).mk
endif


# Handle deprecated uses of CVM_INCLUDE_MIDP and CVM_INCLUDE_JUMP here
# since we rely on the proper setting of USE_MIDP and USE_JUMP just below.
ifdef CVM_INCLUDE_MIDP
$(warning CVM_INCLUDE_MIDP is deprecated. Use USE_MIDP instead.)
USE_MIDP = $(CVM_INCLUDE_MIDP)
endif
ifdef CVM_INCLUDE_JUMP
$(warning CVM_INCLUDE_JUMP is deprecated. Use USE_JUMP instead.)
USE_JUMP = $(CVM_INCLUDE_JUMP)
endif

# Need to initialize J2ME_CLASSLIB before "all" rule below"
J2ME_CLASSLIB		?= cdc

# MIDP requires at least foundation
ifeq ($(USE_MIDP),true)
ifeq ($(J2ME_CLASSLIB), cdc)
ifeq ($(MAKELEVEL), 0)
$(warning Forcing J2ME_CLASSLIB=foundation because USE_MIDP=true requires foundation)
endif
J2ME_CLASSLIB = foundation
endif
endif

# JUMP requires at least basis
ifeq ($(USE_JUMP),true)
ifeq ($(J2ME_CLASSLIB), cdc)
override J2ME_CLASSLIB = basis
$(warning Forcing J2ME_CLASSLIB=basis because USE_JUMP=true requires basis)
endif
ifeq ($(J2ME_CLASSLIB), foundation)
override J2ME_CLASSLIB = basis
$(warning Forcing J2ME_CLASSLIB=basis because USE_JUMP=true requires basis)
endif
endif   # USE_JUMP

# The "all" rule below must come before pulling in any makefiles with rules
all:: printconfig checkconfig $(J2ME_CLASSLIB) tools

# Include TARGET top.mk
include $(CDC_OS_COMPONENT_DIR)/build/$(TARGET_OS)/top.mk

# Include all defs makefiles.
ifeq ($(USE_CDC_COM),true)
include $(CDC_COM_DIR)/build/share/defs.mk
endif
include  $(CDC_DIR)/build/share/defs.mk
-include $(CDC_DIR)/build/share/defs_midp.mk
-include $(CDC_DIR)/build/share/defs_jump.mk
-include $(CDC_DIR)/build/share/defs_gci.mk
-include $(CDC_DIR)/build/share/defs_op.mk
include $(PROFILE_DIR)/build/share/defs_$(J2ME_CLASSLIB).mk
ifneq ($(OPT_PKGS_DEFS_FILES),)
include $(OPT_PKGS_DEFS_FILES)
endif

# Include commercial components build definitions.
ifeq ($(USE_CDC_COM),true)
-include $(CDC_COM_DIR)/build/share/defs_cdc_com.mk
endif
ifneq ($(CVM_TOOLS_BUILD),true)
ifeq ($(CVM_STATICLINK_TOOLS),true)
# Include the makefiles for tool libraries to setup CVM_STATIC_TOOL_LIBS
# dependencies for linking the executable statically:
ifeq ($(CVM_JVMPI),true)
-include $(CDC_DIR)/build/share/defs_jcov.mk
-include $(CDC_OS_COMPONENT_DIR)/build/$(TARGET_OS)/defs_jcov.mk
-include $(CDC_DIR)/build/share/rules_jcov.mk
-include $(CDC_OS_COMPONENT_DIR)/build/$(TARGET_OS)/rules_jcov.mk
-include $(CDC_DIR)/build/share/defs_hprof.mk
-include $(CDC_OS_COMPONENT_DIR)/build/$(TARGET_OS)/defs_hprof.mk
-include $(CDC_DIR)/build/share/rules_hprof.mk
-include $(CDC_OS_COMPONENT_DIR)/build/$(TARGET_OS)/rules_hprof.mk
endif
ifeq ($(CVM_JVMTI),true)
-include $(CDC_DIR)/build/share/defs_jvmti_crw.mk
-include $(CDC_OS_COMPONENT_DIR)/build/$(TARGET_OS)/defs_jvmti_crw.mk
-include $(CDC_DIR)/build/share/defs_jvmti_hprof.mk
-include $(CDC_OS_COMPONENT_DIR)/build/$(TARGET_OS)/defs_jvmti_hprof.mk
-include $(CDC_DIR)/build/share/defs_jdwp.mk
-include $(CDC_OS_COMPONENT_DIR)/build/$(TARGET_OS)/defs_jdwp.mk
-include $(CDC_DIR)/build/share/defs_jdwp_transport.mk
-include $(CDC_OS_COMPONENT_DIR)/build/$(TARGET_OS)/defs_jdwp_transport.mk
-include $(CDC_DIR)/build/share/defs_nb_profiler.mk
-include $(CDC_OS_COMPONENT_DIR)/build/$(TARGET_OS)/defs_nb_profiler.mk
endif
endif
endif

# Include all rule makefiles. Since variables in rules are expanded
# eagerly, they must be included after defs makefiles.
include  $(CDC_DIR)/build/share/rules.mk
-include $(CDC_DIR)/build/share/rules_midp.mk
-include $(CDC_DIR)/build/share/rules_jump.mk
-include $(CDC_DIR)/build/share/rules_gci.mk
include $(PROFILE_DIR)/build/share/rules_$(J2ME_CLASSLIB).mk
ifneq ($(J2ME_PLATFORM),)
include $(PROFILE_DIR)/build/share/rules_$(J2ME_PLATFORM).mk
endif
ifneq ($(OPT_PKGS_RULES_FILES),)
include $(OPT_PKGS_RULES_FILES)
endif

# Include commercial components build rules.
ifeq ($(USE_CDC_COM),true)
-include $(CDC_COM_DIR)/build/share/rules_cdc_com.mk
endif


ifeq ($(CVM_TOOLS_BUILD),true)
# Include the makefiles for tool libraries to build here:
# NOTE: For jcov and hprof, the platform specific makefiles need to be
#       included first.
-include $(CDC_DIR)/build/share/defs_jcov.mk
-include $(CDC_OS_COMPONENT_DIR)/build/$(TARGET_OS)/defs_jcov.mk
-include $(CDC_DIR)/build/share/rules_jcov.mk
-include $(CDC_OS_COMPONENT_DIR)/build/$(TARGET_OS)/rules_jcov.mk
-include $(CDC_DIR)/build/share/defs_hprof.mk
-include $(CDC_OS_COMPONENT_DIR)/build/$(TARGET_OS)/defs_hprof.mk
-include $(CDC_DIR)/build/share/rules_hprof.mk
-include $(CDC_OS_COMPONENT_DIR)/build/$(TARGET_OS)/rules_hprof.mk
-include $(CDC_DIR)/build/share/defs_jvmti_crw.mk
-include $(CDC_OS_COMPONENT_DIR)/build/$(TARGET_OS)/defs_jvmti_crw.mk
-include $(CDC_DIR)/build/share/rules_jvmti_crw.mk
-include $(CDC_OS_COMPONENT_DIR)/build/$(TARGET_OS)/rules_jvmti_crw.mk
-include $(CDC_DIR)/build/share/defs_jvmti_hprof.mk
-include $(CDC_OS_COMPONENT_DIR)/build/$(TARGET_OS)/defs_jvmti_hprof.mk
-include $(CDC_DIR)/build/share/rules_jvmti_hprof.mk
-include $(CDC_OS_COMPONENT_DIR)/build/$(TARGET_OS)/rules_jvmti_hprof.mk
-include $(CDC_DIR)/build/share/defs_jdwp.mk
-include $(CDC_OS_COMPONENT_DIR)/build/$(TARGET_OS)/defs_jdwp.mk
-include $(CDC_DIR)/build/share/rules_jdwp.mk
-include $(CDC_OS_COMPONENT_DIR)/build/$(TARGET_OS)/rules_jdwp.mk
-include $(CDC_DIR)/build/share/defs_jdwp_transport.mk
-include $(CDC_OS_COMPONENT_DIR)/build/$(TARGET_OS)/defs_jdwp_transport.mk
-include $(CDC_DIR)/build/share/rules_jdwp_transport.mk
-include $(CDC_OS_COMPONENT_DIR)/build/$(TARGET_OS)/rules_jdwp_transport.mk
-include $(CDC_DIR)/build/share/defs_nb_profiler.mk
-include $(CDC_OS_COMPONENT_DIR)/build/$(TARGET_OS)/defs_nb_profiler.mk
-include $(CDC_DIR)/build/share/rules_nb_profiler.mk
-include $(CDC_OS_COMPONENT_DIR)/build/$(TARGET_OS)/rules_nb_profiler.mk
-include $(CDC_OS_COMPONENT_DIR)/build/$(TARGET_OS)/cvmc.mk
-include $(CDC_DIR)/build/share/cvmc.mk
endif

