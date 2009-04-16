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

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

    <xsl:template match="screen" mode="Screen-imports">
        <xsl:text>import java.util.Enumeration;&#10;</xsl:text>
        <xsl:text>import java.util.Vector;&#10;</xsl:text>
        <xsl:text>import javax.microedition.lcdui.Displayable;&#10;</xsl:text>
        <xsl:text>import javax.microedition.lcdui.ChoiceGroup;&#10;</xsl:text>
        <xsl:text>import javax.microedition.lcdui.Choice;&#10;</xsl:text>
        <xsl:text>import javax.microedition.lcdui.StringItem;&#10;</xsl:text>
        <xsl:text>import javax.microedition.lcdui.Gauge;&#10;</xsl:text>
        <xsl:text>import javax.microedition.lcdui.Spacer;&#10;</xsl:text>
        <xsl:text>import javax.microedition.lcdui.Item;&#10;</xsl:text>
        <xsl:text>import javax.microedition.lcdui.ItemCommandListener;&#10;</xsl:text>
        <xsl:text>import javax.microedition.lcdui.Command;&#10;</xsl:text>
        <xsl:text>import javax.microedition.lcdui.Form;&#10;</xsl:text>
        <xsl:text>import javax.microedition.lcdui.List;&#10;</xsl:text>
        <xsl:text>&#10;&#10;</xsl:text>
    </xsl:template>

    <xsl:template match="screen" mode="Screen-complete-define">
        <xsl:text>    protected Displayable createDisplayable() {&#10;</xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-create-displayable"/>
        <xsl:text>&#10;</xsl:text>
        <xsl:if test="descendant::dynamic-option|descendant::dynamic-command">
            <xsl:text>        int counter = 0;&#10;&#10;</xsl:text>
        </xsl:if>
        <xsl:if test="descendant::dynamic-command">
            <xsl:text>        final Vector commands = new Vector();&#10;&#10;</xsl:text>
        </xsl:if>
        <xsl:apply-templates select="text|filler|options|progress|command|dynamic-command" mode="LCDUI-append"/>
        <xsl:apply-templates select="." mode="LCDUI-set-command-listener"/>
        <xsl:text>        return d;&#10;</xsl:text>
        <xsl:text>    }&#10;</xsl:text>
    </xsl:template>


    <xsl:template match="screen[not(options/@style='fullscreen')]" mode="LCDUI-create-displayable">
        <xsl:text>        Form d = createForm();&#10;</xsl:text>
    </xsl:template>

    <xsl:template match="screen[options/@style='fullscreen']" mode="LCDUI-create-displayable">
        <xsl:text>        List d = createList();&#10;</xsl:text>
    </xsl:template>


    <xsl:template match="screen" mode="LCDUI-set-command-listener">
        <xsl:if test="options">
            <xsl:text>        final Command selectItemCommand = getSelectItemCommand();&#10;</xsl:text>
        </xsl:if>
        <xsl:if test="options[not(@style='fullscreen')]">
            <xsl:text>        ItemCommandListener icl = new ItemCommandListener() {&#10;</xsl:text>
            <xsl:text>            public void commandAction(Command c, Item item) {&#10;</xsl:text>
            <xsl:apply-templates select="options" mode="LCDUI-map-command"/>
            <xsl:text>&#10;</xsl:text>
            <xsl:text>            }&#10;</xsl:text>
            <xsl:text>        };&#10;</xsl:text>
            <xsl:apply-templates select="options" mode="LCDUI-set-item-command-listener"/>
            <xsl:text>&#10;</xsl:text>
        </xsl:if>
        <xsl:if test="options[@style='fullscreen']">
            <xsl:text>        d.setSelectCommand(selectItemCommand);&#10;</xsl:text>
        </xsl:if>
        <xsl:if test="command|dynamic-command|options[@style='fullscreen']">
            <xsl:text>        javax.microedition.lcdui.CommandListener cl = new javax.microedition.lcdui.CommandListener() {&#10;</xsl:text>
            <xsl:text>            public void commandAction(Command item, Displayable d) {&#10;</xsl:text>
            <xsl:if test="dynamic-command">
                <xsl:text>                int idx;&#10;</xsl:text>
            </xsl:if>
            <xsl:apply-templates select="command|dynamic-command|options[@style='fullscreen']" mode="LCDUI-map-command"/>
            <xsl:text>&#10;</xsl:text>
            <xsl:text>            }&#10;</xsl:text>
            <xsl:text>        };&#10;</xsl:text>
            <xsl:text>        d.setCommandListener(cl);&#10;&#10;</xsl:text>
        </xsl:if>
    </xsl:template>

    <xsl:template match="*" mode="LCDUI-set-item-command-listener">
        <xsl:text>        </xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-varname"/>
        <xsl:text>.setItemCommandListener(icl);&#10;</xsl:text>
        <xsl:text>        </xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-varname"/>
        <xsl:text>.addCommand(selectItemCommand);&#10;</xsl:text>
    </xsl:template>

    <xsl:template match="*" mode="LCDUI-map-command">
        <xsl:if test="position() = 1">
            <xsl:text>                if (false) {&#10;</xsl:text>
            <xsl:text>                } </xsl:text>
        </xsl:if>
        <xsl:text>else if (</xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-map-command-test-command"/>
        <xsl:text>) {&#10;</xsl:text>
        <xsl:text>                    int commandId = -1;&#10;</xsl:text>
        <xsl:text>                    int commandIdIdx = -1;&#10;</xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-map-item-to-command-id"/>
        <xsl:text>                    if (commandIdIdx == -1)&#10;</xsl:text>
        <xsl:text>                        listener.onCommand(</xsl:text>
        <xsl:apply-templates select="ancestor::screen" mode="Screen-classname"/>
        <xsl:text>.this, commandId);&#10;</xsl:text>
        <xsl:text>                    else&#10;</xsl:text>
        <xsl:text>                        listener.onDynamicCommand(</xsl:text>
        <xsl:apply-templates select="ancestor::screen" mode="Screen-classname"/>
        <xsl:text>.this, commandId, commandIdIdx);&#10;</xsl:text>
        <xsl:text>                } </xsl:text>
    </xsl:template>

    <xsl:template match="*" mode="LCDUI-map-item-to-command-id">
        <xsl:text>                    commandId = </xsl:text>
        <xsl:apply-templates select="." mode="Screen-command-id"/>
        <xsl:text>;&#10;</xsl:text>
    </xsl:template>

    <xsl:template match="*" mode="LCDUI-map-command-test-command">
        <xsl:text>item == </xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-varname"/>
    </xsl:template>


    <xsl:template match="*" mode="LCDUI-append">
        <xsl:variable name="varname">
            <xsl:apply-templates select="." mode="LCDUI-varname"/>
        </xsl:variable>
        <xsl:variable name="layout">
            <xsl:apply-templates select="." mode="LCDUI-layout"/>
        </xsl:variable>
        <xsl:apply-templates select="." mode="LCDUI-create"/>
        <xsl:value-of select="concat('        ',$varname,'.setLayout(Item.LAYOUT_2 | ',$layout,');&#10;')"/>
        <xsl:value-of select="concat('        d.append(',$varname,');&#10;')"/>
        <xsl:text>&#10;</xsl:text>
    </xsl:template>

    <xsl:template match="options/dynamic-option|dynamic-command" mode="LCDUI-append">
        <xsl:text>        counter = </xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-baseidx"/>
        <xsl:text>;&#10;</xsl:text>
        <xsl:text>        for (Enumeration e = (Enumeration)getProperty("</xsl:text>
        <xsl:apply-templates select="." mode="Screen-command-id"/>
        <xsl:text>") ; e.hasMoreElements(); ++counter) {&#10;</xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-dynamic-item-create"/>
        <xsl:text>        }&#10;</xsl:text>
        <xsl:text>        final int </xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-range-value"/>
        <xsl:text> = counter;&#10;</xsl:text>
    </xsl:template>

    <xsl:template match="*[@align='center']" mode="LCDUI-layout">
        <xsl:text>Item.LAYOUT_NEWLINE_BEFORE | Item.LAYOUT_CENTER</xsl:text>
    </xsl:template>
    <xsl:template match="*[@align='left' or not(@align)]" mode="LCDUI-layout">
        <xsl:text>Item.LAYOUT_NEWLINE_BEFORE | Item.LAYOUT_LEFT</xsl:text>
    </xsl:template>
    <xsl:template match="*[@align='right']" mode="LCDUI-layout">
        <xsl:text>Item.LAYOUT_NEWLINE_BEFORE | Item.LAYOUT_RIGHT</xsl:text>
    </xsl:template>


    <xsl:template match="screen[progress]" mode="Screen-define-extra-members">
        <xsl:apply-imports/>
        <xsl:apply-templates select="progress" mode="LCDUI-define-progress"/>
        <xsl:text>&#10;</xsl:text>
    </xsl:template>


    <!--
        element's label if any
    -->
    <xsl:template match="*[label/text]" mode="LCDUI-get-label">
        <xsl:apply-templates select="label/text" mode="Screen-printf"/>
    </xsl:template>
    <xsl:template match="*[not(label/text)]" mode="LCDUI-get-label">
        <xsl:text>null</xsl:text>
    </xsl:template>


    <!--
        Top level "text" element
    -->
    <xsl:template match="screen/text" mode="LCDUI-create">
        <xsl:text>        final StringItem </xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-varname"/>
        <xsl:text> = new StringItem(null, </xsl:text>
        <xsl:apply-templates select="." mode="Screen-printf"/>
        <xsl:text>, Item.PLAIN);&#10;</xsl:text>
    </xsl:template>


    <!--
        Top level "filler" element
    -->
    <xsl:template match="screen/filler" mode="LCDUI-create">
        <xsl:text>        final Spacer </xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-varname"/>
        <xsl:text> = new Spacer(1, 0);&#10;</xsl:text>
    </xsl:template>


    <!--
        Top level "options" element
    -->
    <xsl:template match="screen/options[@style='fullscreen']" mode="LCDUI-append">
        <xsl:text>        final Choice </xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-varname"/>
        <xsl:text> = d;&#10;</xsl:text>
        <xsl:apply-templates select="option/text|dynamic-option" mode="LCDUI-append"/>
    </xsl:template>

    <xsl:template match="screen/options" mode="LCDUI-create">
        <xsl:text>        final ChoiceGroup </xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-varname"/>
        <xsl:text> = new ChoiceGroup(</xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-get-label"/>
        <xsl:text>, ChoiceGroup.</xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-options-type"/>
        <xsl:text>);&#10;</xsl:text>
        <xsl:apply-templates select="option/text|dynamic-option" mode="LCDUI-append"/>
    </xsl:template>

    <xsl:template match="*[@style='dropdown']" mode="LCDUI-options-type">
        <xsl:text>POPUP</xsl:text>
    </xsl:template>
    <xsl:template match="*[@style='plain'or not(@style)]" mode="LCDUI-options-type">
        <xsl:text>EXCLUSIVE</xsl:text>
    </xsl:template>

    <xsl:template match="option/text" mode="LCDUI-append">
        <xsl:text>        </xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-varname"/>
        <xsl:text>.append(</xsl:text>
        <xsl:apply-templates select="." mode="Screen-printf"/>
        <xsl:text>, null);&#10;</xsl:text>
    </xsl:template>

    <xsl:template match="screen/options[@style='fullscreen']" mode="LCDUI-options-get-self">
        <xsl:text>d</xsl:text>
    </xsl:template>
    <xsl:template match="screen/options[not(@style='fullscreen')]" mode="LCDUI-options-get-self">
        <xsl:text>item</xsl:text>
    </xsl:template>

    <xsl:template match="screen/options" mode="LCDUI-map-item-to-command-id">
        <xsl:text>                    final int idx = ((Choice)</xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-options-get-self"/>
        <xsl:text>).getSelectedIndex();&#10;</xsl:text>
        <xsl:text>                    if (false) {&#10;</xsl:text>
        <xsl:text>                    }</xsl:text>
        <xsl:apply-templates select="option|dynamic-option" mode="LCDUI-map-item-to-command-id"/>
        <xsl:text>&#10;</xsl:text>
    </xsl:template>

    <xsl:template match="screen/options[@style='fullscreen']" mode="LCDUI-map-command-test-command">
        <xsl:text>item == selectItemCommand</xsl:text>
    </xsl:template>

    <xsl:template match="option" mode="LCDUI-map-item-to-command-id">
        <xsl:text> else if (</xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-baseidx"/>
        <xsl:text> == idx) {&#10;</xsl:text>
        <xsl:text>                        commandId = </xsl:text>
        <xsl:apply-templates select="." mode="Screen-command-id"/>
        <xsl:text>;&#10;</xsl:text>
        <xsl:text>                    }</xsl:text>
    </xsl:template>

    <xsl:template match="dynamic-option" mode="LCDUI-dynamic-item-create">
        <xsl:text>            </xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-varname"/>
        <xsl:text>.append(e.nextElement().toString(), null);&#10;</xsl:text>
    </xsl:template>

    <xsl:template match="dynamic-option" mode="LCDUI-map-item-to-command-id">
        <xsl:text> else if (</xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-baseidx"/>
        <xsl:text><![CDATA[ <= idx && idx < ]]></xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-range-value"/>
        <xsl:text>) {&#10;</xsl:text>
        <xsl:text>                        commandId = </xsl:text>
        <xsl:apply-templates select="." mode="Screen-command-id"/>
        <xsl:text>;&#10;</xsl:text>
        <xsl:text>                        commandIdIdx = idx - (</xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-baseidx"/>
        <xsl:text>);&#10;</xsl:text>
        <xsl:text>                    }</xsl:text>
    </xsl:template>


    <!--
        Top level "progress" element
    -->
    <xsl:template match="screen/progress" mode="LCDUI-create">
        <xsl:text>        </xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-varname"/>
        <xsl:text> = new Gauge(</xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-get-label"/>
        <xsl:text>, false, 100, 0);&#10;</xsl:text>
    </xsl:template>

    <xsl:template match="screen/progress" mode="LCDUI-define-progress">
        <xsl:text>    private Gauge </xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-varname"/>
        <xsl:text>;&#10;</xsl:text>
    </xsl:template>

    <xsl:template match="screen/progress" mode="Screen-progress-define">
        <xsl:variable name="varname">
            <xsl:apply-templates select="." mode="LCDUI-varname"/>
        </xsl:variable>
        <xsl:variable name="id">
            <xsl:apply-templates select="." mode="Screen-progress-id"/>
        </xsl:variable>
        <xsl:value-of select="concat('        if (progressId.equals(',$id,')) {&#10;')"/>
        <xsl:value-of select="concat('            ',$varname,'.setMaxValue(max);&#10;')"/>
        <xsl:value-of select="concat('            ',$varname,'.setValue(value);&#10;')"/>
        <xsl:text>            return;&#10;</xsl:text>
        <xsl:text>        }&#10;</xsl:text>
    </xsl:template>


    <!--
        Top level "command", "dynamic-command" elements.
    -->
    <xsl:template match="command|dynamic-command" mode="LCDUI-command-append-impl">
        <xsl:apply-templates select="." mode="LCDUI-command-ident"/>
        <xsl:text>final Command </xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-varname"/>
        <xsl:text> = new Command(</xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-command-text"/>
        <xsl:text>, </xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-command-type"/>
        <xsl:text>, 1);&#10;</xsl:text>
        <xsl:if test="../dynamic-command">
            <xsl:apply-templates select="." mode="LCDUI-command-ident"/>
            <xsl:text>commands.addElement(</xsl:text>
            <xsl:apply-templates select="." mode="LCDUI-varname"/>
            <xsl:text>);&#10;</xsl:text>
        </xsl:if>
        <xsl:apply-templates select="." mode="LCDUI-command-ident"/>
        <xsl:text>d.addCommand(</xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-varname"/>
        <xsl:text>);&#10;</xsl:text>
    </xsl:template>

    <xsl:template match="command|dynamic-command" mode="LCDUI-command-type">
        <xsl:choose>
            <xsl:when test="@id='OK' or @id='YES'">
                <xsl:text>Command.OK</xsl:text>
            </xsl:when>
            <xsl:when test="@id='CANCEL' or @id='NO'">
                <xsl:text>Command.CANCEL</xsl:text>
            </xsl:when>
            <xsl:otherwise>
                <xsl:text>Command.SCREEN</xsl:text>
            </xsl:otherwise>
        </xsl:choose>
    </xsl:template>


    <!--
        Top level "command" element
    -->
    <xsl:template match="screen/command" mode="LCDUI-append">
        <xsl:apply-templates select="." mode="LCDUI-command-append-impl"/>
        <xsl:text>&#10;</xsl:text>
    </xsl:template>

    <xsl:template match="command" mode="LCDUI-command-text">
        <xsl:apply-templates select="text" mode="Screen-printf"/>
    </xsl:template>

    <xsl:template match="command" mode="LCDUI-command-ident">
        <xsl:text>        </xsl:text>
    </xsl:template>


    <!--
        Top level "dynamic-command" element
    -->
    <xsl:template match="screen/dynamic-command" mode="LCDUI-dynamic-item-create">
        <xsl:apply-templates select="." mode="LCDUI-command-append-impl"/>
    </xsl:template>

    <xsl:template match="screen/dynamic-command" mode="LCDUI-map-command-test-command">
        <xsl:text>(idx = commands.indexOf(item, </xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-baseidx"/>
        <xsl:text><![CDATA[)) >= 0 && idx < ]]></xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-range-value"/>
    </xsl:template>

    <xsl:template match="screen/dynamic-command" mode="LCDUI-map-item-to-command-id">
        <xsl:text>                    commandId = </xsl:text>
        <xsl:apply-templates select="." mode="Screen-command-id"/>
        <xsl:text>;&#10;</xsl:text>
        <xsl:text>                    commandIdIdx = idx - (</xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-baseidx"/>
        <xsl:text>);&#10;</xsl:text>
    </xsl:template>


    <xsl:template match="dynamic-command" mode="LCDUI-command-text">
        <xsl:text>e.nextElement().toString()</xsl:text>
    </xsl:template>

    <xsl:template match="dynamic-command" mode="LCDUI-command-ident">
        <xsl:text>            </xsl:text>
    </xsl:template>


    <!--
        Utils
    -->
    <xsl:template match="dynamic-option|dynamic-command" mode="LCDUI-range-value">
        <xsl:value-of select="concat('range_',generate-id())"/>
    </xsl:template>

    <xsl:template match="option|dynamic-option|dynamic-command" mode="LCDUI-baseidx">
        <xsl:choose>
            <xsl:when test="preceding-sibling::*[self::dynamic-option or self::dynamic-command]">
                <xsl:apply-templates select="preceding-sibling::*[self::dynamic-option or self::dynamic-command][1]" mode="LCDUI-range-value"/>
                <xsl:text> + </xsl:text>
                <xsl:value-of select="
                    count(preceding-sibling::*[self::option or self::command])
                    - count(preceding-sibling::*[self::dynamic-option or self::dynamic-command]/preceding-sibling::*[self::option or self::command])"/>
            </xsl:when>
            <xsl:otherwise>
                <xsl:value-of select="count(preceding-sibling::*[self::dynamic-option or self::dynamic-command or self::option or self::command])"/>
            </xsl:otherwise>
        </xsl:choose>
    </xsl:template>


    <xsl:template match="screen/text|screen/filler|screen/options|screen/progress|screen/options/label|screen/progress/label|screen/command" mode="LCDUI-varname">
        <xsl:value-of select="concat('item_',generate-id())"/>
    </xsl:template>

    <xsl:template match="screen/dynamic-command" mode="LCDUI-varname">
        <xsl:text>t</xsl:text>
    </xsl:template>

    <xsl:template match="*" mode="LCDUI-varname">
        <xsl:apply-templates select=".." mode="LCDUI-varname"/>
    </xsl:template>
</xsl:stylesheet>
