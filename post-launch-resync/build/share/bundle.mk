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
# @(#)bundle.mk	1.34 06/10/25
#
# Makefile for building the source bundle.
#
# Targets (default is src.zip):
#   src.zip:      j2me source bundle. Renamed to <profile>-src.zip.
#   jcov-src.zip: JCOV source bundle
#   all:          both of the above
#
# Options - defaults in parenthesis:
#   J2ME_CLASSLIB(cdc): profile to build source bundle for
#   BUNDLE_PORTS: OS/CPU ports to include (see below)
#   CVM_PRODUCT(oi): "oi" or "ri".
#   INCLUDE_JIT: true for oi builds. false for ri builds.
#   INCLUDE_DUALSTACK(false): Include cdc/cldc dual stack support.
#   INCLUDE_KNI(false): Include support for KNI methods
#   INCLUDE_COMMCONNECTION(true): Include CommConnection support.
#   INCLUDE_MTASK: true for oi build. false for ri builds.
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
all:	 src.zip jcov-src.zip

empty:=
comma:= ,
space:= $(empty) $(empty)

CVM_TOP 	?= ../..
INSTALLDIR	= $(CVM_TOP)/install
ZIP		= zip

J2ME_CLASSLIB	= cdc
J2ME_PRODUCT_NAME = $(J2ME_CLASSLIB)

CVM_PRODUCT = oi
INCLUDE_DUALSTACK	= false
INCLUDE_KNI		= $(INCLUDE_DUALSTACK)
INCLUDE_COMMCONNECTION  = true
ifeq ($(CVM_PRODUCT),ri)
INCLUDE_JIT		= false
INCLUDE_MTASK		= false
else
INCLUDE_JIT		= true
INCLUDE_MTASK		= true
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
override BUNDLE_PORTS := $(addprefix $(CVM_TOP)/build/, $(BUNDLE_PORTS))
override BUNDLE_PORTS := $(foreach port, $(BUNDLE_PORTS), $(wildcard $(port)))
override BUNDLE_DEVICE_PORTS := $(notdir $(BUNDLE_PORTS))

# list of all device ports in the form <os>-<cpu>-<device>
BUNDLE_DEVICE_PORTS = $(foreach port, $(BUNDLE_PORTS), \
	$(port)					\
	$(word 1, $(subst -,$(space),$(port)))	\
	$(word 2, $(subst -,$(space),$(port)))	\
	$(word 1, $(subst -,$(space),$(port)))-$(word 2, $(subst -,$(space),$(port))))

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
	*/JavaAPILister.java	\
	*/mem_mgr*

BUILDDIR_PATTERNS += \
	GNUmakefile \
	top.mk \
	defs.mk \
	rules.mk \
	jdwp*.mk \
	hprof.mk

SRCDIR_PATTERNS += \
	javavm

BUNDLE_INCLUDE_LIST += \
	$(J2ME_PRODUCT_NAME)/src/portlibs \
	$(J2ME_PRODUCT_NAME)/build/portlibs/* \
	$(J2ME_PRODUCT_NAME)/build/share/jcc.mk \
	$(J2ME_PRODUCT_NAME)/src/share/tools/GenerateCurrencyData \
	$(J2ME_PRODUCT_NAME)/src/share/tools/javazic \
	$(J2ME_PRODUCT_NAME)/src/share/lib/security \
	$(foreach os,$(BUNDLE_OS_PORTS), \
		$(J2ME_PRODUCT_NAME)/src/$(os)/bin) \
	$(foreach os,$(BUNDLE_OS_PORTS) share, \
		$(J2ME_PRODUCT_NAME)/src/$(os)/tools/hprof) \
	$(foreach os,$(BUNDLE_OS_PORTS) share, \
		$(J2ME_PRODUCT_NAME)/src/$(os)/tools/jpda) \
	$(foreach os,$(BUNDLE_OS_PORTS), \
		$(J2ME_PRODUCT_NAME)/src/$(os)/lib/tzmappings) \
	$(foreach os,$(BUNDLE_OS_PORTS), \
		$(J2ME_PRODUCT_NAME)/src/$(os)/lib/content-types.properties)

# need to include special java.security for zaurus

ifeq ($(findstring linux-arm-zaurus,$(BUNDLE_DEVICE_PORTS)),linux-arm-zaurus)
BUNDLE_INCLUDE_LIST += \
	$(J2ME_PRODUCT_NAME)/src/linux-arm-zaurus/lib/security/java.security
endif

# For Windows Build
ifeq ($(findstring win32,$(BUNDLE_OS_PORTS)),win32)
BUNDLE_INCLUDE_LIST +=				\
	$(J2ME_PRODUCT_NAME)/build/win32/ppc*_defs.mk \
	$(J2ME_PRODUCT_NAME)/build/win32/*wince*.mk \
	$(J2ME_PRODUCT_NAME)/build/win32/vc*_defs.mk \
	$(J2ME_PRODUCT_NAME)/build/win32/host_defs.mk 
endif

# For Symbian Build
ifeq ($(findstring symbian,$(BUNDLE_OS_PORTS)),symbian)
BUNDLE_INCLUDE_LIST +=				\
	$(J2ME_PRODUCT_NAME)/build/symbian/fix_project.pl \
	$(J2ME_PRODUCT_NAME)/build/symbian/root.sh \
	$(J2ME_PRODUCT_NAME)/build/symbian/winsim.mk \
	$(J2ME_PRODUCT_NAME)/src/symbian/lib/cvm_exports*
endif

# dual stack

ifeq ($(INCLUDE_DUALSTACK), true)

BUNDLE_INCLUDE_LIST += \
	$(J2ME_PRODUCT_NAME)/src/share/lib/MIDP*

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
	$(J2ME_PRODUCT_NAME)/build/share/jcs.mk \

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

# MTask support

ifeq ($(INCLUDE_MTASK), true)

BUNDLE_INCLUDE_LIST += \
	$(J2ME_PRODUCT_NAME)/build/share/cvmc.mk \
	$(J2ME_PRODUCT_NAME)/src/share/tools/cvmc \

# Add every build/<os>/cvmc.mk file
BUNDLE_INCLUDE_LIST += \
	$(foreach os,$(BUNDLE_OS_PORTS),build/$(os)/cvmc.mk)

else

EXCLUDE_PATTERNS += \
	*mtask* \

endif

# cdc

BUILDDIR_PATTERNS += \
	*cdc*.mk \

SRCDIR_PATTERNS += \
	cdc \
	classes \
	native

BUNDLE_INCLUDE_LIST += \
	$(J2ME_PRODUCT_NAME)/build/share/*_zoneinfo.mk \
	$(J2ME_PRODUCT_NAME)/test/share/cdc

# foundation

ifeq ($(INCLUDE_foundation), true)

BUILDDIR_PATTERNS += \
	*foundation*.mk

SRCDIR_PATTERNS += \
	foundation

BUNDLE_INCLUDE_LIST += \
	$(J2ME_PRODUCT_NAME)/test/share/foundation

endif

# basis

ifeq ($(INCLUDE_basis), true)

BUILDDIR_PATTERNS += \
	defs_qt.mk \
	*_basis.mk \
	*_basis_qt.mk

BUNDLE_INCLUDE_LIST += \
	$(J2ME_PRODUCT_NAME)/test/share/basis \
	$(J2ME_PRODUCT_NAME)/src/share/basis/lib/security \
	$(J2ME_PRODUCT_NAME)/src/share/basis/demo \
	$(J2ME_PRODUCT_NAME)/src/share/basis/native/image/jpeg \
	$(J2ME_PRODUCT_NAME)/src/share/basis/native/image/gif \
	$(J2ME_PRODUCT_NAME)/src/share/basis/classes/common \
	$(J2ME_PRODUCT_NAME)/src/share/basis/native/awt/qt \
	$(J2ME_PRODUCT_NAME)/src/share/basis/classes/awt/qt

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
	$(J2ME_PRODUCT_NAME)/test/share/basis \
	$(J2ME_PRODUCT_NAME)/src/share/basis/demo \
	$(J2ME_PRODUCT_NAME)/src/share/basis/native/image/jpeg \
	$(J2ME_PRODUCT_NAME)/src/share/basis/native/image/gif \
	$(J2ME_PRODUCT_NAME)/src/share/basis/classes/common

BUNDLE_INCLUDE_LIST += \
	$(J2ME_PRODUCT_NAME)/test/share/personal \
	$(J2ME_PRODUCT_NAME)/src/share/personal/demo \
	$(J2ME_PRODUCT_NAME)/src/share/personal/lib/security \
	$(J2ME_PRODUCT_NAME)/src/share/personal/classes/awt/peer_based/java \
	$(J2ME_PRODUCT_NAME)/src/share/personal/classes/awt/peer_based/sun/awt/*.java \
	$(J2ME_PRODUCT_NAME)/src/share/personal/classes/awt/peer_based/sun/awt/peer \
	$(J2ME_PRODUCT_NAME)/src/share/personal/classes/awt/peer_based/sun/awt/image \
	$(J2ME_PRODUCT_NAME)/src/share/personal/classes/awt/peer_based/sun/awt/qt \
	$(J2ME_PRODUCT_NAME)/src/share/personal/classes/common \
	$(J2ME_PRODUCT_NAME)/src/share/personal/native/sun/awt/common \
	$(J2ME_PRODUCT_NAME)/src/share/personal/native/awt/qt \
	$(foreach os,$(BUNDLE_OS_PORTS), \
		$(J2ME_PRODUCT_NAME)/src/$(os)/personal/native/sun/audio) \

ifeq ($(findstring linux-arm-zaurus,$(BUNDLE_DEVICE_PORTS)),linux-arm-zaurus)
BUNDLE_INCLUDE_LIST +=				\
	$(J2ME_PRODUCT_NAME)/src/linux-arm-zaurus/personal/qt
else
EXCLUDE_PATTERNS += \
	*/demo/zaurus*
endif

endif

# The VxWorks port needs the build/vxworks/target directory

ifeq ($(findstring vxworks,$(BUNDLE_OS_PORTS)),vxworks)
BUNDLE_INCLUDE_LIST +=				\
	$(J2ME_PRODUCT_NAME)/build/vxworks/target
endif

########################
# Build the include list
########################

# Add every build directory pattern 
BUNDLE_INCLUDE_LIST += \
	$(foreach pat, $(BUILDDIR_PATTERNS), \
	  $(addsuffix /$(pat), \
	    $(J2ME_PRODUCT_NAME)/build/share \
	    $(addprefix $(J2ME_PRODUCT_NAME)/build/,$(BUNDLE_DEVICE_PORTS)) \
	    $(addprefix $(J2ME_PRODUCT_NAME)/build/,$(BUNDLE_PLATFORM_PORTS)) \
	    $(addprefix $(J2ME_PRODUCT_NAME)/build/,$(BUNDLE_OS_PORTS)) \
	    $(addprefix $(J2ME_PRODUCT_NAME)/build/,$(BUNDLE_CPU_PORTS)))) \

# Add every src directory pattern 
BUNDLE_INCLUDE_LIST += \
	$(foreach pat, $(SRCDIR_PATTERNS), \
	  $(addsuffix /$(pat), \
	    $(J2ME_PRODUCT_NAME)/src/share \
	    $(addprefix $(J2ME_PRODUCT_NAME)/src/,$(BUNDLE_PLATFORM_PORTS)) \
	    $(addprefix $(J2ME_PRODUCT_NAME)/src/,$(BUNDLE_OS_PORTS)) \
	    $(addprefix $(J2ME_PRODUCT_NAME)/src/,$(BUNDLE_CPU_PORTS)))) \

################################################
# Rules for building source bundles
################################################

BUNDLE_INCLUDE_LIST := $(sort $(BUNDLE_INCLUDE_LIST))

FEATURE_LIST += J2ME_CLASSLIB \
	CVM_PRODUCT \
	INCLUDE_JIT \
	INCLUDE_MTASK \
	INCLUDE_KNI \
	INCLUDE_DUALSTACK \
	INCLUDE_COMMCONNECTION

FEATURE_LIST_WITH_VALUES += \
	$(foreach feature,$(strip $(FEATURE_LIST)), "$(feature)=$($(feature))")

lib-src: src.zip
src.zip::
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
		formattedF=`echo $$f | sed 's/=/:\t\t/'`; \
		printf "\t%s\n" "$$formattedF" ; \
	done

	rm -rf $(INSTALLDIR)/$(J2ME_CLASSLIB)
	mkdir -p $(INSTALLDIR)/$(J2ME_CLASSLIB)
	ln -ns $(CVM_TOP)/* $(INSTALLDIR)/$(J2ME_CLASSLIB)
	rm -rf $(INSTALLDIR)/$(J2ME_CLASSLIB)-src.zip

	(cd $(INSTALLDIR); \
	 $(ZIP) -r -q - $(BUNDLE_INCLUDE_LIST) \
		-x $(EXCLUDE_PATTERNS)) \
		> $(INSTALLDIR)/$(J2ME_CLASSLIB)-src.zip;
	rm -rf $(INSTALLDIR)/$(J2ME_CLASSLIB)
	@echo "<<<Finished "$@" ..." ;

#######
# JCOV
#######

#
# All jcov bundles include these directories
#
BUNDLE_JCOV_LIST += 				\
	build/share/jcov*.mk	\
	src/share/tools/jcov

# Add every src/<os>/tools/jcov directory
BUNDLE_JCOV_LIST += \
	$(foreach os,$(BUNDLE_OS_PORTS),src/$(os)/tools/jcov)

# Add every build/<os>/jcov.mk file
BUNDLE_JCOV_LIST += \
	$(foreach os,$(BUNDLE_OS_PORTS),build/$(os)/jcov.mk)

jcov-src: jcov-src.zip
jcov-src.zip::
	@echo ">>>Making "$@" ..." ;
	mkdir -p $(INSTALLDIR)
	rm -rf $(INSTALLDIR)/jcov-src.zip
	(cd $(CVM_TOP); \
	 $(ZIP) -r -q  - $(BUNDLE_JCOV_LIST) -x "*SCCS/*" -x "*/.svn/*") \
		 > $(INSTALLDIR)/jcov-src.zip;
	@rm -rf $<;
	@echo "<<<Finished "$@" ..." ;

#
# Include any commercial-specific rules and defs
#
-include bundle-commercial.mk

