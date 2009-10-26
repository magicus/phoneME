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

#####################################
#  Check for obsolete definitions that may have been made by defs files.
#####################################


# CVM include directories used to be specified with CVM_INCLUDES, and included
# the -I option. Now they are specified with CVM_INCLUDE_DIRS and do not
# include the -I option.
ifdef CVM_INCLUDES
$(error CVM_INCLUDES is no longer supported. Use CVM_INCLUDE_DIRS and remove the leading "-I".)
else
# force an error if referenced in a command line
CVM_INCLUDES = "Do not reference CVM_INCLUDES"
endif

# Profile include directories used to be specified with PROFILE_INCLUDES, and 
# included the -I option. Now they are specified with PROFILE_INCLUDE_DIRS
# and do not include the -I option.
ifdef PROFILE_INCLUDES
$(error PROFILE_INCLUDES is no longer supported. Use PROFILE_INCLUDE_DIRS and remove the leading "-I".)
else
# force an error if referenced in a command line
PROFILE_INCLUDES = "Do not reference PROFILE_INCLUDES"
endif

# PROFILE_INCLUDE_DIRS is a list of profile specific directories that contains
# 	the profile specific include paths. These paths should be searched for
#	include files before searching the base configuration include path.
# CVM_INCLUDE_DIRS is a list of directories that defines the base configuration
# 	include path. 
# ALL_INCLUDE_DIRS combines the above lists. On most platforms it needs to
# 	be converted to host from before used.
# ALL_INCLUDE_FLAGS is ALL_INCLUDE_DIRS converted into the compiler
# 	command line option for C include directories.
ALL_INCLUDE_DIRS	:= $(PROFILE_INCLUDE_DIRS) $(CVM_INCLUDE_DIRS)
ALL_INCLUDE_FLAGS	:= $(call makeIncludeFlags,$(ALL_INCLUDE_DIRS))

#
#  Common makefile rules
#

ifneq ($(CVM_TOOLS_BUILD), true)
tools :
	$(MAKE) CVM_TOOLS_BUILD=true tools
endif

#########################
# Print some important configuration information that isn't included
# when we dump build_defs.h later on.
#########################

CALL_TEST = $(1)
ifeq ($(call CALL_TEST,$$$$),$$)
# $(warning Weird make version $(MAKE_VERSION))
# Work around gnumake 3.79.1 weirdness
DDOLLAR := $$$$
else
DDOLLAR := $$
endif

#
# These are some printconfig variables that other hosts or targets
# may want to override.
#
TARGET_CC_VERSION ?= $(shell $(TARGET_CC) -dumpversion; $(TARGET_CC) -dumpmachine)
HOST_CC_VERSION   ?= $(shell $(HOST_CC) -dumpversion; $(HOST_CC) -dumpmachine)
CVM_JAVA_VERSION  ?= $(shell $(CVM_JAVA) -version 2>&1 | grep version)
HOST_UNAME        ?= $(shell uname -a)

# The cygwin "which" command doesn't work when a full path is given.
# The purpose of the following is to break the full path into the dir
# and tool name components, and then setup $PATH before calling "which"
TOOL_PATH0	= $(shell \
	set $(1);	\
	TOOL_WORD1="$$1";	\
	TOOL_CMD="`basename $${TOOL_WORD1}`"; \
	(if [ "$${TOOL_CMD}" = "$${TOOL_WORD1}" ]; then \
		$(call TOOL_WHICH,$(DDOLLAR){TOOL_CMD});	\
	else	\
	 	ls "$${TOOL_WORD1}" 2>&1 ;	\
	fi;) 2>&1 )
TOOL_PATH = $(subst //,/,$(call TOOL_PATH0,$($(1))))

TARGET_CC_PATH	= $(call TOOL_PATH,TARGET_CC)
TARGET_CCC_PATH	= $(call TOOL_PATH,TARGET_CCC)
TARGET_AS_PATH	= $(call TOOL_PATH,TARGET_AS)
TARGET_LD_PATH	= $(call TOOL_PATH,TARGET_LD)
TARGET_AR_PATH	= $(call TOOL_PATH,TARGET_AR)
TARGET_RANLIB_PATH = $(call TOOL_PATH,TARGET_RANLIB)
CVM_JAVA_PATH	= $(call TOOL_PATH,CVM_JAVA)
CVM_JAVAC_PATH	= $(call TOOL_PATH,CVM_JAVAC)
CVM_JAVAH_PATH	= $(call TOOL_PATH,CVM_JAVAH)
CVM_JAR_PATH	= $(call TOOL_PATH,CVM_JAR)
ZIP_PATH	= $(call TOOL_PATH,ZIP)
HOST_CC_PATH	= $(call TOOL_PATH,HOST_CC)
HOST_CCC_PATH	= $(call TOOL_PATH,HOST_CCC)
ifeq ($(CVM_JIT), true)
FLEX_PATH	= $(call TOOL_PATH,FLEX)
BISON_PATH	= $(call TOOL_PATH,BISON)
endif

printconfig::
	@echo "MAKEFLAGS  = $(MAKEFLAGS)"
	@echo "CVM_HOST   = $(CVM_HOST)"
	@echo "CVM_TARGET = $(CVM_TARGET)"
	@echo "SHELL      = $(SHELL)"
	@echo "HOST_CC    = $(HOST_CC_PATH)"
	@echo "HOST_CCC   = $(HOST_CCC_PATH)"
	@echo "ZIP        = $(ZIP_PATH)"
ifeq ($(CVM_JIT), true)
	@echo "FLEX       = $(FLEX_PATH)"
	@echo "BISON      = $(BISON_PATH)"
endif
	@echo "CVM_JAVA   = $(CVM_JAVA_PATH)"
	@echo "CVM_JAVAC  = $(CVM_JAVAC_PATH)"
	@echo "CVM_JAVAH  = $(CVM_JAVAH_PATH)"
	@echo "CVM_JAR    = $(CVM_JAR_PATH)"
	@echo "TARGET_CC     = $(TARGET_CC_PATH)"
	@echo "TARGET_CCC    = $(TARGET_CCC_PATH)"
	@echo "TARGET_AS     = $(TARGET_AS_PATH)"
	@echo "TARGET_LD     = $(TARGET_LD_PATH)"
	@echo "TARGET_AR     = $(TARGET_AR_PATH)"
	@echo "TARGET_RANLIB = $(TARGET_RANLIB_PATH)"
ifeq ($(CVM_BUILD_SO), true)
	@echo "SO_LINKFLAGS  = $(SO_LINKFLAGS)"
else
	@echo "LINKFLAGS  = $(LINKFLAGS)"
endif
	@echo "LINKLIBS   = $(LINKLIBS)"
	@echo "ASM_FLAGS  = $(ASM_FLAGS)"
	@echo "CCCFLAGS   = $(CCCFLAGS)"
	@echo "CCFLAGS_SPEED  = $(CCFLAGS_SPEED)"
	@echo "CCFLAGS_SPACE  = $(CCFLAGS_SPACE)"
	@echo "CCFLAGS_LOOP   = $(CCFLAGS_LOOP)"
	@echo "CCFLAGS_FDLIB  = $(CCFLAGS_FDLIB)"
	@echo "JAVAC_OPTIONS  = $(JAVAC_OPTIONS)"
	@echo "CVM_DEFINES    = $(CVM_DEFINES)"
	@echo "host uname        = $(HOST_UNAME)"
	@echo "TARGET_CC version = $(TARGET_CC_VERSION)"
	@echo "HOST_CC version   = $(HOST_CC_VERSION)"
	@echo "CVM_JAVA version  = $(CVM_JAVA_VERSION)"
	@echo "TOOLS_DIR         = $(TOOLS_DIR)"

#
# Determine if the target compiler is really meant for the device
# we are targetting as specified by $(TARGET_CPU_FAMILY) and $(TARGET_OS),
# which are determined by the build directory.
#
# This compiler check is really gcc specific, and relies on the -dumpmachine
# output. For other platforms you can override them. Simply setting
# CVM_COMPILER_INCOMPATIBLE=false in a porting makefile, including
# in the GNUmakefile, will prevent this check and avoid any incorrect
# error reporting.
#

ifeq ($(CVM_DISABLE_COMPILER_CHECK),true)
CVM_COMPILER_INCOMPATIBLE ?= false
else
CVM_COMPILER_TARGET ?= $(shell $(TARGET_CC) -dumpmachine 2>&1)
# tranform things like i686 into x86
CVM_COMPILER_TARGET := \
    $(shell echo $(CVM_COMPILER_TARGET) | sed -e 's/.*86\(-.*-.*\)/x86\1/')
ifneq ($(findstring $(TARGET_OS),$(CVM_COMPILER_TARGET)),$(TARGET_OS))
CVM_COMPILER_INCOMPATIBLE ?= true
endif
ifneq ($(findstring $(TARGET_CPU_FAMILY),$(CVM_COMPILER_TARGET)),$(TARGET_CPU_FAMILY))
CVM_COMPILER_INCOMPATIBLE ?= true
endif
endif  # CVM_DISABLE_COMPILER_CHECK == false

checkconfig::
ifeq ($(CVM_COMPILER_INCOMPATIBLE),true)
	@echo "Target tools not properly specified. The gcc -dumpmachine"
	@echo "output does not agree with CVM_TARGET. The OS and CPU portions"
	@echo "must match for compatibility. If this is a cross compile, you"
	@echo "probably forgot to set CVM_TARGET_TOOLS_PREFIX. If you want to"
	@echo "to turn off this check, set CVM_COMPILER_INCOMPATIBLE=false"
	@echo "on the make command line or in the GNUmakefile"
	@echo "   CVM_TARGET:      $(CVM_TARGET)"
	@echo "   compiler target: $(CVM_COMPILER_TARGET)"
	exit 2
endif

#########################
# Compiling .java files.
##########################

#
# All directories that contain java source except for test classes.
# Allow profile classes to override all others.
#
JAVA_SRCDIRS += \
	$(PROFILE_VMIMPLCLASSES_SRCDIR) $(CVM_VMIMPLCLASSES_SRCDIR) \
	$(OPT_PKGS_SRCPATH) $(PROFILE_SRCDIRS) \
	$(CVM_SHAREDCLASSES_SRCDIR) $(CVM_TARGETCLASSES_SRCDIR) \
	$(CVM_CLDCCLASSES_SRCDIR) \
	$(CVM_DERIVEDROOT)/classes

CVM_TESTCLASSES_SRCDIRS += $(CVM_TESTCLASSES_SRCDIR)

ifneq ($(CVM_TOOLS_BUILD), true)
# vpath for all directories that contain java source
vpath %.java	$(JAVA_SRCDIRS) \
		$(CVM_TESTCLASSES_SRCDIRS) \
		$(CVM_DEMOCLASSES_SRCDIRS)
endif

#
# Add optional package classes, if any, to CLASSLIB_CLASSES
#
CLASSLIB_CLASSES += $(OPT_PKGS_CLASSES)

#
# If CVM_PRELOAD_LIB is set, all CLASSLIB_CLASSES will be added
# into CVM_BUILDTIME_CLASSES. 
#

CVM_PRELOAD_CLASSES = $(CVM_BUILDTIME_CLASSES_min)

_CVM_PRELOAD_SET := $(patsubst %full,%,$(CVM_PRELOAD_SET))

ifeq ($(_CVM_PRELOAD_SET), min)
    CVM_LIB_CLASSES += $(CVM_BUILDTIME_CLASSES_nullapp)
    CVM_LIB_CLASSES += $(CVM_BUILDTIME_CLASSES)
    CVM_LIB_CLASSES += $(CLASSLIB_CLASSES)
    CVM_BT_nullapp_CLASSESDIR = $(LIB_CLASSESDIR)
else
ifeq ($(_CVM_PRELOAD_SET), nullapp)
    CVM_PRELOAD_CLASSES += $(CVM_BUILDTIME_CLASSES_nullapp)
    CVM_LIB_CLASSES += $(CVM_BUILDTIME_CLASSES)
    CVM_LIB_CLASSES += $(CLASSLIB_CLASSES)
    CVM_BT_nullapp_CLASSESDIR = $(CVM_BUILDTIME_CLASSESDIR)
else
ifeq ($(_CVM_PRELOAD_SET), oldbt)
    CVM_PRELOAD_CLASSES += $(CVM_BUILDTIME_CLASSES_nullapp)
    CVM_PRELOAD_CLASSES += $(CVM_BUILDTIME_CLASSES)
    CVM_LIB_CLASSES += $(CLASSLIB_CLASSES)
    CVM_BT_nullapp_CLASSESDIR = $(CVM_BUILDTIME_CLASSESDIR)
else
ifeq ($(_CVM_PRELOAD_SET), lib)
    CVM_PRELOAD_CLASSES += $(CVM_BUILDTIME_CLASSES_nullapp)
    CVM_PRELOAD_CLASSES += $(CVM_BUILDTIME_CLASSES)
    CVM_PRELOAD_CLASSES += $(CLASSLIB_CLASSES)
    CVM_BT_nullapp_CLASSESDIR = $(CVM_BUILDTIME_CLASSESDIR)
else
ifeq ($(_CVM_PRELOAD_SET), libtest)
    CVM_PRELOAD_CLASSES += $(CVM_BUILDTIME_CLASSES_nullapp)
    CVM_PRELOAD_CLASSES += $(CVM_BUILDTIME_CLASSES)
    CVM_PRELOAD_CLASSES += $(CLASSLIB_CLASSES)
    CVM_BT_nullapp_CLASSESDIR = $(CVM_BUILDTIME_CLASSESDIR)
else
    $(error Unknown value '$(CVM_PRELOAD_SET)' for CVM_PRELOAD_SET)
endif
endif
endif
endif
endif

CVM_PRELOAD_CLASSES := $(CVM_PRELOAD_CLASSES)
CVM_LIB_CLASSES := $(CVM_LIB_CLASSES)

CVM_BUILDTIME_CLASSES := $(CVM_PRELOAD_CLASSES)
CLASSLIB_CLASSES := $(CVM_LIB_CLASSES)

ifeq ($(CVM_PRELOAD_FULL_CLOSURE), true)
CVM_ALLOW_UNRESOLVED ?= false
else
CVM_ALLOW_UNRESOLVED ?= true
endif

CVM_DUPLICATES = $(filter $(CLASSLIB_CLASSES), $(CVM_BUILDTIME_CLASSES))
ifneq ($(CVM_DUPLICATES),)
$(error \
    Duplicate classes found in CVM_BUILDTIME_CLASSES and CLASSLIB_CLASSES: \
    $(CVM_DUPLICATES))
endif

CVM_BT_min_CLASSESDIR = $(CVM_BUILDTIME_CLASSESDIR)

#
# Build various lists of classes to be compiled
#

# buildClassesList(classesTarget,class)
ifeq ($(EVAL_SUPPORTED),true)
define buildClassesList
	$(eval BUILD$(1) += $(2))
endef
else
define buildClassesList
	@echo '$(2)' >> $(CVM_BUILD_TOP)/$(1).plist
endef
endif

# preloaded classes except for test classes

$(CVM_BUILDTIME_CLASSESDIR)/%.class :: %.java
	$(call buildClassesList,.btclasses,$?)
	@echo $*.class >>$(CVM_BUILD_TOP)/.btclasses.clist

# handle hidden dependencies, which include inner classes
# and private classes that do not have their own .java
# file
$(CVM_BUILDTIME_CLASSESDIR)/%.class ::
	@if [ '$?' != '$(filter %.java,$?)' ]; then \
	    echo '$@' depends on '$?'! 1>&2; \
	    false; \
	fi
	@if [ '$?' = '' ]; then \
	    echo '$@' dependency missing! 1>&2; \
	    echo See rules_cdc.mk 1>&2; \
	    false; \
	fi
	@echo '$*.class' >>$(CVM_BUILD_TOP)/.btclasses.clist

# Non-preloaded classes except for test classes

ifneq ($(CVM_PRELOAD_ALL), true)

ifeq ($(CVM_PRELOAD_FULL_CLOSURE), true)
# If the class ends up in btclasses, don't compile again
# into libclasses.
$(LIB_CLASSESDIR)/%.class :: $(CVM_BUILDTIME_CLASSESDIR)/%.class
ifeq ($(EVAL_SUPPORTED),true)
	$(eval BUILDjavahclasses += $(subst /,.,$*))
else
	@echo '$(subst /,.,$*)' >>$(CVM_BUILD_TOP)/.javahclasses.list
endif
endif

$(LIB_CLASSESDIR)/%.class :: %.java
ifeq ($(EVAL_SUPPORTED),true)
	$(eval BUILDjavahclasses += $(subst /,.,$*))
else
	@echo '$(subst /,.,$*)' >>$(CVM_BUILD_TOP)/.javahclasses.list
endif
	$(call buildClassesList,.libclasses,$?)

# handle hidden dependencies
$(LIB_CLASSESDIR)/%.class ::
	@if [ '$?' != '$(filter %.java,$?)' ]; then \
	    echo '$@' depends on '$?'! 1>&2; \
	    false; \
	fi
	@if [ '$?' = '' ]; then \
	    echo '$@' dependency missing! 1>&2; \
	    echo See rules_cdc.mk 1>&2; \
	    false; \
	fi
	@echo '$*.class' >>$(CVM_BUILD_TOP)/.libclasses.clist

endif

# TODO: Build a jsr jar file and have a rule that makes the jar file
# dependent on all the source files, and builds a list of files to
# recompile. 
#define appendjavafiles
#    @_javalist="$?"; ( \
#    if [ -n "$$_javalist" ]; then \
#	echo "$$_javalist" >> $(REBUILD_JAVA_LIST); \
#    fi )
#endef
#$(MIDP_OUTPUT_DIR)/classes.zip:: $(SUBSYSTEM_SATSA_JAVA_FILES)
#	$(appendjavafiles)

# Include Optional packages rules
-include $(CDC_DIR)/build/share/rules_op.mk

# Test classes
$(CVM_TEST_CLASSESDIR)/%.class: %.java
	$(call buildClassesList,.testclasses,$?)

# demo classes
$(CVM_DEMO_CLASSESDIR)/%.class: %.java
	$(call buildClassesList,.democlasses,$?)

#
# Convert the class lists to names of class files so they can be javac'd.
# First convert . to /, then prepend the classes directory and append .class
#

# CLASSLIB_CLASSES are compiled as BUILDTIME_CLASSES when romized
ifneq ($(CVM_PRELOAD_ALL), true)
CLASSLIB_CLASS0 = $(subst .,/,$(CLASSLIB_CLASSES))
CLASSLIB_CLASS_FILES = \
	$(patsubst %,$(LIB_CLASSESDIR)/%.class,$(CLASSLIB_CLASS0))
endif

BUILDTIME_CLASS0 = $(subst .,/,$(CVM_BUILDTIME_CLASSES))
BUILDTIME_CLASS_FILES = \
	$(patsubst %,$(CVM_BUILDTIME_CLASSESDIR)/%.class,$(BUILDTIME_CLASS0))

TEST_CLASS0 = $(subst .,/,$(CVM_TEST_CLASSES))
TEST_CLASS_FILES = \
	$(patsubst %,$(CVM_TEST_CLASSESDIR)/%.class,$(TEST_CLASS0))

DEMO_CLASS0 = $(subst .,/,$(CVM_DEMO_CLASSES))
DEMO_CLASS_FILES = $(patsubst %,$(CVM_DEMO_CLASSESDIR)/%.class,$(DEMO_CLASS0))


# Convert list of Java source directories to colon-separated paths
JAVACLASSES_SRCPATH = \
	$(subst $(space),$(PS),$(call POSIX2HOST,$(strip $(JAVA_SRCDIRS))))
TESTCLASSES_SRCPATH = \
	$(subst $(space),$(PS),$(call POSIX2HOST,$(strip $(CVM_TESTCLASSES_SRCDIRS))))
CVM_DEMOCLASSES_SRCPATH = \
	$(subst $(space),$(PS),$(call POSIX2HOST,$(strip $(CVM_DEMOCLASSES_SRCDIRS))))

# Convert list of classpath entries to colon-separated path
JAVACLASSES_CLASSPATH = $(subst $(space),$(PS),$(strip $(JAVA_CLASSPATH)))

# CR 6214008
# Convert list of OPT_PKGS classpath entries to colon-separated path
# Note, if empty then don't bother, or POSIX2HOST will result in a cygpath
# error message.
ifneq ($(OPT_PKGS_CLASSPATH),)
OPTPKGS_CLASSPATH0 = $(strip $(OPT_PKGS_CLASSPATH))
OPTPKGS_CLASSPATH1 = $(call POSIX2HOST,$(OPTPKGS_CLASSPATH0))
OPTPKGS_CLASSPATH  = $(subst $(space),$(PS),$(OPTPKGS_CLASSPATH1))
endif

# Convert list of jar files to colon-separated path
TEST_JARFILES = $(subst $(space),$(PS),$(strip $(CVM_TEST_JARFILES)))

$(CVM_BUILD_TOP)/%.list : $(CVM_BUILD_TOP)/%.plist
	@$(POSIX2HOST_FILTER) < $< > $@
	@$(RM) $<

$(CVM_BUILD_TOP)/.libclasses.plist : $(CLASSLIB_CLASS_FILES) $(CLASSLIB_JAR_FILES)
ifeq ($(EVAL_SUPPORTED),true)
	@printf %s '$(value BUILDjavahclasses)' > \
	    $(CVM_BUILD_TOP)/.javahclasses.list
	@printf %s '$(BUILD.libclasses)' > $@
else
	@touch $@
endif

$(CVM_BUILD_TOP)/.btclasses.plist : $(BUILDTIME_CLASS_FILES)
ifeq ($(EVAL_SUPPORTED),true)
	@printf %s "$(BUILD.btclasses)" > $@
else
	@touch $@
endif

$(CVM_BUILD_TOP)/.testclasses.plist : $(TEST_CLASS_FILES)
ifeq ($(EVAL_SUPPORTED),true)
	@printf %s "$(BUILD.testclasses)" > $@
else
	@touch $@
endif

$(CVM_BUILD_TOP)/.democlasses.plist : $(DEMO_CLASS_FILES)
ifeq ($(EVAL_SUPPORTED),true)
	@printf %s "$(BUILD.democlasses)" > $@
else
	@touch $@
endif

.delete.libclasses.list:
	@$(RM) $(CVM_BUILD_TOP)/.libclasses.*list
	@$(RM) $(CVM_BUILD_TOP)/.javahclasses.list

.delete.btclasses.list:
	@$(RM) $(CVM_BUILD_TOP)/.btclasses.*list

.delete.testclasses.list:
	@$(RM) $(CVM_BUILD_TOP)/.testclasses.*list

.delete.democlasses.list:
	@$(RM) $(CVM_BUILD_TOP)/.democlasses.*list

.report.libclasses.list:
	@echo "Checking for $(J2ME_PRODUCT_NAME) classes to compile ..."

.report.btclasses.list:
	@echo "Checking for build-time classes to compile ..."

.report.testclasses.list:
	@echo "Checking for test classes to compile ..."

.report.democlasses.list:
	@echo "Checking for demo classes to compile ..."

# Find all the generated btclasses
CVM_BT_FILES0 = $(wildcard \
      $(CVM_BUILDTIME_CLASSESDIR)/*/*/*.class \
      $(CVM_BUILDTIME_CLASSESDIR)/*/*/*/*.class \
      $(CVM_BUILDTIME_CLASSESDIR)/*/*/*/*/*.class \
      $(CVM_BUILDTIME_CLASSESDIR)/*/*/*/*/*/*.class)
CVM_BT_FILES = $(CVM_BT_FILES0)

ifeq ($(CVM_PRELOAD_FULL_CLOSURE), true)
$(CVM_BUILD_TOP)/.btclasses.plist : $(CVM_BT_FILES)
endif

CVM_BT_CLASS_FILES = $(patsubst %,%.class,$(BUILDTIME_CLASS0))

.move.extra.btclasses: $(CLASSLIB_JAR_FILES)
	$(AT)$(MAKE) .move.extra.btclasses2

# javac problably found some dependencies that are not on
# our list, so copy them to libclasses so they end up in
# the jar file

CVM_EXCLUDE_FILE = $(CVM_BUILD_TOP)/.EXCLUDE

.move.extra.btclasses2: $(CVM_BUILD_TOP)/.libclasses.list
ifneq ($(CVM_PRELOAD_FULL_CLOSURE), true)
	@echo '$(CVM_BT_CLASS_FILES)' | tr ' ' '\n' | sort > $(CVM_EXCLUDE_FILE)
	@rsync -qav --exclude-from=$(CVM_EXCLUDE_FILE) \
	    $(CVM_BUILDTIME_CLASSESDIR)/ $(LIB_CLASSESDIR)/
	@touch $(CVM_BUILD_TOP)/.libclasses;
endif

CVM_LIB_BOOTCLASSPATH = \
    $(CVM_BUILDTIME_CLASSESZIP)$(PS)$(JAVACLASSES_CLASSPATH)

.compile.libclasses: .move.extra.btclasses
	$(AT)if [ -s $(CVM_BUILD_TOP)/.libclasses.list ] ; then		\
		echo "Compiling $(J2ME_PRODUCT_NAME) classes...";	\
		$(JAVAC_CMD)						\
			-d $(LIB_CLASSESDIR) 				\
			-bootclasspath $(CVM_LIB_BOOTCLASSPATH) 	\
			-classpath -none-				\
			-sourcepath $(JAVACLASSES_SRCPATH)		\
			@$(CVM_BUILD_TOP)/.libclasses.list ;		\
		touch $(CVM_BUILD_TOP)/.libclasses;			\
	fi

CVM_BT_BOOTCLASSPATH = \
    $(CVM_BUILDTIME_CLASSESDIR)$(PS)$(OPTPKGS_CLASSPATH)

.cvm_bt_files:
	$(AT)$(MAKE) .cvm_bt_files_check
	$(AT)$(MAKE) $(CVM_BUILD_TOP)/.btclasses.list

# We have files in btclasses/ but how about
# btclasses.zip?
.cvm_bt_files_check :
ifneq ($(words $(CVM_BT_FILES)), 0)
	@if [ ! -f $(CVM_BUILDTIME_CLASSESZIP) ]; then			\
	    echo 'btclasses.zip is missing.  Cleaning up.';		\
	    rm -rf $(CVM_BUILDTIME_CLASSESDIR);				\
	    mkdir $(CVM_BUILDTIME_CLASSESDIR);				\
	fi
else
	@true
endif

.compile.btclasses: .cvm_bt_files
	$(AT)if [ -s $(CVM_BUILD_TOP)/.btclasses.list ] ; then		\
		echo "Compiling build-time classes...";			\
		$(JAVAC_CMD)						\
			-d $(CVM_BUILDTIME_CLASSESDIR)			\
			-bootclasspath $(CVM_BT_BOOTCLASSPATH)	 	\
			-classpath -none- \
			-sourcepath $(JAVACLASSES_SRCPATH)		\
			@$(CVM_BUILD_TOP)/.btclasses.list ;		\
		touch $(CVM_BUILD_TOP)/.btclasses;			\
	fi

.compile.testclasses: $(CVM_BUILD_TOP)/.testclasses.list
	$(AT)if [ -s $(CVM_BUILD_TOP)/.testclasses.list ] ; then	\
		echo "Compiling test classes...";			\
		cp -f $(CVM_TESTCLASSES_SRCDIR)/TestSyncLocker.class	\
		      $(CVM_TEST_CLASSESDIR); \
		$(JAVAC_CMD)						\
			-d $(CVM_TEST_CLASSESDIR)			\
			-bootclasspath 					\
			   $(CVM_BUILDTIME_CLASSESZIP)$(PS)$(LIB_CLASSESDIR)\
			-classpath $(CVM_TEST_CLASSESDIR)$(PS)$(TEST_JARFILES) \
			-sourcepath $(TESTCLASSES_SRCPATH)		\
			@$(CVM_BUILD_TOP)/.testclasses.list ;		\
		touch $(CVM_BUILD_TOP)/.testclasses;			\
	fi

.compile.democlasses: $(CVM_BUILD_TOP)/.democlasses.list
	$(AT)if [ -s $(CVM_BUILD_TOP)/.democlasses.list ] ; then	\
		echo "Compiling demo classes...";			\
		$(JAVAC_CMD)						\
			-d $(CVM_DEMO_CLASSESDIR)			\
			-bootclasspath 					\
			   $(CVM_BUILDTIME_CLASSESZIP)$(PS)$(LIB_CLASSESDIR)\
			-classpath $(CVM_DEMO_CLASSESDIR) 		\
			-sourcepath $(CVM_DEMOCLASSES_SRCPATH)		\
			@$(CVM_BUILD_TOP)/.democlasses.list ;	\
		touch $(CVM_BUILD_TOP)/.democlasses;			\
	fi

#
# The rules to compile our four kinds of classes: 
#     library (CDC or CDC+profile)
#     build-time classes (to be pre-loaded by JavaCodeCompact)
#     test classes
#     demo classes
# 
$(J2ME_CLASSLIB)classes:: .delete.libclasses.list .report.libclasses.list .compile.libclasses

# Compute the complete path for the .java files we need
# to generate
GENERATED_JAVA0 = $(subst .,/,$(GENERATED_CLASSES))
GENERATED_JAVA_FILES = \
	$(patsubst %,$(CVM_DERIVEDROOT)/classes/%.java,$(GENERATED_JAVA0))

generatedclasses : $(GENERATED_JAVA_FILES)

btclasses: generatedclasses .delete.btclasses.list .report.btclasses.list .compile.btclasses

testclasses:: .delete.testclasses.list .report.testclasses.list .compile.testclasses

democlasses:: .delete.democlasses.list .report.democlasses.list .compile.democlasses

#
# Unit-testing related targets
#  these targets are double-colon ones to ease populating.
#

.PHONY: build-unittests
build-unittests::

.PHONY: run-unittests
run-unittests::

# if jarfilename is specified, put jsrclasses in jar file
ifeq ($(OP_JAR_FILENAME),)
jsrclasses:: .compile.jsrclasses
else
jsrclasses:: $(OP_JAR_FILENAME)
endif

#####################################
# include jcc and jcs makefiles
#####################################

include $(CDC_DIR)/build/share/jcc.mk
ifeq ($(CVM_JIT),true)
include $(CDC_DIR)/build/share/jcs.mk
endif

#####################################
# include all of the dependency files
#####################################

files := $(foreach file, $(wildcard $(CVM_OBJDIR)/*.d), $(file))
ifneq ($(strip $(files)),)
    include $(files)
endif

#####################################
# 1) Initialize build.
# 2) Compile the build-time classes and zip them
# 3) Compile the library classes and jar them
# 4) Compile the test classes and zip them
# 5) Create any needed jni headers, including those that JCC creates.
# 6) Create any dependencies the profile addes (like shared libraries)
# 7) Build the CVM
# 8) Create miscellneous files needed for the installation.
#####################################

# As a performance improvement, evaluate some flags in case
# they contain shell commands.
# NOTE: Disabled because this causes GCI build failures
#$(J2ME_CLASSLIB):: CPPFLAGS := $(CPPFLAGS)

$(J2ME_CLASSLIB):: initbuild
$(J2ME_CLASSLIB):: btclasses $(CVM_BUILDTIME_CLASSESZIP)
$(J2ME_CLASSLIB):: $(J2ME_CLASSLIB)classes $(LIB_CLASSESJAR)
ifneq ($(JSR_CLASSES),)
$(J2ME_CLASSLIB):: jsrclasses
endif
ifeq ($(JAVASE),)
$(J2ME_CLASSLIB):: testclasses $(CVM_TEST_CLASSESZIP)
$(J2ME_CLASSLIB):: democlasses $(CVM_DEMO_CLASSESJAR)
endif
ifeq ($(USE_JUMP), true)
$(J2ME_CLASSLIB):: jumptargets
endif
$(J2ME_CLASSLIB):: $(JSROP_JARS) $(JSROP_AGENT_JARS)
$(J2ME_CLASSLIB):: headers $(CVM_ROMJAVA_LIST)
ifeq ($(CVM_STATICLINK_TOOLS),true)
$(J2ME_CLASSLIB):: tools
endif
$(J2ME_CLASSLIB):: $(CLASSLIB_DEPS)
$(J2ME_CLASSLIB):: aotdeps
$(J2ME_CLASSLIB):: $(CVM_BINDIR)/$(CVM)
ifneq ($(CVM_RESOURCES_DEPS),)
$(J2ME_CLASSLIB):: $(CVM_RESOURCES_JAR)
endif
ifeq ($(JAVASE),)
ifeq ($(CDC_10),true)
$(J2ME_CLASSLIB):: $(CVM_TZDATAFILE)
endif
$(J2ME_CLASSLIB):: $(CVM_MIMEDATAFILE) $(CVM_PROPS_BUILD) $(CVM_POLICY_BUILD)
ifeq ($(CVM_DUAL_STACK), true)
$(J2ME_CLASSLIB):: $(CVM_MIDPFILTERCONFIG) $(CVM_MIDPCLASSLIST) $(JSR_CDCRESTRICTED_CLASSLIST)
endif
ifeq ($(CVM_CREATE_RTJAR), true)
$(J2ME_CLASSLIB):: $(CVM_RT_JAR)
endif
endif

#####################################
# make empty.mk depend on CVM_SRCDIRS
# This will cause make to re-start if it needs to create any 
# dynamically created directories, so that vpath directives will work
#####################################

$(CVM_DERIVEDROOT)/empty.mk: $(CVM_SRCDIRS) $(JAVA_SRCDIRS)
	touch $(CVM_DERIVEDROOT)/empty.mk
-include $(CVM_DERIVEDROOT)/empty.mk


#####################################
# See if any build flags have toggled since the last build. If so, then
# delete anything that might be dependent on the build flags. 
#####################################
CVM_FLAGS := $(sort $(CVM_FLAGS))
checkflags:: $(CVM_DERIVEDROOT) remove_build_flags $(CVM_FLAGS) $(CVM_BUILD_DEFS_H) $(CVM_BUILD_DEFS_MK)

remove_build_flags:
	@rm -rf $(CVM_BUILD_FLAGS_FILE)

# Make sure all of the build flags files are up to date. If not
# then do the requested cleanup action.
#
# Also, add the option to $(CVM_BUILD_FLAGS_FILE) so we can restore
# it on the next build if CVM_REBUILD=true is specified.
$(CVM_FLAGS):: $(CVM_DERIVEDROOT)/flags
	@if [ ! -f $(CVM_DERIVEDROOT)/flags/$@.$($@) ]; then \
		echo "Flag $@ changed. Cleaning up."; \
		rm -f $(CVM_DERIVEDROOT)/flags/$@.*; \
		touch $(CVM_DERIVEDROOT)/flags/$@.$($@); \
		ACTION='$($@_CLEANUP_ACTION)'; \
		if [ "$$ACTION" = "" ]; then \
		    $(CVM_DEFAULT_CLEANUP_ACTION); \
		else \
		    eval $$ACTION; \
		fi; \
		rm -f $(CVM_BUILD_DEFS_H); \
	fi
	@echo $@=$($@) >> $(CVM_BUILD_FLAGS_FILE)

#
# Generate $(CVM_BUILD_DEFS_H) file
#
CVM_BUILD_DEFS_FLAGS += \
	$(foreach flag,$(strip $(CVM_FLAGS)), "$(flag)=$($(flag))\n")

CVM_BUILD_DEFS_VARS += \
	$(foreach flag,$(strip $(CVM_BUILD_DEF_VARS)), '$(flag)	$($(flag))')

$(CVM_BUILD_DEFS_H): $(wildcard $(CDC_DIR)/build/share/id*.mk)
	@echo ... generating $@
	@echo "/*** Definitions generated at build time ***/" > $@
	@echo "#ifndef _BUILD_DEFS_H" >> $@
	@echo "#define _BUILD_DEFS_H" >> $@
	@echo >> $@
	@printf "#define CVM_BUILD_OPTIONS %c\n"  '\\' >> $@
	@for s in  $(CVM_BUILD_DEFS_FLAGS) ; do \
		printf "\t\"%s\" %c\n" "$$s" '\\' ; \
	done >> $@ 
	@echo >> $@
	@for s in  $(CVM_BUILD_DEFS_VARS) ; do \
		printf "#define %s\n" "$$s" ; \
	done >> $@
	@echo >> $@
	@echo "#endif /* _BUILD_DEFS_H */" >> $@
	@echo
	@cat $(CVM_BUILD_DEFS_H)
	@echo

$(CVM_BUILD_DEFS_MK)::
	$(AT) echo updating $@ ...
	$(AT) echo "#This file is auto-generated by the CVM build process."> $@
	$(AT) echo "#Do not edit." >> $@
	$(AT) echo "CVM_DEFINES = $(CVM_DEFINES)" >> $@
	$(AT) echo "J2ME_CLASSLIB = $(J2ME_CLASSLIB)" >> $@
	$(AT) echo "CVM_MTASK = $(CVM_MTASK)" >> $@
	$(AT) echo "CVM_INCLUDE_JUMP = $(CVM_INCLUDE_JUMP)" >> $@
	$(AT) echo "USE_JUMP = $(USE_JUMP)" >> $@
	$(AT) echo "JUMP_DIR = $(JUMP_DIR)" >> $@
	$(AT) echo 'CVM_PRELOAD_LIB = $(CVM_PRELOAD_LIB)' >> $@
	$(AT) echo "CVM_DLL = $(CVM_DLL)" >> $@
	$(AT) echo "CVM_STATICLINK_LIBS = $(CVM_STATICLINK_LIBS)" >> $@
	$(AT) echo "CCFLAGS_SPEED = $(CCFLAGS_SPEED)" >> $@
	$(AT) echo "" >> $@

#####################################
# system_properties.c
#####################################

#
# Generate the system_properties.c file, which is included by System.c
# when initialising system properties. This allows profiles to add their
# own system properties by adding them to the SYSTEM_PROPERTIES variable.
# This sets the properties defined by the SYSTEM_PROPERTIES variable.
#
# Also, for each library listed in BUILTIN_LIBS, a property called
# java.library.builtin.<libname> is set to true so that when a 
# System.loadLibray is performed for one of these libraries, it won't
# try to find and load the library.
#
SYSTEM_PROPERTIES_C = $(CVM_DERIVEDROOT)/javavm/runtime/system_properties.c
.generate.system_properties.c:
	$(AT) echo "/* This file is included by System.c to setup system properties. */" > $(CVM_BUILD_TOP)/.system_properties.c
	$(AT) echo "/* AUTO-GENERATED - DO NOT EDIT */" >> $(CVM_BUILD_TOP)/.system_properties.c
	$(AT) echo "" >> $(CVM_BUILD_TOP)/.system_properties.c
	$(AT) if [ "$(SYSTEM_PROPERTIES)" != "" ] ; then \
		for s in $(SYSTEM_PROPERTIES)"" ; do \
			printf "%s\n" "$$s" | sed -e 's/\([^=]*\)=\(.*\)/PUTPROP (props, "\1", "\2");/' ; \
		done ; \
	 fi >> $(CVM_BUILD_TOP)/.system_properties.c
	$(AT) echo "" >> $(CVM_BUILD_TOP)/.system_properties.c
	$(AT) if [ "$(BUILTIN_LIBS)" != "" ] ; then \
		echo "/* Defined properties for builtin libraries. */" ; \
		for s in "$(BUILTIN_LIBS)" ; do \
			printf "PUTPROP(props, \"java.library.builtin.%s\", \"true\");\n" $$s; \
		done ; \
	 fi >> $(CVM_BUILD_TOP)/.system_properties.c
	$(AT) if ! cmp -s $(CVM_BUILD_TOP)/.system_properties.c $(SYSTEM_PROPERTIES_C); then \
		echo ... $(SYSTEM_PROPERTIES_C); \
		cp -f $(CVM_BUILD_TOP)/.system_properties.c $(SYSTEM_PROPERTIES_C); \
	fi

#####################################
# create directories
#####################################

$(CVM_BUILDDIRS):
	@echo ... mkdir $@
	@if [ ! -d $@ ]; then mkdir -p $@; fi

#####################################
# Compile C, C++, and assembler source
#####################################

#
# vpath
#
ifneq ($(CVM_TOOLS_BUILD), true)
vpath %.c	$(CVM_ALL_NATIVE_SRCDIRS)
vpath %.cc 	$(CVM_ALL_NATIVE_SRCDIRS)
vpath %.cpp 	$(CVM_ALL_NATIVE_SRCDIRS)
vpath %.S 	$(CVM_ALL_NATIVE_SRCDIRS)
endif

#
# Make sure opcodes.h gets generated before executejava.o gets built
#
$(CVM_OBJDIR)/executejava.o: $(CVM_DERIVEDROOT)/javavm/include/opcodes.h

# command to use to generate dependency makefiles if requested
ifeq ($(GENERATEMAKEFILES), true)
GENERATEMAKEFILES_CMD = \
	@$(TARGET_CC) $(CCDEPEND) $(CC_ARCH_FLAGS) $(CPPFLAGS) $< \
		2> /dev/null | sed 's!$*\.o!$(dir $@)&!g' > $(@:.o=.d)
endif

# command to use to generate stack map analysis files if requested
ifeq ($(CVM_CSTACKANALYSIS), true)
CSTACKANALYSIS_CMD = \
	$(AT)$(TARGET_CC) -S $(CCFLAGS) $(CPPFLAGS) -o $(@:.o=.asm) $<
endif

#
# rules for compiling
#
COMPILE_FLAVOR = SPEED

$(CVM_OBJECTS_SPEED): COMPILE_FLAVOR = SPEED
$(CVM_OBJECTS_SPACE): COMPILE_FLAVOR = SPACE
$(CVM_OBJECTS_LOOP): COMPILE_FLAVOR = LOOP

ifneq ($(CVM_PROVIDE_TARGET_RULES), true)
$(CVM_OBJDIR)/%.o: %.cc
	@echo "c++ $<"
	$(CCC_CMD_$(COMPILE_FLAVOR))
	$(GENERATEMAKEFILES_CMD)
	$(CSTACKANALYSIS_CMD)

$(CVM_OBJDIR)/%.o: %.cpp
	@echo "c++ $<"
	$(CCC_CMD_$(COMPILE_FLAVOR))
	$(GENERATEMAKEFILES_CMD)
	$(CSTACKANALYSIS_CMD)

$(CVM_OBJDIR)/%.o: %.c
	@echo "cc  $<"
	$(CC_CMD_$(COMPILE_FLAVOR))
	$(GENERATEMAKEFILES_CMD)
	$(CSTACKANALYSIS_CMD)


$(CVM_OBJDIR)/%.o: %.S
	@echo "as  $<"
	$(ASM_CMD)
ifeq ($(GENERATEMAKEFILES), true)
	@$(TARGET_CC) $(ASM_ARCH_FLAGS) $(CCDEPEND) $(CPPFLAGS) $< \
		2> /dev/null | sed 's!$*\.o!$(dir $@)&!g' > $(@:.o=.d)
endif
ifeq ($(CVM_CSTACKANALYSIS), true)
	cp $< $(@:.o=.asm)
endif
endif

#
# floating point code often needs to be compiled with special flags
# because of bugs in gcc. This is the case with on x86 where we
# need to compile with -ffloat-store to avoid some gcc bugs.
#
CVM_FDLIBM_SRCDIR = $(CVM_SHAREROOT)/native/java/lang/fdlibm/src
CVM_FDLIB	  = $(CVM_OBJDIR)/fdlibm.a

ifneq ($(CVM_PROVIDE_TARGET_RULES), true)
$(CVM_FDLIB_FILES): $(CVM_OBJDIR)/%.o: $(CVM_FDLIBM_SRCDIR)/%.c
	@echo "cc  $<"
	$(CC_CMD_FDLIB)
ifeq ($(GENERATEMAKEFILES), true)
	@$(TARGET_CC) $(CC_ARCH_FLAGS) $(CCDEPEND) $(CPPFLAGS) $< \
		2> /dev/null | sed 's!$*\.o!$(dir $@)&!g' > $(@:.o=.d)
endif

$(CVM_FDLIB): $(CVM_FDLIB_FILES)
	@echo lib $@
	$(AT)$(call TARGET_AR_CREATE,$@) $(call POSIX2HOST,$^)
	$(AT)$(call TARGET_AR_UPDATE,$@)
endif

#
# If CVM_AOT is enabled, compute the SHA-1 checksum for the CVM.
# A .c file that contains the checksum value is generated,
# compiled and linked into CVM.
#
ifeq ($(CVM_AOT), true)
CVM_SHA1OBJ	= $(CVM_OBJDIR)/cvm_sha1.o
CVM_SHA1	= $(CVM_DERIVEDROOT)/javavm/runtime/cvm_sha1.c

$(CVM_SHA1): FORCE
	$(AT)$(JAVAC_CMD) -d $(CVM_MISC_TOOLS_CLASSPATH) \
		$(CVM_MISC_TOOLS_SRCDIR)/sha1/sha1.java; \
	echo "Computing CVM SHA-1 checksum ..."; \
	$(call TARGET_AR_CREATE,cvm.a) $(CVM_OBJECTS) $(CVM_OBJDIR)/$(CVM_ROMJAVA_O) $(CVM_FDLIB); \
	sum=`($(CVM_JAVA) \
		-classpath $(CVM_MISC_TOOLS_CLASSPATH) \
		sha1 cvm.a)`; \
	printf "static const char *cvmsha1sum = \"%s\";\n" $$sum > $@; \
	printf "const char* CVMgetSha1Hash() {\n" >> $@; \
	printf "    return cvmsha1sum;\n" >> $@; \
	printf "}\n" >> $@; \
	rm -rf cvm.a

FORCE:

else
CVM_SHA1OBJ	=
endif

#####################################
# Build CVM and everything it needs
#####################################

#
# Initialize the environment for the build process: 
#
initbuild: checkflags $(CVM_BUILDDIRS) initbuild_profile
initbuild_profile ::

# make sure we build the headers before the objects
$(CVM_BINDIR)/$(CVM) :: $(CVM_ROMJAVA_LIST) # jcc creates some of the headers
ifneq ($(CVM_PRELOAD_LIB), true)
$(CVM_BINDIR)/$(CVM) :: headers
endif

$(CVM_BINDIR)/$(CVM) :: .generate.system_properties.c

LINKCVM_OBJECTS = \
	$(CVM_OBJECTS) $(CVM_OBJDIR)/$(CVM_ROMJAVA_O) $(CVM_FDLIB) \
	$(CVM_SHA1OBJ) $(CVM_RESOURCES)

ifneq ($(CVM_PROVIDE_TARGET_RULES), true)
$(CVM_BINDIR)/$(CVM) :: $(LINKCVM_OBJECTS) $(LINKCVM_EXTRA_DEPS)
	@echo "Linking $@"
	$(call CVM_LINK_CMD, $(LINKCVM_OBJECTS) $(LINKCVM_EXTRA_OBJECTS), $(LINKCVM_LIBS))
	@echo "Done Linking $@"
endif

#####################################
# cleanup
#####################################

# Rerun make so tool makefiles (jcov, hprof, and jdwp) are included
clean::
	$(MAKE) CVM_TOOLS_BUILD=true tool-clean

ifeq ($(CVM_REBUILD), true)
clean::
	rm -rf $(INSTALLDIR)
	rm -rf $(CVM_BUILD_TOP)/.*classes
	rm -rf $(CVM_BUILD_TOP)/.*.list
	rm -rf $(CVM_BUILD_TOP)/.*.clist
	rm -rf $(CVM_BUILD_TOP)/.system_properties.c
	rm -rf $(CVM_BUILD_FLAGS_FILE)
	rm -rf $(BUILDFLAGS_JAVA)
	rm -rf $(CVM_BUILDTIME_CLASSESDIR) \
	       $(CVM_TEST_CLASSESDIR) $(CVM_DEMO_CLASSESDIR) *_classes
	rm -rf $(CVM_BUILDTIME_CLASSESZIP) \
	       $(CVM_TEST_CLASSESZIP) $(CVM_DEMO_CLASSESJAR)
	rm -rf $(CVM_LIBDIR)
	rm -rf $(CVM_BUILDDIRS) $(CVM_BINDIR)/$(CVM)
	rm -rf $(CVM_JCC_CLASSPATH)
	rm -rf $(CVM_DERIVEDROOT)
	rm -rf $(CVM_PROPS_BUILD) $(CVM_POLICY_BUILD)
	rm -rf $(CVM_JCS_BUILDDIR)
	rm -rf $(CVM_RESOURCES_DIR)
else
clean::
	$(MAKE) CVM_REBUILD=true clean
endif

#####################################
# zip or jar class files
#####################################

CVM_CLASSES_TMP = $(CVM_BUILD_TOP)/.classes.tmp

$(CVM_BUILDTIME_CLASSESZIP): $(CVM_BUILD_TOP)/.btclasses
	@echo ... $@
ifeq ($(CVM_PRELOAD_FULL_CLOSURE), true)
	$(AT)(cd $(CVM_BUILDTIME_CLASSESDIR); $(ZIP) -r -0 -q - * ) \
		> $(CVM_CLASSES_TMP)
	$(AT)mv -f $(CVM_CLASSES_TMP) $@
else
	$(AT)mv -f $@ $(CVM_BUILD_TOP)/.classes.tmp || true
	@cp $(CVM_BUILD_TOP)/.btclasses.clist $(CVM_BUILDTIME_CLASSESDIR)
	$(AT)(cd $(CVM_BUILDTIME_CLASSESDIR); \
	    $(ZIP) -r -D -u -0 -q ../.classes.tmp . -i@.btclasses.clist)
	$(AT)mv -f $(CVM_BUILD_TOP)/.classes.tmp $@
endif

ifeq ($(JAVASE),)

$(CVM_TEST_CLASSESZIP): $(CVM_BUILD_TOP)/.testclasses
	@echo ... $@
	$(AT)(cd $(CVM_TEST_CLASSESDIR); $(ZIP) -r -0 -q - * ) \
		> $(CVM_CLASSES_TMP)
	$(AT)mv -f $(CVM_CLASSES_TMP) $@

$(CVM_DEMO_CLASSESJAR): $(CVM_BUILD_TOP)/.democlasses
	@echo ... $@
	$(AT)for dir in $(CVM_DEMOCLASSES_SRCDIRS); do \
	    files=`(cd $$dir; find . -name .svn -prune -o -type f -print)`; \
	    (cd $$dir; tar -cf - $$files) | \
		(cd $(CVM_DEMO_CLASSESDIR); tar -xf - ); \
	done 
	$(AT)(cd $(CVM_DEMO_CLASSESDIR); $(ZIP) -r -q - *) > $(CVM_CLASSES_TMP)
	$(AT)mv -f $(CVM_CLASSES_TMP) $@

endif


# Create resources jar file
ifneq ($(CVM_RESOURCES_DEPS),)
CVM_JARFILES := "$(CVM_RESOURCES_JAR_FILENAME)", $(CVM_JARFILES)
$(CVM_RESOURCES_JAR): $(CVM_RESOURCES_DEPS)
	@echo ... $@
	$(AT)(cd $(CVM_RESOURCES_DIR); $(CVM_JAR) cf $(call POSIX2HOST, $@) *)
endif

#
# Create the profile jar file if there is anything in the profile
# classes directory.
#
ifneq ($(CVM_BUILD_LIB_CLASSESJAR), true)
$(LIB_CLASSESJAR):
else
$(LIB_CLASSESJAR): $(CVM_BUILD_TOP)/.libclasses
	@echo ... $@	
	$(AT)(cd $(LIB_CLASSESDIR); $(CVM_JAR) cf $(call POSIX2HOST, $@) *)
endif

#####################################
# Run JAVAH.
#####################################

#
# If this is a not fully romized build, then we need to use javah to create
# the jni headers for the runtime classes since JCC won't have seen these.
#
# NOTE: we could probably fix JCC so that we can generate
# offset and CNI header files separately too.
#
ifeq ($(CVM_PRELOAD_LIB), true)
headers:
else
headers: $(CVM_DERIVEDROOT)/jni/.time.stamp

JAVAH_HOST_BT_PATH = $(call POSIX2HOST,$(CVM_BUILDTIME_CLASSESZIP))  
JAVAH_HOST_CP_PATH = $(call POSIX2HOST,$(LIB_CLASSESJAR))$(JSR_JNI_CLASSPATH)

$(CVM_DERIVEDROOT)/jni/.time.stamp : $(LIB_CLASSESJAR)
	$(AT)if [ -s $(CVM_BUILD_TOP)/.javahclasses.list ] ; then	\
		echo ... generating jni class headers ;			\
		$(CVM_JAVAH) -jni					\
			-d $(CVM_DERIVEDROOT)/jni			\
			-bootclasspath $(JAVAH_HOST_BT_PATH)		\
			-classpath $(JAVAH_HOST_CP_PATH)		\
			$(JSR_JNI_CLASSES) 				\
			@$(CVM_BUILD_TOP)/.javahclasses.list		\
			$(CVM_EXTRA_JNI_CLASSES) ;			\
	fi
	@touch $@
endif

# Only need to copy methodsList.txt when CVM_AOT is true and
# CVM_MTASK is false.
ifeq ($(findstring falsetrue,$(CVM_MTASK)$(CVM_AOT)), falsetrue)
aotdeps:
	$(AT)if [ -s $(CVM_SHAREROOT)/lib/profiles/$(J2ME_CLASSLIB)/methodsList.txt ]; then	\
		rm -rf $(CVM_LIBDIR)/methodsList.txt;	\
		cp $(CVM_SHAREROOT)/lib/profiles/$(J2ME_CLASSLIB)/methodsList.txt $(CVM_LIBDIR);	\
	fi
	$(AT)rm -rf $(CVM_LIBDIR)/cvm.aot
else
ifeq ($(CVM_AOT), true)
aotdeps:
	$(AT)rm -rf $(CVM_LIBDIR)/cvm.aot
else
aotdeps:
endif
endif

#####################################
# Copy tzmappings file to the lib directory. Not all platforms have this
# file, so only do this if CVM_TZDATAFILE is defined.
#####################################

ifeq ($(CDC_10),true)
ifneq ($(CVM_TZDATAFILE), )
$(CVM_TZDATAFILE): $(CVM_TZDIR)/tzmappings
	@echo "Updating tzmappings...";
	@cp -f $< $@;
	@echo "<<<Finished copying $@";
endif
endif

#####################################
# Copy content-types.properties to the lib directory
#####################################

$(CVM_MIMEDATAFILE): $(CVM_MIMEDIR)/content-types.properties
	@echo "Updating default MIME table...";
	@cp -f $< $@;
	@echo "<<<Finished copying $@";

#####################################
# Copy midp member configuration files to the lib directory. 
# Only do this for dual stack support
#####################################

ifeq ($(CVM_DUAL_STACK), true)
ifneq ($(CVM_MIDPFILTERCONFIG), )
$(CVM_MIDPFILTERCONFIG): $(CVM_MIDPDIR)/MIDPFilterConfig.txt
	@echo "Updating MIDPFilterConfig...";
	@cp -f $< $@;

$(CVM_MIDPCLASSLIST): $(CVM_MIDPCLASSLIST_FILES)
	@echo "Updating MIDPPermittedClasses...";
	$(AT)rm -rf $@
	$(AT)cat $^ > $@
endif

###############################################
# Rule for generating dual-stack member filter
###############################################
gen_member_filter:: initbuild btclasses $(CVM_BUILDTIME_CLASSESZIP) 
gen_member_filter:: $(J2ME_CLASSLIB)classes $(LIB_CLASSESJAR) $(CVM_ROMJAVA_LIST)
ifeq ($(CVM_MIDPFILTERINPUT),)
	$(error Need to set CVM_MIDPFILTERINPUT to a valid jar file)
else
	@echo "generating dual-stack member filter ..."
endif

###########################################################
# Generate MIDP_PKG_CHECKER using the RomConfProcessor tool
###########################################################
$(CVM_DERIVEDROOT)/classes/sun/misc/$(MIDP_PKG_CHECKER):
	@echo "... $@"
	$(AT)$(JAVAC_CMD) -d $(CVM_MISC_TOOLS_CLASSPATH) \
		$(CVM_MISC_TOOLS_SRCDIR)/RomConfProcessor/RomConfProcessor.java
	$(AT)$(CVM_JAVA) -classpath $(CVM_MISC_TOOLS_CLASSPATH) \
		RomConfProcessor -dirs $(ROMGEN_INCLUDE_PATHS) \
		-romfiles $(ROMGEN_CFG_FILES)
	$(AT)mv $(MIDP_PKG_CHECKER) $(CVM_DERIVEDROOT)/classes/sun/misc/
endif

ifeq ($(CVM_CREATE_RTJAR), true)
################################################
# Rule for creating rt.jar
################################################
$(CVM_RT_JAR):: $(CVM_RTJARS_LIST)
	@echo "Packaging $@ ..."; \
	mkdir -p $(CVM_BUILD_TOP)/.rtclasses; \
	cd $(CVM_BUILD_TOP)/.rtclasses; \
	for j in $(CVM_RTJARS_LIST); do \
		$(CVM_JAR) xf $$j;	\
	done; \
	$(CVM_JAR) cf $@ *
	$(AT)rm -rf $(CVM_BUILD_TOP)/.rtclasses
endif

################################################
# Rules for building documentation
################################################

update_docs:: lib-src
	@echo ">>>Updating "$@" ..." ;
	(cd $(INSTALLDIR); $(ZIP) -r -u -q install/$(J2ME_CLASSLIB)-src.zip doc -x $(EXCLUDE_PATTERNS) ) ;
	@echo "<<<Finished "$@" ..." ;

$(INSTALLDIR)/javadoc:
	@echo ... mkdir $@
	@mkdir -p $@

javadoc.zip: javadoc-$(J2ME_CLASSLIB) $(OPT_PKGS_JAVADOC_RULES)
	@echo ">>>Making "$@" ..." ;
	(cd $(INSTALLDIR); \
	$(ZIP) -r -q - javadoc) \
	> $(INSTALLDIR)/javadoc.zip;
	@echo "<<<Finished "$@" ..." ;

################################################
# Install security-related files
################################################

ifeq ($(DO_SECURITY_PROVIDER_FILTERING),false)
$(CVM_PROPS_BUILD): $(CVM_PROPS_SRC)
	@echo "Updating java.security file...";
	@cp -f $< $@;
	@echo "<<<Finished copying $@";
else
$(CVM_PROPS_BUILD): $(CVM_PROPS_SRC)
	@echo "Updating java.security file...";
	$(AT)provs="$(SECURITY_PROVIDERS)" ; \
	 if [ -z "$$provs" ]; then \
	     provs="sun.security.provider.Sun" ; \
	 fi ; \
	 awk -F= 'BEGIN { count = 1; } \
	    NR == 1 { \
		 includeCount = split( includedProviders, included, " " ); \
	     } \
	     /^security.provider.[0-9]*/ { \
		for ( i = 1 ; i <= includeCount ; i++ ) { \
		    if ( included[i] == $$2 ) { \
	                printf "security.provider.%d=%s\n", count, $$2; \
	                count++; \
			next; \
		    } \
                }\
	        next; \
             } \
	     { print; }' includedProviders="$$provs" $< > $@ ;
	@echo "<<<Finished updating $@";
endif

$(CVM_POLICY_BUILD): $(CVM_POLICY_SRC)
	@echo "Updating java.policy file...";
	@cp -f $< $@;
	@echo "<<<Finished copying $@";

################################################
# Rule for building binary bundle
################################################

#
# To build the binary bundle, just add the "bin" target to your make command.
# You will also need to set some of the following options:
#
# - JAVAME_LEGAL_DIR: directory containing legal documents. If not set,
#	the makefiles will automatically do an svn checkout of the files
#	from the respository. This will result in a password prompt if you
# 	haven't already authenticated with the repository.
# - BINARY_BUNDLE_NAME: name of the binary bundle, excluding .zip
#	extension.
# - BINARY_BUNDLE_DIRNAME: directory name the binary bundle will unzip
# 	into. Defaults to $(BINARY_BUNDLE_NAME).
# - BINARY_BUNDLE_APPEND_REVISION: Defaults to true. Appends the repository
#	revision number to the end of BINARY_BUNDLE_NAME and 
#	BINARY_BUNDLE_DIRNAME
#
# To build the device  binary bundle, just add the "device_bin" target to your
# make command. You will also need to set some of the following options:
# 
# - DEVICE_BUNDLE_NAME: name of the device binary bundle, excluding .zip
#	extension.
# - DEVICE_BUNDLE_DIRNAME: directory name the device binary bundle will unzip
# 	into. Defaults to $(DEVICE_BUNDLE_NAME).
# - JAVAME_LEGAL_DIR and BINARY_BUNDLE_APPEND_REVISION also work
#       with the device bundle target


# Patterns that we want to bundle. BINARY_BUNDLE_PATTERNS can be appended
# to from other component makefiles if necssary.
BINARY_BUNDLE_PATTERNS += \
	$(CVM_BINDIR)/* \
	$(CVM_LIBDIR)/* \
	$(CVM_DEMO_CLASSESJAR) \
	$(CVM_TEST_CLASSESZIP) \
	$(CVM_BUILD_TOP)/legal

# Replace $(CVM_BUILD_TOP) with $(BINARY_BUNDLE_NAME)
BINARY_BUNDLE_PATTERNS := \
	$(patsubst $(CVM_BUILD_TOP)%,$(BINARY_BUNDLE_DIRNAME)%,$(BINARY_BUNDLE_PATTERNS))

# Replace $(CVM_BUILD_TOP_ABS) with $(BINARY_BUNDLE_NAME)
BINARY_BUNDLE_PATTERNS := \
	$(patsubst $(CVM_BUILD_TOP_ABS)%,$(BINARY_BUNDLE_DIRNAME)%,$(BINARY_BUNDLE_PATTERNS))

CHECK_LEGAL_DIR = \
	if [ ! -d $(JAVAME_LEGAL_DIR) ]; then				\
	    echo '*** JAVAME_LEGAL_DIR must be set to the "legal"'	\
	         'directory. The OSS version can be found at'		\
		 'https://phoneme.dev.java.net/svn/phoneme/legal.';	\
	    exit 2;							\
	fi

.PHONY : bin
bin: all
	@echo ">>>Making binary bundle ..."
	@echo "	BINARY_BUNDLE_APPEND_REVISION = $(BINARY_BUNDLE_APPEND_REVISION)"
	@echo "	BINARY_BUNDLE_NAME	= $(BINARY_BUNDLE_NAME)"
	@echo "	BINARY_BUNDLE_DIRNAME	= $(BINARY_BUNDLE_DIRNAME)"
	@echo "	JAVAME_LEGAL_DIR	= $(JAVAME_LEGAL_DIR)"

	$(AT)mkdir -p $(INSTALLDIR)
	$(AT)rm -rf $(INSTALLDIR)/$(BINARY_BUNDLE_DIRNAME)
	$(AT)mkdir -p $(INSTALLDIR)/$(BINARY_BUNDLE_DIRNAME)
	$(AT)ln -ns $(CVM_BUILD_TOP)/* $(INSTALLDIR)/$(BINARY_BUNDLE_DIRNAME)
	$(AT)rm -rf $(INSTALLDIR)/$(BINARY_BUNDLE_NAME).zip

	$(AT)$(CHECK_LEGAL_DIR)
	$(AT)ln -ns $(JAVAME_LEGAL_DIR) \
		$(INSTALLDIR)/$(BINARY_BUNDLE_DIRNAME)/legal

	$(AT)cp $(CVM_BUILDTIME_CLASSESZIP) $(CVM_LIBDIR)/

	$(AT)(cd $(INSTALLDIR); \
	 $(ZIP) -r -q - $(BINARY_BUNDLE_PATTERNS) -x */.svn/*) \
		> $(INSTALLDIR)/$(BINARY_BUNDLE_NAME).zip;
	$(AT)rm -rf $(INSTALLDIR)/$(BINARY_BUNDLE_DIRNAME)
	@echo "<<<Finished binary bundle" ;

.PHONY :  device_bin

device_bin: all  device_bin_prep device_bin_legal device_bin_zip

device_bin_zip:
	@echo ">>>Making device binary bundle ..."
	@echo "	DEVICE_BUNDLE_NAME	= $(DEVICE_BUNDLE_NAME)"
	@echo "	JAVAME_LEGAL_DIR	= $(JAVAME_LEGAL_DIR)"

	$(AT)(cd $(INSTALLDIR); \
	 $(ZIP) -rq  - $(DEVICE_BUNDLE_DIRNAME) ) \
		> $(INSTALLDIR)/$(DEVICE_BUNDLE_NAME).zip;
	$(AT)rm -rf $(INSTALLDIR)/$(DEVICE_BUNDLE_DIRNAME)
	@echo "<<<Finished device binary bundle" ;

device_bin_prep ::
	$(AT)rm -f $(INSTALLDIR)/$(DEVICE_BUNDLE_NAME).zip
	$(AT)rm -rf $(INSTALLDIR)/$(DEVICE_BUNDLE_DIRNAME)
	$(AT)mkdir -p $(INSTALLDIR)/$(DEVICE_BUNDLE_DIRNAME)

device_bin_legal:
	$(AT)$(CHECK_LEGAL_DIR)
	$(AT)ln -ns $(JAVAME_LEGAL_DIR) \
		$(INSTALLDIR)/$(DEVICE_BUNDLE_DIRNAME)/legal


################################################
# Include target makefiles last
################################################

-include $(CDC_OS_COMPONENT_DIR)/build/$(TARGET_OS)/rules.mk
-include $(CDC_CPU_COMPONENT_DIR)/build/$(TARGET_CPU_FAMILY)/rules.mk
-include $(CDC_OSCPU_COMPONENT_DIR)/build/$(TARGET_OS)-$(TARGET_CPU_FAMILY)/rules.mk
-include $(CDC_DEVICE_COMPONENT_DIR)/build/$(TARGET_OS)-$(TARGET_CPU_FAMILY)-$(TARGET_DEVICE)/rules.mk


######################################
# Handle build flag violations here
######################################

ifeq ($(MAKELEVEL), 0)

ifeq ($(CVM_JIT),true)
ifeq ($(CVM_JVMTI),true)
$(warning JVMTI debugging is not supported in JITed code.  Compiler is turned off if connected to a debugger)
endif
ifeq ($(CVM_JVMPI),true)
$(warning JVMPI is not fully supported in JIT builds. Programs may not behave properly.)
endif
endif

endif

################################################
# sun.misc.DefaultLocaleList.java
################################################

# 
# Generate sun.misc.DefaultLoacleList.java which contains a list 
# of default locales in the system. The list is generated by parsing
# CVM_BUILDTIME_CLASSES list, which contains the romized classes.
#

ifeq ($(CDC_10),true)
LOCALE_ELEMENTS_PREFIX = java.text.resources.LocaleElements_
else
LOCALE_ELEMENTS_PREFIX = sun.text.resources.LocaleElements_
endif
LOCALE_ELEMENTS_LIST = $(patsubst $(LOCALE_ELEMENTS_PREFIX)%,%,$(filter $(LOCALE_ELEMENTS_PREFIX)%,$(CVM_BUILDTIME_CLASSES)))

DEFAULTLOCALELIST_JAVA_TMP = $(CVM_DERIVEDROOT)/.DefaultLocaleList.java
$(DEFAULTLOCALELIST_JAVA):
	@echo ... generating sun.misc.DefaultLocaleList.java
	$(AT) echo "/* This file is used by LocaleData.java */" > $(DEFAULTLOCALELIST_JAVA_TMP)
	$(AT) echo "/* AUTO-GENERATED - DO NOT EDIT */" >> $(DEFAULTLOCALELIST_JAVA_TMP)
	$(AT) echo "" >> $(DEFAULTLOCALELIST_JAVA_TMP)
	$(AT) echo "package sun.misc; " >> $(DEFAULTLOCALELIST_JAVA_TMP)
	$(AT) echo "" >> $(DEFAULTLOCALELIST_JAVA_TMP)
	$(AT) echo "public class DefaultLocaleList { " >> $(DEFAULTLOCALELIST_JAVA_TMP)
	$(AT) echo "   public final static String list[] = { " >> $(DEFAULTLOCALELIST_JAVA_TMP)
	$(AT) if [ "$(LOCALE_ELEMENTS_LIST)" != "" ] ; then \
		for s in "$(LOCALE_ELEMENTS_LIST)" ; do \
			printf "\t\"%s\", " $$s; \
		done ; \
	 fi >> $(DEFAULTLOCALELIST_JAVA_TMP)
	$(AT) printf "\t};" >> $(DEFAULTLOCALELIST_JAVA_TMP)
	$(AT) echo "}" >> $(DEFAULTLOCALELIST_JAVA_TMP)
	$(AT) echo "" >> $(DEFAULTLOCALELIST_JAVA_TMP)
	$(AT) if ! cmp -s $(DEFAULTLOCALELIST_JAVA_TMP) $(DEFAULTLOCALELIST_JAVA); then \
		echo ... $(DEFAULTLOCALELIST_JAVA); \
		cp -f $(DEFAULTLOCALELIST_JAVA_TMP) $(DEFAULTLOCALELIST_JAVA); \
	fi
	$(AT) rm $(DEFAULTLOCALELIST_JAVA_TMP)

#####################################
# BuildFlags.java
#####################################

ifneq ($(CDC_10),true)

#
# Generate the BuildFlags.java file, which consists of one field,
# final static boolean qAssertEnabled.
#
# The value of this field corresponds to CVM_DEBUG_ASSERTS.
#

BUIDLFLAGS_JAVA_TMP = $(CVM_DERIVEDROOT)/.BuildFlags.java
BUILDFLAGS_JAVA = $(CVM_DERIVEDROOT)/classes/sun/misc/BuildFlags.java
$(BUILDFLAGS_JAVA): 
	@echo ... generating BuildFlags.java
	$(AT) echo "/* This file contains information determined at a build time*/" > $(BUIDLFLAGS_JAVA_TMP)
	$(AT) echo "/* AUTO-GENERATED - DO NOT EDIT */" >> $(BUIDLFLAGS_JAVA_TMP)
	$(AT) echo "" >> $(BUIDLFLAGS_JAVA_TMP)
	$(AT) echo "package sun.misc; " >> $(BUIDLFLAGS_JAVA_TMP)
	$(AT) echo "" >> $(BUIDLFLAGS_JAVA_TMP)
	$(AT) echo "public class BuildFlags { " >> $(BUIDLFLAGS_JAVA_TMP)
	$(AT) echo "   public final static boolean qAssertsEnabled = $(CVM_DEBUG_ASSERTS); " >> $(BUIDLFLAGS_JAVA_TMP)
	$(AT) echo "}" >> $(BUIDLFLAGS_JAVA_TMP)
	$(AT) echo "" >> $(BUIDLFLAGS_JAVA_TMP)
	$(AT) if ! cmp -s $(BUIDLFLAGS_JAVA_TMP) $(BUILDFLAGS_JAVA); then \
		echo ... $(BUILDFLAGS_JAVA); \
		cp -f $(BUIDLFLAGS_JAVA_TMP) $(BUILDFLAGS_JAVA); \
	fi
	$(AT) rm $(BUIDLFLAGS_JAVA_TMP)

endif

#############################
# Source directory
############################

#
# This is a bit different than just building the source bundle like
# bundle.mk does. We actually need to copy the sources to the directory
# specified by SOURCE_OUTPUT_DIR. However, we still use the bundle.mk
# support to get all the needed files into a zip bundle first.
#

# The following can all be set on the make command line
SOURCE_OUTPUT_DIR	= $(INSTALLDIR)/$(J2ME_BUILD_VERSION)
CDC_SOURCE_OUTPUT_SUBDIR= cdc
INCLUDE_JIT		= $(CVM_JIT)
INCLUDE_MTASK		= $(CVM_MTASK)
INCLUDE_DUALSTACK	= $(CVM_DUAL_STACK)
INCLUDE_JCOV		= $(CVM_CLASSLIB_JCOV)
INCLUDE_GCI		= $(USE_GCI)
BUNDLE_PORTS 		= "$(TARGET_OS)-$(TARGET_CPU_FAMILY)-*"
#BUNDLE_PORTS 		= "linux-x86-* linux-arm-*"

# The following are meant for internal use only
SRC_BUNDLE_NAME		= $(CDC_SOURCE_OUTPUT_SUBDIR)
SRC_BUNDLE_DIRNAME	= $(SRC_BUNDLE_NAME)
SRC_BUNDLE_APPEND_REVISION = false

ifneq ($(patsubst /%,/,$(SOURCE_OUTPUT_DIR)),/)
$(error SOURCE_OUTPUT_DIR must be an absolute path: $(SOURCE_OUTPUT_DIR))
endif

# flags we need to pass to bundle.mk
BUNDLE_FLAGS += J2ME_CLASSLIB=$(J2ME_CLASSLIB)
BUNDLE_FLAGS += INCLUDE_JIT=$(INCLUDE_JIT)
BUNDLE_FLAGS += INCLUDE_MTASK=$(INCLUDE_MTASK)
BUNDLE_FLAGS += INCLUDE_DUALSTACK=$(INCLUDE_DUALSTACK)
BUNDLE_FLAGS += INCLUDE_JCOV=$(INCLUDE_JCOV)
BUNDLE_FLAGS += INCLUDE_GCI=$(INCLUDE_GCI)
BUNDLE_FLAGS += BUNDLE_PORTS="$(BUNDLE_PORTS)"
BUNDLE_FLAGS += SRC_BUNDLE_NAME=$(SRC_BUNDLE_NAME)
BUNDLE_FLAGS += SRC_BUNDLE_DIRNAME=$(SRC_BUNDLE_DIRNAME)
BUNDLE_FLAGS += SRC_BUNDLE_APPEND_REVISION=$(SRC_BUNDLE_APPEND_REVISION)
BUNDLE_FLAGS += SOURCE_OUTPUT_DIR=$(SOURCE_OUTPUT_DIR)
BUNDLE_FLAGS += JAVAME_LEGAL_DIR=$(JAVAME_LEGAL_DIR)
BUNDLE_FLAGS += USE_CDC_COM=$(USE_CDC_COM)
BUNDLE_FLAGS += CDC_COM_DIR=$(CDC_COM_DIR)
BUNDLE_FLAGS += USE_VERBOSE_MAKE=$(USE_VERBOSE_MAKE)
BUNDLE_FLAGS += AT=$(AT)
BUNDLE_FLAGS += INSTALLDIR=$(INSTALLDIR)

# Build the cdc source bundle
source_bundle::
	@echo " ... cdc source bundle"
	$(AT)cd $(CVM_TOP)/build/share
	$(AT)$(MAKE) $(MAKE_NO_PRINT_DIRECTORY) \
		     -f $(CVM_TOP)/build/share/bundle.mk $(BUNDLE_FLAGS)
	$(AT)$(UNZIP) -q $(INSTALLDIR)/$(CDC_SOURCE_OUTPUT_SUBDIR).zip \
	      -d $(SOURCE_OUTPUT_DIR)
	$(AT)rm $(INSTALLDIR)/$(CDC_SOURCE_OUTPUT_SUBDIR).zip

# Not only trigger source_bundle rules, but also setup the legal
# directory properly, and zip it all up.
source_bundles::  clean_source_bundles source_bundle_dir source_bundle
	@echo " ... moving legal docs to source bundle root"
	$(AT)mv $(SOURCE_OUTPUT_DIR)/cdc/legal $(SOURCE_OUTPUT_DIR)/legal
	@echo " ... creating $(SOURCE_OUTPUT_DIR).zip"
	$(AT)rm -rf $(SOURCE_OUTPUT_DIR).zip
	$(AT)(cd $(SOURCE_OUTPUT_DIR)/..; $(ZIP) -r -q $(notdir $(SOURCE_OUTPUT_DIR)).zip $(notdir $(SOURCE_OUTPUT_DIR)))

source_bundle_dir:
	@echo " ... creating $(SOURCE_OUTPUT_DIR)"
	$(AT)mkdir -p $(SOURCE_OUTPUT_DIR)

clean_source_bundles:
	@echo " ... removing $(SOURCE_OUTPUT_DIR)"
	$(AT)rm -rf $(SOURCE_OUTPUT_DIR)

