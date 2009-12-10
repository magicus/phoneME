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
# This source file is specific for Qt-based configurations.
#
######################################################################
#
# Makefile for building and packaging.
#
######################################################################

# Workspace directory
PCSL_DIR		= $(CURDIR)/../../..
NETWORK_MODULE		= bsd/qte

# include top.gmk for various directory and module definitions
ifdef PCSL_PLATFORM
include $(PCSL_DIR)/makefiles/top.gmk
endif

# define 'all' target and all dependencies
# 'all' is the default target

# Default Qt directory
ifndef QTOPIA_SDK_DIR
export QTOPIA_SDK_DIR=/opt/Qtopia
endif

LIBNAME=$(OUTPUT_LIB_DIR)/libpcsl_network.a

QT_CFLAGS=-pipe -fno-exceptions -fno-rtti -W -Wall

OBJS = $(OUTPUT_OBJ_DIR)/pcsl_network.o \
       $(OUTPUT_OBJ_DIR)/pcsl_network_na_qte.o \
       $(OUTPUT_OBJ_DIR)/pcsl_network_qte.o \
       $(OUTPUT_OBJ_DIR)/pcsl_socket.o

ifeq ($(USE_DATAGRAM),true)
OBJS += $(OUTPUT_OBJ_DIR)/pcsl_datagram.o
endif

ifeq ($(USE_SERVER_SOCKET), true)
OBJS += $(OUTPUT_OBJ_DIR)/pcsl_serversocket.o
endif

all: verify $(OUTPUT_OBJ_DIR) $(OUTPUT_LIB_DIR) $(OUTPUT_INC_DIR) $(OUTPUT_GEN_DIR)\
     $(LIBNAME)
	@cp -f $(CURDIR)/../../pcsl_network.h $(OUTPUT_INC_DIR)
	@cp -f $(CURDIR)/../pcsl_network_md.h $(OUTPUT_INC_DIR)
	@cp -f $(CURDIR)/../../pcsl_socket.h $(OUTPUT_INC_DIR)
	@cp -f $(CURDIR)/../../pcsl_network_notifier.h $(OUTPUT_INC_DIR)
	@cp -f $(CURDIR)/pcsl_network_qte.h $(OUTPUT_INC_DIR)
ifeq ($(USE_DATAGRAM),true)
	@cp -f $(CURDIR)/../../pcsl_datagram.h $(OUTPUT_INC_DIR)
endif
ifeq ($(USE_SERVER_SOCKET),true)
	@cp -f $(CURDIR)/../../pcsl_serversocket.h $(OUTPUT_INC_DIR)
endif

include $(PCSL_DIR)/makefiles/verify.gmk

$(LIBNAME): $(OBJS)
	@$(AR) $(AR_OUTPUT)$(LIBNAME) `$(call fixcygpath, $^)`
	@ranlib $(LIBNAME)

$(OUTPUT_OBJ_DIR)/pcsl_network.o: ../pcsl_network.c
	@$(CC) -I. -I.. -I../.. -I$(MEMORY_DIR) -I$(MEMORY_SELECT_DIR) -I$(OUTPUT_INC_DIR)\
 	 $(CFLAGS) $(CC_OUTPUT)$@ `$(call fixcygpath, $<)`

$(OUTPUT_OBJ_DIR)/pcsl_socket.o: ../pcsl_socket.c
	@$(CC) -I. -I.. -I../.. $(CFLAGS) $(CC_OUTPUT)$@ `$(call fixcygpath, $<)`

$(OUTPUT_OBJ_DIR)/pcsl_network_na_qte.o: $(CURDIR)/pcsl_network_na_qte.cpp
	@$(CPP) -I. -I.. -I../.. -I$(QTOPIA_SDK_DIR)/include \
	 $(CFLAGS) $(QT_CFLAGS) $(CC_OUTPUT)$@ `$(call fixcygpath, $<)`

$(OUTPUT_GEN_DIR)/moc_pcsl_network_qte.cpp: $(CURDIR)/pcsl_network_qte.h
	@$(QTOPIA_SDK_DIR)/bin/moc -i $< -o $@

$(OUTPUT_OBJ_DIR)/pcsl_network_qte.o: pcsl_network_qte.cpp \
    $(OUTPUT_GEN_DIR)/moc_pcsl_network_qte.cpp
	@$(CPP) -I. -I.. -I../.. -I$(OUTPUT_INC_DIR) -I$(OUTPUT_GEN_DIR) \
	 -I$(QTOPIA_SDK_DIR)/include \
	 $(CFLAGS) $(QT_CFLAGS) $(CC_OUTPUT)$@ `$(call fixcygpath, $<)`

$(OUTPUT_OBJ_DIR)/pcsl_datagram.o: ../pcsl_datagram.c
	@$(CC) -I. -I.. -I../.. $(CFLAGS) $(CC_OUTPUT)$@ `$(call fixcygpath, $<)`

$(OUTPUT_OBJ_DIR)/pcsl_serversocket.o: ../pcsl_serversocket.c
	@$(CC) -I. -I.. -I../.. $(CFLAGS) $(CC_OUTPUT)$@ `$(call fixcygpath, $<)`

$(OUTPUT_OBJ_DIR) $(OUTPUT_LIB_DIR) $(OUTPUT_INC_DIR) $(OUTPUT_GEN_DIR):
	@mkdir -p $@

# doc stuff. The 'doc' target is defined in Docs.gmk.
# Just have to define DOXYGEN_INPUT_LIST and force
# rebuild

DOXYGEN_INPUT_LIST += $(NETWORK_DIR) \
                      $(NETWORK_DIR)/bsd
FRC_DOC_REBUILD = force_doc_rebuild
include $(PCSL_DIR)/makefiles/share/Docs.gmk

# define ''clean' target

clean: verify_for_clean
	rm -rf $(OUTPUT_OBJ_DIR)/pcsl_network.o
	rm -rf $(OUTPUT_OBJ_DIR)/pcsl_socket.o
	rm -rf $(OUTPUT_OBJ_DIR)/pcsl_serversocket.o
	rm -rf $(OUTPUT_OBJ_DIR)/pcsl_datagram.o
	rm -rf $(OUTPUT_OBJ_DIR)/pcsl_network_na_qte.o
	rm -rf $(OUTPUT_OBJ_DIR)/pcsl_network_qte.o
	rm -rf $(OUTPUT_INC_DIR)/pcsl_network.h
	rm -rf $(OUTPUT_INC_DIR)/pcsl_network_md.h
	rm -rf $(OUTPUT_INC_DIR)/pcsl_network_qte.h
	rm -rf $(OUTPUT_GEN_DIR)/pcsl_network_na_qte.cpp
	rm -rf $(OUTPUT_GEN_DIR)/pcsl_network_notifier.cpp
	rm -rf $(OUTPUT_INC_DIR)/pcsl_datagram.h
	rm -rf $(OUTPUT_INC_DIR)/pcsl_socket.h
	rm -rf $(OUTPUT_INC_DIR)/pcsl_serversocket.h
	rm -rf $(OUTPUT_LIB_DIR)/libpcsl_network$(LIB_EXT)
	rm -rf $(DOC_DIR)

.PHONY: all clean donuts doc verify
