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
    <!-- -->
    <xsl:variable name="pass1Results">
        <xsl:element name="configuration">
            <!-- process filesList if it isn't empty -->
            <xsl:if test="boolean($filesList)">
                <xsl:call-template name="processFiles">
                    <xsl:with-param name="filesList" select="$filesList"/>
                </xsl:call-template>
            </xsl:if>
        </xsl:element>
    </xsl:variable>        

    <xsl:variable name="pass2Results">
        <xsl:apply-templates select="xalan:nodeset($pass1Results)" 
            mode="joinClasses"/>
    </xsl:variable>            

    <xsl:variable name="pass3Results">
        <xsl:apply-templates select="xalan:nodeset($pass2Results)/configuration" 
            mode="generateAutoValues"/>
    </xsl:variable>

    <xsl:apply-templates select="xalan:nodeset($pass3Results)/configuration" 
        mode="assignKeysValues"/>
    
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



<!-- 
    Pass 2 templates
-->

<!-- nodes which map to Java class indexed by class name -->
<xsl:key name="classesNodes" 
    match="/configuration/constants/constant_class | 
           /configuration/localized_strings" 
    use="concat(@Package,'.',@Name)"/>

<!-- join nodes which map to same class to one node -->
<xsl:template match="constant_class | localized_strings" mode="joinClasses">
    <!-- name of the class to which this node maps to -->
    <xsl:variable name="className" select="concat(@Package,'.',@Name)"/>
    <!-- if we havent seen this class name yet -->
    <xsl:if test="generate-id()=generate-id(key('classesNodes', $className)[1])">
        <!-- output matched node -->
        <xsl:copy>
            <!-- output all matched node attributes -->
            <xsl:copy-of select="@*"/>
            <!-- for each child of nodes with same class name -->
            <xsl:for-each select="key('classesNodes', $className)/child::*">
                <!-- output child node -->
                <xsl:apply-templates select="." mode="joinClasses"/>
            </xsl:for-each>
        </xsl:copy>
    </xsl:if>        
</xsl:template>

<!-- copy all other nodes or attributes to the output -->
<xsl:template match="@* | node()" mode="joinClasses">
    <xsl:copy>
        <xsl:apply-templates select="@* | node()" mode="joinClasses"/>
    </xsl:copy>
</xsl:template>



<!-- 
    Pass 3 templates
-->

<!-- assign AutoValues -->
<xsl:template match="constant_class[@AutoValue='true']" 
    mode="generateAutoValues">
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
           /configuration/skin/skin_properties/child::*"
    use="concat(../@Package,'.',../@Name,':',../@KeysClass,'.',@Key)"/>

<xsl:template match="node()[@KeysClass != '']" 
    mode="assignKeysValues">

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

