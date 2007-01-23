#
#
# Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

.PHONY: all local_stubs

all: javacall_common local_stubs

ifeq ($(CC),armcc)
  CFLAGS= -c
  AR=armar.exe 
  ARFLAGS=-create $@
  OUTPUT_OPTION=-o$@
  JAVACALL_PLATFORM=arm
else
  ifeq ($(CC),gcc)
    CFLAGS= -c
    #AR=gcc
    #ARFLAGS=-o$@
    ARFLAGS=-rf lib/javacall.lib
    AR=ar.exe 
    OUTPUT_OPTION=-o$@
    JAVACALL_PLATFORM=gcc
  else
    CFLAGS=-nologo /c /W3
    CC=cl.exe
    AR=lib.exe 
    ARFLAGS=-nologo -out:$@
    OUTPUT_OPTION=-Fo$@
    JAVACALL_PLATFORM=win32
  endif
endif

include ../../utils/build/common.gmk

local_stubs: $(JAVACALL_OUTPUT_LIB_DIR)/javacall$(BUILD_EXT).lib 

vpath

vpath %.c ../../utils
vpath %.c ../src/midp
vpath %.c ../src/jsr120_wma
vpath %.c ../src/jsr135_mmapi
vpath %.c ../src/jsr177_satsa
vpath %.c ../src/jsr179_location
vpath %.c ../src/jsr205_wma20
vpath %.c ../src/jsr211_chapi
vpath %.c ../src/jsr234_amms
vpath %.c ../src/jsr238_mi18n
vpath %.c ../src/jsr75_pim_fc
vpath %.c ../src/jsr82_bt

PORTING_OBJECTS= \
	annunciator.obj	\
	dir.obj		\
	file.obj	\
	font.obj	\
	lcd.obj		\
	lifecycle.obj	\
	logging.obj	\
	memory.obj	\
	multimedia.obj	\
	network.obj	\
	push.obj	\
	cbs.obj		\
	sms.obj		\
	mms.obj		\
	socket.obj	\
	events.obj	\
	time.obj	\
	main.obj \
	input.obj \
	serial.obj \
	datagram.obj \
	image.obj \
	security.obj \
	push.obj \
	fileconnection.obj \
	pim.obj \
	landmarkstore.obj \
	location.obj \
	pti.obj \
	bt.obj \
	vscl.obj \
	sprint.obj \
	chapi.obj \
	carddevice_simu.obj \
	mi18n_collation.obj \
	mi18n_format.obj \
	mi18n_resources.obj


UTILITY_OBJECTS = \
        javautil_jad_parser.obj \
        javautil_string.obj

include $(JAVACALL_INCLUDE_DIR)/include.gmk
  JAVAUTIL_INCLUDE_DIR = ../../utils/inc

ifeq ($(USE_DEBUG),true)
ifeq ($(CC),armcc)
	CFLAGS+= -g
else
	CFLAGS+= -02 -g+
endif
endif

BUILD_EXT_debug=_g
BUILD_EXT=$(BUILD_EXT_$(BUILD))

vpath %.obj $(JAVACALL_OUTPUT_OBJ_DIR)

CFLAGS+= -I$(JAVACALL_OUTPUT_INCLUDE_DIR) -I$(JAVAUTIL_INCLUDE_DIR) -I../../include

$(PORTING_OBJECTS): local_defs.h

$(JAVACALL_OUTPUT_OBJ_DIR)/%.obj: %.c
ifeq ($(CC),cl)
	@echo -n "...compiling: "
else
	@echo "...compiling: $<"
endif
	@$(CC) $(CFLAGS) $(OUTPUT_OPTION) $<

$(JAVACALL_OUTPUT_LIB_DIR)/javacall$(BUILD_EXT).lib: $(patsubst %,$(JAVACALL_OUTPUT_OBJ_DIR)/%,$(PORTING_OBJECTS)) \
                              $(patsubst %,$(JAVACALL_OUTPUT_OBJ_DIR)/%,$(UTILITY_OBJECTS))
	@echo "...Generating Library: $@"
	@$(AR) $(ARFLAGS) $^

