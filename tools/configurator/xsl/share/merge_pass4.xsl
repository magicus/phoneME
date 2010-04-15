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


<!-- stylesheet parameter: space separated list of file names to merge -->

<xsl:template match="/">
    <xsl:apply-templates select="." mode="assignKeysValues"/>
</xsl:template>

<!-- 
    Pass 4 templates
-->

<!-- nodes providing keys values, indexed by fully qualified keys names -->
<xsl:key name="keysValuesNodes" 
    match="/configuration/constants/
        constant_class[@KeysValuesProvider='true']/constant"
    use="concat(../@Package,'.',../@Name,'.',@Name)"/>
    
<!-- 
    nodes that use keys (refer to them), indexed by fully 
    qualified keys names 
--> 
<xsl:key name="keysUsersNodes"
    match="/configuration/localized_strings/child::* | 
           /configuration/skin/skin_properties/child::* |
           /configuration/keyboards/keyboard/child::*"
    use="concat(../@Package,'.',../@Name,':',../@KeysClass,'.',@Key)"/>

<xsl:template match="node()[@KeysClass != '']" mode="assignKeysValues">

    <xsl:variable name="nodeName" select="name(.)"/>
    <xsl:variable name="nodePackage" select="./@Package"/>
    <xsl:variable name="nodeClass" select="./@Name"/>

    <!-- name of the constants class that provides keys values -->
    <xsl:variable name="keysClassName" select="@KeysClass"/>

    
    <!-- 
       Do some error checking: make sure that we got 
       one to one relationship there
    -->
    
    <!-- nodes providing keys values for this relationship -->
    <xsl:variable name="keysValuesNodes" 
        select="/configuration/constants/constant_class[$keysClassName=concat(
        @Package,'.',@Name)]/constant"/>

    <!-- for each key -->
    <xsl:for-each select="$keysValuesNodes">
        <!-- key name -->
        <xsl:variable name="keyName" select="concat($keysClassName,'.',@Name)"/>
        
        <!-- nodes that use this key (refer to it) -->
        <xsl:variable name="keyUsersNodes" 
        select="key('keysUsersNodes', concat($nodePackage,'.',$nodeClass,':',$keyName))"/>
    
        <!-- error: this key is not used -->
        <xsl:if test="count($keyUsersNodes)=0">
            <xsl:message terminate="yes">
Merging error: Key '<xsl:value-of select="$keyName"/>' 
is unused in '<xsl:value-of select="$nodeName"/>'
            </xsl:message>
        </xsl:if>

        <!-- error: this key is used more than once -->
        <xsl:if test="count($keyUsersNodes)>1">
            <xsl:message terminate="yes">
Merging error: Key '<xsl:value-of select="$keyName"/>' 
is used more than once in '<xsl:value-of select="$nodeName"/>'
            </xsl:message>
        </xsl:if>           
    </xsl:for-each>
    
    <xsl:copy>
        <xsl:copy-of select="@*"/>

        <!-- for each node that use keys -->
        <xsl:for-each select="child::*">
            <!-- name of the key this node refers to -->
            <xsl:variable name="keyName" 
                select="concat($keysClassName,'.',@Key)"/>
        
            <!-- constant nodes providing value for this key -->
            <xsl:variable name="keyValueNodes" 
                select="key('keysValuesNodes', $keyName)"/>
    
            <!-- error: there is no constant corresponding to this key -->
            <xsl:if test="count($keyValueNodes)=0">
                <xsl:message terminate="yes">
Merging error: key '<xsl:value-of select="@Key"/>' has no corresponding constant in 
'<xsl:value-of select="$keysClassName"/>' 
                </xsl:message>
            </xsl:if>

            <!-- error: there is more than one constant corresponding to this key -->
            <xsl:if test="count($keyValueNodes)>1">
                <xsl:message terminate="yes">
Merging error: there is more than one constant in '<xsl:value-of select="$keysClassName"/>'
corresponding to key '<xsl:value-of select="@Key"/>'
                </xsl:message>
            </xsl:if>
  
            <!-- key value -->
            <xsl:variable name="keyValue" select="$keyValueNodes[1]/@Value"/>
    
            <xsl:copy>
                <!-- output generated 'ValueIndex' attribute -->
                <xsl:attribute name="KeyValue">
                    <xsl:value-of select="$keyValue"/>
                </xsl:attribute>
                <!-- output all attributes -->
                <xsl:copy-of select="@*" />  
            </xsl:copy>
        </xsl:for-each>
    </xsl:copy>
</xsl:template>

<!-- copy all other nodes or attributes to the output -->
<xsl:template match="@* | node()" mode="assignKeysValues">
    <xsl:copy>
        <xsl:apply-templates select="@* | node()" mode="assignKeysValues"/>
    </xsl:copy>
</xsl:template>


</xsl:stylesheet>
