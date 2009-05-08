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
# @(#)defs_jvmti_crw.mk	1.23 06/10/24
#

#
#  Makefile for building the Class Read/Write library
#

###############################################################################
# Make definitions:

CVM_CRW_LIBDIR        ?= $(CVM_LIBDIR)
CVM_CRW_LIB           = \
	$(CVM_CRW_LIBDIR)/$(LIB_PREFIX)java_crw_demo$(LIB_POSTFIX)

CVM_CRW_JAR           = java_crw_demo.jar

CVM_CRW_BUILD_TOP     = $(CVM_BUILD_TOP)/jvmti/crw
CVM_CRW_OBJDIR        := $(call abs2rel,$(CVM_CRW_BUILD_TOP)/obj)
CVM_CRW_FLAGSDIR      = $(CVM_CRW_BUILD_TOP)/flags
CVM_CRW_CLASSES	      = $(CVM_CRW_BUILD_TOP)/classes

CVM_CRW_JARDIR        = $(CVM_LIBDIR)

CVM_CRW_SHAREROOT     = $(CVM_SHAREROOT)/tools/jvmti/crw
CVM_CRW_SHARECLASSESROOT = $(CVM_SHAREDCLASSES_SRCDIR)/com/sun/demo/jvmti/hprof
CVM_CRW_TARGETROOT    = $(CVM_TARGETROOT)/tools/jvmti/crw

CVM_CRW_BUILDDIRS += \
        $(CVM_CRW_OBJDIR) \
        $(CVM_CRW_FLAGSDIR) \
	$(CVM_CRW_CLASSES)

#
# Search path for include files:
#
CVM_CRW_INCLUDES  += \
	$(CVM_BUILD_TOP)/generated/javavm/include \
        $(CVM_CRW_SHAREROOT) \
        $(CVM_CRW_TARGETROOT)

ifeq ($(CVM_STATICLINK_TOOLS), true)
CVM_STATIC_TOOL_LIBS += $(CVM_CRW_LIB)
endif


#
# List of object files to build:
#
CVM_CRW_SHAREOBJS += \
	java_crw_demo.o \

CVM_CRW_OBJECTS0 = $(CVM_CRW_SHAREOBJS) $(CVM_CRW_TARGETOBJS)
CVM_CRW_OBJECTS  = $(patsubst %.o,$(CVM_CRW_OBJDIR)/%.o,$(CVM_CRW_OBJECTS0))

CVM_CRW_TRACKER0 = Tracker.class
CVM_CRW_TRACKER  = $(patsubst %.class,$(CVM_CRW_CLASSES)/%.class,$(CVM_CRW_TRACKER0))

CVM_CRW_SRCDIRS  = \
	$(CVM_CRW_SHAREROOT) \
	$(CVM_CRW_TARGETROOT)

CVM_CRW_FLAGS += \
        CVM_SYMBOLS \
        CVM_OPTIMIZED \
        CVM_DEBUG \
        CVM_DEBUG_CLASSINFO \
        CVM_JVMTI \
        CVM_DYNAMIC_LINKING \

CVM_CRW_FLAGS0 = $(foreach flag, $(CVM_CRW_FLAGS), $(flag).$($(flag)))


CVM_CRW_CLEANUP_ACTION = \
        rm -rf $(CVM_CRW_BUILD_TOP)

CVM_CRW_CLEANUP_OBJ_ACTION = \
        rm -rf $(CVM_CRW_OBJDIR)

