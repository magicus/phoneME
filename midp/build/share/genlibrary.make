#
#   
#
# Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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
######################################################################
#
# This makefile generates the main makefile library.make.
#
# Following variables should have been set before including this file:
#  LIBRARY   - name of a library to generate makefile for
#  LIB_IMPL  - name of the implementation selected for the library
#
######################################################################

.PHONY: all clean

# Make sure all is the default target
all:

include $(JWC_WORK_SPACE)/build/share/libdefs.gmk

#
# Include library descriptor and implementation descriptor. 
#
include $(JWC_WORK_SPACE)/src/$(LIBRARY)/libexport.gmk
include $(JWC_WORK_SPACE)/src/$(LIBRARY)/$(LIB_IMPL)/libinfo.gmk

LIB_BLD_PATH=$(JWC_BUILD_SPACE)/$(LIBRARY)

LIB_SRC_PATH=$(JWC_WORK_SPACE)/src/$(LIBRARY)

LIB_MAKEFILE=$(LIB_BLD_PATH)/library.make

#
# List of library.make makefiles for libraries which this library depends on
#
DEPENDENCY_MAKEFILES= \
   $(addprefix $(JWC_BUILD_SPACE)/, $(addsuffix /library.make, $(LIB_DEPENDENCIES)))

#
# List of <library>_all targets for libraries which this library depends on
#
DEPENDENCIES_ALL=$(addsuffix _all, $(LIB_DEPENDENCIES))

#
# List of <library>_clean targets for libraries which this library depends on
#
DEPENDENCIES_CLEAN=$(addsuffix _clean, $(LIB_DEPENDENCIES))

#
# Library name with slashed replaced with undescores, used to name the native library
#
LIB_NAME=$(subst /,_,$(LIBRARY))

######################################################################
#
# Java compilation definitions
#
######################################################################

LIB_CLASS_DIR=$(LIB_BLD_PATH)/impl_classes
LIB_CLASSES_ZIP=$(LIB_BLD_PATH)/impl_classes.zip
LIB_CLASS_LIST=$(LIB_BLD_PATH)/impl_classlist
LIB_TEMP_CLASS_LIST=$(LIB_BLD_PATH)/impl_classlist.tmp

#
# Full paths to Java implementation files for the selected implementation
#
LIB_JAVA_PATHS=$(addprefix $(LIB_SRC_PATH)/$(LIB_IMPL)/classes/,$(LIB_JAVA_FILES))

LIB_EXPORT_CLASS_DIR=$(LIB_BLD_PATH)/export_classes
LIB_EXPORT_CLASSES_ZIP=$(LIB_BLD_PATH)/export_classes.zip
LIB_EXPORT_CLASS_LIST=$(LIB_BLD_PATH)/export_classlist
LIB_TEMP_EXPORT_CLASS_LIST=$(LIB_BLD_PATH)/export_classlist.tmp

#
# Full paths to Java interface files for this library
#
LIB_EXPORT_JAVA_PATHS=$(addprefix $(LIB_SRC_PATH)/classes/,$(LIB_EXPORT_JAVA_FILES))

LIB_I3TEST_CLASS_DIR=$(LIB_BLD_PATH)/i3test_classes
LIB_I3TEST_CLASSES_ZIP=$(LIB_BLD_PATH)/i3test_classes.zip
LIB_I3TEST_CLASS_LIST=$(LIB_BLD_PATH)/i3test_classlist
LIB_TEMP_I3TEST_CLASS_LIST=$(LIB_BLD_PATH)/i3test_classlist.tmp

#
# Full paths to Java I3 test files for this implementation
#
LIB_I3TEST_JAVA_PATHS=$(addprefix $(LIB_SRC_PATH)/$(LIB_IMPL)/i3test/,$(LIB_I3TEST_FILES))

#
# Way to specify an space as an argument  for subst
#
EMPTY=
ONESPACE=$(EMPTY) $(EMPTY)

#
# Convert a space-separated list of libraries to a pathsep-separated list 
# of interface classes zip files for those libraries
#
LIB_JAVAC_CLASSPATH= \
    $(subst $(ONESPACE),\$$(JVM_PATHSEP), $(strip \
            $(addsuffix /export_classes.zip, \
                $(addprefix $(JWC_BUILD_SPACE)/,$(LIB_DEPENDENCIES)))))

#
# Temporary file used to keep the signature of the interface classes
# to test compatibility between exported interface and selected 
# implementation for this library
#
LIB_SIGNATURE_FILE=$(LIB_BLD_PATH)/export_classes.sgn

#
# IMPL_NOTE: revome when decoupling is complete
# For now we append all the classes from the old build to the classpath
#
ifeq ($(LIB_USES_LEGACY_CLASSES),true)

LIB_JAVAC_FULL_CLASSPATH = $(LIB_JAVAC_CLASSPATH)\$$(JVM_PATHSEP)$(MIDP_CLASSES_ZIP)

else 

LIB_JAVAC_FULL_CLASSPATH = $(LIB_JAVAC_CLASSPATH)

endif

######################################################################
#
# Native compilation definitions
#
######################################################################

LIB_OBJ_DIR=$(LIB_BLD_PATH)/obj$(DEBUG_SUFFIX)

#
# Full path of the native library target for this library
#
LIB_LIBRARY=$(LIB_BLD_PATH)/\$$(JVM_LIB_PREFIX)$(LIB_NAME)\$$(JVM_BUILD_EXT)\$$(JVM_LIB_SUFFIX)

LIB_OBJ_FILES = \
    $(addprefix $(LIB_OBJ_DIR)/, \
        $(patsubst %.cpp,%\$$(JVM_OBJ_SUFFIX), \
            $(filter %.cpp,$(LIB_NATIVE_FILES))))

LIB_OBJ_FILES += \
    $(addprefix $(LIB_OBJ_DIR)/, \
        $(patsubst %.c,%\$$(JVM_OBJ_SUFFIX), \
            $(filter %.c,$(LIB_NATIVE_FILES))))

#
# Convert list of space-separated list of libraries to include list
#
LIB_INCLUDE_PATH += \
    $(addsuffix /include, \
        $(addprefix -I$(JWC_BUILD_SPACE)/,$(LIB_DEPENDENCIES)))

#
# Append the native exported interface of this library to the include list
#
LIB_INCLUDE_PATH += -I$(LIB_SRC_PATH)/include

#
# IMPL_NOTE: revome when decoupling is complete
# For now we append legacy include path
#
ifeq ($(LIB_USES_LEGACY_HEADERS),true)

include $(JWC_WORK_SPACE)/build/common/makefiles/SubsystemDefs.gmk
LIB_INCLUDE_PATH += $(EXTRA_INCLUDES)

endif

######################################################################
#
# Native header installation definitions
#
######################################################################

LIB_HEADER_DIR=$(LIB_BLD_PATH)/include

LIB_HEADER_PATHS = $(addprefix $(LIB_BLD_PATH)/include/,$(LIB_HEADER_FILES))

#
# Rule to generate library.make for the specified library and implementation
#
# IMPL_NOTE: insert fixcygpath where necessary
#
all:
	$(A)echo ... Generating makefile for $(LIBRARY) library, $(LIB_IMPL) implementation
	$(A)mkdir -p $(LIB_BLD_PATH)
	$(A)echo "" > $(LIB_MAKEFILE)
	$(A)echo "ifndef _$(LIBRARY)_makefile_" >> $(LIB_MAKEFILE)
	$(A)echo "_$(LIBRARY)_makefile_ = $(LIBRARY)" >> $(LIB_MAKEFILE)
	$(A)echo "" >> $(LIB_MAKEFILE)
	$(A)echo "include $(JWC_WORK_SPACE)/build/share/libdefs.gmk" >> $(LIB_MAKEFILE)
	$(A)echo "" >> $(LIB_MAKEFILE)
	$(A)echo ".PHONY: $(LIBRARY)_all $(LIBRARY)_clean" >> $(LIB_MAKEFILE)
	$(A)echo ".PHONY: $(DEPENDENCIES_ALL) $(DEPENDENCIES_CLEAN)" >> $(LIB_MAKEFILE)
	$(A)echo ".PHONY: $(LIBRARY)_self $(LIBRARY)_classes" >> $(LIB_MAKEFILE)
	$(A)echo ".PHONY: $(LIBRARY)_lib $(LIBRARY)_headers" >> $(LIB_MAKEFILE)
	$(A)echo "" >> $(LIB_MAKEFILE)
	$(A)echo "$(LIBRARY)_all: $(DEPENDENCY_MAKEFILES) $(NL)" >> $(LIB_MAKEFILE) 
	$(A)echo "                $(DEPENDENCIES_ALL) $(LIBRARY)_self" >> $(LIB_MAKEFILE)
	$(A)echo "" >> $(LIB_MAKEFILE)
	$(A)echo "$(LIBRARY)_clean: $(DEPENDENCY_MAKEFILES) $(NL)" >> $(LIB_MAKEFILE) 
	$(A)echo "     $(LIBRARY)_clean_self $(DEPENDENCIES_CLEAN)" >> $(LIB_MAKEFILE)
	$(A)echo "" >> $(LIB_MAKEFILE)
	$(A)for i in $(LIB_DEPENDENCIES); do \
	     echo "-include $(JWC_BUILD_SPACE)/$$i/library.make" >> $(LIB_MAKEFILE); \
	done
	$(A)echo "" >> $(LIB_MAKEFILE)
	$(A)echo "$(LIBRARY)_self: $(LIBRARY)_classes $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "      $(LIBRARY)_lib $(LIBRARY)_headers \$$(SIGTEST_JAR)" >> $(LIB_MAKEFILE)
	$(A)echo "" >> $(LIB_MAKEFILE)
	$(A)echo "$(LIBRARY)_clean_self:" >> $(LIB_MAKEFILE)
	$(A)echo "	\$$(A)echo ... Cleaning $(LIBRARY)" >> $(LIB_MAKEFILE)
	$(A)echo "	\$$(A)rm -rf $(LIB_CLASS_DIR)" >> $(LIB_MAKEFILE)
	$(A)echo "	\$$(A)rm -f $(LIB_CLASS_LIST)" >> $(LIB_MAKEFILE)
	$(A)echo "	\$$(A)rm -f $(LIB_CLASSES_ZIP)" >> $(LIB_MAKEFILE)
	$(A)echo "	\$$(A)rm -rf $(LIB_EXPORT_CLASS_DIR)" >> $(LIB_MAKEFILE)
	$(A)echo "	\$$(A)rm -f $(LIB_EXPORT_CLASS_LIST)" >> $(LIB_MAKEFILE)
	$(A)echo "	\$$(A)rm -f $(LIB_EXPORT_CLASSES_ZIP)" >> $(LIB_MAKEFILE)
	$(A)echo "	\$$(A)rm -rf $(LIB_I3TEST_CLASS_DIR)" >> $(LIB_MAKEFILE)
	$(A)echo "	\$$(A)rm -f $(LIB_I3TEST_CLASS_LIST)" >> $(LIB_MAKEFILE)
	$(A)echo "	\$$(A)rm -f $(LIB_I3TEST_CLASSES_ZIP)" >> $(LIB_MAKEFILE)
	$(A)echo "	\$$(A)rm -rf $(LIB_OBJ_DIR)" >> $(LIB_MAKEFILE)
	$(A)echo "	\$$(A)rm -f $(LIB_LIBRARY)" >> $(LIB_MAKEFILE)
	$(A)echo "	\$$(A)rm -rf $(LIB_HEADER_DIR)" >> $(LIB_MAKEFILE)
	$(A)echo "" >> $(LIB_MAKEFILE)
	$(A)echo "" >> $(LIB_MAKEFILE)
	$(A)echo "$(LIBRARY)_classes: $(LIB_EXPORT_CLASSES_ZIP) $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "               $(LIB_CLASSES_ZIP)" >> $(LIB_MAKEFILE)
	$(A)echo "" >> $(LIB_MAKEFILE)
	$(A)echo "" >> $(LIB_MAKEFILE)
	$(A)echo "$(LIB_CLASSES_ZIP): $(LIB_CLASS_DIR) $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "      $(LIB_CLASS_LIST) $(LIB_EXPORT_CLASSES_ZIP) " >> $(LIB_MAKEFILE)
	$(A)echo "	\$$(A)if grep -q java $(LIB_CLASS_LIST); then $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	    echo ... Compiling $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo             "implementation classes for $(LIBRARY);$(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	    sort $(LIB_CLASS_LIST) | $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	        uniq > $(LIB_TEMP_CLASS_LIST); $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	    mv $(LIB_TEMP_CLASS_LIST) $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	        $(LIB_CLASS_LIST); $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	    \$$(JAVAC_CMD) @$(LIB_CLASS_LIST) $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	        -g\$$(JAVAC_DEBUG) $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	        -classpath $(LIB_CLASS_DIR)\$$(JVM_PATHSEP)$(LIB_EXPORT_CLASSES_ZIP)\$$(JVM_PATHSEP)$(LIB_JAVAC_FULL_CLASSPATH) $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	        -bootclasspath $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	          $(CLDC_DIST_DIR)/lib/cldc_classes.zip $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	        -d $(LIB_CLASS_DIR);  $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	fi" >> $(LIB_MAKEFILE)
	$(A)echo "	\$$(A)echo > $(LIB_CLASS_LIST)" >> $(LIB_MAKEFILE)
	$(A)echo "	\$$(A)\$$(JAR_CMD) -cf $(LIB_CLASSES_ZIP) -C $(LIB_CLASS_DIR) ." >> $(LIB_MAKEFILE)
	$(A)echo "	\$$(A)echo ... Compatibility test for $(LIBRARY)" >> $(LIB_MAKEFILE)
	$(A)echo "	\$$(A)cd $(shell dirname $(LIB_SIGNATURE_FILE)); $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	\$$(JAVA_CMD) -cp $(LIB_EXPORT_CLASSES_ZIP)\$$(JVM_PATHSEP)$(LIB_JAVAC_FULL_CLASSPATH)\$$(JVM_PATHSEP)\$$(SIGTEST_JAR) $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	   com.sun.tdk.signaturetest.Setup -allpublic $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	   -classpath $(LIB_EXPORT_CLASSES_ZIP) $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	   -filename $(notdir $(LIB_SIGNATURE_FILE)); $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	if [ "$$\$$?" != "95" ]; then $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	    rm -f $(LIB_CLASSES_ZIP); $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	    exit 1; $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	fi" >> $(LIB_MAKEFILE)
	$(A)echo "	\$$(A)cd $(shell dirname $(LIB_SIGNATURE_FILE)); $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	\$$(JAVA_CMD) -cp $(LIB_CLASSES_ZIP)\$$(JVM_PATHSEP)$(LIB_EXPORT_CLASSES_ZIP)\$$(JVM_PATHSEP)$(LIB_JAVAC_FULL_CLASSPATH)\$$(JVM_PATHSEP)\$$(SIGTEST_JAR) $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	    com.sun.tdk.signaturetest.SignatureTest -allpublic $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	    -classpath $(LIB_CLASSES_ZIP)\$$(JVM_PATHSEP)$(LIB_EXPORT_CLASSES_ZIP) $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	    -enablesuperset $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	    -filename $(notdir $(LIB_SIGNATURE_FILE)); $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	if [ "$$\$$?" != "95" ]; then $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	    rm -f $(LIB_CLASSES_ZIP); $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	    rm -f $(LIB_SIGNATURE_FILE); $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	    exit 1; $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	fi" >> $(LIB_MAKEFILE)
	$(A)echo "	$(A)rm -f $(LIB_SIGNATURE_FILE)" >> $(LIB_MAKEFILE)
	$(A)echo "" >> $(LIB_MAKEFILE)
	$(A)echo "" >> $(LIB_MAKEFILE)
	$(A)echo "$(LIB_CLASS_LIST): $(LIB_JAVA_PATHS)" >> $(LIB_MAKEFILE)
	$(A)echo "	\$$(A)echo \$$? >> \$$@" >> $(LIB_MAKEFILE)
	$(A)echo "" >> $(LIB_MAKEFILE)
	$(A)echo "$(LIB_EXPORT_CLASSES_ZIP): $(LIB_EXPORT_CLASS_DIR) $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "                           $(LIB_EXPORT_CLASS_LIST)" >> $(LIB_MAKEFILE)
	$(A)echo "	\$$(A)if grep -q java $(LIB_EXPORT_CLASS_LIST); then $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	    echo ... Compiling $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo                  "interface classes for $(LIBRARY);$(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	    sort $(LIB_EXPORT_CLASS_LIST) | $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	        uniq > $(LIB_TEMP_EXPORT_CLASS_LIST); $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	    mv $(LIB_TEMP_EXPORT_CLASS_LIST) $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	        $(LIB_EXPORT_CLASS_LIST); $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	    \$$(JAVAC_CMD) @$(LIB_EXPORT_CLASS_LIST) $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	        -g\$$(JAVAC_DEBUG) $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	        -classpath $(LIB_EXPORT_CLASS_DIR)\$$(JVM_PATHSEP)$(LIB_JAVAC_FULL_CLASSPATH) $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	        -bootclasspath $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	          $(CLDC_DIST_DIR)/lib/cldc_classes.zip $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	        -d $(LIB_EXPORT_CLASS_DIR); $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	fi" >> $(LIB_MAKEFILE)
	$(A)echo "	\$$(A)echo > $(LIB_EXPORT_CLASS_LIST)" >> $(LIB_MAKEFILE)
	$(A)echo "	\$$(A)\$$(JAR_CMD) -cf $(LIB_EXPORT_CLASSES_ZIP) $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	    -C $(LIB_EXPORT_CLASS_DIR) ." >> $(LIB_MAKEFILE)
	$(A)echo "" >> $(LIB_MAKEFILE)
	$(A)echo "$(LIB_EXPORT_CLASS_LIST): $(LIB_EXPORT_JAVA_PATHS)" >> $(LIB_MAKEFILE)
	$(A)echo "	\$$(A)echo \$$? >> \$$@" >> $(LIB_MAKEFILE)
	$(A)echo "" >> $(LIB_MAKEFILE)
	$(A)echo "$(LIB_I3TEST_CLASSES_ZIP): $(LIB_I3TEST_CLASS_DIR) $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "                           $(LIB_I3TEST_CLASS_LIST)" >> $(LIB_MAKEFILE)
	$(A)echo "	\$$(A)if grep -q java $(LIB_I3TEST_CLASS_LIST); then $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	    echo ... Compiling $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo                  "I3 test classes for $(LIBRARY);$(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	    sort $(LIB_I3TEST_CLASS_LIST) | $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	        uniq > $(LIB_TEMP_I3TEST_CLASS_LIST); $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	    mv $(LIB_TEMP_I3TEST_CLASS_LIST) $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	        $(LIB_I3TEST_CLASS_LIST); $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	    \$$(JAVAC_CMD) @$(LIB_I3TEST_CLASS_LIST) $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	        -g\$$(JAVAC_DEBUG) $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	        -classpath $(LIB_I3TEST_CLASS_DIR)\$$(JVM_PATHSEP)$(LIB_JAVAC_FULL_CLASSPATH) $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	        -bootclasspath $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	          $(CLDC_DIST_DIR)/lib/cldc_classes.zip $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	        -d $(LIB_I3TEST_CLASS_DIR); $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	fi" >> $(LIB_MAKEFILE)
	$(A)echo "	\$$(A)echo > $(LIB_I3TEST_CLASS_LIST)" >> $(LIB_MAKEFILE)
	$(A)echo "	\$$(A)\$$(JAR_CMD) -cf $(LIB_I3TEST_CLASSES_ZIP) $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	    -C $(LIB_I3TEST_CLASS_DIR) ." >> $(LIB_MAKEFILE)
	$(A)echo "" >> $(LIB_MAKEFILE)
	$(A)echo "$(LIB_I3TEST_CLASS_LIST): $(LIB_I3TEST_JAVA_PATHS)" >> $(LIB_MAKEFILE)
	$(A)echo "	\$$(A)echo \$$? >> \$$@" >> $(LIB_MAKEFILE)
	$(A)echo "" >> $(LIB_MAKEFILE)
	$(A)echo "$(LIB_CLASS_DIR) $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "$(LIB_EXPORT_CLASS_DIR) $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "$(LIB_I3TEST_CLASS_DIR):" >> $(LIB_MAKEFILE)
	$(A)echo "	\$$(A)mkdir -p \$$@" >> $(LIB_MAKEFILE)
	$(A)echo "" >> $(LIB_MAKEFILE)
	$(A)echo "$(LIBRARY)_lib: $(LIB_OBJ_DIR) $(LIB_LIBRARY)" >> $(LIB_MAKEFILE)
	$(A)echo "" >> $(LIB_MAKEFILE)
	$(A)echo "$(LIB_LIBRARY): $(LIB_OBJ_FILES)" >> $(LIB_MAKEFILE)
	$(A)echo "	\$$(A)echo ... Linking \$$@" >> $(LIB_MAKEFILE)
	$(A)echo "	\$$(A)\$$(JVM_LIBMGR) \$$(JVM_LIB_FLAGS) $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "	      \$$(JVM_LINK_OUT_SWITCH2)\$$@ $(LIB_OBJ_FILES)" >> $(LIB_MAKEFILE)
	$(A)echo "" >> $(LIB_MAKEFILE)
	$(A)echo "$(LIB_OBJ_DIR)/%\$$(JVM_OBJ_SUFFIX): $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "             $(LIB_SRC_PATH)/$(LIB_IMPL)/native/%.c" >> $(LIB_MAKEFILE)
	$(A)echo "	\$$(A)echo ... Compiling \$$<" >> $(LIB_MAKEFILE)
	$(A)echo "	\$$(A)\$$(JVM_CC) \$$(CFLAGS) $(LIB_INCLUDE_PATH) $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "      \$$(JVM_CC_OUT_SWITCH)\$$@ -c \$$<" >> $(LIB_MAKEFILE)
	$(A)echo "" >> $(LIB_MAKEFILE)
	$(A)echo "$(LIB_OBJ_DIR)/%\$$(JVM_OBJ_SUFFIX): $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "             $(LIB_SRC_PATH)/$(LIB_IMPL)/native/%.cpp" >> $(LIB_MAKEFILE)
	$(A)echo "	\$$(A)echo ... Compiling \$$<" >> $(LIB_MAKEFILE)
	$(A)echo "	\$$(A)\$$(JVM_CPP) \$$(CPPFLAGS) $(LIB_INCLUDE_PATH) $(NL)" >> $(LIB_MAKEFILE)
	$(A)echo "      \$$(JVM_CC_OUT_SWITCH)\$$@  -c \$$<" >> $(LIB_MAKEFILE)
	$(A)echo "" >> $(LIB_MAKEFILE)
	$(A)echo "$(LIBRARY)_headers: $(LIB_HEADER_DIR) $(LIB_HEADER_PATHS)" >> $(LIB_MAKEFILE)
	$(A)echo "" >> $(LIB_MAKEFILE)
	$(A)echo "$(LIB_HEADER_DIR)/%.h: $(LIB_SRC_PATH)/include/%.h" >> $(LIB_MAKEFILE)
	$(A)echo "	\$$(A)echo ... Installing \$$@" >> $(LIB_MAKEFILE)
	$(A)echo "	\$$(A)cp -vf \$$? \$$@" >> $(LIB_MAKEFILE)
	$(A)echo "" >> $(LIB_MAKEFILE)
	$(A)echo "$(LIB_OBJ_DIR) $(LIB_HEADER_DIR):" >> $(LIB_MAKEFILE)
	$(A)echo "	\$$(A)mkdir -p \$$@" >> $(LIB_MAKEFILE)
	$(A)echo "" >> $(LIB_MAKEFILE)
	$(A)echo "include $(JWC_WORK_SPACE)/build/share/gendeps.make" >> $(LIB_MAKEFILE)
	$(A)echo "" >> $(LIB_MAKEFILE)
	$(A)echo "endif" >> $(LIB_MAKEFILE)
	$(A)echo "" >> $(LIB_MAKEFILE)
ifdef VERBOSE_BUILD
	$(A)echo '=========================================================='
	$(A)cat $(LIB_MAKEFILE)
	$(A)echo '=========================================================='
endif

clean:
	$(A)echo ... Cleaning makefile for library $(LIBRARY)
	$(A)rm -f $(LIB_MAKEFILE)
