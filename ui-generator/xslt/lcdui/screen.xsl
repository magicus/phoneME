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

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

    <xsl:template match="screen" mode="Screen-imports">
        <xsl:text>import javax.microedition.lcdui.Displayable;&#10;</xsl:text>
        <xsl:text>import javax.microedition.lcdui.ChoiceGroup;&#10;</xsl:text>
        <xsl:text>import javax.microedition.lcdui.StringItem;&#10;</xsl:text>
        <xsl:text>import javax.microedition.lcdui.Spacer;&#10;</xsl:text>
        <xsl:text>import javax.microedition.lcdui.Item;&#10;</xsl:text>
        <xsl:text>import javax.microedition.lcdui.ItemCommandListener;&#10;</xsl:text>
        <xsl:text>import javax.microedition.lcdui.Command;&#10;</xsl:text>
        <xsl:text>import javax.microedition.lcdui.Form;&#10;</xsl:text>
        <xsl:text>&#10;&#10;</xsl:text>
    </xsl:template>

    <xsl:template match="screen" mode="Screen-complete-define">
        <xsl:text>    protected Displayable getDisplayable() {&#10;</xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-create-displayable"/>
        <xsl:text>&#10;</xsl:text>
        <xsl:apply-templates select="text|filler|options" mode="LCDUI-append"/>
        <xsl:apply-templates select="." mode="LCDUI-set-command-listener"/>
        <xsl:text>        return d;&#10;</xsl:text>
        <xsl:text>    }&#10;</xsl:text>
    </xsl:template>


    <xsl:template match="screen" mode="LCDUI-create-displayable">
        <xsl:text>        Form d = createForm();&#10;</xsl:text>
    </xsl:template>


    <xsl:template match="screen[not(descendant::*/@id)]" mode="LCDUI-set-command-listener"/>
    <xsl:template match="screen[descendant::*/@id]" mode="LCDUI-set-command-listener">
        <xsl:text>        Command selectItemCommand = getSelectItemCommand();&#10;</xsl:text>
        <xsl:text>        ItemCommandListener l = new ItemCommandListener() {&#10;</xsl:text>
        <xsl:text>            public void commandAction(Command c, Item item) {&#10;</xsl:text>
        <xsl:apply-templates select="options" mode="LCDUI-map-command"/>
        <xsl:text>&#10;</xsl:text>
        <xsl:text>            }&#10;</xsl:text>
        <xsl:text>        };&#10;</xsl:text>
        <xsl:apply-templates select="options" mode="LCDUI-set-command-listener"/>
        <xsl:text>&#10;</xsl:text>
    </xsl:template>

    <xsl:template match="*" mode="LCDUI-set-command-listener">
        <xsl:text>        </xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-varname"/>
        <xsl:text>.setItemCommandListener(l);&#10;</xsl:text>
        <xsl:text>        </xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-varname"/>
        <xsl:text>.addCommand(selectItemCommand);&#10;</xsl:text>
    </xsl:template>

    <xsl:template match="*" mode="LCDUI-map-command">
        <xsl:text>                </xsl:text>
        <xsl:if test="position() != 1">
            <xsl:text>else </xsl:text>
        </xsl:if>
        <xsl:text>if (item == </xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-varname"/>
        <xsl:text>) {&#10;</xsl:text>
        <xsl:text>                    int commandId = -1;&#10;</xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-map-item-to-command-id"/>
        <xsl:text>                    listener.onCommand(</xsl:text>
        <xsl:apply-templates select="ancestor::screen" mode="Screen-classname"/>
        <xsl:text>.this, commandId);&#10;                } </xsl:text>
    </xsl:template>

    <xsl:template match="*" mode="LCDUI-map-item-to-command-id">
        <xsl:text>                    commandId = </xsl:text>
        <xsl:apply-templates select="." mode="Screen-command-id"/>
        <xsl:text>;&#10;</xsl:text>
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

    <xsl:template match="*[@align='center']" mode="LCDUI-layout">
        <xsl:text>Item.LAYOUT_NEWLINE_BEFORE | Item.LAYOUT_CENTER</xsl:text>
    </xsl:template>
    <xsl:template match="*[@align='left' or not(@align)]" mode="LCDUI-layout">
        <xsl:text>Item.LAYOUT_NEWLINE_BEFORE | Item.LAYOUT_LEFT</xsl:text>
    </xsl:template>
    <xsl:template match="*[@align='right']" mode="LCDUI-layout">
        <xsl:text>Item.LAYOUT_NEWLINE_BEFORE | Item.LAYOUT_RIGHT</xsl:text>
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
    <xsl:template match="screen/options" mode="LCDUI-create">
        <xsl:text>        final ChoiceGroup </xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-varname"/>
        <xsl:text> = new ChoiceGroup(null, ChoiceGroup.EXCLUSIVE);&#10;</xsl:text>
        <xsl:apply-templates select="option/text" mode="LCDUI-create"/>
    </xsl:template>

    <xsl:template match="option/text" mode="LCDUI-create">
        <xsl:text>        </xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-varname"/>
        <xsl:text>.append(</xsl:text>
        <xsl:apply-templates select="." mode="Screen-printf"/>
        <xsl:text>, null);&#10;</xsl:text>
    </xsl:template>

    <xsl:template match="screen/options" mode="LCDUI-map-item-to-command-id">
        <xsl:text>                    switch(((ChoiceGroup)item).getSelectedIndex()) {&#10;</xsl:text>
        <xsl:apply-templates select="option" mode="LCDUI-map-item-to-command-id"/>
        <xsl:text>                    }&#10;</xsl:text>
    </xsl:template>

    <xsl:template match="option" mode="LCDUI-map-item-to-command-id">
        <xsl:text>                    case </xsl:text>
        <xsl:value-of select="position() - 1"/>
        <xsl:text>:&#10;</xsl:text>
        <xsl:text>                        commandId = </xsl:text>
        <xsl:apply-templates select="." mode="Screen-command-id"/>
        <xsl:text>;&#10;</xsl:text>
        <xsl:text>                         break;&#10;</xsl:text>
    </xsl:template>


    <xsl:template match="screen/text|screen/filler|screen/options" mode="LCDUI-varname">
        <xsl:variable name="screen-id" select="ancestor::screen/@name"/>
        <xsl:text>item</xsl:text>
        <xsl:value-of select="count((preceding-sibling::text|preceding-sibling::filler|preceding-sibling::options)[ancestor::screen/@name=$screen-id]) + 1"/>
    </xsl:template>

    <xsl:template match="*" mode="LCDUI-varname">
        <xsl:apply-templates select=".." mode="LCDUI-varname"/>
    </xsl:template>
</xsl:stylesheet>
