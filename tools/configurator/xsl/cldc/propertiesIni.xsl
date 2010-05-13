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
    This stylesheet outputs properties into an ini-file with two sections:
    [application] and [internal]
-->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method="text"/>

<xsl:template match="/">

<xsl:text>#
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

[application]
</xsl:text>
<!-- for each non-callout property with system scope -->
<xsl:for-each select="/configuration/properties/property[@Scope = 'system' and not(@Callout)]">
<!-- add comment if specified -->
<xsl:if test="@Comment != ''">
<xsl:text># </xsl:text><xsl:value-of select="@Comment"/>
</xsl:if>
<xsl:text>
</xsl:text>
<!-- output "name = value" pair -->
<xsl:value-of select="@Key"/> = <xsl:value-of select="@Value"/>
<xsl:text>
</xsl:text>
</xsl:for-each>

<xsl:text>

[internal]
</xsl:text>
<!-- for each non-callout property with internal scope -->
<xsl:for-each select="/configuration/properties/property[@Scope = 'internal' and not(@Callout)]">
<!-- add comment if specified -->
<xsl:if test="@Comment != ''">
<xsl:text># </xsl:text><xsl:value-of select="@Comment"/>
</xsl:if>
<xsl:text>
</xsl:text>
<!-- output "name = value" pair -->
<xsl:value-of select="@Key"/> = <xsl:value-of select="@Value"/>
<xsl:text>
</xsl:text>
</xsl:for-each>

</xsl:template>
</xsl:stylesheet>
