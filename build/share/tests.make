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

PATHSEP_win32        = \;
PATHSEP_linux        = :
PATHSEP_solaris      = :
PATHSEP              = ${PATHSEP_$(host_os)}

EXE_SUFFIX_win32     = .exe
EXE_SUFFIX_linux     = 
EXE_SUFFIX           = ${EXE_SUFFIX_$(host_os)}

DIST_DIR             = ../dist
TOOLS_DIR            = ../tools
CLDC_ZIP             = $(DIST_DIR)/lib/cldc_classes.zip
CLDCX_ZIP            = $(DIST_DIR)/lib/cldcx_classes.zip
ROM_GENERATOR        = $(DIST_DIR)/bin/romgen$(EXE_SUFFIX)
JCC	             = $(DIST_DIR)/bin/NativesTableGen.jar
JAVA		     = $(JDK_DIR)/bin/java
JAVADOC		     = $(JDK_DIR)/bin/javadoc
JAVAC                = $(JDK_DIR)/bin/javac -source 1.4 -target 1.4 -g:none
JAR                  = $(JDK_DIR)/bin/jar
JAVA_TOOLS           = $(JDK_DIR)/lib/tools.jar
ASM_TOOLS            = $(WorkSpace)/build/share/bin/asmtools.jar
PREVERIFY            = $(DIST_DIR)/bin/preverify
TEST_SRC_DIR         = $(WorkSpace)/src/tests
TOOL_SRC_DIR         = $(WorkSpace)/src/tools/testtool
HTML_FILE            = html/index.html
ROMCLASSES_ZIP       = romtestclasses.zip
RESOURCES_ZIP        = $(WorkSpace)/build/share/tests/romized.zip
BUILDTOOL_JAR        = $(TOOLS_DIR)/buildtool.jar

#----------------------------------------------------------------------
# Note: the following variables are defined in the auto-generated file
# testcases.make:
#
#    TEST_JASM_SRCS
#    TEST_JAVA_SRCS
#
# If you add new test cases under $(WorkSpace)/src/tests, please do a
# clean build to refresh testcases.make.
#----------------------------------------------------------------------
TOOL_SRCS            = $(wildcard $(TOOL_SRC_DIR)/*.java)
NATIVE_TEST_SRCS     = ROMImage.cpp NativesTable.cpp

ifndef ROMIZING
ROMIZING             = true
endif

all: javatests

ifeq ($(ROMIZING), true)
all: nativetests
endif

VM_JAR_TESTS = bad_test.jar good_test.jar

_javatests:  sanity tests.jar testtool.jar _build_ams_jars _build_shlib_jars _build_dyn_native_jars runtests.sh
tests.jar:   $(VM_JAR_TESTS)
javatests:   _javatests $(HTML_FILE)
nativetests: romtestvm_g
nativetests: romtestvm_r
nativetests: romtestvm

non-rom: javatests ROMImage.cpp NativesTable.cpp

_build_jasm: $(TEST_JASM_SRCS)
	@if test ! -d testclasses; then \
		mkdir testclasses; \
	fi
	@echo "compiling test.jar jasm sources ..."
	@$(JAVA) -cp $(JAVA_TOOLS)$(PATHSEP)$(ASM_TOOLS) \
		javasoft.sqe.jasm.Main \
		-d testclasses $?
	@touch $@

# All test cases that are not romized
tests.jar: $(BUILDTOOL_JAR) _build_jasm $(TEST_JAVA_SRCS)
	@if test ! -d tmpclasses; then \
		mkdir tmpclasses; \
	fi
	@if test ! -d testclasses; then \
		mkdir testclasses; \
	fi
	@echo "compiling test.jar java sources ..."
	@$(JAVAC) -d tmpclasses -bootclasspath $(CLDC_ZIP) \
		-classpath testclasses \
		@TEST_JAVA_SRCS.lst
	@$(PREVERIFY) -classpath $(CLDC_ZIP)$(PATHSEP)testclasses \
		-d testclasses tmpclasses
	@rm -rf $@ tmpclasses
	@$(JAR) -cfM0 $@ -C testclasses .
	@$(JAVA) -jar $(BUILDTOOL_JAR) testjarentries $@
	@echo created $(THIS_DIR)/tests.jar

AMS_JARS_DIR = amsjars

_build_ams_jars: tests.jar
	@if test ! -d $(AMS_JARS_DIR); then \
		mkdir $(AMS_JARS_DIR); \
	fi
	$(JAR) -cfM0 $(AMS_JARS_DIR)/HelloWorld.jar -C testclasses \
		amstest/HelloWorld.class
	@touch $@

SHARED_LIB_JARS_DIR = sharedlibjars

_build_shlib_jars: tests.jar
ifeq ($(ENABLE_LIB_IMAGES), true)
	@if test ! -d $(SHARED_LIB_JARS_DIR); then \
		mkdir $(SHARED_LIB_JARS_DIR); \
	fi

	$(JAR) -cfM0 $(SHARED_LIB_JARS_DIR)/lib.jar -C testclasses \
		shared_libraries/simple/ShLib.class
	$(JAR) -cfM0 $(SHARED_LIB_JARS_DIR)/b.jar -C testclasses \
		shared_libraries/simple/ShLibB.class
ifeq ($(ENABLE_ISOLATES), true)
	$(JAR) -cfM0 $(SHARED_LIB_JARS_DIR)/a.jar -C testclasses \
		shared_libraries/simple/MShLibA.class
	@touch $@
endif
endif

DYN_NATIVES_DIR = dyn_natives

_build_dyn_native_jars: tests.jar
ifeq ($(ENABLE_ISOLATES), true)
	@if test ! -d $(DYN_NATIVES_DIR); then \
		mkdir $(DYN_NATIVES_DIR); \
	fi
	$(JAR) -cfM0 $(DYN_NATIVES_DIR)/sys_cp.jar -C testclasses dynamic_natives/system_package/SystemCls.class \
               -C testclasses dynamic_natives/restricted_package/Cls2.class
	@touch $@
	$(JAR) -cfM0 $(DYN_NATIVES_DIR)/app_cp.jar -C testclasses dynamic_natives/MainCls.class -C testclasses dynamic_natives/restricted_package/Cls.class -C testclasses dynamic_natives/hidden_package/Cls.class
	@touch $@ 
ifeq ($(os_family), linux)
	@cp $(JVMWorkSpace)/src/tests/dynamic_natives/lib.c $(DYN_NATIVES_DIR) 
	@gcc -fPIC -I$(DIST_DIR)/include/ -DLINUX -c dyn_natives/lib.c  -o dyn_natives/lib.obj
	@gcc -shared -o dyn_natives/lib.so dyn_natives/lib.obj
endif
endif
testtool.jar: $(TOOL_SRCS)
	@if test ! -d testtool; then \
		mkdir testtool; \
	fi
	$(JAVAC) -classpath $(JAVA_TOOLS)$(PATHSEP)testtool -d testtool $?
	@rm -f $@
	@$(JAR) -cfM0 $@ -C testtool .

$(HTML_FILE): $(TEST_JAVA_SRCS) testtool.jar
	@if test ! -d html; then \
		mkdir html; \
	fi
	$(JAVADOC) -doclet TestDoclet \
		-docletpath testtool.jar \
		@TEST_JAVA_SRCS.lst

sanity:
	@if test -f $(CLDC_ZIP); then \
	    true; \
	 else \
	    echo please make debug, release and/or product first; \
	    exit -1; \
	 fi
	@if test ! -f $(JAVA_TOOLS); then \
	    echo JDK_DIR/lib/tools.jar not found. Please set your; \
	    echo JDK_DIR environment variable; \
	fi

run:
	$(JAVA) -cp testtool.jar Executor

runtests.sh: $(WorkSpace)/src/tests/runtests.sh
	rm -f $@
	cp $< $@

%.jar: $(WorkSpace)/src/tests/jars/%.jar
	@echo "Getting $@..."
	rm -f $@
	cp $< $@

clean:
	rm -rf testclasses romtestclasses html
	rm -rf tmpclasses positiveclasses.txt tests.db negativeclasses.txt
	rm -rf _* *~
	rm -rf tests.jar testtool.jar
	rm -rf testtool
	rm -rf romtestvm_g romtestvm_r romtestvm
	rm -rf ROMImage.cpp ROMLog.txt ROMStructs.h NativesTable.cpp
	rm -rf $(ROMCLASSES_ZIP)
	rm -rf amsjars

# Short-hands and synonyms
rom:  romtestvm
romr: romtestvm_r
romg: romtestvm_g
test: run

runams:
	romtestvm/ams_romtestvm

runamsr:
	romtestvm_r/ams_romtestvm_r

runamsg:
	romtestvm_g/ams_romtestvm_g

#----------------------------------------------------------------------
# Native test files
#----------------------------------------------------------------------
ROM_CFG = $(WorkSpace)/src/tests/test_rom.cfg

ROMImage.cpp: $(ROMCLASSES_ZIP) $(ROM_CFG) $(ROM_GENERATOR) 
	$(ROM_GENERATOR) -cp $(ROMCLASSES_ZIP)$(PATHSEP)$(RESOURCES_ZIP) \
		-romize =HeapCapacity8M \
		-romconfig $(ROM_CFG) \
		-romincludepath $(WorkSpace)/src/vm \
		+EnableAllROMOptimizations \
		+RewriteROMConstantPool \
		-AggressiveROMSymbolRenaming \
		+GenerateROMStructs ${EXTRA_TEST_ROMGEN_FLAGS}

test-romize:
	rm -f ROMImage.cpp
	${MAKE} ROMImage.cpp \
	    EXTRA_TEST_ROMGEN_FLAGS="+GenerateROMComments +VerboseROMComments"
	mv ROMImage.cpp ROMImage.test.cpp 

NativesTable.cpp: $(ROMCLASSES_ZIP) $(JCC)
	@echo generating $@ ...
	@$(JAVA) -cp $(JCC) JavaCodeCompact \
		 -writer CLDC_HI_Natives -o $@ $(ROMCLASSES_ZIP)

# $(ROMCLASSES_ZIP) -- 
#
# This file contains all CLDC system classes, plus parts of tests.jar
# that should be included in the romtestvm executables. The rules for
# deciding which part of tests.jar to include is too complicated to
# put in a Makefile. Please see
# src/tools/buildtool/tests/CreateRomTestClasses.java
$(ROMCLASSES_ZIP): $(BUILDTOOL_JAR) $(CLDC_ZIP) $(CLDCX_ZIP) tests.jar
	@echo generating $@ ...
	@$(JAVA) -jar $(BUILDTOOL_JAR) romtestclasses \
		$(CLDC_ZIP) $(CLDCX_ZIP) tests.jar $@

#----------------------------------------------------------------------
# Test VMs -- these are the VM executables that contain native code
#             for performing extra tests that are not possible with
#             pure Java test code.
#----------------------------------------------------------------------
romtestvm_g: _build_ams_jars $(NATIVE_TEST_SRCS) romtestvm_g/Makefile FORCE
	@echo "=============================="
	@echo "creating test VM (debug mode)"
	@echo "=============================="
	@$(MAKE) -C romtestvm_g
	@echo "done"

romtestvm_r: _build_ams_jars $(NATIVE_TEST_SRCS) romtestvm_r/Makefile FORCE
	@echo "=============================="
	@echo "creating test VM (release mode)"
	@echo "=============================="
	@$(MAKE) -C romtestvm_r
	@echo "done"

romtestvm: _build_ams_jars $(NATIVE_TEST_SRCS) romtestvm/Makefile FORCE
	@echo "=============================="
	@echo "creating test VM (product mode)"
	@echo "=============================="
	@$(MAKE) -C romtestvm
	@echo "done"

romtestvm_g/Makefile:
	@if test ! -d romtestvm_g; then \
		mkdir -p romtestvm_g; \
	fi
	@rm -f $@
	@echo '# This file is auto-generated. Do not edit'      >> $@
	@echo 'JVMWorkSpace=$(WorkSpace)'                       >> $@
	@echo 'JVMBuildSpace=$(BuildSpace)'                     >> $@
	@echo 'WorkSpace=$(WorkSpace)'                          >> $@
	@echo 'BuildSpace=$(BuildSpace)'                        >> $@
	@echo 'os_family=$(os_family)'                          >> $@
	@echo 'BUILD=debug'                                     >> $@
	@echo ''                                                >> $@
	@echo 'default:: all'                                   >> $@
	@echo ''                                                >> $@
	@echo 'include $(WorkSpace)/build/share/romtestvm.make' >> $@
	@echo created $@

romtestvm_r/Makefile:
	@if test ! -d romtestvm_r; then \
		mkdir -p romtestvm_r; \
	fi
	@rm -f $@
	@echo '# This file is auto-generated. Do not edit'      >> $@
	@echo 'JVMWorkSpace=$(WorkSpace)'                       >> $@
	@echo 'JVMBuildSpace=$(BuildSpace)'                     >> $@
	@echo 'WorkSpace=$(WorkSpace)'                          >> $@
	@echo 'BuildSpace=$(BuildSpace)'                        >> $@
	@echo 'os_family=$(os_family)'                          >> $@
	@echo 'BUILD=release'                                   >> $@
	@echo ''                                                >> $@
	@echo 'default:: all'                                   >> $@
	@echo ''                                                >> $@
	@echo 'include $(WorkSpace)/build/share/romtestvm.make' >> $@
	@echo created $@

romtestvm/Makefile:
	@if test ! -d romtestvm; then \
		mkdir -p romtestvm; \
	fi
	@rm -f $@
	@echo '# This file is auto-generated. Do not edit'      >> $@
	@echo 'JVMWorkSpace=$(WorkSpace)'                       >> $@
	@echo 'JVMBuildSpace=$(BuildSpace)'                     >> $@
	@echo 'WorkSpace=$(WorkSpace)'                          >> $@
	@echo 'BuildSpace=$(BuildSpace)'                        >> $@
	@echo 'os_family=$(os_family)'                          >> $@
	@echo 'BUILD=product'                                   >> $@
	@echo ''                                                >> $@
	@echo 'default:: all'                                   >> $@
	@echo ''                                                >> $@
	@echo 'include $(WorkSpace)/build/share/romtestvm.make' >> $@
	@echo created $@

FORCE:

testcases.make: FORCE
	@$(JAVA) -jar ../tools/buildtool.jar testcases \
		$(JVMWorkSpace) .
	@echo updated $@

tools:
	@$(MAKE) -C ../tools

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
