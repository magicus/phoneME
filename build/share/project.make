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

BUILD_DIR := $(os_family)_$(arch)

PROJECT_MAKE_INCLUDED = true

MakeDeps_JvmSourceIncludes=\
	-relativeInclude src/vm/share/compiler    \
	-relativeInclude src/vm/share/debugger    \
	-relativeInclude src/vm/share/float       \
	-relativeInclude src/vm/share/handles     \
	-relativeInclude src/vm/share/interpreter \
	-relativeInclude src/vm/share/isolate     \
	-relativeInclude src/vm/share/memory      \
	-relativeInclude src/vm/share/natives     \
	-relativeInclude src/vm/share/reflection  \
	-relativeInclude src/vm/share/runtime     \
	-relativeInclude src/vm/share/utilities   \
	-relativeInclude src/vm/share/ROM         \
	-relativeInclude src/vm/share/verifier    \
	-relativeInclude src/vm/os/$(os_family)   \
	-relativeInclude src/vm/cpu/$(arch)

# Common Opts for the IDE builds
MakeDepsIDEOpts= \
	-sourceBase $(JVMWorkSpace) \
	-startAt src \
	-$(ide_project_ext)FileName cldc_vm.$(ide_project_ext) \
	-envVar JVMWorkSpace \
	-exeLoc $(JVMBuildSpace)/bin \
	-perFileLine JVM.cpp ' ADD CPP /Yc"incls/_precompiled.incl"' \
	-perFileLine OS_$(os_family).cpp ' SUBTRACT CPP /YX /Yc /Yu' \
	-perFileLine NativesTable.cpp    ' SUBTRACT CPP /YX /Yc /Yu' \
	-perFileLine ROMImage.cpp        ' SUBTRACT CPP /YX /Yc /Yu' \
	-additionalFile includeDB \
	-ignoreFile Interpreter_$(arch)$(ASM_SUFFIX) \
	-ignoreFile NativesTable.cpp \
	-ignoreFile ROMImage.cpp \
	-additionalGeneratedFile JVMBuildSpace \
              $(BUILD_DIR)/target/generated ROMImage.cpp \
	-additionalGeneratedFile JVMBuildSpace \
              $(BUILD_DIR)/target/generated NativesTable.cpp \
	-absoluteInclude $(JVMBuildSpace)/$(BUILD_DIR)/target/generated \
	$(MakeDeps_JvmSourceIncludes)

ifeq ("$(ROMIZING)", "true")
MakeDepsIDEOpts += -define ROMIZING
endif

cldc_vm.$(ide_project_ext): $(GEN_DIR)/Dependencies
	@echo "=============================="
	@echo "creating IDE project file"
	@echo "=============================="
	@rm -f $@
	@cat $(IncludeDB) > includeDB
	@cd $(GEN_DIR); $(JAVA) \
		-jar $(BUILDTOOLS_DIR)/buildtool.jar \
		makedep $(MakeDepsMain) platform includeDB $(MakeDepsOpts) \
		$(MakeDepsIDEOpts)
	@echo "# last updated `date`" >> $@
	@echo "done"

cldc_vm.$(ide_workspace_ext): 
	@cp $(JVMWorkSpace)/build/share/bin/$(BUILD_DIR)/cldc_vm.$(ide_workspace_ext) ./
