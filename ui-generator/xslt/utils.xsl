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
        Context independent utility stuff
    -->

    <xsl:param name="terminate-on-error">yes</xsl:param>

    <!--
        Converts class name in file path.
    -->
    <xsl:function name="uig:classname-to-filepath">
        <xsl:param name="classname" as="xs:string"/>
        <xsl:value-of select="concat($output-java-dir,'/',$classname,'.java')"/>
    </xsl:function>


    <!--
        Utility functions to handle format strings.
    -->
    <xsl:function name="uig:format-string-get-args" as="xs:string*">
        <xsl:param name="str" as="xs:string"/>
        <xsl:sequence select="uig:format-string-base($str, 'get-args')"/>
    </xsl:function>

    <xsl:function name="uig:format-string-get-self" as="xs:string">
        <xsl:param name="str" as="xs:string"/>
        <xsl:value-of>
            <xsl:for-each select="uig:format-string-base($str, 'get-self')">
                <xsl:value-of select="."/>
            </xsl:for-each>
        </xsl:value-of>
    </xsl:function>

    <xsl:function name="uig:format-string-base" as="xs:string*">
        <xsl:param name="str"       as="xs:string"/>
        <xsl:param name="action"    as="xs:string"/>
        <xsl:sequence select="
            uig:format-string-base-impl(
                $str,
                $action,
                number('0'),
                replace(
                    translate($str, '%', '&#333;'),
                    '&#333;&#333;',
                    '%%'))"/>
    </xsl:function>

    <xsl:function name="uig:format-string-base-impl" as="xs:string*">
        <xsl:param name="str"           as="xs:string"/>
        <xsl:param name="action"        as="xs:string"/>
        <xsl:param name="level"         as="xs:double"/>
        <xsl:param name="filtered-str"  as="xs:string"/>

        <xsl:variable name="tail" select="substring-after($filtered-str, '&#333;')"/>
        <xsl:choose>
            <xsl:when test="$tail">
                <xsl:variable name="argname" select="substring-before($tail, '&#333;')"/>
                <xsl:if test="not($argname)">
                    <xsl:message terminate="yes">
                        <xsl:text>Missing '%' character detected in: "</xsl:text>
                        <xsl:value-of select="$str"/>
                        <xsl:text>"&#10;</xsl:text>
                    </xsl:message>
                </xsl:if>
                <xsl:choose>
                    <xsl:when test="$action='get-args'">
                        <xsl:value-of select="$argname"/>
                    </xsl:when>
                    <xsl:when test="$action='get-self'">
                        <xsl:value-of>
                            <xsl:value-of select="substring-before($filtered-str, '&#333;')"/>
                            <xsl:text>%</xsl:text>
                            <xsl:value-of select="$level"/>
                        </xsl:value-of>
                    </xsl:when>
                </xsl:choose>
                <xsl:sequence select="
                    uig:format-string-base-impl(
                        $str,
                        $action,
                        number($level + 1),
                        substring-after($tail, '&#333;'))"/>
            </xsl:when>
            <xsl:when test="$action='get-self'">
                <xsl:value-of select="$filtered-str"/>
            </xsl:when>
        </xsl:choose>
    </xsl:function>


    <!--
        Returns sequence of all elements that may have format strings.
    -->
    <xsl:function name="uig:get-all-format-string-elements" as="element()*">
        <xsl:param name="ctx" as="document-node()"/>
        <xsl:sequence select="$ctx//(option|label|text|command|title)"/>
    </xsl:function>

    <!--
        Returns sequence of child elements of the given element that may have
        format strings.
    -->
    <xsl:function name="uig:get-format-string-elements" as="element()*">
        <xsl:param name="ctx" as="element()"/>
        <xsl:sequence select="$ctx//(option|label|text|command|title)"/>
    </xsl:function>

    <!--
        Returns 'true' if the given element may have format string.
    -->
    <xsl:function name="uig:is-format-string" as="xs:boolean">
        <xsl:param name="e" as="element()"/>
        <xsl:value-of select="
            count(uig:get-format-string-elements(
                $e/ancestor-or-self::screen)[generate-id(.) = generate-id($e)]) != 0"/>
    </xsl:function>


    <!--
        Increases indentation and strips trailing
        whitespace in the given multi-line string.
    -->
    <xsl:function name="uig:add-indentation" as="xs:string">
        <xsl:param name="text" as="xs:string"/>
        <xsl:param name="ident" as="xs:string"/>
        <xsl:value-of select="
                replace(
                    replace($text, '^', $ident, 'm'),
                    '^[ ]{1,};&#10;',
                    '',
                    'm')"/>
    </xsl:function>

    <xsl:template name="add-indentation">
        <xsl:param name="text" as="xs:string"/>
        <xsl:param name="ident" as="xs:string"/>
        <xsl:value-of select="uig:add-indentation($text, $ident)"/>
    </xsl:template>


    <!--
        Report error.
    -->
    <xsl:template name="error">
        <xsl:param name="msg"/>

        <xsl:message terminate="{$terminate-on-error}">
            <xsl:text>Error: </xsl:text>
            <xsl:value-of select="$msg"/>
            <xsl:text>; path=</xsl:text>
            <xsl:value-of select="saxon:path()"
                            xmlns:saxon="http://saxon.sf.net/"/>
        </xsl:message>
    </xsl:template>

    <!--
        Report that default template rule that should not be executed is
        triggered as there is no template rule that overwrites it.
    -->
    <xsl:template name="error-not-implemented">
        <xsl:call-template name="error">
            <xsl:with-param name="msg">Template rule not implemented</xsl:with-param>
        </xsl:call-template>
    </xsl:template>

    <!--
        Report that template rule that should never be executed is triggered.
    -->
    <xsl:template name="error-unexpected">
        <xsl:call-template name="error">
            <xsl:with-param name="msg">Template rule should never get called</xsl:with-param>
        </xsl:call-template>
    </xsl:template>
</xsl:stylesheet>
