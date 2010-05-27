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
PCSL_DIR			= $(CURDIR)/../..
PRINT_MODULE  = javacall

# include top.gmk for various directory and module definitions
ifdef PCSL_PLATFORM
include $(PCSL_DIR)/makefiles/top.gmk
endif

# define 'all' target and all dependencies
# 'all' is the default target

all: $(OUTPUT_LIB_DIR)/libpcsl_print$(LIB_EXT)

$(OUTPUT_INC_DIR)/pcsl_print.h: ../pcsl_print.h
	$(AT)cp -f $< $@

$(OUTPUT_LIB_DIR)/libpcsl_print$(LIB_EXT): $(OUTPUT_OBJ_DIR)/pcsl_print.o
	$(AT)$(AR) $(AR_OUTPUT)`$(call fixcygpath,$@)` `$(call fixcygpath, $^)`

$(OUTPUT_OBJ_DIR)/pcsl_print.o: pcsl_print.c $(OUTPUT_INC_DIR)/pcsl_print.h
	$(AT)$(CC) $(CFLAGS) $(CC_OUTPUT)`$(call fixcygpath, $@)` $<

# doc stuff. The 'doc' target is defined in Docs.gmk.
# Just have to define DOXYGEN_INPUT_LIST and force
# rebuild

DOXYGEN_INPUT_LIST += $(PRINT_DIR)
FRC_DOC_REBUILD = force_doc_rebuild
include $(PCSL_DIR)/makefiles/share/Docs.gmk

# define 'donuts' and all dependencies
#

DONUTS_FILES += $(PRINT_DIR)/testPrint.c
DONUTS_OBJS += $(OUTPUT_OBJ_DIR)/testPrint.o
DONUTS_LIBS += $(OUTPUT_LIB_DIR)/libpcsl_print$(LIB_EXT)

donuts: verify $(OUTPUT_OBJ_DIR) $(OUTPUT_LIB_DIR) $(OUTPUT_OBJ_DIR)/testPrint.o \
        $(OUTPUT_LIB_DIR)/libpcsl_print$(LIB_EXT)
	$(AT)cd $(DONUTS_DIR) ; $(MAKE) DONUTS_FILES="$(DONUTS_FILES)" DONUTS_OBJS="$(DONUTS_OBJS)" \
                             DONUTS_LIBS="$(DONUTS_LIBS)" all

$(OUTPUT_OBJ_DIR)/testPrint.o: $(PRINT_DIR)/testPrint.c
	$(AT)$(CC) -I$(DONUTS_DIR) -I$(PRINT_DIR) $(CFLAGS) $(CC_OUTPUT)$@ `$(call fixcygpath, $<)`

# define ''clean' target

clean: verify_for_clean
	$(AT)rm -rf $(OUTPUT_OBJ_DIR)/pcsl_print.o
	$(AT)rm -rf $(OUTPUT_OBJ_DIR)/testPrint.o
	$(AT)rm -rf $(OUTPUT_BIN_DIR)/donuts$(EXE)
	$(AT)rm -rf $(OUTPUT_GEN_DIR)/donuts_generated.c
	$(AT)rm -rf $(OUTPUT_INC_DIR)/porting_logging.h
	$(AT)rm -rf $(OUTPUT_LIB_DIR)/libpcsl_print$(LIB_EXT)
	$(AT)rm -rf $(DOC_DIR)

.PHONY: all clean donuts doc verify
