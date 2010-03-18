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
                xmlns:xs="http://www.w3.org/2001/XMLSchema"
                xmlns:uig="foo://sun.me.ui-generator.net/">

    <!--
        Generate i18n stuff
    -->
    <xsl:template name="top-I18N">
        <xsl:call-template name="I18N-StringIds"/>
        <xsl:call-template name="I18N-StringTable"/>
        <xsl:call-template name="I18N-strings_en.properties"/>
    </xsl:template>


    <!--
        Generate StringIds class
    -->
    <xsl:template name="I18N-StringIds">
        <xsl:variable name="href" select="uig:classname-to-filepath('StringIds')"/>
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
        <xsl:for-each select="uig:get-all-format-string-elements(/)">
            <xsl:text>    public final static int </xsl:text>
            <xsl:value-of select="uig:I18N-key(.)"/>
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
        <xsl:variable name="href" select="uig:classname-to-filepath('StringTable')"/>
        <xsl:value-of select="concat($href,'&#10;')"/>
        <xsl:result-document href="{$href}">
            <xsl:call-template name="I18N-StringTable-impl"/>
        </xsl:result-document>
    </xsl:template>

    <xsl:template name="I18N-StringTable-impl">
        <xsl:text>package </xsl:text>
        <xsl:value-of select="$package-name"/>
        <xsl:text>;&#10;&#10;&#10;</xsl:text>
        <xsl:text>import com.sun.uig.Strings;&#10;&#10;&#10;</xsl:text>
        <xsl:text>final public class StringTable implements Strings {&#10;</xsl:text>
        <xsl:text>    private static final String strings[] = new String[] {&#10;</xsl:text>
        <xsl:for-each select="uig:get-all-format-string-elements(/)">
            <xsl:text>        "</xsl:text>
            <xsl:value-of select="uig:format-string-get-self(.)"/>
            <xsl:text>",&#10;</xsl:text>
        </xsl:for-each>
        <xsl:text>    };&#10;&#10;</xsl:text>
        <xsl:text>    public String getString(int id) {&#10;</xsl:text>
        <xsl:text>        return strings[id];&#10;</xsl:text>
        <xsl:text>    }&#10;</xsl:text>
        <xsl:text>}&#10;</xsl:text>
    </xsl:template>


    <!--
        Output i18n key
    -->
    <xsl:function name="uig:I18N-key" as="xs:string">
        <xsl:param name="e" as="element()"/>
        <xsl:apply-templates select="$e" mode="I18N-key-impl"/>
    </xsl:function>

    <xsl:template match="*" mode="I18N-key-impl">
        <xsl:variable name="screen-id" select="ancestor::screen/@name"/>
        <xsl:value-of select="concat(
            upper-case($screen-id),
            '_ID',
            count(preceding::*[ancestor::screen/@name=$screen-id and uig:is-format-string(.)]) + 1
            )"/>
    </xsl:template>


    <!--
        Generate strings_en.properties file.
    -->
    <xsl:template name="I18N-strings_en.properties">
        <xsl:variable name="href"
            select="concat($output-java-dir,'/strings_en.properties')"/>
        <xsl:value-of select="concat($href,'&#10;')"/>
        <xsl:result-document href="{$href}" encoding="utf8">
            <xsl:call-template name="I18N-strings_en.properties-impl"/>
        </xsl:result-document>
    </xsl:template>

    <xsl:template name="I18N-strings_en.properties-impl">
        <xsl:text>####
# Do not change the order of the lines!
# Do not remove empty lines!
# Do not split one line in two or more lines!
# If you put some comments be sure to put '#' in front of it!
# File encoding is UTF8!
####
# COUNT=</xsl:text>
<xsl:value-of select="count(uig:get-all-format-string-elements(/))"/>
<xsl:text>
####
# Main section
</xsl:text>
        <xsl:for-each select="uig:get-all-format-string-elements(/)">
            <xsl:value-of select="position() - 1"/>
            <xsl:text> = </xsl:text>
            <xsl:value-of select="uig:format-string-get-self(.)"/>
            <xsl:text>&#10;</xsl:text>
        </xsl:for-each>
        <xsl:text>####&#10;</xsl:text>
    </xsl:template>

</xsl:stylesheet>
