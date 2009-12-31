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
# Makefile for building the Native Application Manager Sample (Win32)
#
# Before building the Application Manager, javacall, pcsl, cldc and
# midp components must be built.
#
# To use this makefile, the following variables must be set:
#
# WIN_DRIVE       Windows drive letter where the sources are located
#                 (for example, c:)
# COMPONENTS_DIR  must point to the directory containing the sources
#                 of midp, cldc, pcsl and javacall components
# JAVACALL_OUTPUT_DIR must point to the directory containing the
#                 result of the javacall component build
# PCSL_OUTPUT_DIR must point to the directory containing the results
#                 of the pcsl component build
# CLDC_OUTPUT_DIR must point to the directory containing the results
#                 of the cldc component build
# MIDP_OUTPUT_DIR must point to the directory containing the results
#                 of midp component build
#
# All paths contained in these variables (except WIN_DRIVE) must be
# given in Cygwin (Linux/Unix) notation (for example, /myhome/phoneME)
#
######################################################################

USE_DEBUG ?= true

# use "make A= " for verbosity
A ?= @

ifeq ($(USE_DEBUG), true)
DEBUG_SUFFIX=_g
else
DEBUG_SUFFIX=
endif

# macros verifying the build environment
define verify_directory
    if [ -z "$($(1))" ]; then \
        echo "ERROR: \"$(1)\" environment variable must be set"; \
        exit -1; \
    else \
        if [ ! -d "$($(1))" ]; then \
            echo "ERROR: $(1) ($($(1))) does not point to a valid directory"; \
            exit -1; \
        fi \
    fi
endef

define verify_environment
    $(call verify_directory,WIN_DRIVE); \
    $(call verify_directory,COMPONENTS_DIR); \
    $(call verify_directory,JAVACALL_OUTPUT_DIR); \
    $(call verify_directory,PCSL_OUTPUT_DIR); \
    $(call verify_directory,CLDC_OUTPUT_DIR); \
    $(call verify_directory,MIDP_OUTPUT_DIR);
endef


###############################################################################
#
# Definitions for compiler flags, libraries, etc.
#
###############################################################################

# directory containg the sources of the application manager
SRC_DIR=javacall

# will be copied to the destination directory after the build
VC_PDB=vc70.pdb

PCSL_LIB_DIR?=$(PCSL_OUTPUT_DIR)/javacall_i386/lib
CLDC_LIB?=$(WIN_DRIVE)$(CLDC_OUTPUT_DIR)/javacall_i386_vc/dist/lib/cldc_vm$(DEBUG_SUFFIX).lib

PCSL_LIB=$(WIN_DRIVE)$(PCSL_LIB_DIR)/libpcsl_file.lib \
         $(WIN_DRIVE)$(PCSL_LIB_DIR)/libpcsl_memory.lib \
         $(WIN_DRIVE)$(PCSL_LIB_DIR)/libpcsl_network.lib \
         $(WIN_DRIVE)$(PCSL_LIB_DIR)/libpcsl_print.lib \
         $(WIN_DRIVE)$(PCSL_LIB_DIR)/libpcsl_string.lib \
         $(WIN_DRIVE)$(PCSL_LIB_DIR)/libpcsl_escfilenames.lib

JAVACALL_LIB=$(WIN_DRIVE)$(JAVACALL_OUTPUT_DIR)/lib/javacall$(DEBUG_SUFFIX).lib

MIDP_OBJ_DIR=$(MIDP_OUTPUT_DIR)/obj$(DEBUG_SUFFIX)/i386
MIDP_EXE_DIR=$(MIDP_OUTPUT_DIR)/bin/i386
MIDP_LIB=$(WIN_DRIVE)$(MIDP_OBJ_DIR)/libobj.lib

RC  = rc
CC  = cl
CPP = cl
LD  = link
LD_OUTPUT = -out:
CC_OUTPUT = -Fo

OBJ = .obj
EXE = .exe

CFLAGS = /c /D_UNICODE /DUNICODE

ifeq ($(USE_DEBUG), true)

DEBUG_CFLAGS 	+= -Zi -Od -MDd
LD_FLAGS 	+= -debug 
# Force use of debug libraries
LD_FLAGS	+= -nodefaultlib:msvcrt.lib msvcrtd.lib

else # USE_DEBUG

# flags inherited from the VM : -Ox -Os -Gy -GF
EXTRA_CFLAGS    += -nologo -DWIN32 -D_WINDOWS 
EXTRA_CFLAGS 	+= -MD
LD_FLAGS 	+= -merge:CODESEGMENT=.text -merge:DATASEGMENT=.data \
		   -opt:REF -opt:ICF,8
# Force use of non-debug libraries
LD_FLAGS 	+= -nodefaultlib:msvcrtd.lib msvcrt.lib

endif # USE_DEBUG

EXTRA_CFLAGS += -I$(WIN_DRIVE)$(JAVACALL_OUTPUT_DIR)/inc

LIBS += user32.lib gdi32.lib kernel32.lib winmm.lib wsock32.lib vfw32.lib shell32.lib comdlg32.lib comctl32.lib
LIBS += $(JAVACALL_LIB) $(PCSL_LIB) $(CLDC_LIB) $(MIDP_LIB)

###############################################################################
#
# Rules for builing appManager
#
###############################################################################

all: environment appManager background splashscreen

environment:
	$(A)$(verify_environment)

appManager: $(MIDP_OBJ_DIR)/appManager$(OBJ) $(MIDP_OBJ_DIR)/javacallImpl$(OBJ) $(MIDP_OBJ_DIR)/appManagerUtils$(OBJ) $(MIDP_OBJ_DIR)/appManagerProgress$(OBJ) $(MIDP_OBJ_DIR)/appManagerPermissions$(OBJ) $(MIDP_OBJ_DIR)/appManagerLCDUI$(OBJ) $(MIDP_OBJ_DIR)/appManager_resource.res
	$(A)$(LD) $(LD_FLAGS) $(WIN_DRIVE)$(MIDP_OBJ_DIR)/appManager$(OBJ) $(WIN_DRIVE)$(MIDP_OBJ_DIR)/javacallImpl$(OBJ) $(WIN_DRIVE)$(MIDP_OBJ_DIR)/appManagerUtils$(OBJ) $(WIN_DRIVE)$(MIDP_OBJ_DIR)/appManagerProgress$(OBJ) $(WIN_DRIVE)$(MIDP_OBJ_DIR)/appManagerPermissions$(OBJ) $(WIN_DRIVE)$(MIDP_OBJ_DIR)/appManagerLCDUI$(OBJ) $(WIN_DRIVE)$(MIDP_OBJ_DIR)/appManager_resource.res $(LIBS) $(LD_OUTPUT)$(WIN_DRIVE)$(MIDP_EXE_DIR)/appManager$(EXE)
	$(A)cp $(VC_PDB) $(WIN_DRIVE)$(MIDP_EXE_DIR)

$(MIDP_OBJ_DIR)/%$(OBJ): $(SRC_DIR)/%.cpp $(SRC_DIR)/*.h
	$(A)$(CPP) $(CFLAGS) $(DEBUG_CFLAGS) $(EXTRA_CFLAGS) $< $(CC_OUTPUT)$@

$(MIDP_OBJ_DIR)/%.res: $(SRC_DIR)/res/%.rc $(SRC_DIR)/res/toolbar.bmp $(SRC_DIR)/res/*.ico
	$(A)$(RC) $(RCFLAGS)  /d "NDEBUG" /fo $@ $<

background: $(MIDP_EXE_DIR)/background.bmp

splashscreen: $(MIDP_EXE_DIR)/splash_screen_240x320.bmp 

$(MIDP_EXE_DIR)/%.bmp: $(SRC_DIR)/res/%.bmp
	$(A)cp $< $(WIN_DRIVE)$(MIDP_EXE_DIR)

.PHONY: all environment
