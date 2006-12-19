#
# @(#)rules_op.mk	1.3 06/10/10
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

ifeq ($(JSR_CLASSES),)
.compile.jsrclasses:
else
.compile.jsrclasses: .compile.jsr177 .compile.jsr172
endif

# compileJSR(dir,SOURCEPATH,FILES)
define compileJSR
	echo "Compiling "$(1)" classes...";			\
	mkdir -p $(CVM_JSR_CLASSESDIR)/$(1);			\
	$(JAVAC_CMD)						\
		-d $(CVM_JSR_CLASSESDIR)/$(1)			\
		-bootclasspath $(CVM_BUILDTIME_CLASSESDIR) 	\
		-classpath $(JAVACLASSES_CLASSPATH)$(PS)$(CVM_JSR_CLASSESDIR)		\
		-sourcepath $(2)	\
		$(3)
endef

# parseManifest(dir,jarFileName,manifestFileName,buildID)
define parseManifest
	@echo ...$(2); \
if [ -r $(3) ]; then \
	mkdir -p $(CVM_JSR_CLASSESDIR)/META-INF;\
	sed -e "s/PLACEHOLDER/$(4)/" $(3) > $(CVM_JSR_CLASSESDIR)/META-INF/MANIFEST.MF; \
	cd $(CVM_JSR_CLASSESDIR)/$(1); $(CVM_JAR) cmf ../META-INF/MANIFEST.MF ../../lib/$(2) *; \
else \
	cd $(CVM_JSR_CLASSESDIR)/$(1); $(CVM_JAR) cf ../../lib/$(2) *; \
fi
endef

.compile.jsr177:
ifeq ($(USE_JSR_177), true)
	@echo $(SUBSYSTEM_SATSA_SOURCEPATH):
	$(call compileJSR,jsr177,$(SUBSYSTEM_SATSA_SOURCEPATH),$(SUBSYSTEM_SATSA_JAVA_FILES))
	$(call parseManifest,jsr177,$(JSR177_JAR_FILENAME),$(OP_MANIFEST_FILENAME),$(SATSA_BUILD_ID))
endif

.compile.jsr172:
ifeq ($(USE_JSR_172), true)
	@echo $(SUBSYSTEM_WEBSERVICES_SOURCEPATH):
	$(call compileJSR,jsr172,$(SUBSYSTEM_WEBSERVICES_SOURCEPATH),$(SUBSYSTEM_WEBSERVICES_JAVA_FILES))
	$(call parseManifest,jsr172,$(JSR172_JAR_FILENAME),$(JSR172_MANIFEST_FILENAME),$(WEBSERVICES_BUILD_ID))
endif



$(JSR177_JAR_FILENAME): .compile.jsr177
	
$(JSR172_JAR_FILENAME): .compile.jsr172

