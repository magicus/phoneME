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
# @(#)rules.mk	1.18 06/10/10
#

# If the GNUmakefile added /Od, then we will get compiler warnings for
# overriding the other optimization options, so remove them here.
ifeq ($(findstring /Od,$(CCFLAGS_FDLIB)),/Od)
CCFLAGS_FDLIB	:= $(patsubst /O%,,$(CCFLAGS_FDLIB))	
CCFLAGS_FDLIB	+= /Od
endif

# Windows specific make rules
printconfig::
	@echo "SDK_DIR             = $(call CHKWINPATH,$(SDK_DIR))"
	@echo "VC_PATH	           = `ls -d \"$(VC_PATH)\" 2>&1`"
	@echo "PLATFORM_SDK_DIR    = $(call CHKWINPATH,$(PLATFORM_SDK_DIR))"
	@echo "PLATFORM_TOOLS_PATH = `ls -d \"$(PLATFORM_TOOLS_PATH)\" 2>&1`"
	@echo "COMMON_TOOLS_PATH   = `ls -d \"$(COMMON_TOOLS_PATH)\" 2>&1`"
	@echo "INCLUDE             = $$INCLUDE"
	@echo "LIB                 = $$LIB"

#
# Check for compiler compatiblity
#

# Get the supported CPU from the compiler
COMPILER_CPU0 := $(shell  \
	PATH="$(PATH)"; $(TARGET_CC) 2>&1 | \
	grep " for " | \
	sed -e 's/.* for \(.*\)/\1/')

# Translate to match the CPU that we would expect
COMPILER_CPU := $(subst 80x86,x86,$(COMPILER_CPU0))
COMPILER_CPU := $(subst ARM,arm,$(COMPILER_CPU))
COMPILER_CPU := $(subst PowerPC,powerpc,$(COMPILER_CPU))
COMPILER_CPU := $(patsubst MIPS%,mips,$(COMPILER_CPU))

# Make sure the compiler supports the TARGET_CPU_FAMILY
ifneq ($(findstring $(TARGET_CPU_FAMILY),$(COMPILER_CPU)),$(TARGET_CPU_FAMILY))
CVM_COMPILER_INCOMPATIBLE = true
endif
checkconfig::
ifeq ($(CVM_COMPILER_INCOMPATIBLE),true)
	@echo "Target tools not properly specified. The target compiler"
	@echo "specified by TARGET_CC does not agree with the target cpu. You"
	@echo "probably need to fix TARGET_CC, SDK_DIR, VS_DIR, VC_PATH,"
	@echo "PLATFORM, or PLATFORM_OS. Fix these in the GNUmakefile or on "
	@echo "the make command line. If you want to turn off this check, set"
	@echo "CVM_COMPILER_INCOMPATIBLE=false on the make command line"
	@echo "or in the GNUmakefile"
	@echo "   TARGET_CPU_FAMILY: $(TARGET_CPU_FAMILY)"
	@echo "   TARGET_CC CPU:     $(COMPILER_CPU0)"
	exit 2
endif

#
# clean
#

clean:: win32-clean

win32-clean:
	$(CVM_WIN32_CLEANUP_ACTION)

CVM_WIN32_CLEANUP_ACTION = \
	rm -rf *.lst \
	rm -rf *.pdb \
	rm -rf *.ipch

#
# We can't support CVM_DLL=false CVM_PRELOAD_LIB=false on wince platforms
# because symbolic lookup of JNI methods in cvm.exe won't work. It would
# be nice to put this check in  build/win32/defs.mk, but WIN32_PLATFORM
# is not always set in time.
#
ifneq ($(CVM_DLL),true)
ifneq ($(CVM_PRELOAD_LIB),true)
ifeq ($(WIN32_PLATFORM),wince)
$(error Cannot set CVM_DLL=false CVM_PRELOAD_LIB=false for wince platforms)
endif
endif
endif



#
# cvm.exe - a little program to launch cvmi.dll.
#
ifeq ($(CVM_DLL),true)
CVM_EXE = $(CVM_BUILD_SUBDIR_NAME)/bin/cvm.exe
$(J2ME_CLASSLIB) :: $(CVM_EXE)
 
# Override MT_FLAGS for object file dependencies of cvm.exe
$(CVM_EXE) : MT_FLAGS = $(MT_EXE_FLAGS)

$(CVM_EXE) : $(patsubst %,$(CVM_OBJDIR)/%,$(CVMEXE_OBJS))
	@echo "Linking $@"
	$(AT)$(TARGET_LINK) $(LINKFLAGS) $(LINKEXE_FLAGS) /out:$@ \
		$(call POSIX2HOST,$^) $(LINKEXE_LIBS) $(LINKCVMEXE_LIBS)
	$(AT)$(LINK_MANIFEST)
endif

ifeq ($(USE_SPLASH_SCREEN),true)
ifeq ($(WIN32_PLATFORM),wince)

ifeq ($(CVM_DLL),true)
$(CVM_EXE): $(SPLASH_RES)
endif

$(SPLASH_RES): $(SPLASH_RC)
	@echo "rc $@"
	$(AT)$(TARGET_RC) /fo $@ $(call POSIX2HOST,$<)
endif
endif
