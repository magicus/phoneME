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
# Alternative compiler section for Metrowerks CodeWarrior
# IMPL_NOTE: windows only for now
#
# This file is included by these lines in jvm.make:
#
# ifdef ALT_COMPILER
# include $(SHARE_DIR)/$(ALT_COMPILER)/alt_config.make
# endif
#
# The symbol ALT_COMPILER is defined by the Metrowerks build
# configuration files such as
# $(JVMWorkSpace)/build/win32_i386_cw/win32_i386_cw.cfg
#----------------------------------------------------------------------

ifeq ($(compiler), cw)

ifneq ($(findstring Windows_NT, $(shell uname)), CYGWIN)
    define fixcygpath
    echo $(1)
    endef
else
    define fixcygpath
    /usr/bin/cygpath -w $(1)
    endef
endif


ifneq ($(IsTarget), true)
# If we're building the generators, LIB, INCLUDE probably were set for
# the target, rather than for X86. Let's revert that.

# ifdef X86_LIB
# override LIB     := $(X86_LIB)
# export   LIB
# endif

# ifdef X86_INCLUDE
# override INCLUDE := $(X86_INCLUDE)
# export   INCLUDE
# endif

# ifdef X86_PATH
# override PATH    := $(X86_PATH);$(PATH)
# export   PATH
# endif

endif # IsTarget != true

# IMPL_NOTE: adding "-sym on" here seems to generate relocations for extern
# "C" symbols that mwld does not grok (also, see ASM_FLAGS below)
CPP_DBG_FLAGS_debug     = #-sym on,inlines,fullpath
CPP_DBG_FLAGS_release   = #-sym on
CPP_DBG_FLAGS_product   = #-sym off
CPP_DBG_FLAGS          += $(CPP_DBG_FLAGS_$(BUILD))

CPP_OPT_FLAGS_debug     = -opt off
CPP_OPT_FLAGS_release   = -opt space,level=4,intrinsics,debug
CPP_OPT_FLAGS_product   = -opt space,level=4,intrinsics
CPP_OPT_FLAGS          += $(CPP_OPT_FLAGS_$(BUILD))

CPP_DEF_FLAGS_debug     = -D_DEBUG -DAZZERT
CPP_DEF_FLAGS_release   =
CPP_DEF_FLAGS_product   = -DPRODUCT

CPP_DEF_FLAGS          += -DWIN32 -D_WINDOWS
CPP_DEF_FLAGS          += $(CPP_DEF_FLAGS_$(BUILD))
CPP_DEF_FLAGS          += -warnings iserror,on,nounusedarg \
                          $(SAVE_TEMPS_CFLAGS) \
                          $(ENABLE_CFLAGS) \
                          $(ROMIZING_CFLAGS) \
                          $(BUILD_VERSION_CFLAGS) \
                          $(CPP_DBG_FLAGS)
CPP_FLAGS_EXPORT        = $(CPP_DEF_FLAGS) $(CPP_OPT_FLAGS) \
                          -DREQUIRES_JVMCONFIG_H=1
CPP_FLAGS               = $(CPP_DEF_FLAGS) $(CPP_INCLUDE_DIRS)

# IMPL_NOTE: for MASM, not CW's inline assembler!
# IMPL_NOTE: /Zi produces relocations that mwld does not grok.
ASM_FLAGS               = /nologo /coff /c

BUILD_PCH		= _build_pch_cw.stamp
CPP_USE_PCH		= # nothing

LINK_OPT_FLAGS_debug    = -sym on,inlines,fullpath
LINK_OPT_FLAGS_release  = -sym on
LINK_OPT_FLAGS_product  = -sym off -mergesects -fold all
LINK_OPT_FLAGS          = $(LINK_OPT_FLAGS_$(BUILD))

LINK_PROF_FLAGS_true    =
LINK_PROF_FLAGS         = $(LINK_PROF_FLAGS_$(PROFILING))
LINK_OPT_FLAGS         += $(LINK_PROF_FLAGS)

LINK_FLAGS_EXPORT       = -subsystem console
LINK_FLAGS_EXPORT      += $(LINK_OPT_FLAGS)
LINK_FLAGS             += $(LINK_FLAGS_EXPORT) -lwsock32.lib

LIB_FLAGS_debug         =
LIB_FLAGS_release       =
LIB_FLAGS_product       =
LIB_FLAGS               = $(LIB_FLAGS_$(BUILD))

$(Obj_Files):            $(BUILD_PCH)
InterpreterSkeleton.obj: $(BUILD_PCH)

# IMPL_NOTE: "incls/_precompiled_pch.incl" and "cldcvm.mch" names must match
#      makedep's CWPlatform.java
$(BUILD_PCH): $(Precompiled_Headers)
	@echo generating precompiled header ...
	@echo '#include "incls/_precompiled_pch.incl"' > \
		$(GEN_DIR)/_build_pch_cw.cpp
	@$(CPP) $(CPP_OPT_FLAGS) $(CPP_FLAGS) \
		-precompile cldcvm.mch $(GEN_DIR)/_build_pch_cw.cpp
	@touch $@
	@echo '    done'


ifeq ($(IsRomGen), true)
ifeq ($(ENABLE_C_INTERPRETER), true)
# $(ROM_GENERATOR): $(CLDC_ZIP) $(Obj_Files) Interpreter_c.obj  \
#                                            OopMapsSkeleton.obj
# 	echo "linking ROM generator: $(ROM_GENERATOR)"
# 	$(LINK) $(LINK_FLAGS) /out:$@ \
# 		$(Obj_Files) Interpreter_c.obj OopMapsSkeleton.obj
# 	$(ROM_GENERATOR) +GenerateOopMaps
# 	$(MAKE) OopMaps.obj
# 	$(LINK) $(LINK_FLAGS) /out:$@ \
# 		$(Obj_Files) Interpreter_c.obj OopMaps.obj
# 	echo generated `pwd`/$@
else
$(ROM_GENERATOR): $(BUILD_PCH) $(Obj_Files) \
                  InterpreterSkeleton.obj OopMapsSkeleton.obj
	@$(LINK) -o $@ $(LINK_FLAGS) $(Obj_Files) \
		InterpreterSkeleton.obj OopMapsSkeleton.obj
	@$(ROM_GENERATOR) $(LOOP_GEN_ARG)
	@$(ROM_GENERATOR) +GenerateOopMaps
	@$(MAKE) Interpreter_$(arch).obj
	@$(MAKE) OopMaps.obj
	@$(LINK) -o $@ $(LINK_FLAGS) $(Obj_Files) \
		Interpreter_$(arch).obj OopMaps.obj
	@echo generated `pwd`/$@
endif
endif # IsRomGen == true


ifeq ($(IsLoopGen), true)
ifeq ($(ENABLE_C_INTERPRETER), true)
$(LOOP_GENERATOR):
	@touch $@
	@echo "created dummy loop generator"
else
$(LOOP_GENERATOR): $(BUILD_PCH) $(Obj_Files) \
                   InterpreterSkeleton.obj OopMapsSkeleton.obj
	@$(LINK) -o $@ $(LINK_FLAGS) $(Obj_Files) \
		   InterpreterSkeleton.obj OopMapsSkeleton.obj
	@echo generated `pwd`/$@
endif
endif # IsLoopGen == true


ifeq ($(ENABLE_C_INTERPRETER), true)
# Can't use precompiled headers
Interpreter_c.obj: Interpreter_c.cpp
	@echo " ... $(notdir $<)"
	@$(CPP) $(CPP_OPT_FLAGS) $(CPP_FLAGS) -c `$(call fixcygpath, $<)`

AsmStubs_$(target_arch).obj: $(JVMWorkSpace)/src/vm/cpu/c/AsmStubs_$(target_arch).asm
	@echo "generating CPU stubs $<"
	@$(ASM) $(ASM_FLAGS) /Sc \
		/Fo$@ /FlAsmStubs_$(target_arch).lst \
		`$(call fixcygpath, $<)`
else
ifeq ($(ENABLE_INLINEASM_INTERPRETER), true)
Interpreter_$(arch).obj: Interpreter_$(arch).c
	@echo " ... $(notdir $<)"
	@$(CPP) $(CPP_OPT_FLAGS) $(CPP_FLAGS) \
		-c `$(call fixcygpath, $<)`
else
Interpreter_$(arch).obj: Interpreter_$(arch).asm
	@echo " ... $(notdir $<)"
	@$(ASM) $(ASM_FLAGS) /Sc \
		/FoInterpreter_$(arch).obj /FlInterpreter_$(arch).lst \
		`$(call fixcygpath, $(THIS_DIR)/$<)`
endif
endif

# Can't use precompiled headers
ROMImage.obj: $(GENERATED_ROM_FILE)
	@echo " ... $(notdir $<)"
	@$(CPP) $(CPP_OPT_FLAGS) $(CPP_FLAGS) -c `$(call fixcygpath, $<)`

# Can't use precompiled headers
NativesTable.obj: $(NATIVES_TABLE)
	@echo " ... $(notdir $<)"
	@$(CPP) $(CPP_OPT_FLAGS) $(CPP_FLAGS) -c `$(call fixcygpath, $<)`


# Generic compilation rule that uses precompiled headers (if requested)
define BUILD_C_TARGET
	@echo " ... $(notdir $<)"
	@$(CPP) $(CPP_OPT_FLAGS) $(CPP_FLAGS) $(CPP_USE_PCH) \
		-c `$(call fixcygpath, $<)`
endef

# Split the object files into:
#     - LIB_OBJS:  the ones that are exported publicly in $(JVM_LIB)
#     - LIBX_OBJS: the ones that are exported privately in $(JVMX_LIB)
#     - LIBTEST_OBJS: the ones that are exported privately in $(JVMTEST_LIB)
#     - EXE_OBJS:  the ones that are used only by $(JVM_EXE)
LIB_OBJS := $(Obj_Files) Interpreter_$(arch).obj OopMaps.obj
LIB_OBJS := $(subst BSDSocket.obj,,$(LIB_OBJS))
LIBX_OBJS+=         BSDSocket.obj
LIB_OBJS := $(subst ReflectNatives.obj,,$(LIB_OBJS))
LIBTEST_OBJS+=      ReflectNatives.obj
LIB_OBJS := $(subst jvmspi.obj,,$(LIB_OBJS))
EXE_OBJS +=         jvmspi.obj
LIB_OBJS := $(subst Main_$(os_family).obj,,$(LIB_OBJS))
EXE_OBJS +=         Main_$(os_family).obj
LIB_OBJS := $(subst NativesTable.obj,,$(LIB_OBJS))
EXE_OBJS +=         NativesTable.obj
LIB_OBJS := $(subst ROMImage.obj,,$(LIB_OBJS))
EXE_OBJS +=         ROMImage.obj
ifeq ($(ENABLE_USE_BINARY_ROM_IMAGE), true)
LIB_OBJS := $(subst ROM.obj,,$(LIB_OBJS))
EXE_OBJS +=         ROM.obj
endif

$(JVM_LIB): $(BIN_DIR) $(BUILD_PCH) $(LIB_OBJS)
	@$(LIBMGR) -o $@ $(LIB_FLAGS) $(LIB_OBJS)
	@echo generated `pwd`/$@

$(JVMX_LIB): $(BIN_DIR) $(BUILD_PCH) $(LIBX_OBJS)
	@$(LIBMGR) -o $@ $(LIB_FLAGS) $(LIBX_OBJS)
	@echo generated `pwd`/$@

$(JVMTEST_LIB): $(BIN_DIR) $(BUILD_PCH) $(LIBTEST_OBJS)
	@$(LIBMGR) -o $@ $(LIB_FLAGS) $(LIBTEST_OBJS)
	@echo generated `pwd`/$@

$(JVM_EXE): $(BIN_DIR) $(BUILD_PCH) $(JVMX_LIB) $(JVMTEST_LIB) $(JVM_LIB) $(EXE_OBJS)
	@$(LINK) -o $@ $(LINK_FLAGS) $(EXE_OBJS) $(JVMX_LIB) $(JVMTEST_LIB) $(JVM_LIB)
	@echo generated `pwd`/$@


ANI_OBJS  = ani.obj os_port.obj poolthread.obj
ANIX_OBJS = ani_bsd_socket.obj

$(ANI_LIB): $(JVM_LIB) $(ANI_OBJS)
	@$(LIBMGR) -o $@ $(LIB_FLAGS) $(ANI_OBJS)
	@echo generated `pwd`/$@

$(ANIX_LIB): $(JVM_LIB) $(ANIX_OBJS)
	@$(LIBMGR) -o $@ $(LIB_FLAGS) $(ANIX_OBJS)
	@echo generated `pwd`/$@


$(DIST_LIB_DIR)/$(JVM_LOG_NAME)::
	@rm -f $@
	@echo TBD > $@

endif # compiler == "cw"
