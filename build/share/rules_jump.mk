#
# @(#)rules_jump.mk	1.3 06/10/25
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

ifeq ($(USE_JUMP),true)

# print our configuration
printconfig::
	@echo "JUMP_DIR           = $(JUMP_DIR)"

.PHONY: jumptargets jumptargets-api jumptargets-impl 
jumptargets: jumptargets-api jumptargets-impl

jumptargets-api: force_jump_build-api
jumptargets-impl: $(JSROP_JARS) force_jump_build-impl $(CVM_BINDIR)/runjump $(CVM_BINDIR)/autotest $(CVM_BINDIR)/runinstall $(CVM_BINDIR)/runxinstall

$(CVM_BUILD_DEFS_MK)::
	$(AT) echo updating $@ [from rules_jump.mk]
	$(AT) echo "# JUMP specific exports" >> $@
	$(AT) echo "JUMP_API_CLASSESZIP = $(JUMP_API_CLASSESZIP)" >> $@
	$(AT) echo "" >> $@

#
# For now we are forcing a jump build because we can't deduce dependencies
# that force it to be rebuilt. But list $(JUMP_DEPENDENCIES) anyway just to
# make things explicit
#
force_jump_build-api: $(JUMP_DEPENDENCIES)
	@echo "====> start building jump api's"
	$(AT)(cd $(JUMP_DIR); $(CVM_ANT) $(CVM_ANT_OPTIONS) $(JUMP_ANT_OPTIONS) -f build/build.xml build-api)
	@echo "====> done building jump api's"

force_jump_build-impl: $(JUMP_DEPENDENCIES)
	@echo "====> start building jump impl's"
	$(AT)(cd $(JUMP_DIR); $(CVM_ANT) $(CVM_ANT_OPTIONS) $(JUMP_ANT_OPTIONS) -f build/build.xml build-impl)
	$(AT)cp $(JUMP_SHARED_BOOTCLASSESZIP) \
                $(JUMP_EXECUTIVE_BOOTCLASSESZIP) \
                $(CVM_LIBDIR)
	@echo  "<==== done building jump implementation"

#
# For a non-romized build we should add JSRs jarfiles to Xbootclasspath
# for a server cvm instance. After forking the classpath will be inherited by
# executive and isolates cvm instances.
# For a romized build all JSR classes are included in cvm executable.
#
$(CVM_BINDIR)/runjump: $(JUMP_SCRIPTS_DIR)/runjump
ifneq ($(CVM_PRELOAD_LIB), true)
	$(AT)sed -e "s,^ *SERVER_JARFILE=.*$$,&$(JUMP_JSROP_JARS)," $^ > $@
else
	$(AT)cp $^ $@
endif
	$(AT)chmod 755 $@

$(CVM_BINDIR)/%: $(JUMP_SCRIPTS_DIR)/%
	$(AT)cp $^ $@
	$(AT)chmod 755 $@

.PHONY: javadoc-api
javadoc-api:
	@echo "====> start building Javadoc for jump APIs"
	$(AT)(cd $(JUMP_DIR); $(CVM_ANT) $(CVM_ANT_OPTIONS) $(JUMP_ANT_OPTIONS) -f build/build.xml javadoc-api)
	@echo "<==== end building Javadoc for jump APIs"

$(JUMP_NATIVE_LIBRARY_PATHNAME) :: $(JUMP_NATIVE_LIB_OBJS)
	@echo "Linking $@"
	$(call SO_LINK_CMD, $^,)
	$(AT)cp $@ $(CVM_LIBDIR)

#
# JUMP unit testing
#
# NOTE: due to quirks of Ant 1.6.x JUnit3.8.x jar should be added into ant libs
#

BUILD_UNITTEST_ANT_OPTIONS = $(CVM_ANT_OPTIONS) $(JUMP_ANT_OPTIONS) -Djunit3.8.jar=$(JUNIT_JAR)
RUN_UNITTEST_ANT_OPTIONS = $(BUILD_UNITTEST_ANT_OPTIONS) -lib $(JUNIT_JAR) -Dreports.dir=$(REPORTS_DIR)/jump

# Provide a default value to JUNIT_JAR if it's not set
ifdef JUNIT_JARFILE
JUNIT_JAR ?= $(JUNIT_JARFILE)
else
JUNIT_JAR ?= /usr/share/java/junit.jar
endif

# Quick check of JUNIT_JAR validity
define check_JUNIT_JAR
	$(AT)($(CVM_JAR) tf $(JUNIT_JAR) > /dev/null || (echo "JUNIT_JAR appears to be invalid or missing: [$(JUNIT_JAR)]" ; exit -1))
endef

build-unittests::
	@echo "====> start building jump unit-tests"
	$(check_JUNIT_JAR)
	$(AT)(cd $(JUMP_DIR); $(CVM_ANT) $(BUILD_UNITTEST_ANT_OPTIONS) -f build/build.xml only-build-unittests)
	@echo "<==== end building jump unit-tests"

run-unittests::
	@echo "====> start running jump unit-tests"
	$(check_JUNIT_JAR)
	$(AT)(cd $(JUMP_DIR); $(CVM_ANT) $(RUN_UNITTEST_ANT_OPTIONS) -f build/build.xml only-run-unittests)
	@echo "<==== end running jump unit-tests"

source_bundle::
	@echo " ... jump source bundle"
	$(AT)(cd $(JUMP_DIR); \
	      $(CVM_ANT) $(CVM_ANT_OPTIONS) $(JUMP_ANT_OPTIONS) \
	      		-Denv.SOURCE_OUTPUT_DIR=$(SOURCE_OUTPUT_DIR) \
	      		-f build/build-source-bundle.xml source-bundle)

clean::
	$(AT)(cd $(JUMP_DIR); $(CVM_ANT) $(CVM_ANT_OPTIONS) $(JUMP_ANT_OPTIONS) -f build/build.xml clean)

endif
