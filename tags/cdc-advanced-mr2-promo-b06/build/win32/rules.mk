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
	@echo "VC_PATH		   = `ls -d \"$(VC_PATH)\" 2>&1`"
	@echo "PLATFORM_SDK_DIR    = $(call CHKWINPATH,$(PLATFORM_SDK_DIR))"
	@echo "PLATFORM_TOOLS_PATH = `ls -d \"$(PLATFORM_TOOLS_PATH)\" 2>&1`"
	@echo "COMMON_TOOLS_PATH   = `ls -d \"$(COMMON_TOOLS_PATH)\" 2>&1`"

clean:: win32-clean

win32-clean:
	$(CVM_WIN32_CLEANUP_ACTION)

CVM_WIN32_CLEANUP_ACTION = \
	rm -rf *.lst \
	rm -rf *.pdb \
	rm -rf *.ipch

#
# cvm.exe - a little program to launch cvm.dll.
#
CVM_EXE = $(CVM_BUILD_SUBDIR_NAME)/bin/cvm.exe
$(J2ME_CLASSLIB) :: $(CVM_EXE)

# Override MT_FLAGS for object file dependencies of cvm.exe
$(CVM_EXE) : MT_FLAGS = $(MT_EXE_FLAGS)

$(CVM_EXE) : $(CVM_OBJDIR)/ansi_java_md.o $(CVM_OBJDIR)/java_md.o
	@echo "Linking $@"
	$(AT)$(LINKEXE_CMD)
