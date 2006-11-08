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

# This makefile is now in charge of determining which presentation modes get built.
# The determination is made using the command line variable PRESENTATION_MODES.
# The use of this variable is similar to that of optional packages.
#   PRESENTATION_MODES=all | <mode name>[,<mode name>]
# Ex. make J2ME_PLATFORM=appmanager PRESENTATION_MODES=AwtPDA,PBP
#     make J2ME_PLATFORM=appmanager PRESENTATION_MODES=all

# When creating makefiles for presentation modes, you will likely need to use and
# add to the following list of makefile variables.  Use the currently available
# presentation mode makefiles as examples.  Makefile variables to use:
#
# PRESENTATION_MODE_FILES (Use += to add the name of the mode's jarfile)
# APPMANAGER_CLASSES (Use += to add the mode's class files)
# APPS (Use += to add this mode's list of applications)
# APP_IMAGEDIRS (Use += to add any image directories that need to be created for apps)
# APPMANAGER_BUILDDIRS (Use += to add any resource dirs that the mode needs created)


J2ME_CLASSLIB ?= personal
CVM_MTASK=true

ifeq ($(J2ME_CLASSLIB),basis)
  override APPMANAGER_BUILD_OTA=false
endif
APPMANAGER_BUILD_OTA ?= true

APPNAME = appmanager
APPMANAGER_TARFILE = $(APPNAME).tar
APPMANAGER_JARFILE = $(APPNAME).jar
APPMANAGER_CLIENT_JARFILE = $(APPNAME)-client.jar
APPMANAGER_CLASSESDIR = $(CVM_BUILD_TOP)/appmanager_classes
APPMANAGER_JAVADOCDIR = $(CVM_BUILD_TOP)/appmanager_javadoc
APPMANAGER_APPREPOSITORY_DIR = $(CVM_BUILD_TOP)/repository
APPMANAGER_APPMENU_DIR = $(APPMANAGER_APPREPOSITORY_DIR)/menu
APPMANAGER_APPICONS_DIR = $(APPMANAGER_APPREPOSITORY_DIR)/icons
APPMANAGER_APPDIR = $(APPMANAGER_APPREPOSITORY_DIR)/apps
APPMANAGER_PREFSDIR = $(APPMANAGER_APPREPOSITORY_DIR)/preferences
APPMANAGER_PREFSDIR_SYSTEM = $(APPMANAGER_APPREPOSITORY_DIR)/preferences/System
APPMANAGER_PREFSDIR_USER = $(APPMANAGER_APPREPOSITORY_DIR)/preferences/User

WS_APPMANAGER_SRCDIR = $(CVM_SHAREROOT)/appmanager/classes
WS_APPMANAGER_JAVADOC_DIR = $(CVM_SHAREROOT)/appmanager/javadoc
WS_APPMANAGER_APPS_DIR = $(CVM_SHAREROOT)/appmanager/repository/apps
WS_APPMANAGER_APPICONS_DIR = $(CVM_SHAREROOT)/appmanager/repository/icons
WS_APPMANAGER_APPMENU_DIR = $(CVM_SHAREROOT)/appmanager/repository/menu
WS_APPMANAGER_PROFILES_DIR = $(CVM_SHAREROOT)/appmanager/profiles/pp
WS_APPMANAGER_PREFSDIR_SYSTEM = $(CVM_SHAREROOT)/appmanager/preferences/System

DEFAULT_PRESENTATION_MODE=AwtPDA

# This is the bootclasspath that is used to compile each of our sample xlets
APPMANAGER_APP_BOOTCLASSPATH = $(CVM_BUILDTIME_CLASSESDIR)$(PS)$(LIB_CLASSESDIR)$(PS)$(CVM_LIBDIR)/$(APPMANAGER_CLIENT_JARFILE) 

# Now the security policy
WS_APPMANAGER_LIBDIR = $(CVM_SHAREROOT)/appmanager/lib
APPMANAGER_SECURITY_POLICY_NAME  = appmanager.security.policy
APPMANAGER_SECURITY_POLICY_SRC   = $(WS_APPMANAGER_LIBDIR)/security/$(APPMANAGER_SECURITY_POLICY_NAME)
APPMANAGER_SECURITY_POLICY_BUILD = $(CVM_LIBDIR)/security/$(APPMANAGER_SECURITY_POLICY_NAME)

# Now the permissive policy
APPMANAGER_SECURITY_PERMISSIVE_NAME  = appmanager.security.permissive
APPMANAGER_SECURITY_PERMISSIVE_SRC   = $(WS_APPMANAGER_LIBDIR)/security/$(APPMANAGER_SECURITY_PERMISSIVE_NAME)
APPMANAGER_SECURITY_PERMISSIVE_BUILD = $(CVM_LIBDIR)/security/$(APPMANAGER_SECURITY_PERMISSIVE_NAME)

# Now the constrained policy
APPMANAGER_SECURITY_CONSTRAINED_NAME  = appmanager.security.constrained
APPMANAGER_SECURITY_CONSTRAINED_SRC   = $(WS_APPMANAGER_LIBDIR)/security/$(APPMANAGER_SECURITY_CONSTRAINED_NAME)
APPMANAGER_SECURITY_CONSTRAINED_BUILD = $(CVM_LIBDIR)/security/$(APPMANAGER_SECURITY_CONSTRAINED_NAME)

# Add the src directory to vpath
JAVA_SRCDIRS += $(WS_APPMANAGER_SRCDIR)

# When compiling basis, override these variables
ifeq ($(J2ME_CLASSLIB), basis)
PRESENTATION_MODES := PBP
CVM_DEFINES  += -DBASIS_APPMANAGER
WS_APPMANAGER_PROFILES_DIR=$(CVM_SHAREROOT)/appmanager/profiles/pbp
endif

# The following is the set of classes that define the AppManager interfaces
APPMANAGER_JAVADOC_CLASSES = \
	com.sun.appmanager \
	com.sun.appmanager.mtask \
	com.sun.appmanager.apprepository \
	com.sun.appmanager.appmodel \
	com.sun.appmanager.store \
	com.sun.appmanager.preferences \
	com.sun.appmanager.services \
	com.sun.appmanager.client \
	com.sun.appmanager.ota \
	com.sun.appmanager.presentation

# The following is the set of public classes that need to be built
APPMANAGER_CLASSES += \
	com.sun.appmanager.AppManager \
	com.sun.appmanager.impl.CDCAmsAppManager \
	com.sun.appmanager.impl.CDCAmsAppController \
	com.sun.appmanager.mtask.Client \
	com.sun.appmanager.mtask.TaskListener \
	com.sun.xlet.mvmixc.CDCAmsIxcRegistry \
	com.sun.xlet.mvmixc.CDCAmsXletContext \
	com.sun.xlet.mvmixc.ServiceRegistry \
	com.sun.xlet.mvmixc.XletLauncher \
	com.sun.xlet.mvmixc.XletLauncherInterface \
	com.sun.xlet.mvmixc.XletLifecycleInterface \
	com.sun.xlet.mvmixc.XletLifecycleController \
	com.sun.tck.xlet.ManageableXlet \
	com.sun.tck.xlet.XletManager \
	com.sun.tck.xlet.CDCAmsXletManagerImpl

REPOSITORY_CLASSES += \
	com.sun.appmanager.apprepository.AppRepository \
	com.sun.appmanager.apprepository.AppModule

REPOSITORY_IMPL_CLASSES += \
	com.sun.appmanager.impl.apprepository.CDCAmsAppRepository

APP_MODEL_CLASSES += \
	com.sun.appmanager.appmodel.AppModelController \
	com.sun.appmanager.appmodel.XletAppModelController \

OTA_CLASSES += \
	com.sun.appmanager.ota.Application \
	com.sun.appmanager.ota.DLIndicator \
	com.sun.appmanager.ota.Descriptor \
	com.sun.appmanager.ota.Destination \
	com.sun.appmanager.ota.OTA \
	com.sun.appmanager.ota.OTAException \
	com.sun.appmanager.ota.OTAFactory \
	com.sun.appmanager.ota.OTANotSupportedException \
	com.sun.appmanager.ota.SyntaxException

OTA_IMPL_CLASSES += \
	com.sun.appmanager.impl.ota.CDCAmsOTAFactory

PRESENTATION_MODE_CLASSES += \
	com.sun.appmanager.presentation.Presentation \
	com.sun.appmanager.presentation.PresentationMode

PREFERENCES_CLASSES += \
	com.sun.appmanager.preferences.Preferences

PREFERENCES_IMPL_CLASSES += \
	com.sun.appmanager.impl.preferences.CDCAmsPreferences

STORE_CLASSES += \
	com.sun.appmanager.store.PersistentStore

STORE_IMPL_CLASSES += \
	com.sun.appmanager.impl.store.PersistentStore

CLIENT_CLASSES += \
	com.sun.appmanager.client.Client \

CLIENT_IMPL_CLASSES += \
	com.sun.appmanager.impl.client.CDCAmsClient \

APPMANAGER_CLASSES += $(PRESENTATION_MODE_CLASSES) \
	$(PREFERENCES_CLASSES) \
	$(PREFERENCES_IMPL_CLASSES) \
	$(CLIENT_CLASSES) \
	$(CLIENT_IMPL_CLASSES) \
	$(APP_MODEL_CLASSES) \
	$(REPOSITORY_CLASSES) \
	$(REPOSITORY_IMPL_CLASSES) \
	$(OTA_CLASSES) \
	$(OTA_IMPL_CLASSES)

APP_DIRS = $(APPS)

# Create the directories that will live off of build/<platform>
# by taking the values in APP_DIRS and pre-pending the path
# to build/<platform>.  Then append the classes directory.  The classes
# directory will contain the results of the build, but will be
# removed after the contents are placed in a jar file.
APPMANAGER_APPDIRS = \
	$(patsubst %,$(APPMANAGER_APPDIR)/%/classes,$(APP_DIRS))
APPMANAGER_APPIMAGEDIRS = \
	$(patsubst %,$(APPMANAGER_APPDIR)/%/classes/images,$(APP_IMAGEDIRS))
APPMANAGER_PROFILES_DIR = $(APPMANAGER_APPREPOSITORY_DIR)/profiles

APPMANAGER_RESOURCES_DIR = \
	$(APPMANAGER_CLASSESDIR)/com/sun/appmanager/resources

APPREPOSITORY_RESOURCES_DIRS += \
	$(APPMANAGER_CLASSESDIR)/com/sun/appmanager/apprepository/resources \
	$(APPMANAGER_CLASSESDIR)/com/sun/appmanager/apprepository/resources/icons

# directories that need to be created
APPMANAGER_BUILDDIRS += \
	$(APPMANAGER_PROFILES_DIR) \
	$(APPMANAGER_CLASSESDIR) \
	$(APPMANAGER_JAVADOCDIR) \
	$(APPMANAGER_RESOURCES_DIR) \
	$(APPREPOSITORY_RESOURCES_DIRS) \
	$(APPMANAGER_APPREPOSITORY_DIR) \
	$(APPMANAGER_APPDIR) \
	$(APPMANAGER_APPDIRS) \
	$(APPMANAGER_APPIMAGEDIRS) \
	$(APPMANAGER_APPMENU_DIR) \
	$(APPMANAGER_APPICONS_DIR) \
	$(APPMANAGER_PREFSDIR) \
	$(APPMANAGER_PREFSDIR_SYSTEM) \
	$(APPMANAGER_PREFSDIR_USER) \
	$(APPMANAGER_WI_CLASSESDIR) \
	$(APPMANAGER_WI_LIBDIR)

### Include presentation modes
empty:=
comma:= ,
space:= $(empty) $(empty)

ifeq ($(strip $(PRESENTATION_MODES)),)
  PRESENTATION_MODES=$(DEFAULT_PRESENTATION_MODE)
endif

ifneq ($(strip $(PRESENTATION_MODES)),)
  ifeq ($(PRESENTATION_MODES), all)
    PRESENTATION_MODES_DEFS_FILES := $(wildcard ../share/defs_appmanager_*_pmode.mk)
  else    
    PRESENTATION_MODES_DEFS_LIST  := $(subst $(comma),$(space),$(PRESENTATION_MODES))
    PRESENTATION_MODES_DEFS_FILES := $(patsubst %,defs_appmanager_%_pmode.mk,$(PRESENTATION_MODES_DEFS_LIST))
  endif
  PRESENTATION_MODES_RULES_FILES := $(subst defs,rules,$(PRESENTATION_MODES_DEFS_FILES))
endif

# Include presentation mode defs files
ifneq ($(PRESENTATION_MODES_DEFS_FILES),)
include $(patsubst %,../share/%,$(PRESENTATION_MODES_DEFS_FILES))
endif

# Include OTA defs file if appropriate
ifeq ($(APPMANAGER_BUILD_OTA),true)
-include ../share/defs_appmanager_ota.mk
endif
