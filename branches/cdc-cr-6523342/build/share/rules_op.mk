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

# compileJSROP(dir,JSROPDIR,FILES)
define compileJSROP
	@echo "Compiling "$(1)" classes...";			\
	mkdir -p $(2)/classes;			\
	$(JAVAC_CMD)						\
		-d $(2)/classes \
		-bootclasspath $(CVM_BUILDTIME_CLASSESDIR) 	\
		-classpath $(JAVACLASSES_CLASSPATH)$(PS)$(JSROP_JUMP_API)$(PS)$(ABSTRACTIONS_JAR)$(MIDP_API_CLASSPATH)$(DEPS_CLASSPATH) \
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
	$(call compileJSROP,jsr$(1),$(JSR_$(1)_BUILD_DIR),$(SUBSYSTEM_JSR_$(1)_JAVA_FILES))
	$(call makeJSROPJar,$(JSR_$(1)_JAR),$(JSR_$(1)_BUILD_DIR)/classes)
endef

