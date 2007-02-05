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

.PHONY: all local_win32

all: javacall_common local_win32

JAVACALL_PLATFORM=win32

JAVACALL_UTIL_DIR := ../../utils
include $(JAVACALL_UTIL_DIR)/build/base.gmk

# Extra settings for JSR 238 implementation

# Avoiding using local digits in number formatting (required for TCK tests passing)
#
JSR_238_FORMATTER_DO_NOT_USE_NATIVE_DIGITS = TRUE

# Placing negative sign after the number will cause TCK to fail
#
JSR_238_FORMATTER_NEGATIVE_SIGN_ALWAYS_FIRST = TRUE

# Enable multiple runMidlet instances running on the same machine
# Setting this to TRUE disables remote control of running runMidlet instance
# implemented vie interprocess communication
#
USE_MULTIPLE_INSTANCES = TRUE


local_win32: $(JAVACALL_OUTPUT_LIB_DIR)/emulator.res $(JAVACALL_OUTPUT_LIB_DIR)/javacall$(BUILD_EXT).lib

vpath
vpath %.c ../../utils
vpath %.c ../src/midp
vpath %.c ../src/jsr120_wma
vpath %.c ../src/jsr177_satsa
vpath %.c ../src/jsr179_location
vpath %.c ../src/jsr205_wma20
vpath %.c ../src/jsr211_chapi
vpath %.c ../src/jsr238_mi18n
vpath %.c ../src/jsr75_pim_fc
vpath %.c ../src/jsr82_bt
vpath %.rc ../src/midp/res

ifdef SUBSYSTEM_EVENTS_MODULES
   ifeq ($(SUBSYSTEM_EVENTS_MODULES), slave_mode)
      EVENTS_OBJ=events_slave_mode.obj
   else
      EVENTS_OBJ=events.obj
   endif
else
   EVENTS_OBJ=events.obj
endif

PORTING_OBJECTS = \
	annunciator.obj	\
	dir.obj         \
	file.obj	\
	font.obj	\
	lcd.obj		\
	lifecycle.obj	\
	logging.obj	\
	memory.obj	\
	network.obj	\
	sms.obj		\
        mms.obj         \
        cbs.obj         \
	wma_UDPEmulator.obj \
	time.obj	\
	main.obj	\
        $(EVENTS_OBJ)      \
        input.obj       \
        serial.obj      \
        datagram.obj    \
        image.obj       \
        security.obj    \
        push.obj        \
	location.obj	\
	landmarkstore.obj \
	fileconnection.obj \
        pti.obj          \
        pim.obj         \
	carddevice_simu.obj \
        chapi.obj \
	mi18n_collation.obj \
	mi18n_format.obj \
	mi18n_resources.obj \
	mi18n_locales.obj

#Specify list of utility objects necessary for the build
UTILITIES_LIST :=           \
        javautil_jad_parser \
        javautil_string     \
        javautil_unicode

PORTING_OBJECTS += $(foreach util,$(UTILITIES_LIST),$(util).obj)
JAVACALL_INCLUDE_SOURCE_FILES_SET+=$(foreach util,$(UTILITIES_LIST),$(JAVACALL_UTIL_DIR)/inc/$(util).h)

ifneq ($(UTILITIES_LIST)x, x)
VPATH+=$(JAVACALL_UTIL_DIR)
endif

MIDP_IMPL_INCLUDE_DIR = ../src/midp

ifeq ($(USE_JSR_135),true)
JSR_135_DIR ?= ../src/jsr135_mmapi
PROJECT_JSR_135_DIR ?= $(JSR_135_DIR)
include $(PROJECT_JSR_135_DIR)/javacall_impl.gmk
endif

ifeq ($(USE_JSR_234),true)
include $(PROJECT_JSR_234_DIR)/javacall_impl.gmk
endif

include $(JAVACALL_UTIL_DIR)/build/common.gmk

# This is a hidden dependencies workaround (ti762)
# NEED REVISIT: provide pure solution without hidden dependencies
ifeq ($(ENABLE_HIDDEN_DEP_FEATURES),true)
HIDDEN_DEP_FEATURES_DEF=-DENABLE_HIDDEN_DEP_FEATURES
else
HIDDEN_DEP_FEATURES_DEF=
endif # ifeq ($(ENABLE_HIDDEN_DEP_FEATURES),true)


CC=cl.exe
CXX=cl.exe
RC=rc.exe

vpath %.obj $(JAVACALL_OUTPUT_OBJ_DIR)

LOCAL_CFLAGS_debug = -Gm -GX -Zi -Od -D_DEBUG -D_MBCS -D_LIB -YX -FD -GZ -W3
LOCAL_CFLAGS=-ML -GX -O2 -D_MBCS -D_LIB /YX -FD -W3  

ifneq ($(findstring CYGWIN, $(shell uname)), CYGWIN)
    define fixcygpath_m
    echo $(1)
    endef
else
    define fixcygpath_m
    echo $(1) | xargs -n1 cygpath -m
    endef
endif


CXXFLAGS=-nologo $(LOCAL_CFLAGS_$(BUILD))
CFLAGS=-nologo $(LOCAL_CFLAGS_$(BUILD))
CPPFLAGS+= -I$(JAVACALL_OUTPUT_INCLUDE_DIR)
CPPFLAGS+= -I$(MIDP_IMPL_INCLUDE_DIR) 
CPPFLAGS+= $(addprefix -I, $(ADDITIONAL_INCLUDE_DIRS))
CPPFLAGS+= -DWIN32
CPPFLAGS+= $(HIDDEN_DEP_FEATURES_DEF)

ifeq ($(JSR_238_FORMATTER_DO_NOT_USE_NATIVE_DIGITS), TRUE)
CPPFLAGS += -DNOT_USE_NATIVE_DIGITS=1 
endif

ifeq ($(JSR_238_FORMATTER_NEGATIVE_SIGN_ALWAYS_FIRST), TRUE)
CPPFLAGS += -DMINUS_ALWAYS_INFRONT=1 
endif

ifeq ($(USE_MULTIPLE_INSTANCES), TRUE)
CPPFLAGS += -DENABLE_MULTIPLE_INSTANCES=1
endif

AR=lib.exe 
ARFLAGS=-nologo -MACHINE:X86

OUTPUT_OPTION=-Fo$@  \
              -Fp$(JAVACALL_OUTPUT_OBJ_DIR)/javacall \
              -Fd$(JAVACALL_OUTPUT_OBJ_DIR)/javacall

$(PORTING_OBJECTS): ../src/midp/local_defs.h

$(JAVACALL_OUTPUT_LIB_DIR)/%.res: %.rc
	@echo "...compiling resources ... "
	@$(RC) /l 0x409 /d "NDEBUG" /fo"$@" $<

$(JAVACALL_OUTPUT_OBJ_DIR)/%.obj: %.cpp
	@echo -n "...compiling: "
	@$(COMPILE.cc) $(OUTPUT_OPTION) $<

$(JAVACALL_OUTPUT_OBJ_DIR)/%.obj: %.c
	@echo -n "...compiling: "
	@$(COMPILE.c) $(OUTPUT_OPTION) $<

$(JAVACALL_OUTPUT_LIB_DIR)/javacall$(BUILD_EXT).lib: \
	$(patsubst %,$(JAVACALL_OUTPUT_OBJ_DIR)/%,$(PORTING_OBJECTS)) 
	@echo "...Generating Library: $@"
	@$(AR) -out:`$(call fixcygpath_m, $@)` $(ARFLAGS) `$(call fixcygpath_m, $^)` $(ADDITIONAL_LIBS)

