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
# @(#)defs.mk	1.72 06/10/24
#
# defs for VxWorks target
#

CVM_TARGETROOT	= $(CVM_TOP)/src/$(TARGET_OS)

#
# All the platform specific build flags we need to keep track of in
# case they are toggled. OK to leave the CVM_ prefix off.
#
CVM_FLAGS += \
	CPU \

CPU_CLEANUP_ACTION = $(CVM_DEFAULT_CLEANUP_ACTION)

#
# Platform specific source directories
#
CVM_SRCDIRS   += $(CVM_TOP)/src/portlibs/ansi_c
CVM_SRCDIRS   += $(CVM_TOP)/src/portlibs/posix
CVM_SRCDIRS   += \
	$(CVM_TARGETROOT)/javavm/runtime \
	$(CVM_TARGETROOT)/bin \
	$(CVM_TARGETROOT)/native/java/lang \
	$(CVM_TARGETROOT)/native/java/io \
	$(CVM_TARGETROOT)/native/java/net \

CVM_INCLUDE_DIRS  += \
	$(CVM_TOP)/src \
	$(CVM_TARGETROOT) \
	$(CVM_TARGETROOT)/native/java/net \
	$(CVM_TARGETROOT)/native/common \

#
# Platform specific objects
#

CVM_TARGETOBJS_SPACE += \
	java_md.o \
	ansi_java_md.o \
	ansi_clib_md.o \
	io_md.o \
	posix_io_md.o \
	posix_net_md.o \
	posix_time_md.o \
	io_util.o \
	sync_md.o \
	system_md.o \
	threads_md.o \
	globals_md.o \
	java_props_md.o \

ifeq ($(CVM_DYNAMIC_LINKING), true)
	CVM_TARGETOBJS_SPACE += linker_md.o
endif

###########################################
# Overrides of stuff setup in share/defs.mk
###########################################

#
# TOOL, CPU, and TGT_DIR are all required by the tornado makefiles.
# CPU normally comes from TARGET_DEVICE (which is derived from the
# build directory name), but can be overrided by setting VXWORKS_CPU
# in the GNUmakefile.
#
TOOL	= gnu
VXWORKS_CPU ?= $(TARGET_DEVICE)
CPU	= $(VXWORKS_CPU)
TGT_DIR = $(WIND_BASE)/target

#
# Include the tornado makefiles. We need to setup all our TARGET tools
# after doing this, since these vxworks makefiles are resposible for
# setting things like $(CC) and $(AS).
#
include ../vxworks/config.mk
include $(TGT_DIR)/h/make/defs.bsp
include $(TGT_DIR)/h/make/make.$(CPU)$(TOOL)
include $(TGT_DIR)/h/make/defs.$(WIND_HOST_TYPE)
TARGET_CC	= $(CC)
TARGET_CCC	= $(CC)
TARGET_AS	= $(AS)
TARGET_LD	= $(LD)
TARGET_AR	= $(AR)
TARGET_RANLIB	= $(RANLIB)

# Override the VxWorks optimized build if debugging is enabled.
# See $(TGT_DIR)/h/make/make.SIMSPARCSOLARISgnu for the definition of
# this variable. It appears that CC_OPTIM is usually set in defs.bsp.
# %comment f001
ifeq ($(CVM_DEBUG), true)
	CC_OPTIM = $(CC_OPTIM_DRIVER)
endif

# overide default name of zip tool
ZIP	= ZIP

# override the default name of the cvm executable
CVM	= cvm.o

LIB_POSTFIX = $(DEBUG_POSTFIX).o

LINKFLAGS    	:= $(subst -Wl$(comma)-export-dynamic,,$(LINKFLAGS))
LINKFLAGS	+= -nostdlib -r
SO_LINKFLAGS	:= $(subst -shared,,$(SO_LINKFLAGS)) -r
LINKLIBS	= $(LINK_ARCH_LIBS)
ASM_FLAGS	+= $(CC_INCLUDE) $(CC_DEFINES)

# override CVM_TZDATAFILE. vxworks doesn't have a tzmappings file
CVM_TZDATAFILE =

# LINK_CMD(objFiles, extraLibs)
# Link cvm as a .o library, not as an executable
LINK_CMD  = $(AT)$(TARGET_CC) -g -nostdlib -Wl,-r -o $@ $(1) $(LINKLIBS)
