<?xml version="1.0" ?>
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

<xsl:stylesheet version="2.0"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:fn="http://www.w3.org/2005/xpath-functions">

    <!--
        Utility templates that doesn't depend on context item
    -->
    <xsl:template name="classname2filepath">
        <xsl:param name="classname"/>
        <xsl:value-of select="concat($output-java-dir,'/',$classname,'.java')"/>
    </xsl:template>

    <xsl:template name="toupper">
        <xsl:param name="str"/>
        <xsl:value-of select="translate($str, 'abcdefghijklmnopqrstuvwxyz', 'ABCDEFGHIJKLMNOPQRSTUVWXYZ')"/>
    </xsl:template>

    <xsl:template name="replace">
        <xsl:param name="str"/>
        <xsl:param name="from"/>
        <xsl:param name="to"/>

        <xsl:variable name="head" select="substring-before($str, $from)"/>
        <xsl:value-of select="$head"/>
        <xsl:choose>
            <xsl:when test="$from=substring($str,string-length($head)+1,string-length($from))">
                <xsl:value-of select="$to"/>
                <xsl:call-template name="replace">
                    <xsl:with-param name="str" select="substring($str,string-length($head)+string-length($from)+1)"/>
                    <xsl:with-param name="from" select="$from"/>
                    <xsl:with-param name="to" select="$to"/>
                </xsl:call-template>
            </xsl:when>
            <xsl:otherwise>
                <xsl:value-of select="$str"/>
            </xsl:otherwise>
        </xsl:choose>
    </xsl:template>


    <xsl:template name="uniq">
        <xsl:param name="str"/>
        <xsl:variable name="res">
            <xsl:for-each select="fn:distinct-values(tokenize($str, '&#10;'))">
                <xsl:if test=".">
                    <xsl:copy-of select="concat(.,'&#10;')"/>
                </xsl:if>
            </xsl:for-each>
        </xsl:variable>
        <xsl:value-of select="fn:replace($res,'&#10; ','&#10;')"/>
    </xsl:template>


    <xsl:template name="expand">
        <xsl:param name="str"/>
        <xsl:param name="action"/>
        <xsl:param name="action-arg0"/>
        <xsl:param name="level" select="number('0')"/>
        <xsl:param name="filtered-str">
            <xsl:text>"</xsl:text>
            <xsl:call-template name="replace">
                <xsl:with-param name="str" select="translate($str, '%', '&#333;')"/>
                <xsl:with-param name="from" select="'&#333;&#333;'"/>
                <xsl:with-param name="to" select="'%%'"/>
            </xsl:call-template>
            <xsl:text>"</xsl:text>
        </xsl:param>

        <xsl:variable name="tail" select="substring-after($filtered-str, '&#333;')"/>
        <xsl:choose>
            <xsl:when test="$tail">
                <xsl:variable name="varname" select="substring-before($tail, '&#333;')"/>
                <xsl:if test="not($varname)">
                    <xsl:message terminate="yes">
                        <xsl:text>Missing '%' character detected in: "</xsl:text>
                        <xsl:value-of select="$str"/>
                        <xsl:text>"&#10;</xsl:text>
                    </xsl:message>
                </xsl:if>
                <xsl:variable name="keyname">
                    <xsl:text>KEY_</xsl:text>
                    <xsl:call-template name="toupper">
                        <xsl:with-param name="str" select="$varname"/>
                    </xsl:call-template>
                </xsl:variable>
                <xsl:choose>
                    <xsl:when test="$action='get-printf-args'">
                        <xsl:value-of select="concat('getProperty(',$keyname,'), ')"/>
                    </xsl:when>
                    <xsl:when test="$action='define-argkey'">
                        <xsl:text>    public final static String </xsl:text>
                        <xsl:value-of select="$keyname"/>
                        <xsl:text>="</xsl:text>
                        <xsl:value-of select="$varname"/>
                        <xsl:text>";&#10;</xsl:text>
                    </xsl:when>
                    <xsl:when test="$action='get-printf-format'">
                        <xsl:value-of select="substring-before($filtered-str, '&#333;')"/>
                        <xsl:text>%</xsl:text>
                        <xsl:value-of select="$level"/>
                    </xsl:when>
                    <xsl:when test="$action='utest-get-value'">
                        <xsl:text>                    else if (</xsl:text>
                        <xsl:value-of select="concat($action-arg0,$keyname)"/>
                        <xsl:text>.equals(key)) return "&lt;</xsl:text>
                        <xsl:value-of select="$varname"/>
                        <xsl:text>&gt;";&#10;</xsl:text>
                    </xsl:when>
                </xsl:choose>
                <xsl:call-template name="expand">
                    <xsl:with-param name="str" select="$str"/>
                    <xsl:with-param name="action" select="$action"/>
                    <xsl:with-param name="action-arg0" select="$action-arg0"/>
                    <xsl:with-param name="level" select="number($level + 1)"/>
                    <xsl:with-param name="filtered-str" select="substring-after($tail, '&#333;')"/>
                </xsl:call-template>
            </xsl:when>
            <xsl:when test="$action='get-printf-format'">
                <xsl:value-of select="$filtered-str"/>
            </xsl:when>
        </xsl:choose>
    </xsl:template>


    <xsl:template name="fatal">
        <xsl:message terminate="yes">This rule must never get called, it must be overridden.&#10;</xsl:message>
    </xsl:template>
</xsl:stylesheet>
