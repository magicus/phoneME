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
# @(#)rules_zoneinfo.mk	1.8 06/10/19
#

#
# Makefile for building javazic utility & TimeZone resource files
#

$(ZONEINFO_CLASSES_DIR)/%.class: $(CVM_SHAREROOT)/tools/javazic/%.java
	$(AT)echo $? >> $(ZONEINFO_CLASSES_DIR)/.classes.list

$(ZONEINFO_CLASSES_DIR)/%.class: $(CVM_SHAREROOT)/classes/%.java 
	$(AT)echo $? >> $(ZONEINFO_CLASSES_DIR)/.classes.list

$(J2ME_CLASSLIB):: $(ZONEINFO_CLASSES_DIR) .delete.classlist $(FILES_class) .compile.classlist $(ZONEINFO_INSTALLDIR)/$(MAPFILE)

$(ZONEINFO_CLASSES_DIR):
	$(AT)mkdir -p $@

.delete.classlist:
	$(AT)rm -rf $(ZONEINFO_CLASSES_DIR)/.classes.list

.compile.classlist:
	$(AT)if [ -s $(ZONEINFO_CLASSES_DIR)/.classes.list ] ; then	\
	     echo "Compiling zic classes... ";				\
	     $(JAVAC_CMD)						\
			-d $(ZONEINFO_CLASSES_DIR)			\
			@$(ZONEINFO_CLASSES_DIR)/.classes.list ;	\
	fi

$(ZONEINFO_WORKDIR)/$(MAPFILE): $(FILES_class) $(TZFILES)
	$(AT)rm -rf $(ZONEINFO_WORKDIR)
	$(AT)$(CVM_JAVA) -classpath $(ZONEINFO_CLASSES_DIR) \
	    sun.tools.MyClassPath \
	    sun.tools.javazic.Main \
	    -V "$(TZDATA_VER)" -d $(ZONEINFO_WORKDIR) $(TZFILES)

$(ZONEINFO_INSTALLDIR)/$(MAPFILE): $(ZONEINFO_WORKDIR)/$(MAPFILE)
	$(AT)if [ ! -d $(ZONEINFO_INSTALLDIR) ] ; then \
		mkdir $(ZONEINFO_INSTALLDIR); \
	else \
		rm -rf $(ZONEINFO_INSTALLDIR)/*; \
	fi
	$(AT)cp -r $(ZONEINFO_WORKDIR)/* $(ZONEINFO_INSTALLDIR)

clean::
	rm -rf $(ZONEINFO_CLASSES_DIR) $(ZONEINFO_INSTALLDIR)
