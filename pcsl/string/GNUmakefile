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

STRING_UTIL_DIR         = $(CURDIR)/util

ifdef PCSL_PLATFORM
include $(PCSL_DIR)/makefiles/top.gmk
endif

# What to build
#

all : $(STRING_UTIL_DIR) $(STRING_SELECT_DIR) \
     $(OUTPUT_INC_DIR)/pcsl_string_status.h $(OUTPUT_INC_DIR)/pcsl_string.h
	@echo "building string util..."
	$(AT)cd $(STRING_UTIL_DIR); $(MAKE) $@
	@echo "building string module..."
	$(AT)cd $(STRING_SELECT_DIR); $(MAKE) $@

$(OUTPUT_INC_DIR)/pcsl_string_status.h: pcsl_string_status.h
	$(AT)cp -f $< $@

$(OUTPUT_INC_DIR)/pcsl_string.h: pcsl_string.h
	$(AT)cp -f $< $@

clean : verify_for_clean types $(STRING_UTIL_DIR) $(STRING_SELECT_DIR)
	$(AT)cd $(STRING_UTIL_DIR); $(MAKE) $@
	$(AT)cd $(STRING_SELECT_DIR); $(MAKE) $@
	$(AT)rm -rf $(OUTPUT_INC_DIR)/pcsl_string_status.h
	$(AT)rm -rf $(OUTPUT_INC_DIR)/pcsl_string.h
	$(AT)rm -rf $(OUTPUT_OBJ_DIR)/testString.o
	$(AT)rm -rf $(OUTPUT_BIN_DIR)/donuts$(EXE)
	$(AT)rm -rf $(OUTPUT_GEN_DIR)/donuts_generated.c
	$(AT)rm -rf $(DOC_DIR)

doc : verify types $(STRING_UTIL_DIR) $(STRING_SELECT_DIR)
	@echo "building string util..."
	$(AT)cd $(STRING_UTIL_DIR); $(MAKE) $@
	@echo "building string module..."
	$(AT)cd $(STRING_SELECT_DIR); $(MAKE) $@

# define 'donuts' and all dependencies
#

DONUTS_FILES += $(STRING_DIR)/testString.c
DONUTS_OBJS += $(OUTPUT_OBJ_DIR)/testString.o
DONUTS_LIBS += $(OUTPUT_LIB_DIR)/libpcsl_string$(LIB_EXT)

donuts: all $(OUTPUT_OBJ_DIR) $(OUTPUT_LIB_DIR) $(DONUTS_LIBS) $(DONUTS_OBJS)
	$(AT)cd $(DONUTS_DIR);$(MAKE) DONUTS_FILES="$(DONUTS_FILES)" DONUTS_OBJS="$(DONUTS_OBJS)" \
                            DONUTS_LIBS="$(DONUTS_LIBS)" all

$(OUTPUT_OBJ_DIR)/testString.o: $(STRING_DIR)/testString.c
	$(AT)$(CC) -I$(DONUTS_DIR) $(OUTPUT_INC_DIR) \
	$(CFLAGS) $(CC_OUTPUT)$@ `$(call fixcygpath, $<)`

include $(PCSL_DIR)/makefiles/verify.gmk

.PHONY : all doc clean donuts verify
