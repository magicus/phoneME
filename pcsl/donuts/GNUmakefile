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

all:  $(OUTPUT_BIN_DIR)/donuts$(EXE)

include $(PCSL_DIR)/makefiles/verify.gmk

ifndef DONUTS_FILES
DONUTS_FILES := $(shell ls \
  $(MEMORY_DIR)/test*.c \
  $(FILE_DIR)/test*.c \
  $(ESCFILENAMES_DIR)/test*.c \
  $(PRINT_DIR)/test*.c \
  $(NETWORK_SELECT_DIR)/test*.c \
  $(STRING_DIR)/test*.c \
  2>/dev/null)
endif

ifndef DONUTS_LIBS
DONUTS_LIBS = $(OUTPUT_LIB_DIR)/libpcsl_print$(LIB_EXT) \
	      $(OUTPUT_LIB_DIR)/libpcsl_file$(LIB_EXT) \
	      $(OUTPUT_LIB_DIR)/libpcsl_escfilenames$(LIB_EXT) \
	      $(OUTPUT_LIB_DIR)/libpcsl_memory$(LIB_EXT) \
	      $(OUTPUT_LIB_DIR)/libpcsl_string$(LIB_EXT) \
	      $(OUTPUT_LIB_DIR)/libpcsl_network$(LIB_EXT)

$(DONUTS_LIBS):
	$(AT)cd $(PCSL_DIR); $(MAKE) all
endif

ifndef DONUTS_OBJS

DONUTS_OBJS=$(addprefix $(OUTPUT_OBJ_DIR)/,$(notdir $(DONUTS_FILES:.c=.o)))

DONUTS_DIRS=$(sort $(dir $(DONUTS_FILES)))
vpath %.c $(DONUTS_DIRS)

$(DONUTS_OBJS): $(OUTPUT_OBJ_DIR)/%.o : %.c
	$(AT)$(CC) -I$(OUTPUT_INC_DIR) -I$(DONUTS_DIR) \
        $(CFLAGS) $(CC_OUTPUT)$@ `$(call fixcygpath, $<)`

endif

# change the list of donuts test files of the form /a/b/testFoo.c to 
# a list of the form testFoo
DONUTS_NAME := $(foreach f,$(DONUTS_FILES),$(basename $(notdir $(f))))

$(OUTPUT_GEN_DIR)/donuts_generated.c: $(DONUTS_FILES)
	$(AT)sh donuts.sh $(DONUTS_NAME) > $@

$(OUTPUT_OBJ_DIR)/donuts_generated.o: $(OUTPUT_GEN_DIR)/donuts_generated.c
	$(AT)$(CC) -I. $(CFLAGS) $(CC_OUTPUT)$@ `$(call fixcygpath, $<)`

$(OUTPUT_OBJ_DIR)/donuts.o: donuts.c
	$(AT)$(CC) -I. $(CFLAGS) $(CC_OUTPUT)$@ `$(call fixcygpath, $<)`

#$(OUTPUT_OBJ_DIR)/testFile.o: $(FILE_DIR)/testFile.c
#	@$(CC) -I$(DONUTS_SELECT_DIR) -I$(OUTPUT_INC_DIR) $(CFLAGS) $(CC_OUTPUT)$@ `$(call fixcygpath, $<)`

#$(OUTPUT_OBJ_DIR)/testMem.o: $(MEMORY_DIR)/testMem.c
#	@$(CC) -I$(DONUTS_SELECT_DIR) -I$(OUTPUT_INC_DIR) $(CFLAGS) $(CC_OUTPUT)$@ `$(call fixcygpath, $<)`

$(OUTPUT_BIN_DIR)/donuts$(EXE): $(OUTPUT_OBJ_DIR)/donuts.o $(OUTPUT_OBJ_DIR)/donuts_generated.o \
                                $(DONUTS_LIBS) $(DONUTS_OBJS)

	$(AT)$(LD) $(LD_FLAGS) `$(call fixcygpath, $(OUTPUT_OBJ_DIR)/donuts.o $(OUTPUT_OBJ_DIR)/donuts_generated.o $(DONUTS_OBJS))` $(LD_OUTPUT)$@ `$(call fixcygpath, $(DONUTS_LIBS))` $(LIBS)

clean: verify_for_clean
	rm -rf $(OUTPUT_OBJ_DIR)/test*.o
	rm -rf $(OUTPUT_BIN_DIR)/donuts$(EXE)
	rm -rf $(OUTPUT_GEN_DIR)/donuts_generated.c

.PHONY: all clean
