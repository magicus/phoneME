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
# @(#)bundle.mk	1.34 06/10/25
#
# Makefile for building the source bundle.
#
# Targets (default is src.zip):
#   src.zip:      j2me source bundle. Renamed to <profile>-src.zip.
#   all:          src.zip target
#
# Options - defaults in parenthesis:
#   J2ME_CLASSLIB(cdc): profile to build source bundle for.
#   JAVAME_LEGAL_DIR: directory containing legal documents. If not set,
#   	the makefiles will automatically do an svn checkout of the files.
#   	from the respository. This will result in a password prompt if you
#   	haven't already authenticated with the repository.
#   SRC_BUNDLE_NAME: name of the source bundle, excluding .zip extension.
#	Defaults to $(J2ME_CLASSLIB)-src
#   SRC_BUNDLE_DIRNAME: directory name the source bundle will unzip
# 	into. Defaults to $(SRC_BUNDLE_NAME).
#   SRC_BUNDLE_APPEND_REVISION(true): Appends the repository
#	revision number to the end of SRC_BUNDLE_NAME.
#   BUNDLE_PORTS: OS/CPU ports to include (see below)
#   CVM_PRODUCT(oi): "oi" or "ri".
#   INCLUDE_JIT: true for oi builds. false for ri builds.
#   INCLUDE_DUALSTACK(false): Include cdc/cldc dual stack support.
#   INCLUDE_KNI(false): Include support for KNI methods
#   INCLUDE_COMMCONNECTION(true): Include CommConnection support.
#   INCLUDE_JCOV(false): Include support for JCOV
#   INCLUDE_MTASK: true for oi build. false for ri builds.
#   INCLUDE_GCI(false): Set true to include GCI makefiles
#   USE_CDC_COM(false): set true for commericial source bundles
#   CDC_COM_DIR: directory of cdc-com component
#
# BUNDLE_PORTS lists all the ports to include in the source bundle using
# the format <os>-<cpu>-<device>. Wildcards are supported, but you must
# specify something for each of the 3 parts that make up the name of the
# port. The following are examples of valid port names
#
#   BUNDLE_PORTS=linux-arm-netwinder
#   BUNDLE_PORTS="linux-arm-* linux-mips-*"
#   BUNDLE_PORTS="*-arm-*"
#   BUNDLE_PORTS="*-*-*"
#
# Defaults for BUNDLE_PORTS are:
#
#   linux-x86-suse:   for ri builds.
#   linux-arm-zaurus: for oi builds.
# 
# Example invocation:
#   gmake -f bundle.mk all BUNDLE_PORTS="linux-arm-*" \
#       INCLUDE_DUALSTACK=true J2ME_CLASSLIB=cdc CVM_PRODUCT=oi
#

default: src.zip
all:	 src.zip

empty:=
comma:= ,
space:= $(empty) $(empty)

ABSPATH 	= $(shell cd $(1); echo `pwd`)

CVM_TOP		:= ../..
CVM_TOP_ABS	:= $(call ABSPATH,$(CVM_TOP))
INSTALLDIR	:= $(CVM_TOP_ABS)/install
ZIP		= zip

USE_VERBOSE_MAKE	= false
J2ME_CLASSLIB		= cdc
SRC_BUNDLE_NAME		= $(J2ME_CLASSLIB)-src
SRC_BUNDLE_DIRNAME 	= $(SRC_BUNDLE_NAME)

# Add the svn revison number to SRC_BUNDLE_NAME if requested.
SRC_BUNDLE_APPEND_REVISION = true
ifeq ($(SRC_BUNDLE_APPEND_REVISION),true)
ifneq ($(wildcard .svn),.svn)
REVISION_NUMBER = UNKNOWN
else
REVISION_NUMBER = \
     $(shell svn info | grep "Revision:" | sed -e 's/Revision: \(.*\)/\1/')
override SRC_BUNDLE_DIRNAME := $(SRC_BUNDLE_DIRNAME)-rev$(REVISION_NUMBER)
override SRC_BUNDLE_NAME := $(SRC_BUNDLE_NAME)-rev$(REVISION_NUMBER)
endif
endif

CVM_PRODUCT = oi
INCLUDE_DUALSTACK	= false
INCLUDE_KNI		= $(INCLUDE_DUALSTACK)
INCLUDE_COMMCONNECTION  = true
INCLUDE_JCOV		= false
INCLUDE_GCI		= false
ifeq ($(CVM_PRODUCT),ri)
INCLUDE_JIT		= false
INCLUDE_MTASK		= false
else
INCLUDE_JIT		= true
INCLUDE_MTASK		= true
endif

# If mtask is enabled, include up to basis.
ifeq ($(INCLUDE_MTASK), true)
ifeq ($(J2ME_CLASSLIB), cdc)
override J2ME_CLASSLIB		= basis
endif
ifeq ($(J2ME_CLASSLIB), foundation)
override J2ME_CLASSLIB		= basis
endif
endif

# If dualstack is enabled, include up to foundation.
ifeq ($(INCLUDE_DUALSTACK), true)
ifeq ($(J2ME_CLASSLIB), cdc)
override J2ME_CLASSLIB		= foundation
endif
endif 

ifeq ($(J2ME_CLASSLIB),foundation)
INCLUDE_foundation = true
endif

ifeq ($(J2ME_CLASSLIB),basis)
INCLUDE_foundation	= true
INCLUDE_basis		= true
endif

ifeq ($(J2ME_CLASSLIB),personal)
INCLUDE_foundation	= true
INCLUDE_basis		= true
INCLUDE_personal	= true
endif

ifneq ($(J2ME_CLASSLIB),cdc)
ifneq ($(INCLUDE_foundation),true)
$(error "Invalid setting for J2ME_CLASSLIB: \"$(J2ME_CLASSLIB)\"")
endif
endif

#
# List all the ports you want to include in the source bundle using
# the format <os>-<cpu>-<device>. Wildcards are supported, but you must
# specify something for each of the 3 parts that makes up the name of the
# port. The following are examples of valid port names:
#
#  linux-arm-netwinder
#  linux-arm-*
#  *-arm-*
#  *-*-*
#
# If used on the command line and either more than one port is listed
# or * is used, then the entire string must be in quotes. For example:
#
#  gnumake -f bundle.mk BUNDLE_PORTS="linux-arm-* linux-x86-suse"
#

ifeq ($(CVM_PRODUCT),ri)
BUNDLE_PORTS = linux-x86-*
else
BUNDLE_PORTS = linux-x86-*
endif

# Do wildcard expansion of ports listed in BUNDLE_PORTS
override BUNDLE_PORTS := \
	$(addprefix $(CDC_DIR)/build/, $(BUNDLE_PORTS)) \
	$(addprefix $(CVM_TOP)/build/, $(BUNDLE_PORTS))
override BUNDLE_PORTS := $(foreach port, $(BUNDLE_PORTS), $(wildcard $(port)))

# list of all device ports in the form <os>-<cpu>-<device>
BUNDLE_DEVICE_PORTS = $(notdir $(BUNDLE_PORTS))

# list of all OS ports in the form <os>
BUNDLE_OS_PORTS = \
	$(sort $(foreach port, $(BUNDLE_DEVICE_PORTS), \
			       $(word 1,$(subst -,$(space),$(port)))))
# list of all CPU ports in the form <cpu>
BUNDLE_CPU_PORTS = \
	$(sort $(foreach port, $(BUNDLE_DEVICE_PORTS), \
			       $(word 2,$(subst -,$(space),$(port)))))

# list of all platform ports in the form <os>-<cpu>
BUNDLE_PLATFORM_PORTS = \
	$(sort $(foreach port, \
	        $(BUNDLE_DEVICE_PORTS), \
	        $(word 1,$(subst -,$(space),$(port)))-$(word 2,$(subst -,$(space),$(port)))))

####################################################
# File and directory patterns to include and exclude
####################################################

EXCLUDE_PATTERNS += \
	*SCCS/* \
	*/.svn/* \
	*gunit*

# cvm

ifneq ($(CVM_PRODUCT),oi)
EXCLUDE_PATTERNS += \
        *gc/generational-seg/* \
        *gc/marksweep/* \
        *gc/semispace/* \
        */executejava_aligned.c \
        */executejava_split1.c \
        */executejava_split2.c
endif

EXCLUDE_PATTERNS +=		\
	*/mem_mgr*

BUILDDIR_PATTERNS += \
	GNUmakefile \
	top.mk \
	defs.mk \
	rules.mk \
	*_jdwp*.mk \
	*_jvmti*.mk \
	*_hprof.mk \
	*_jcov.mk \
	*_nb_profiler.mk

SRCDIR_PATTERNS += \
	javavm

BUNDLE_INCLUDE_LIST += \
	src/portlibs \
	build/portlibs/* \
	build/share/bundle.mk \
	build/share/jcc.mk \
	build/share/*_op.mk \
	src/share/tools/GenerateCurrencyData \
	src/share/tools/javazic \
	src/share/tools/xml \
	src/share/tools/sha1 \
	src/share/lib/security \
	src/share/lib/profiles \
	$(foreach os,$(BUNDLE_OS_PORTS), \
		src/$(os)/bin) \
	$(foreach os,$(BUNDLE_OS_PORTS) share, \
		src/$(os)/tools/hprof) \
	$(foreach os,$(BUNDLE_OS_PORTS) share, \
		src/$(os)/tools/jpda) \
	$(foreach os,$(BUNDLE_OS_PORTS) share, \
		src/$(os)/tools/jvmti) \
	$(foreach os,$(BUNDLE_OS_PORTS), \
		src/$(os)/lib/tzmappings) \
	$(foreach os,$(BUNDLE_OS_PORTS), \
		src/$(os)/lib/content-types.properties)

# For Windows Build
ifeq ($(findstring win32,$(BUNDLE_OS_PORTS)),win32)
CVM_INCLUDE_WIN32_HOST_DEFS_MK = true
BUNDLE_INCLUDE_LIST +=				\
	build/win32/ppc*_defs.mk \
	build/win32/wm5*_defs.mk \
	build/win32/*wince*.mk \
	build/win32/vc*_defs.mk
endif

# For Symbian Build
ifeq ($(findstring symbian,$(BUNDLE_OS_PORTS)),symbian)
CVM_INCLUDE_WIN32_HOST_DEFS_MK = true
BUNDLE_INCLUDE_LIST +=				\
	build/symbian/fix_project.pl \
	build/symbian/root.sh \
	build/symbian/winsim.mk \
	src/symbian/lib/cvm_exports*
endif

ifeq ($(CVM_INCLUDE_WIN32_HOST_DEFS_MK),true)
BUNDLE_INCLUDE_LIST +=				\
	build/win32/host_defs.mk 
endif

# dual stack

ifeq ($(INCLUDE_DUALSTACK), true)

BUNDLE_INCLUDE_LIST += \
	src/share/lib/dualstack \
	src/share/tools/RomConfProcessor

BUILDDIR_PATTERNS += \
       *_midp.mk

else

EXCLUDE_PATTERNS += 			\
	*/AuxPreloadClassLoader.c		\
	*/AuxPreloadClassLoader.java		\
	*/auxPreloader.c			\
	*/MemberFilter.c			\
	*/MemberFilter.java			\
	*/MemberFilterConfig.java		\
	*/MIDletClassLoader.java		\
	*/MIDPConfig.java			\
	*/MIDPImplementationClassLoader.java	\
	*/MIDPFilterConfig.txt			\
	*/MIDPPermittedClasses.txt		\
	*/test/dualstack			\
	*/test/dualstack/*
endif

# kni support

ifneq ($(INCLUDE_KNI), true)

EXCLUDE_PATTERNS += 	\
	*KNI*		\
	*kni*		\
	*sni_impl*
endif

# jit

ifeq ($(INCLUDE_JIT), true)

BUNDLE_INCLUDE_LIST += \
	build/share/jcs.mk \

else

EXCLUDE_PATTERNS += \
	*jit* \
	*ccm* \
	*jcs/* \
	*segvhandler*

endif

# CommConnection directories

ifneq ($(INCLUDE_COMMCONNECTION),true)
EXCLUDE_PATTERNS += \
       *share/classes/com/sun/cdc/io/j2me/comm/* \
       *linux/native/com/sun/cdc/io/j2me/comm/* \
       *solaris/native/com/sun/cdc/io/j2me/comm/* \
       *win32/native/com/sun/cdc/io/j2me/comm/*
endif

# jcov suport

ifeq ($(INCLUDE_JCOV), true)

BUNDLE_INCLUDE_LIST += 		\
	build/share/jcov*.mk	\
	src/share/tools/jcov

BUNDLE_INCLUDE_LIST += \
	$(foreach os,$(BUNDLE_OS_PORTS),src/$(os)/tools/jcov)

BUNDLE_INCLUDE_LIST += \
	$(foreach os,$(BUNDLE_OS_PORTS),build/$(os)/jcov.mk)

endif

# MTask support

ifeq ($(INCLUDE_MTASK), true)

BUNDLE_INCLUDE_LIST += \
	build/share/cvmc.mk \
	src/share/tools/cvmc  

BUILDDIR_PATTERNS += \
       *_jump.mk

# Add every build/<os>/cvmc.mk file
BUNDLE_INCLUDE_LIST += \
	$(foreach os,$(BUNDLE_OS_PORTS),build/$(os)/cvmc.mk)

else

EXCLUDE_PATTERNS += \
	*mtask* \

endif

# gci

ifeq ($(INCLUDE_GCI), true)

BUILDDIR_PATTERNS += \
	defs_gci.mk \
	rules_gci.mk

endif

# cdc

BUILDDIR_PATTERNS += \
	*cdc*.mk \

SRCDIR_PATTERNS += \
	cdc \
	classes \
	native

BUNDLE_INCLUDE_LIST += \
	build/share/*_zoneinfo.mk \
	test/share/cdc

# foundation

ifeq ($(INCLUDE_foundation), true)

BUILDDIR_PATTERNS += \
	*foundation*.mk

SRCDIR_PATTERNS += \
	foundation

BUNDLE_INCLUDE_LIST += \
	test/share/foundation

endif

# basis

ifeq ($(INCLUDE_basis), true)

BUILDDIR_PATTERNS += \
	defs_qt.mk \
	*_basis.mk \
	*_basis_qt.mk

BUNDLE_INCLUDE_LIST += \
	test/share/basis \
	src/share/basis/lib/security \
	src/share/basis/demo \
	src/share/basis/native/image/jpeg \
	src/share/basis/native/image/gif \
	src/share/basis/classes/common \
	src/share/basis/native/awt/qt \
	src/share/basis/classes/awt/qt

endif

# personal

ifeq ($(INCLUDE_personal), true)

BUILDDIR_PATTERNS += \
	defs_qt.mk \
	*_basis.mk \
	*_personal.mk \
	*_personal_peer_based.mk \
	*_personal_qt.mk

BUNDLE_INCLUDE_LIST += \
	test/share/basis \
	src/share/basis/demo \
	src/share/basis/native/image/jpeg \
	src/share/basis/native/image/gif \
	src/share/basis/classes/common

BUNDLE_INCLUDE_LIST += \
	test/share/personal \
	src/share/personal/demo \
	src/share/personal/lib/security \
	src/share/personal/classes/awt/peer_based/java \
	src/share/personal/classes/awt/peer_based/sun/awt/*.java \
	src/share/personal/classes/awt/peer_based/sun/awt/peer \
	src/share/personal/classes/awt/peer_based/sun/awt/image \
	src/share/personal/classes/awt/peer_based/sun/awt/qt \
	src/share/personal/classes/common \
	src/share/personal/native/sun/awt/common \
	src/share/personal/native/awt/qt \
	$(foreach os,$(BUNDLE_OS_PORTS), \
		src/$(os)/personal/native/sun/audio) \

ifeq ($(findstring linux-arm-zaurus,$(BUNDLE_DEVICE_PORTS)),linux-arm-zaurus)
BUNDLE_INCLUDE_LIST +=				\
	src/linux-arm-zaurus/personal/qt
else
EXCLUDE_PATTERNS += \
	*/demo/zaurus*
endif

endif

# The VxWorks port needs the build/vxworks/target directory

ifeq ($(findstring vxworks,$(BUNDLE_OS_PORTS)),vxworks)
BUNDLE_INCLUDE_LIST +=				\
	build/vxworks/target
endif

######################
# legal files
######################

# The OSS repository legal directory
BUNDLE_INCLUDE_LIST += legal

JAVAME_LEGAL_DIR ?= $(COMPONENTS_DIR)/legal

# Make sure the "legal" directory is available.
CHECK_LEGAL_DIR = \
	if [ ! -d $(JAVAME_LEGAL_DIR) ]; then				\
	    echo '*** JAVAME_LEGAL_DIR must be set to the "legal"'	\
	         'directory. The OSS version can be found at'		\
		 'https://phoneme.dev.java.net/svn/phoneme/legal.';	\
	    exit 2;							\
	fi

########################
# Build the include list
########################

# Add every build directory pattern 
BUNDLE_INCLUDE_LIST += \
	$(foreach pat, $(BUILDDIR_PATTERNS), \
	  $(addsuffix /$(pat), \
	    build/share \
	    $(addprefix build/,$(BUNDLE_DEVICE_PORTS)) \
	    $(addprefix build/,$(BUNDLE_PLATFORM_PORTS)) \
	    $(addprefix build/,$(BUNDLE_OS_PORTS)) \
	    $(addprefix build/,$(BUNDLE_CPU_PORTS)))) \

# Add every src directory pattern 
BUNDLE_INCLUDE_LIST += \
	$(foreach pat, $(SRCDIR_PATTERNS), \
	  $(addsuffix /$(pat), \
	    src/share \
	    $(addprefix src/,$(BUNDLE_PLATFORM_PORTS)) \
	    $(addprefix src/,$(BUNDLE_OS_PORTS)) \
	    $(addprefix src/,$(BUNDLE_CPU_PORTS)))) \

# Prefix $(SRC_BUNDLE_DIRNAME) to all names in BUNDLE_INCLUDE_LIST
BUNDLE_INCLUDE_LIST := \
	 $(patsubst %,$(SRC_BUNDLE_DIRNAME)/%,$(BUNDLE_INCLUDE_LIST))

BUNDLE_INCLUDE_LIST := $(sort $(BUNDLE_INCLUDE_LIST))

################################################
# Rules for building source bundles
################################################

FEATURE_LIST += J2ME_CLASSLIB \
	CVM_PRODUCT \
	INCLUDE_JIT \
	INCLUDE_MTASK \
	INCLUDE_KNI \
	INCLUDE_JCOV \
	INCLUDE_GCI \
	INCLUDE_DUALSTACK \
	INCLUDE_COMMCONNECTION

FEATURE_LIST_WITH_VALUES += \
	$(foreach feature,$(strip $(FEATURE_LIST)), "$(feature)=$($(feature))")


ifneq ($(USE_VERBOSE_MAKE), true)
SVN_QUIET_CHECKOUT = -q
endif

lib-src: src.zip
src.zip::
ifeq ($(USE_VERBOSE_MAKE), true)
	@echo ">>>FLAGS:"
	@echo "	SRC_BUNDLE_APPEND_REVISION = $(SRC_BUNDLE_APPEND_REVISION)"
	@echo "	SRC_BUNDLE_NAME		= $(SRC_BUNDLE_NAME)"
	@echo "	SRC_BUNDLE_DIRNAME	= $(SRC_BUNDLE_DIRNAME)"
	@echo "	JAVAME_LEGAL_DIR	= $(JAVAME_LEGAL_DIR)"

	@echo ">>>Making "$@" for the following devices:"
	@for s in "$(BUNDLE_DEVICE_PORTS)" ; do \
		printf "\t%s\n" $$s; \
	done

	@echo ">>>Supported OS ports:"
	@for s in "$(BUNDLE_OS_PORTS)" ; do \
		printf "\t%s\n" $$s; \
	done

	@echo ">>>Supported CPU ports:"
	@for s in "$(BUNDLE_CPU_PORTS)" ; do \
		printf "\t%s\n" $$s; \
	done

	@echo ">>>Supported features:"
	@for f in $(FEATURE_LIST_WITH_VALUES); do \
		formattedF=`echo $$f | sed 's/=/:		/'`; \
		printf "\t%s\n" "$$formattedF" ; \
	done
endif

	$(AT)rm -rf $(INSTALLDIR)/$(SRC_BUNDLE_DIRNAME)
	$(AT)mkdir -p $(INSTALLDIR)/$(SRC_BUNDLE_DIRNAME)
	$(AT)ln -ns $(CDC_DIR)/* $(INSTALLDIR)/$(SRC_BUNDLE_DIRNAME)
	$(AT)rm -rf $(INSTALLDIR)/$(SRC_BUNDLE_NAME).zip

	$(AT)$(CHECK_LEGAL_DIR)
	$(AT)ln -ns $(JAVAME_LEGAL_DIR) \
		$(INSTALLDIR)/$(SRC_BUNDLE_DIRNAME)/legal

	$(AT)(cd $(INSTALLDIR); \
	 $(ZIP) -r -q \
		$(INSTALLDIR)/$(SRC_BUNDLE_NAME).zip \
		$(BUNDLE_INCLUDE_LIST) -x $(EXCLUDE_PATTERNS))
	$(AT)rm -rf $(INSTALLDIR)/$(SRC_BUNDLE_DIRNAME)

ifeq ($(USE_CDC_COM),true)
	$(AT)mkdir -p $(INSTALLDIR)/$(SRC_BUNDLE_DIRNAME)/build/share
ifdef CDC_PROJECT
# copy id_project.mk so it can be added to the zip file
	$(AT)cp $(CDC_COM_DIR)/projects/$(CDC_PROJECT)/build/share/id_project.mk \
		$(INSTALLDIR)/$(SRC_BUNDLE_DIRNAME)/build/share
# Add the id_project.mk to the zip file
	$(AT)(cd $(INSTALLDIR); \
	      $(ZIP) -r -q \
		$(INSTALLDIR)/$(SRC_BUNDLE_NAME).zip \
		$(SRC_BUNDLE_DIRNAME)/build/share/id_project.mk)
endif
# copy cdc-com version of defs_qt.mk so it can be added to the zip file
ifeq ($(findstring defs_qt.mk,$(BUILDDIR_PATTERNS)),defs_qt.mk)
	$(AT)cp $(CDC_COM_DIR)/build/share/defs_qt.mk \
		$(INSTALLDIR)/$(SRC_BUNDLE_DIRNAME)/build/share
# Add the commercial defs_qt.mk to the zip file
	$(AT)(cd $(INSTALLDIR); \
	      $(ZIP) -r -q \
		$(INSTALLDIR)/$(SRC_BUNDLE_NAME).zip \
		$(SRC_BUNDLE_DIRNAME)/build/share/defs_qt.mk)
endif
	$(AT)rm -rf $(INSTALLDIR)/$(SRC_BUNDLE_DIRNAME)
endif

ifeq ($(USE_VERBOSE_MAKE), true)
	@echo "<<<Finished "$@" ..." ;
endif

#
# Include any commercial-specific rules and defs
#
-include bundle-commercial.mk

