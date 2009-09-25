#
# Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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
######################################################################
#
# Makefile for building and packaging.
#
######################################################################

# Workspace directory
PCSL_DIR		= $(CURDIR)/..

ifdef PCSL_PLATFORM
include $(PCSL_DIR)/makefiles/top.gmk
endif

# What to build
#

$(OUTPUT_INC_DIR)/java_types.h : $(TYPES_DIR)/java_types.h
	$(AT)cp -f $< $@

$(OUTPUT_INC_DIR)/java_types_md.h : $(TYPES_MD_DIR)/java_types_md.h
	$(AT)cp -f $< $@

all : $(OUTPUT_INC_DIR)/java_types.h $(OUTPUT_INC_DIR)/java_types_md.h

# doc stuff. The 'doc' target is defined in Docs.gmk.
# Just have to define DOXYGEN_INPUT_LIST and force
# rebuild

DOXYGEN_INPUT_LIST += $(TYPES_DIR) $(TYPES_MD_DIR)
FRC_DOC_REBUILD = force_doc_rebuild
include $(PCSL_DIR)/makefiles/share/Docs.gmk

# define ''clean' target

clean:
	$(AT)rm -rf $(OUTPUT_INC_DIR)/java_types.h
	$(AT)rm -rf $(OUTPUT_INC_DIR)/java_types_md.h

.PHONY : all clean
