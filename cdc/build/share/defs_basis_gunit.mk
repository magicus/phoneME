

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
# @(#)defs_basis_gunit.mk	1.8 06/10/10
#
# GUnit Makefile
#

CVM_TESTCLASSES_SRCDIRS += \
   $(CVM_TOP)/test/share/basis/gunit/classes \
   $(CVM_TOP)/test/share/basis/gunittests

# Basis GUnit Test cases
CVM_TEST_CLASSES  += \
   tests.volatileImage.ImageTest \
   tests.appcontext.FocusMgmtTest \
   tests.appcontext.FullScreenTest \
   tests.ixcpermission.IxcPermissionTest \

# Tests to run by make run-unittests.
CVM_CDC_TESTS_TORUN = \
   tests.appcontext.FocusMgmtTest \
   tests.ixcpermission.IxcPermissionTest

# tests.volatileImage.ImageTest is interactive
# tests.appcontext.FullScreenTest fails

include $(CDC_DIR)/build/share/defs_gunit.mk
