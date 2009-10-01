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

ifeq ($(HOST_DEVICE), Interix)
CVM_JAVAC       = javac.exe
CVM_JAVAH       = javah.exe
CVM_JAR         = jar.exe
endif

#
# We rely on many things setup in build/share/defs.mk. However, since
# it is meant primarily unix build environments and GNU tools, we have
# to override a lot of values.
#

# prefix and postfix for shared libraries
LIB_PREFIX		=
LIB_POSTFIX		= $(DEBUG_POSTFIX).dll
LIB_LINK_POSTFIX	= $(DEBUG_POSTFIX).lib

# Miscellaneous options we override
override GENERATEMAKEFILES = false  

#
# Specify all the host and target tools. 
# CC and AS are specific in the win32-<cpu>/defs.mk file.
#
TARGET_LD		= $(TARGET_LINK)
TARGET_LINK		= LINK.EXE
TARGET_AR		= $(TARGET_LINK) -lib /nologo
TARGET_AR_CREATE	= $(TARGET_AR) /out:$(call POSIX2HOST, $(1))
TARGET_AR_UPDATE	= true $(TARGET_AR_CREATE)
TARGET_RC		= RC.EXE

# Override the default TARGET_CC_VERSION, since it relies on the gcc
# -dumpversion and -dumpmachine options.
TARGET_CC_VERSION ?= $(shell PATH="$(PATH)"; $(TARGET_CC) 2>&1 | grep -i version)

#
# Compiler and linker flags
#
CCDEPEND	= /FD

ASM_FLAGS	= $(ASM_ARCH_FLAGS)
CCFLAGS     	= /nologo /c /W2 $(CC_ARCH_FLAGS)
ifeq ($(CVM_BUILD_SUBDIR),true)
CCFLAGS		+= /Fd$(call POSIX2HOST,$(CVM_BUILD_TOP_ABS))/cvm.pdb
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
CCFLAGS_SPEED     = $(CCFLAGS) /O2 /Ob2 /Ot -DNDEBUG
CCFLAGS_SPACE     = $(CCFLAGS) /O1 /Ob1 -DNDEBUG
endif
# vc debug
else
CCFLAGS_SPEED     = $(CCFLAGS) /Od -D_DEBUG -DDEBUG
CCFLAGS_SPACE     = $(CCFLAGS) /Od -D_DEBUG -DDEBUG
endif

ifeq ($(CVM_SYMBOLS), true)
CCFLAGS += /Zi
endif

ifeq ($(CVM_DEBUG), true)
VC_DEBUG_POSTFIX = d
endif
M_DLL_FLAGS = /MD$(VC_DEBUG_POSTFIX)
M_EXE_FLAGS = /MT$(VC_DEBUG_POSTFIX)

ifeq ($(CVM_DLL),true)
M_FLAGS = $(M_DLL_FLAGS)
else
M_FLAGS = $(M_EXE_FLAGS)
endif
CCFLAGS += $(M_FLAGS)

# Setup links flags used for everything we link
LINKFLAGS = /incremental:no /nologo /map \
	    $(LINK_ARCH_LIBS) $(LINK_ARCH_FLAGS) $(EXTRA_PROFILING_FLAGS)
ifeq ($(CVM_SYMBOLS), true)
LINKFLAGS += /DEBUG
endif

# Setup exports that other dlls may need.
LINKCVM_EXPORTS	+= \
	/export:jio_snprintf  /export:CVMexpandStack /export:CVMtimeMillis \
	/export:CVMIDprivate_allocateLocalRootUnsafe /export:CVMglobals,DATA \
	/export:CVMsystemPanic /export:CVMcsRendezvous \
	/export:CVMconsolePrintf /export:CVMthrowOutOfMemoryError \
	/export:CVMthrowNoSuchMethodError \
	/export:CVMthrowIllegalArgumentException
ifeq ($(CVM_DEBUG), true)
LINKCVM_EXPORTS	+= /export:CVMassertHook /export:CVMdumpStack
endif

LINKLIBS_JCS    =

LINKALL_LIBS 	+= $(LINK_ARCH_LIBS) $(LIBPATH)

# setup flags and libs used to link every exe.
LINKEXE_LIBS	+= $(LINKALL_LIBS)
LINKEXE_FLAGS	+= $(LINKFLAGS) /fixed:no $(LINKEXE_ENTRY) $(LINKEXE_STACK)

# setup flags used to link every dll
LINKDLL_LIBS	+= $(LINKALL_LIBS)
LINKDLL_FLAGS	+= $(LINKFLAGS) /dll 

# setup libs flags and libs for linking cvm, whether it is cvm.exe or cvmi.dll
LINKCVM_LIBS	+= $(sort $(WIN_LINKLIBS))
CVM_IMPL_LIB	= $(CVM_BUILD_SUBDIR_NAME)/bin/cvmi.lib
LINKCVM_FLAGS	= /implib:$(CVM_IMPL_LIB) $(LINKCVM_EXPORTS)
ifeq ($(CVM_DLL),true)
LINKCVM_FLAGS	+= $(LINKDLL_FLAGS) $(LINKDLL_BASE)
LINKCVM_LIBS	+= $(LINKDLL_LIBS)
else
LINKCVM_FLAGS	+= $(LINKEXE_FLAGS)
LINKCVM_LIBS	+= $(LINKEXE_LIBS) $(LINKCVMEXE_LIBS)
endif

# libs and flags that all shared libraries will want
SO_LINKLIBS	= $(LINKALL_LIBS) $(CVM_IMPL_LIB)
SO_LINKFLAGS	= $(LINKDLL_FLAGS)

#
# commands for running the tools
#

# compileCCC(flags, objfile, srcfiles)
compileCCC = $(AT)$(TARGET_CCC) $(1) /Fo$(2) $(call abs2rel,$(3))

# compileCC(flags, objfile, srcfiles)
compileCC  = $(AT)$(TARGET_CC) $(1) /Fo$(2) $(call abs2rel,$(3))

LINK_MANIFEST = \
	if [ -f $@.manifest ] ; then \
	    echo "   Linking in manifest file $(notdir $@.manifest)"; \
	    mt.exe -nologo -manifest $(call POSIX2HOST,$@).manifest \
		"-outputresource:$(call POSIX2HOST,$@);\#2" ;\
	fi;

# LINK_CMD(objFiles, extraLibs)
LINK_CMD	= $(AT)\
	$(eval OUT := $(call POSIX2HOST,$@)) \
	$(call POSIX2HOST_CMD,$(1)) > $(OUT).lst; \
	$(TARGET_LINK) $(LINKCVM_FLAGS) /out:$(OUT) @$(OUT).lst $(2); \
	$(LINK_MANIFEST)

# SO_LINK_CMD(objFiles, extraLibs)
SO_LINK_CMD	= $(AT)\
	$(eval OUT := $(call POSIX2HOST,$@)) \
	$(call POSIX2HOST_CMD,$(1)) > $(OUT).lst; \
	$(TARGET_LD) $(SO_LINKFLAGS) /out:$(OUT) @$(OUT).lst \
		$(SO_LINKLIBS) $(2); \
	$(LINK_MANIFEST)

# Don't let the default compiler compatibility check be done
# since we are not using gcc
CVM_DISABLE_COMPILER_CHECK = true

WIN32_QUERY_REG = $(shell REG.EXE QUERY $(1) /v $(2) 2> /dev/null | \
	awk -F'\t' '/$(2)/ { print $$3 }')
