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

<xsl:stylesheet version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

    <!--
        Generate i18n stuff
    -->
    <xsl:template name="top-I18N">
        <xsl:call-template name="I18N-StringIds"/>
        <xsl:call-template name="I18N-StringTable"/>
    </xsl:template>


    <!--
        Generate StringIds class
    -->
    <xsl:template name="I18N-StringIds">
        <xsl:variable name="href">
            <xsl:call-template name="classname2filepath">
                <xsl:with-param name="classname" select="'StringIds'"/>
            </xsl:call-template>
        </xsl:variable>
        <xsl:value-of select="concat($href,'&#10;')"/>
        <xsl:result-document href="{$href}">
            <xsl:call-template name="I18N-StringIds-impl"/>
        </xsl:result-document>
    </xsl:template>

    <xsl:template name="I18N-StringIds-impl">
        <xsl:text>package </xsl:text>
        <xsl:value-of select="$package-name"/>
        <xsl:text>;&#10;&#10;&#10;</xsl:text>
        <xsl:text>interface StringIds {&#10;</xsl:text>
        <xsl:for-each select="//text">
            <xsl:text>    public final static int </xsl:text>
            <xsl:apply-templates select="." mode="I18N-key"/>
            <xsl:text> = </xsl:text>
            <xsl:value-of select="position() - 1"/>
            <xsl:text>;&#10;</xsl:text>
        </xsl:for-each>
        <xsl:text>}&#10;</xsl:text>
    </xsl:template>


    <!--
        Generate StringTable class
    -->
    <xsl:template name="I18N-StringTable">
        <xsl:variable name="href">
            <xsl:call-template name="classname2filepath">
                <xsl:with-param name="classname" select="'StringTable'"/>
            </xsl:call-template>
        </xsl:variable>
        <xsl:value-of select="concat($href,'&#10;')"/>
        <xsl:result-document href="{$href}">
            <xsl:call-template name="I18N-StringTable-impl"/>
        </xsl:result-document>
    </xsl:template>

    <xsl:template name="I18N-StringTable-impl">
        <xsl:text>package </xsl:text>
        <xsl:value-of select="$package-name"/>
        <xsl:text>;&#10;&#10;&#10;</xsl:text>
        <xsl:text>final class StringTable {&#10;</xsl:text>
        <xsl:text>    private static final String strings[] = new String[] {&#10;</xsl:text>
        <xsl:apply-templates select="//screen" mode="I18N-StringTable-element"/>
        <xsl:text>    };&#10;&#10;</xsl:text>
        <xsl:text>    static String getString(int idx) {&#10;</xsl:text>
        <xsl:text>        return strings[idx];&#10;</xsl:text>
        <xsl:text>    }&#10;</xsl:text>
        <xsl:text>}&#10;</xsl:text>
    </xsl:template>

    <xsl:template match="screen" mode="I18N-StringTable-element">
        <xsl:apply-templates select="descendant::text" mode="I18N-StringTable-element"/>
    </xsl:template>

    <xsl:template match="text" mode="I18N-StringTable-element">
        <xsl:text>        </xsl:text>
        <xsl:apply-templates select="." mode="I18N-value"/>
        <xsl:text>,&#10;</xsl:text>
    </xsl:template>


    <!--
        Output i18n key
    -->
    <xsl:template match="text" mode="I18N-key">
        <xsl:variable name="screen-id" select="ancestor::screen/@name"/>
        <xsl:call-template name="toupper">
            <xsl:with-param name="str" select="$screen-id"/>
        </xsl:call-template>
        <xsl:text>_ID</xsl:text>
        <xsl:value-of select="count(preceding::text[ancestor::screen/@name=$screen-id]) + 1"/>
    </xsl:template>


    <!--
        Output default i18n value
    -->
    <xsl:template match="text" mode="I18N-value">
        <xsl:call-template name="expand">
            <xsl:with-param name="str" select="."/>
            <xsl:with-param name="action" select="'get-printf-format'"/>
        </xsl:call-template>
    </xsl:template>

</xsl:stylesheet>
