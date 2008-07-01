<?xml version="1.0" encoding="UTF-8"?>
<!--
          

        Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
        DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
        
        This program is free software; you can redistribute it and/or
        modify it under the terms of the GNU General Public License version
        2 only, as published by the Free Software Foundation.
        
        This program is distributed in the hope that it will be useful, but
        WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
        General Public License version 2 for more details (a copy is
        included at /legal/license.txt).
        
        You should have received a copy of the GNU General Public License
        version 2 along with this work; if not, write to the Free Software
        Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
        02110-1301 USA
        
        Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
        Clara, CA 95054 or visit www.sun.com if you need additional
        information or have any questions.
-->
<!--
    This stylesheet outputs names of the packages for which constants
    definitions should be generated.
-->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method="text"/>

<xsl:template match="/">
<!-- for each node corresponding to package -->
<xsl:for-each select="/configuration/localized_strings/localized_string">
<!-- look for th first occurance of AMS_MGR_XXX_SETTINGS key
     The rest of the group is append in scanOneGroup -->
<xsl:if test="@Group and starts-with(@Key,'AMS_MGR') and not(contains(@Key,'SETTINGS_'))">
<xsl:value-of select="@Group"/>:
<xsl:call-template name="scanOneGroup">
       <xsl:with-param name="theGroup" select="@Group"/>
</xsl:call-template>
</xsl:if>
</xsl:for-each>
</xsl:template>

<xsl:template name="scanOneGroup">
<xsl:param name="theGroup"/>
<xsl:for-each select="/configuration/localized_strings/localized_string">
<xsl:if test="@Group and starts-with(@Group,$theGroup)">
<xsl:text> </xsl:text><xsl:value-of select="@Value"/>,
</xsl:if>
</xsl:for-each>
<xsl:text>
</xsl:text>
</xsl:template>

</xsl:stylesheet>
