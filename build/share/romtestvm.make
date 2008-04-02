#
#   
#
# Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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

#----------------------------------------------------------------------
#
# Include exported compiler options from the VM in $(CLDC_MAK).
# These macros are defined in $(CLDC_MAK)
#
#     CPP
#     CC
#     LINK
#
#     BUILD_EXT
#     OBJ_SUFFIX
#     LIB_PREFIX
#     LIB_SUFFIX
#     EXE_SUFFIX
#     CPP_FLAGS
#     CC_FLAGS
#     LINK_FLAGS
#     JVM_LIB_NAME
#
# These macros defines the values used to build this VM binaries.
# Note that CPP, CC and LINK may contain pathnames of the computer that
# created this VM binaries. If they do not match the pathnames of
# your computer, you can override their values using:
#
# 	gnumake CPP=/usr/local/stuff/gcc
#
# Except for CPP, CC and LINK, all the other macros do not contain
# pathnames so you should be able to use them without change.
#----------------------------------------------------------------------

BUILD_EXT_debug      = _g
BUILD_EXT_release    = _r
BUILD_EXT_product    =
BUILD_EXT            = $(BUILD_EXT_$(BUILD))
DIST_DIR             = ../../dist

JVM_MAKE             = $(DIST_DIR)/lib/cldc_vm$(BUILD_EXT).make
JVM_LIB              = $(DIST_DIR)/lib/$(JVM_LIB_NAME)
JVMX_LIB             = $(DIST_DIR)/lib/$(JVMX_LIB_NAME)
JVMTEST_LIB          = $(DIST_DIR)/lib/$(JVMTEST_LIB_NAME)
ANI_LIB              = $(DIST_DIR)/lib/$(ANI_LIB_NAME)
ANIX_LIB             = $(DIST_DIR)/lib/$(ANIX_LIB_NAME)
ROMTESTVM            = romtestvm$(BUILD_EXT)$(EXE_SUFFIX)
AMS_ROMTESTVM        = ams_romtestvm$(BUILD_EXT)$(EXE_SUFFIX)
ANI_ROMTESTVM        = ani_romtestvm$(BUILD_EXT)$(EXE_SUFFIX)
TEST_DLL             = testlib$(BUILD_EXT)$(DLL_SUFFIX)
TEST_SRC_DIR         = $(WorkSpace)/src/tests

include $(JVM_MAKE)

BUILD_EXT                     = $(JVM_BUILD_EXT)
OBJ_SUFFIX                    = $(JVM_OBJ_SUFFIX)
DLL_SUFFIX                    = $(JVM_DLL_SUFFIX)
LIB_PREFIX                    = $(JVM_LIB_PREFIX)
LIB_SUFFIX                    = $(JVM_LIB_SUFFIX)
EXE_SUFFIX                    = $(JVM_EXE_SUFFIX)
HOST_EXE_SUFFIX               = $(JVM_HOST_EXE_SUFFIX)
CC_FLAGS                      = $(JVM_CC_FLAGS)
CPP_FLAGS                     = $(JVM_CPP_FLAGS)
CC_OPT_FLAGS                  = $(JVM_CC_OPT_FLAGS)
CPP_OPT_FLAGS                 = $(JVM_CPP_OPT_FLAGS)
ARM_CFLAGS                    = $(JVM_ARM_CFLAGS)
THUMB_CFLAGS                  = $(JVM_THUMB_CFLAGS)
LINK_FLAGS                    = $(JVM_LINK_FLAGS)
JVM_LIB_NAME                  = $(JVM_JVM_LIB_NAME)
JVMX_LIB_NAME                 = $(JVM_JVMX_LIB_NAME)
JVMTEST_LIB_NAME              = $(JVM_JVMTEST_LIB_NAME)
ANI_LIB_NAME                  = $(JVM_ANI_LIB_NAME)
ANIX_LIB_NAME                 = $(JVM_ANIX_LIB_NAME)
CC                            = $(JVM_CC)
CPP                           = $(JVM_CPP)
ASM                           = $(JVM_ASM)
LINK                          = $(JVM_LINK)
LINK_OUT_SWITCH1              = $(JVM_LINK_OUT_SWITCH1)
LINK_OUT_SWITCH2              = $(JVM_LINK_OUT_SWITCH2)
EXTRA_LIBS                    = $(JVM_EXTRA_LIBS)
THREAD_LIBS                   = $(JVM_THREAD_LIBS)
ROMIZING                      = $(JVM_ROMIZING)
ENABLE_FLOAT                  = $(JVM_ENABLE_FLOAT)
ENABLE_SOFT_FLOAT             = $(JVM_ENABLE_SOFT_FLOAT)
ENABLE_JAVA_DEBUGGER          = $(JVM_ENABLE_JAVA_DEBUGGER)
ENABLE_MEMORY_PROFILER        = $(JVM_ENABLE_MEMORY_PROFILER)
ENABLE_ISOLATES               = $(JVM_ENABLE_ISOLATES)
ENABLE_REFLECTION             = $(JVM_ENABLE_REFLECTION)
ENABLE_DYNAMIC_NATIVE_METHODS = $(JVM_ENABLE_DYNAMIC_NATIVE_METHODS)
ENABLE_METHOD_TRAPS           = $(JVM_ENABLE_METHOD_TRAPS)
ENABLE_PROFILER               = $(JVM_ENABLE_PROFILER)
PATHSEP                       = $(JVM_PATHSEP)
VM_os                         = $(JVM_os)
VM_arch                       = $(JVM_arch)
VM_host_os                    = $(JVM_host_os)
VM_host_arch                  = $(JVM_host_arch)


OBJS                 = ../../target/$(BUILD)/jvmspi$(OBJ_SUFFIX) \
                       ROMImage$(OBJ_SUFFIX) \
                       NativesTable$(OBJ_SUFFIX) \
                       InternalNatives$(OBJ_SUFFIX) \
                       KniNatives$(OBJ_SUFFIX) \
                       SniNatives$(OBJ_SUFFIX) \
                       IsolateTestNatives$(OBJ_SUFFIX)

SNI_OBJS             = $(OBJS) \
                       ../../target/$(BUILD)/Main_$(os_family)$(OBJ_SUFFIX) \
                       AniNativesDummies$(OBJ_SUFFIX)

AMS_OBJS             = $(OBJS) \
                       AmsMain$(OBJ_SUFFIX) \
                       AniNativesDummies$(OBJ_SUFFIX)

ANI_OBJS             = $(OBJS) \
                       AniNatives$(OBJ_SUFFIX) \
                       AniMain$(OBJ_SUFFIX)

DLL_OBJS             = DLLNatives$(OBJ_SUFFIX)


INCLUDE_DIRS         = -I$(DIST_DIR)/include \
                       -I../../target/generated \
                       -I../

CPP_FLAGS           += $(INCLUDE_DIRS)
CC_FLAGS            += $(INCLUDE_DIRS)

ifeq ($(os_family), javacall)
CPP_FLAGS           += -DNEED_CHECK_EVENTS
endif

all: $(ROMTESTVM)
all: $(AMS_ROMTESTVM)

ifeq ($(os_family), linux)
all: $(TEST_DLL)
endif

ifeq ($(BUILD_ANI_LIB), true)
all: $(ANI_ROMTESTVM)
endif

$(ROMTESTVM): $(SNI_OBJS) $(JVMX_LIB) $(JVMTEST_LIB) $(JVM_LIB)
	@echo linking $@ ...
	@$(LINK) $(SNI_OBJS) $(JVMX_LIB) $(JVMTEST_LIB) $(JVM_LIB) \
		$(EXTRA_LIBS) $(THREAD_LIBS) $(LINK_FLAGS) $(LINK_OUT_SWITCH1) $(LINK_OUT_SWITCH2)$@
	@echo ... generated $@

$(AMS_ROMTESTVM): $(AMS_OBJS) $(JVMX_LIB) $(JVMTEST_LIB) $(JVM_LIB)
	@echo linking $@ ...
	@$(LINK) $(AMS_OBJS) $(JVMX_LIB) $(JVMTEST_LIB) $(JVM_LIB) \
		$(EXTRA_LIBS) $(THREAD_LIBS) $(LINK_FLAGS) $(LINK_OUT_SWITCH1) $(LINK_OUT_SWITCH2)$@
	@echo ... generated $@

## IMPL_NOTE: linux specific!
$(TEST_DLL): $(DLL_OBJS)
	$(LINK) $(LINK_FLAGS) -shared $(DLL_OBJS) -o $@
	@echo ... generated $@

$(ANI_ROMTESTVM): $(ANI_OBJS) $(JVMX_LIB) $(JVMTEST_LIB) $(JVM_LIB)
	@echo linking $@ ...
	@$(LINK) $(ANI_OBJS) $(ANIX_LIB) $(ANI_LIB) $(JVM_LIB) \
	       	$(EXTRA_LIBS) $(THREAD_LIBS) $(LINK_FLAGS)     \
                $(LINK_OUT_SWITCH1) $(LINK_OUT_SWITCH2)$@
	@echo ... generated $@

ROMImage$(OBJ_SUFFIX): ../ROMImage.cpp
	@echo compiling $< ...
	@$(CPP) $(CPP_FLAGS) -c $< -o $@

NativesTable$(OBJ_SUFFIX): ../NativesTable.cpp
	@echo compiling $< ...
	@$(CPP) $(CPP_FLAGS) -c $< -o $@

InternalNatives$(OBJ_SUFFIX): $(TEST_SRC_DIR)/natives/InternalNatives.cpp
	@echo compiling $< ...
	@$(CPP) $(CPP_FLAGS) -c $< -o $@

DLLNatives$(OBJ_SUFFIX): $(TEST_SRC_DIR)/natives/DLLNatives.cpp
	@echo compiling $< ...
	@$(CPP) $(CPP_FLAGS) -c $< -o $@

KniNatives$(OBJ_SUFFIX): $(TEST_SRC_DIR)/natives/KniNatives.cpp
	@echo compiling $< ...
	@$(CPP) $(CPP_FLAGS) -c $< -o $@

SniNatives$(OBJ_SUFFIX): $(TEST_SRC_DIR)/natives/SniNatives.cpp
	@echo compiling $< ...
	@$(CPP) $(CPP_FLAGS) -c $< -o $@

IsolateTestNatives$(OBJ_SUFFIX): $(TEST_SRC_DIR)/natives/IsolateTestNatives.cpp
	@echo compiling $< ...
	@$(CPP) $(CPP_FLAGS) -c $< -o $@

AniNatives$(OBJ_SUFFIX): $(TEST_SRC_DIR)/natives/AniNatives.cpp
	@echo compiling $< ...
	@$(CPP) $(CPP_FLAGS) -c $< -o $@

AmsMain$(OBJ_SUFFIX): $(TEST_SRC_DIR)/natives/AmsMain.cpp
	@echo compiling $< ...
	@$(CPP) $(CPP_FLAGS) -c $< -o $@

AniMain$(OBJ_SUFFIX): $(TEST_SRC_DIR)/natives/AniMain.cpp
	@echo compiling $< ...
	@$(CPP) $(CPP_FLAGS) -c $< -o $@

AniNativesDummies$(OBJ_SUFFIX): $(TEST_SRC_DIR)/natives/AniNativesDummies.cpp
	@echo compiling $< ...
	@$(CPP) $(CPP_FLAGS) -c $< -o $@

#----------------------------------------------------------------------
#
# These built-in GNU-make rules for SCCS are troublesome, especially
# we don't have functioning SCCS on Windows. By declaring them here
# without a command, these built-in rules are cancelled.
#
#----------------------------------------------------------------------

%:: s.%

%:: SCCS/s.%

#----------------------------------------------------------------------
#
# Remove all these bogus default implicit rules that make building VERY
# slow over Samba.
#
#----------------------------------------------------------------------

%: %.o
%: %.c
%: %.cc
%: %.C
%: %.cpp
%: %.p
%: %.f
%: %.F
%: %.r
%: %.s
%: %.S
%: %.mod
%: %.sh
%:: %,v
%:: RCS/%,v
%:: RCS/%
