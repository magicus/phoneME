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
PCSL_DIR		= $(CURDIR)/../

ifdef PCSL_PLATFORM
include $(PCSL_DIR)/makefiles/top.gmk
endif

# What to build
#

all doc donuts : $(ESCFILENAMES_SELECT_DIR)
	@echo "building ESCFILENAMES module..."
	$(AT)cd $(ESCFILENAMES_SELECT_DIR); $(MAKE) $@

clean: verify_for_clean $(ESCFILENAMES_SELECT_DIR)
	@echo "cleaning ESCFILENAMES module..."
	$(AT)cd $(ESCFILENAMES_SELECT_DIR); $(MAKE) $@

# define 'donuts' and all dependencies
#

DONUTS_FILES += $(ESCFILENAMES_DIR)/testEsc.c
DONUTS_OBJS += $(OUTPUT_OBJ_DIR)/testEsc.o
DONUTS_LIBS += $(OUTPUT_LIB_DIR)/libpcsl_escfilenames$(LIB_EXT)

donuts: all $(OUTPUT_OBJ_DIR) $(OUTPUT_LIB_DIR) $(DONUTS_LIBS) $(DONUTS_OBJS)
	$(AT)cd $(DONUTS_DIR);$(MAKE) DONUTS_FILES="$(DONUTS_FILES)" DONUTS_OBJS="$(DONUTS_OBJS)" \
                            DONUTS_LIBS="$(DONUTS_LIBS)" all

$(OUTPUT_OBJ_DIR)/testEsc.o: $(ESCFILENAMES_DIR)/testEsc.c
	$(AT)$(CC) -I$(DONUTS_DIR) $(OUTPUT_INC_DIR) \
	$(CFLAGS) $(CC_OUTPUT)$@ `$(call fixcygpath, $<)`

include $(PCSL_DIR)/makefiles/verify.gmk

.PHONY : all doc clean donuts
