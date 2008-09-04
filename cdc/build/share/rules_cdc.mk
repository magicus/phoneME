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

$(CVM_DERIVEDROOT)/classes/java/util/CurrencyData.java: $(CVM_MISC_TOOLS_SRCDIR)/GenerateCurrencyData/GenerateCurrencyData.java $(CVM_SHAREDCLASSES_SRCDIR)/java/util/CurrencyData.properties
	@echo ... generating CurrencyData.java
	$(AT)$(JAVAC_CMD) -d $(CVM_MISC_TOOLS_CLASSPATH) \
		$(CVM_MISC_TOOLS_SRCDIR)/GenerateCurrencyData/GenerateCurrencyData.java
	$(AT)$(CVM_JAVA) \
		-classpath $(CVM_MISC_TOOLS_CLASSPATH) \
		GenerateCurrencyData \
		< $(CVM_SHAREDCLASSES_SRCDIR)/java/util/CurrencyData.properties > $@

# This rule is temporary. It should be removed when real PackageManager is implemented.
$(CVM_DERIVEDROOT)/classes/com/sun/cdc/config/PackageManager.java: \
	    $(CONFIGURATOR_JAR_FILE) \
	    $(CVM_MISC_TOOLS_SRCDIR)/xml/empty.xml \
	    $(CONFIGURATOR_DIR)/xsl/cdc/propertiesInit.xsl
	@echo ... generating PackageManager.java
	$(AT)$(call runJarFile, $(CONFIGURATOR_JAR_FILE), \
	-xml $(CVM_MISC_TOOLS_SRCDIR)/xml/empty.xml \
	-xsl $(call POSIX2HOST,$(CONFIGURATOR_DIR)/xsl/cdc/propertiesInit.xsl)\
	-params initializers '$(JSR_INITIALIZER_LIST)' \
	-out $(CVM_DERIVEDROOT)/classes/com/sun/cdc/config/PackageManager.java)

# CDC test classes are built by the 'all' target.
build-unittests::

cdc-reports-clean:
	$(AT)$(RM) -rf $(CDC_REPORTS_DIR)

cdc-reports-dir: cdc-reports-clean
	$(AT)mkdir $(CDC_REPORTS_DIR)

clean:: cdc-reports-clean

# Run CDC tests with ANT's test runner saving reports to $(CDC_REPORTS_DIR)
GUNIT_CMD=$(foreach test,$(CVM_CDC_TESTS_TORUN), \
	$(CVM_JAVA) -classpath $(GUNIT_CLASSPATH) \
	$(JUNIT_TESTRUNNER) $(test) \
	printsummary=on \
	haltonfailure=false \
	formatter=org.apache.tools.ant.taskdefs.optional.junit.PlainJUnitResultFormatter,$(CDC_REPORTS_DIR)/TEST-$(test).txt \
	formatter=org.apache.tools.ant.taskdefs.optional.junit.XMLJUnitResultFormatter,$(CDC_REPORTS_DIR)/TEST-$(test).xml \
	haltonerror=false \
	filtertrace=true && ) true

run-unittests:: cdc-reports-dir
	$(AT)echo "====> start running CDC unit-tests"	
	$(check_JUNIT_JAR)
	$(AT)$(GUNIT_CMD)
	$(AT)echo "<==== end running CDC unit-tests"

-include $(CDC_DIR)/build/share/rules_cdc-commercial.mk
include $(CDC_DIR)/build/share/rules_zoneinfo.mk
