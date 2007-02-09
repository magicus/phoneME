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

$(CVM_DERIVEDROOT)/classes/java/util/CurrencyData.java: $(CVM_MISC_TOOLS_SRCDIR)/GenerateCurrencyData/GenerateCurrencyData.java $(CVM_SHAREDCLASSES_SRCDIR)/java/util/CurrencyData.properties
	@echo ... generating CurrencyData.java
	$(AT)$(JAVAC_CMD) -d $(CVM_MISC_TOOLS_CLASSPATH) \
		$(CVM_MISC_TOOLS_SRCDIR)/GenerateCurrencyData/GenerateCurrencyData.java
	$(AT)$(CVM_JAVA) \
		-classpath $(CVM_MISC_TOOLS_CLASSPATH) \
		GenerateCurrencyData \
		< $(CVM_SHAREDCLASSES_SRCDIR)/java/util/CurrencyData.properties > $@

# This rule is temporary. It should be removed when real PackageManager is implemented.
$(CVM_DERIVEDROOT)/classes/com/sun/cdc/config/PackageManager.java: $(CONFIGURATOR_JAR_FILE)
	@echo ... generating PackageManager.java
	$(AT)$(CVM_JAVA) -jar $(CONFIGURATOR_JAR_FILE) \
	-xml $(CVM_MISC_TOOLS_SRCDIR)/xml/empty.xml \
	-xsl $(CONFIGURATOR_DIR)/xsl/cdc/propertiesInit.xsl \
	-params propertyInitializers '$(PROPERTY_INITIALIZER_LIST)' \
	-out $(CVM_DERIVEDROOT)/classes/com/sun/cdc/config/PackageManager.java

-include ../share/rules_cdc-commercial.mk
include ../share/rules_zoneinfo.mk
