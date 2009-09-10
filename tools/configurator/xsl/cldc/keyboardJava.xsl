<?xml version="1.0" encoding="UTF-8"?>
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
<!--
    This stylesheet outputs constants definitions for specified package
    in form of Java class
-->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<!-- stylesheet parameter: name of the package to generate constants for -->
<xsl:strip-space elements="*"/>
<xsl:output method="text"/>
<!-- for pretty printing the comments -->
<xsl:include href="prettyPrint.xsl"/>


<xsl:template match="/">
<!--<xsl:template match="node()[../@KeysClass != $packageName]">-->

<!-- nodes providing keys values for this relationship -->
    <xsl:variable name="keyboardsNodes"
        select="/configuration/keyboards/keyboard"/>
/**
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt).
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions.
 *
 * These are constant defines both in native and Java layers.
 * NOTE: DO NOT EDIT. THIS FILE IS GENERATED. If you want to
 * edit it, you need to modify the corresponding XML files.
 *
 * Patent pending.
 */
package com.sun.midp.chameleon.keyboards;

import com.sun.midp.lcdui.Key;
import com.sun.midp.lcdui.VirtualKeyboard;
import java.util.Hashtable;
    
public class Keyboards {
    static Hashtable keyboards;
<!-- for each key -->
    private static void createKeyboardMap() {

        keyboards = new Hashtable();
    <xsl:for-each select="$keyboardsNodes">
        <xsl:variable name="currentKeyboard" select="@Name"/>
            keyboards.put(VirtualKeyboard.<xsl:value-of select="$currentKeyboard"/>, new Key[][] {
            <xsl:variable name="linesNodes"
            select="/configuration/keyboards/keyboard/line[../@Name=$currentKeyboard]"/>
                <xsl:for-each select="$linesNodes">
                    <xsl:variable name="currentLine" select="@Num"/>
                    <xsl:if test="@Num!='0'">
                            <xsl:text> , </xsl:text>
                        </xsl:if>
                    {
                    <xsl:variable name="buttonsNodes"
                        select="/configuration/keyboards/keyboard/line/button[../../@Name=$currentKeyboard][../@Num=$currentLine]"/>
                    <xsl:for-each select="$buttonsNodes">
                        <xsl:if test="@KeyValue!='0'">
                            <xsl:text> , </xsl:text>
                        </xsl:if>
                        new Key(<xsl:text>'</xsl:text><xsl:value-of select="@Key"/><xsl:text>'</xsl:text>,<xsl:value-of select="@X"/>, <xsl:value-of select="@Y"/>, Key.<xsl:value-of select="@KeyImageType"/> )
                    </xsl:for-each> }
                </xsl:for-each>
            }
            );
    </xsl:for-each>
    }

    public static Hashtable getKeyboards() {
        createKeyboardMap();
        return keyboards;
    }
}
</xsl:template>
</xsl:stylesheet>
