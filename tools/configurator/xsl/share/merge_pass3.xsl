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
    This stylesheet merges several configuration XML files together.
    Merging is done in several passes, where output XML tree from 
    previous pass is used as input XML tree for the next pass.
    Currently, there are following passes:

    Pass 1: Merge trees from several input files into single tree.
    
    Pass 2: For nodes which map to Java class, like 'constant_class' 
    and 'localized_strings' nodes, ensure that for each class name 
    there is only one such node. If there are several nodes which map 
    to same class, then leave only one such node in output tree
    and hook all childs from other nodes with same class name to that
    single node.

    Pass 3: Generate AutoValues for 'constant_class' nodes which have
    'AutoValue' attribute with value 'true'.

    Pass 4: Assign numeric values to localized strings. More specifically,
    each localized string (represented by 'localized_string' node) has
    a key associated with it (represented by 'Key' attribute). Also, there
    is special constants class, with localized strings keys as constants 
    names. So, for each locale (represented by 'localized_strings' node), 
    there is one-to-one relationship between localized strings and constants
    from this special constants class. Assigning numeric value to localized
    string means finding the constant corresponding to this localized string,
    and assigning constant's value to 'ValueIndex' attribute of localized
    string. Besides assigning numeric values, this pass also checks that 
    one-to-one relationship described above isn't broken.
-->
<xsl:stylesheet version="1.0" 
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:xalan="http://xml.apache.org/xalan">

<xsl:output method="xml" indent="yes" />


<xsl:template match="/">
        <xsl:apply-templates select="." mode="generateAutoValues"/>
</xsl:template>

<!-- 
    Pass 3 templates
-->

<!-- assign AutoValues -->
<xsl:template match="constant_class[@AutoValue='true']" mode="generateAutoValues">
    <xsl:copy>
        <!-- output all attributes -->
        <xsl:copy-of select="@*"/>
        <xsl:for-each select="constant">
            <xsl:copy>
                <!-- output generated 'Value' attribute -->
                    <xsl:attribute name="Value">
                        <!-- the value is position of node -->
                        <xsl:value-of select="position()-1"/>
                    </xsl:attribute>
                <!-- output all other attributes -->
                <xsl:copy-of select="@*[local-name() != 'Value']" />
            </xsl:copy>
        </xsl:for-each>
    </xsl:copy>
</xsl:template>

<!-- copy all other nodes or attributes to the output -->
<xsl:template match="@* | node()" mode="generateAutoValues">
    <xsl:copy>
        <xsl:apply-templates select="@* | node()" mode="generateAutoValues"/>
    </xsl:copy>
</xsl:template>

</xsl:stylesheet>
