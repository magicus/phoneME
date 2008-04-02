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
# Alternative compiler section for ADS (ARM Developer's	Suite 1.1, 1.2)
#
# This file is included	by these lines in jvm.make:
#
# ifdef	ALT_COMPILER
# include $(SHARE_DIR)/$(ALT_COMPILER)/alt_config.make
# endif
#
# The symbol ALT_COMPILER is defined by	the ADS	build
# configuration	file $(JVMWorkSpace)/build/ads_arm/ads_arm.config
#----------------------------------------------------------------------

ifeq ($(compiler), ads)

ifneq ($(NO_DEBUG_SYMBOLS), true)
DEBUG_SYMBOLS_FLAGS	= -g -gtp
else
DEBUG_SYMBOLS_FLAGS	= -O2 -Otime
endif

# By default, the default FPU option is	used (on ADS 1.2 this would
# be "-fpu softvfp".) You can override it for platforms	with hardware
# FPU support:
# If you're using ENABLE_THUMB_VM=false, try
#     JVM_FPU_FLAGS = -fpu vfp
# If you're using ENABLE_THUMB_VM=true,	try
#     JVM_FPU_FLAGS = -fpu softvfp+vfp

ifndef JVM_FPU_FLAGS
JVM_FPU_FLAGS		=
endif

ifeq ($(ENABLE_PCSL), true)
PCSL_DIST_DIR           = $(PCSL_OUTPUT_DIR)/armsd_arm

CPP_INCLUDE_DIRS       += -I"$(PCSL_DIST_DIR)/inc"

PCSL_LIBS               = $(PCSL_DIST_DIR)/lib/libpcsl_memory.lib    \
                          $(PCSL_DIST_DIR)/lib/libpcsl_print.lib     \
                          $(PCSL_DIST_DIR)/lib/libpcsl_network.lib   \
                          $(PCSL_DIST_DIR)/lib/libpcsl_string.lib    \
                          $(PCSL_DIST_DIR)/lib/libpcsl_file.lib
LINK_FLAGS             += $(PCSL_LIBS)
MAKE_EXPORT_EXTRA_LIBS += $(PCSL_LIBS)
endif

CPP_OPT_FLAGS		= 

CPP_OPT_FLAGS_debug	= $(DEBUG_SYMBOLS_FLAGS)
CPP_OPT_FLAGS_release	= -O2 -Otime
CPP_OPT_FLAGS_product	= -O2 -Otime
CPP_OPT_FLAGS	       += $(CPP_OPT_FLAGS_$(BUILD))

CPP_DEF_FLAGS_debug	= -D_DEBUG -DAZZERT
CPP_DEF_FLAGS_release	=
CPP_DEF_FLAGS_product	= -DPRODUCT

ifndef TARGET_CPU
TARGET_CPU = ARM10200E
endif

ifeq ($(ENABLE_RVDS), true)
    #
    # RealView Developer Suite (version	2.x)
    #
    EXTRA_DASH		= -
    ASM_FLAGS		+= --checkreglist --32 
    ASM_FLAGS           += --cpu $(TARGET_CPU)
    LINKER_OUTPUT	= -o

    # Temp: supress #111-D: statement is unreachable
    CPP_DEF_FLAGS	+= --diag_suppress 111

    # Temp: supress #815-D:  type qualifier on return type is meaningless
    CPP_DEF_FLAGS	+= --diag_suppress 815

    # Temp: supress 191-D: type	qualifier is meaningless on cast type
    CPP_DEF_FLAGS	+= --diag_suppress 191

    # Run-time type info is not needed by the VM. Disable it to save space.
    # CPP_DEF_FLAGS	+= --no_rtti

    # Used in SAVE_TEMPS mode
    INTERLEAVE_LISTING  = --interleave

    ifeq ($(ENABLE_ARM_VFP), true)
    CPP_DEF_FLAGS	+= -DSETUP_UNDEF_STACK \
	                   -DPATCH_UNDEF_VECTOR

    ASM_FLAGS		+= --predefine "SETUP_UNDEF_STACK SETL {TRUE}" 
    ASM_FLAGS		+= --predefine "PATCH_UNDEF_VECTOR SETL {TRUE}"
    ASM_FLAGS           += --fpu VFP
    endif

    ifeq ($(ENABLE_THUMB_VM), true)
    ASM_FLAGS           += --thumb
    endif

else
    #
    # ADS version 1.2
    #
    ASM_FLAGS          += -CheckRegList -32 
    ifeq ($(ENABLE_ARM_VFP), true)
    ASM_FLAGS          += -FPU VFP -CPU ARM10200E
    endif
    LINKER_OUTPUT	= -O

    # enable warning: extended ANSI: unwidened type, etc
    CPP_DEF_FLAGS       += -W+b

    # enable warning: lower precision in wider context
    CPP_DEF_FLAGS       += -W+l

    # Used in SAVE_TEMPS mode
    INTERLEAVE_LISTING  = 
endif

# and add Thumb	interwork if needed
ifeq ($(ENABLE_THUMB_VM), true)
APCS_FLAGS		= -apcs	/interwork
else
    ifeq ($(ENABLE_THUMB_COMPILER), true)
    # IMPL_NOTE: consider if it is necessary.
    APCS_FLAGS		= -apcs	/interwork
    endif
endif

CPP_DEF_FLAGS	       += ${JVM_FPU_FLAGS}
CPP_DEF_FLAGS	       += -DARM=1 -DUNDER_ADS=1	$(APCS_FLAGS)
CPP_DEF_FLAGS	       += $(CPP_DEF_FLAGS_$(BUILD))
CPP_DEF_FLAGS	       += $(BUILD_VERSION_CFLAGS)

# You can use USER_CPP_FLAGS to	pass additional	flags to the C compiler.
# E.g.,	gnumake	USER_CPP_FLAGS="-DMAX_METHOD_TO_COMPILE=400"
ifdef USER_CPP_FLAGS
CPP_DEF_FLAGS	       += $(USER_CPP_FLAGS)
endif

CPP_FLAGS_EXPORT	= $(CPP_DEF_FLAGS) \
			  $(SAVE_TEMPS_CFLAGS) $(ROMIZING_CFLAGS) \
			  $(ENABLE_CFLAGS)

CPP_FLAGS		= $(CPP_OPT_FLAGS) $(CPP_FLAGS_EXPORT) $(CPP_INCLUDE_DIRS)

LINK_OPT_FLAGS_debug	=
LINK_OPT_FLAGS_release	=
LINK_OPT_FLAGS_product	=

LINK_FLAGS	       += $(LINK_OPT_FLAGS_$(BUILD))

ASM_FLAGS_debug	       += -G
ASM_FLAGS_release      += -G
ASM_FLAGS_product      +=
ASM_FLAGS	       += $(ASM_FLAGS_$(BUILD))	$(APCS_FLAGS)

Interpreter_$(arch).o: Interpreter_$(arch).asm
	$(A)echo "ASMing $<"
	$(A)rm -f $@
	$(A)$(ASM) $(ASM_FLAGS)	-o Interpreter_$(arch).o Interpreter_$(arch).asm
	$(A)if test ! -f "$@"; then exit 2; fi

define BUILD_C_TARGET
	$(A)echo " ... $(notdir	$<)"
	$(A)rm -f $@
	$(A)if test "$(SAVE_TEMPS)" = "true"; \
	    then $(CPP)	$(CPP_FLAGS) -S	${INTERLEAVE_LISTING} $<; echo ...; \
	 fi
	$(A)$(CPP) $(CPP_FLAGS)	-c $<
	$(A)if test ! -f "$@"; then exit 2; fi
endef

define BUILD_C_TARGET_ARM_MODE
	$(A)echo " ... $(notdir	$<)"
	$(A)rm -f $@
	$(A)if test "$(SAVE_TEMPS)" = "true"; \
	    then armcpp	$(CPP_FLAGS) -S	${INTERLEAVE_LISTING} $<; echo ...; \
	 fi
	$(A)armcpp $(CPP_FLAGS)	-c $<
	$(A)if test ! -f "$@"; then exit 2; fi
endef

OS_ads.o: $(WorkSpace)/src/vm/os/ads/OS_ads.cpp
	$(BUILD_C_TARGET_ARM_MODE)

HotRoutines1.o:	$(WorkSpace)/src/vm/share/runtime/HotRoutines1.cpp
	$(BUILD_C_TARGET_ARM_MODE)

# See src/vm/os/ads/vfp/README
ifeq ($(ENABLE_RVDS), true)
ifeq ($(ENABLE_ARM_VFP), true)

controlbuffer.o : $(WorkSpace)/src/vm/os/ads/vfp/controlbuffer.c
	$(A)armcc -c $(CPP_DEF_FLAGS) $(JVM_FPU_FLAGS) $<

vfpundef.o : $(WorkSpace)/src/vm/os/ads/vfp/vfpundef.c
	$(A)armcc -c $(CPP_DEF_FLAGS) $(JVM_FPU_FLAGS) $<

sldummy.o: $(WorkSpace)/src/vm/os/ads/vfp/sldummy.s
	$(A)echo "ASMing $<"
	$(A)rm -f $@
	$(A)$(ASM) $(ASM_FLAGS)	$(JVM_FPU_FLAGS) -o $@ $<
	$(A)if test ! -f "$@"; then exit 2; fi

tlundef.o: $(WorkSpace)/src/vm/os/ads/vfp/tlundef.s
	$(A)echo "ASMing $<"
	$(A)rm -f $@
	$(A)$(ASM) $(ASM_FLAGS)	$(JVM_FPU_FLAGS) -o $@ $<
	$(A)if test ! -f "$@"; then exit 2; fi

vfpfptrap.o: $(WorkSpace)/src/vm/os/ads/vfp/vfpfptrap.s
	$(A)echo "ASMing $<"
	$(A)rm -f $@
	$(A)$(ASM) $(ASM_FLAGS)	$(JVM_FPU_FLAGS) -o $@ $<
	$(A)if test ! -f "$@"; then exit 2; fi

vfpsubarch.o: $(WorkSpace)/src/vm/os/ads/vfp/vfpsubarch.s
	$(A)echo "ASMing $<"
	$(A)rm -f $@
	$(A)$(ASM) $(ASM_FLAGS)	$(JVM_FPU_FLAGS) -o $@ $<
	$(A)if test ! -f "$@"; then exit 2; fi

vfpwrapper.o: $(WorkSpace)/src/vm/os/ads/vfp/vfpwrapper.s
	$(A)echo "ASMing $<"
	$(A)rm -f $@
	$(A)$(ASM) $(ASM_FLAGS)	$(JVM_FPU_FLAGS) -o $@ $<
	$(A)if test ! -f "$@"; then exit 2; fi

vfpinit.o: $(WorkSpace)/src/vm/os/ads/vfp/vfpinit.s
	$(A)echo "ASMing $<"
	$(A)rm -f $@
	$(A)$(ASM) $(ASM_FLAGS)	$(JVM_FPU_FLAGS) -o $@ $<
	$(A)if test ! -f "$@"; then exit 2; fi

fpinit.o : $(WorkSpace)/src/vm/os/ads/vfp/fpinit.c
	$(A)armcc -c $(CPP_DEF_FLAGS) $(JVM_FPU_FLAGS) $<

Obj_Files += controlbuffer.o vfpundef.o sldummy.o \
             tlundef.o vfpfptrap.o vfpsubarch.o \
             vfpwrapper.o vfpinit.o fpinit.o

EXE_OBJS += controlbuffer.o vfpundef.o sldummy.o \
             tlundef.o vfpfptrap.o vfpsubarch.o \
             vfpwrapper.o vfpinit.o fpinit.o
             
endif
endif

# Definitions to be exported in	$(DIST_LIB_DIR)/$(JVM_MAK_NAME)
MAKE_EXPORT_LINK_OUT_SWITCH1	 = -O
MAKE_EXPORT_LINK_OUT_SWITCH2	 =
MAKE_EXPORT_EXTRA_LIBS		 =
MAKE_EXPORT_THREAD_LIBS		 =

$(DIST_LIB_DIR)/$(JVM_LOG_NAME)::
	$(A)rm -f $@
	$(A)echo TBD > $@

# Split	the object files into:
#     -	LIB_OBJS: the ones that	are exported
#     -	EXE_OBJS: the ones that	are used only by $(JVM_EXE)
LIB_OBJS := $(Obj_Files) Interpreter_$(arch).o OopMaps.o
LIB_OBJS := $(subst BSDSocket.o,,$(LIB_OBJS))
LIBX_OBJS +=	    BSDSocket.o
LIB_OBJS := $(subst ReflectNatives.o,,$(LIB_OBJS))
LIBTEST_OBJS +=	    ReflectNatives.o
LIB_OBJS := $(subst jvmspi.o,,$(LIB_OBJS))
EXE_OBJS +=	    jvmspi.o
LIB_OBJS := $(subst Main_$(os_family).o,,$(LIB_OBJS))
EXE_OBJS +=	    Main_$(os_family).o
LIB_OBJS := $(subst NativesTable.o,,$(LIB_OBJS))
EXE_OBJS +=	    NativesTable.o
LIB_OBJS := $(subst ROMImage.o,,$(LIB_OBJS))
EXE_OBJS +=	    ROMImage.o

$(JVM_LIB): $(BIN_DIR) $(BUILD_PCH) $(LIB_OBJS)
	$(A)echo "creating $@ ... "
	$(A)rm -f $@
	$(A)$(LIBMGR) cq $@ $(LIB_OBJS)
	$(A)echo generated `pwd`/$@

$(JVMX_LIB): $(BIN_DIR)	$(BUILD_PCH) $(LIBX_OBJS)
	$(A)echo "creating $@ ... "
	$(A)rm -f $@
	$(A)$(LIBMGR) cq $@ $(LIBX_OBJS)
	$(A)echo generated `pwd`/$@

$(JVMTEST_LIB):	$(BIN_DIR) $(BUILD_PCH)	$(LIBTEST_OBJS)
	$(A)echo "creating $@ ... "
	$(A)rm -f $@
	$(A)$(LIBMGR) cq $@ $(LIBTEST_OBJS)
	$(A)echo generated `pwd`/$@

SCATTER_MAP = $(WorkSpace)/src/vm/os/$(os_family)/cldc_vm.load
$(JVM_EXE): $(BIN_DIR) $(CLDC_ZIP) $(EXE_OBJS) $(JVMX_LIB) $(JVMTEST_LIB) \
	    $(JVM_LIB) $(SCATTER_MAP)
ifeq ($(skip_link_image), true)
	$(A)touch $@
else
	$(A)echo "Linking $@..."
	$(A)$(LINK) $(LINKER_OUTPUT) $@ \
	    $(EXE_OBJS) $(JVM_LIB) $(JVMX_LIB) $(JVMTEST_LIB) $(LINK_FLAGS) \
	    $(EXTRA_DASH)-map $(EXTRA_DASH)-symbols \
	    $(EXTRA_DASH)-list $(JVM_MAP) $(EXTRA_DASH)-entry 0x00008000 \
	    $(EXTRA_DASH)-scatter $(SCATTER_MAP)
endif
	$(A)echo generated `pwd`/$@

PRINT_CONFIG = print_config

print_config:
	$(A)echo "=========================================="
	$(A)echo "CPP			   = $(CPP)"
	$(A)echo "ADS_LINUX_HOST	   = $(ADS_LINUX_HOST)"
	$(A)echo "ENABLE_THUMB_VM	   = $(ENABLE_THUMB_VM)"
	$(A)echo "ENABLE_SOFT_FLOAT	   = $(ENABLE_SOFT_FLOAT)"
	$(A)echo "ENABLE_SOURCE_GENERATORS = $(ENABLE_SOURCE_GENERATORS)"
	$(A)echo "=========================================="
endif
