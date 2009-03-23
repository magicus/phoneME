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
# @(#)defs.mk	1.79 06/10/13
#

#
# defs for Linux target
#

# enable/disable all IAI optimizations
LINUX_ENABLE_SET_AFFINITY ?= false
CVM_FLAGS	+= LINUX_ENABLE_SET_AFFINITY
LINUX_ENABLE_SET_AFFINITY_CLEANUP_ACTION = \
	rm -rf $(CVM_OBJDIR)/globals_md.o
ifeq ($(LINUX_ENABLE_SET_AFFINITY), true)
CVM_DEFINES += -DLINUX_ENABLE_SET_AFFINITY
endif

CVM_TARGETROOT	= $(CVM_TOP)/src/$(TARGET_OS)

#
# Platform specific defines
#
CVM_DEFINES	+= -D_GNU_SOURCE

#
# Platform specific source directories
#
CVM_SRCDIRS   += $(CVM_TOP)/src/portlibs/ansi_c
CVM_SRCDIRS   += $(CVM_TOP)/src/portlibs/posix
CVM_SRCDIRS   += $(CVM_TOP)/src/portlibs/dlfcn
CVM_SRCDIRS   += $(CVM_TOP)/src/portlibs/realpath
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
	canonicalize_md.o \
	posix_sync_md.o \
	posix_threads_md.o \
	io_md.o \
	posix_io_md.o \
	posix_net_md.o \
	net_md.o \
	time_md.o \
	io_util.o \
	sync_md.o \
	system_md.o \
	threads_md.o \
	globals_md.o \
	java_props_md.o \
	memory_md.o \

#
# On linux, USE_JUMP=true if and only if CVM_MTASK=true
#
ifeq ($(USE_JUMP), true)
override CVM_MTASK	= true
endif
ifeq ($(CVM_MTASK), true)
ifneq ($(USE_JUMP), true)
# It is too late to force USE_JUMP=true at this point, so produce an error.
$(error CVM_MTASK=true requires USE_JUMP=true)
endif
endif

ifeq ($(CVM_MTASK), true)
CLASSLIB_CLASSES += \
       sun.misc.Warmup 
CVM_DEFINES   += -DCVM_MTASK
CVM_SHAREOBJS_SPACE += \
	mtask.o 
endif

ifeq ($(CVM_USE_MEM_MGR), true)
CVM_TARGETOBJS_SPACE += \
	mem_mgr_md.o
endif


ifeq ($(CVM_JIT), true)
CVM_SRCDIRS   += \
	$(CVM_TOP)/src/portlibs/unix/javavm/runtime/jit

CVM_TARGETOBJS_SPACE += \
	jit_md.o
endif

ifeq ($(CVM_DYNAMIC_LINKING), true)
	CVM_TARGETOBJS_SPACE += linker_md.o
endif

# Only add GCF CommProtocol if requested
ifeq ($(CVM_INCLUDE_COMMCONNECTION),true)
ifneq ($(CDC_10),true)
CLASSLIB_CLASSES	+= com.sun.cdc.io.j2me.comm.Protocol
CVM_TARGETOBJS_SPACE	+= commProtocol_md.o
CVM_SRCDIRS		+= $(CVM_TARGETROOT)/native/com/sun/cdc/io/j2me/comm
endif
endif
