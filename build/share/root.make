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
# root.make --
#
#     This file is the center of the build process. It is included by
#     each of the platform Makefiles (i.e.,
#     ${JVMWorkSpace}/build/${PLATFORM}/Makefile).
#
#     The rules in this file controls the creation of the following
#     subdirectories:
#         tools            - platform independent build tools
#                            (mostlt Java programs)
#         romgen           - ROM generator: a VM that runs on
#                            the host computer
#         loopgen          - Loop generator: a VM that runs on
#                            the host computer
#         target           - VM for the target device.
#
#----------------------------------------------------------------------

BUILD_DIR_NAME = $(notdir $(shell pwd))

# Some implementations of Win32 sh.exe capitalize environment variable names.
ifndef JVMWorkSpace
ifdef JVMWORKSPACE
export JVMWorkSpace := $(JVMWORKSPACE)
endif
endif

ifndef JVMBuildSpace
ifdef JVMBUILDSPACE
export JVMBuildSpace := $(JVMBUILDSPACE)
endif
endif

# If JVMBuildSpace is not defined in the environment, auto-assign it. Please
# note for IDE build (the project target), JVMBuildSpace must be explicitly
# defined.
ifndef JVMBuildSpace
JVMBuildSpace = $(JVMWorkSpace)/build
BuildSpaceNotSet = true
endif

ifndef ROMIZING
ROMIZING      = true
endif

# used to be verbose on what make is really doing
# duplicated in jvm.make 
ifndef VERBOSE_BUILD
A = @
else
A = 
endif

THISDIR    = $(JVMWorkSpace)/build/$(BUILD_DIR_NAME)
SHAREDIR   = $(JVMWorkSpace)/build/share
OUTDIR     = $(JVMBuildSpace)/$(BUILD_DIR_NAME)
GEN_DIR    = ../generated
CLASSES    = $(JVMBuildSpace)/classes
CLDC_ZIP   = $(JVMBuildSpace)/classes.zip

ifeq ($(ENABLE_CLDC_111), true)
CLDC_LIB   = cldc1.1.1
else
ifeq ($(ENABLE_REFLECTION),true)
CLDC_LIB   = cldc1.1plus
else
ifneq ($(ENABLE_CLDC_11), false)
CLDC_LIB   = cldc1.1
else
CLDC_LIB   = cldc1.0
endif
endif
endif

# By default the Romizer runs in debug mode, which runs more slowly
# but performs more checks to ensure a healthy ROM image. If speed of
# romization truely becomes an issue, you can set
# USE_DEBUG_ROMGEN=false in your environment variables.
ifneq ($(USE_DEBUG_ROMGEN), false)
    ifneq ($(USE_DEBUG_ROMGEN_SYMBOLS), true)
    ROMGEN_MAKE_ARG = NO_DEBUG_SYMBOLS=true
    endif
    ROMGEN_BUILD=debug
else
    ROMGEN_BUILD=release
endif

# By default the loopgen runs in debug mode to perform sanity checks,
# but is built without C++ debug symbols to reduce the size of the binaries.
ifneq ($(USE_DEBUG_LOOPGEN), false)
    ifneq ($(USE_DEBUG_LOOPGEN_SYMBOLS), true)
    LOOPGEN_MAKE_ARG = NO_DEBUG_SYMBOLS=true
    endif
    LOOPGEN_BUILD=debug
else
    LOOPGEN_BUILD=release
endif

ifeq ($(ENABLE_ISOLATES),true)
PRUNE_ISOLATE= -name Dummy.java -prune -o
else
PRUNE_ISOLATE= -name isolate -prune -o -name Reflect.java -prune -o
endif

ifeq ($(ENABLE_METHOD_TRAPS),true)
PRUNE_METHOD_TRAPS=
else
PRUNE_METHOD_TRAPS= -name MethodTrap.java -prune -o
endif

ifneq ($(ENABLE_SEMAPHORE), false)
PRUNE_SEMAPHORE=
else
PRUNE_SEMAPHORE= -name Semaphore.java -prune -o
endif

ifneq ($(ENABLE_JAVA_DEBUGGER), true)
PRUNE_DEBUGGER= -name DebuggerInvoke.java -prune -o
endif

JAVA_FILES=$(shell echo >&2 Finding .java files ...; \
  find \
  $(JVMWorkSpace)/src/javaapi/$(CLDC_LIB) \
  $(JVMWorkSpace)/src/javaapi/share       \
     $(PRUNE_ISOLATE)                     \
     $(PRUNE_METHOD_TRAPS)                \
     $(PRUNE_SEMAPHORE)                   \
     $(PRUNE_DEBUGGER)                    \
     -name SCCS -prune -o                 \
     -name j2se -prune -o                 \
     -name '*.java' -print;               \
  echo >&2 '   done')

all: $(OUTDIR) $(CLASSES)
all: debug release product

$(OUTDIR):
	$A if test ! -d $(OUTDIR); then \
	    mkdir -p $(OUTDIR); \
	    $(MAKE) copy-root-makefiles; \
	fi

copy-root-makefiles:
	$(A)cat Makefile | grep -v include > $(OUTDIR)/Makefile
	$(A)echo 'include $(SHAREDIR)/root.make'    >> $(OUTDIR)/Makefile
	$(A)if test "$(PROJECT_MAKE_INCLUDED)" = "true"; then \
	 echo 'include $(SHAREDIR)/project.make' >> $(OUTDIR)/Makefile; \
	fi
	$(A)cat $(BUILD_DIR_NAME).cfg > $(OUTDIR)/$(BUILD_DIR_NAME).cfg
	$(A)echo ... copied root Makefiles

$(CLASSES):
	$(A)mkdir -p $(CLASSES)

tools: sanity print_config $(OUTDIR)/tools/Makefile FORCE
	$(A)echo "=============================="
	$(A)echo "creating build tools..."
	$(A)echo "=============================="
	$(A)$(MAKE) -C $(OUTDIR)/tools
	$(A)echo "done"

_romgen:  $(OUTDIR)/romgen/app/Makefile FORCE
	$(A)echo "=============================="
	$(A)echo "creating ROM generator in ${ROMGEN_BUILD} mode"
	$(A)echo "build flags: ${ROMGEN_MAKE_ARG}"
	$(A)echo "=============================="
	$(A)$(MAKE) $(ROMGEN_MAKE_ARG) -C $(OUTDIR)/romgen/app dependencies
	$(A)$(MAKE) $(ROMGEN_MAKE_ARG) -C $(OUTDIR)/romgen/app ${PARALLEL_ARGS}
	$(A)echo "done"

romgen: sanity print_config tools loopgen _romgen

_loopgen: $(OUTDIR)/loopgen/app/Makefile FORCE
	$(A)echo "=============================="
	$(A)echo "creating ASM loop generator in ${LOOPGEN_BUILD} mode"
	$(A)echo "build flags: ${LOOPGEN_MAKE_ARG}"
	$(A)echo "=============================="
	$(A)$(MAKE) $(LOOPGEN_MAKE_ARG) -C $(OUTDIR)/loopgen/app dependencies
	$(A)$(MAKE) $(LOOPGEN_MAKE_ARG) -C $(OUTDIR)/loopgen/app \
		${PARALLEL_ARGS}
	$(A)echo "done"

loopgen: sanity print_config tools _loopgen

dox: $(OUTDIR)/dox/Makefile FORCE
	$(A)echo "=============================="
	$(A)echo "creating documentation..."
	$(A)echo "=============================="
	$(A)$(MAKE) -C $(OUTDIR)/dox
	$(A)echo "done"

_tests: sanity $(OUTDIR)/tests/Makefile FORCE
	$(A)echo "=============================="
	$(A)echo "creating unit tests..."
	$(A)echo "=============================="
	$(A)$(MAKE) -C $(OUTDIR)/tests tests.jar

_romtestvm: sanity $(OUTDIR)/tests/Makefile FORCE
	$(A)$(MAKE) -C $(OUTDIR)/tests romtestvm

_romtestvm_r: sanity $(OUTDIR)/tests/Makefile FORCE
	$(A)$(MAKE) -C $(OUTDIR)/tests romtestvm_r

_romtestvm_g: sanity $(OUTDIR)/tests/Makefile FORCE
	$(A)$(MAKE) -C $(OUTDIR)/tests romtestvm_g

tests: debug release product _tests
	$(A)$(MAKE) -C $(OUTDIR)/tests all
	$(A)echo "done"

# Hints: if you want a faster build, try this (with caution):
#
#	gnumake _debug
#
# This will build ONLY inside the target/debug directory. It assumes
# that your loopgen and romgen directories are up-to-date.
#
# Whenever in doubt that your build is not completely up do date, do
#
#	gnumake clean all

debug:   romgen
release: romgen
product: romgen

debug:   romgen tools loopgen _debug
release: romgen tools loopgen _release
product: romgen tools loopgen _product
project: tools _project

_debug: sanity print_config $(OUTDIR)/target/debug/Makefile FORCE
	$(A)echo "=============================="
	$(A)echo "starting DEBUG build"
	$(A)echo "=============================="
	$(A)$(MAKE) -C $(OUTDIR)/target/debug dependencies
	$(A)$(MAKE) -C $(OUTDIR)/target/debug ${PARALLEL_ARGS}
	$(A)echo "done"

_release: sanity print_config $(OUTDIR)/target/release/Makefile FORCE
	$(A)echo "=============================="
	$(A)echo "starting RELEASE build"
	$(A)echo "=============================="
	$(A)$(MAKE) -C $(OUTDIR)/target/release dependencies
	$(A)$(MAKE) -C $(OUTDIR)/target/release ${PARALLEL_ARGS}
	$(A)echo "done"

_product: sanity print_config $(OUTDIR)/target/product/Makefile FORCE
	$(A)echo "=============================="
	$(A)echo "starting PRODUCT build"
	$(A)echo "=============================="
	$(A)$(MAKE) -C $(OUTDIR)/target/product dependencies
	$(A)$(MAKE) -C $(OUTDIR)/target/product ${PARALLEL_ARGS}
	$(A)echo "done"

_project: sanity print_config $(OUTDIR)/project/Makefile FORCE
	$(A)if test "$(BuildSpaceNotSet)" = "true"; then \
	    echo Please set JVMBuildSpace environment variable explicitly; \
	    exit 1; \
	fi
	$(A)echo "=============================="
	$(A)echo "starting project file build"
	$(A)echo "=============================="
	$(A)$(MAKE) -C $(OUTDIR)/project
	$(A)echo "done"

$(OUTDIR)/dox/Makefile:
	$(A)if test ! -d $(OUTDIR)/dox; then \
		mkdir -p $(OUTDIR)/dox; \
	fi
	$(A)rm -f $@
	$(A)echo '# This file is auto-generated. Do not edit' >> $@
	$(A)echo 'WorkSpace=$(JVMWorkSpace)'                  >> $@
	$(A)echo 'BuildSpace=$(JVMBuildSpace)'                >> $@
	$(A)echo 'BUILD_DIR_NAME=$(BUILD_DIR_NAME)'           >> $@
	$(A)echo 'THIS_DIR=$(OUTDIR)/dox'                     >> $@
	$(A)echo ''                                           >> $@
	$(A)echo 'default:: all'                              >> $@
	$(A)echo ''                                           >> $@
	$(A)echo 'include $(THISDIR)/$(BUILD_DIR_NAME).cfg'   >> $@
	$(A)echo 'include $(SHAREDIR)/dox.make'               >> $@
	$(A)echo created $@

$(OUTDIR)/tests/Makefile: tools
	$(A)if test ! -d $(OUTDIR)/tests; then \
		mkdir -p $(OUTDIR)/tests; \
	fi
	$(A)$(JDK_DIR)/bin/java -jar $(OUTDIR)/tools/buildtool.jar testcases \
		$(JVMWorkSpace) $(OUTDIR)/tests
	$(A)rm -f $@
	$(A)echo '# This file is auto-generated. Do not edit' >> $@
	$(A)echo 'WorkSpace=$(JVMWorkSpace)'                  >> $@
	$(A)echo 'BuildSpace=$(JVMBuildSpace)'                >> $@
	$(A)echo 'IsTarget=true'                              >> $@
	$(A)echo 'BUILD_DIR_NAME=$(BUILD_DIR_NAME)'           >> $@
	$(A)echo 'THIS_DIR=$(OUTDIR)/tests'                   >> $@
	$(A)echo ''                                           >> $@
	$(A)echo 'default:: all'                              >> $@
	$(A)echo ''                                           >> $@
	$(A)echo 'include $(THISDIR)/$(BUILD_DIR_NAME).cfg'   >> $@
	$(A)echo 'include $(OUTDIR)/tests/testcases.make'     >> $@
	$(A)echo 'include $(SHAREDIR)/tests.make'             >> $@
	$(A)echo created $@

$(OUTDIR)/tools/Makefile:
	$(A)if test ! -d $(OUTDIR)/tools; then \
		mkdir -p $(OUTDIR)/tools; \
	fi
	$(A)rm -f $@
	$(A)echo '# This file is auto-generated. Do not edit' >> $@
	$(A)echo 'WorkSpace=$(JVMWorkSpace)'                  >> $@
	$(A)echo 'BuildSpace=$(JVMBuildSpace)'                >> $@
	$(A)echo 'BUILD_DIR_NAME=$(BUILD_DIR_NAME)'           >> $@
	$(A)echo 'THIS_DIR=$(OUTDIR)/tools'                   >> $@
	$(A)echo ''                                           >> $@
	$(A)echo 'default:: all'                              >> $@
	$(A)echo ''                                           >> $@
	$(A)echo 'include $(THISDIR)/$(BUILD_DIR_NAME).cfg'   >> $@
	$(A)echo 'include $(SHAREDIR)/tools.make'             >> $@
	$(A)echo created $@

$(OUTDIR)/romgen/app/Makefile:
	$(A)if test ! -d $(OUTDIR)/romgen/app; then \
		mkdir -p $(OUTDIR)/romgen/app; \
	fi
	$(A)rm -f $@
	$(A)echo '# This file is auto-generated. Do not edit' >> $@
	$(A)echo 'WorkSpace=$(JVMWorkSpace)'                  >> $@
	$(A)echo 'BuildSpace=$(JVMBuildSpace)'                >> $@
	$(A)echo 'BUILD_DIR_NAME=$(BUILD_DIR_NAME)'           >> $@
	$(A)echo 'THIS_DIR=$(OUTDIR)/romgen/app'              >> $@
	$(A)echo 'GEN_DIR=$(GEN_DIR)'                         >> $@
	$(A)echo 'IsSelfHosted=$(IsSelfHosted)'               >> $@
	$(A)echo 'IsGenerator=true'                           >> $@
	$(A)echo 'IsRomGen=true'                              >> $@
	$(A)echo 'BUILD=$(ROMGEN_BUILD)'                      >> $@
	$(A)echo 'CLDC_LIB=$(CLDC_LIB)'                       >> $@
	$(A)echo 'JAVA_FILES=$(JAVA_FILES)'                   >> $@
	$(A)echo ''                                           >> $@
	$(A)echo 'default:: all'                              >> $@
	$(A)echo ''                                           >> $@
	$(A)echo 'include $(THISDIR)/$(BUILD_DIR_NAME).cfg'   >> $@
	$(A)echo '-include $(GEN_DIR)/Dependencies'           >> $@
	$(A)echo 'include $(SHAREDIR)/jvm.make'               >> $@
	$(A)echo created $@

$(OUTDIR)/loopgen/app/Makefile:
	$(A)if test ! -d $(OUTDIR)/loopgen/app; then \
		mkdir -p $(OUTDIR)/loopgen/app; \
	fi
	$(A)rm -f $@
	$(A)echo '# This file is auto-generated. Do not edit' >> $@
	$(A)echo 'WorkSpace=$(JVMWorkSpace)'                  >> $@
	$(A)echo 'BUILD_DIR_NAME=$(BUILD_DIR_NAME)'           >> $@
	$(A)echo 'THIS_DIR=$(OUTDIR)/loopgen/app'             >> $@
	$(A)echo 'GEN_DIR=$(GEN_DIR)'                         >> $@
	$(A)echo 'BuildSpace=$(JVMBuildSpace)'                >> $@
	$(A)echo 'IsSelfHosted=$(IsSelfHosted)'               >> $@
	$(A)echo 'IsGenerator=true'                           >> $@
	$(A)echo 'IsLoopGen=true'                             >> $@
	$(A)echo 'BUILD=${LOOPGEN_BUILD}'                     >> $@
	$(A)echo 'CLDC_LIB=$(CLDC_LIB)'                       >> $@
	$(A)echo 'JAVA_FILES=$(JAVA_FILES)'                   >> $@
	$(A)echo ''                                           >> $@
	$(A)echo 'default:: all'                              >> $@
	$(A)echo ''                                           >> $@
	$(A)echo 'include $(THISDIR)/$(BUILD_DIR_NAME).cfg'   >> $@
	$(A)echo '-include $(GEN_DIR)/Dependencies'           >> $@
	$(A)echo 'include $(SHAREDIR)/jvm.make'               >> $@
	$(A)echo created $@

$(OUTDIR)/target/debug/Makefile:
	$(A)if test ! -d $(OUTDIR)/target/debug; then \
		mkdir -p $(OUTDIR)/target/debug; \
	fi
	$(A)rm -f $@
	$(A)echo '# This file is auto-generated. Do not edit' >> $@
	$(A)echo 'WorkSpace=$(JVMWorkSpace)'                  >> $@
	$(A)echo 'BuildSpace=$(JVMBuildSpace)'                >> $@
	$(A)echo 'BUILD_DIR_NAME=$(BUILD_DIR_NAME)'           >> $@
	$(A)echo 'THIS_DIR=$(OUTDIR)/target/debug'            >> $@
	$(A)echo 'GEN_DIR=$(GEN_DIR)'                         >> $@
	$(A)echo 'IsSelfHosted=$(IsSelfHosted)'               >> $@
	$(A)echo 'IsTarget=true'                              >> $@
	$(A)echo 'BUILD=debug'                                >> $@
	$(A)echo 'CLDC_LIB=$(CLDC_LIB)'                       >> $@
	$(A)echo 'JAVA_FILES=$(JAVA_FILES)'                   >> $@
	$(A)echo ''                                           >> $@
	$(A)echo 'default:: all'                              >> $@
	$(A)echo ''                                           >> $@
	$(A)echo 'include $(THISDIR)/$(BUILD_DIR_NAME).cfg'   >> $@
	$(A)echo '-include $(GEN_DIR)/Dependencies'           >> $@
	$(A)echo 'include $(SHAREDIR)/jvm.make'               >> $@
	$(A)echo created $@

$(OUTDIR)/target/release/Makefile:
	$(A)if test ! -d $(OUTDIR)/target/release; then \
		mkdir -p $(OUTDIR)/target/release; \
	fi
	$(A)rm -f $@
	$(A)echo '# This file is auto-generated. Do not edit' >> $@
	$(A)echo 'WorkSpace=$(JVMWorkSpace)'                  >> $@
	$(A)echo 'BuildSpace=$(JVMBuildSpace)'                >> $@
	$(A)echo 'BUILD_DIR_NAME=$(BUILD_DIR_NAME)'           >> $@
	$(A)echo 'THIS_DIR=$(OUTDIR)/target/release'          >> $@
	$(A)echo 'GEN_DIR=$(GEN_DIR)'                         >> $@
	$(A)echo 'IsSelfHosted=$(IsSelfHosted)'               >> $@
	$(A)echo 'IsTarget=true'                              >> $@
	$(A)echo 'BUILD=release'                              >> $@
	$(A)echo 'CLDC_LIB=$(CLDC_LIB)'                       >> $@
	$(A)echo 'JAVA_FILES=$(JAVA_FILES)'                   >> $@
	$(A)echo ''                                           >> $@
	$(A)echo 'default:: all'                              >> $@
	$(A)echo ''                                           >> $@
	$(A)echo 'include $(THISDIR)/$(BUILD_DIR_NAME).cfg'   >> $@
	$(A)echo '-include $(GEN_DIR)/Dependencies'           >> $@
	$(A)echo 'include $(SHAREDIR)/jvm.make'               >> $@
	$(A)echo created $@

$(OUTDIR)/target/product/Makefile:
	$(A)if test ! -d $(OUTDIR)/target/product; then \
		mkdir -p $(OUTDIR)/target/product; \
	fi
	$(A)rm -f $@
	$(A)echo '# This file is auto-generated. Do not edit' >> $@
	$(A)echo 'WorkSpace=$(JVMWorkSpace)'                  >> $@
	$(A)echo 'BuildSpace=$(JVMBuildSpace)'                >> $@
	$(A)echo 'BUILD_DIR_NAME=$(BUILD_DIR_NAME)'           >> $@
	$(A)echo 'THIS_DIR=$(OUTDIR)/target/product'          >> $@
	$(A)echo 'GEN_DIR=$(GEN_DIR)'                         >> $@
	$(A)echo 'IsSelfHosted=$(IsSelfHosted)'               >> $@
	$(A)echo 'IsTarget=true'                              >> $@
	$(A)echo 'BUILD=product'                              >> $@
	$(A)echo 'CLDC_LIB=$(CLDC_LIB)'                       >> $@
	$(A)echo 'JAVA_FILES=$(JAVA_FILES)'                   >> $@
	$(A)echo ''                                           >> $@
	$(A)echo 'default:: all'                              >> $@
	$(A)echo ''                                           >> $@
	$(A)echo 'include $(THISDIR)/$(BUILD_DIR_NAME).cfg'   >> $@
	$(A)echo '-include $(GEN_DIR)/Dependencies'           >> $@
	$(A)echo 'include $(SHAREDIR)/jvm.make'               >> $@
	$(A)echo created $@

rebuild-rom: FORCE print_config \
		$(OUTDIR)/target/debug/Makefile   \
		$(OUTDIR)/target/release/Makefile \
		$(OUTDIR)/target/product/Makefile
	$(MAKE) -s -C $(OUTDIR)/target/debug delete-rom
	$(MAKE) -s -C $(OUTDIR)/target/release delete-rom
	$(MAKE) -s -C $(OUTDIR)/target/product delete-rom
	$(MAKE) debug release product

$(OUTDIR)/project/Makefile:
	$(A)if test ! -d $(OUTDIR)/project; then \
		mkdir -p $(OUTDIR)/project; \
	fi
	$(A)rm -f $@
	$(A)echo '# This file is auto-generated. Do not edit' >> $@
	$(A)echo 'JVMWorkSpace=$(JVMWorkSpace)'               >> $@
	$(A)echo 'JVMBuildSpace=$(JVMBuildSpace)'             >> $@
	$(A)echo 'WorkSpace=$(JVMWorkSpace)'                  >> $@
	$(A)echo 'BuildSpace=$(JVMBuildSpace)'                >> $@
	$(A)echo 'BUILD_DIR_NAME=$(BUILD_DIR_NAME)'           >> $@
	$(A)echo 'GEN_DIR=.'                                  >> $@
	$(A)echo 'IsProject=true'                             >> $@
	$(A)echo 'BUILD=debug'                                >> $@
	$(A)echo ''                                           >> $@
	$(A)echo 'include $(THISDIR)/$(BUILD_DIR_NAME).cfg'   >> $@
	$(A)echo ''                                           >> $@
	$(A)echo 'default:: all'                              >> $@
	$(A)echo ''                                           >> $@
	$(A)echo 'include $(SHAREDIR)/jvm.make'               >> $@
	$(A)echo 'include $(SHAREDIR)/project.make'           >> $@
	$(A)echo created $@

FORCE:

_clean:	print_config
	$(A)echo "cleaning up target only..."
	$(A)-rm -rf $(OUTDIR)/target

clean: print_config
	$(A)echo "cleaning up everything..."
	$(A)-rm -rf $(JVMBuildSpace)/tmpclasses
	$(A)-rm -rf $(CLASSES) $(CLDC_ZIP)
	$(A)-rm -rf $(GEN_DIR)
	$(A)-rm -rf $(OUTDIR)/tools
	$(A)-rm -rf $(OUTDIR)/romgen
	$(A)-rm -rf $(OUTDIR)/ipegen
	$(A)-rm -rf $(OUTDIR)/loopgen
	$(A)-rm -rf $(OUTDIR)/target
	$(A)-rm -rf $(OUTDIR)/dist
	$(A)-rm -rf $(OUTDIR)/project
	$(A)-rm -rf $(OUTDIR)/bin
	$(A)-rm -rf $(OUTDIR)/generated
	$(A)-rm -rf $(OUTDIR)/tests
	$(A)-rm -rf $(OUTDIR)/dox

# Clean all platforms that are syblings to this directory
clean-all-platforms:
	$(A)cd $(JVMBuildSpace); \
	 for i in *; do \
	     if test -f $$i/Makefile; then \
	         echo ================; \
	         echo cleaning $$i;     \
	         echo ================; \
                 $(MAKE) -C $$i clean || true; \
	     fi; \
	 done

print_config:
	$(A)echo '=================================='
	$(A)if test "$(BuildSpaceNotSet)" = "true"; then \
	 echo 'JVMBuildSpace  = $(JVMBuildSpace) (auto-assigned)'; \
	else \
	 echo 'JVMBuildSpace  = $(JVMBuildSpace)'; \
	fi
	$(A)echo 'JVMWorkSpace    = $(JVMWorkSpace)'
	$(A)echo 'ROMIZING        = $(ROMIZING)'
	$(A)echo 'SHELL           = $(SHELL)'
	$(A)echo 'ENABLE_ISOLATES = $(ENABLE_ISOLATES)'
	$(A)echo 'ENABLE_MONET    = $(ENABLE_MONET)'	
	$(A)echo '=================================='

sanity:
	$(A)if test ! -d $(JVMWorkSpace)/src/vm/share; then \
	    echo '==========================================================';\
	    echo 'JVMWorkSpace must be set. See build documentation';\
	    echo 'Note: forward slash / must be used on Win32 and'; \
	    echo '      no slash allowed at the end'; \
	    echo '==========================================================';\
	    exit -1; \
	fi
	$(A)if test ! -f $(JDK_DIR)/jre/lib/rt.jar && test ! -d /System/Library/Java; then \
	    echo '==========================================================';\
	    echo 'JDK_DIR must be set. I.e., set it such that you can access';\
	    echo 'javac as $$(JDK_DIR)/bin/javac'; \
	    echo Note: forward slash / must be used on Win32; \
	    echo '==========================================================';\
	    exit -1; \
	fi

ifndef NUM_JOBS
NUM_JOBS = 3
endif
parallel:
	$(A)echo start parallel build with NUM_JOBS=$(NUM_JOBS)
	$(MAKE) tools
	$(MAKE) PARALLEL_ARGS="-j $(NUM_JOBS)" tools
	$(MAKE) PARALLEL_ARGS="-j $(NUM_JOBS)" romgen
	$(MAKE) PARALLEL_ARGS="-j $(NUM_JOBS)" loopgen
	$(MAKE) PARALLEL_ARGS="-j $(NUM_JOBS)" _debug
	$(MAKE) PARALLEL_ARGS="-j $(NUM_JOBS)" _release
	$(MAKE) PARALLEL_ARGS="-j $(NUM_JOBS)" _product
	$(A)echo done parallel build with NUM_JOBS=$(NUM_JOBS)

# Build romgen and loopgen in parallel, but build the targets in
# with single process. Use this if you have a limited number of ADS
# compiler licenses.
parallel_ads:
	$(A)echo start parallel build with NUM_JOBS=$(NUM_JOBS)
	$(MAKE) tools
	$(MAKE) PARALLEL_ARGS="-j $(NUM_JOBS)" tools
	$(MAKE) PARALLEL_ARGS="-j $(NUM_JOBS)" romgen
	$(MAKE) PARALLEL_ARGS="-j $(NUM_JOBS)" loopgen
	$(MAKE) _debug
	$(MAKE) _release
	$(MAKE) _product
	$(A)echo done parallel build with NUM_JOBS=$(NUM_JOBS)

parallel_debug parallel_release parallel_product:
	$(A)echo start parallel build with NUM_JOBS=$(NUM_JOBS)
	$(MAKE) tools
	$(MAKE) PARALLEL_ARGS="-j $(NUM_JOBS)" tools
	$(MAKE) PARALLEL_ARGS="-j $(NUM_JOBS)" romgen
	$(MAKE) PARALLEL_ARGS="-j $(NUM_JOBS)" loopgen
	$(MAKE) PARALLEL_ARGS="-j $(NUM_JOBS)" $(@:parallel%=%)
	$(A)echo done parallel build with NUM_JOBS=$(NUM_JOBS)

parallel_romgen:
	$(A)echo start parallel build with NUM_JOBS=$(NUM_JOBS)
	$(MAKE) tools
	$(MAKE) PARALLEL_ARGS="-j $(NUM_JOBS)" tools
	$(MAKE) PARALLEL_ARGS="-j $(NUM_JOBS)" romgen
	$(A)echo done parallel build with NUM_JOBS=$(NUM_JOBS)

parallel_loopgen:
	$(A)echo start parallel build with NUM_JOBS=$(NUM_JOBS)
	$(MAKE) tools
	$(MAKE) PARALLEL_ARGS="-j $(NUM_JOBS)" tools
	$(MAKE) PARALLEL_ARGS="-j $(NUM_JOBS)" loopgen
	$(A)echo done parallel build with NUM_JOBS=$(NUM_JOBS)

parallel__debug parallel__release parallel__product:
	$(A)echo start parallel build with NUM_JOBS=$(NUM_JOBS)
	$(MAKE) PARALLEL_ARGS="-j $(NUM_JOBS)" $(@:parallel_%=%)
	$(A)echo done parallel build with NUM_JOBS=$(NUM_JOBS)

test-romize:
	$(A)$(MAKE) -C $(OUTDIR)/romgen/app test-romize

test-loopgen:
	$(A)$(OUTDIR)/loopgen/app/loopgen -generate -generateoptimized

test-loopgen-gnu:
	$(A)$(OUTDIR)/loopgen/app/loopgen -generate -generateoptimized \
		+GenerateGNUCode

help:
	$(A)echo "Help placeholder"

#----------------------------------------------------------------------
#
# function and target for source bundle
#
#----------------------------------------------------------------------
include ../share/release.make

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
