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
# @(#)id_cdc.mk	1.2 06/10/10
#


J2ME_PROFILE_NAME		= CDC
J2ME_PROFILE_SPEC_VERSION	= 1.1

CVM_BUILD_ID 		= b168
CVM_BUILD_NAME		= CVM
CVM_BUILD_VERSION	= phoneme_advanced_mr2

# NOTE: the build/<os>-<cpu>-<device>/id_personal.mk file can be used
# to override the following values, which you may want to do for
# any product that is shipped.
J2ME_PRODUCT_NAME	= phoneME Advanced
J2ME_BUILD_VERSION	= $(CVM_BUILD_VERSION)
J2ME_BUILD_ID		= $(CVM_BUILD_ID)

# override with commercial versioning if present
ifdef CDC_PROJECT
ifeq ($(USE_CDC_COM),true)
-include $(CDC_COM_DIR)/projects/$(CDC_PROJECT)/build/share/id_project.mk
endif
endif

# override with any id_project.mk that may haven been placed in this directory
-include $(CDC_DIR)/build/share/id_project.mk
