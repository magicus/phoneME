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

#
# Makefile for project PMEA wm5 builds. This makefile will build all
# the supported build flavors. You may override default flags for any
# or all build flavors by adding flags on the gnumake command line.
# For example:
#
#   cd $COMPONENTS_DIR/cdc/build/win32-arm-wm5
#   make -f \
#     $COMPONENTS_DIR/tools/projects/pmea/build/win32-arm-wm5/GNUmakefile \
#     opt_nosym USE_VERBOSE_MAKE=false
#
# Supported targets are:
#	all:            build all build flavors
#	list:           list all build flavors
#	source_bundles: builds the source bundle into $(SOURCE_OUTPUT_DIR)
#	<build_flavor>: build specified build flavor. More than one
#			<build_flavor> target can be specified.
#
# Note you must be in the cdc/build/win32-arm-wm5 device build
# directory before executing this makefile.
#
# Some cdc makefile defaults setup by this GNUmakefile:
#	COMPONENTS_DIR:          ../../.. (relative to device build directory)
#	USE_VERBOSE_MAKE:        true
#	CVM_BUILD_SUBDIR_SUFFIX: ""
#	CVM_BUILD_SUBDIR_NAME:   <build_flavor>$(CVM_BUILD_SUBDIR_SUFFIX)
#	SOURCE_OUTPUT_DIR:	$(COMPONENTS_DIR)/cdc/install/pmea_mr2
#	SUBMAKE_TARGETS:	Targets passed to submake, such as "bin"
#
# You can export CVM_BUILD_SUBDIR_SUFFIX from the shell if you always
# want a suffix. For example, a suffix of "_dir" is useful for easy
# cleanup of all build subdirs.

# All supported build flavors
BUILD_FLAVORS = \
	opt_preload_jit

# Build all the build flavors
.PHONY: all
all: BUILD_LIST=$(BUILD_FLAVORS)
all: print_build_list $(BUILD_FLAVORS)

# List all the build flavors that can be built
.PHONY: list
list:
	@printf "$(patsubst %,\t%\n,$(BUILD_FLAVORS))"

# List all the build flavors that will be built
.PHONY: print_build_list
print_build_list:
	@printf "Building the following build flavor targets:\n"
	@printf "$(patsubst %,\t%\n,$(BUILD_LIST))"

# Macro for CVM_BUILD_SUBDIR_NAME based on rule target name ($@)
# and supplied suffix, if any.
CVM_BUILD_SUBDIR_SUFFIX ?= 
SUBDIR_FLAG = CVM_BUILD_SUBDIR_NAME=$@$(CVM_BUILD_SUBDIR_SUFFIX)

# Setup COMPONENTS_DIR based on current build dir
ABSPATH = $(shell mkdir -p $(1); cd $(1); echo `pwd`)
ifndef COMPONENTS_DIR
COMPONENTS_DIR     := $(call ABSPATH, ../../..)
endif

# Default flags that all builds use. Some builds may override some flags
DEFAULT_FLAGS = \
	CDC_PROJECT=pmea \
	$(SUBDIR_FLAG) \
	J2ME_CLASSLIB=foundation \
	USE_VERBOSE_MAKE=true \
	CVM_SYMBOLS=true \
	CVM_PRELOAD_LIB=true \

MIDP_FLAGS = \
	USE_MIDP=true \
	USE_JPEG=true \
	CVM_INCLUDE_JAVACALL=false \
	PCSL_TARGET=wince_arm \

define TRACE_BUILD_START
	@echo "*********************************************************"
	@echo "Starting build for $@: `date`"
	@echo "*********************************************************"
endef

define TRACE_BUILD_END
	@echo "*********************************************************"
	@echo "Ending build for $@: `date`"
	@echo "*********************************************************"
endef

# Strip command line options from MAKEFLAGS and convert targets specified
# by SUBMAKE_TARGETS to simple target names.
SUBMAKE_MAKEFLAGS_NO_TARGETS = $(foreach flag,$(MAKEFLAGS),\
	$(shell echo $(flag) | grep = | grep -v SUBMAKE_TARGETS))
SUBMAKE_MAKEFLAGS = $(SUBMAKE_MAKEFLAGS_NO_TARGETS) $(SUBMAKE_TARGETS)

#
# Make rule for the source bundles
#

SOURCE_OUTPUT_DIR=$(COMPONENTS_DIR)/cdc/install/pmea_mr2

.PHONY: source_bundle
.PHONY: source_bundles
source_bundle: source_bundles
source_bundles:
	$(TRACE_BUILD_START)
	make source_bundles $(DEFAULT_FLAGS) $(MIDP_FLAGS) \
		SOURCE_OUTPUT_DIR=$(SOURCE_OUTPUT_DIR) \
		BUNDLE_PORTS="win32-arm-*" \
		$(SUBMAKE_MAKEFLAGS_NO_TARGETS)
	$(TRACE_BUILD_END)

#
# Make rules for all the various build flavors
#

.PHONY: opt_preload_jit
opt_preload_jit:
	$(TRACE_BUILD_START)
	$(AT)make $(DEFAULT_FLAGS) $(SUBMAKE_MAKEFLAGS)
	$(TRACE_BUILD_END)

.PHONY: opt_preload_jit_midp
opt_preload_jit_midp:
	$(TRACE_BUILD_START)
	$(AT)make $(DEFAULT_FLAGS) $(MIDP_FLAGS) $(SUBMAKE_MAKEFLAGS)
	$(TRACE_BUILD_END)
