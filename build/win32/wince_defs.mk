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
# @(#)wince_defs.mk	1.15 06/10/10
#
# defs for WinCE target
#

CVM_DEFINES += -DWINCE -DWIN32_LEAN_AND_MEAN -DWIN32_PLATFORM_PSPC
CVM_DEFINES += -DUNICODE -D_UNICODE
CC_ARCH_FLAGS += -D__STDC__
#M_DLL_FLAGS =
#M_EXE_FLAGS =

# Some WinCE devices don't support the embedding of the code cache
# as a read/write/execute segment in the executable. For these devices
# we need a way to turn off the static code cache. This also implies
# turning off trap-based null checks.
WINCE_DISABLE_STATIC_CODECACHE ?= false
CVM_FLAGS += WINCE_DISABLE_STATIC_CODECACHE
WINCE_DISABLE_STATIC_CODECACHE_CLEANUP_ACTION = $(CVM_DEFAULT_CLEANUP_ACTION)
ifeq ($(WINCE_DISABLE_STATIC_CODECACHE), true)
	CVM_DEFINES += -DWINCE_DISABLE_STATIC_CODECACHE
endif

CVM_INCLUDE_DIRS  += \
        $(CVM_TARGETROOT)/javavm/include/ansi \

#
# Support for sending CVMioWrite to OUT.txt and ERR.txt and reading
# CVMioRead() from IN.txt when there is no console support. Profiles
# can override WCE_IOWRITE if they want different behaviour.
#
WCE_CONSOLE = wceIOWrite.o
CVM_TARGETOBJS_SPACE += $(WCE_CONSOLE)

CVM_TARGETOBJS_SPEED += \
        wceUtil.o

# libs to link just with cvm, whether it is an exe or dll
WIN_LINKLIBS += commctrl.lib winsock.lib

# libs to link with every dll and exe
LINKALL_LIBS += \
	/nodefaultlib:oldnames.lib \
	/nodefaultlib:libcmt.lib /nodefaultlib:libcmtd.lib \
	/nodefaultlib:libc.lib /nodefaultlib:libcd.lib \
	coredll.lib /defaultlib:corelibc.lib

# libs to link with every dll
LINKDLL_LIBS += \
	/nodefaultlib:msvcrt.lib /nodefaultlib:msvcrtd.lib

# libs to link with every exe
LINKEXE_LIBS += \

# entry to every exe
LINKEXE_ENTRY = /entry:mainACRTStartup

################################################
# Setup INCLUDE, LIB, and PATH for the VC tools.
################################################

PLATFORM_TOOLS_PATH ?= $(VC_PATH)/EVC/$(PLATFORM_OS)/bin
COMMON_TOOLS_PATH   ?= $(VC_PATH)/Common/EVC/Bin

PLATFORM_SDK_DIR ?= $(SDK_DIR)/$(PLATFORM_OS)/$(PLATFORM)
INCLUDE	:= $(foreach dir,$(PLATFORM_INCLUDE_DIRS),$(PLATFORM_SDK_DIR)/$(dir);)EOL
INCLUDE	:= $(subst ;EOL,,$(INCLUDE))
INCLUDE	:= $(subst ;$(space),;,$(INCLUDE))
LIB	:= $(foreach dir,$(PLATFORM_LIB_DIRS),$(PLATFORM_SDK_DIR)/$(dir);)EOL
LIB	:= $(subst ;EOL,,$(LIB))
LIB	:= $(subst ;$(space),;,$(LIB))
PATH	:= $(PLATFORM_TOOLS_PATH):$(COMMON_TOOLS_PATH):$(PATH)

# export them all to the shell
export INCLUDE
export LIB
export PATH
