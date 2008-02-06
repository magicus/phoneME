<?xml version="1.0" ?>
<!--
Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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
        Output unit tests for screen classes
    -->
    <xsl:template name="top-UTest">
        <xsl:call-template name="TestClassList"/>
        <xsl:apply-templates select="//screen" mode="UTest-class"/>
    </xsl:template>


    <!--
        Output TestClassList class
    -->
    <xsl:template name="TestClassList">
        <xsl:variable name="href">
            <xsl:call-template name="classname2filepath">
                <xsl:with-param name="classname" select="'TestClassList'"/>
            </xsl:call-template>
        </xsl:variable>
        <xsl:result-document href="{$href}">
            <xsl:call-template name="TestClassList-impl"/>
        </xsl:result-document>
    </xsl:template>

    <xsl:template name="TestClassList-impl">
        <xsl:text>package </xsl:text>
        <xsl:value-of select="$package-name"/>
        <xsl:text>;&#10;&#10;&#10;</xsl:text>
        <xsl:text>final class TestClassList {&#10;</xsl:text>
        <xsl:text>    static final String names[] = new String[] {&#10;</xsl:text>
        <xsl:apply-templates select="//screen" mode="TestClassList-element"/>
        <xsl:text>    };&#10;</xsl:text>
        <xsl:text>}&#10;</xsl:text>
    </xsl:template>

    <xsl:template match="screen" mode="TestClassList-element">
        <xsl:text>        "</xsl:text>
        <xsl:value-of select="$package-name"/>
        <xsl:text>.</xsl:text>
        <xsl:apply-templates select="." mode="UTest-classname"/>
        <xsl:text>",&#10;</xsl:text>
    </xsl:template>


    <!--
        Output screen unit test class
    -->
    <xsl:template match="screen" mode="UTest-class">
        <xsl:variable name="href">
            <xsl:call-template name="classname2filepath">
                <xsl:with-param name="classname">
                    <xsl:apply-templates select="." mode="UTest-classname"/>
                </xsl:with-param>
            </xsl:call-template>
        </xsl:variable>
        <xsl:result-document href="{$href}">
            <xsl:apply-templates select="." mode="UTest-define"/>
        </xsl:result-document>
    </xsl:template>

    <xsl:template match="screen" mode="UTest-define">
        <xsl:text>package </xsl:text>
        <xsl:value-of select="$package-name"/>
        <xsl:text>;&#10;&#10;&#10;</xsl:text>
        <xsl:text>public final class </xsl:text>
        <xsl:apply-templates select="." mode="UTest-classname"/>
        <xsl:text> {&#10;</xsl:text>
        <xsl:text>    public </xsl:text>
        <xsl:apply-templates select="." mode="UTest-classname"/>
        <xsl:text>() {&#10;</xsl:text>
        <xsl:text>        final Screen s = new </xsl:text>
        <xsl:apply-templates select="." mode="Screen-classname"/>
        <xsl:text>(&#10;</xsl:text>
        <xsl:apply-templates select="." mode="UTest-screen-props"/>
        <xsl:apply-templates select="." mode="UTest-screen-command-listener"/>
        <xsl:text>);&#10;</xsl:text>
        <xsl:text>        s.show();&#10;</xsl:text>
        <xsl:text>    }&#10;</xsl:text>
        <xsl:text>}&#10;</xsl:text>
    </xsl:template>


    <xsl:template match="screen" mode="UTest-classname">
        <xsl:text>Test</xsl:text>
        <xsl:apply-templates select="." mode="Screen-classname"/>
    </xsl:template>


    <xsl:template match="screen" mode="UTest-screen-props">
        <xsl:text>            new ScreenProperties() {&#10;</xsl:text>
        <xsl:text>                public String get(String key) {&#10;</xsl:text>
        <xsl:text>                    if(false) return null;&#10;</xsl:text>
        <xsl:call-template name="uniq">
            <xsl:with-param name="str">
                <xsl:apply-templates select="descendant::text" mode="UTest-screen-props"/>
            </xsl:with-param>
        </xsl:call-template>
        <xsl:text>                    throw new RuntimeException("unexpected key: " + key);&#10;</xsl:text>
        <xsl:text>                }&#10;</xsl:text>
        <xsl:text>            }</xsl:text>
    </xsl:template>

    <xsl:template match="text" mode="UTest-screen-props">
        <xsl:call-template name="expand">
            <xsl:with-param name="str" select="."/>
            <xsl:with-param name="action" select="'utest-get-value'"/>
            <xsl:with-param name="action-arg0">
                <xsl:apply-templates select="ancestor::screen" mode="Screen-classname"/>
                <xsl:text>.</xsl:text>
            </xsl:with-param>
        </xsl:call-template>
    </xsl:template>


    <xsl:template match="screen[not(descendant::*/@id)]" mode="UTest-screen-command-listener"/>
    <xsl:template match="screen[descendant::*/@id]" mode="UTest-screen-command-listener">
        <xsl:text>,&#10;</xsl:text>
        <xsl:text>            new CommandListener() {&#10;</xsl:text>
        <xsl:text>                public void onCommand(Screen sender, int commandId) {&#10;</xsl:text>
        <xsl:text>                    switch(commandId) {&#10;</xsl:text>
        <xsl:apply-templates select="descendant::*[@id]" mode="UTest-screen-command-listener"/>
        <xsl:text>                    default:&#10;</xsl:text>
        <xsl:text>                        throw new RuntimeException("unexpected commandId: " + commandId);&#10;</xsl:text>
        <xsl:text>                    }&#10;</xsl:text>
        <xsl:text>                }&#10;</xsl:text>
        <xsl:text>            }</xsl:text>
    </xsl:template>

    <xsl:template match="*[@id]" mode="UTest-screen-command-listener">
        <xsl:text>                    case </xsl:text>
        <xsl:apply-templates select="ancestor::screen" mode="Screen-classname"/>
        <xsl:text>.</xsl:text>
        <xsl:apply-templates select="." mode="Screen-command-id"/>
        <xsl:text>:&#10;</xsl:text>
        <xsl:text>                        System.out.println("Command: </xsl:text>
        <xsl:apply-templates select="." mode="Screen-command-id"/>
        <xsl:text>");&#10;</xsl:text>
        <xsl:text>                        break;&#10;</xsl:text>
    </xsl:template>

</xsl:stylesheet>
