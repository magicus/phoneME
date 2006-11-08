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


all:: PBP_compile_apps PBP_resources PBP_install

PBP_compile_apps: $(PBP_APPS)

PBP_resources:
	$(AT)cp -f $(WS_APPMANAGER_SRCDIR)/com/sun/appmanager/impl/presentation/PBP/resources/*.properties $(APPMANAGER_CLASSESDIR)/com/sun/appmanager/impl/presentation/PBP/resources/

PBP_install:
	$(AT)echo "Installing PBP Presentation Mode..."
	$(AT)cp -f $(WS_APPMANAGER_APPMENU_DIR)/cdplayer.app $(APPMANAGER_APPMENU_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPMENU_DIR)/jonix.app $(APPMANAGER_APPMENU_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPMENU_DIR)/radio.app $(APPMANAGER_APPMENU_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPMENU_DIR)/tv.app $(APPMANAGER_APPMENU_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPMENU_DIR)/photoviewer.app $(APPMANAGER_APPMENU_DIR)/
	$(AT)cp -f $(WS_APPMANAGER_APPMENU_DIR)/programguide.app $(APPMANAGER_APPMENU_DIR)/
	$(AT)cd $(APPMANAGER_CLASSESDIR); $(CVM_JAR) cf ../lib/$(PBP_PRESENTATION_MODE_JARFILE) com/sun/appmanager/impl/presentation/PBP/*

################################ APPS ################################

# add the list of all apps here
compile_apps: $(APPS)

##### cdplayer
cdplayer:  .delete.cdplayerclasses.list .report.cdplayerclasses $(cdplayer_class_files) .compile.cdplayerclasses .copy.cdplayerresources .jar.cdplayerclasses

$(cdplayer_classesdir)/%.class: $(cdplayer_srcdir)/%.java
	$(AT)echo $? >> .cdplayerclasses.list

.report.cdplayerclasses:
	@echo "Checking for cdplayer classes to compile..."

.compile.cdplayerclasses:
	$(AT)if [ -s .cdplayerclasses.list ] ; then                   \
                echo "Compiling cdplayer classes...";                 \
                $(JAVAC_CMD)                                            \
                        -d $(cdplayer_classesdir)                     \
                        -bootclasspath $(APPMANAGER_APP_BOOTCLASSPATH) \
                        -sourcepath $(cdplayer_srcdir)       \
                        @.cdplayerclasses.list ;                      \
        fi

.delete.cdplayerclasses.list:
	$(AT)$(RM) .cdplayerclasses.list

.copy.cdplayerresources:
	$(AT)echo "Copying cdplayer resources..."
	$(AT)cp -f $(cdplayer_srcdir)/*.png $(cdplayer_classesdir)/

.jar.cdplayerclasses:
	$(AT)echo "Jarring cdplayer..."
	$(AT)cd $(cdplayer_classesdir); $(CVM_JAR) cf ../cdplayer.jar *
	$(AT)cd $(CVM_BUILD_TOP); rm -rf $(cdplayer_classesdir)


##### jonix
jonix:  .delete.jonixclasses.list .report.jonixclasses $(jonix_class_files) .compile.jonixclasses .copy.jonixresources .jar.jonixclasses

$(jonix_classesdir)/%.class: $(jonix_srcdir)/%.java
	$(AT)echo $? >> .jonixclasses.list

.report.jonixclasses:
	@echo "Checking for jonix classes to compile..."

.compile.jonixclasses:
	$(AT)if [ -s .jonixclasses.list ] ; then                   \
                echo "Compiling jonix classes...";                 \
                $(JAVAC_CMD)                                            \
                        -d $(jonix_classesdir)                     \
                        -bootclasspath $(APPMANAGER_APP_BOOTCLASSPATH) \
                        -sourcepath $(jonix_srcdir)       \
                        @.jonixclasses.list ;                      \
        fi

.delete.jonixclasses.list:
	$(AT)$(RM) .jonixclasses.list

.copy.jonixresources:
	$(AT)echo "Copying jonix resources..."
	$(AT)cp -f $(jonix_srcdir)/*.gif $(jonix_classesdir)/

.jar.jonixclasses:
	$(AT)echo "Jarring jonix..."
	$(AT)cd $(jonix_classesdir); $(CVM_JAR) cf ../jonix.jar *
	$(AT)cd $(CVM_BUILD_TOP); rm -rf $(jonix_classesdir)


##### photoviewer
photoviewer:  .delete.photoviewerclasses.list .report.photoviewerclasses $(photoviewer_class_files) .compile.photoviewerclasses .copy.photoviewerresources .jar.photoviewerclasses

$(photoviewer_classesdir)/%.class: $(photoviewer_srcdir)/%.java
	$(AT)echo $? >> .photoviewerclasses.list

.report.photoviewerclasses:
	@echo "Checking for photoviewer classes to compile..."

.compile.photoviewerclasses:
	$(AT)if [ -s .photoviewerclasses.list ] ; then                   \
                echo "Compiling photoviewer classes...";                 \
                $(JAVAC_CMD)                                            \
                        -d $(photoviewer_classesdir)                     \
                        -bootclasspath $(APPMANAGER_APP_BOOTCLASSPATH) \
                        -sourcepath $(photoviewer_srcdir)       \
                        @.photoviewerclasses.list ;                      \
        fi

.delete.photoviewerclasses.list:
	$(AT)$(RM) .photoviewerclasses.list

.copy.photoviewerresources:
	$(AT)echo "Copying photoviewer resources..."
	$(AT)cp -f $(photoviewer_srcdir)/*.jpg $(photoviewer_classesdir)/

.jar.photoviewerclasses:
	$(AT)echo "Jarring photoviewer..."
	$(AT)cd $(photoviewer_classesdir); $(CVM_JAR) cf ../photoviewer.jar *
	$(AT)cd $(CVM_BUILD_TOP); rm -rf $(photoviewer_classesdir)


##### programguide
programguide:  .delete.programguideclasses.list .report.programguideclasses $(programguide_class_files) .compile.programguideclasses .copy.programguideresources .jar.programguideclasses

$(programguide_classesdir)/%.class: $(programguide_srcdir)/%.java
	$(AT)echo $? >> .programguideclasses.list

.report.programguideclasses:
	@echo "Checking for programguide classes to compile..."

.compile.programguideclasses:
	$(AT)if [ -s .programguideclasses.list ] ; then                   \
                echo "Compiling programguide classes...";                 \
                $(JAVAC_CMD)                                            \
                        -d $(programguide_classesdir)                     \
                        -bootclasspath $(APPMANAGER_APP_BOOTCLASSPATH) \
                        -sourcepath $(programguide_srcdir)       \
                        @.programguideclasses.list ;                      \
        fi

.delete.programguideclasses.list:
	$(AT)$(RM) .programguideclasses.list

.copy.programguideresources:
	$(AT)echo "Copying programguide resources..."
	$(AT)cp -f $(programguide_srcdir)/*.jpg $(programguide_classesdir)/

.jar.programguideclasses:
	$(AT)echo "Jarring programguide..."
	$(AT)cd $(programguide_classesdir); $(CVM_JAR) cf ../programguide.jar *
	$(AT)cd $(CVM_BUILD_TOP); rm -rf $(programguide_classesdir)

##### radio
radio:  .delete.radioclasses.list .report.radioclasses $(radio_class_files) .compile.radioclasses .copy.radioresources .jar.radioclasses

$(radio_classesdir)/%.class: $(radio_srcdir)/%.java
	$(AT)echo $? >> .radioclasses.list

.report.radioclasses:
	@echo "Checking for radio classes to compile..."

.compile.radioclasses:
	$(AT)if [ -s .radioclasses.list ] ; then                   \
                echo "Compiling radio classes...";                 \
                $(JAVAC_CMD)                                            \
                        -d $(radio_classesdir)                     \
                        -bootclasspath $(APPMANAGER_APP_BOOTCLASSPATH) \
                        -sourcepath $(radio_srcdir)       \
                        @.radioclasses.list ;                      \
        fi

.delete.radioclasses.list:
	$(AT)$(RM) .radioclasses.list

.copy.radioresources:
	$(AT)echo "Copying radio resources..."
	$(AT)cp -f $(radio_srcdir)/*.png $(radio_classesdir)/

.jar.radioclasses:
	$(AT)echo "Jarring radio..."
	$(AT)cd $(radio_classesdir); $(CVM_JAR) cf ../radio.jar *
	$(AT)cd $(CVM_BUILD_TOP); rm -rf $(radio_classesdir)

##### tv
tv:  .delete.tvclasses.list .report.tvclasses $(tv_class_files) .compile.tvclasses .copy.tvresources .jar.tvclasses

$(tv_classesdir)/%.class: $(tv_srcdir)/%.java
	$(AT)echo $? >> .tvclasses.list

.report.tvclasses:
	@echo "Checking for tv classes to compile..."

.compile.tvclasses:
	$(AT)if [ -s .tvclasses.list ] ; then                   \
                echo "Compiling tv classes...";                 \
                $(JAVAC_CMD)                                            \
                        -d $(tv_classesdir)                     \
                        -bootclasspath $(APPMANAGER_APP_BOOTCLASSPATH) \
                        -sourcepath $(tv_srcdir)       \
                        @.tvclasses.list ;                      \
        fi

.delete.tvclasses.list:
	$(AT)$(RM) .tvclasses.list

.copy.tvresources:
	$(AT)echo "Copying tv resources..."
	$(AT)cp -f $(tv_srcdir)/*.jpg $(tv_classesdir)/

.jar.tvclasses:
	$(AT)echo "Jarring tv..."
	$(AT)cd $(tv_classesdir); $(CVM_JAR) cf ../tv.jar *
	$(AT)cd $(CVM_BUILD_TOP); rm -rf $(tv_classesdir)
