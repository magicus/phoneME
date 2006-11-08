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

PBP_PRESENTATION_MODE_JARFILE = PBP_PresentationMode.jar

PRESENTATION_MODE_JARFILES    += lib/$(PBP_PRESENTATION_MODE_JARFILE)
APPMANAGER_CLASSES            += $(PBP_PRESENTATION_MODE_IMPL_CLASSES)
APPS                          += $(PBP_APPS)
APP_IMAGEDIRS                 += $(PBP_APP_IMAGEDIRS)
APPMANAGER_BUILDDIRS          += $(PBP_PRESENTATION_MODE_RESOURCES_DIRS)

PBP_PRESENTATION_MODE_IMPL_CLASSES += \
	com.sun.appmanager.impl.presentation.PBP.PBPPresentationMode \
	com.sun.appmanager.impl.presentation.PBP.PBPButton \
	com.sun.appmanager.impl.presentation.PBP.PBPButtonListener \


PBP_PRESENTATION_MODE_RESOURCES_DIRS += \
	$(APPMANAGER_CLASSESDIR)/com/sun/appmanager/impl/presentation/PBP/resources

PBP_COPYRIGHT_APPS += \
	cdplayer \
	jonix \
	photoviewer \
	programguide \
	radio \
	tv \

PBP_COPYRIGHT_APPS_IMAGEDIRS += \

PBP_NON_COPYRIGHT_APPS += \

PBP_NON_COPYRIGHT_APPS_IMAGEDIRS += \

# Add new apps
ifeq ($(APPMANAGER_ALL_APPS), true)
PBP_APPS += \
	$(PBP_COPYRIGHT_APPS) \
	$(PBP_NON_COPYRIGHT_APPS)

PBP_APP_IMAGEDIRS +=  \
	$(PBP_COPYRIGHT_APPS_IMAGEDIRS) \
	$(PBP_NON_COPYRIGHT_APPS_IMAGEDIRS)
else
PBP_APPS += \
	$(PBP_COPYRIGHT_APPS)

PBP_APP_IMAGEDIRS +=  \
	$(PBP_COPYRIGHT_APPS_IMAGEDIRS)
endif

################################ APPS ################################

##### cdplayer
cdplayer_classes = \
	CDPlayer \

cdplayer_class_files = \
        $(patsubst %,$(cdplayer_classesdir)/%.class,$(cdplayer_classes))
cdplayer_classesdir = $(APPMANAGER_APPDIR)/cdplayer/classes
cdplayer_srcdir = $(WS_APPMANAGER_APPS_DIR)/cdplayer
cdplayer_icondir = $(cdplayer_srcdir)/icons

##### photoviewer
photoviewer_classes = \
	PhotoViewer \

photoviewer_class_files = \
        $(patsubst %,$(photoviewer_classesdir)/%.class,$(photoviewer_classes))
photoviewer_classesdir = $(APPMANAGER_APPDIR)/photoviewer/classes
photoviewer_srcdir = $(WS_APPMANAGER_APPS_DIR)/photoviewer
photoviewer_icondir = $(photoviewer_srcdir)/icons

##### programguide
programguide_classes = \
	ProgramGuide \

programguide_class_files = \
        $(patsubst %,$(programguide_classesdir)/%.class,$(programguide_classes))
programguide_classesdir = $(APPMANAGER_APPDIR)/programguide/classes
programguide_srcdir = $(WS_APPMANAGER_APPS_DIR)/programguide
programguide_icondir = $(programguide_srcdir)/icons 

##### radio
radio_classes = \
	Radio \

radio_class_files = \
        $(patsubst %,$(radio_classesdir)/%.class,$(radio_classes))
radio_classesdir = $(APPMANAGER_APPDIR)/radio/classes
radio_srcdir = $(WS_APPMANAGER_APPS_DIR)/radio
radio_icondir = $(radio_srcdir)/icons

##### tv
tv_classes = \
	TV \

tv_class_files = \
        $(patsubst %,$(tv_classesdir)/%.class,$(tv_classes))
tv_classesdir = $(APPMANAGER_APPDIR)/tv/classes
tv_srcdir = $(WS_APPMANAGER_APPS_DIR)/tv
tv_icondir = $(tv_srcdir)/icons

##### jonix
jonix_classes = \
	Demo \
	DemoButton \
	DemoButtonListener \
	GameDemo

jonix_class_files = \
        $(patsubst %,$(jonix_classesdir)/%.class,$(jonix_classes))
jonix_classesdir = $(APPMANAGER_APPDIR)/jonix/classes
jonix_srcdir = $(WS_APPMANAGER_APPS_DIR)/jonix
jonix_icondir = $(jonix_srcdir)/icon
