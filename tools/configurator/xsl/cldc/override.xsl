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
    This stylesheet overrides constants values in input file 
    by values from file specified as parameter
-->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method="xml" indent="yes"/>

<!-- stylesheet parameter: name of file with overrides -->
<xsl:param name="overrideFile"></xsl:param>

<xsl:variable name="constantOverrides" select="document($overrideFile)"/>

<xsl:template match="@* | node()">
    <xsl:copy>
        <xsl:apply-templates select="@* | node()" />
    </xsl:copy>
</xsl:template>

<xsl:template match="constant">
    <!-- constant's name  -->
    <xsl:variable name="constantName" select="@Name"/>

    <!-- lookup for overridden value  -->
    <xsl:variable name="newValue">
        <xsl:value-of select="$constantOverrides/configuration/constants/constant_class/constant[@Name = $constantName][1]/@Value"/>
    </xsl:variable>

    <!-- output 'Value' attribute -->
    <xsl:copy>
        <!-- check if value has been overridden -->
        <xsl:choose>
            <!-- if so, output new value -->
            <xsl:when test="string-length($newValue)">
                <xsl:attribute name="Value">
                    <xsl:value-of select="$newValue"/>
                </xsl:attribute>
            </xsl:when>
            <!-- otherwise output original value -->
            <xsl:otherwise>
                <xsl:attribute name="Value">
                    <xsl:value-of select="@Value"/>
                </xsl:attribute>
            </xsl:otherwise>
        </xsl:choose>
        
        <!-- output all other attributes -->
        <xsl:copy-of select="@*[local-name() != 'Value']" />
   </xsl:copy>
</xsl:template>

</xsl:stylesheet>

