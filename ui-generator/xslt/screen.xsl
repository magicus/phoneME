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
        Output all screen classes
    -->
    <xsl:template name="top-Screen">
        <xsl:apply-templates select="//screen" mode="Screen-class"/>
    </xsl:template>


    <!--
        Output screen class
    -->
    <xsl:template match="screen" mode="Screen-class">
        <xsl:variable name="href">
            <xsl:call-template name="classname2filepath">
                <xsl:with-param name="classname">
                    <xsl:apply-templates select="." mode="Screen-classname"/>
                </xsl:with-param>
            </xsl:call-template>
        </xsl:variable>
        <xsl:value-of select="concat($href,'&#10;')"/>
        <xsl:result-document href="{$href}">
            <xsl:apply-templates select="." mode="Screen-define"/>
        </xsl:result-document>
    </xsl:template>

    <xsl:template match="screen" mode="Screen-define">
        <xsl:text>package </xsl:text>
        <xsl:value-of select="$package-name"/>
        <xsl:text>;&#10;&#10;&#10;</xsl:text>
        <xsl:apply-templates select="." mode="Screen-imports"/>
        <xsl:text>public final class </xsl:text>
        <xsl:apply-templates select="." mode="Screen-classname"/>
        <xsl:text> extends Screen </xsl:text>
        <xsl:apply-templates select="." mode="Screen-implements" />
        <xsl:text> {&#10;</xsl:text>
        <xsl:apply-templates select="." mode="Screen-define-constants"/>
        <xsl:apply-templates select="." mode="Screen-define-extra-members"/>
        <xsl:text>    public </xsl:text>
        <xsl:apply-templates select="." mode="Screen-classname"/>
        <xsl:text>(ScreenProperties props</xsl:text>
        <xsl:apply-templates select="." mode="Screen-extra-ctor-args"/>
        <xsl:text>) {&#10;        super(props);&#10;</xsl:text>
        <xsl:apply-templates select="." mode="Screen-init-extra-members"/>
        <xsl:text>    }&#10;&#10;</xsl:text>
        <xsl:apply-templates select="." mode="Screen-progress-define"/>
        <xsl:apply-templates select="." mode="Screen-complete-define"/>
        <xsl:text>}&#10;</xsl:text>
    </xsl:template>

    <xsl:template match="screen" mode="Screen-imports"/>


    <!--
        Output screen class name
    -->
    <xsl:template match="screen" mode="Screen-classname">
        <xsl:value-of select="@name"/>
    </xsl:template>


    <!--
        Output interfaces
    -->
    <xsl:template match="screen[not(progress)]" mode="Screen-implements" />
    <xsl:template match="screen[progress]" mode="Screen-implements">
        <xsl:text>implements ProgressUpdater</xsl:text>
    </xsl:template>


    <!--
        Additional stuff for screen classes with CommandListener
    -->
    <xsl:template match="screen[not(descendant::*/@id)]" mode="Screen-define-extra-members"/>
    <xsl:template match="screen[descendant::*/@id]" mode="Screen-define-extra-members">
        <xsl:text>    private CommandListener listener;&#10;&#10;</xsl:text>
    </xsl:template>


    <xsl:template match="screen[not(descendant::*/@id)]" mode="Screen-extra-ctor-args"/>
    <xsl:template match="screen[descendant::*/@id]" mode="Screen-extra-ctor-args">
        <xsl:text>, CommandListener listener</xsl:text>
    </xsl:template>


    <xsl:template match="screen[not(descendant::*/@id)]" mode="Screen-init-extra-members"/>
    <xsl:template match="screen[descendant::*/@id]" mode="Screen-init-extra-members">
        <xsl:text>        this.listener = listener;&#10;</xsl:text>
    </xsl:template>


    <!--
        Additional stuff for screen with progress item(s).
    -->
    <xsl:template match="screen[not(progress)]" mode="Screen-progress-define" />
    <xsl:template match="screen[progress]" mode="Screen-progress-define">
        <xsl:text>&#10;</xsl:text>
        <xsl:text>    public void updateProgress(Object progressId, int value, int max) {&#10;</xsl:text>
        <xsl:apply-templates select="progress" mode="Screen-progress-define" />
        <xsl:text>        throw new RuntimeException(progressId + " not found");&#10;</xsl:text>
        <xsl:text>    }&#10;</xsl:text>
    </xsl:template>


    <!--
        Toolkit specific screen initialization.
    -->
    <xsl:template match="screen" mode="Screen-complete-define">
        <xsl:call-template name="fatal"/>
    </xsl:template>

    <xsl:template match="screen/progress" mode="Screen-progress-define">
        <xsl:call-template name="fatal"/>
    </xsl:template>


    <!--
        Output class constants
    -->
    <xsl:template match="screen" mode="Screen-define-constants">
        <xsl:if test="descendant::text">
            <xsl:call-template name="uniq">
                <xsl:with-param name="str">
                    <xsl:apply-templates select="descendant::text" mode="Screen-define-prop-keys"/>
                </xsl:with-param>
            </xsl:call-template>
            <xsl:text>&#10;</xsl:text>
        </xsl:if>
        <xsl:if test="descendant::*[@id|@pid]">
            <xsl:apply-templates select="descendant::*[@id|@pid]" mode="Screen-define-ids"/>
            <xsl:text>&#10;</xsl:text>
        </xsl:if>
    </xsl:template>

    <xsl:template match="text" mode="Screen-define-prop-keys">
        <xsl:call-template name="expand">
            <xsl:with-param name="str" select="."/>
            <xsl:with-param name="action" select="'define-argkey'"/>
        </xsl:call-template>
    </xsl:template>

    <xsl:template match="*[@id]" mode="Screen-define-ids">
        <xsl:text>    public final static int </xsl:text>
        <xsl:apply-templates select="." mode="Screen-command-id"/>
        <xsl:text> = </xsl:text>
        <xsl:value-of select="count(preceding::*[@id]) + 1"/>
        <xsl:text>;&#10;</xsl:text>
    </xsl:template>

    <xsl:template match="progress" mode="Screen-define-ids">
        <xsl:text>    public final static Object </xsl:text>
        <xsl:apply-templates select="." mode="Screen-progress-id"/>
        <xsl:text> = "</xsl:text>
        <xsl:value-of select="@pid"/>
        <xsl:text>";&#10;</xsl:text>
    </xsl:template>

    <xsl:template match="*" mode="Screen-command-id">
        <xsl:text>COMMAND_ID_</xsl:text>
        <xsl:call-template name="toupper">
            <xsl:with-param name="str" select="@id"/>
        </xsl:call-template>
    </xsl:template>

    <xsl:template match="*" mode="Screen-progress-id">
        <xsl:text>PROGRESS_ID_</xsl:text>
        <xsl:call-template name="toupper">
            <xsl:with-param name="str" select="@pid"/>
        </xsl:call-template>
    </xsl:template>


    <!--
        Output java code to create string to display
        at run-time from "text" element body
    -->
    <xsl:template match="text" mode="Screen-printf">
        <xsl:text>printf(</xsl:text>
        <xsl:apply-templates select="." mode="I18N-key"/>
        <xsl:text>, new Object[] { </xsl:text>
        <xsl:call-template name="expand">
            <xsl:with-param name="str" select="."/>
            <xsl:with-param name="action" select="'get-printf-args'"/>
        </xsl:call-template>
        <xsl:text>})</xsl:text>
    </xsl:template>

</xsl:stylesheet>
