#
# @(#)defs_webservices_pkg.mk	1.3 06/10/10
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

# FIXME: Need to configure to auto-find in SVN repository
JSR_172_DIR=$(CVM_TOP)/../web-services-cdc
ifeq ($(wildcard $(JSR_172_DIR)/src/config/subsystem.gmk),)
$(error JSR_172_DIR must point to the JSR 172 directory)
endif
include $(JSR_172_DIR)/src/config/subsystem.gmk

OPT_PKGS_CLASSES += $(SUBSYSTEM_WEBSERVICES_JAVA_CLASSES)
OPT_PKGS_SRCPATH += $(SUBSYSTEM_WEBSERVICES_SOURCEPATH)

OPT_PKGS_JAVADOC_RULES += javadoc-webservices
JSR172_MANIFEST_FILENAME = $(JSR_172_DIR)/src/config/Manifest
WEBSERVICES_BUILD_ID = 1.0
