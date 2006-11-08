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

# If we are building the back-end server portions, we need
# pointers to a locally installed J2EE 1.3 server, and to a
# visible JSR-124 hierarchy. The build will fail without
# them, so we will abort right away.
ifeq ($(BUILDING_WEB),true)
  ifeq ($(strip $(J2EE_HOME)),)
    $(error J2EE_HOME must point to a locally installed J2EE 1.3 server.)
  endif
  ifeq ($(strip $(JSR124_HOME)),)
    $(error JSR124_HOME must point to a visible instance of a JSR-124 hierarchy.)
  endif
endif


# Definitions for the ri-test target
APPMANAGER_WARFILE = test.war
APPMANAGER_EARFILE = web.ear
APPMANAGER_WARDIR = $(CVM_BUILD_TOP)/war
APPMANAGER_WIDIR = $(APPMANAGER_WARDIR)/WEB-INF
APPMANAGER_WI_CLASSESDIR = $(APPMANAGER_WIDIR)/classes
APPMANAGER_WI_LIBDIR = $(APPMANAGER_WIDIR)/lib
WS_APPMANAGER_PROV_DIR = $(CVM_SHAREROOT)/appmanager/provisioning
JSR124_PROV_SRCDIR=$(JSR124_HOME)/test/src
APPMANAGER_JAXP_CLASSESDIR = $(CVM_BUILD_TOP)/jaxp_classes
WS_APPMANAGER_JAXP_SRCDIR = $(CVM_SHAREROOT)/appmanager/jaxp

# Definitions for the JSR124 server-side adapter target
APPMANAGER_ADAPTER_JARFILE = oma-adapter.jar
APPMANAGER_ADAPTER_CLASSESDIR = $(CVM_BUILD_TOP)/oma-adapter_classes
WS_APPMANAGER_ADAPTER_SRCDIR = $(WS_APPMANAGER_PROV_DIR)/adapters
J2EE_SERVER_HOST = local

# Definitions for the par file target
APPMANAGER_PARFILE = appmanager.par
APPMANAGER_PARDIR = $(CVM_BUILD_TOP)/par
APPMANAGER_PAR_MIDIR = $(APPMANAGER_PARDIR)/META-INF
WS_APPMANAGER_PARDIR = $(CVM_SHAREROOT)/appmanager/provisioning/par

# Add the SAX jarfile to the classpath for compilation
APPMANAGER_SAX_JARFILE = j2me_xml_cdc.jar
SAX_JARFILE_SRC = $(WS_APPMANAGER_LIBDIR)/$(APPMANAGER_SAX_JARFILE)
SAX_JARFILE_BUILD = $(CVM_LIBDIR)/$(APPMANAGER_SAX_JARFILE)

OTA_IMPL_CLASSES += \
	com.sun.appmanager.impl.ota.CDCAmsOTA \
	com.sun.appmanager.impl.ota.Document \
	com.sun.appmanager.impl.ota.DocumentElement \
	com.sun.appmanager.impl.ota.JNLPOTA \
	com.sun.appmanager.impl.ota.OMADescriptor \
	com.sun.appmanager.impl.ota.OMAOTA \
	com.sun.appmanager.impl.ota.MIDPOTA \
	com.sun.appmanager.impl.ota.MainApplication \
	com.sun.appmanager.impl.ota.MidletApplication \
	com.sun.appmanager.impl.ota.Parser \
	com.sun.appmanager.impl.ota.XletApplication

ifeq ($(BUILDING_WEB),true)
PROVISIONING_CLASSES += \
	ByteBuffer \
	DeviceTableTest \
	Discover \
	Download \
	Dump \
	FilterTest \
	JAM \
	JavaWS \
	MultiPartParser \
	MyCapabilities \
	MyDeliveryComponent \
	MyDeliveryContext \
	Part \
	ShowBundle \
	ShowBundles \
	ShowText \
	Stocking \
	TestMatchPolicy \
	UploadPAR
endif

ADAPTER_CLASSES += \
	com.sun.provisioning.adapters.oma.AdapterOMA \
	com.sun.provisioning.adapters.oma.ClientRequest \
	com.sun.provisioning.adapters.oma.DescriptorFileDD \
	com.sun.provisioning.adapters.oma.DiscoveryOMA \
	com.sun.provisioning.adapters.oma.MimeTool \
	com.sun.provisioning.adapters.oma.ServletOMA

APPMANAGER_CLASSES += \
	$(OTA_IMPL_CLASSES) \

# directories that need to be created
APPMANAGER_BUILDDIRS += \
	$(APPMANAGER_ADAPTER_CLASSESDIR) \
	$(APPMANAGER_PAR_MIDIR) \
	$(APPMANAGER_JAXP_CLASSESDIR)

# Add source directories to the vpath
JAVA_SRCDIRS += \
	$(WS_APPMANAGER_JAXP_SRCDIR) \
	$(WS_APPMANAGER_ADAPTER_SRCDIR)
ifeq ($(BUILDING_WEB),true)
JAVA_SRCDIRS += \
	$(JSR124_PROV_SRCDIR)/web
endif

JAXP_CLASSES += \
        javax.xml.parsers.SAXParser \
        javax.xml.parsers.SAXParserFactory \
        javax.xml.parsers.FactoryConfigurationError \
        javax.xml.parsers.ParserConfigurationException \
        org.xml.sax.Attributes \
        org.xml.sax.InputSource \
        org.xml.sax.Locator \
        org.xml.sax.SAXException \
        org.xml.sax.SAXParseException \
        org.xml.sax.SAXNotRecognizedException \
        org.xml.sax.SAXNotSupportedException \
        org.xml.sax.helpers.DefaultHandler \
        com.sun.ukit.jaxp.ParserFactory \
        com.sun.ukit.jaxp.Parser \
        com.sun.ukit.jaxp.Attrs \
        com.sun.ukit.jaxp.Pair \
        com.sun.ukit.jaxp.Input \
        com.sun.ukit.jaxp.ReaderUTF8 \
        com.sun.ukit.jaxp.ReaderUTF16

