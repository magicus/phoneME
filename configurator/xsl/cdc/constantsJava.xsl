<?xml version="1.0" encoding="UTF-8"?>
<!--
          

        Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved.
        DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
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
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
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
