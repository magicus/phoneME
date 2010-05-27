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


<!-- 
    Pass 1 templates
-->

<!-- stylesheet parameter: space separated list of file names to merge -->
<xsl:param name="filesList"></xsl:param>

<xsl:template match="/">
        <xsl:element name="configuration">
            <!-- process filesList if it isn't empty -->
            <xsl:if test="boolean($filesList)">
                <xsl:call-template name="processFiles">
                    <xsl:with-param name="filesList" select="$filesList"/>
                </xsl:call-template>
            </xsl:if>
        </xsl:element>
</xsl:template>

<!-- process list of XML files names -->
<xsl:template name="processFiles">
<!-- template parameter: space separated list of file names -->
<xsl:param name="filesList"/>
    <!-- get first file name from the list -->
    <xsl:variable name="fileName">
        <xsl:choose>
            <!-- when there is more than one element in the list -->
            <xsl:when test="contains($filesList,' ')">
                <xsl:value-of select="substring-before($filesList,' ')"/>
            </xsl:when>
            <xsl:otherwise>
                <xsl:value-of select="$filesList"/>
            </xsl:otherwise>
        </xsl:choose>
    </xsl:variable>
    <!-- process it -->
    <xsl:call-template name="processFile">
        <xsl:with-param name="fileName" select="$fileName"/>
    </xsl:call-template>
    <!-- and call this template recursively to process rest of file names -->
    <xsl:if test="contains($filesList,' ')">
        <xsl:call-template name="processFiles">
            <xsl:with-param name="filesList" select="substring-after($filesList,' ')"/>
        </xsl:call-template>
    </xsl:if>
</xsl:template>

<!-- process single file name -->
<xsl:template name="processFile">
<!-- template parameter: name of XML file to process -->
<xsl:param name="fileName"/>
    <!-- copy all children of /configuration node to the output -->
    <xsl:copy-of select="document($fileName)/configuration/child::*"/>
</xsl:template>

</xsl:stylesheet>
