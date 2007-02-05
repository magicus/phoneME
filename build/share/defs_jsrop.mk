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

JSROP_NUMBERS = 75 82 120 135 172 177 179 180 184 205 211 229 234 238 239

JSROP_LIB_DIR   = $(CVM_LIBDIR)
JSROP_BUILD_DIR = $(CDC_DIST_DIR)
JSROP_OBJ_DIR   = $(CVM_OBJDIR)

JSROP_JARS_FLAGS = $(foreach jsr_number,$(JSROP_NUMBERS),$(JSROP_LIB_DIR)/jsr$(jsr_number).jar=$(USE_JSR_$(jsr_number)))
JSROP_BUILD_JARS = $(patsubst %=true,%,$(filter %true, $(JSROP_JARS_FLAGS)))
MIDP_JSROP_USE_FLAGS = $(foreach jsr_number,$(JSROP_NUMBERS),USE_JSR_$(jsr_number)=false)

ifneq ($(JSROP_BUILD_JARS),)
JAVACALL_TARGET=$(TARGET_OS)-$(TARGET_CPU_FAMILY)
include $(JAVACALL_DIR)/module.gmk
include $(JSROP_ABSTRACTS_DIR)/build/$(SUBSYSTEM_MAKE_FILE)
JSROP_JARS=$(JSROP_ABSTRACTS_JAR) $(JSROP_BUILD_JARS)
endif

# Include JSR 75
ifeq ($(USE_JSR_75), true)
include $(JSR_75_DIR)/build/$(SUBSYSTEM_MAKE_FILE)
endif

CVM_INCLUDES    += $(JSROP_EXTRA_INCLUDES)

ifeq ($(CVM_PRELOAD_LIB), true)
CVM_JCC_INPUT   += $(JSROP_JARS)
CVM_CNI_CLASSES += $(JSROP_CNI_CLASSES)
CVM_OBJECTS     += $(JSROP_NATIVE_OBJS)
else
CLASSLIB_DEPS   += $(JSROP_NATIVE_LIBS)
endif

