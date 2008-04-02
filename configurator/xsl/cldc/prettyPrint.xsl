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
    This stylesheet contains a template for printing a string in a way
    where each line printed is no longer than specified number of characters.
-->
<xsl:stylesheet version="1.0" 
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">


<!--
    This template prints a string in a way where each line printed
    is no longer than specified number of characters.
-->
<xsl:template name="prettyPrint">

<!-- template parameter: string to print -->
<xsl:param name="outputString"/>

<!-- 
    template parameter: indentation string to print at 
    the beginning of each line.
-->
<xsl:param name="indentString"/>

<!-- template parameter: max width of each printed line -->
<xsl:param name="maxWidth"/>

<xsl:param name="isFirstLine" select="true()"/>
    <!-- if there anything to print -->
    <xsl:if test="boolean($outputString)">
        <xsl:choose>
            <!-- no formatting needed: string may be printed as is -->
            <xsl:when test="$maxWidth >= string-length($outputString)">
                <!-- output line break -->
                <xsl:if test="$isFirstLine = false()">
<xsl:text>
</xsl:text>
                </xsl:if>              
                <xsl:value-of select="$indentString"/>
                <xsl:value-of select="$outputString"/>
            </xsl:when>
            <!-- string is longer than max width. split it -->
            <xsl:otherwise>
                <!-- find where to split the string -->
                <xsl:variable name="splitPos">
                    <xsl:call-template name="findSplitPos">
                        <xsl:with-param name="str" select="$outputString"/>
                        <xsl:with-param name="maxWidth" select="$maxWidth"/>
                    </xsl:call-template>
                </xsl:variable>

                <!-- output line break -->
                <xsl:if test="$isFirstLine = false()">
<xsl:text>
</xsl:text>
                </xsl:if>

                <!-- output portion of the string up to the split position -->
                <xsl:value-of select="$indentString"/>
                <xsl:value-of select="substring($outputString, 1, $splitPos)"/>

                <!-- and print the rest of the string recursively. -->
                <xsl:call-template name="prettyPrint">
                    <xsl:with-param name="outputString" 
                        select="substring($outputString, $splitPos + 1)"/>
                    <xsl:with-param name="indentString" select="$indentString"/>
                    <xsl:with-param name="maxWidth" select="$maxWidth"/>
                    <xsl:with-param name="isFirstLine" select="false()"/>
                </xsl:call-template>
            </xsl:otherwise>
        </xsl:choose>
    </xsl:if>
</xsl:template>

<xsl:template name="findSplitPos">
<xsl:param name="str"/>
<xsl:param name="maxWidth"/>
    <xsl:choose>
        <xsl:when test="$maxWidth = 0">
            <xsl:value-of select="string-length($str)"/>
        </xsl:when>
        <xsl:otherwise>
            <xsl:choose>
                <!-- 
                    If we cut maxWidth chars from beginning of the string,
                    we will end up on word break. This is exactly what we
                    need, so return current maxWidth value as split position.
                -->
                <xsl:when test="substring($str, $maxWidth, 1 ) = ' '">
                    <xsl:value-of select="$maxWidth"/> 
                </xsl:when>
                <!--
                    Otherwise decrease maxWidth value by 1, and call this
                    template recursively.
                -->
                <xsl:otherwise>
                    <xsl:call-template name="findSplitPos">
                        <xsl:with-param name="str" select="$str"/>
                        <xsl:with-param name="maxWidth" select="$maxWidth - 1"/>
                    </xsl:call-template>
                </xsl:otherwise>
            </xsl:choose>
        </xsl:otherwise>
    </xsl:choose>
</xsl:template>

</xsl:stylesheet>

