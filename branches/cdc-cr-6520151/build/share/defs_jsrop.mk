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

SUBSYSTEM_MAKE_FILE       = subsystem.gmk

JSROP_NUMBERS = 75 82 120 135 172 177 205 211 234

JSROP_JARS_FLAGS = $(foreach jsr_number,$(JSROP_NUMBERS),$(CVM_LIBDIR)/jsr$(jsr_number).jar=$(USE_JSR_$(jsr_number)))
JSROP_BUILD_JARS = $(patsubst %=true,%,$(filter %true, $(JSROP_JARS_FLAGS)))
MIDP_JSROP_USE_FLAGS = $(foreach jsr_number,$(JSROP_NUMBERS),USE_JSR_$(jsr_number)=false)

ifneq ($(JSROP_BUILD_JARS),)
JAVACALL_TARGET=linux-x86
include $(JAVACALL_DIR)/module.gmk
include $(JSROP_ABSTRACTS_DIR)/build/$(SUBSYSTEM_MAKE_FILE)
JSROP_JARS=$(JSROP_ABSTRACTS_JAR) $(JSROP_BUILD_JARS)
endif

# Include JSR 75
ifeq ($(USE_JSR_75), true)
JSR_75_BUILD_DIR      = $(CDC_DIST_DIR)/jsr75
JSR_75_JAR            = $(CVM_LIBDIR)/jsr75.jar

JPP_DEFS += -DENABLE_JSR_75
include $(JSR_75_DIR)/build/$(SUBSYSTEM_MAKE_FILE)
JSR_75_NATIVE_LIB_OBJS = $(patsubst %.c,$(CVM_OBJDIR)/%.o,$(SUBSYSTEM_JSR_75_NATIVE_FILES))
ifeq ($(CVM_PRELOAD_LIB), true)
JSROP_NATIVE_OBJS += $(JSR_75_NATIVE_LIB_OBJS)
JSROP_CNI_CLASSES += $(SUBSYSTEM_JSR_75_CNI_CLASSES)
else
JSR_75_NATIVE_LIB = $(CVM_LIBDIR)/$(LIB_PREFIX)jsr75$(LIB_POSTFIX)
JSROP_NATIVE_LIBS += $(JSR_75_NATIVE_LIB) 
endif

endif

ifeq ($(CVM_PRELOAD_LIB), true)
CVM_JCC_INPUT   += $(JSROP_JARS)
CVM_CNI_CLASSES += $(JSROP_CNI_CLASSES)
CVM_OBJECTS     += $(JSROP_NATIVE_OBJS)
else
CLASSLIB_DEPS   += $(JSROP_NATIVE_LIBS)
endif
