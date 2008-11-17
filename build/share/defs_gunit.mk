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
# @(#)defs_gunit.mk	1.8 06/10/10
#
# GUnit Makefile
#

# guint depends on junit jar file
JUNIT_JARFILE ?= $(CVM_TOOLS_DIR)/java/junit/junit.jar
CVM_TEST_JARFILES += $(JUNIT_JARFILE)

CVM_TESTCLASSES_SRCDIRS += \
   $(CVM_TOP)/test/share/gunit/classes

CVM_TEST_CLASSES  += \
   gunit.framework.TestContainer \
   gunit.framework.BaseTestCase \
   gunit.framework.TestFilter \
   gunit.framework.TestCase \
   gunit.framework.TestContext \
   gunit.framework.TestFactory \
   gunit.framework.TestResultVerifier \
   gunit.framework.TestResultDescription \
   \
   gunit.lister.BaseTestLister \
   \
   gunit.textui.TestRunner \
   gunit.textui.ResultVerifier \
   gunit.textui.TestLister \
   gunit.textui.XMLTestLister \
   \
   gunit.container.AWTTestContainer \
   \
   gunit.image.RefImageNotFoundException \

ANT_LIB_DIR     ?= /usr/share/ant/lib
ANT_JUNIT_CP     = $(ANT_LIB_DIR)/ant-junit.jar$(PS)$(ANT_LIB_DIR)/ant.jar

GUNIT_CLASSPATH  = $(JUNIT_JARFILE)$(PS)$(CVM_TEST_CLASSESZIP)$(PS)$(CVM_LIBDIR)/basis.jar$(PS)$(ANT_JUNIT_CP) 
JUNIT_TESTRUNNER = org.apache.tools.ant.taskdefs.optional.junit.JUnitTestRunner

