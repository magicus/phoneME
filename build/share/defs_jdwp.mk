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
# @(#)defs_jdwp.mk	1.18 06/10/26
#

#
#  Makefile for building the jdwp tool
#

###############################################################################
# Make definitions:

CVM_JDWP_BUILD_TOP     = $(CVM_BUILD_TOP)/jdwp
CVM_JDWP_OBJDIR        := $(call abs2rel,$(CVM_JDWP_BUILD_TOP)/obj)
CVM_JDWP_LIBDIR        ?= $(CVM_LIBDIR)
CVM_JDWP_FLAGSDIR      = $(CVM_JDWP_BUILD_TOP)/flags
CVM_JDWP_CLASSES       = $(CVM_JDWP_BUILD_TOP)/classes

CVM_JDWP_SHAREROOT     = $(CVM_TOP)/src/share/tools/jpda
CVM_JDWP_TARGETROOT    = $(CDC_OS_COMPONENT_DIR)/src/$(TARGET_OS)/tools/jpda
CVM_JDWP_TARGETSHAREROOT = $(CDC_OS_COMPONENT_DIR)/src/$(TARGET_OS)

CVM_JDWP_BUILDDIRS += \
        $(CVM_JDWP_OBJDIR) \
        $(CVM_JDWP_FLAGSDIR) \
        $(CVM_JDWP_CLASSES)

CVM_JDWP_LIB = $(CVM_JDWP_LIBDIR)/$(LIB_PREFIX)jdwp$(LIB_POSTFIX)

CVM_JDWP_TRANSPORT = socket

#
# Search path for include files:
#
CVM_JDWP_INCLUDE_DIRS  += \
        $(CVM_SHAREROOT)/javavm/export \
	$(CVM_JDWP_TARGETROOT)/back \
	$(CVM_JDWP_SHAREROOT)/back \
	$(CVM_JDWP_SHAREROOT)/back/npt \
	$(CVM_JDWP_TARGETROOT)/back/npt \
	$(CVM_JDWP_SHAREROOT)/transport/export \
	$(CVM_JDWP_TARGETROOT)/transport/$(CVM_JDWP_TRANSPORT) \
	$(CVM_JDWP_BUILD_TOP)

ifeq ($(CVM_DEBUG), true)
CVM_JDWP_DEFINES += -DDEBUG
#CVM_JDWP_DEFINES +=  -DJDWP_LOGGING
endif

JPDA_NO_DLALLOC = true

ifeq ($(JPDA_NO_DLALLOC), true)
CVM_JDWP_DEFINES += -DJPDA_NO_DLALLOC
endif

ifeq ($(CVM_STATICLINK_TOOLS), true)
CVM_STATIC_TOOL_LIBS += $(CVM_JDWP_LIB)
endif

#
# List of object files to build:
#
CVM_JDWP_SHAREOBJS += \
	ArrayReferenceImpl.o \
	ArrayTypeImpl.o \
	ClassTypeImpl.o \
	ClassLoaderReferenceImpl.o \
	ClassObjectReferenceImpl.o \
	EventRequestImpl.o \
	FieldImpl.o \
	FrameID.o \
	MethodImpl.o \
	ObjectReferenceImpl.o \
	ReferenceTypeImpl.o \
	SDE.o \
	StackFrameImpl.o \
	StringReferenceImpl.o \
	ThreadGroupReferenceImpl.o \
	ThreadReferenceImpl.o \
	VirtualMachineImpl.o \
	bag.o \
	classTrack.o \
	commonRef.o \
	debugDispatch.o \
	debugInit.o \
	debugLoop.o \
	error_messages.o \
	eventFilter.o \
	eventHandler.o \
	eventHelper.o \
	inStream.o \
	invoker.o \
	log_messages.o \
	npt.o \
	outStream.o \
	standardHandlers.o \
	stepControl.o \
	stream.o \
	threadControl.o \
	transport.o \
	utf.o \
	utf_md.o \
	util.o \
	linker_md.o \
	exec_md.o \
	util_md.o

ifneq ($(JPDA_NO_DLALLOC), true)
CVM_JDWP_SHAREOBJS += dlAlloc.o
endif



CVM_JDWP_OBJECTS0 = $(CVM_JDWP_SHAREOBJS) $(CVM_JDWP_TARGETOBJS)
CVM_JDWP_OBJECTS  = $(patsubst %.o,$(CVM_JDWP_OBJDIR)/%.o,$(CVM_JDWP_OBJECTS0))

CVM_JDWP_SRCDIRS  = \
	$(CVM_JDWP_SHAREROOT)/back \
	$(CVM_JDWP_SHAREROOT)/back/npt \
	$(CVM_JDWP_TARGETROOT)/back/npt \
	$(CVM_JDWP_SHAREROOT)/transport \
	$(CVM_JDWP_SHAREROOT)/transport/$(CVM_JDWP_TRANSPORT) \
	$(CVM_JDWP_TARGETROOT)/back \
	$(CVM_JDWP_TARGETROOT)/transport \
	$(CVM_JDWP_TARGETROOT)/transport/$(CVM_JDWP_TRANSPORT) \
	$(CVM_JDWP_TARGETSHAREROOT)/javavm/runtime \
	$(CVM_TOP)/src/portlibs/dlfcn

CVM_JDWP_FLAGS += \
        CVM_SYMBOLS \
        CVM_OPTIMIZED \
        CVM_DEBUG \
        CVM_JVMTI \
        CVM_DYNAMIC_LINKING \


CVM_JDWP_FLAGS0 = $(foreach flag, $(CVM_JDWP_FLAGS), $(flag).$($(flag)))

CVM_JDWP_CLEANUP_ACTION = \
        rm -rf $(CVM_JDWP_BUILD_TOP)

CVM_JDWP_CLEANUP_OBJ_ACTION = \
        rm -rf $(CVM_JDWP_OBJDIR)

