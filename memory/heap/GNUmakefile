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
PCSL_DIR		= $(CURDIR)/../../
MEMORY_MODULE		= heap

# include top.gmk for various directory and module definitions
ifdef PCSL_PLATFORM
include $(PCSL_DIR)/makefiles/top.gmk
endif


# define 'all' target and all dependencies
# 'all' is the default target

OBJS = $(OUTPUT_OBJ_DIR)/pcsl_memory.o \
       $(OUTPUT_OBJ_DIR)/pcsl_memory_port.o \
       $(OUTPUT_OBJ_DIR)/pcsl_chunkheap.o

INCS = $(OUTPUT_INC_DIR)/pcsl_memory.h $(OUTPUT_INC_DIR)/pcsl_memory_impl.h

all: $(OUTPUT_LIB_DIR)/libpcsl_memory$(LIB_EXT)

$(OUTPUT_INC_DIR)/pcsl_memory.h: ../pcsl_memory.h
	$(AT)cp -f $< $@

$(OUTPUT_INC_DIR)/pcsl_memory_impl.h: pcsl_memory_impl.h
	$(AT)cp -f $< $@

$(OUTPUT_LIB_DIR)/libpcsl_memory$(LIB_EXT): $(OBJS)
	$(AT)$(AR) $(AR_OUTPUT)$@ `$(call fixcygpath, $^)`

$(OUTPUT_OBJ_DIR)/pcsl_memory.o: pcsl_memory.c $(INCS)
	$(AT)$(CC) -I. -I./.. -I./../../print -I$(OUTPUT_INC_DIR) $(CFLAGS) $(CC_OUTPUT)$@ `$(call fixcygpath, $<)`

$(OUTPUT_OBJ_DIR)/pcsl_chunkheap.o: pcsl_chunkheap.c $(INCS)
	$(AT)$(CC) -I. -I./.. -I./../../print -I$(OUTPUT_INC_DIR) $(CFLAGS) $(CC_OUTPUT)$@ `$(call fixcygpath, $<)`

# doc stuff. The 'doc' target is defined in Docs.gmk.
# Just have to define DOXYGEN_INPUT_LIST and force
# rebuild

DOXYGEN_INPUT_LIST += $(MEMORY_DIR)
FRC_DOC_REBUILD = force_doc_rebuild
include $(PCSL_DIR)/makefiles/share/Docs.gmk

# define 'donuts' and all dependencies
#

DONUTS_FILES += $(MEMORY_DIR)/testMem.c
DONUTS_OBJS += $(OUTPUT_OBJ_DIR)/testMem.o
DONUTS_LIBS += $(OUTPUT_LIB_DIR)/libpcsl_memory$(LIB_EXT)

donuts: verify $(OUTPUT_OBJ_DIR) $(OUTPUT_LIB_DIR) $(DONUTS_LIBS) $(DONUTS_OBJS)
	$(AT)cd $(DONUTS_DIR);$(MAKE) DONUTS_FILES="$(DONUTS_FILES)" DONUTS_OBJS="$(DONUTS_OBJS)" \
                            DONUTS_LIBS="$(DONUTS_LIBS)" all

$(OUTPUT_OBJ_DIR)/testMem.o: $(MEMORY_DIR)/testMem.c
	$(AT)$(CC) -I$(DONUTS_DIR) -I$(MEMORY_DIR) -I$(MEMORY_SELECT_DIR) -I$(OUTPUT_INC_DIR) \
	$(CFLAGS) $(CC_OUTPUT)$@ `$(call fixcygpath, $<)`

# define ''clean' target

clean: verify_for_clean
	$(AT)rm -rf $(OUTPUT_OBJ_DIR)/pcsl_memory.o
	$(AT)rm -rf $(OUTPUT_OBJ_DIR)/pcsl_chunkheap.o
	$(AT)rm -rf $(OUTPUT_OBJ_DIR)/pcsl_chunkmmap_linux.o
	$(AT)rm -rf $(OUTPUT_INC_DIR)/pcsl_memory.h
	$(AT)rm -rf $(OUTPUT_INC_DIR)/pcsl_memory_impl.h
	$(AT)rm -rf $(OUTPUT_LIB_DIR)/libpcsl_memory$(LIB_EXT)
	$(AT)rm -rf $(OUTPUT_OBJ_DIR)/testMem.o
	$(AT)rm -rf $(OUTPUT_BIN_DIR)/donuts$(EXE)
	$(AT)rm -rf $(OUTPUT_GEN_DIR)/donuts_generated.c
	$(AT)rm -rf $(DOC_DIR)

.PHONY: all clean donuts doc verify
