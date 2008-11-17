#
# @(#)rules_personal_peer_based.mk	1.9 06/10/10
# 
# Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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

printconfig::
	@echo "AWT_PEERSET        = $(AWT_PEERSET)"

#
# Include any peerset-specific, target-specific rules file.
#
-include $(CDC_DEVICE_COMPONENT_DIR)/build/$(TARGET_OS)-$(TARGET_CPU_FAMILY)-$(TARGET_DEVICE)/rules_$(J2ME_CLASSLIB)_$(AWT_PEERSET).mk
-include $(CDC_OS_COMPONENT_DIR)/build/$(TARGET_OS)/rules_personal_$(AWT_PEERSET).mk

#
# Include shared awt peerset specific makefile
#
-include $(CDC_DIR)/build/share/rules_personal_$(AWT_PEERSET).mk

