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

# By default build the parfile target
all:: parfile

# Target for the back-end server classes.
web: $(J2ME_CLASSLIB) $(APPMANAGER_BUILDDIRS) $(SAX_JARFILE_BUILD) provisioning_classes adapter_jar web.ear 

# Target for the par file to deploy to a provisioning server.
parfile:: appmanager_create $(APPMANAGER_PAR_MIDIR)/provisioning.xml .construct.parlist

# Build to parfile and earfile, and then deploy the earfile to
# a server (requires that J2EE_HOME and JSR124_HOME be set).
deploy:: parfile web .deploy_earfile

ifeq ($(BUILDING_WEB),true)
PROVISIONING_CLASS_FILES = \
        $(patsubst %,$(APPMANAGER_WI_CLASSESDIR)/%.class,$(PROVISIONING_CLASSES))
$(APPMANAGER_WI_CLASSESDIR)/%.class: %.java
	$(AT)echo $? >> .provisioningclasses.list
endif

ADAPTER_CLASS_TMP = $(subst .,/,$(ADAPTER_CLASSES))
ADAPTER_CLASS_FILES = \
        $(patsubst %,$(APPMANAGER_ADAPTER_CLASSESDIR)/%.class,$(ADAPTER_CLASS_TMP))
ADAPTER_CLASSES_SRCPATH = $(subst $(space),:,$(strip $(WS_APPMANAGER_ADAPTER_SRCDIR)))
$(APPMANAGER_ADAPTER_CLASSESDIR)/%.class: %.java
		$(AT)echo $? >> .adapterclasses.list

#
# Rule to install the SAX jarfile prior to building the appmanager
# classes
#
JAXP_CLASS_TMP = $(subst .,/,$(JAXP_CLASSES))
JAXP_CLASS_FILES = \
        $(patsubst %,$(APPMANAGER_JAXP_CLASSESDIR)/%.class,$(JAXP_CLASS_TMP))
JAXP_CLASSES_SRCPATH = $(subst $(space),:,$(strip $(WS_APPMANAGER_JAXP_SRCDIR)))
$(APPMANAGER_JAXP_CLASSESDIR)/%.class: %.java
		$(AT)echo $? >> .jaxpclasses.list

$(SAX_JARFILE_BUILD): $(APPMANAGER_BUILD_DIRS) .delete.jaxpclasses.list .report.jaxpclasses $(JAXP_CLASS_FILES) .compile.jaxpclasses

.report.jaxpclasses:
	@echo "Checking for jaxp classes to compile..."

.compile.jaxpclasses:
	$(AT)if [ -s .jaxpclasses.list ] ; then                   \
                echo "Compiling jaxp classes...";                 \
                $(JAVAC_CMD)                                      \
                        -d $(APPMANAGER_JAXP_CLASSESDIR)          \
                        -sourcepath $(WS_APPMANAGER_JAXP_SRCDIR)  \
                        -bootclasspath $(CVM_BUILDTIME_CLASSESDIR)$(PS)$(LIB_CLASSESDIR) \
                        @.jaxpclasses.list ;                      \
        fi
	$(AT)(cd $(APPMANAGER_JAXP_CLASSESDIR); jar cf ../lib/$(APPMANAGER_SAX_JARFILE) * )

.delete.jaxpclasses.list:
	$(AT)$(RM) .jaxpclasses.list

provisioning_classes: .delete.provisioningclasses.list .report.provisioningclasses $(PROVISIONING_CLASS_FILES) .compile.provisioningclasses

.report.provisioningclasses:
	@echo "Checking for provisioning classes to compile..."

# Compile provisioning classes which will be deployed to a J2EE 1.3
# server. For the provisioning back-end classes to build properly we must
# have J2EE_HOME pointing to a locally installed J2EE 1.3 server instance,
# and JSR124_HOME pointing to an instance of a JSR-124 source tree.
# To use the error directive for these variables, we currently recurse.
.compile.provisioningclasses: $(PROVISIONING_CLASS_FILES)
ifneq ($(BUILDING_WEB),true)
	$(MAKE) BUILDING_WEB=true .compile.provisioningclasses
else
	$(AT)if [ -s .provisioningclasses.list ] ; then                   \
                echo "Compiling provisioning classes...";                 \
                $(JAVAC_CMD)                                            \
                        -d $(APPMANAGER_WI_CLASSESDIR)                     \
			-classpath $(J2EE_HOME)/lib/j2ee.jar$(PS)$(JSR124_HOME)/lib/ri.jar \
                        -sourcepath $(JSR124_PROV_SRCDIR)/web    \
                        @.provisioningclasses.list ;                      \
        fi
endif

.delete.provisioningclasses.list:
	$(AT)$(RM) .provisioningclasses.list

adapter_jar: .delete.adapterclasses.list .report.adapterclasses $(ADAPTER_CLASS_FILES) .compile.adapterclasses

.report.adapterclasses:
	@echo "Checking for adapter classes to compile..."

# Compile adapter classes which will go into the earfile to be deployed
# to a J2EE 1.3 server. J2EE_HOME must point to a locally visible instance
# of a J2EE 1.3 server, and JSR124_HOME must point to an instance of a
# JSR-124 source tree. To use the error directive for these variables,
# we currently recurse.
.compile.adapterclasses:
ifneq ($(BUILDING_WEB),true)
	$(MAKE) BUILDING_WEB=true .compile.adapterclasses
else
	$(AT)if [ -s .adapterclasses.list ] ; then                   \
                echo "Compiling adapter classes...";                 \
                $(JAVAC_CMD)                                            \
                        -d $(APPMANAGER_ADAPTER_CLASSESDIR)                     \
                        -sourcepath $(WS_APPMANAGER_ADAPTER_SRCDIR)    \
			-classpath $(J2EE_HOME)/lib/j2ee.jar$(PS)$(JSR124_HOME)/lib/ri.jar \
                        @.adapterclasses.list ;                      \
        fi
	$(AT)(cd $(APPMANAGER_ADAPTER_CLASSESDIR); jar cf ../war/WEB-INF/lib/$(APPMANAGER_ADAPTER_JARFILE) com )
endif

.delete.adapterclasses.list:
	$(AT)$(RM) .adapterclasses.list

web.war:
	$(AT)echo "Constructing Web Archive file"
	# Include unmodified files from the JSR124 installation.
	$(AT)cp -f $(JSR124_PROV_SRCDIR)/web/docs/index.html $(APPMANAGER_WARDIR)
	$(AT)cp -f $(JSR124_PROV_SRCDIR)/web/docs/stockingforms.html $(APPMANAGER_WARDIR)
	$(AT)cp -f $(JSR124_HOME)/lib/ri.jar $(APPMANAGER_WI_LIBDIR)
	$(AT)cp -f $(JSR124_HOME)/lib/generic.jar $(APPMANAGER_WI_LIBDIR)
	$(AT)cp -f $(JSR124_HOME)/lib/jnlp.jar $(APPMANAGER_WI_LIBDIR)
	$(AT)cp -f $(JSR124_HOME)/lib/midp.jar $(APPMANAGER_WI_LIBDIR)
	$(AT)cp -f $(JSR124_HOME)/lib/ri-ejb.jar $(APPMANAGER_WIDIR)
	$(AT)cp -f $(JSR124_PROV_SRCDIR)/matchers.xml $(APPMANAGER_WIDIR)
	# The xml configuration files for the JSR-124 test app
	# need to be modified to include information about our
	# adapter classes and client device. Do this by stripping
	# off the end of the relevant config files and adding our
	# own information.
	$(AT)sed -e '/^<\/adapters>$$/d' \
	    $(JSR124_PROV_SRCDIR)/adapters.xml | \
	    sed -e '$$r $(WS_APPMANAGER_PROV_DIR)/adapters.xml' \
            > $(APPMANAGER_WIDIR)/adapters.xml
	$(AT)sed '/^<\/devices>$$/d' $(JSR124_PROV_SRCDIR)/devices.xml | \
	    sed '$$r $(WS_APPMANAGER_PROV_DIR)/devices.xml' \
	    > $(APPMANAGER_WIDIR)/devices.xml
	$(AT)sed '/^<\/web-app>$$/d' $(JSR124_PROV_SRCDIR)/web/web.xml | \
	    sed '$$r $(WS_APPMANAGER_PROV_DIR)/web.xml' \
	    > $(APPMANAGER_WIDIR)/web.xml
	$(AT)cd $(APPMANAGER_WARDIR) ; $(CVM_JAR) cf ../$(APPMANAGER_WARFILE) *

web.ear: web.war
	$(AT)echo "Constructing Enterprise Archive file"
	$(AT)$(CVM_JAVA) \
		-classpath $(J2EE_HOME)/lib/j2ee.jar$(PS)$(J2EE_HOME)/lib/locale \
		com.sun.enterprise.tools.packager.Main \
		-enterpriseArchive $(CVM_BUILD_TOP)/$(APPMANAGER_WARFILE)$(PS)$(JSR124_HOME)/lib/ri-ejb.jar \
		ri_test \
		$(CVM_BUILD_TOP)/$(APPMANAGER_EARFILE)
	$(AT)echo "Setting Runtime Info in Archive file"
	$(AT)$(CVM_JAVA) \
		-classpath $(J2EE_HOME)/lib/j2ee.jar$(PS)$(J2EE_HOME)/lib/locale \
		com.sun.enterprise.tools.packager.Main \
		-setRuntime \
		$(CVM_BUILD_TOP)/$(APPMANAGER_EARFILE) \
		$(JSR124_PROV_SRCDIR)/web/sun-j2ee-ri.xml

clean::
	rm -f $(SAX_JARFILE_BUILD)
	rm -rf $(APPMANAGER_WARDIR)
	rm -rf $(APPMANAGER_PARDIR)
	rm -f $(CVM_BUILD_TOP)/$(APPMANAGER_WARFILE)
	rm -f $(CVM_BUILD_TOP)/$(APPMANAGER_EARFILE)
	rm -f $(CVM_BUILD_TOP)/$(APPMANAGER_PARFILE)
	rm -f $(CVM_BUILD_TOP)/$(APPMANAGER_ADAPTER_JARFILE)

#
# Rules to construct a deployment Parfile
#

# Construct the provisioning.xml file from a header piece, followed
# by xml fragments provided by each app/xlet to be downloaded, and
# then append a closing xml tag.
$(APPMANAGER_PAR_MIDIR)/provisioning.xml: $(PROVISIONED_APPS)
	$(AT)cp -f $(WS_APPMANAGER_PARDIR)/parheader.xml $@
	$(AT)chmod +w $@
	$(AT)for app in $(PROVISIONED_APPS) ; do \
	    cat $(WS_APPMANAGER_APPS_DIR)/$${app}/par/jsr124/$${app}.xml >> $@ ; \
	    echo "" >> $@ ; \
	done
	$(AT)echo "</provisioning-archive>" >> $@

# For each app, copy its jarfile, data descriptor and icon file (if
# any) to the parfile directory.
.construct.parlist: $(PROVISIONED_APPS)
	$(AT)echo "building parfile contents" ; \
	    for app in $(PROVISIONED_APPS) ; do \
	        cp -f $(CVM_BUILD_TOP)/repository/apps/$${app}/$${app}.jar $(APPMANAGER_PARDIR) ; \
	        cp -f $(WS_APPMANAGER_APPS_DIR)/$${app}/par/oma/$${app}.dd $(APPMANAGER_PARDIR) ; \
			cp -f $(WS_APPMANAGER_APPICONS_DIR)/$${app}.* $(APPMANAGER_PARDIR) ; \
	    done
	$(AT)cd $(APPMANAGER_PARDIR) ; $(CVM_JAR) cf ../$(APPMANAGER_PARFILE) *

#
# Rules to deploy the web enterprise archive file to a J2EE 1.3
# server. The J2EE Server must already be running on this host.
#
.deploy_earfile:
	$(AT)echo "deploying Enterprise Archive file to J2EE server"
	$(AT)$(CVM_JAVA) \
		-classpath $(J2EE_HOME)/lib/j2ee.jar$(PS)$(J2EE_HOME)/lib/locale \
		-Dorg.omg.CORBA.ORBInitialPort=1050 \
		-Dcom.sun.enterprise.home=$(J2EE_HOME) \
		-Djava.security.policy=$(J2EE_HOME)/lib/security/server.policy \
		com.sun.enterprise.tools.deployment.main.Main \
		-deploy $(CVM_BUILD_TOP)/$(APPMANAGER_EARFILE) $(J2EE_SERVER_HOST)

