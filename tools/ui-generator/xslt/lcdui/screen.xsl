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

    <xsl:template match="screen" mode="Screen-imports">
        <xsl:sequence select="(
            'javax.microedition.lcdui.Displayable',
            'javax.microedition.lcdui.ChoiceGroup',
            'javax.microedition.lcdui.Choice',
            'javax.microedition.lcdui.StringItem',
            'javax.microedition.lcdui.Gauge',
            'javax.microedition.lcdui.TextField',
            'javax.microedition.lcdui.Spacer',
            'javax.microedition.lcdui.Item',
            'javax.microedition.lcdui.ItemCommandListener',
            'javax.microedition.lcdui.Command',
            'javax.microedition.lcdui.Form',
            'javax.microedition.lcdui.List')"/>
    </xsl:template>


    <xsl:template match="screen" mode="Screen-define-initializer-signature">
        <xsl:text>protected Displayable createDisplayable()</xsl:text>
    </xsl:template>


    <xsl:template match="screen" mode="Screen-define-initializer-header">
        <xsl:apply-imports/>
        <xsl:apply-templates select="." mode="LCDUI-create-displayable"/>
        <xsl:text>;&#10;&#10;</xsl:text>
    </xsl:template>

    <xsl:template match="screen" mode="LCDUI-create-displayable">
        <xsl:text>Form d = createForm(</xsl:text>
        <xsl:value-of select="uig:Screen-title(.)"/>
        <xsl:text>)</xsl:text>
    </xsl:template>

    <xsl:template match="screen[options/@style='fullscreen']" mode="LCDUI-create-displayable">
        <xsl:text>List d = createList(</xsl:text>
        <xsl:value-of select="uig:Screen-title(.)"/>
        <xsl:text>)</xsl:text>
    </xsl:template>


    <xsl:template match="screen" mode="Screen-define-initializer-footer">
        <xsl:if test="options">
            <xsl:text>final Command selectItemCommand = getSelectItemCommand();&#10;</xsl:text>
        </xsl:if>
        <xsl:if test="options[not(@style='fullscreen')]">
            <xsl:text>ItemCommandListener icl = new ItemCommandListener() {&#10;</xsl:text>
            <xsl:text>    public void commandAction(Command c, Item item)</xsl:text>
            <xsl:value-of select="uig:Screen-event-handler-body(., options)"/>
            <xsl:text>};&#10;</xsl:text>
            <xsl:for-each select="options">
                <xsl:value-of select="concat(
                    uig:Screen-item-variable-name(.),
                    '.setItemCommandListener(icl);&#10;',
                    uig:Screen-item-variable-name(.),
                    '.addCommand(selectItemCommand);&#10;')"/>
            </xsl:for-each>
            <xsl:text>&#10;</xsl:text>
        </xsl:if>
        <xsl:if test="options[@style='fullscreen']">
            <xsl:text>d.setSelectCommand(selectItemCommand);&#10;</xsl:text>
        </xsl:if>

        <xsl:variable name="items" select="command|dynamic-command|options[@style='fullscreen']" as="element()*"/>
        <xsl:if test="$items">
            <xsl:text>javax.microedition.lcdui.CommandListener cl = new javax.microedition.lcdui.CommandListener() {&#10;</xsl:text>
            <xsl:text>    public void commandAction(Command item, Displayable d)</xsl:text>
            <xsl:value-of select="uig:Screen-event-handler-body(., $items)"/>
            <xsl:text>};&#10;</xsl:text>
            <xsl:text>d.setCommandListener(cl);&#10;&#10;</xsl:text>
        </xsl:if>
        <xsl:text>return d;&#10;</xsl:text>
    </xsl:template>


    <xsl:template match="screen/text" mode="Screen-item-variable-type-impl">
        <xsl:text>StringItem</xsl:text>
    </xsl:template>
    <xsl:template match="screen/command|screen/dynamic-command" mode="Screen-item-variable-type-impl">
        <xsl:text>Command</xsl:text>
    </xsl:template>
    <xsl:template match="screen/progress" mode="Screen-item-variable-type-impl">
        <xsl:text>Gauge</xsl:text>
    </xsl:template>
    <xsl:template match="screen/text-field" mode="Screen-item-variable-type-impl">
        <xsl:text>TextField</xsl:text>
    </xsl:template>
    <xsl:template match="screen/options[not(@style='fullscreen')]" mode="Screen-item-variable-type-impl">
        <xsl:text>ChoiceGroup</xsl:text>
    </xsl:template>
    <xsl:template match="screen/options/option|screen/options/dynamic-option" mode="Screen-item-variable-type-impl">
        <xsl:text>String</xsl:text>
    </xsl:template>
    <xsl:template match="screen/options[@style='fullscreen']" mode="Screen-item-variable-type-impl">
        <xsl:text>Choice</xsl:text>
    </xsl:template>
    <xsl:template match="*" mode="Screen-item-variable-type-impl">
        <xsl:call-template name="error-unexpected"/>
    </xsl:template>


    <xsl:template match="options" mode="Screen-options-get-selected-item">
        <xsl:value-of select="
            concat(uig:Screen-item-variable-name(.), '.getSelectedIndex()')"/>
    </xsl:template>


    <!--
        Top level 'text' element.
    -->
    <xsl:template match="screen/text" mode="Screen-create-item">
        <xsl:value-of select="concat(
            'new ',
            uig:Screen-item-variable-type(.),
            '(null, ',
            uig:Screen-item-get-label(.),
            ', Item.PLAIN)')"/>
    </xsl:template>


    <!--
        Top level 'options' element.
    -->
    <xsl:template match="screen/options[@style='fullscreen']" mode="Screen-create-item">
        <xsl:text>d</xsl:text>
    </xsl:template>

    <xsl:template match="screen/options[not(@style='fullscreen')]" mode="Screen-create-item">
        <xsl:value-of select="concat(
            'new ',
            uig:Screen-item-variable-type(.),
            '(',
            uig:Screen-item-get-label(.))"/>
        <xsl:text>, ChoiceGroup.</xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-options-type"/>
        <xsl:text>)</xsl:text>
    </xsl:template>

    <xsl:template match="*[@style='dropdown']" mode="LCDUI-options-type">
        <xsl:text>POPUP</xsl:text>
    </xsl:template>
    <xsl:template match="*[@style='plain'or not(@style)]" mode="LCDUI-options-type">
        <xsl:text>EXCLUSIVE</xsl:text>
    </xsl:template>

    <!--
        'options(option|dynamic-option)' element.
    -->
    <xsl:template match="screen/options/option|screen/options/dynamic-option" mode="Screen-add-item-impl">
        <xsl:value-of select="concat(
            uig:Screen-item-variable-name(..),
            '.append(',
            uig:Screen-item-get-label(.),
            ', null)')"/>
    </xsl:template>


    <!--
        Top level 'progress' element.
    -->
    <xsl:template match="screen/progress" mode="Screen-create-item">
        <xsl:value-of select="concat(
            'new ',
            uig:Screen-item-variable-type(.),
            '(',
            uig:Screen-item-get-label(.))"/>
        <xsl:text>, false, 100, 0)</xsl:text>
    </xsl:template>


    <!--
        Top level 'text-field' element.
    -->
    <xsl:template match="screen/text-field" mode="Screen-create-item">
        <xsl:value-of select="concat(
            'new ',
            uig:Screen-item-variable-type(.),
            '(',
            uig:Screen-item-get-label(.))"/>
        <xsl:text>, "", </xsl:text>
        <xsl:value-of select="@maxchars"/>
        <xsl:text>, </xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-text-field-style"/>
        <xsl:text>)</xsl:text>
    </xsl:template>

    <xsl:template match="text-field[@style='password']" mode="LCDUI-text-field-style">
        <xsl:text>TextField.ANY | TextField.PASSWORD</xsl:text>
    </xsl:template>
    <xsl:template match="text-field[@style='plain' or not(@style)]" mode="LCDUI-text-field-style">
        <xsl:text>TextField.ANY</xsl:text>
    </xsl:template>


    <!--
        Top level 'command' and 'dynamic-command' elements.
    -->
    <xsl:template match="command|dynamic-command" mode="Screen-create-item">
        <xsl:value-of select="concat(
            'new ',
            uig:Screen-item-variable-type(.),
            '(',
            uig:Screen-item-get-label(.))"/>
        <xsl:text>, </xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-command-type"/>
        <xsl:text>, 1)</xsl:text>
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
        Appends an item to the form.
    -->
    <xsl:template match="text|progress|text-field|options[not(@style='fullscreen')]" mode="Screen-add-item-impl">
        <xsl:value-of select="uig:Screen-item-variable-name(.)"/>
        <xsl:text>.setLayout(Item.LAYOUT_2 | </xsl:text>
        <xsl:apply-templates select="." mode="LCDUI-layout"/>
        <xsl:text>);&#10;d.append(</xsl:text>
        <xsl:value-of select="uig:Screen-item-variable-name(.)"/>
        <xsl:text>)</xsl:text>
    </xsl:template>

    <xsl:template match="options[@style='fullscreen']" mode="Screen-add-item-impl"/>

    <xsl:template match="command|dynamic-command" mode="Screen-add-item-impl">
        <xsl:text>d.addCommand(</xsl:text>
        <xsl:value-of select="uig:Screen-item-variable-name(.)"/>
        <xsl:text>)</xsl:text>
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
        ==================================================================
        LCDUI specifics going beyond of standard templates to override.
        ==================================================================
    -->
    <xsl:template match="screen/options[@style='fullscreen']" mode="Screen-event-handler-body-test-item">
        <xsl:text>item == selectItemCommand</xsl:text>
    </xsl:template>

</xsl:stylesheet>
