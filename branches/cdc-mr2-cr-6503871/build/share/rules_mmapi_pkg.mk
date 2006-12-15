#
# @(#)rules_mmapi_pkg.mk	1.6 06/10/10
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

all:: mmapi

mmapi: mmapi-src $(CVM_MMAPI_JARFILE) $(CVM_MMAPI_LIBRARY) mmapi-demos

mmapi-src:      
	ant $(ANT_OPTIONS_MMAPI_LIBRARY) -f $(ANT_BUILD_FILE_MMAPI_LIBRARY)

$(CVM_MMAPI_JARFILE): $(MMAPI_JARFILE)
	cp -f $(MMAPI_JARFILE) $(CVM_MMAPI_JARFILE)

$(CVM_MMAPI_LIBRARY): $(MMAPI_LIBRARY)
	cp -f $(MMAPI_LIBRARY) $(CVM_MMAPI_LIBRARY)

mmapi-demos:
	ant $(ANT_OPTIONS_MMAPI_DEMOS) -f $(ANT_BUILD_FILE_MMAPI_DEMOS) compile
	(cd $(MMAPI_DEMOS_DIR)/build; rm -f $(MMAPI_DEMO_CLASSESJAR); jar cf $(MMAPI_DEMO_CLASSESJAR) *)
	cp $(MMAPI_DEMO_CLASSESJAR) $(CVM_MMAPI_DEMO_CLASSESJAR)
# I would like to use the command below, but can't get it to work, even when using cygpath
#	$(CVM_JAR) cvf $(CVM_MMAPI_DEMO_CLASSESJAR) -C $(MMAPI_DEMOS_DIR)/build *

clean::
	ant -f $(ANT_BUILD_FILE_MMAPI_LIBRARY) clean-all
	ant -f $(ANT_BUILD_FILE_MMAPI_DEMOS) clean
	rm -f $(CVM_MMAPI_LIBRARY)
