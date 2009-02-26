#
#   
#
# Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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
FILE_MODULE		= armsd

# include top.gmk for various directory and module definitions
ifdef PCSL_PLATFORM
include $(PCSL_DIR)/makefiles/top.gmk
endif

# define 'all' target and all dependencies
# 'all' is the default target

LIBNAME=$(OUTPUT_LIB_DIR)/libpcsl_file$(LIB_EXT)
UTIL_DIR=$(CURDIR)/../util
OBJS = $(OUTPUT_OBJ_DIR)/pcsl_armsd.o \
       $(OUTPUT_OBJ_DIR)/pcsl_dir_armsd.o \
       $(OUTPUT_OBJ_DIR)/pcsl_util_filelist.o

all: verify memory $(OUTPUT_OBJ_DIR) $(OUTPUT_LIB_DIR) $(OUTPUT_INC_DIR) \
     $(LIBNAME)
	@cp -f ../pcsl_file.h $(OUTPUT_INC_DIR)
	@cp -f ../pcsl_directory.h $(OUTPUT_INC_DIR)

include $(PCSL_DIR)/makefiles/verify.gmk

$(LIBNAME): $(OBJS)
	@$(AR) $(AR_OUTPUT)$(LIBNAME) `$(call fixcygpath, $^)`
	@ranlib $(LIBNAME)

$(OUTPUT_OBJ_DIR)/pcsl_armsd.o: pcsl_armsd.c
	@$(CC)  -I.. -I../util -I $(OUTPUT_INC_DIR) \
        $(CFLAGS) $(CC_OUTPUT)$@ `$(call fixcygpath, $<)`

$(OUTPUT_OBJ_DIR)/pcsl_dir_armsd.o: pcsl_dir_armsd.c
	@$(CC)  -I.. -I../util -I $(OUTPUT_INC_DIR) \
        $(CFLAGS) $(CC_OUTPUT)$@ `$(call fixcygpath, $<)`

$(OUTPUT_OBJ_DIR)/pcsl_util_filelist.o: $(UTIL_DIR)/pcsl_util_filelist.c
	@$(CC) -I.. -I $(OUTPUT_INC_DIR) -I../util \
        $(CFLAGS) $(CC_OUTPUT)$@ `$(call fixcygpath, $<)`

$(OUTPUT_OBJ_DIR) $(OUTPUT_LIB_DIR) $(OUTPUT_INC_DIR):
	@mkdir -p $@

# doc stuff. The 'doc' target is defined in Docs.gmk.
# Just have to define DOXYGEN_INPUT_LIST and force
# rebuild

DOXYGEN_INPUT_LIST += $(FILE_DIR)
FRC_DOC_REBUILD = force_doc_rebuild
include $(PCSL_DIR)/makefiles/share/Docs.gmk

# define ''clean' target
clean: verify_for_clean
	rm -rf $(OUTPUT_OBJ_DIR)/pcsl_armsd.o
	rm -rf $(OUTPUT_OBJ_DIR)/pcsl_dir_armsd.o
	rm -rf $(OUTPUT_INC_DIR)/pcsl_file.h
	rm -rf $(OUTPUT_INC_DIR)/pcsl_directory.h
	rm -rf $(OUTPUT_LIB_DIR)/libpcsl_file$(LIB_EXT)
	rm -rf $(OUTPUT_OBJ_DIR)/testFile.o
	rm -rf $(OUTPUT_OBJ_DIR)/testDir_win32.o
	rm -rf $(OUTPUT_BIN_DIR)/donuts$(EXE)
	rm -rf $(OUTPUT_GEN_DIR)/donuts_generated.c
	rm -rf $(DOC_DIR)

.PHONY: all clean doc verify
