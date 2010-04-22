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
        Writes all screen classes source code.
    -->
    <xsl:template name="top-Screen">
        <xsl:apply-templates select="//screen" mode="Screen-class"/>
    </xsl:template>


    <!--
        Writes screen class source code.
    -->
    <xsl:template match="screen" mode="Screen-class">
        <xsl:variable name="href" select="uig:classname-to-filepath(uig:Screen-classname(.))"/>
        <xsl:value-of select="concat($href,'&#10;')"/>
        <xsl:result-document href="{$href}">
            <xsl:apply-templates select="." mode="Screen-define"/>
        </xsl:result-document>
    </xsl:template>

    <xsl:template match="screen" mode="Screen-define">
        <!-- package name and bunch of imports -->
        <xsl:text>package </xsl:text>
        <xsl:value-of select="$package-name"/>
        <xsl:text>;&#10;&#10;&#10;</xsl:text>
        <xsl:value-of>
            <xsl:variable name="imports" as="xs:string*">
                <xsl:sequence select="(
                    'com.sun.uig.*',
                    'java.util.Enumeration',
                    'java.util.Vector')"/>
                <xsl:apply-templates select="." mode="Screen-imports"/>
            </xsl:variable>
            <xsl:for-each select="$imports">
                <xsl:value-of select="concat('import ', ., ';&#10;')"/>
            </xsl:for-each>
            <xsl:text>&#10;&#10;</xsl:text>
        </xsl:value-of>

        <!-- class hierarchy -->
        <xsl:text>public final class </xsl:text>
        <xsl:value-of select="uig:Screen-classname(.)"/>
        <xsl:text> extends Screen </xsl:text>
        <xsl:variable name="ifaces" as="xs:string*">
            <xsl:if test="uig:Screen-class-with-ProgressUpdater(.)">ProgressUpdater</xsl:if>
            <xsl:if test="uig:Screen-class-with-TextFieldAccessor(.)">TextFieldAccessor</xsl:if>
        </xsl:variable>
        <xsl:if test="count($ifaces) != 0">
            <xsl:text>implements </xsl:text>
            <xsl:for-each select="$ifaces">
                <xsl:value-of select="."/>
                <xsl:if test="position() != last()">, </xsl:if>
            </xsl:for-each>
        </xsl:if>
        <xsl:text> {&#10;</xsl:text>

        <!-- define constants -->
        <xsl:apply-templates select="." mode="Screen-define-constants"/>

        <!-- member fields -->
        <xsl:for-each select="descendant::*[uig:Screen-has-member-field-for-item(.)]">
            <xsl:text>    private </xsl:text>
            <xsl:value-of select="uig:Screen-item-variable-type(.)"/>
            <xsl:text> </xsl:text>
            <xsl:value-of select="uig:Screen-item-variable-name(.)"/>
            <xsl:text>;&#10;</xsl:text>
            <xsl:apply-templates select="." mode="Screen-item-declare-cache-variables"/>
        </xsl:for-each>
        <xsl:if test="uig:Screen-class-with-CommandListener(.)">
            <xsl:text>    private CommandListener listener;&#10;&#10;</xsl:text>
        </xsl:if>

        <!-- ctor -->
        <xsl:text>    public </xsl:text>
        <xsl:value-of select="uig:Screen-classname(.)"/>
        <xsl:text>(ScreenProperties props, Strings strings</xsl:text>
        <xsl:if test="uig:Screen-class-with-CommandListener(.)">
            <xsl:text>, CommandListener listener</xsl:text>
        </xsl:if>
        <xsl:text>) {&#10;        super(props, strings);&#10;</xsl:text>
        <xsl:if test="uig:Screen-class-with-CommandListener(.)">
            <xsl:text>&#10;        this.listener = listener;&#10;&#10;</xsl:text>
        </xsl:if>
        <xsl:apply-templates select="." mode="Screen-ctor-body"/>
        <xsl:text>    }&#10;&#10;</xsl:text>

        <xsl:if test="uig:Screen-class-with-ProgressUpdater(.)">
            <xsl:value-of select="uig:Screen-define-item-accessor(
                    progress,
                    'public void updateProgress(Object itemId, int value, int max)',
                    true())"/>
        </xsl:if>

        <xsl:if test="uig:Screen-class-with-TextFieldAccessor(.)">
            <xsl:value-of select="uig:Screen-define-item-accessor(
                    text-field,
                    'public void setTextFieldValue(Object itemId, String value)',
                    true())"/>
            <xsl:value-of select="uig:Screen-define-item-accessor(
                    text-field,
                    'public String getTextFieldValue(Object itemId)',
                    false())"/>
        </xsl:if>

        <!-- initializer -->
        <xsl:apply-templates select="." mode="Screen-define-initializer"/>
        <xsl:text>}&#10;</xsl:text>
    </xsl:template>


    <xsl:function name="uig:Screen-define-item-accessor" as="xs:string">
        <xsl:param name="items" as="element()*"/>
        <xsl:param name="signature" as="xs:string"/>
        <xsl:param name="setter" as="xs:boolean"/>

        <xsl:value-of>
            <xsl:value-of select="uig:add-indentation($signature, '    ')"/>

            <xsl:text> {&#10;        synchronized (layoutLock) {&#10;</xsl:text>
            <xsl:call-template name="add-indentation">
                <xsl:with-param name="text">
                    <xsl:for-each select="$items">
                        <xsl:value-of select="
                                concat('if (itemId.equals(', uig:Screen-item-id(.), ')) {&#10;')"/>
                        <xsl:choose>
                            <xsl:when test="$setter">
                                <xsl:value-of select="
                                    uig:add-indentation(uig:Screen-item-init-cache(.), '    ')"/>
                                <xsl:value-of select="
                                    uig:add-indentation(uig:Screen-item-init-from-cache(.), '    ')"/>
                                <xsl:text>    return;&#10;</xsl:text>
                            </xsl:when>
                            <xsl:otherwise>
                                <xsl:value-of select="
                                    uig:add-indentation(uig:Screen-item-update-cache(.), '    ')"/>
                                <xsl:text>    return </xsl:text>
                                <xsl:value-of select="uig:Screen-item-variable-name(.)"/>
                                <xsl:text>_cached_value;&#10;</xsl:text>
                            </xsl:otherwise>
                        </xsl:choose>
                        <xsl:text>}&#10;</xsl:text>
                    </xsl:for-each>
                    <xsl:text>throw new RuntimeException(itemId + " not found");&#10;</xsl:text>
                </xsl:with-param>
                <xsl:with-param name="ident" select="'            '"/>
            </xsl:call-template>
            <xsl:text>        }&#10;    }&#10;&#10;</xsl:text>
        </xsl:value-of>
    </xsl:function>


    <!--
        Returns screen class name.
    -->
    <xsl:function name="uig:Screen-classname" as="xs:string">
        <xsl:param name="screen" as="element()"/>
        <xsl:value-of select="$screen/@name"/>
    </xsl:function>


    <!--
        Returns screen title.
    -->
    <xsl:function name="uig:Screen-title" as="xs:string">
        <xsl:param name="screen" as="element()"/>
        <xsl:choose>
            <xsl:when test="$screen/title">
                <xsl:value-of select="uig:Screen-printf($screen/title)"/>
            </xsl:when>
            <xsl:when test="$screen/options[@style='fullscreen']">
                <xsl:value-of select="uig:Screen-printf($screen/options/label)"/>
            </xsl:when>
            <xsl:otherwise>null</xsl:otherwise>
        </xsl:choose>
    </xsl:function>


    <!--
        Returns 'true' if screen class needs CommandListener.
    -->
    <xsl:function name="uig:Screen-class-with-CommandListener" as="xs:boolean">
        <xsl:param name="screen" as="element()"/>
        <xsl:value-of select="count($screen/descendant::*[not(self::progress|self::text-field)]/@id) != 0"/>
    </xsl:function>


    <!--
        Returns 'true' if screen class implements ProgressUpdater.
    -->
    <xsl:function name="uig:Screen-class-with-ProgressUpdater" as="xs:boolean">
        <xsl:param name="screen" as="element()"/>
        <xsl:value-of select="count($screen[progress]) != 0"/>
    </xsl:function>


    <!--
        Returns 'true' if screen class implements TextFieldAccessor.
    -->
    <xsl:function name="uig:Screen-class-with-TextFieldAccessor" as="xs:boolean">
        <xsl:param name="screen" as="element()"/>
        <xsl:value-of select="count($screen[text-field]) != 0"/>
    </xsl:function>


    <!--
        Variable name for a screen item.
    -->
    <xsl:function name="uig:Screen-item-variable-name" as="xs:string">
        <xsl:param name="e" as="element()"/>
        <xsl:value-of select="concat('item_', generate-id($e))"/>
    </xsl:function>


    <!--
        Variable type for a screen item.
    -->
    <xsl:function name="uig:Screen-item-variable-type" as="xs:string">
        <xsl:param name="e" as="element()"/>
        <xsl:apply-templates select="$e" mode="Screen-item-variable-type-impl"/>
    </xsl:function>


    <!--
        Returns 'true' if specified screen item should have corresponding
        member field in screen class.
    -->
    <xsl:function name="uig:Screen-has-member-field-for-item" as="xs:boolean">
        <xsl:param name="e" as="element()"/>
        <xsl:apply-templates select="$e" mode="Screen-has-member-field-for-item-impl"/>
    </xsl:function>

    <xsl:template match="*" mode="Screen-has-member-field-for-item-impl">
        <xsl:value-of select="false()"/>
    </xsl:template>

    <xsl:template match="screen/progress|screen/text-field" mode="Screen-has-member-field-for-item-impl">
        <xsl:value-of select="true()"/>
    </xsl:template>


    <!--
        Id for screen item.
    -->
    <xsl:function name="uig:Screen-item-id" as="xs:string">
        <xsl:param name="e" as="element()"/>
        <xsl:apply-templates select="$e" mode="Screen-item-id-impl"/>
    </xsl:function>

    <xsl:template match="command|option|dynamic-command|dynamic-option" mode="Screen-item-id-impl">
        <xsl:value-of select="concat('COMMAND_ID_', upper-case(@id))"/>
    </xsl:template>

    <xsl:template match="progress" mode="Screen-item-id-impl">
        <xsl:value-of select="concat('PROGRESS_ID_', upper-case(@id))"/>
    </xsl:template>

    <xsl:template match="text-field" mode="Screen-item-id-impl">
        <xsl:value-of select="concat('TEXT_FIELD_ID_', upper-case(@id))"/>
    </xsl:template>


    <!--
        Output java code to create string to display
        at run-time from some element body.
    -->
    <xsl:function name="uig:Screen-printf" as="xs:string">
        <xsl:param name="e" as="element()"/>
        <xsl:value-of>
            <xsl:text>printf(StringIds.</xsl:text>
            <xsl:value-of select="uig:I18N-key($e)"/>
            <xsl:text>, new Object[] { </xsl:text>
            <xsl:for-each select="uig:format-string-get-args($e)">
                <xsl:value-of select="concat('getProperty(KEY_', upper-case(.), '), ')"/>
            </xsl:for-each>
            <xsl:text>})</xsl:text>
        </xsl:value-of>
    </xsl:function>


    <!--
        Item's label if any
    -->
    <xsl:function name="uig:Screen-item-get-label" as="xs:string">
        <xsl:param name="e" as="element()"/>
        <xsl:apply-templates select="$e" mode="Screen-item-get-label-impl"/>
    </xsl:function>

    <xsl:template match="*[label]" mode="Screen-item-get-label-impl">
        <xsl:value-of select="uig:Screen-printf(label)"/>
    </xsl:template>
    <xsl:template match="text|option|command" mode="Screen-item-get-label-impl">
        <xsl:value-of select="uig:Screen-printf(.)"/>
    </xsl:template>
    <xsl:template match="dynamic-command|dynamic-option" mode="Screen-item-get-label-impl">
        <xsl:text>e.nextElement().toString()</xsl:text>
    </xsl:template>
    <xsl:template match="*" mode="Screen-item-get-label-impl">
        <xsl:text>null</xsl:text>
    </xsl:template>


    <!--
        Defines class constants.
    -->
    <xsl:template match="screen" mode="Screen-define-constants">
        <xsl:variable name="all-format-string-args" as="xs:string*">
            <xsl:for-each select="uig:get-format-string-elements(.)">
                <xsl:for-each select="uig:format-string-get-args(.)">
                    <xsl:value-of>
                        <xsl:text>    public final static String </xsl:text>
                        <xsl:value-of select="concat('KEY_', upper-case(.))"/>
                        <xsl:text> = "</xsl:text>
                        <xsl:value-of select="."/>
                        <xsl:text>";&#10;</xsl:text>
                    </xsl:value-of>
                </xsl:for-each>
            </xsl:for-each>
        </xsl:variable>
        <xsl:if test="count($all-format-string-args) != 0">
            <xsl:for-each select="distinct-values($all-format-string-args)">
                <xsl:value-of select="."/>
            </xsl:for-each>
            <xsl:text>&#10;</xsl:text>
        </xsl:if>

        <xsl:if test="descendant::*[@id]">
            <xsl:apply-templates select="descendant::*[@id]" mode="Screen-define-ids"/>
            <xsl:text>&#10;</xsl:text>
        </xsl:if>
    </xsl:template>

    <xsl:template match="*" mode="Screen-define-ids">
        <xsl:text>    public final static int </xsl:text>
        <xsl:value-of select="uig:Screen-item-id(.)"/>
        <xsl:text> = </xsl:text>
        <xsl:value-of select="count(preceding::*[@id]) + 1"/>
        <xsl:text>;&#10;</xsl:text>
    </xsl:template>

    <xsl:template match="screen/progress|screen/text-field" mode="Screen-define-ids">
        <xsl:text>    public final static Object </xsl:text>
        <xsl:value-of select="uig:Screen-item-id(.)"/>
        <xsl:text> = "</xsl:text>
        <xsl:value-of select="@id"/>
        <xsl:text>";&#10;</xsl:text>
    </xsl:template>


    <!--
        Defines initializer function.
    -->
    <xsl:template match="screen" mode="Screen-define-initializer">
        <xsl:text>    </xsl:text>
        <xsl:apply-templates select="." mode="Screen-define-initializer-signature"/>
        <xsl:text> {
        synchronized (layoutLock) {
</xsl:text>
        <xsl:value-of>
            <xsl:variable name="body">
                <xsl:apply-templates select="." mode="Screen-define-initializer-header"/>
                <xsl:apply-templates select="." mode="Screen-define-initializer-body"/>
                <xsl:apply-templates select="." mode="Screen-define-initializer-footer"/>
            </xsl:variable>
            <xsl:value-of select="uig:add-indentation($body, '            ')"/>
        </xsl:value-of>
        <xsl:text>        }&#10;    }&#10;</xsl:text>
    </xsl:template>

    <xsl:template match="screen" mode="Screen-define-initializer-header">
        <xsl:if test="descendant::dynamic-option|descendant::dynamic-command">
            <xsl:text>int counter = 0;&#10;&#10;</xsl:text>
        </xsl:if>

        <xsl:if test="descendant::dynamic-command">
            <xsl:text>final Vector commands = new Vector();&#10;&#10;</xsl:text>
        </xsl:if>
    </xsl:template>

    <xsl:template match="screen" mode="Screen-define-initializer-body">
        <xsl:apply-templates select="*[not(self::title)]" mode="Screen-add-item"/>
    </xsl:template>

    <xsl:template match="screen" mode="Screen-init-items-from-cache">
        <!-- init from cache -->
        <xsl:for-each select="descendant::*[uig:Screen-has-member-field-for-item(.)]">
            <xsl:value-of select="uig:Screen-item-init-from-cache(.)"/>
        </xsl:for-each>
    </xsl:template>


    <!--
        Adds an item to the screen.
    -->
    <xsl:template match="*" mode="Screen-item-legend">
        <xsl:text>// </xsl:text>
        <xsl:value-of select="name()"/>
        <xsl:for-each select="attribute::*">
            <xsl:value-of select="concat(' ', name())"/>
            <xsl:text>="</xsl:text>
            <xsl:value-of select="."/>
            <xsl:text>"</xsl:text>
        </xsl:for-each>
        <xsl:text>&#10;</xsl:text>
    </xsl:template>

    <xsl:template match="*" mode="Screen-declare-item">
        <xsl:if test="not(uig:Screen-has-member-field-for-item(.))">
            <xsl:text>final </xsl:text>
            <xsl:value-of select="uig:Screen-item-variable-type(.)"/>
            <xsl:text> </xsl:text>
        </xsl:if>
        <xsl:value-of select="uig:Screen-item-variable-name(.)"/>
        <xsl:text> = </xsl:text>
    </xsl:template>

    <xsl:template match="text|progress|text-field" mode="Screen-add-item">
        <xsl:apply-templates select="." mode="Screen-item-legend"/>
        <xsl:apply-templates select="." mode="Screen-declare-item"/>
        <xsl:apply-templates select="." mode="Screen-create-item"/>
        <xsl:text>;&#10;</xsl:text>
        <xsl:apply-templates select="." mode="Screen-add-item-impl"/>
        <xsl:text>;&#10;&#10;</xsl:text>
    </xsl:template>

    <xsl:template match="command" mode="Screen-add-item">
        <xsl:apply-templates select="." mode="Screen-item-legend"/>
        <xsl:apply-templates select="." mode="Screen-declare-item"/>
        <xsl:apply-templates select="." mode="Screen-create-item"/>
        <xsl:text>;&#10;</xsl:text>
        <xsl:if test="../dynamic-command">
            <xsl:value-of select="
                concat(
                    'commands.addElement(',
                    uig:Screen-item-variable-name(.),
                    ');&#10;')"/>
        </xsl:if>
        <xsl:apply-templates select="." mode="Screen-add-item-impl"/>
        <xsl:text>;&#10;&#10;</xsl:text>
    </xsl:template>

    <xsl:template match="option" mode="Screen-add-item">
        <xsl:apply-templates select="." mode="Screen-item-legend"/>
        <xsl:apply-templates select="." mode="Screen-add-item-impl"/>
        <xsl:text>;&#10;</xsl:text>
    </xsl:template>

    <xsl:template match="options" mode="Screen-add-item">
        <xsl:apply-templates select="." mode="Screen-item-legend"/>
        <xsl:apply-templates select="." mode="Screen-declare-item"/>
        <xsl:apply-templates select="." mode="Screen-create-item"/>
        <xsl:text>;&#10;</xsl:text>
        <xsl:apply-templates select="option|dynamic-option" mode="Screen-add-item"/>
        <xsl:apply-templates select="." mode="Screen-add-item-impl"/>
        <xsl:text>;&#10;</xsl:text>
    </xsl:template>

    <xsl:template match="dynamic-command|dynamic-option" mode="Screen-add-item">
        <xsl:apply-templates select="." mode="Screen-item-legend"/>
        <xsl:text>counter = </xsl:text>
        <xsl:apply-templates select="." mode="Screen-item-base-idx"/>
        <xsl:text>;&#10;</xsl:text>
        <xsl:text>for (Enumeration e = (Enumeration)getProperty("</xsl:text>
        <xsl:value-of select="uig:Screen-item-id(.)"/>
        <xsl:text>") ; e.hasMoreElements(); ++counter) {&#10;</xsl:text>
        <xsl:value-of>
            <xsl:variable name="body">
                <xsl:if test="../dynamic-command">
                    <xsl:apply-templates select="." mode="Screen-declare-item"/>
                    <xsl:apply-templates select="." mode="Screen-create-item"/>
                    <xsl:text>;&#10;</xsl:text>
                </xsl:if>
                <xsl:apply-templates select="." mode="Screen-add-item-impl"/>
                <xsl:text>;&#10;</xsl:text>
                <xsl:if test="../dynamic-command">
                    <xsl:value-of select="
                        concat(
                            'commands.addElement(',
                            uig:Screen-item-variable-name(.),
                            ');&#10;')"/>
                </xsl:if>
            </xsl:variable>
            <xsl:value-of select="replace($body, '^', '    ', 'm')"/>
        </xsl:value-of>
        <xsl:text>}&#10;</xsl:text>
        <xsl:text>final int </xsl:text>
        <xsl:apply-templates select="." mode="Screen-dynamic-item-range-value"/>
        <xsl:text> = counter;&#10;</xsl:text>
        <xsl:if test="parent::screen">&#10;</xsl:if>
    </xsl:template>

    <xsl:template match="dynamic-option|dynamic-command" mode="Screen-dynamic-item-range-value">
        <xsl:value-of select="concat('range_',generate-id())"/>
    </xsl:template>

    <xsl:template match="command|option|dynamic-option|dynamic-command" mode="Screen-item-base-idx">
        <xsl:choose>
            <xsl:when test="preceding-sibling::*[self::dynamic-option or self::dynamic-command]">
                <xsl:apply-templates select="preceding-sibling::*[self::dynamic-option or self::dynamic-command][1]" mode="Screen-dynamic-item-range-value"/>
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


    <!--
        Body of compound event handler for all top level items.
    -->
    <xsl:function name="uig:Screen-event-handler-body" as="xs:string">
        <xsl:param name="screen"    as="element()"/>
        <xsl:param name="items"     as="element()*"/>

        <xsl:variable name="body">
            <xsl:if test="$screen/dynamic-command">
                <xsl:text>int idx;&#10;&#10;</xsl:text>
            </xsl:if>
            <xsl:text>int commandId = -1;&#10;</xsl:text>
            <xsl:text>int commandIdIdx = -1;&#10;</xsl:text>
            <xsl:text>if (false) {&#10;</xsl:text>
            <xsl:text>} </xsl:text>

            <xsl:apply-templates select="$items" mode="Screen-event-handler-body"/>

            <xsl:text>else {&#10;    return;&#10;}</xsl:text>

            <xsl:text>&#10;&#10;</xsl:text>
            <xsl:text>if (commandIdIdx == -1)&#10;</xsl:text>
            <xsl:text>    listener.onCommand(</xsl:text>
            <xsl:value-of select="uig:Screen-classname($screen)"/>
            <xsl:text>.this, commandId);&#10;</xsl:text>
            <xsl:text> else&#10;</xsl:text>
            <xsl:text>    listener.onDynamicCommand(</xsl:text>
            <xsl:value-of select="uig:Screen-classname($screen)"/>
            <xsl:text>.this, commandId, commandIdIdx);&#10;</xsl:text>
        </xsl:variable>
        <xsl:value-of select="
            concat(
                ' {&#10;',
                replace($body, '^', '        ', 'm'),
                '    }&#10;')"/>
    </xsl:function>

    <xsl:template match="*" mode="Screen-event-handler-body">
        <xsl:text>else if (</xsl:text>
        <xsl:apply-templates select="." mode="Screen-event-handler-body-test-item"/>
        <xsl:text>) {&#10;</xsl:text>
        <xsl:value-of select="replace(uig:Screen-item-map-command-id(.), '^', '    ', 'm')"/>
        <xsl:text>} </xsl:text>
    </xsl:template>

    <xsl:template match="*" mode="Screen-event-handler-body-test-item">
        <xsl:value-of select="concat('item == ', uig:Screen-item-variable-name(.))"/>
    </xsl:template>

    <xsl:template match="option" mode="Screen-event-handler-body-test-item">
        <xsl:apply-templates select="." mode="Screen-item-base-idx"/>
        <xsl:text> == idx</xsl:text>
    </xsl:template>

    <xsl:template match="dynamic-command" mode="Screen-event-handler-body-test-item">
        <xsl:apply-templates select="." mode="Screen-item-base-idx"/>
        <xsl:text><![CDATA[ <= (idx = commands.indexOf(item, ]]></xsl:text>
        <xsl:apply-templates select="." mode="Screen-item-base-idx"/>
        <xsl:text><![CDATA[)) && idx < ]]></xsl:text>
        <xsl:apply-templates select="." mode="Screen-dynamic-item-range-value"/>
    </xsl:template>

    <xsl:template match="dynamic-option" mode="Screen-event-handler-body-test-item">
        <xsl:apply-templates select="." mode="Screen-item-base-idx"/>
        <xsl:text><![CDATA[ <= idx && idx < ]]></xsl:text>
        <xsl:apply-templates select="." mode="Screen-dynamic-item-range-value"/>
    </xsl:template>


    <!--
        Maps item event into item's ID.
    -->
    <xsl:function name="uig:Screen-item-map-command-id" as="xs:string">
        <xsl:param name="e" as="element()"/>
        <xsl:value-of>
            <xsl:apply-templates select="$e" mode="Screen-item-map-command-id"/>
        </xsl:value-of>
    </xsl:function>

    <xsl:template match="options" mode="Screen-item-map-command-id">
        <xsl:text>final int idx = </xsl:text>
        <xsl:apply-templates select="." mode="Screen-options-get-selected-item"/>
        <xsl:text>;&#10;</xsl:text>
        <xsl:text>if (false) {&#10;</xsl:text>
        <xsl:text>} </xsl:text>
        <xsl:apply-templates select="option|dynamic-option" mode="Screen-event-handler-body"/>
        <xsl:text>&#10;</xsl:text>
    </xsl:template>

    <xsl:template match="command|option" mode="Screen-item-map-command-id">
        <xsl:text>commandId = </xsl:text>
        <xsl:value-of select="uig:Screen-item-id(.)"/>
        <xsl:text>;&#10;</xsl:text>
    </xsl:template>

    <xsl:template match="dynamic-command|dynamic-option" mode="Screen-item-map-command-id">
        <xsl:text>commandId = </xsl:text>
        <xsl:value-of select="uig:Screen-item-id(.)"/>
        <xsl:text>;&#10;</xsl:text>
        <xsl:text>commandIdIdx = idx - (</xsl:text>
        <xsl:apply-templates select="." mode="Screen-item-base-idx"/>
        <xsl:text>);&#10;</xsl:text>
    </xsl:template>




    <!--
        Item cache variables declaration.
    -->
    <xsl:template match="screen/text-field" mode="Screen-item-declare-cache-variables">
        <xsl:text>    private String </xsl:text>
        <xsl:value-of select="concat(uig:Screen-item-variable-name(.), '_cached_value')"/>
        <xsl:text> = "";&#10;</xsl:text>
    </xsl:template>

    <xsl:template match="screen/progress" mode="Screen-item-declare-cache-variables">
        <xsl:text>    private int </xsl:text>
        <xsl:value-of select="concat(uig:Screen-item-variable-name(.), '_cached_max')"/>
        <xsl:text> = 100;&#10;</xsl:text>
        <xsl:text>    private int </xsl:text>
        <xsl:value-of select="concat(uig:Screen-item-variable-name(.), '_cached_value')"/>
        <xsl:text> = 0;&#10;</xsl:text>
    </xsl:template>

    <xsl:template match="*" mode="Screen-item-declare-cache-variables">
        <xsl:call-template name="error-unexpected"/>
    </xsl:template>


    <!--
        Update cached item value.
    -->
    <xsl:function name="uig:Screen-item-update-cache" as="xs:string">
        <xsl:param name="e" as="element()"/>
        <xsl:value-of>
            <xsl:value-of select="
                concat('if (', uig:Screen-item-variable-name($e), ' != null) {&#10;')"/>
            <xsl:call-template name="add-indentation">
                <xsl:with-param name="text">
                    <xsl:apply-templates select="$e" mode="Screen-item-update-cache"/>
                </xsl:with-param>
                <xsl:with-param name="ident" select="'    '"/>
            </xsl:call-template>
            <xsl:text>}&#10;</xsl:text>
        </xsl:value-of>
    </xsl:function>

    <xsl:template match="screen/text-field" mode="Screen-item-update-cache">
        <xsl:value-of select="concat(
            uig:Screen-item-variable-name(.),
            '_cached_value = Screen.getTextFieldValue(',
            uig:Screen-item-variable-name(.),
            ');&#10;')"/>
    </xsl:template>

    <xsl:template match="screen/progress" mode="Screen-item-update-cache">
        <xsl:value-of select="concat(
            uig:Screen-item-variable-name(.),
            '_cached_value = Screen.getProgressValue(',
            uig:Screen-item-variable-name(.),
            ');&#10;')"/>
        <xsl:value-of select="concat(
            uig:Screen-item-variable-name(.),
            '_cached_max = Screen.getProgressMaxValue(',
            uig:Screen-item-variable-name(.),
            ');&#10;')"/>
    </xsl:template>

    <xsl:template match="*" mode="Screen-item-update-cache">
        <xsl:call-template name="error-unexpected"/>
    </xsl:template>


    <!--
        Init item value from cache.
    -->
    <xsl:function name="uig:Screen-item-init-from-cache" as="xs:string">
        <xsl:param name="e" as="element()"/>
        <xsl:value-of>
            <xsl:value-of select="
                concat('if (', uig:Screen-item-variable-name($e), ' != null) {&#10;')"/>
            <xsl:call-template name="add-indentation">
                <xsl:with-param name="text">
                    <xsl:apply-templates select="$e" mode="Screen-item-init-from-cache"/>
                </xsl:with-param>
                <xsl:with-param name="ident" select="'    '"/>
            </xsl:call-template>
            <xsl:text>}&#10;</xsl:text>
        </xsl:value-of>
    </xsl:function>

    <xsl:template match="screen/text-field" mode="Screen-item-init-from-cache">
        <xsl:value-of select="concat(
            'Screen.setTextFieldValue(',
            uig:Screen-item-variable-name(.),
            ', ', uig:Screen-item-variable-name(.),
            '_cached_value);&#10;')"/>
    </xsl:template>

    <xsl:template match="screen/progress" mode="Screen-item-init-from-cache">
        <xsl:value-of select="concat(
            'Screen.setProgressValues(',
            uig:Screen-item-variable-name(.),
            ', ',
            uig:Screen-item-variable-name(.),
            '_cached_value, ',
            uig:Screen-item-variable-name(.),
            '_cached_max);&#10;')"/>
    </xsl:template>

    <xsl:template match="*" mode="Screen-item-init-from-cache">
        <xsl:call-template name="error-unexpected"/>
    </xsl:template>


    <!--
        Init item cache value.
    -->
    <xsl:function name="uig:Screen-item-init-cache" as="xs:string">
        <xsl:param name="e" as="element()"/>
        <xsl:value-of>
            <xsl:apply-templates select="$e" mode="Screen-item-init-cache"/>
        </xsl:value-of>
    </xsl:function>

    <xsl:template match="screen/text-field" mode="Screen-item-init-cache">
        <xsl:value-of select="concat(
            uig:Screen-item-variable-name(.), '_cached_value = value;&#10;')"/>
    </xsl:template>

    <xsl:template match="screen/progress" mode="Screen-item-init-cache">
        <xsl:value-of select="concat(
            uig:Screen-item-variable-name(.), '_cached_value = value;&#10;')"/>
        <xsl:value-of select="concat(
            uig:Screen-item-variable-name(.), '_cached_max = max;&#10;')"/>
    </xsl:template>

    <xsl:template match="*" mode="Screen-item-init-cache">
        <xsl:call-template name="error-unexpected"/>
    </xsl:template>




    <!--
        ==========================================================
                        Toolkit specifics.
        ==========================================================
    -->
    <xsl:template match="screen" mode="Screen-imports"/>

    <xsl:template match="screen" mode="Screen-ctor-body"/>

    <xsl:template match="screen" mode="Screen-define-initializer-signature">
        <xsl:call-template name="error-not-implemented"/>
    </xsl:template>

    <xsl:template match="screen" mode="Screen-define-initializer-footer"/>

    <xsl:template match="*" mode="Screen-item-variable-type-impl">
        <xsl:text>Object</xsl:text>
    </xsl:template>

    <xsl:template match="options" mode="Screen-options-get-selected-item">
        <xsl:call-template name="error-not-implemented"/>
    </xsl:template>

    <xsl:template match="*" mode="Screen-add-item-impl">
        <xsl:call-template name="error-not-implemented"/>
    </xsl:template>

    <xsl:template match="*" mode="Screen-create-item">
        <xsl:call-template name="error-not-implemented"/>
    </xsl:template>

</xsl:stylesheet>
