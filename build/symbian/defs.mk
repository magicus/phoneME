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
# @(#)defs.mk	1.19 06/10/10
#
# defs for Symbian target
#

# Build CVM as a DLL by default.
CVM_DLL ?= true

CVM_FLAGS += \
	CVM_DLL \

CVM_DLL_CLEANUP_ACTION = $(CVM_SYMBIAN_CLEANUP_ACTION)

ifeq ($(HOST_DEVICE), Interix)
CVM_JAVAC       = javac.exe
CVM_JAVAH       = javah.exe
CVM_JAR         = jar.exe
endif

ifeq ($(origin EPOCDEVICE), undefined)
SYMBIAN_DEVICE := S60_2nd_FP2:com.nokia.Series60
else
SYMBIAN_DEVICE := $(EPOCDEVICE)
endif

# Environment for running the "devices" command, which root.sh runs
# to get information on SYMBIAN_DEVICE

SYSTEMDRIVE ?= C:
SYSTEM_ROOT_U = $(call WIN2POSIX,$(SYSTEMDRIVE))
SYMBIAN_PERL_PATH ?= $(SYSTEM_ROOT_U)/Perl/bin
SYMBIAN_TOOLS_PATH ?= $(SYSTEM_ROOT_U)/Program Files/Common Files/Symbian/Tools

SYMBIAN_DEVICES_PATH = $(SYMBIAN_PERL_PATH):$(SYMBIAN_TOOLS_PATH)
SYMBIAN_ENV_CMD := env PATH="$(SYMBIAN_DEVICES_PATH):$$PATH"

SYMBIAN_ROOT := $(shell $(SYMBIAN_ENV_CMD) sh ../symbian/root.sh $(SYMBIAN_DEVICE))
SYMBIAN_ROOT_U := $(call WIN2POSIX,'$(SYMBIAN_ROOT)')

# Environment for a specific SYMBIAN_DEVICE
EPOC = $(SYMBIAN_ROOT)/epoc32
EPOC_U = $(SYMBIAN_ROOT_U)/epoc32

SYMBIAN_DEV_CMD = env PATH="$(SYMBIAN_DEVICES_PATH):$$PATH" \
	cmd /c \
	"$(call POSIX2WIN,$(1))" \
	@$(SYMBIAN_DEVICE) $(2)

ifeq ($(CVM_DEBUG), true)
SYMBIAN_BLD		?= udeb
else
SYMBIAN_BLD		?= urel
endif

# SYMBIAN_TARGET specifies the CVM binary type.
ifeq ($(CVM_DLL),true)
SYMBIAN_TARGET		= DLL
else
SYMBIAN_TARGET		= EXE
endif

# CVM_DEFFILE specifies the .def file that contains exported functions.
CVM_DEFFILE		= cvm_exports_$(J2ME_CLASSLIB).def

CVM_TARGETROOT	= $(CVM_TOP)/src/$(TARGET_OS)

CVM_DEFINES	+= -D_GNU_SOURCE -DCVM_JNI_C_INTERFACE
ifeq ($(CVM_DLL),true)
CVM_DEFINES	+= -DCVM_DLL
endif

# We don't want to create ROMJAVA_O (see share/jcc.mk) when building
# for Symbian.
ROMJAVA_AR_CMD	=

#
# Platform specific source directories
#
CVM_SRCDIRS   += $(CVM_TOP)/src/portlibs/ansi_c
CVM_SRCDIRS   += $(CVM_TOP)/src/portlibs/posix
CVM_SRCDIRS   += $(CVM_TOP)/src/portlibs/realpath
CVM_SRCDIRS   += \
	$(CVM_TARGETROOT)/javavm/runtime \
	$(CVM_TARGETROOT)/bin \
	$(CVM_TARGETROOT)/native/java/lang \
	$(CVM_TARGETROOT)/native/java/io \
	$(CVM_TARGETROOT)/native/java/net \
	$(CVM_TARGETROOT)/native/common \
	$(CVM_TARGETROOT)/native/com/sun/cdc/io/j2me/comm \

CVM_INCLUDE_DIRS  += \
	$(CVM_TOP)/src \
	$(CVM_TARGETROOT) \
	$(CVM_TARGETROOT)/native/java/net \
	$(CVM_TARGETROOT)/native/common \
	$(CVM_TARGETROOT)/native/$(J2ME_CLASSLIB) \

#
# Platform specific objects
#

CVM_TARGETOBJS_SPACE += \
	java_md.o \
	ansi_java_md.o \
	canonicalize_md.o \
	io_md.o \
	posix_io_md.o \
	net_md.o \
	time_md.o \
	io_util.o \
	sync_md.o \
	system_md.o \
	threads_md.o \
	globals_md.o \
	java_props_md.o \
	memory_md.o \
	cvmi_md.o \

ifeq ($(CVM_JIT), true)
CVM_SRCDIRS   += \
	$(CVM_TARGETROOT)/javavm/runtime/jit
CVM_TARGETOBJS_SPACE += \
	jit_md.o
endif

ifeq ($(CVM_DYNAMIC_LINKING), true)
	CVM_TARGETOBJS_SPACE += linker_md.o cvm_lookup.o
endif

ifeq ($(CVM_PROVIDE_TARGET_RULES), true)
	CVM_TARGETOBJS_SPACE += threads_md.o
endif

#
# FIXME: It looks like the objects in X_MATHOBJS needs to be included
# for the Symbian target build.
#
MATHOBJS += \
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
	w_sinh.o


CVM_SYMBIAN_CLEANUP_ACTION	= \
	if [ -f $(ABLD_BAT) ]; then \
	    $(call SYMBIAN_DEV_CMD,$(ABLD_BAT) reallyclean $(SYMBIAN_PLATFORM)); \
	fi; \
	rm -rf $(CVM_BUILD_SUBDIR_NAME)/cvm.*; \
	rm -rf $(BLD_INF); \
	rm -rf $(ABLD_BAT)
