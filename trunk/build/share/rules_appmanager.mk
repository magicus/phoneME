#
# Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
# DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 only,
# as published by the Free Software Foundation.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
# version 2 for more details (a copy is included at /legal/license.txt).
# 
# You should have received a copy of the GNU General Public License version
# 2 along with this work; if not, write to the Free Software Foundation,
# Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
# 
# Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
# CA 95054 or visit www.sun.com if you need additional information or have
# any questions.
#

all:: appmanager_create

appmanager_create:  appmanager_build appmanager_resources 

appmanager_build: remove-repository ${APPMANAGER_JARFILE} ${APPMANAGER_CLIENT_JARFILE}

appmanager_resources::
	$(AT)echo "Copying appmanager resources..."
	$(AT)cp -f $(WS_APPMANAGER_SRCDIR)/com/sun/appmanager/resources/AppManagerResources.properties $(APPMANAGER_CLASSESDIR)/com/sun/appmanager/resources
	$(AT)cp -f $(WS_APPMANAGER_PROFILES_DIR)/*.txt $(APPMANAGER_PROFILES_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_PREFSDIR_SYSTEM)/*.props $(APPMANAGER_PREFSDIR_SYSTEM)/

appmanager_install: all
	$(AT)echo "Packaging up the AppManager build... Please wait."
	$(AT)rm -f $(CVM_BUILD_TOP)/$(APPMANAGER_TARFILE).gz
	$(AT)(cd $(APPMANAGER_APPREPOSITORY_DIR); chmod -Rf a+wr *)
	$(AT)(cd $(CVM_BUILD_TOP); tar cf $(APPMANAGER_TARFILE) bin lib repository $(PRESENTATION_MODE_JARFILES))
	$(AT)gzip $(CVM_BUILD_TOP)/$(APPMANAGER_TARFILE)
	$(AT)echo "$(APPMANAGER_TARFILE).gz has been created for transferring appmanager to a device."

# Unconditionally remove the repository directory for each build to prevent PBP and PP
# apps from existing in the repository directory together.
remove-repository:
	rm -rf $(APPMANAGER_APPREPOSITORY_DIR)

CVM_FLAGS                         += PRESENTATION_MODES
PRESENTATION_MODES_CLEANUP_ACTION = rm -f $(PRESENTATION_MODE_JARFILES) \
				    rm -rf $(APPMANAGER_APPREPOSITORY_DIR) \
				    rm -rf $(APPMANAGER_CLASSESDIR)

${APPMANAGER_JARFILE}: $(APPMANAGER_BUILDDIRS) appmanager_classes appmanager_resources
	$(AT)cd $(APPMANAGER_CLASSESDIR); $(CVM_JAR) cf ../lib/$(APPMANAGER_JARFILE) com/sun/appmanager/resources com/sun/appmanager/client/* com/sun/appmanager/impl/client/* com/sun/appmanager/preferences com/sun/appmanager/impl/preferences com/sun/appmanager/impl/ota com/sun/appmanager/impl/client com/sun/appmanager/apprepository com/sun/appmanager/impl/apprepository com/sun/appmanager/impl/store com/sun/appmanager/*.class com/sun/appmanager/impl/*.class com/sun/appmanager/presentation com/sun/appmanager/store com/sun/appmanager/ota com/sun/appmanager/mtask com/sun/appmanager/appmodel com/sun/appmanager/client com/sun/xlet/mvmixc/*.class com/sun/tck/xlet/*.class

${APPMANAGER_CLIENT_JARFILE}: $(APPMANAGER_BUILDDIRS) appmanager_classes appmanager_resources
	$(AT)cd $(APPMANAGER_CLASSESDIR); $(CVM_JAR) cf ../lib/$(APPMANAGER_CLIENT_JARFILE) com/sun/appmanager/client/* com/sun/appmanager/impl/client/* com/sun/tck/xlet/*.class com/sun/xlet/mvmixc/*Interface.class com/sun/xlet/mvmixc/CDCAmsXletContext.class

APPMANAGER_CLASS_TMP = $(subst .,/,$(APPMANAGER_CLASSES))
APPMANAGER_CLASS_FILES = \
        $(patsubst %,$(APPMANAGER_CLASSESDIR)/%.class,$(APPMANAGER_CLASS_TMP))

APPMANAGER_CLASSES_SRCPATH = $(subst $(space),:,$(strip $(WS_APPMANAGER_SRCDIR)))

$(APPMANAGER_CLASSESDIR)/%.class: %.java
	$(AT)echo $? >>.appmanagerclasses.list

#
# Rule to install appmanager security policy
#
$(APPMANAGER_SECURITY_POLICY_BUILD): $(APPMANAGER_SECURITY_POLICY_SRC)
	@echo "Updating appmanager security policy...";
	@cp -f $< $@;
	@echo "<<<Finished copying $@";

#
# Rule to install permissive app security policy
#
$(APPMANAGER_SECURITY_PERMISSIVE_BUILD): $(APPMANAGER_SECURITY_PERMISSIVE_SRC)
	@echo "Updating appmanager security permissive...";
	@cp -f $< $@;
	@echo "<<<Finished copying $@";

#
# Rule to install constrained app security policy
#
$(APPMANAGER_SECURITY_CONSTRAINED_BUILD): $(APPMANAGER_SECURITY_CONSTRAINED_SRC)
	@echo "Updating appmanager security constrained...";
	@cp -f $< $@;
	@echo "<<<Finished copying $@";

#
## create directories
#
$(APPMANAGER_BUILDDIRS):
	@echo ... mkdir -p $@
	@if [ ! -d $@ ]; then mkdir -p $@; fi

appmanager_classes: .delete.appmanagerclasses.list .report.appmanagerclasses $(SAX_JARFILE_BUILD) $(APPMANAGER_SECURITY_POLICY_BUILD) $(APPMANAGER_SECURITY_PERMISSIVE_BUILD) $(APPMANAGER_SECURITY_CONSTRAINED_BUILD) $(APPMANAGER_CLASS_FILES) .compile.appmanagerclasses

.report.appmanagerclasses:
	@echo "Checking for appmanager classes to compile..."

.compile.appmanagerclasses:
	$(AT)if [ -s .appmanagerclasses.list ] ; then                   \
                echo "Compiling appmanager classes...";                 \
                $(JAVAC_CMD)                                            \
                        -d $(APPMANAGER_CLASSESDIR)                     \
                        -bootclasspath $(CVM_BUILDTIME_CLASSESDIR)$(PS)$(LIB_CLASSESDIR)$(PS)$(SAX_JARFILE_BUILD) \
                        -sourcepath $(APPMANAGER_CLASSES_SRCPATH)       \
                        @.appmanagerclasses.list ;                      \
        fi

.delete.appmanagerclasses.list:
	$(AT)$(RM) .appmanagerclasses.list


javadoc-appmanager: $(APPMANAGER_JAVADOCDIR) $(WS_APPMANAGER_JAVADOC_DIR)/appmanager-overview-description.html appmanager_classes
	$(CVM_JAVADOC) -source 1.4 -use -overview $(WS_APPMANAGER_JAVADOC_DIR)/appmanager-overview-description.html -d $(APPMANAGER_JAVADOCDIR) -splitIndex -windowtitle 'AppManager Interfaces' -doctitle 'AppManager Interfaces' -header 'AppManager Interfaces' -bottom '<font size="-1">Copyright 1994-2002 Sun Microsystems, Inc. All Rights Reserved.</font> ' -classpath $(APPMANAGER_CLASSESDIR) -sourcepath $(APPMANAGER_CLASSES_SRCPATH) $(APPMANAGER_JAVADOC_CLASSES)

clean::
	rm -rf .*classes.list
	rm -rf $(APPMANAGER_JARFILE)
	rm -rf $(APPMANAGER_CLIENT_JARFILE)
	rm -rf $(APPMANAGER_BUILDDIRS)
	rm -rf $(APPMANAGER_TARFILE).gz
	rm -f $(APPMANAGER_SECURITY_POLICY_NAME)
	rm -f $(APPMANAGER_SECURITY_PERMISSIVE_NAME)
	rm -f $(APPMANAGER_SECURITY_CONSTRAINED_NAME)

# Include presentation mode rules files
ifneq ($(PRESENTATION_MODES_RULES_FILES),)
include $(patsubst %,../share/%,$(PRESENTATION_MODES_RULES_FILES))
endif

# Include OTA rules file if appropriate
ifeq ($(APPMANAGER_BUILD_OTA),true)
-include ../share/rules_appmanager_ota.mk
endif
