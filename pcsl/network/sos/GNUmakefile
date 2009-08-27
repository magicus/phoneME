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
PCSL_DIR		= $(CURDIR)/../..
NETWORK_MODULE		= sos

# include top.gmk for various directory and module definitions
ifdef PCSL_PLATFORM
include $(PCSL_DIR)/makefiles/top.gmk
endif

# define 'all' target and all dependencies
# 'all' is the default target

LIBNAME=$(OUTPUT_LIB_DIR)/libpcsl_network$(LIB_EXT)

SERIAL_DIR=$(CURDIR)/../serial

OBJS = $(OUTPUT_OBJ_DIR)/pcsl_network.o \
    $(OUTPUT_OBJ_DIR)/pcsl_serial_linux.o \

all : verify $(OUTPUT_OBJ_DIR) $(OUTPUT_LIB_DIR) $(OUTPUT_INC_DIR) \
     $(LIBNAME)
	@cp -f $(CURDIR)/../pcsl_network.h $(OUTPUT_INC_DIR)/
	@cp -f $(CURDIR)/pcsl_network_md.h $(OUTPUT_INC_DIR)/
	@cp -f $(CURDIR)/../pcsl_socket.h $(OUTPUT_INC_DIR)/
	@cp -f $(CURDIR)/../pcsl_datagram.h $(OUTPUT_INC_DIR)/
	@cp -f $(CURDIR)/../pcsl_network_notifier.h $(OUTPUT_INC_DIR)/
ifeq ($(USE_SERVER_SOCKET),true)
	@cp -f $(CURDIR)/../pcsl_serversocket.h $(OUTPUT_INC_DIR)/
endif

include $(PCSL_DIR)/makefiles/verify.gmk

$(LIBNAME): $(OBJS)
	@$(AR) $(AR_OUTPUT)$(LIBNAME) `$(call fixcygpath, $^)`
	@ranlib $(LIBNAME)

$(OUTPUT_OBJ_DIR)/pcsl_network.o: pcsl_network.c
	@$(CC) -g -I.. -I. -I$(SERIAL_DIR) $(CFLAGS) $(CC_OUTPUT)$@ `$(call fixcygpath, $<)`

$(OUTPUT_OBJ_DIR)/pcsl_serial_linux.o: $(SERIAL_DIR)/pcsl_serial_linux.c
	@$(CC) -g -I.. -I. -I$(SERIAL_DIR) $(CFLAGS) $(CC_OUTPUT)$@ `$(call fixcygpath, $<)`

$(OUTPUT_OBJ_DIR) $(OUTPUT_LIB_DIR) $(OUTPUT_INC_DIR):
	@mkdir -p $@

# doc stuff. The 'doc' target is defined in Docs.gmk.
# Just have to define DOXYGEN_INPUT_LIST and force
# rebuild

DOXYGEN_INPUT_LIST += $(NETWORK_DIR) \
                      $(NETWORK_DIR)/serial
FRC_DOC_REBUILD = force_doc_rebuild
include $(PCSL_DIR)/makefiles/share/Docs.gmk

# define 'donuts' and all dependencies
#

DONUTS_FILES += $(CURDIR)/testSimpleNetwork.c \
                $(CURDIR)/testInterleavedRead.c
DONUTS_OBJS += $(OUTPUT_OBJ_DIR)/testSimpleNetwork.o \
               $(OUTPUT_OBJ_DIR)/testInterleavedRead.o
DONUTS_LIBS += $(OUTPUT_LIB_DIR)/libpcsl_network$(LIB_EXT)

donuts: verify $(OUTPUT_OBJ_DIR) $(OUTPUT_LIB_DIR) $(DONUTS_LIBS) $(DONUTS_OBJS)
	@cd $(DONUTS_DIR);$(MAKE) DONUTS_FILES="$(DONUTS_FILES)" DONUTS_OBJS="$(DONUTS_OBJS)" \
                            DONUTS_LIBS="$(DONUTS_LIBS)" all

$(OUTPUT_OBJ_DIR)/testSimpleNetwork.o: $(CURDIR)/testSimpleNetwork.c
	@$(CC) -I./.. -I$(CURDIR) -I$(DONUTS_DIR) -I$(NETWORK_SELECT_DIR) \
        $(CFLAGS) $(CC_OUTPUT)$@ `$(call fixcygpath, $<)`

$(OUTPUT_OBJ_DIR)/testInterleavedRead.o: $(CURDIR)/testInterleavedRead.c
	@$(CC) -I./.. -I$(CURDIR) -I$(DONUTS_DIR) -I$(NETWORK_SELECT_DIR) \
        $(CFLAGS) $(CC_OUTPUT)$@ `$(call fixcygpath, $<)`

# define ''clean' target

clean: verify_for_clean
	rm -rf $(OUTPUT_OBJ_DIR)/pcsl_network.o
	rm -rf $(OUTPUT_OBJ_DIR)/pcsl_serial_linux.o
	rm -rf $(OUTPUT_INC_DIR)/pcsl_network.h
	rm -rf $(OUTPUT_INC_DIR)/pcsl_network_md.h
	rm -rf $(OUTPUT_INC_DIR)/pcsl_network_notifier.h
	rm -rf $(OUTPUT_LIB_DIR)/libpcsl_network$(LIB_EXT)
	rm -rf $(DOC_DIR)

.PHONY: all clean donuts doc verify
