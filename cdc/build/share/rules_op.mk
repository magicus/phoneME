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

SUBSYSTEM_RULES_FILE     = subsystem_rules.gmk

ifneq ($(CVM_STATICLINK_LIBS), true)
JSR_NATIVE_LIBS = "$(5)"
else
JSR_NATIVE_LIBS = ""
endif

# generateJSRInitializer(xmlFiles,generatedDir,initializerPackage,outputFile,nativeLibs)
define generateJSRInitializer
	$(AT)$(call runJarFile, $(CONFIGURATOR_JAR_FILE),      \
	-xml $(CVM_MISC_TOOLS_SRCDIR)/xml/empty.xml            \
	-xsl $(CONFIGURATOR_DIR)/xsl/share/merge.xsl           \
	-params filesList '$(1)'                               \
	-out $(2)/properties_merged.xml                        \
	-xml $(2)/properties_merged.xml                        \
	-xsl $(CONFIGURATOR_DIR)/xsl/cdc/propertiesJava.xsl    \
	-params packageName $(3) nativeLibs $(JSR_NATIVE_LIBS) \
	-out $(4))
endef

# Generate constant classes
# generateConstantClasses(generatedDirectory, constantsXmlFile)
define generateConstantClasses
    $(foreach class, $(shell \
        $(call runJarFile, $(CONFIGURATOR_JAR_FILE), \
            -xml $(2) \
            -xsl $(CONFIGURATOR_DIR)/xsl/cdc/constantClasses.xsl \
            -out $(call constantClassesListFile, $(1));) \
        cat $(call constantClassesListFile, $(1))), \
	    $(call runJarFile, $(CONFIGURATOR_JAR_FILE), \
            -xml $(2) \
            -xsl $(CONFIGURATOR_DIR)/xsl/cdc/constantsJava.xsl \
            -params fullClassName $(class) \
            -out $(1)/classes/$(subst .,/,$(class)).java;))
endef

# Gets a list of Java files which have been generated from an XML constant file
# getConstantClassFileList(generatedDirectory)
define getConstantClassFileList
    $(foreach class, $(shell \
        cat $(call constantClassesListFile, $(1))), \
        $(1)/classes/$(subst .,/,$(class)).java)
endef

# Returns the name of a file which contains a list of classes generated from an XML constant file
# constantClassesListFile(generatedDirectory)
define constantClassesListFile
    $(1)/.constant.class.list
endef

# compileJSROP(jsrXXX,distDir,FILES,EXTRA_CLASSPATH)
define compileJSROP
	@echo "Compiling "$(1)" classes...";			\
	mkdir -p $(2);			\
	$(JAVAC_CMD)						\
		-d $(2) \
		-bootclasspath $(CVM_BUILDTIME_CLASSESDIR) 	\
		-classpath $(JAVACLASSES_CLASSPATH)$(PS)$(JSROP_JUMP_API)$(PS)$(ABSTRACTIONS_JAR)$(4) \
		$(3)
endef

# makeJSROPJar(jarFileName,jsrDir)
define makeJSROPJar
	@echo ...$(1);     \
	$(CVM_JAR) cf $(1) -C $(2) .;
endef

# Creates an additional jar file containing classes implemented outside JSR's
# component directory but included in JSR API specification
# makeExtraJar(extraJarFileName,jsrApiClassesList,jsrClassesDir,libJars,jsrExtraClassesDir)
define makeExtraJar
	$(AT)for class in $(2); do if !(test -r $(3)/$$class); then \
	    for jar in $(4); do if $(UNZIP) -l $$jar $$class | grep "$$class" >/dev/null; then \
	        $(UNZIP) -qo $$jar $$class -d $(5); break; \
	    fi; done; \
	    if !(test -r $(5)/$$class); then \
                echo "Error: could not find $$class needed for $(1)"; exit 1; \
            fi; \
	fi; done
        $(AT)if (test -r $(5)); then \
	    echo ...$(1); \
	    $(CVM_JAR) cf $(1) -C $(5) .;\
        fi
endef

# makeJSRExtraJar(jsrNumber)
# The following variables MUST BE defined
# JSR_#_BUILD_DIR            - path to JSR's build directory
# JSR_#_EXTRA_JAR            - JSR's extras jar file path
# JSR_#_API_CLASSES          - JSR's API classes list
define makeJSRExtraJar
	$(call makeExtraJar,$(JSR_$(1)_EXTRA_JAR),$(JSR_$(1)_API_CLASSES),\
	    $(JSR_$(1)_BUILD_DIR)/classes,\
	    $(JSROP_EXTRA_SEARCHPATH), \
	    $(JSR_$(1)_BUILD_DIR)/extraclasses)
endef

# compileJSRClasses(jsrNumber)
# The following variables MUST BE defined
# JSR_#_BUILD_DIR            - path to JSR's build directory
# SUBSYSTEM_JSR_#_JAVA_FILES - list of JSR's java sources paths
# JSR_#_JAR                  - JSR's jar file path
define compileJSRClasses
	$(call compileJSROP,jsr$(1),$(JSR_$(1)_BUILD_DIR)/classes,$(SUBSYSTEM_JSR_$(1)_JAVA_FILES),$(2))
	$(call makeJSROPJar,$(JSR_$(1)_JAR),$(JSR_$(1)_BUILD_DIR)/classes)
endef

#Command for building shared libraries
define makeSharedLibrary
	$(TARGET_LD) $(SO_LINKFLAGS) -o $@ $(1) $(JSROP_LINKLIBS)
endef

# Command for reading API classes list from file
# readClassList(fileName)
define readClassList
	$(foreach class,$(subst .,/,$(shell cat $(1) | grep -v "\#")),$(class).class)
endef

ifeq ($(CVM_DUAL_STACK), true)
#
# Run JavaAPILister to generate the list of classes that are 
# hidden from CDC.
#
$(JSR_CDCRESTRICTED_CLASSLIST): $(JSROP_JARS)
	@echo "Generating JSR restricted class list ..."
	$(AT)$(CVM_JAVA) -cp  $(CVM_BUILD_TOP)/classes.jcc JavaAPILister \
	    -listapi:include=java/*,include=javax/*,input=$(JSROP_HIDE_JARS),cout=$(JSR_CDCRESTRICTED_CLASSLIST)

#
# Generate a list of all JSR classes. These classes will be
# add to the $(CVM_MIDPCLASSLIST) to allow accessing from
# midlets. The JSROP classes don't need to be added to 
# $(CVM_MIDPFILTERCONFIG) and ROMized member filter because 
# there is no restrictions for midlets to accessing the JSROP 
# class' public members.
#
API_EXTENSIONS_CLASSLIST = $(subst $(space),$(comma),$(API_EXTENSIONS_LIST))

define genMIDPPermittedJSRClasses
    $(CVM_JAVA) -cp  $(CVM_BUILD_TOP)/classes.jcc JavaAPILister \
        -listapi:include=java/*,include=javax/*,include=javacard/*,include=org/xml/sax/*,include=org/w3c/dom/*,$(API_EXTENSIONS_CLASSLIST),input=$(1),cout=$(2)
    $(foreach elem,$(JSR_EXTRA_PERMITTED_CLASSES),$(AT)`echo $(elem) >> $(2)`)
endef

ifeq ($(CVM_PRELOAD_LIB), true)
#
# FOR CVM_PRELOAD_LIB=true build, generate JSR_MIDPPERMITTED_CLASSLIST.
# The list will be appended to the main CVM_MIDPCLASSLIST.
#
$(JSR_MIDPPERMITTED_CLASSLIST): $(JSROP_JARS) $(JSROP_EXTRA_JARS)
	@echo "Generating MIDP permitted JSR class list ...";
	$(call genMIDPPermittedJSRClasses,$(JSROP_JARS_LIST),$(JSR_MIDPPERMITTED_CLASSLIST))
else
#
# For non-ROMized build:
# For each jar file in the JSROP_JARS_LIST, generate the
# CVM_MIDPCLASSLIST and add that to the jar file. So each JSR jar
# file contains its own CVM_MIDPCLASSLIST. The API hiding 
# layer lookes into individual jar file in the bootclasspath for 
# its CVM_MIDPCLASSLIST besides the main CVM_MIDPCLASSLIST in
# the lib/ directory. This is mainly for supporting downloaded
# JSRs.
#
define addMIDPPermittedJSRClassList
    $(call genMIDPPermittedJSRClasses,$(1),$(CVM_MIDPCLASSLISTNAME))
    $(AT)$(CVM_JAR) -uf $(1) $(CVM_MIDPCLASSLISTNAME)
    $(AT)rm -rf $(CVM_MIDPCLASSLISTNAME);
endef

$(JSR_MIDPPERMITTED_CLASSLIST): $(JSROP_JARS) $(JSROP_EXTRA_JARS)
	@echo "Updating JSR jar files with MIDP permitted JSR class list...";
	$(AT)rm -rf $@
	$(foreach jarfile, $^, \
            $(call addMIDPPermittedJSRClassList,$(jarfile)))
	$(AT)touch $@
endif
endif

clean::
	$(AT)rm -rf $(JSROP_OUTPUT_DIRS)

# Include JSR 75
ifeq ($(USE_JSR_75), true)
export JSR_75_DIR ?= $(COMPONENTS_DIR)/jsr75
JSR_75_RULES_FILE = $(JSR_75_DIR)/build/cdc_share/$(SUBSYSTEM_RULES_FILE)
ifeq ($(wildcard $(JSR_75_RULES_FILE)),)
$(error JSR_75_DIR must point to a directory containing JSR 75 sources)
endif
include $(JSR_75_RULES_FILE)
endif

# Include JSR 120
ifeq ($(USE_JSR_120), true)
export JSR_120_DIR ?= $(COMPONENTS_DIR)/jsr120
JSR_120_RULES_FILE = $(JSR_120_DIR)/build/cdc_share/$(SUBSYSTEM_RULES_FILE)
ifeq ($(wildcard $(JSR_120_RULES_FILE)),)
$(error JSR_120_DIR must point to a directory containing JSR 120 sources)
endif
include $(JSR_120_RULES_FILE)
endif

# Include JSR 172
ifeq ($(USE_JSR_172), true)
export JSR_172_DIR ?= $(COMPONENTS_DIR)/jsr172
JSR_172_MAKE_FILE = $(JSR_172_DIR)/build/$(SUBSYSTEM_RULES_FILE)
ifeq ($(wildcard $(JSR_172_MAKE_FILE)),)
$(error JSR_172_DIR must point to a directory containing JSR 172 sources)
endif
include $(JSR_172_MAKE_FILE)
endif

# Include JSR 205
ifeq ($(USE_JSR_205), true)
export JSR_205_DIR ?= $(COMPONENTS_DIR)/jsr205
JSR_205_RULES_FILE = $(JSR_205_DIR)/build/$(SUBSYSTEM_RULES_FILE)
ifeq ($(wildcard $(JSR_205_RULES_FILE)),)
$(error JSR_205_DIR must point to a directory containing JSR 205 sources)
endif
include $(JSR_205_RULES_FILE)
endif

# Include JSR 256
ifeq ($(USE_JSR_256), true)
export JSR_256_DIR ?= $(COMPONENTS_DIR)/jsr256
JSR_256_MAKE_FILE = $(JSR_256_DIR)/build/cdc_share/$(SUBSYSTEM_RULES_FILE)
ifeq ($(wildcard $(JSR_256_MAKE_FILE)),)
$(error JSR_256_DIR must point to a directory containing JSR 256 sources)
endif
include $(JSR_256_MAKE_FILE)
endif

# Include JSR 280
ifeq ($(USE_JSR_280), true)
export JSR_280_DIR ?= $(COMPONENTS_DIR)/jsr280
JSR_280_MAKE_FILE = $(JSR_280_DIR)/build/cdc_share/$(SUBSYSTEM_RULES_FILE)
ifeq ($(wildcard $(JSR_280_MAKE_FILE)),)
$(error JSR_280_DIR must point to a directory containing JSR 280 sources)
endif
include $(JSR_280_MAKE_FILE)
endif

# Include API Extensions
ifeq ($(USE_API_EXTENSIONS), true)
API_EXTENSIONS_RULES_FILE = $(API_EXTENSIONS_DIR)/build/cdc_share/$(SUBSYSTEM_RULES_FILE)
ifeq ($(wildcard $(API_EXTENSIONS_RULES_FILE)),)
$(error API_EXTENSIONS_DIR must point to a directory containing API Extensions sources)
endif
include $(API_EXTENSIONS_RULES_FILE)
endif

# Include XML PARSER
ifeq ($(USE_XMLPARSER), true)
export XMLPARSER_DIR ?= $(COMPONENTS_DIR)/xmlparser
XMLPARSER_MAKE_FILE = $(XMLPARSER_DIR)/build/cdc_share/$(SUBSYSTEM_RULES_FILE)
ifeq ($(wildcard $(XMLPARSER_MAKE_FILE)),)
$(error XMLPARSER_DIR must point to a directory containing xmlparser sources)
endif
include $(XMLPARSER_MAKE_FILE)
endif

ifeq ($(CVM_INCLUDE_JAVACALL), true)
ifneq ($(JSROP_JAVACALL_DEPENDENTS),)
$(JSROP_JAVACALL_DEPENDENTS): $(JAVACALL_LIBRARY)
endif
endif

ifeq ($(USE_JDBC), true)
  include $(JDBC_RULES_FILE)
endif

ifeq ($(USE_RMI), true)
  include $(RMI_RULES_FILE)
endif
