#
# @(#)top_op.mk	1.3 06/10/10
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

# Include JSR 177 - SASTA
ifeq ($(USE_JSR_177), true)
JSR_177_DIR=/ws/jsr177
JPP_DEFS += -DENABLE_JSR_177
include $(JSR_177_DIR)/src/config/$(SUBSYSTEM_MAKE_FILE)
JSR_CLASSES += $(SUBSYSTEM_SATSA_JAVA_FILES)
CVM_SHAREOBJS_SPEED += $(patsubst %.c,%.o,$(SUBSYSTEM_SATSA_NATIVE_FILES))
#CVM_SRCDIRS += $(SUBSYSTEM_SATSA_NATIVE_SRCDIRS)
CVM_INCLUDES += $(SUBSYSTEM_SATSA_EXTRA_INCLUDES)
JSR177_JAR_FILENAME = javame_cdc_satsa.jar
JSR_JNI_CLASSPATH+=$(PS)$(CVM_JSR_CLASSESDIR)/jsr177
CVM_BUILDDIRS += $(CVM_JSR_CLASSESDIR)/jsr177
endif

# Include JSR 172 - Web services
ifeq ($(USE_JSR_172), true)
JSR_172_DIR=/ws/jsr172
JPP_DEFS += -DENABLE_JSR_172
include $(JSR_172_DIR)/src/config/$(SUBSYSTEM_MAKE_FILE)
JSR_CLASSES += $(SUBSYSTEM_WEBSERVICES_JAVA_FILES)
JSR172_JAR_FILENAME = javame_cdc_webservices.jar
OP_JAR_FILENAME += $(empty) $(JSR172_JAR_FILENAME)
CVM_BUILDDIRS += $(CVM_JSR_CLASSESDIR)/jsr172
endif


