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
# @(#)host_defs.mk	1.10 06/10/24
#

ifeq ($(HOST_OS), Interix)
DOS2UNIX	= dos2unix.exe
WIN2POSIX_CMD	= chgpath
WIN2POSIX	= $(shell chgpath "$(1)")
CHKWINPATH	= $(shell test -d "`chgpath \"$(1)\"`" 2>&1 && echo $(1))
CVM_JAVAC	= javac.exe
CVM_JAVAH	= javah.exe
CVM_JAR		= jar.exe
endif
ifeq ($(HOST_OS), cygwin)
DOS2UNIX	= dos2unix
WIN2POSIX_CMD	= cygpath
WIN2POSIX	= $(shell cygpath "$(1)")
POSIX2WIN	= $(shell cygpath -m "$(1)")
CHKWINPATH	= $(shell ls -d "$(1)" 2>&1)
endif

#
# We rely on many things setup in build/share/defs.mk. However, since
# it is meant primarily unix build environments and GNU tools, we have
# to override a lot of values.
#

# prefix and postfix for shared libraries
LIB_PREFIX		= lib
LIB_POSTFIX		= $(DEBUG_POSTFIX).dll
LIB_LINK_POSTFIX	= $(DEBUG_POSTFIX).lib

# Miscellaneous options we override
override GENERATEMAKEFILES = false  

#
# Location of JDK
#
JDK_HOME	?= h:/jdk1.3.1_01

#
# Specify all the host and target tools. 
# CC and AS are specific in the win32-<cpu>/defs.mk file.
#
TARGET_LD		= $(TARGET_LINK)
TARGET_LINK		= LINK.EXE
TARGET_AR		= LINK.EXE -lib /nologo
TARGET_AR_CREATE	= $(TARGET_AR) /out:$(1)
TARGET_AR_UPDATE	= true $(TARGET_AR_CREATE)

#
# Compiler and linker flags
#
CCDEPEND	= /FD

ASM_FLAGS	= $(ASM_ARCH_FLAGS)
CCFLAGS     	= /nologo /c /W2 $(CC_ARCH_FLAGS)
ifeq ($(CVM_BUILD_SUBDIR),true)
CCFLAGS		+= /Fd$(CVM_BUILD_SUBDIR_NAME)/cvm.pdb
endif
CCCFLAGS	=
ifeq ($(CVM_OPTIMIZED), true)
ifeq ($(TARGET_DEVICE), NET)
# Compiler optimization flags for Target MS Visual Studio .NET 2003 Standard
# package
CCFLAGS_SPEED     = $(CCFLAGS) /GB -DNDEBUG
CCFLAGS_SPACE     = $(CCFLAGS) /GB -DNDEBUG
else
#  optimized
CCFLAGS_SPEED     = $(CCFLAGS) /O2 /Ob2 /Og /Ot -DNDEBUG
CCFLAGS_SPACE     = $(CCFLAGS) /O1 /Ob1 /Og -DNDEBUG
endif
# vc debug
else
CCFLAGS_SPEED     = $(CCFLAGS) /Od -D_DEBUG -DDEBUG
CCFLAGS_SPACE     = $(CCFLAGS) /Od -D_DEBUG -DDEBUG
endif

ifeq ($(CVM_SYMBOLS), true)
CCFLAGS += /Zi /Yd
endif

ifeq ($(CVM_DEBUG), true)
DEBUG_LINKFLAGS = /debug
MT_DLL_FLAGS = /MDd
MT_EXE_FLAGS = /MTd
else
MT_DLL_FLAGS = /MD
MT_EXE_FLAGS = /MT
endif
MT_FLAGS = $(MT_DLL_FLAGS)
CCFLAGS += $(MT_FLAGS)

ifeq ($(CVM_DLL),true)
CVM_IMPL_LIB	= $(CVM_BUILD_SUBDIR_NAME)/bin/cvmi.lib
LINKFLAGS	= /implib:$(CVM_IMPL_LIB) /export:jio_snprintf $(SO_LINKFLAGS)
else
LINKFLAGS	=
endif
LINKLIBS = $(WIN_LINKLIBS) $(LINK_ARCH_LIBS) $(LIBPATH)
LINKLIBS_JCS    =

SO_LINKLIBS	= $(LINKLIBS) $(LIBPATH)
SO_LINKFLAGS	= \
	/nologo /map /dll /incremental:yes \
	$(DEBUG_LINKFLAGS) $(LINK_ARCH_FLAGS) \

LINKEXE_LIBS = $(LINKEXE_ARCH_LIBS) $(LIBPATH)

LINKEXE_FLAGS = /nologo $(DEBUG_LINKFLAGS) \
		/incremental:no $(LINKEXE_ARCH_FLAGS)

LINKEXE_CMD	= $(AT)$(TARGET_LINK) $(LINKEXE_FLAGS) /out:$@ $^ \
			$(LINKEXE_LIBS)

#
# commands for running the tools
#
CCC_CMD		= $(AT)$(TARGET_CCC) $(1) /Fo$(2) $(3)
CCC_CMD_SPEED	= $(call CCC_CMD,$(CFLAGS_SPEED) $(CCCFLAGS),$@,$<)
CCC_CMD_SPACE	= $(call CCC_CMD,$(CFLAGS_SPACE) $(CCCFLAGS),$@,$<)

CC_CMD		= $(AT)$(TARGET_CC) $(1) /Fo$(2) $(3)
CC_CMD_SPEED	= $(call CC_CMD,$(CFLAGS_SPEED),$@,$<)
CC_CMD_SPACE	= $(call CC_CMD,$(CFLAGS_SPACE),$@,$<)
CC_CMD_LOOP	= $(call CC_CMD,$(CFLAGS_LOOP) ,$@,$<)
CC_CMD_FDLIB	= $(call CC_CMD,$(CFLAGS_FDLIB),$@,$<)

LINK_CMD	= $(AT)$(TARGET_LINK) $(LINKFLAGS) /out:$@ $^ $(LINKLIBS)

SO_CC_CMD	= $(AT)$(TARGET_CC) $(SO_CFLAGS) /Fo$@ $<
SO_LINK_CMD	= $(AT)$(TARGET_LD) $(SO_LINKFLAGS) /out:$@ $^ $(SO_LINKLIBS)
