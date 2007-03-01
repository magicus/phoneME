#
# Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

.PHONY: javacall_lib

# generatePropertyInitializer(xmlFiles,generatedDir,initializerPackage,outputFile)
define generatePropertyInitializer
	$(CVM_JAVA) -jar $(CONFIGURATOR_JAR_FILE)           \
	-xml $(CVM_MISC_TOOLS_SRCDIR)/xml/empty.xml         \
	-xsl $(CONFIGURATOR_DIR)/xsl/share/merge.xsl        \
	-params filesList '$(1)'                            \
	-out $(2)/properties_merged.xml                     \
	-xml $(2)/properties_merged.xml                     \
	-xsl $(CONFIGURATOR_DIR)/xsl/cdc/propertiesJava.xsl \
	-params packageName $(3)                            \
	-out $(4)
endef


# Macro to pre-process Jpp file into Java file
# runjpp(<input_jpp_file>, <output_java_file>)
define runjpp
    $(CVM_JAVA) -classpath $(TOOLS_OUTPUT_DIR) Jpp $(JPP_DEFS) -o $(2) $(1)
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

# compileJSRClasses(jsrNumber)
# The following variables MUST BE defined
# JSR_#_BUILD_DIR            - path to JSR's build directory
# SUBSYSTEM_JSR_#_JAVA_FILES - list of JSR's java sources paths
# JSR_#_JAR                  - JSR's jar file path
define compileJSRClasses
	$(call compileJSROP,jsr$(1),$(JSR_$(1)_BUILD_DIR)/classes,$(SUBSYSTEM_JSR_$(1)_JAVA_FILES))
	$(call makeJSROPJar,$(JSR_$(1)_JAR),$(JSR_$(1)_BUILD_DIR)/classes)
endef

#Command for building shared libraries
define makeSharedLibrary
	$(TARGET_LD) $(SO_LINKFLAGS) -o $@ $(1) $(JSROP_LINKLIBS) -L$(JSROP_LIB_DIR)
endef

ifeq ($(CVM_INCLUDE_JAVACALL), true)
javacall_lib: $(JAVACALL_LIBRARY)
else
javacall_lib:
endif

