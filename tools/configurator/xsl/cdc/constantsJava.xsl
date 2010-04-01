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
    This stylesheet outputs constants definitions for specified package 
    in form of Java class
-->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<!-- stylesheet parameter: name of the package to generate constants for -->
<xsl:strip-space elements="*"/>
<xsl:param name="fullClassName">error</xsl:param>
<xsl:output method="text"/>
<xsl:template match="/">
<!-- constant_class nodes with given package name -->
<xsl:variable name="constantClasses" select="/configuration/constants/constant_class[$fullClassName=concat(@Package,'.',@Name)]"/>
<xsl:if test="$constantClasses[1]/@NativeOnly!='true'">
/**
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions.
 * 
 * These are constant defines both in native and Java layers.
 * NOTE: DO NOT EDIT. THIS FILE IS GENERATED. If you want to 
 * edit it, you need to modify the corresponding XML files.
 *
 * Patent pending.
 */
package <xsl:value-of select="$constantClasses[1]/@Package"/>;
<!-- output class name -->
<xsl:value-of select="$constantClasses[1]/@Comment"/>
<xsl:value-of select="$constantClasses[1]/@Scope"/> class <xsl:value-of select="$constantClasses[1]/@Name"/> {
<xsl:for-each select="$constantClasses/constant">
<xsl:if test="@NativeOnly!='true'">
    /**
     * <xsl:value-of select="normalize-space(@Comment)"/>
     */
    <xsl:value-of select="@VScope"/> final static <xsl:value-of select="@Type"/>
    <xsl:text> </xsl:text> <xsl:value-of select="@Name"/>
    <xsl:text> = </xsl:text>
    <xsl:choose>
        <xsl:when test="@Type='String'">
            <xsl:text>"</xsl:text>
            <xsl:value-of select="@Value"/>
            <xsl:text>"</xsl:text>
        </xsl:when>
        <xsl:otherwise>
            <xsl:value-of select="@Value"/>
        </xsl:otherwise>
    </xsl:choose>
    <xsl:text>;&#10;</xsl:text>
</xsl:if> <!-- constant NativeOnly -->
</xsl:for-each>
}
</xsl:if> <!-- constant_class NativeOnly -->
</xsl:template>
</xsl:stylesheet>
