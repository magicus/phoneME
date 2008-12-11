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
    This stylesheet outputs all constants definitions in form of C header file
-->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:strip-space elements="*"/>
<xsl:output method="text"/>
<!-- for pretty printing the comments -->
<xsl:include href="prettyPrint.xsl"/>

<xsl:template match="/">
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

#include &lt;pcsl_string.h&gt;
#include &lt;midp_constants_data.h&gt;

<!-- for each constant_class -->
<xsl:for-each select="/configuration/constants/constant_class">
    <!-- 
        generate constants only when value of 'JavaOnly' 
        attribute isn't 'true'
    -->
    <xsl:if test="@JavaOnly!='true'">
        <xsl:apply-templates/>
    </xsl:if>
</xsl:for-each>
</xsl:template>

<!-- template to define string literal as array of character literals -->
<xsl:template name="escapeString">
<xsl:param name="str"/>
<xsl:param name="pos"/>
<xsl:param name="width"/>
    <xsl:if test="$pos &gt; $width and $pos mod $width = 1">
    	<xsl:text>&#10;    </xsl:text>
    </xsl:if>
    <xsl:if test="$pos = 1">
	<xsl:text>{</xsl:text>
    </xsl:if>
    <xsl:choose>
        <xsl:when test="$pos &lt;= string-length($str)">
    	    <xsl:text>'</xsl:text>
    	    <xsl:value-of select="substring($str, $pos, 1)"/>
    	    <xsl:text>', </xsl:text>
    	    <xsl:call-template name="escapeString">
        	<xsl:with-param name="str" select="$str"/>
        	<xsl:with-param name="pos" select="$pos + 1"/>
        	<xsl:with-param name="width" select="$width"/>
    	    </xsl:call-template>
        </xsl:when>
	<xsl:when test="$pos = string-length($str)+1">
    	    <xsl:text>'\0'}</xsl:text>
    	</xsl:when>
    </xsl:choose>
</xsl:template>

<!-- template to define PCSL constant string -->
<xsl:template name="pcslString">
<xsl:param name="name"/>
<xsl:param name="value"/>
PCSL_DEFINE_ASCII_STRING_LITERAL_START(<xsl:value-of select="$name"/>)
    <xsl:call-template name="escapeString">
        <xsl:with-param name="str" select="$value"/>
        <xsl:with-param name="pos" select="1"/>
        <xsl:with-param name="width" select="12"/>
    </xsl:call-template>
PCSL_DEFINE_ASCII_STRING_LITERAL_END(<xsl:value-of select="$name"/>);
</xsl:template>

<!-- template to generate String constant definition -->
<xsl:template match="constant">
<xsl:if test="@JavaOnly!='true'">
    <xsl:choose>
        <xsl:when test="@Type='String'">
	    <xsl:text>/**&#10;</xsl:text>
	    <xsl:call-template name="prettyPrint">
		<xsl:with-param name="outputString" select="normalize-space(@Comment)"/>
		<xsl:with-param name="indentString" select="' * '"/>
		<xsl:with-param name="maxWidth" select="75"/>
	    </xsl:call-template>
	    <xsl:text>&#10;</xsl:text>
	    <xsl:text> * &#10;</xsl:text>
	    <xsl:text> * Package: </xsl:text> 
	    <xsl:value-of select="../@Name"/> 
	    <xsl:text>&#10;</xsl:text>
	    <xsl:text> */</xsl:text>
            <xsl:call-template name="pcslString">
	        <xsl:with-param name="name" select="@Name"/>
    		<xsl:with-param name="value" select="@Value"/>
    	    </xsl:call-template>
        </xsl:when>
    </xsl:choose>
</xsl:if> <!-- JavaOnly -->
</xsl:template>
</xsl:stylesheet>
