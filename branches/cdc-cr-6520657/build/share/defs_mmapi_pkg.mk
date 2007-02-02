#
# @(#)defs_mmapi_pkg.mk	1.8 06/10/10
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

# Optional package for including the Multi Media Api's

ifeq ($(J2ME_CLASSLIB), cdc)
$(error MMAPI O.P. requires at least Personal Basis Profile (J2ME_CLASSLIB=basis) or Personal Profile (J2ME_CLASSLIB=personal))
endif

ifeq ($(J2ME_CLASSLIB), foundation)
$(error MMAPI O.P. requires at least Personal Basis Profile (J2ME_CLASSLIB=basis) or Personal Profile (J2ME_CLASSLIB=personal))
endif

ifeq ($(MMAPI_HOME),)
$(error MMAPI_HOME must be defined to compile the MMAPI Optional Package.  MMAPI_HOME should point to the directory containing the MMAPI source)
endif

# No need to use OPT_PKGS_SRCPATH and OPT_PKGS_CLASSES since this optional package already uses ant.
ANT_BUILD_FILE_MMAPI_LIBRARY = $(MMAPI_HOME)/build/build.xml

### Name of the jar file built by the MMAPI's build system
MMAPI_JARFILE_NAME = mmapi.jar
CVM_MMAPI_JARFILE_NAME = javame_cdc_mmapi.jar

### Name of the native library file built by the MMAPI's build system
ifeq ($(HOST_DEVICE), win32)
MMAPI_LIBRARY_NAME = mmapi.dll
else 
MMAPI_LIBRARY_NAME = libmmapi.so
endif

### Rename the library file if necessary for a debug build.  The MMAPI's build system
### does not rename debug library files with a _g as needed by the CVM.
ifeq ($(CVM_DEBUG), true)
    ANT_OPTIONS_MMAPI_LIBRARY += -Ddebug=true
    ANT_OPTIONS_MMAPI_DEMOS += -Ddebug=true
ifeq ($(HOST_DEVICE), win32)
    CVM_MMAPI_LIBRARY_NAME = libmmapi_g.dll
else
    CVM_MMAPI_LIBRARY_NAME = libmmapi_g.so
endif
else 
    ifeq ($(HOST_DEVICE), win32)
        CVM_MMAPI_LIBRARY_NAME = libmmapi.dll
    else
        CVM_MMAPI_LIBRARY_NAME = $(MMAPI_LIBRARY_NAME)
    endif
endif

CURRENT_DIR := $(shell pwd)

### Settings for personal profile builds
ifeq ($(J2ME_CLASSLIB), personal)
    ANT_OPTIONS_MMAPI_LIBRARY += -Dneed.amms=false -Dpersonal.btclasses=${CURRENT_DIR}/${CVM_BUILDTIME_CLASSESZIP} -Dpersonal.jar=${CURRENT_DIR}/${LIB_CLASSESJAR} build-personal
### Give CVM the system properties defined in the configuration's system.properties file
    MMAPI_SYSTEM_PROPERTIES_FILE = $(MMAPI_HOME)/src/components/personal/configuration/system.properties
    MMAPI_JARFILE  = $(MMAPI_HOME)/build/targets/personal/release/$(MMAPI_JARFILE_NAME)
    MMAPI_LIBRARY  = $(MMAPI_HOME)/build/targets/personal/release/$(MMAPI_LIBRARY_NAME)
    MMAPI_DEMOS_DIR = $(MMAPI_HOME)/demo/personal
endif

### Settings for basis profile builds
ifeq ($(J2ME_CLASSLIB), basis)
ifeq ($(HOST_DEVICE), win32)
    JARFILE   := $(call POSIX2WIN,${CURRENT_DIR}/${LIB_CLASSESJAR})
    BTCLASSES := $(call POSIX2WIN,${CURRENT_DIR}/${CVM_BUILD_TOP}/${CVM_BUILDTIME_CLASSESZIP})
    MMAPI_LIBRARY  = $(MMAPI_HOME)/build/targets/personal-basis/release/$(MMAPI_LIBRARY_NAME)
    MMAPI_JARFILE  = $(MMAPI_HOME)/build/targets/personal-basis/release/$(MMAPI_JARFILE_NAME)
    MMAPI_DEMOS_DIR = $(MMAPI_HOME)/demo/personal-basis
else
### The values in this else block have not been tested and will need to change.
    ANT_OPTIONS_MMAPI_LIBRARY += -Dneed.amms=false -Dpersonal-basis.btclasses=${CURRENT_DIR}/${CVM_BUILD_TOP}/${CVM_BUILDTIME_CLASSESZIP} -Dpersonal-basis.jar=${CURRENT_DIR}/${LIB_CLASSESJAR} build-personal-basis
### Give CVM the system properties defined in the configuration's system.properties file
    MMAPI_JARFILE  = $(MMAPI_HOME)/build/targets/personal-basis/release/$(MMAPI_JARFILE_NAME)
    MMAPI_LIBRARY  = $(MMAPI_HOME)/build/targets/personal-basis/release/$(MMAPI_LIBRARY_NAME)
    MMAPI_DEMOS_DIR = $(MMAPI_HOME)/demo/personal-basis
endif
    ANT_OPTIONS_MMAPI_LIBRARY += -Dneed.amms=false -Dpersonal-basis.btclasses=${BTCLASSES} -Dpersonal-basis.jar=${JARFILE} build-personal-basis
endif

### Location of libraries in the CDC-based workspace
CVM_MMAPI_JARFILE = $(CVM_LIBDIR)/$(CVM_MMAPI_JARFILE_NAME)
CVM_MMAPI_LIBRARY = $(CVM_LIBDIR)/$(CVM_MMAPI_LIBRARY_NAME)
MMAPI_DEMO_CLASSESJAR_NAME = mmapi-democlasses.jar
MMAPI_DEMO_CLASSESJAR = $(MMAPI_DEMOS_DIR)/build/$(MMAPI_DEMO_CLASSESJAR_NAME)
CVM_MMAPI_DEMO_CLASSESJAR = $(CVM_LIBDIR)/$(MMAPI_DEMO_CLASSESJAR_NAME)

### Add the mmapi jarfile to the default search path
CVM_JARFILES +=, "$(CVM_MMAPI_JARFILE_NAME)"

MMAPI_SYSTEM_PROPERTIES_FILE = $(MMAPI_HOME)/src/components/personal-basis/configuration/system.properties
MMAPI_SYSTEM_PROPERTIES := $(shell cat $(MMAPI_SYSTEM_PROPERTIES_FILE) | sed 's/^\#.*//' )
SYSTEM_PROPERTIES += $(MMAPI_SYSTEM_PROPERTIES)

ANT_BUILD_FILE_MMAPI_DEMOS = $(MMAPI_DEMOS_DIR)/build.xml
