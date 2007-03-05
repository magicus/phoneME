#
# @(#)rules_webservices_pkg.mk	1.2 06/10/10
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

# Optional package for including the Web Services.

$(INSTALLDIR)/javadoc-webservices:
	mkdir -p $(INSTALLDIR)/javadoc-webservices

javadoc-webservices: $(INSTALLDIR)/javadoc-webservices 
	$(CVM_JAVADOC) \
	-source 1.4 -use -overview $(SUBSYSTEM_WEBSERVICES_SOURCEPATH)/../webservices-overview.html \
	-d $(INSTALLDIR)/javadoc-webservices -splitIndex \
	-windowtitle 'Web Services Optional Package' \
	-doctitle 'Web Services Optional Package' -header 'Web Services Optional Package' \
	-bottom '<font size="-1">Copyright 1994-2006 Sun Microsystems, Inc. All Rights Reserved.</font> ' \
	-classpath $(SUBSYSTEM_WEBSERVICES_SOURCEPATH) \
	-sourcepath $(JAVADOC_WEBSERVICES_SOURCEPATH) \
	$(SUBSYSTEM_WEBSERVICES_PACKAGES);
	(cd $(INSTALLDIR); \
	$(ZIP) -r -q - javadoc-webservices) > $(INSTALLDIR)/javadoc-webservices.zip

clean::
	@$(RM) -rf $(INSTALLDIR)/javadoc-webservices

