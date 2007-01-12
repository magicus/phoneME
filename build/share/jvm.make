#
#   
#
# Portions Copyright  2003-2006 Sun Microsystems, Inc. All Rights Reserved.
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
#!c<
# Copyright 2006 Intel Corporation. All rights reserved.
#!c>
#

IncludeDB            = $(WorkSpace)/src/vm/includeDB
BIN_DIR              = ../bin
DIST_DIR             = ../../dist
DIST_BIN_DIR         = $(DIST_DIR)/bin
DIST_LIB_DIR         = $(DIST_DIR)/lib
DIST_INC_DIR         = $(DIST_DIR)/include
CLDC_ZIP             = $(BuildSpace)/classes.zip
CLASSES              = $(BuildSpace)/classes
TMPCLASSES           = $(BuildSpace)/tmpclasses
JAVA		     = $(JDK_DIR)/bin/java
JAVAC                = $(JDK_DIR)/bin/javac -source 1.4 -target 1.4
JAVAC_DEBUG          = :none
JAR                  = $(JDK_DIR)/bin/jar
VMSRC_DIR            = $(WorkSpace)/src/vm
SHARE_DIR            = $(WorkSpace)/build/share
HOST_TOOLS_DIR       = $(WorkSpace)/build/share/bin/$(host_os)_$(host_arch)
NATIVES_TABLE        = $(GEN_DIR)/NativesTable.cpp
BUILDTOOLS_DIR       = $(BuildSpace)/$(BUILD_DIR_NAME)/tools
BUILDTOOL_JAR        = $(BUILDTOOLS_DIR)/buildtool.jar

#----------------------------------------------------------------------
#
# Build-time flags and their default values
#
# You can change these flags at build-time to come up with
# non-standard builds. E.g.
#
#	setenv PROFILING true
#	gnumake
# or
# 	gnumake PROFILING=true
#----------------------------------------------------------------------
ifneq ($(PROFILING), true)
PROFILING            := false
endif

ifeq ($(ENABLE_JVMPI_PROFILE), true)
MakeDepsOpts  += ENABLE_JVMPI_PROFILE=true
LINK_FLAGS += -rdynamic -ldl
else
MakeDepsOpts  += ENABLE_JVMPI_PROFILE=false
endif

ifeq ($(ENABLE_JVMPI_PROFILE_VERIFY), true)
MakeDepsOpts  += ENABLE_JVMPI_PROFILE_VERIFY=true
else
MakeDepsOpts  += ENABLE_JVMPI_PROFILE_VERIFY=false
endif

# Use ENABLE_SEGMENTED_ROM_TEXT_BLOCK by default
ifndef ENABLE_SEGMENTED_ROM_TEXT_BLOCK  
export ENABLE_SEGMENTED_ROM_TEXT_BLOCK     := true
export ENABLE_SEGMENTED_ROM_TEXT_BLOCK__BY := jvm.make
endif

ifeq ($(IsTarget)+$(ENABLE_MONET), true+true)
# do nothing
else
# ENABLE_MONET flag must be turned off in romgen and loopgen
override ENABLE_MONET     := false
override ENABLE_MONET__BY := jvm.make
export ENABLE_MONET
export ENABLE_MONET__BY
endif

ifndef SOURCE_MERGER_SIZE
SOURCE_MERGER_SIZE  = 35
endif

# used to be verbose on what make is really doing
# duplicated in root.make
ifndef VERBOSE_BUILD
A = @
else
A =
endif

# Product-specific definitions
-include $(SHARE_DIR)/product.make

ifndef PRODUCT_NAME
PRODUCT_NAME        = phoneME Feature VM
endif

ifndef RELEASE_VERSION
RELEASE_VERSION     = 1.1
endif

ifndef BUILD_VERSION
BUILD_VERSION       = internal
endif

#----------------------------------------------------------------------
#
# Platform configuration section:
#
# If you are adding any new definitions for os_family, arch, compiler,
# add a corresponding MACRO_xxx entry to the following.
#
#----------------------------------------------------------------------

ifdef ALT_PLATFORM
include $(SHARE_DIR)/$(ALT_PLATFORM)/alt_platform.make
endif

MakeDepsMain_win32    = WinGammaPlatform
MakeDepsMain_wince    = WinCEGammaPlatform
MakeDepsMain_linux    = UnixPlatform
ifneq ($(MakeDepsMain_$(os_family)_$(compiler)),)
MakeDepsMain          = $(MakeDepsMain_$(os_family)_$(compiler))
else
MakeDepsMain          = $(MakeDepsMain_$(os_family))
endif

ifeq ($(MERGE_SOURCE_FILES), true)
MakeDepsOpts         += -sourceMergerLimit $(SOURCE_MERGER_SIZE)
endif

ifeq ($(CHECK_INCLUDEDB), true)
MakeDepsOpts         += -checkIncludeDB
endif

MakeDepsOpts_win32    = -resolveVpath true
MakeDepsOpts_wince    = -resolveVpath true
MakeDepsOpts_linux    = -resolveVpath true
MakeDepsOpts         += -gendir $(GEN_DIR) -workspace $(WorkSpace)
MakeDepsOpts         += $(MakeDepsOpts_$(os_family))

HOST_EXE_SUFFIX_win32  = .exe
HOST_EXE_SUFFIX_cygwin = .exe
HOST_EXE_SUFFIX_linux  =
HOST_EXE_SUFFIX_solaris =
HOST_EXE_SUFFIX         = $(HOST_EXE_SUFFIX_$(host_os))

PATHSEP_win32        = \;
PATHSEP_linux        = :
PATHSEP_solaris      = :
PATHSEP              = ${PATHSEP_$(host_os)}

ROMGEN               = romgen$(HOST_EXE_SUFFIX)
ROM_GENERATOR        = ../../romgen/app/$(ROMGEN)

ifndef ROM_CONFIG_FILE
ROM_CONFIG_FILE = $(WorkSpace)/src/vm/cldctest_rom.cfg
endif

ifdef ROM_GEN_CLASSPATH_APPEND
ROM_GEN_CLASSPATH = $(CLDC_ZIP)$(PATHSEP)$(ROM_GEN_CLASSPATH_APPEND)
else
ROM_GEN_CLASSPATH = $(CLDC_ZIP)
endif

ROM_GEN_ARG         += $(ROM_GEN_FLAGS) =HeapCapacity16M
ROM_GEN_ARG         += -romconfig $(ROM_CONFIG_FILE)
ROM_GEN_ARG         += -romincludepath $(WorkSpace)/src/vm
ROM_GEN_ARG         += +RewriteROMConstantPool

ROM_GEN_ARG         += +EnableAllROMOptimizations
ifeq ($(ENABLE_JAVA_DEBUGGER), true)
ifeq ($(ENABLE_SYSTEM_CLASSES_DEBUG), true)
  ROM_GEN_ARG       += +MakeROMDebuggable
  JAVAC_DEBUG       =
else
  # For debugging application classes -CompactROMMethodTables   
  # is the only optimization which is incompatible with the Java debugger.
  # For debugging Monet bundle the +MakeROMDebuggable is reduced
  # to -CompactROMMethodTables (all the other optimizations are
  # disabled for binary image generator)
  ROM_GEN_ARG       += -CompactROMMethodTables
endif
endif 

# While building VM with C interpreter (arch = c)
# target_arch may be different, e.g. target_arch = mips
ifndef target_arch
  target_arch = $(host_arch)
endif

# very minimal CPU specific ASM stubs
ifeq ($(ENABLE_C_INTERPRETER), true)
Obj_Files           +=         AsmStubs_$(target_arch)$(OBJ_SUFFIX)
endif

LOOP_GENERATOR       = ../../loopgen/app/loopgen$(HOST_EXE_SUFFIX)

ifeq ($(MAKE_DETERMINISTIC), true)
DETERMINISTIC_FLAG   = +Deterministic
else
DETERMINISTIC_FLAG   =
endif

ifeq ($(TRACE_BYTECODES), true)
TRACE_BYTECODES_FLAG   = +TraceBytecodes
else
TRACE_BYTECODES_FLAG   =
endif

ifeq ($(ENABLE_INLINEASM_INTERPRETER), true)
LOOP_GEN_ARG        += +GenerateInlineAsm
endif

LOOP_GEN_ARG_debug   = $(DETERMINISTIC_FLAG) $(TRACE_BYTECODES_FLAG) \
                       -generate +GenerateDebugAssembly
LOOP_GEN_ARG_release = $(DETERMINISTIC_FLAG) -generateoptimized
LOOP_GEN_ARG_product = $(DETERMINISTIC_FLAG) -generateoptimized
LOOP_GEN_ARG        += $(LOOP_GEN_ARG_$(BUILD)) $(LOOP_GEN_FLAGS)

PREVERIFY_ORIGINAL   = $(HOST_TOOLS_DIR)/preverify$(HOST_EXE_SUFFIX)
PREVERIFY            = $(DIST_BIN_DIR)/preverify$(HOST_EXE_SUFFIX)

CHMOD                = chmod

ASM_visCPP           = ml.exe
ASM_evc_arm          = armasm.exe
ASM_evc_i386         = ml.exe
ASM_evc              = $(ASM_evc_$(arch))
ASM                  = $(ASM_$(compiler))

# Note: CPP_gcc is set below in the gcc section, but $(CPP) will still
# get the right value even when CPP is defined before CPP_gcc.

CPP_visCPP           = cl.exe
CPP_evc_arm          = clarm.exe
CPP_evc_i386         = cl.exe
CPP_evc              = $(CPP_evc_$(arch))
CPP                  = $(CPP_$(compiler))

CC_visCPP            = cl.exe
CC_evc_arm           = clarm.exe
CC_evc_i386          = cl.exe
CC_evc               = $(CC_evc_$(arch))
CC                   = $(CC_$(compiler))

SAVE_TEMPS_visCPP    = /Fa$(basename $(notdir $<))
SAVE_TEMPS_evc	     = /Fa$(basename $(notdir $<))
SAVE_TEMPS_gcc	     = -save-temps

ifeq ($(SAVE_TEMPS), true)
SAVE_TEMPS_CFLAGS    = $(SAVE_TEMPS_$(compiler))
endif

LINK_visCPP          = link.exe
LINK_evc             = link.exe
LINK                 = $(LINK_$(compiler))

LIBMGR_visCPP        = lib.exe
LIBMGR_evc           = lib.exe
LIBMGR               = $(LIBMGR_$(compiler))

OBJ_SUFFIX_visCPP    = .obj
OBJ_SUFFIX_evc       = .obj
OBJ_SUFFIX_gcc       = .o
ifeq ($(host_os), cygwin)
OBJ_SUFFIX_gcc       = .obj
endif
OBJ_SUFFIX           = $(OBJ_SUFFIX_$(compiler))

DLL_SUFFIX_visCPP    = .dll
DLL_SUFFIX_evc       = .dll
DLL_SUFFIX_gcc       = .so
ifeq ($(host_os), cygwin)
DLL_SUFFIX_gcc       = .dll
endif
DLL_SUFFIX           = $(DLL_SUFFIX_$(compiler))

LIB_PREFIX_visCPP    =
LIB_PREFIX_evc       =
LIB_PREFIX_gcc       = lib
LIB_PREFIX           = $(LIB_PREFIX_$(compiler))

LIB_SUFFIX_visCPP    = .lib
LIB_SUFFIX_evc       = .lib
LIB_SUFFIX_gcc       = .a
LIB_SUFFIX           = $(LIB_SUFFIX_$(compiler))

EXE_SUFFIX_visCPP    = .exe
EXE_SUFFIX_evc       = .exe
EXE_SUFFIX_gcc       =
ifeq ($(host_os), cygwin)
EXE_SUFFIX_gcc       = .exe
endif

EXE_SUFFIX           = $(EXE_SUFFIX_$(compiler))

BUILD_EXT_debug      = _g
BUILD_EXT_release    = _r
BUILD_EXT_product    =
BUILD_EXT            = $(BUILD_EXT_$(BUILD))

EXTRA_CLEAN_visCPP   = *.sbr *.pch *.pdb *.asm *.lst
EXTRA_CLEAN_evc      = *.sbr *.pch *.pdb *.asm *.lst
EXTRA_CLEAN_gcc      = Interpreter_$(arch).s
EXTRA_CLEAN          = $(EXTRA_CLEAN_$(compiler))

ifeq ($(ENABLE_INLINEASM_INTERPRETER), true)
ASM_SUFFIX_visCPP    = .c
else
ASM_SUFFIX_visCPP    = .asm
endif
ASM_SUFFIX_evc       = .asm
ASM_SUFFIX_gcc       = .s
ASM_SUFFIX           = $(ASM_SUFFIX_$(compiler))

EXTRA_JVMCONFIG_win32   = USE_UNICODE_FOR_FILENAMES=1
EXTRA_JVMCONFIG_linux   =
EXTRA_JVMCONFIG_solaris =
EXTRA_JVMCONFIG         = $(EXTRA_JVMCONFIG_$(os_family))

ifndef ROMIZING
ROMIZING             = true
endif

ROMIZING_CFLAGS_visCPP 	= /D "ROMIZING"
ROMIZING_CFLAGS_evc    	= /D "ROMIZING"
ROMIZING_CFLAGS_gcc    	= -DROMIZING=1

ifeq ($(ROMIZING)+$(IsTarget), true+true)
ROMIZING_CFLAGS        	= $(ROMIZING_CFLAGS_$(compiler))
endif

BUILD_VERSION_CFLAGS = -DJVM_RELEASE_VERSION='"$(RELEASE_VERSION)"' \
    -DJVM_BUILD_VERSION='"$(BUILD_VERSION)"' \
    -DJVM_NAME='"$(PRODUCT_NAME)"'

# in alphabetical order:

ifeq ($(ENABLE_JAZELLE),true)
export ENABLE_CPU_VARIANT     := true
export ENABLE_CPU_VARIANT__BY := jvm.make
export CPU_VARIANT            := jazelle
endif

ifeq ($(ENABLE_CPU_VARIANT), true)
ROM_GEN_ARG += -OptimizeBytecodes
endif

ifneq ($(ENABLE_INTERPRETER_GENERATOR), false)
MakeDepsOpts  += ENABLE_INTERPRETER_GENERATOR=true
endif

ifdef ROM_GEN_ARG_APPEND
ROM_GEN_ARG         += $(ROM_GEN_ARG_APPEND)
endif

ifndef HARDWARE_LITTLE_ENDIAN
HARDWARE_LITTLE_ENDIAN=true
endif
ifeq ($(HARDWARE_LITTLE_ENDIAN), false)
ENABLE_CFLAGS += -DHARDWARE_LITTLE_ENDIAN=0
endif
ifeq ($(HARDWARE_LITTLE_ENDIAN), true)
ENABLE_CFLAGS += -DHARDWARE_LITTLE_ENDIAN=1
endif

ifndef HOST_LITTLE_ENDIAN
HOST_LITTLE_ENDIAN=true
endif
ifeq ($(HOST_LITTLE_ENDIAN), false)
ENABLE_CFLAGS += -DHOST_LITTLE_ENDIAN=0
endif
ifeq ($(HOST_LITTLE_ENDIAN), true)
ENABLE_CFLAGS += -DHOST_LITTLE_ENDIAN=1
endif

# We don't give this an explicit value if we weren't passed one
ifeq ($(MSW_FIRST_FOR_DOUBLE), true)
ENABLE_CFLAGS += -DMSW_FIRST_FOR_DOUBLE=1
endif
ifeq ($(MSW_FIRST_FOR_DOUBLE), false)
ENABLE_CFLAGS += -DMSW_FIRST_FOR_DOUBLE=0
endif

ifeq ($(TARGET_MSW_FIRST_FOR_DOUBLE), true)
ENABLE_CFLAGS += -DTARGET_MSW_FIRST_FOR_DOUBLE=1
endif
ifeq ($(TARGET_MSW_FIRST_FOR_DOUBLE), false)
ENABLE_CFLAGS += -DTARGET_MSW_FIRST_FOR_DOUBLE=0
endif

ifeq ($(TARGET_MSW_FIRST_FOR_LONG), true)
ENABLE_CFLAGS += -DTARGET_MSW_FIRST_FOR_LONG=1
endif
ifeq ($(TARGET_MSW_FIRST_FOR_LONG), false)
ENABLE_CFLAGS += -DTARGET_MSW_FIRST_FOR_LONG=0
endif

ifeq ($(IsRomGen), true)
# Always need ENABLE_ROM_GENERATOR for romgen
override ENABLE_ROM_GENERATOR := true
override ENABLE_ROM_GENERATOR__BY := jvm.make
export ENABLE_ROM_GENERATOR
export ENABLE_ROM_GENERATOR__BY
endif

ifeq ($(IsLoopGen), true)
# Always need ENABLE_INTERPRETER_GENERATOR for interpreter loop generator
override ENABLE_INTERPRETER_GENERATOR := true
override ENABLE_INTERPRETER_GENERATOR__BY := jvm.make
export ENABLE_INTERPRETER_GENERATOR
export ENABLE_INTERPRETER_GENERATOR__BY
endif

ifeq ($(IsTarget)+$(ROMIZING)+$(ENABLE_SEGMENTED_ROM_TEXT_BLOCK), true+true+true)
  # by default separate ROMImage.cpp into smaller parts
  override SeparateROMImage := true
  
  # by default compile ROMImage.cpp as one file
  CompileROMImageSeparately := false

  # Then, for win32 and linux/arm builds, use individual ROMImage_??.cpp files.
  # For the other builds, use a single ROMImage.cpp file.
  ifeq ($(os_family), win32)
    CompileROMImageSeparately := true
  endif

  ifeq ($(arch), i386)
    ifeq ($(os_family), linux)
      CompileROMImageSeparately := true
    endif
  endif

  ifeq ($(arch), arm)
    ifeq ($(os_family), linux)
      CompileROMImageSeparately := true
    endif
  endif
else
  override SeparateROMImage := false
  override CompileROMImageSeparately := false
endif

#----------------------------------------------------------------------
# Files used by AOT-enabled romgen
#----------------------------------------------------------------------

ifeq ($(ENABLE_C_INTERPRETER), true)
  ifdef carch
  GP_TABLE_OBJ = Interpreter_$(carch)$(OBJ_SUFFIX)
  else
  GP_TABLE_OBJ = GPSkeleton$(OBJ_SUFFIX)
  endif
endif

#----------------------------------------------------------------------
# This rule lets you see what the ENABLE_XXX flags are like
# after they are changed by the settings in jvm.make and <platform>.cfg
#----------------------------------------------------------------------
print_env:
	env
	which $(CPP)
	which $(ASM)
	which $(LINK)

#--------------------------------------------------------
#
# Names of the generated files
#
# -------------------------------------------------------
BINARY_NAME        = cldc_vm
JVM_EXE_NAME       = $(BINARY_NAME)$(BUILD_EXT)$(EXE_SUFFIX)
JVM_MAP_NAME       = $(BINARY_NAME)$(BUILD_EXT).map
JVM_LIB_NAME       = $(LIB_PREFIX)$(BINARY_NAME)$(BUILD_EXT)$(LIB_SUFFIX)
JVMX_LIB_NAME      = $(LIB_PREFIX)$(BINARY_NAME)x$(BUILD_EXT)$(LIB_SUFFIX)
JVMTEST_LIB_NAME   = $(LIB_PREFIX)$(BINARY_NAME)test$(BUILD_EXT)$(LIB_SUFFIX)
ANI_LIB_NAME       = $(LIB_PREFIX)$(BINARY_NAME)_ani$(BUILD_EXT)$(LIB_SUFFIX)
ANIX_LIB_NAME      = $(LIB_PREFIX)$(BINARY_NAME)_anix$(BUILD_EXT)$(LIB_SUFFIX)
JVM_MAK_NAME       = $(BINARY_NAME)$(BUILD_EXT).make
JVM_LOG_NAME       = $(BINARY_NAME)$(BUILD_EXT).log
JVM_EXE            = $(BIN_DIR)/$(JVM_EXE_NAME)
JVM_MAP            = $(BIN_DIR)/$(JVM_MAP_NAME)
JVM_LIB            = $(BIN_DIR)/$(JVM_LIB_NAME)
JVMX_LIB           = $(BIN_DIR)/$(JVMX_LIB_NAME)
JVMTEST_LIB        = $(BIN_DIR)/$(JVMTEST_LIB_NAME)
ANI_LIB            = $(BIN_DIR)/$(ANI_LIB_NAME)
ANIX_LIB           = $(BIN_DIR)/$(ANIX_LIB_NAME)

#--------------------------------------------------------
#
# CLDC Java classes
#
# -------------------------------------------------------
$(CLDC_ZIP): $(JAVA_FILES)
	$(A)echo "=============================="
	$(A)num=`echo $? | wc -w`; \
         num=`echo $$num | cut -f1 -d' '`; \
	 num=`echo $$num`; \
         echo "compiling $(CLDC_LIB) classes ($$num files)..."
	$(A)if test ! -d $(TMPCLASSES); then \
            mkdir $(TMPCLASSES); \
        fi
	$(A)files="$?"; \
	 if test "$$files" = ""; then exit 1; fi; \
	 $(JAVAC) -g$(JAVAC_DEBUG) -d $(TMPCLASSES) \
                  -bootclasspath $(CLASSES) \
                  -classpath $(CLASSES) $$files
	$(A)echo "preverifying classes...";
	$(A)if test ! -f $(PREVERIFY); then \
		$(MAKE) $(PREVERIFY) ; \
	fi
	$(A)chmod a+rx $(PREVERIFY)
	$(A)$(PREVERIFY) -classpath $(CLASSES) -d $(CLASSES) $(TMPCLASSES)
	$(A)echo rm -rf $@ $(TMPCLASSES)
	$(A)$(JAR) -cfM0 $@ -C $(CLASSES) .
	$(A)echo "=============================="

#----------------------------------------------------------------------
#
# Files generated by MakeDeps
#
#----------------------------------------------------------------------
DEPEND_MAKEFILE  = $(GEN_DIR)/Dependencies
DEPEND_TIMESTAMP = $(GEN_DIR)/Dependencies.timestamp

dependencies: $(GEN_DIR)/platform $(GEN_DIR)/jvmconfig.h
dependencies: $(DEPEND_TIMESTAMP)

# platform:
# This file is used by MakeDeps/MakeDeps.class to generate the
# incls/*.incl files
$(GEN_DIR)/platform:
	$(A)if test ! -d $(GEN_DIR); then \
	    mkdir $(GEN_DIR); \
	fi
	$(A)echo '// auto-generated. Do not edit.'  > $@
	$(A)echo 'os_family    = $(os_family)'         >> $@
	$(A)echo 'arch         = $(arch)'              >> $@
	$(A)echo 'carch        = $(carch)'             >> $@
	$(A)echo 'iarch        = $(iarch)   '          >> $@
	$(A)echo 'os_arch      = $(os_family)_$(arch)' >> $@
	$(A)echo 'compiler     = $(compiler)'          >> $@
	$(A)echo 'cpu_variant  = $(CPU_VARIANT)'       >> $@

$(DEPEND_TIMESTAMP): $(BUILDTOOL_JAR) \
                     $(GEN_DIR)/platform \
		     $(IncludeDB)
	$(A)if test ! -d $(GEN_DIR)/incls; then \
	    mkdir $(GEN_DIR)/incls; \
	fi
	$(A)echo generating $(DEPEND_MAKEFILE) ...
	$(A)rm -f $(DEPEND_MAKEFILE)
	$(A)cd $(GEN_DIR); $(JAVA) \
		-jar $(BUILDTOOL_JAR) \
		makedep $(MakeDepsMain) platform $(IncludeDB) \
		$(MakeDepsOpts) $(Alt_MakeDepsOpts)
	$(A)echo "# last updated `date`" >> $(DEPEND_MAKEFILE)
	$(A)echo "# last updated `date`" >> $@
	@-if test $(IncludeDB) -nt $@; then \
	       (echo ""; \
		echo "Clock skew detected"; \
		echo ""; \
		echo "If you see $(DEPEND_MAKEFILE) being generated " \
                     "repeatedly, check the timestamp of $(IncludeDB)"; \
		echo ""); fi

BUILD_FLAGS_HPP = $(WorkSpace)/src/vm/share/utilities/BuildFlags.hpp

$(GEN_DIR)/jvmconfig.h:	$(BUILDTOOL_JAR) $(BUILD_FLAGS_HPP) $(GEN_DIR)/platform
	$(A)if test ! -d $(GEN_DIR); then \
	    mkdir $(GEN_DIR); \
	fi
	$(A)$(JAVA) -jar $(BUILDTOOL_JAR) config $(GEN_DIR)/platform \
                $(BUILD_FLAGS_HPP) $@ $(EXTRA_JVMCONFIG)

$(GEN_DIR):
	$(A)mkdir -p $@

ifneq ($(IsRomGen), true)
$(ROM_GENERATOR):
	$(MAKE) -s -C ../../romgen/app
endif

ifneq ($(IsLoopGen), true)
$(LOOP_GENERATOR):
	$(MAKE) -s -C ../../loopgen/app
endif

$(NATIVES_TABLE): $(CLDC_ZIP) $(BUILDTOOLS_DIR)/jcc.jar
	$(A)echo generating $@ ...
	$(A)$(JAVA) -jar $(BUILDTOOLS_DIR)/jcc.jar \
		-writer CLDC_HI_Natives -o $@ $(CLDC_ZIP)

#----------------------------------------------------------------------
#
# Files generated by $(LOOP_GENERATOR) $(ROM_GENERATOR)
#
#----------------------------------------------------------------------

ROM_binary.bun: $(GENERATED_ROM_FILE)

GENERATED_ROM_FILE = $(GEN_DIR)/ROMImage.cpp

ifeq ($(IsTarget)+$(ROMIZING), true+true)

$(GENERATED_ROM_FILE): $(ROM_GENERATOR) $(CLDC_ZIP) \
                       $(WorkSpace)/src/vm/cldc_rom.cfg \
                       $(WorkSpace)/src/vm/cldcx_rom.cfg \
                       $(WorkSpace)/src/vm/cldctest_rom.cfg
	$(ROM_GENERATOR) -cp $(ROM_GEN_CLASSPATH) $(ROM_GEN_ARG) -romize
	$(A)mv ROMImage*.*pp `dirname $@`;
	$(A)mv ROMLog.txt $(GEN_DIR)

else

$(GENERATED_ROM_FILE): $(WorkSpace)/src/vm/share/ROM/ROMSkeleton.cpp
	$(A)echo copying empty ROMImage.cpp
	$(A)rm -f $@
	$(A)cp $< $@
	$(A)$(CHMOD) u+w $@

endif

test-romize:: $(ROM_GENERATOR)
	$(ROM_GENERATOR) -cp $(CLDC_ZIP) $(ROM_GEN_ARG) \
	    -romize +GenerateROMComments +VerboseROMComments \
	    +GenerateCompilerComments =HeapCapacity32M =HeapMin32M \
	    +GenerateROMStructs
	mv ROMImage.cpp ROMImage.test.cpp

test-romize-2:: $(ROM_GENERATOR)
	$(ROM_GENERATOR) -cp $(CLDC_ZIP) $(ROM_GEN_ARG) \
	    -romize +GenerateROMComments +VerboseROMComments \
	    +GenerateCompilerComments =HeapCapacity32M =HeapMin32M \
	    +GenerateROMStructs -VerbosePointers
	mv ROMImage.cpp ROMImage.test.cpp

test-romize-3:: $(ROM_GENERATOR)
	$(ROM_GENERATOR) -cp $(CLDC_ZIP) $(ROM_GEN_ARG) \
	    -romize +GenerateROMComments \
	    =HeapCapacity32M =HeapMin32M \
	    +GenerateROMStructs -VerbosePointers
	mv ROMImage.cpp ROMImage.test.cpp

# Test new romization features
test-romize-new:: $(ROM_GENERATOR)
	$(ROM_GENERATOR) -cp $(CLDC_ZIP) $(ROM_GEN_ARG) $(NEW_ROM_GEN_ARG) \
	    -romize +GenerateROMComments +VerboseROMComments \
	    +GenerateROMStructs
	mv ROMImage.cpp ROMImage.test.cpp

test-romize-quick:: $(ROM_GENERATOR)
	$(ROM_GENERATOR) -cp $(CLDC_ZIP) $(ROM_GEN_ARG) \
	    -romize
	mv ROMImage.cpp ROMImage.test.cpp

ifneq ($(IsRomGen), true)
Interpreter_$(arch)$(ASM_SUFFIX): $(LOOP_GENERATOR)
	$(LOOP_GENERATOR) $(LOOP_GEN_ARG)

OopMaps.obj: OopMaps.cpp

OopMaps.cpp: $(ROM_GENERATOR)
	$(ROM_GENERATOR) +GenerateOopMaps
else
# Who generates the Interpreter_xxx.asm and OopMaps.cpp file for the ROMgen?
# A ROMgen is really a self-hosted VM build. It knows how to generate the
# interpreter loop for itself. See the rule for "$(ROM_GENERATOR):"
# of your favorite host compiler.
endif

#----------------------------------------------------------------------
#
# Generic compiler settings. Override the macros defines in this section
# if your compiler is somewhat 'different'
#
#----------------------------------------------------------------------

# This syntax of include flags should be acceptable to most C/C++ compilers.
# You need to redefine it only if you have a strange compiler

CPP_INCLUDE_DIRS = \
  -I"$(GEN_DIR)"                            \
  -I"$(WorkSpace)/src/vm/share/compiler"    \
  -I"$(WorkSpace)/src/vm/share/debugger"    \
  -I"$(WorkSpace)/src/vm/share/handles"     \
  -I"$(WorkSpace)/src/vm/share/memory"      \
  -I"$(WorkSpace)/src/vm/share/interpreter" \
  -I"$(WorkSpace)/src/vm/share/isolate"     \
  -I"$(WorkSpace)/src/vm/share/natives"     \
  -I"$(WorkSpace)/src/vm/share/reflection"  \
  -I"$(WorkSpace)/src/vm/share/runtime"     \
  -I"$(WorkSpace)/src/vm/share/utilities"   \
  -I"$(WorkSpace)/src/vm/share/ROM"         \
  -I"$(WorkSpace)/src/vm/share/verifier"    \
  -I"$(WorkSpace)/src/vm/share/float"       \
  -I"$(WorkSpace)/src/vm/os/utilities"      \
  -I"$(WorkSpace)/src/vm/share/memoryprofiler" \
  -I"$(WorkSpace)/src/vm/os/$(os_family)"   \
  -I"$(WorkSpace)/src/midp"                 \
  -I"$(WorkSpace)/src/vm/cpu/arm"           \
  -I"$(WorkSpace)/src/vm/cpu/c"             \
  -I"$(WorkSpace)/src/vm/cpu/i386"          \
  -I"$(WorkSpace)/src/vm/cpu/sh"            \
  -I"$(WorkSpace)/src/vm/cpu/thumb"         \
  -I"$(WorkSpace)/src/vm/cpu/thumb2"        \
  -I"$(WorkSpace)/src/anilib/share"         \
  -I"$(WorkSpace)/src/anilib/$(os_family)"  \
  -I"$(WorkSpace)/src/tools/ads_extender"

# Don't use PCSL for the generators. This way you don't need to build
# two versions of PCSL (host and target) just to build the VM.
ifeq ($(ENABLE_PCSL)+$(IsGenerator), true+true)
override ENABLE_PCSL := false
export ENABLE_PCSL
else
PCSL_DIST_DIR = $(PCSL_OUTPUT_DIR)/$(os_family)_$(arch)
endif

ifeq ($(ENABLE_PCSL), true)
CPP_INCLUDE_DIRS += -I"$(PCSL_DIST_DIR)/inc"
endif

#
# Note: If you change the vpaths, you also need to edit
# $(WorkSpace)/src/tools/buildtool/makedeps/Database.java
# to keep its vpath list in sync.
#
VPATH_PATTERNS = %.cpp %.hpp %.incl %.rc %.h
vpath
vpath $(VPATH_PATTERNS) $(GEN_DIR)
vpath $(VPATH_PATTERNS) $(GEN_DIR)/incls
vpath $(VPATH_PATTERNS) $(WorkSpace)/src/vm/cpu/arm
vpath $(VPATH_PATTERNS) $(WorkSpace)/src/vm/cpu/c
vpath $(VPATH_PATTERNS) $(WorkSpace)/src/vm/cpu/i386
vpath $(VPATH_PATTERNS) $(WorkSpace)/src/vm/cpu/sh
vpath $(VPATH_PATTERNS) $(WorkSpace)/src/vm/cpu/thumb
vpath $(VPATH_PATTERNS) $(WorkSpace)/src/vm/os/$(os_family)
vpath $(VPATH_PATTERNS) $(WorkSpace)/src/vm/os/utilities
vpath $(VPATH_PATTERNS) $(WorkSpace)/src/vm/share/compiler
vpath $(VPATH_PATTERNS) $(WorkSpace)/src/vm/share/debugger
vpath $(VPATH_PATTERNS) $(WorkSpace)/src/vm/share/handles
vpath $(VPATH_PATTERNS) $(WorkSpace)/src/vm/share/interpreter
vpath $(VPATH_PATTERNS) $(WorkSpace)/src/vm/share/isolate
vpath $(VPATH_PATTERNS) $(WorkSpace)/src/vm/share/memory
vpath $(VPATH_PATTERNS) $(WorkSpace)/src/vm/share/natives
vpath $(VPATH_PATTERNS) $(WorkSpace)/src/vm/share/ROM
vpath $(VPATH_PATTERNS) $(WorkSpace)/src/vm/share/reflection
vpath $(VPATH_PATTERNS) $(WorkSpace)/src/vm/share/runtime
vpath $(VPATH_PATTERNS) $(WorkSpace)/src/vm/share/verifier
vpath $(VPATH_PATTERNS) $(WorkSpace)/src/vm/share/utilities
vpath $(VPATH_PATTERNS) $(WorkSpace)/src/vm/share/memoryprofiler
vpath $(VPATH_PATTERNS) $(WorkSpace)/src/vm/share/float
vpath $(VPATH_PATTERNS) $(WorkSpace)/src/anilib/share
vpath $(VPATH_PATTERNS) $(WorkSpace)/src/anilib/$(os_family)

ifdef CPU_VARIANT
CPP_INCLUDE_DIRS += -I"$(WorkSpace)/src/vm/cpu/$(arch)/$(CPU_VARIANT)"
CPP_INCLUDE_DIRS += -I"$(WorkSpace)/src/vm/cpu/$(carch)/$(CPU_VARIANT)"
vpath $(VPATH_PATTERNS) $(WorkSpace)/src/vm/cpu/$(arch)/$(CPU_VARIANT)
vpath $(VPATH_PATTERNS) $(WorkSpace)/src/vm/cpu/$(carch)/$(CPU_VARIANT)
endif

#----------------------------------------------------------------------
#
# Compiler Section: visCPP (Visual C++ 6.0 for Win32)
#
# [NOTE] Use this section as an example if you're defining a new HOST
#        compiler.
#----------------------------------------------------------------------

ifeq ($(compiler), visCPP)

ifneq ($(findstring CYGWIN, $(shell uname)), CYGWIN)
    define fixcygpath
    echo $(1)
    endef
else
    define fixcygpath
    echo $(1) | xargs -n1 cygpath -w
    endef
endif

# /Ox (full optimization)
# /GB (optimize for processor) blend
# /Os (Favor small code)
# /Gy (Enable function-level linking)
# /GF (Enable read -only string pooling)

ifeq ($(NO_DEBUG_SYMBOLS), true)
CPP_DBG_FLAGS_debug     = /MD /Ox
else
CPP_DBG_FLAGS_debug     = /MDd /Zi /Od
endif

CPP_DBG_FLAGS_release   = /MD /Zi
ifeq ($(PRODUCT_DEBUG), true)
CPP_DBG_FLAGS_product   = /MD /Zi
else
CPP_DBG_FLAGS_product   = /MD
endif
CPP_DBG_FLAGS          += $(CPP_DBG_FLAGS_$(BUILD))

CPP_OPT_FLAGS_debug     =
CPP_OPT_FLAGS_release   = /Ox /GB /Os /Gy /GF
CPP_OPT_FLAGS_product   = /Ox /GB /Os /Gy /GF
CPP_OPT_FLAGS          += $(CPP_OPT_FLAGS_$(BUILD))

CPP_DEF_FLAGS_debug     = -D_DEBUG -DAZZERT
CPP_DEF_FLAGS_release   = 
CPP_DEF_FLAGS_product   = -DPRODUCT

ifeq ($(USE_VS2005), true)
CPP_DEF_FLAGS          += -D_CRT_SECURE_NO_DEPRECATE
endif

CPP_DEF_FLAGS          += -DWIN32 -D_WINDOWS
CPP_DEF_FLAGS          += $(CPP_DEF_FLAGS_$(BUILD))
CPP_DEF_FLAGS          += /W3 /nologo  \
                          $(SAVE_TEMPS_CFLAGS) \
                          $(ENABLE_CFLAGS) \
                          $(ROMIZING_CFLAGS) \
                          $(BUILD_VERSION_CFLAGS) \
                          $(CPP_DBG_FLAGS)
CPP_FLAGS_EXPORT        = $(CPP_DEF_FLAGS) $(CPP_OPT_FLAGS) \
                          -DREQUIRES_JVMCONFIG_H=1
CPP_FLAGS               = $(CPP_DEF_FLAGS) $(CPP_INCLUDE_DIRS)

ifneq ($(NO_DEBUG_SYMBOLS), true)
LINK_FLAGS_DBG_debug    = /debug
LINK_FLAGS_DBG_release  = /debug
endif

ifeq ($(PRODUCT_DEBUG), true)
LINK_FLAGS_DBG_product  = /debug
else
LINK_FLAGS_DBG_product  =
endif
LINK_FLAGS_DBG          = $(LINK_FLAGS_DBG_$(BUILD))
LINK_OPT_FLAGS_debug    =
LINK_OPT_FLAGS_release  =
LINK_OPT_FLAGS_product  = /merge:CODESEGMENT=.text \
                          /merge:DATASEGMENT=.data \
			  /opt:REF /opt:ICF,8
LINK_OPT_FLAGS          = $(LINK_FLAGS_DBG) $(LINK_OPT_FLAGS_$(BUILD))

LINK_PROF_FLAGS_true    = /PROFILE /FIXED:NO
LINK_PROF_FLAGS         = $(LINK_PROF_FLAGS_$(PROFILING))
LINK_OPT_FLAGS         += $(LINK_PROF_FLAGS)

LINK_FLAGS_EXPORT       = /MAP /incremental:no \
		          /nologo /subsystem:console /machine:i386
LINK_FLAGS_EXPORT      += $(LINK_OPT_FLAGS)
LINK_FLAGS             += $(LINK_FLAGS_EXPORT) wsock32.lib

ifeq ($(ENABLE_PCSL), true)
PCSL_LIBS               = `$(call fixcygpath, $(PCSL_DIST_DIR)/lib/libpcsl_memory.lib)`   \
                          `$(call fixcygpath, $(PCSL_DIST_DIR)/lib/libpcsl_print.lib)`    \
                          `$(call fixcygpath, $(PCSL_DIST_DIR)/lib/libpcsl_network.lib)`  \
                          `$(call fixcygpath, $(PCSL_DIST_DIR)/lib/libpcsl_string.lib)`   \
                          `$(call fixcygpath, $(PCSL_DIST_DIR)/lib/libpcsl_file.lib)`
MAKE_EXPORT_EXTRA_LIBS += $(PCSL_LIBS)
endif


LIB_FLAGS_debug         = /DEBUGTYPE:CV
LIB_FLAGS_release       =
LIB_FLAGS_product       =
LIB_FLAGS               = /nologo $(LIB_FLAGS_$(BUILD))

CPP_USE_PCH             = /Fp"cldchi.pch" /Yu"incls/_precompiled.incl"
ASM_FLAGS               = /nologo /c /coff /Zi /FR
BUILD_PCH               = _build_pch_visCPP.obj

$(Obj_Files):            $(BUILD_PCH)
InterpreterSkeleton.obj: $(BUILD_PCH)

$(BUILD_PCH): $(Precompiled_Headers) $(GEN_DIR)/jvmconfig.h
	$(A)echo Generating $@ ...
	$(A)echo '#include "incls/_precompiled.incl"' > \
		$(GEN_DIR)/_build_pch_visCPP.cpp
	$(A)$(CPP) $(CPP_OPT_FLAGS) $(CPP_FLAGS) /Fp"cldchi.pch" \
	    /Yc"incls/_precompiled.incl" /c $(GEN_DIR)/_build_pch_visCPP.cpp
	$(A)echo '    done'

ifeq ($(ENABLE_C_INTERPRETER), true)

AsmStubs_$(target_arch).obj: \
		$(JVMWorkSpace)/src/vm/cpu/c/AsmStubs_$(target_arch).asm
	$(A) echo "generating CPU stubs $<"
	$(A)$(ASM) /Zi $(ASM_FLAGS) /Fo$@ \
		/Sc /FlAsmStubs_$(target_arch).lst $<

else
ifeq ($(ENABLE_INLINEASM_INTERPRETER), true)
Interpreter_$(arch).obj: Interpreter_$(arch).c
	$(A)$(CPP) $(CPP_OPT_FLAGS) $(CPP_FLAGS) /FaInterpreter_$(arch).asm \
		-c `$(call fixcygpath, $<)`
else
Interpreter_$(arch).obj: Interpreter_$(arch).asm
	$(A)$(ASM) /Zi $(ASM_FLAGS) /FoInterpreter_$(arch).obj \
		/Sc /FlInterpreter_$(arch).lst \
		`$(call fixcygpath, $(THIS_DIR)/$<)`
endif
endif

# Can't use precompiled headers
ifneq ($(CompileROMImageSeparately), true)
ROMImage.obj: $(GENERATED_ROM_FILE)
	$(A)$(CPP) $(CPP_OPT_FLAGS) $(CPP_FLAGS) -c `$(call fixcygpath, $<)`
else
ROMImage_%.obj: ROMImage_%.cpp
	$(A)$(CPP) $(CPP_OPT_FLAGS) $(CPP_FLAGS) -c `$(call fixcygpath, $<)`
endif

# Can't use precompiled headers

NativesTable.obj: $(NATIVES_TABLE)
	$(A)$(CPP) $(CPP_OPT_FLAGS) $(CPP_FLAGS) -c `$(call fixcygpath, $<)`

ifeq ($(SAVE_PREPROCESSOR_OUTPUT),true)
define BUILD_C_TARGET
	$(A)$(CPP) $(CPP_OPT_FLAGS) $(CPP_FLAGS) $(CPP_USE_PCH) /P \
		`$(call fixcygpath, $<)`
	$(A)$(CPP) $(CPP_OPT_FLAGS) $(CPP_FLAGS) $(CPP_USE_PCH) -c \
		`$(call fixcygpath, $<)`
endef
else
define BUILD_C_TARGET
	$(A)$(CPP) $(CPP_OPT_FLAGS) $(CPP_FLAGS) $(CPP_USE_PCH) -c \
		`$(call fixcygpath, $<)`
endef
endif

ifeq ($(IsLoopGen), true)
ifeq ($(ENABLE_C_INTERPRETER), true)
$(LOOP_GENERATOR):
	$(A)touch $@
	$(A)echo "created dummy loop generator"
else
$(LOOP_GENERATOR): $(BUILD_PCH) $(Obj_Files) \
		   InterpreterSkeleton.obj OopMapsSkeleton.obj
	$(A)$(LINK) $(PCSL_LIBS) $(LINK_FLAGS) /out:$@ $(Obj_Files) \
		   InterpreterSkeleton.obj OopMapsSkeleton.obj
	$(A)echo generated `pwd`/$@
endif
endif

ifeq ($(IsRomGen), true)

ifeq ($(ENABLE_C_INTERPRETER), true)

ifdef carch
$(GP_TABLE_OBJ): $(LOOP_GENERATOR)
	$(A)$(LOOP_GENERATOR) -generate +GenerateGPTableOnly
	if test "$(carch)" = "thumb"; then \
	    mv $(THIS_DIR)/Interpreter_arm.asm \
               $(THIS_DIR)/Interpreter_$(carch).asm; \
	fi
	$(A)$(ASM) /Zi $(ASM_FLAGS) /Fo$(GP_TABLE_OBJ) \
		/Sc /FlInterpreter_$(carch).lst \
		`$(call fixcygpath, $(THIS_DIR)/Interpreter_$(carch).asm)`
endif

$(ROM_GENERATOR): $(CLDC_ZIP) $(Obj_Files) OopMapsSkeleton.obj $(GP_TABLE_OBJ)
	$(A)echo "linking ROM generator: $(ROM_GENERATOR)"
	$(A)$(LINK) $(PCSL_LIBS) $(LINK_FLAGS) /out:$@ \
		$(Obj_Files) OopMapsSkeleton.obj $(GP_TABLE_OBJ)
	$(A)$(ROM_GENERATOR) +GenerateOopMaps
	$(A)$(MAKE) OopMaps.obj
	$(A)echo "re-linking ROM generator: $(ROM_GENERATOR)"
	$(A)$(LINK) $(PCSL_LIBS) $(LINK_FLAGS) /out:$@ $(Obj_Files) \
                OopMaps.obj \
                $(GP_TABLE_OBJ)
	$(A)echo generated `pwd`/$@
else
$(ROM_GENERATOR): $(BUILD_PCH) $(Obj_Files) InterpreterSkeleton.obj \
                                            OopMapsSkeleton.obj
	$(A)$(LINK) $(PCSL_LIBS) $(LINK_FLAGS) /out:$@ $(Obj_Files) \
		InterpreterSkeleton.obj OopMapsSkeleton.obj
	$(A)$(ROM_GENERATOR) $(LOOP_GEN_ARG)
	$(A)$(ROM_GENERATOR) +GenerateOopMaps
	$(A)$(MAKE) Interpreter_$(arch).obj
	$(A)$(MAKE) OopMaps.obj
	$(A)$(LINK) $(PCSL_LIBS) $(LINK_FLAGS) /out:$@ \
		$(Obj_Files) Interpreter_$(arch).obj OopMaps.obj
	$(A)echo generated `pwd`/$@

endif
endif

MAKE_EXPORT_LINK_OUT_SWITCH1     =
MAKE_EXPORT_LINK_OUT_SWITCH2     = /out:
MAKE_EXPORT_EXTRA_LIBS          += wsock32.lib
MAKE_EXPORT_THREAD_LIBS          =

$(DIST_LIB_DIR)/$(JVM_LOG_NAME)::
	$(A)rm -f $@
	$(A)echo TBD > $@

# Split the object files into:
#     - LIB_OBJS:  the ones that are exported publicly in $(JVM_LIB)
#     - LIBX_OBJS: the ones that are exported privately in $(JVMX_LIB)
#     - LIBTEST_OBJS: the ones that are exported privately in $(JVMTEST_LIB)
#     - EXE_OBJS:  the ones that are used only by $(JVM_EXE)
LIB_OBJS := $(Obj_Files) OopMaps.obj
LIB_OBJS := $(subst BSDSocket.obj,,$(LIB_OBJS))
LIBX_OBJS +=        BSDSocket.obj
LIB_OBJS := $(subst ReflectNatives.obj,,$(LIB_OBJS))
LIBTEST_OBJS +=     ReflectNatives.obj
LIB_OBJS := $(subst jvmspi.obj,,$(LIB_OBJS))
EXE_OBJS +=         jvmspi.obj
LIB_OBJS := $(subst Main_$(os_family).obj,,$(LIB_OBJS))
EXE_OBJS +=         Main_$(os_family).obj
LIB_OBJS := $(subst NativesTable.obj,,$(LIB_OBJS))
EXE_OBJS +=         NativesTable.obj
LIB_OBJS := $(subst ROMImage.obj,,$(LIB_OBJS))

ifneq ($(ENABLE_C_INTERPRETER), true)
LIB_OBJS += Interpreter_$(arch).obj
endif

ifeq ($(ENABLE_USE_BINARY_ROM_IMAGE), true)
LIB_OBJS := $(subst ROM.obj,,$(LIB_OBJS))
EXE_OBJS +=         ROMImage.obj ROM.obj
else
EXE_OBJS +=         ROMImage.obj
endif

ifeq ($(SeparateROMImage), true)
ifeq ($(CompileROMImageSeparately), true)
ROM_SEGMENTS_OBJS = ROMImage_00.obj \
                    ROMImage_01.obj \
                    ROMImage_02.obj \
                    ROMImage_03.obj \
                    ROMImage_04.obj \
                    ROMImage_05.obj \
                    ROMImage_06.obj \
                    ROMImage_07.obj \
                    ROMImage_08.obj \
                    ROMImage_09.obj \
                    ROMImage_10.obj \
                    ROMImage_11.obj \
                    ROMImage_12.obj

EXE_OBJS := $(subst ROMImage.obj,,$(EXE_OBJS))
EXE_OBJS += $(ROM_SEGMENTS_OBJS)
$(ROM_SEGMENTS_OBJS): $(GENERATED_ROM_FILE)

endif

ROM_SEGMENTS = $(GEN_DIR)/ROMImage_00.cpp \
               $(GEN_DIR)/ROMImage_01.cpp \
               $(GEN_DIR)/ROMImage_02.cpp \
               $(GEN_DIR)/ROMImage_03.cpp \
               $(GEN_DIR)/ROMImage_04.cpp \
               $(GEN_DIR)/ROMImage_05.cpp \
               $(GEN_DIR)/ROMImage_06.cpp \
               $(GEN_DIR)/ROMImage_07.cpp \
               $(GEN_DIR)/ROMImage_08.cpp \
               $(GEN_DIR)/ROMImage_09.cpp \
               $(GEN_DIR)/ROMImage_10.cpp \
               $(GEN_DIR)/ROMImage_11.cpp \
               $(GEN_DIR)/ROMImage_12.cpp \
               $(GEN_DIR)/ROMImageGenerated.hpp

# Use $(GENERATED_ROM_FILE) as a marker to regenerate $(ROM_SEGMENTS).
# Add $(ROM_SEGMENTS) into the set of prerequisites of $(GENERATED_ROM_FILE),
# without overriding the command for $(GENERATED_ROM_FILE) target.
$(GENERATED_ROM_FILE): $(ROM_SEGMENTS)
$(ROM_SEGMENTS):

endif


$(JVM_LIB): $(BIN_DIR) $(BUILD_PCH) $(LIB_OBJS)
	$(A)$(LIBMGR) $(LIB_FLAGS) /out:$@ $(LIB_OBJS)
	$(A)echo generated `pwd`/$@

$(JVMX_LIB): $(BIN_DIR) $(BUILD_PCH) $(LIBX_OBJS)
	$(A)$(LIBMGR) $(LIB_FLAGS) /out:$@ $(LIBX_OBJS)
	$(A)echo generated `pwd`/$@

$(JVMTEST_LIB): $(BIN_DIR) $(BUILD_PCH) $(LIBTEST_OBJS)
	$(A)$(LIBMGR) $(LIB_FLAGS) /out:$@ $(LIBTEST_OBJS)
	$(A)echo generated `pwd`/$@

$(JVM_EXE): $(BIN_DIR) $(BUILD_PCH) $(JVMX_LIB) $(JVM_LIB) $(JVMTEST_LIB) \
	    $(EXE_OBJS)
	$(A)$(LINK) $(LINK_FLAGS) /out:$@ $(EXE_OBJS) $(JVMX_LIB) $(JVM_LIB) \
		$(JVMTEST_LIB) $(PCSL_LIBS)
	$(A)echo generated `pwd`/$@

ANI_OBJS  = ani.obj os_port.obj poolthread.obj
ANIX_OBJS = ani_bsd_socket.obj

$(ANI_LIB): $(JVM_LIB) $(ANI_OBJS)
	$(A)$(LIBMGR) $(LIB_FLAGS) /out:$@ $(ANI_OBJS)
	$(A)echo generated `pwd`/$@

$(ANIX_LIB): $(JVM_LIB) $(ANIX_OBJS)
	$(A)$(LIBMGR) $(LIB_FLAGS) /out:$@ $(ANIX_OBJS)
	$(A)echo generated `pwd`/$@

endif

#----------------------------------------------------------------------
#
# Compiler Section: evc (Embedded Visual C++ 3.0 WinCE)
#
# [NOTE] Use this section as an example if you're defining a new TARGET
#        compiler.
#----------------------------------------------------------------------

ifeq ($(compiler), evc)

ifneq ($(findstring CYGWIN, $(shell uname)), CYGWIN)
    define fixcygpath
    echo $(1)
    endef
else
    define fixcygpath
    cygpath -w $(1)
    endef
endif

ifeq ($(USE_VS2005), true)
    ifeq ($(arch), arm)
	EVC_LIB_PATH    = $(VS2005_CE_ARM_LIB)
	EVC_INCLUDE_PATH= $(VS2005_CE_ARM_INCLUDE)
	EVC_ARCH_PATH   = $(VS2005_CE_ARM_PATH)
        ARCH_ASM        = armasm.exe
    else
        # i386 build of WinCE is no longer supported for Visual Studio 2005.
        # this is just a place holder.
	EVC_LIB_PATH    = $(VS2005_CE_I386_LIB)
	EVC_INCLUDE_PATH= $(VS2005_CE_I386_INCLUDE)
	EVC_ARCH_PATH   = $(VS2005_CE_I386_PATH)
        ARCH_ASM        = ml.exe
    endif

    CPP                := $(EVC_ARCH_PATH)/cl.exe
    CC                 := $(EVC_ARCH_PATH)/cl.exe
    ASM                := $(EVC_ARCH_PATH)/$(ARCH_ASM)
    LINK               := $(EVC_ARCH_PATH)/link.exe
    LIB                := $(EVC_ARCH_PATH)/lib.exe
    RC                 := $(VS2005_COMMON_PATH)/rc.exe

    CESubsystem         = windowsce,5.01
    CEVersion           = 0x501

else
    ifeq ($(arch), arm)
	EVC_LIB_PATH    = $(EVC_ARM_LIB)
	EVC_INCLUDE_PATH= $(EVC_ARM_INCLUDE)
	EVC_ARCH_PATH   = $(EVC_ARM_PATH)
    else
	EVC_LIB_PATH    = $(EVC_I386_LIB)
	EVC_INCLUDE_PATH= $(EVC_I386_INCLUDE)
	EVC_ARCH_PATH   = $(EVC_I386_PATH)
    endif

    CPP                := $(EVC_ARCH_PATH)/$(CPP)
    CC                 := $(EVC_ARCH_PATH)/$(CC)
    LINK               := $(EVC_ARCH_PATH)/$(LINK)
    ASM                := $(EVC_ARCH_PATH)/$(ASM)
    LIB                := $(EVC_ARCH_PATH)/$(LIB)
    RC                 := $(EVC_COMMON_PATH)/rc.exe

    LINK_ARCH_FLAGS_arm+= /MACHINE:ARM /base:"0x00010000" /stack:0x10000,0x1000

    CESubsystem         = windowsce,4.00
    CEVersion           = 400

endif

LIBS                    = commctrl.lib coredll.lib  \
                          winsock.lib

CPP_OPT_FLAGS_debug     = /Zi /Od
CPP_OPT_FLAGS_release   = /Oxs
CPP_OPT_FLAGS_product   = /Oxs
CPP_OPT_FLAGS           =
CPP_OPT_FLAGS          += $(CPP_OPT_FLAGS_$(BUILD))

CPP_ARCH_FLAGS_arm      = -DARM -D_ARM -D_ARM_
CPP_ARCH_FLAGS_i386     = -D_X86_ -D_X86 -Dx86
CPP_ARCH_FLAGS          = $(CPP_ARCH_FLAGS_$(arch))

CPP_DEF_FLAGS_debug     = /D "_DEBUG" /D "AZZERT" /D "DEBUG"
CPP_DEF_FLAGS_release   = -DNDEBUG
CPP_DEF_FLAGS_product   = -DPRODUCT -DNDEBUG

CPP_DEF_FLAGS          += $(CPP_DEF_FLAGS_$(BUILD)) /X /I"${EVC_INCLUDE_PATH}"

CPP_FLAGS_EXPORT        = $(CPP_DEF_FLAGS) -DREQUIRES_JVMCONFIG_H=1
CPP_FLAGS_EXPORT       += /W3 -D_WIN32_WCE=$(CEVersion) $(CPP_ARCH_FLAGS) \
                          -DUNDER_CE=$(CEVersion) \
                          -DUNICODE -D_UNICODE /nologo $(SAVE_TEMPS_CFLAGS) \
                          $(ENABLE_CFLAGS) $(ROMIZING_CFLAGS) \
                          $(BUILD_VERSION_CFLAGS) $(CPP_OPT_FLAGS)

CPP_FLAGS               = $(CPP_INCLUDE_DIRS) $(CPP_FLAGS_EXPORT)

LIB_FLAGS_debug         = /DEBUGTYPE:CV
LIB_FLAGS_release       =
LIB_FLAGS_product       =
LIB_FLAGS               = /nologo $(LIB_FLAGS_$(BUILD))

ifeq ($(ENABLE_PCSL), true)
PCSL_LIBS               = $(PCSL_DIST_DIR)/lib/libpcsl_memory.lib   \
                          $(PCSL_DIST_DIR)/lib/libpcsl_print.lib    \
                          $(PCSL_DIST_DIR)/lib/libpcsl_network.lib  \
                          $(PCSL_DIST_DIR)/lib/libpcsl_string.lib   \
                          $(PCSL_DIST_DIR)/lib/libpcsl_file.lib
MAKE_EXPORT_EXTRA_LIBS += $(PCSL_LIBS)
endif

LINK_OPT_FLAGS_debug    = /pdb:$(basename $@).pdb
LINK_OPT_FLAGS_release  =
LINK_OPT_FLAGS_product  =

LINK_OPT_FLAGS_EXPORT_debug    = /debug 
LINK_OPT_FLAGS_EXPORT_release  =
LINK_OPT_FLAGS_EXPORT_product  =

LINK_ARCH_FLAGS_arm    += /SUBSYSTEM:$(CESubsystem)
LINK_ARCH_FLAGS_i386   += /MACHINE:ix86 /SUBSYSTEM:$(CESubsystem) \
                          /base:"0x00010000" /stack:0x10000,0x1000
LINK_ARCH_FLAGS         = $(LINK_ARCH_FLAGS_$(arch)) /LIBPATH:$(EVC_LIB_PATH) \
                          /VERBOSE:LIB

LINK_FLAGS_EXPORT       = /incremental:no /nologo /entry:"WinMainCRTStartup" \
                          /MAP $(LINK_ARCH_FLAGS) corelibc.lib aygshell.lib \
                          /nodefaultlib:libc.lib \
                          /nodefaultlib:libcd.lib /nodefaultlib:libcmt.lib \
                          /nodefaultlib:libcmtd.lib /nodefaultlib:msvcrt.lib \
                          /nodefaultlib:msvcrtd.lib /nodefaultlib:oldnames.lib

LINK_FLAGS_EXPORT      += $(LINK_OPT_FLAGS_EXPORT_$(BUILD))
LINK_FLAGS              = $(LINK_FLAGS_EXPORT) 

CPP_USE_PCH             = /Fp"cldchi.pch" /Yu"incls/_precompiled.incl"
BUILD_PCH               = _build_pch_visCPP.obj

$(BUILD_PCH): $(Precompiled_Headers)
	$(A)echo Generating $@ ...
	$(A)echo '#include "incls/_precompiled.incl"' > \
		$(GEN_DIR)/_build_pch_visCPP.cpp
	$(A)$(CPP) $(CPP_OPT_FLAGS) $(CPP_FLAGS) /Fp"cldchi.pch" \
	    /Yc"incls/_precompiled.incl" /c $(GEN_DIR)/_build_pch_visCPP.cpp
	$(A)echo '    done'

# Special case (fails to compile with precompiled header files)

OS_$(os_family).obj: $(WorkSpace)/src/vm/os/$(os_family)/OS_$(os_family).cpp
	$(A)$(CPP) $(CPP_OPT_FLAGS) $(CPP_FLAGS) -c $<

# Can't use precompiled headers
ifneq ($(CompileROMImageSeparately), true)
ROMImage.obj: $(GENERATED_ROM_FILE)
	$(A)$(CPP) $(CPP_OPT_FLAGS) $(CPP_FLAGS) -c $<
else
ROMImage_%.obj: ROMImage_%.cpp
	$(A)$(CPP) $(CPP_OPT_FLAGS) $(CPP_FLAGS) -c $<
endif

# Can't use precompiled headers
NativesTable.obj: $(NATIVES_TABLE)
	$(A)$(CPP) $(CPP_OPT_FLAGS) $(CPP_FLAGS) -c $<

ifeq ($(SAVE_PREPROCESSOR_OUTPUT),true)
define BUILD_C_TARGET
	$(A)$(CPP) $(CPP_OPT_FLAGS) $(CPP_FLAGS) $(CPP_USE_PCH) /P \
		`$(call fixcygpath, $<)`
	$(A)$(CPP) $(CPP_OPT_FLAGS) $(CPP_FLAGS) $(CPP_USE_PCH) -c \
		`$(call fixcygpath, $<)`
endef
else
define BUILD_C_TARGET
	$(A)$(CPP) $(CPP_OPT_FLAGS) $(CPP_FLAGS) $(CPP_USE_PCH) -c \
		`$(call fixcygpath, $<)`
endef
endif

Interpreter_arm.obj: Interpreter_arm.asm
	$(A)$(ASM) -o $@ -list "Interpreter_$(arch).lst" $(THIS_DIR)/$<

Interpreter_i386.obj: Interpreter_i386.asm
	$(A)$(ASM) /Zi /FoInterpreter_i386.obj /coff \
		/Sc /FlInterpreter_i386.lst /c $(THIS_DIR)/$<

%.obj: %.cpp
	$(A)$(CPP) $(CPP_OPT_FLAGS) $(CPP_FLAGS) $(CPP_USE_PCH) -c $<

MAKE_EXPORT_LINK_OUT_SWITCH1     =
MAKE_EXPORT_LINK_OUT_SWITCH2     = /out:
MAKE_EXPORT_EXTRA_LIBS          += $(LIBS)
MAKE_EXPORT_THREAD_LIBS          =

$(DIST_LIB_DIR)/$(JVM_LOG_NAME)::
	$(A)rm -f $@
	$(A)echo 'ARM_PATH $(PATH)' > $@
	$(A)echo "ARM_LIB $(LIB)" >> $@
	$(A)echo "ARM_INCLUDE $(INCLUDE)" >> $@
	$(A)echo TBD >> $@

# Split the object files into:
#     - LIB_OBJS:  the ones that are exported publicly in $(JVM_LIB)
#     - LIBX_OBJS: the ones that are exported privately in $(JVMX_LIB)
#     - LIBTEST_OBJS: the ones that are exported privately in $(JVMTEST_LIB)
#     - EXE_OBJS:  the ones that are used only by $(JVM_EXE)
LIB_OBJS := $(Obj_Files) OopMaps.obj
LIB_OBJS := $(subst BSDSocket.obj,,$(LIB_OBJS))
LIBX_OBJS +=        BSDSocket.obj
LIB_OBJS := $(subst ReflectNatives.obj,,$(LIB_OBJS))
LIBTEST_OBJS +=     ReflectNatives.obj
LIB_OBJS := $(subst jvmspi.obj,,$(LIB_OBJS))
EXE_OBJS +=         jvmspi.obj
LIB_OBJS := $(subst Main_$(os_family).obj,,$(LIB_OBJS))
EXE_OBJS +=         Main_$(os_family).obj
LIB_OBJS := $(subst NativesTable.obj,,$(LIB_OBJS))
EXE_OBJS +=         NativesTable.obj
LIB_OBJS := $(subst ROMImage.obj,,$(LIB_OBJS))

ifneq ($(ENABLE_C_INTERPRETER), true)
LIB_OBJS += Interpreter_$(arch).obj
endif

EXE_OBJS +=         ROMImage.obj
EXE_OBJS +=         resources.res

resources.res: $(WorkSpace)/src/vm/os/wince/resources.rc \
               $(WorkSpace)/src/vm/os/wince/resources.h
	@echo creating $@
	$(A)$(RC) -D_WIN32_WCE=$(CEVersion) /I"${EVC_INCLUDE_PATH}" \
		-DUNDER_CE=$(CEVersion) /d "UNICODE" /d "_UNICODE" /fo"$@" \
		$(WorkSpace)/src/vm/os/wince/resources.rc

ifeq ($(SeparateROMImage), true)
ifeq ($(CompileROMImageSeparately), true)
ROM_SEGMENTS_OBJS = ROMImage_00.obj \
		            ROMImage_01.obj \
		            ROMImage_02.obj \
		            ROMImage_03.obj \
		            ROMImage_04.obj \
		            ROMImage_05.obj \
		            ROMImage_06.obj \
		            ROMImage_07.obj \
		            ROMImage_08.obj \
		            ROMImage_09.obj \
		            ROMImage_10.obj \
		            ROMImage_11.obj \
		            ROMImage_12.obj
		            
EXE_OBJS := $(subst ROMImage.obj,,$(EXE_OBJS))
EXE_OBJS += $(ROM_SEGMENTS_OBJS)
$(ROM_SEGMENTS_OBJS): $(GENERATED_ROM_FILE)

endif

ROM_SEGMENTS = $(GEN_DIR)/ROMImage_00.cpp \
               $(GEN_DIR)/ROMImage_01.cpp \
               $(GEN_DIR)/ROMImage_02.cpp \
               $(GEN_DIR)/ROMImage_03.cpp \
               $(GEN_DIR)/ROMImage_04.cpp \
               $(GEN_DIR)/ROMImage_05.cpp \
               $(GEN_DIR)/ROMImage_06.cpp \
               $(GEN_DIR)/ROMImage_07.cpp \
               $(GEN_DIR)/ROMImage_08.cpp \
               $(GEN_DIR)/ROMImage_09.cpp \
               $(GEN_DIR)/ROMImage_10.cpp \
               $(GEN_DIR)/ROMImage_11.cpp \
               $(GEN_DIR)/ROMImage_12.cpp \
               $(GEN_DIR)/ROMImageGenerated.hpp

# Use $(GENERATED_ROM_FILE) as a marker to regenerate $(ROM_SEGMENTS).
# Add $(ROM_SEGMENTS) into the set of prerequisites of $(GENERATED_ROM_FILE),
# without overriding the command for $(GENERATED_ROM_FILE) target.
$(GENERATED_ROM_FILE): $(ROM_SEGMENTS)
$(ROM_SEGMENTS):

endif

$(JVM_LIB): $(BIN_DIR) $(BUILD_PCH) $(LIB_OBJS)
	$(A)$(LIBMGR) $(LIB_FLAGS) /out:$@ $(LIB_OBJS)
	$(A)echo generated `pwd`/$@

$(JVMX_LIB): $(BIN_DIR) $(BUILD_PCH) $(LIBX_OBJS)
	$(A)$(LIBMGR) $(LIB_FLAGS) /out:$@ $(LIBX_OBJS)
	$(A)echo generated `pwd`/$@

$(JVMTEST_LIB): $(BIN_DIR) $(BUILD_PCH) $(LIBTEST_OBJS)
	$(A)$(LIBMGR) $(LIB_FLAGS) /out:$@ $(LIBTEST_OBJS)
	$(A)echo generated `pwd`/$@

$(JVM_EXE): $(BIN_DIR) $(BUILD_PCH) $(JVMX_LIB) $(JVMTEST_LIB) \
            $(JVM_LIB) $(EXE_OBJS)
	$(A)$(LINK) $(LINK_FLAGS) /out:$@ $(EXE_OBJS) $(JVMX_LIB) \
            $(JVMTEST_LIB) $(JVM_LIB) $(LINK_OPT_FLAGS_$(BUILD)) \
            $(LIBS) $(PCSL_LIBS)

	$(A)echo generated `pwd`/$@

endif

#----------------------------------------------------------------------
#
# Compiler Section: gcc (GNU C++ compiler 2/3.x)
#
#----------------------------------------------------------------------

ifeq ($(compiler), gcc)

ifeq ($(IsTarget), true)
gcc_arch = $(arch)
else
gcc_arch = $(host_arch)
endif

GCC_PREFIX_arm     = $(GNU_TOOLS_DIR)/bin/
GCC_PREFIX_sh      = $(GNU_TOOLS_DIR)/bin/
GCC_PREFIX_mips    = $(GNU_TOOLS_DIR)/bin/
GCC_PREFIX_i386    =
GCC_PREFIX_sparc   =
GCC_PREFIX_powerpc =
GCC_PREFIX_c       = $(GCC_PREFIX_$(target_arch))
GCC_PREFIX         = $(GCC_PREFIX_$(gcc_arch))

# for now GCC_VERSION is relevant only on i386
GCC_POSTFIX_i386   = $(GCC_VERSION)
GCC_POSTFIX        = $(GCC_POSTFIX_$(gcc_arch))

ifeq ("$(FORCE_GCC)x", "x")

 ASM_gcc           = $(GCC_PREFIX)as
 CPP_gcc           = $(GCC_PREFIX)g++$(GCC_POSTFIX)
 CC_gcc            = $(GCC_PREFIX)gcc$(GCC_POSTFIX)
 LINK_gcc          = $(GCC_PREFIX)g++$(GCC_POSTFIX)
 LIBMGR_gcc        = $(GCC_PREFIX)ar

else

 CPP_gcc           = $(FORCE_GCC)
 CC_gcc            = $(FORCE_GCC)
 LINK_gcc          = $(FORCE_GCC)
 ASM_gcc           = as
 LIBMGR_gcc        = ar

endif

LIB_FLAGS          = cqs

ifeq ($(os), darwin)
 # darwin is picky on the lib manager
 LIBMGR_gcc        = libtool
 LIB_FLAGS         = -static -o
endif

CPP_OPT_FLAGS		=

ifeq ($(IsTarget), true)
ifeq ($(ENABLE_STATIC_ANALYZER), true)
CPP_OPT_FLAGS           += -fno-default-inline -fno-inline \
                           -fdump-translation-unit
NO_DEBUG_SYMBOLS         = true
endif
endif

ifeq ($(NO_DEBUG_SYMBOLS), true)
    DEBUG_SYMBOLS_FLAGS     =
    CPP_OPT_FLAGS_debug     = -O2 -Wuninitialized -fexpensive-optimizations
    ifneq ($(PROFILING), true)
    CPP_OPT_FLAGS_debug    += -fomit-frame-pointer
    endif
else
    DEBUG_SYMBOLS_FLAGS     = -g
    CPP_OPT_FLAGS_debug     = -g
endif

# -Winline -fkeep-inline-functions  -fno-inline

ifeq ($(PRODUCT_DEBUG), true)
CPP_OPT_FLAGS_release   += -g
CPP_OPT_FLAGS_product   += -g
else
CPP_OPT_FLAGS_release   = -O2 -Wuninitialized
CPP_OPT_FLAGS_product   = -O2 -Wuninitialized \
                          -fexpensive-optimizations
  ifneq ($(PROFILING), true)
  CPP_OPT_FLAGS_product+= -fomit-frame-pointer
  endif
endif

ifeq ($(USE_GCOV), true)
   CPP_OPT_FLAGS_debug   = -O0 -fprofile-arcs -ftest-coverage -g
   CPP_OPT_FLAGS_release = -O0 -fprofile-arcs -ftest-coverage
   CPP_OPT_FLAGS_product = -O0 -fprofile-arcs -ftest-coverage
   LINK_FLAGS	      	+= -lgcov
endif

# here C++ specific optimization flags
CPP_OPT_FLAGS_release   +=
CPP_OPT_FLAGS_debug     +=
CPP_OPT_FLAGS_product   +=
CPP_OPT_FLAGS           += $(CPP_OPT_FLAGS_$(BUILD))

CPP_DEF_FLAGS_debug      = -D_DEBUG -DAZZERT
CPP_DEF_FLAGS_release    =
CPP_DEF_FLAGS_product    = -DPRODUCT
CPP_DEF_FLAGS	        += $(CPP_DEF_FLAGS_$(BUILD))

# C compiler flags
CPP_DEF_FLAGS           += -pipe -DGCC
CPP_DEF_FLAGS           += -Wreturn-type -Wcomment -Wparentheses -Wformat
CPP_DEF_FLAGS           += -fstrict-aliasing

# C++ specific compiler flags
CPLUSPLUS_FLAGS         += -fno-gnu-keywords
CPLUSPLUS_FLAGS         += -fno-operator-names
CPLUSPLUS_FLAGS         += -fno-exceptions
CPLUSPLUS_FLAGS         += -fno-optional-diags
CPLUSPLUS_FLAGS         += -fno-rtti

CPP_DEF_FLAGS_i386       = -Di386
CPP_DEF_FLAGS_arm	 =
CPP_DEF_FLAGS_win32      = -DWIN32 -D_WINDOWS
CPP_DEF_FLAGS_linux      = -DLINUX
ifeq ($(host_os), cygwin)
CPP_DEF_FLAGS            += -DCYGWIN
ENABLE_MAP_FILE          = false
endif
CPP_DEF_FLAGS           += $(CPP_DEF_FLAGS_$(BUILD)) $(CPP_DEF_FLAGS_$(arch)) \
                           $(CPP_DEF_FLAGS_$(os_family))

CPP_DEF_FLAGS           += -DREQUIRES_JVMCONFIG_H=1 \
                           $(SAVE_TEMPS_CFLAGS) $(ENABLE_CFLAGS) \
                           $(ROMIZING_CFLAGS) \
                           $(BUILD_VERSION_CFLAGS) $(USER_CFLAGS)

CC_FLAGS_EXPORT          = $(CPP_DEF_FLAGS)
CC_OPT_FLAGS             = $(CPP_OPT_FLAGS)
CPP_FLAGS_EXPORT         = $(CPP_DEF_FLAGS) $(CPLUSPLUS_FLAGS)

CPP_FLAGS                = $(CPP_FLAGS_EXPORT) $(CPP_INCLUDE_DIRS)

ifneq ($(ENABLE_COMPILATION_WARNINGS), true)
CPP_FLAGS               += -Werror
endif

LINK_OPT_FLAGS_debug    = $(DEBUG_SYMBOLS_FLAGS)
LINK_OPT_FLAGS_release  =
ifeq ($(PRODUCT_DEBUG), true)
LINK_OPT_FLAGS_product  = -g
else
ifneq ($(PROFILING), true)
LINK_OPT_FLAGS_product  =
endif
endif

ifeq ($(ENABLE_TIMER_THREAD), true)
LINK_PTHREAD=true
endif
ifeq ($(LINK_PTHREAD), true)
LINK_FLAGS             += -lpthread
endif

ifeq ($(ENABLE_PCSL), true)
PCSL_LIBS               = $(PCSL_DIST_DIR)/lib/libpcsl_memory.a  \
                          $(PCSL_DIST_DIR)/lib/libpcsl_print.a   \
                          $(PCSL_DIST_DIR)/lib/libpcsl_network.a \
                          $(PCSL_DIST_DIR)/lib/libpcsl_string.a  \
                          $(PCSL_DIST_DIR)/lib/libpcsl_file.a
MAKE_EXPORT_EXTRA_LIBS += $(PCSL_LIBS)
endif

ifeq ($(ENABLE_DYNAMIC_NATIVE_METHODS), true)
LINK_FLAGS             += -ldl -Wl,-export-dynamic
endif

ifeq ($(ENABLE_C_INTERPRETER), true)
LINK_FLAGS             += -lm
endif

LINK_FLAGS             += $(LINK_OPT_FLAGS_$(BUILD))

ifeq ("$(ENABLE_STATIC_LINK)-$(IsTarget)", "true-true")
LINK_FLAGS             += -static
endif

# statically link romgen, so that it can be used on another Linux distribution
# that may have an incompatible libc.so
ifeq ($(host_os)-$(IsRomGen), linux-true)
LINK_FLAGS             += -static
endif

ifeq ($(PROFILING), true)
CPP_FLAGS              += -pg
LINK_FLAGS             += -pg
endif

ifeq ($(ENABLE_XSCALE_WMMX_INSTRUCTIONS)-$(IsTarget)-$(arch), true-true-arm)
ASM_FLAGS              += -mcpu=iwmmxt
endif
ifneq ($(ENABLE_C_INTERPRETER), true)
Interpreter_$(arch)$(OBJ_SUFFIX): Interpreter_$(arch).s
	$(A) echo "ASMing $<"
	$(A)$(ASM) $(ASM_FLAGS) -o Interpreter_$(arch)$(OBJ_SUFFIX) \
		$(THIS_DIR)/Interpreter_$(arch).s
else
AsmStubs_$(target_arch)$(OBJ_SUFFIX): $(JVMWorkSpace)/src/vm/cpu/c/AsmStubs_$(target_arch).s
	$(A)echo "generating CPU stubs $<"
	$(A)cpp -D$(host_os) $< > $(THIS_DIR)/AsmStubs_$(target_arch).s
	$(A)$(ASM) $(ASM_FLAGS) -o $@ $(THIS_DIR)/AsmStubs_$(target_arch).s
endif

define BUILD_C_TARGET
	$(A)echo " ... $(notdir $<)"
	$(A)$(CPP) $(CPP_OPT_FLAGS) $(CPP_FLAGS) $(THUMB_CFLAGS) -c $< -o $@
endef

ifeq ($(ENABLE_THUMB_VM)+$(IsTarget), true+true)
%$(OBJ_SUFFIX): $(WorkSpace)/src/vm/os/linux/%.cpp
	$(A)echo " ... $(notdir $<) to ARM"
	$(A)$(CPP) $(CPP_OPT_FLAGS) $(CPP_FLAGS) $(ARM_CFLAGS) -c $<
else
%$(OBJ_SUFFIX): $(WorkSpace)/src/vm/os/linux/%.cpp
	$(BUILD_C_TARGET)
endif

MAKE_EXPORT_LINK_OUT_SWITCH1     = -o
MAKE_EXPORT_LINK_OUT_SWITCH2     =
MAKE_EXPORT_EXTRA_LIBS          +=
MAKE_EXPORT_THREAD_LIBS          = -lpthread

$(DIST_LIB_DIR)/$(JVM_LOG_NAME)::
	$(A)rm -f $@
	$(A)echo ===================================  > $@
	$(A)echo JVM Build Log                       >> $@
	$(A)echo "Created on `date`"                 >> $@
	$(A)echo "JVMWorkSpace=${WorkSpace}"         >> $@
	$(A)echo "JVMBuildSpace=${BuildSpace}"       >> $@
	$(A)echo "GNU_TOOLS_DIR=${GNU_TOOLS_DIR}"    >> $@
	$(A)echo ""                                  >> $@
	$(A)echo + JAVAC version:                    >> $@
	$(A)sh -c '$(JAVAC) -J-version 2>&1' | cat   >> $@
	$(A)echo ""                                  >> $@
	$(A)echo + GCC executable:                   >> $@
	$(A)ls -l `which $(CPP)`                     >> $@
	$(A)echo ""                                  >> $@
	$(A)echo + GCC version:                      >> $@
	$(A)sh -c '$(CPP) -v 2>&1' | cat             >> $@
	$(A)sh -c '$(CPP) -version 2>&1' | cat       >> $@
	$(A)echo ""                                  >> $@
	$(A)echo + Host info:                        >> $@
	$(A)hostname                                 >> $@
	$(A)uname -a                                 >> $@
	$(A)echo ""                                  >> $@
	$(A)echo + QT version:                       >> $@
	$(A)if test -d /opt/Qtopia/lib; then \
		echo "ls -l /opt/Qtopia/lib" >> $@; \
		ls -l /opt/Qtopia/lib >> $@; \
	 else \
		echo "Qt does not seem to be installed" >> $@; \
	 fi
	$(A)echo ""                                  >> $@
	$(A)echo + jvm_config.h:                     >> $@
	$(A)cat $(GEN_DIR)/jvmconfig.h               >> $@

ifeq ($(IsLoopGen), true)
ifeq ($(ENABLE_C_INTERPRETER), true)
$(LOOP_GENERATOR):
	$(A)touch $@
	$(A)echo "created dummy loop generator"
else
$(LOOP_GENERATOR): $(CLDC_ZIP) $(Obj_Files) InterpreterSkeleton$(OBJ_SUFFIX) \
					    OopMapsSkeleton$(OBJ_SUFFIX)
	$(A)echo "linking ASM loop generator: $(LOOP_GENERATOR)"
	$(A)$(LINK) -o $@ $(Obj_Files) InterpreterSkeleton$(OBJ_SUFFIX) \
		OopMapsSkeleton$(OBJ_SUFFIX) $(PCSL_LIBS) $(LINK_FLAGS)
	$(A)echo generated `pwd`/$@
endif
endif

ifeq ($(IsRomGen), true)

ifeq ($(ENABLE_C_INTERPRETER), true)

ifdef carch
$(GP_TABLE_OBJ): $(LOOP_GENERATOR)
	$(A)$(LOOP_GENERATOR) -generate +GenerateGPTableOnly +GenerateGNUCode \
		$(LOOP_GEN_ARG)
	if test "$(carch)" = "thumb"; then \
	    mv $(THIS_DIR)/Interpreter_arm.s \
               $(THIS_DIR)/Interpreter_$(carch).s; \
	fi
	$(A)$(ASM) $(ASM_FLAGS) -o $@ \
		$(THIS_DIR)/Interpreter_$(carch).s
endif

$(ROM_GENERATOR): $(CLDC_ZIP) $(Obj_Files) OopMapsSkeleton$(OBJ_SUFFIX) \
	 	  $(GP_TABLE_OBJ)
	$(A)echo "linking ROM generator: $(ROM_GENERATOR)"
	$(A)$(LINK) -o $@ $(Obj_Files) OopMapsSkeleton$(OBJ_SUFFIX) \
	          $(GP_TABLE_OBJ) $(PCSL_LIBS) $(LINK_FLAGS)
	$(A)$(ROM_GENERATOR) +GenerateOopMaps
	$(A)$(MAKE) OopMaps$(OBJ_SUFFIX)
	$(A)echo "re-linking ROM generator: $(ROM_GENERATOR)"
	$(A)$(LINK) -o $@ $(Obj_Files) OopMaps$(OBJ_SUFFIX) $(GP_TABLE_OBJ) \
		$(PCSL_LIBS) $(LINK_FLAGS)
	$(A)echo generated `pwd`/$@
else
$(ROM_GENERATOR): $(CLDC_ZIP) $(Obj_Files) InterpreterSkeleton$(OBJ_SUFFIX) \
					   OopMapsSkeleton$(OBJ_SUFFIX)
	$(A)echo "linking ROM generator: $(ROM_GENERATOR)"
	$(A)$(LINK) $(Obj_Files) InterpreterSkeleton$(OBJ_SUFFIX) \
		OopMapsSkeleton$(OBJ_SUFFIX) \
		-o $@ $(PCSL_LIBS) $(LINK_FLAGS)
	$(A)$(ROM_GENERATOR) $(LOOP_GEN_ARG)
	$(A)$(ROM_GENERATOR) +GenerateOopMaps
	$(A)$(MAKE) Interpreter_$(arch)$(OBJ_SUFFIX)
	$(A)$(MAKE) OopMaps$(OBJ_SUFFIX)
	$(A)$(LINK) -o $@ \
		$(Obj_Files) Interpreter_$(arch)$(OBJ_SUFFIX) \
                OopMaps$(OBJ_SUFFIX) $(PCSL_LIBS) $(LINK_FLAGS)
	$(A)echo generated `pwd`/$@
endif

endif

# Split the object files into:
#     - LIB_OBJS:  the ones that are exported publicly in $(JVM_LIB)
#     - LIBX_OBJS: the ones that are exported privately in $(JVMX_LIB)
#     - LIBTEST_OBJS: the ones that are exported privately in $(JVMTEST_LIB)
#     - EXE_OBJS:  the ones that are used only by $(JVM_EXE)
LIB_OBJS := $(Obj_Files) OopMaps$(OBJ_SUFFIX)
LIB_OBJS := $(subst BSDSocket$(OBJ_SUFFIX),,$(LIB_OBJS))
LIBX_OBJS +=        BSDSocket$(OBJ_SUFFIX)
LIB_OBJS := $(subst ReflectNatives$(OBJ_SUFFIX),,$(LIB_OBJS))
LIBTEST_OBJS +=     ReflectNatives$(OBJ_SUFFIX)
LIB_OBJS := $(subst jvmspi$(OBJ_SUFFIX),,$(LIB_OBJS))
EXE_OBJS +=         jvmspi$(OBJ_SUFFIX)
LIB_OBJS := $(subst Main_$(os_family)$(OBJ_SUFFIX),,$(LIB_OBJS))
EXE_OBJS +=         Main_$(os_family)$(OBJ_SUFFIX)
LIB_OBJS := $(subst NativesTable$(OBJ_SUFFIX),,$(LIB_OBJS))
EXE_OBJS +=         NativesTable$(OBJ_SUFFIX)
LIB_OBJS := $(subst ROMImage$(OBJ_SUFFIX),,$(LIB_OBJS))
EXE_OBJS +=         ROMImage$(OBJ_SUFFIX)	

ifeq ($(SeparateROMImage), true)
ifeq ($(CompileROMImageSeparately), true)
ROM_SEGMENTS_OBJS = ROMImage_00$(OBJ_SUFFIX) \
		            ROMImage_01$(OBJ_SUFFIX) \
		            ROMImage_02$(OBJ_SUFFIX) \
		            ROMImage_03$(OBJ_SUFFIX) \
		            ROMImage_04$(OBJ_SUFFIX) \
		            ROMImage_05$(OBJ_SUFFIX) \
		            ROMImage_06$(OBJ_SUFFIX) \
		            ROMImage_07$(OBJ_SUFFIX) \
		            ROMImage_08$(OBJ_SUFFIX) \
		            ROMImage_09$(OBJ_SUFFIX) \
		            ROMImage_10$(OBJ_SUFFIX) \
		            ROMImage_11$(OBJ_SUFFIX) \
		            ROMImage_12$(OBJ_SUFFIX)

EXE_OBJS := $(subst ROMImage$(OBJ_SUFFIX),,$(EXE_OBJS))
EXE_OBJS += $(ROM_SEGMENTS_OBJS)
$(ROM_SEGMENTS_OBJS): $(GENERATED_ROM_FILE)

endif

ROM_SEGMENTS = $(GEN_DIR)/ROMImage_00.cpp \
               $(GEN_DIR)/ROMImage_01.cpp \
               $(GEN_DIR)/ROMImage_02.cpp \
               $(GEN_DIR)/ROMImage_03.cpp \
               $(GEN_DIR)/ROMImage_04.cpp \
               $(GEN_DIR)/ROMImage_05.cpp \
               $(GEN_DIR)/ROMImage_06.cpp \
               $(GEN_DIR)/ROMImage_07.cpp \
               $(GEN_DIR)/ROMImage_08.cpp \
               $(GEN_DIR)/ROMImage_09.cpp \
               $(GEN_DIR)/ROMImage_10.cpp \
               $(GEN_DIR)/ROMImage_11.cpp \
               $(GEN_DIR)/ROMImage_12.cpp \
               $(GEN_DIR)/ROMImageGenerated.hpp

# Use $(GENERATED_ROM_FILE) as a marker to regenerate $(ROM_SEGMENTS).
# Add $(ROM_SEGMENTS) into the set of prerequisites of $(GENERATED_ROM_FILE),
# without overriding the command for $(GENERATED_ROM_FILE) target.
$(GENERATED_ROM_FILE): $(ROM_SEGMENTS)
$(ROM_SEGMENTS):
endif

ifneq ($(ENABLE_C_INTERPRETER), true)
LIB_OBJS += Interpreter_$(arch)$(OBJ_SUFFIX)
endif

$(JVM_LIB): $(BUILD_PCH) $(LIB_OBJS)
	$(A)echo "creating $@ ... "
	$(A)rm -f $@
	$(A)$(LIBMGR) $(LIB_FLAGS) $@ $(LIB_OBJS)
	$(A)echo generated `pwd`/$@

$(JVMX_LIB): $(BUILD_PCH) $(LIBX_OBJS)
	$(A)echo "creating $@ ... "
	$(A)rm -f $@
	$(A)$(LIBMGR) $(LIB_FLAGS) $@ $(LIBX_OBJS)
	$(A)echo generated `pwd`/$@

$(JVMTEST_LIB): $(BUILD_PCH) $(LIBTEST_OBJS)
	$(A)echo "creating $@ ... "
	$(A)rm -f $@
	$(A)$(LIBMGR) $(LIB_FLAGS) $@ $(LIBTEST_OBJS)
	$(A)echo generated `pwd`/$@

ANI_OBJS  = ani$(OBJ_SUFFIX) os_port$(OBJ_SUFFIX) poolthread$(OBJ_SUFFIX)
ANIX_OBJS = ani_bsd_socket$(OBJ_SUFFIX)

$(ANI_LIB): $(JVM_LIB) $(ANI_OBJS)
	$(A)$(LIBMGR) $(LIB_FLAGS) $@ $(ANI_OBJS)
	$(A)echo generated `pwd`/$@

$(ANIX_LIB): $(JVM_LIB) $(ANIX_OBJS)
	$(A)$(LIBMGR)  $(LIB_FLAGS) $@ $(ANIX_OBJS)
	$(A)echo generated `pwd`/$@

$(JVM_EXE): $(CLDC_ZIP) $(EXE_OBJS) $(JVM_LIB) $(JVMX_LIB) $(JVMTEST_LIB)
	$(A)echo "linking $@ ... "
	$(A)$(LINK) -o $@ $(EXE_OBJS) $(JVMX_LIB) $(JVMTEST_LIB) $(JVM_LIB) \
	     $(PCSL_LIBS) $(LINK_FLAGS)
	$(A)if [ "$(ENABLE_MAP_FILE)" != "false" ] &&             \
            [ "$(host_os)" != "solaris" ] &&                      \
	     [ "$(host_os)" != "darwin" ]; then                   \
	 echo "linking $@ with MAP file ... ";                    \
	 $(LINK) -o $@ $(EXE_OBJS) $(JVMX_LIB) $(JVMTEST_LIB)     \
	     $(JVM_LIB)  $(PCSL_LIBS) $(LINK_FLAGS)               \
	     -Xlinker -M 2> $(JVM_MAP);                           \
        fi
	$(A)echo generated `pwd`/$@

PRINT_CONFIG = print_config

print_config:
	$(A)if test "$(IsTarget)-$(arch)" = "true-arm"; then \
	 echo ==================================================; \
	 echo 'GNU_TOOLS_DIR = $(GNU_TOOLS_DIR)'; \
	 echo ==================================================; \
	fi
	$(A)if test "$(FORCE_GCC)" != ""; then \
         echo ==================================================; \
	 echo 'FORCE_GCC = $(FORCE_GCC)'; \
	 echo ==================================================; \
	fi
endif

#----------------------------------------------------------------------
#
# Compiler Section: alternative compilers
#
#----------------------------------------------------------------------

ifdef ALT_COMPILER
include $(SHARE_DIR)/$(ALT_COMPILER)/alt_compiler.make
endif

#----------------------------------------------------------------------
#
# Default rules: build common C source files. This section is needed
# because some versions of "make" are buggy and cannot find the source
# files with the %$(OBJ_SUFFIX): %.cpp rule.
#
#----------------------------------------------------------------------
%$(OBJ_SUFFIX): %.cpp
	$(BUILD_C_TARGET)

%$(OBJ_SUFFIX): $(WorkSpace)/src/vm/share/natives/%.cpp
	$(BUILD_C_TARGET)

%$(OBJ_SUFFIX): $(WorkSpace)/src/vm/share/interpreter/%.cpp
	$(BUILD_C_TARGET)

%$(OBJ_SUFFIX): $(WorkSpace)/src/vm/os/win32/%.cpp
	$(BUILD_C_TARGET)

%$(OBJ_SUFFIX): $(WorkSpace)/src/vm/share/runtime/%.cpp
	$(BUILD_C_TARGET)

%$(OBJ_SUFFIX): $(GEN_DIR)/%.cpp
	$(BUILD_C_TARGET)

%$(OBJ_SUFFIX): $(WorkSpace)/src/vm/cpu/arm/%.cpp
	$(BUILD_C_TARGET)

%$(OBJ_SUFFIX): $(WorkSpace)/src/vm/cpu/c/%.cpp
	$(BUILD_C_TARGET)

%$(OBJ_SUFFIX): $(WorkSpace)/src/vm/cpu/i386/%.cpp
	$(BUILD_C_TARGET)

%$(OBJ_SUFFIX): $(WorkSpace)/src/vm/cpu/sh/%.cpp
	$(BUILD_C_TARGET)

%$(OBJ_SUFFIX): $(WorkSpace)/src/vm/cpu/thumb/%.cpp
	$(BUILD_C_TARGET)

%$(OBJ_SUFFIX): $(WorkSpace)/src/vm/os/utilities/%.cpp
	$(BUILD_C_TARGET)

%$(OBJ_SUFFIX): $(WorkSpace)/src/vm/share/compiler/%.cpp
	$(BUILD_C_TARGET)

%$(OBJ_SUFFIX): $(WorkSpace)/src/vm/share/debugger/%.cpp
	$(BUILD_C_TARGET)

%$(OBJ_SUFFIX): $(WorkSpace)/src/vm/share/handles/%.cpp
	$(BUILD_C_TARGET)

%$(OBJ_SUFFIX): $(WorkSpace)/src/vm/share/interpreter/%.cpp
	$(BUILD_C_TARGET)

%$(OBJ_SUFFIX): $(WorkSpace)/src/vm/share/isolate/%.cpp
	$(BUILD_C_TARGET)

%$(OBJ_SUFFIX): $(WorkSpace)/src/vm/share/memory/%.cpp
	$(BUILD_C_TARGET)

%$(OBJ_SUFFIX): $(WorkSpace)/src/vm/share/natives/%.cpp
	$(BUILD_C_TARGET)

%$(OBJ_SUFFIX): $(WorkSpace)/src/vm/share/ROM/%.cpp
	$(BUILD_C_TARGET)

%$(OBJ_SUFFIX): $(WorkSpace)/src/vm/share/reflection/%.cpp
	$(BUILD_C_TARGET)

%$(OBJ_SUFFIX): $(WorkSpace)/src/vm/share/runtime/%.cpp
	$(BUILD_C_TARGET)

%$(OBJ_SUFFIX): $(WorkSpace)/src/vm/share/verifier/%.cpp
	$(BUILD_C_TARGET)

%$(OBJ_SUFFIX): $(WorkSpace)/src/vm/share/utilities/%.cpp
	$(BUILD_C_TARGET)

%$(OBJ_SUFFIX): $(WorkSpace)/src/vm/share/memoryprofiler/%.cpp
	$(BUILD_C_TARGET)

%$(OBJ_SUFFIX): $(WorkSpace)/src/vm/share/float/%.cpp
	$(BUILD_C_TARGET)

%$(OBJ_SUFFIX): $(WorkSpace)/src/anilib/share/%.cpp
	$(BUILD_C_TARGET)

%$(OBJ_SUFFIX): $(WorkSpace)/src/anilib/$(os_family)/%.cpp
	$(BUILD_C_TARGET)

%$(OBJ_SUFFIX): $(WorkSpace)/src/vm/cpu/$(arch)/$(CPU_VARIANT)/%.cpp
	$(BUILD_C_TARGET)

%$(OBJ_SUFFIX): $(WorkSpace)/src/vm/cpu/$(carch)/$(CPU_VARIANT)/%.cpp
	$(BUILD_C_TARGET)


#----------------------------------------------------------------------
#
# Default rules: create the VM or Generator
#
#----------------------------------------------------------------------

DIST_DIRS    = $(DIST_BIN_DIR) \
               $(DIST_LIB_DIR) \
               $(DIST_INC_DIR)

DIST_FILES   = $(DIST_BIN_DIR)/NativesTableGen.jar \
	       $(DIST_BIN_DIR)/kdp.jar \
	       $(DIST_BIN_DIR)/buildtool.jar \
	       $(PREVERIFY) \
               $(DIST_LIB_DIR)/cldc_classes.zip \
               $(DIST_LIB_DIR)/cldcx_classes.zip \
               $(DIST_LIB_DIR)/cldctest_classes.zip \
               $(DIST_LIB_DIR)/ROMSkeleton.cpp \
               $(DIST_LIB_DIR)/cldc_rom.cfg \
               $(DIST_LIB_DIR)/cldcx_rom.cfg \
               $(DIST_LIB_DIR)/cldctest_rom.cfg \
               $(DIST_INC_DIR)/sni.h \
               $(DIST_INC_DIR)/kni.h \
               $(DIST_INC_DIR)/jvm.h \
               $(DIST_INC_DIR)/jvmspi.h \
               $(DIST_INC_DIR)/kvmcompat.h \
               $(DIST_INC_DIR)/NativesTable.hpp \
               $(DIST_INC_DIR)/ROMImage.hpp

ifeq ($(IsRomGen), true)
OUTPUT_FILES = $(NATIVES_TABLE) $(ROM_GENERATOR)
DIST_FILES  += $(DIST_BIN_DIR)/$(ROMGEN)
endif

ifeq ($(IsLoopGen), true)
OUTPUT_FILES = $(NATIVES_TABLE) $(LOOP_GENERATOR)
endif

ifeq ($(IsTarget), true)
OUTPUT_FILES = $(NATIVES_TABLE) $(JVM_LIB) $(JVMX_LIB) $(JVMTEST_LIB) $(JVM_EXE)
DIST_FILES  += $(DIST_BIN_DIR)/$(JVM_EXE_NAME)
DIST_FILES  += $(DIST_BIN_DIR)/$(JVM_MAP_NAME)
DIST_FILES  += $(DIST_LIB_DIR)/$(JVM_LIB_NAME)
DIST_FILES  += $(DIST_LIB_DIR)/$(JVMX_LIB_NAME)
DIST_FILES  += $(DIST_LIB_DIR)/$(JVMTEST_LIB_NAME)
DIST_FILES  += $(DIST_LIB_DIR)/$(JVM_MAK_NAME)
DIST_FILES  += $(DIST_LIB_DIR)/$(JVM_LOG_NAME)
DIST_FILES  += $(DIST_INC_DIR)/kni_md.h
DIST_FILES  += $(DIST_INC_DIR)/jvmconfig.h

ifeq ($(ROMIZING), true)
DIST_FILES  += $(DIST_LIB_DIR)/ROMLog.txt
endif

ifeq ($(BUILD_ANI_LIB), true)
OUTPUT_FILES+= $(ANI_LIB) $(ANIX_LIB)
DIST_FILES  += $(DIST_LIB_DIR)/$(ANI_LIB_NAME)
DIST_FILES  += $(DIST_LIB_DIR)/$(ANIX_LIB_NAME)
DIST_FILES  += $(DIST_INC_DIR)/ani.h
endif

ifeq ($(ENABLE_JVM_MALLOC), true)
DIST_FILES  += $(DIST_INC_DIR)/JVM_Malloc.hpp
MakeDepsOpts += ENABLE_JVM_MALLOC=true
endif

ifeq ($(ENABLE_RMFS), true)
DIST_FILES  += $(DIST_INC_DIR)/rmfsApi.hpp
MakeDepsOpts += ENABLE_RMFS=true
endif
endif


ifeq ($(IsProject), true)
OUTPUT_FILES = cldc_vm.$(ide_workspace_ext) cldc_vm.$(ide_project_ext)
DIST_FILES=
endif

all: $(PRINT_CONFIG) dependencies $(BIN_DIR) $(DIST_DIRS) $(OUTPUT_FILES)
all: $(DIST_FILES)

all: $(EXTRA_OUTPUT_FILES) $(EXTRA_DIST_FILES)

$(BIN_DIR):
	mkdir -p $@

# if compiler's C and C++ flags are the same
ifndef CC_FLAGS_EXPORT
 CC_FLAGS_EXPORT = $(CPP_FLAGS_EXPORT)
endif


# Always (re)generate $(JVM_MAK_NAME) to reflect the latest env settings
$(DIST_LIB_DIR)/$(JVM_MAK_NAME)::
	$(A)rm -f $@
	$(A)echo "JVM_BUILD_EXT            = $(BUILD_EXT)"                >> $@
	$(A)echo "JVM_OBJ_SUFFIX           = $(OBJ_SUFFIX)"               >> $@
	$(A)echo "JVM_DLL_SUFFIX           = $(DLL_SUFFIX)"               >> $@
	$(A)echo "JVM_LIB_PREFIX           = $(LIB_PREFIX)"               >> $@
	$(A)echo "JVM_LIB_SUFFIX           = $(LIB_SUFFIX)"               >> $@
	$(A)echo "JVM_EXE_SUFFIX           = $(EXE_SUFFIX)"               >> $@
	$(A)echo "JVM_HOST_EXE_SUFFIX      = $(HOST_EXE_SUFFIX)"          >> $@
	$(A)echo "JVM_CC_FLAGS             = $(CC_FLAGS_EXPORT)"          >> $@
	$(A)echo "JVM_CPP_FLAGS            = $(CPP_FLAGS_EXPORT)"         >> $@
	$(A)echo "JVM_CC_OPT_FLAGS         = $(CC_OPT_FLAGS)"             >> $@
	$(A)echo "JVM_CPP_OPT_FLAGS        = $(CPP_OPT_FLAGS)"            >> $@
	$(A)echo "JVM_ARM_CFLAGS           = $(ARM_CFLAGS)"               >> $@
	$(A)echo "JVM_THUMB_CFLAGS         = $(THUMB_CFLAGS)"             >> $@
	$(A)echo "JVM_LINK_FLAGS           = $(LINK_FLAGS)"               >> $@
	$(A)echo "JVM_LIB_FLAGS            = $(LIB_FLAGS)"                >> $@
	$(A)echo "JVM_JVM_LIB_NAME         = $(JVM_LIB_NAME)"             >> $@
	$(A)echo "JVM_JVMX_LIB_NAME        = $(JVMX_LIB_NAME)"            >> $@
	$(A)echo "JVM_JVMTEST_LIB_NAME     = $(JVMTEST_LIB_NAME)"         >> $@
	$(A)echo "JVM_ANI_LIB_NAME         = $(ANI_LIB_NAME)"             >> $@
	$(A)echo "JVM_ANIX_LIB_NAME        = $(ANIX_LIB_NAME)"            >> $@
	$(A)echo "JVM_CC                   = $(CC)"                       >> $@
	$(A)echo "JVM_CPP                  = $(CPP)"                      >> $@
	$(A)echo "JVM_ASM                  = $(ASM)"                      >> $@
	$(A)echo "JVM_LINK                 = $(LINK)"                     >> $@
	$(A)echo "JVM_LIBMGR               = $(LIBMGR)"                   >> $@
	$(A)echo "JVM_LINK_OUT_SWITCH1 = $(MAKE_EXPORT_LINK_OUT_SWITCH1)" >> $@
	$(A)echo "JVM_LINK_OUT_SWITCH2 = $(MAKE_EXPORT_LINK_OUT_SWITCH2)" >> $@
	$(A)echo "JVM_EXTRA_LIBS           = $(MAKE_EXPORT_EXTRA_LIBS)"   >> $@
	$(A)echo "JVM_THREAD_LIBS          = $(MAKE_EXPORT_THREAD_LIBS)"  >> $@
	$(A)echo "JVM_ROMIZING             = $(ROMIZING)"                 >> $@
	$(A)echo "JVM_ENABLE_FLOAT         = $(ENABLE_FLOAT)"             >> $@
	$(A)echo "JVM_ENABLE_SOFT_FLOAT    = $(ENABLE_SOFT_FLOAT)"        >> $@
	$(A)echo "JVM_ENABLE_JAVA_DEBUGGER = $(ENABLE_JAVA_DEBUGGER)"     >> $@
	$(A)echo "JVM_ENABLE_MEMORY_PROFILER = $(ENABLE_MEMORY_PROFILER)" >> $@
	$(A)echo "JVM_ENABLE_ISOLATES      = $(ENABLE_ISOLATES)"          >> $@
	$(A)echo "JVM_ENABLE_REFLECTION    = $(ENABLE_REFLECTION)"        >> $@
	$(A)echo "JVM_ENABLE_DYNAMIC_NATIVE_METHODS = $(ENABLE_DYNAMIC_NATIVE_METHODS)" >> $@
	$(A)echo "JVM_ENABLE_METHOD_TRAPS  = $(ENABLE_METHOD_TRAPS)"      >> $@
	$(A)echo "JVM_ENABLE_PROFILER      = $(ENABLE_PROFILER)"          >> $@
	$(A)echo "JVM_PATHSEP              = $(PATHSEP)"                  >> $@
	$(A)echo "JVM_os                   = $(os_family)"                >> $@
	$(A)echo "JVM_arch                 = $(arch)"                     >> $@
	$(A)echo "JVM_host_os              = $(host_os)"                  >> $@
	$(A)echo "JVM_host_arch            = $(host_arch)"                >> $@

#----------------------------------------------------------------------
#
# Rebuilding ROM:
#
# delete-rom is a convenient rule for removing all files that must be
# rebuilt if you change the value of the ROMIZING flag.
#
# This rule may be used together with the rebuild-rom rule in root.make
# to quickly rebuild the VM after changing the ROMIZING flag.
#----------------------------------------------------------------------

ROM_RELATED_FILES = \
	Globals$(OBJ_SUFFIX) \
	Natives$(OBJ_SUFFIX) \
	NativesTable$(OBJ_SUFFIX) \
	ROM$(OBJ_SUFFIX) \
	ROMImage$(OBJ_SUFFIX) \
	StringTable$(OBJ_SUFFIX) \
	SymbolTable$(OBJ_SUFFIX) \
	SystemDictionary$(OBJ_SUFFIX) \
	Universe$(OBJ_SUFFIX) \
	$(GENERATED_ROM_FILE) \
	$(BUILD_PCH)

delete-rom::
	rm -f $(ROM_RELATED_FILES)

#----------------------------------------------------------------------
#
# Binary distribution
#
#----------------------------------------------------------------------

$(DIST_BIN_DIR):
	mkdir -p $@

$(DIST_LIB_DIR):
	mkdir -p $@

$(DIST_INC_DIR):
	mkdir -p $@

$(DIST_BIN_DIR)/$(ROMGEN): $(ROM_GENERATOR)
	$(A)rm -f $@
	$(A)cp $< $@
	$(A)echo installed $@

$(DIST_BIN_DIR)/$(JVM_EXE_NAME): $(JVM_EXE)
	$(A)rm -f $@
	$(A)cp $< $@
	$(A)echo installed $@

$(DIST_BIN_DIR)/$(JVM_MAP_NAME): $(JVM_EXE)
	$(A)rm -f $@
	$(A)if test -f ${JVM_MAP}; then \
	    cp ${JVM_MAP} $@; \
	 else \
	    echo 'map file not available' > $@; \
	 fi
	$(A)echo installed $@

ifeq ($(ENABLE_GENERATE_BINARY_ROM_IMAGE), true)
$(DIST_BIN_DIR)/ROM_binary.bun: $(GEN_DIR)/ROM_binary.bun
	$(A)rm -f $@
	$(A)cp $< $@
	$(A)echo installed $@
endif

$(DIST_BIN_DIR)/NativesTableGen.jar: $(BUILDTOOLS_DIR)/jcc.jar
	$(A)rm -f $@
	$(A)cp $< $@
	$(A)echo installed $@

$(DIST_BIN_DIR)/buildtool.jar: $(BUILDTOOL_JAR)
	$(A)rm -f $@
	$(A)cp $< $@
	$(A)echo installed $@

$(DIST_BIN_DIR)/kdp.jar: $(SHARE_DIR)/bin/kdp.jar
	$(A)rm -f $@
	$(A)cp $< $@
	$(A)$(CHMOD) u+w $@
	$(A)echo installed $@

$(PREVERIFY): $(PREVERIFY_ORIGINAL)
	$(A)rm -f $@
	$(A)cp $< $@
	$(A)$(CHMOD) u+wx $@
	$(A)echo installed $@

$(DIST_LIB_DIR)/ROMSkeleton.cpp: $(WorkSpace)/src/vm/share/ROM/ROMSkeleton.cpp
	$(A)rm -f $@
	$(A)cp $< $@
	$(A)$(CHMOD) u+w $@
	$(A)echo installed $@

$(DIST_LIB_DIR)/cldc_rom.cfg: $(WorkSpace)/src/vm/cldc_rom.cfg
	$(A)rm -f $@
	$(A)cp $< $@
	$(A)$(CHMOD) u+w $@
	$(A)echo installed $@

$(DIST_LIB_DIR)/cldcx_rom.cfg: $(WorkSpace)/src/vm/cldcx_rom.cfg
	$(A)rm -f $@
	$(A)cp $< $@
	$(A)$(CHMOD) u+w $@
	$(A)echo installed $@

$(DIST_LIB_DIR)/cldctest_rom.cfg: $(WorkSpace)/src/vm/cldctest_rom.cfg
	$(A)rm -f $@
	$(A)cp $< $@
	$(A)$(CHMOD) u+w $@
	$(A)echo installed $@

$(DIST_LIB_DIR)/$(JVM_LIB_NAME): $(JVM_LIB)
	$(A)rm -f $@
	$(A)cp $< $@
	$(A)echo installed $@

$(DIST_LIB_DIR)/$(JVMX_LIB_NAME): $(JVMX_LIB)
	$(A)rm -f $@
	$(A)cp $< $@
	$(A)echo installed $@

$(DIST_LIB_DIR)/$(JVMTEST_LIB_NAME): $(JVMTEST_LIB)
	$(A)rm -f $@
	$(A)cp $< $@
	$(A)echo installed $@

$(DIST_LIB_DIR)/$(ANI_LIB_NAME): $(ANI_LIB)
	$(A)rm -f $@
	$(A)cp $< $@
	$(A)echo installed $@

$(DIST_LIB_DIR)/$(ANIX_LIB_NAME): $(ANIX_LIB)
	$(A)rm -f $@
	$(A)cp $< $@
	$(A)echo installed $@

$(DIST_LIB_DIR)/cldc_classes.zip: $(CLDC_ZIP)
	$(A)echo ... removing com/sun/cldc/io/j2me/socket
	$(A)cp -r $(CLASSES) .
	$(A)rm -rf classes/com/sun/cldc/io/j2me/socket
	$(A)rm -rf classes/com/sun/cldchi/test
	$(A)cd classes; $(JAR) cf ../cldc_classes.zip *
	$(A)rm -rf classes
	$(A)mv cldc_classes.zip $@
	$(A)echo installed $@

$(DIST_LIB_DIR)/cldcx_classes.zip: $(CLDC_ZIP) $(DIST_LIB_DIR)/cldc_classes.zip
	$(A)echo ... archiving extra classes
	$(A)cp -r $(CLASSES) ./classesx
	$(A)cd classesx; $(JAR) cf ../cldcx_classes.zip com/sun/cldc/io/j2me/socket
	$(A)rm -rf classesx
	$(A)mv cldcx_classes.zip $@
	$(A)echo installed $@

$(DIST_LIB_DIR)/cldctest_classes.zip: $(CLDC_ZIP) $(DIST_LIB_DIR)/cldc_classes.zip
	$(A)echo ... archiving test classes
	$(A)cp -r $(CLASSES) ./classestest
	$(A)cd classestest; $(JAR) cf ../cldctest_classes.zip com/sun/cldchi/test
	$(A)rm -rf classestest
	$(A)mv cldctest_classes.zip $@
	$(A)echo installed $@

$(DIST_LIB_DIR)/ROMLog.txt: $(GENERATED_ROM_FILE)
	$(A)rm -f $@
	$(A)-cp $(GEN_DIR)/ROMLog.txt $@
	$(A)echo installed $@

$(DIST_INC_DIR)/ani.h: $(WorkSpace)/src/anilib/share/ani.h
	$(A)rm -f $@
	$(A)cp $< $@
	$(A)$(CHMOD) u+w $@
	$(A)echo installed $@

$(DIST_INC_DIR)/sni.h: $(VMSRC_DIR)/share/natives/sni.h
	$(A)rm -f $@
	$(A)cp $< $@
	$(A)$(CHMOD) u+w $@
	$(A)echo installed $@

$(DIST_INC_DIR)/kni.h: $(VMSRC_DIR)/share/natives/kni.h
	$(A)rm -f $@
	$(A)cp $< $@
	$(A)$(CHMOD) u+w $@
	$(A)echo installed $@

$(DIST_INC_DIR)/kni_md.h: $(VMSRC_DIR)/cpu/$(arch)/kni_md.h
	$(A)rm -f $@
	$(A)cp $< $@
	$(A)$(CHMOD) u+w $@
	$(A)echo installed $@

$(DIST_INC_DIR)/jvm.h: $(VMSRC_DIR)/share/runtime/jvm.h
	$(A)rm -f $@
	$(A)cp $< $@
	$(A)$(CHMOD) u+w $@
	$(A)echo installed $@

$(DIST_INC_DIR)/jvmconfig.h: $(GEN_DIR)/jvmconfig.h
	$(A)rm -f $@
	$(A)cp $< $@
	$(A)echo installed $@

$(DIST_INC_DIR)/jvmspi.h: $(VMSRC_DIR)/share/runtime/jvmspi.h
	$(A)rm -f $@
	$(A)cp $< $@
	$(A)$(CHMOD) u+w $@
	$(A)echo installed $@

$(DIST_INC_DIR)/kvmcompat.h: $(VMSRC_DIR)/share/natives/kvmcompat.h
	@rm -f $@
	@cp $< $@
	@$(CHMOD) u+w $@
	@echo installed $@

$(DIST_INC_DIR)/NativesTable.hpp: $(VMSRC_DIR)/share/natives/NativesTable.hpp
	$(A)rm -f $@
	$(A)cp $< $@
	$(A)$(CHMOD) u+w $@
	$(A)echo installed $@

$(DIST_INC_DIR)/ROMImage.hpp: $(VMSRC_DIR)/share/ROM/ROMImage.hpp
	$(A)rm -f $@
	$(A)cp $< $@
	$(A)$(CHMOD) u+w $@
	$(A)echo installed $@

$(DIST_INC_DIR)/JVM_Malloc.hpp: $(VMSRC_DIR)/os/utilities/JVM_Malloc.hpp
	$(A)rm -f $@
	$(A)cp $< $@
	$(A)$(CHMOD) u+w $@
	$(A)echo installed $@

$(DIST_INC_DIR)/rmfsApi.hpp: $(VMSRC_DIR)/os/utilities/rmfsApi.hpp
	$(A)rm -f $@
	$(A)cp $< $@
	$(A)$(CHMOD) u+w $@
	$(A)echo installed $@

#----------------------------------------------------------------------
#
# Rules used by the simulated ROM file system.
#
#----------------------------------------------------------------------
#FSImage.c:
#	$(A)tclsh $(WorkSpace)/internal_misc/make_fs_image.tcl $@ $(FSIMAGE_SRCS)

#----------------------------------------------------------------------
#
# Cleaning
#
#----------------------------------------------------------------------

clean:
	-rm -rf *.obj *.o $(EXTRA_CLEAN)
	-rm -rf $(OUTPUT_FILES)
	$(A)if test "$(GEN_DIR)" != "."; then \
		rm -rf $(GEN_DIR) || true; \
	fi

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

%: %$(OBJ_SUFFIX)
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

FORCE:
