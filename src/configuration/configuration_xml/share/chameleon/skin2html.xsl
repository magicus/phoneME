<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet version="1.0"
xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<!--
          

        Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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
        This stylesheet is used for presenting nicely formatted
        skin XML file in Web browser. It assumes that skin resource,
        like images, are stored in default location.
-->

<xsl:variable name="skinResourcesDir" select="'../../../highlevelui/lcdlf/lfjava/resource/skin/'"/>
<xsl:variable name="constfile" select="document('skin_constants.xml')"/>

<xsl:template match="/">
<html>

<!-- <link href="./styles.css" rel="stylesheet" type="text/css" /> -->

<!-- 
    We need a way to show that an image is taking precedence
    over a color, and thus mark the color as "not currently used".  
    E.g SCREEN_IMAGE_BG overrides SCREEN_COLOR_BG
-->

<body bgcolor="#F3FAE9">
<h1>Adaptive User Interface Technology</h1>

<i>Tip: Hover over Name to see Description</i>

<p></p>

<table border="1">
    <tr bgcolor="#5382A1">
        <th title="Hello World"><font color="#FFFFFF">Color Name</font></th>
        <th><font color="#FFFFFF">Value</font></th>
        <th><font color="#FFFFFF">Color</font></th>
    </tr>
<xsl:for-each select="configuration//integer">
    <xsl:if test="substring(@Value, 1, 2)='0x'">
    <tr>
        <xsl:call-template name="tooltip">
            <xsl:with-param name="lookup" select="@Key"/>
        </xsl:call-template>

        <td><xsl:value-of select="@Value"/></td> 
        <td> 
        <xsl:attribute name="BGCOLOR">
            <xsl:value-of select="concat('#',substring(@Value,3,8))" />
        </xsl:attribute> 
    color
        </td>
    </tr>
    </xsl:if>
</xsl:for-each>
</table>

<p> </p>

<table border="1">
    <tr bgcolor="#B2BC00">
        <th>Integer Name</th>
        <th>Value</th>
    </tr>
<xsl:for-each select="configuration//integer">
    <xsl:choose>
        <xsl:when test="substring(@Value, 1, 2)='0x'">     
        </xsl:when>
        <xsl:otherwise>
    <tr>
            <xsl:call-template name="tooltip">
                <xsl:with-param name="lookup" select="@Key"/>
            </xsl:call-template>
        <td><xsl:value-of select="@Value"/></td> 
    </tr>
        </xsl:otherwise>
    </xsl:choose>
</xsl:for-each>
</table>

<p> </p>

<table border="1">
    <tr bgcolor="#B2BC00">
        <th>String Name</th>
        <th>Value</th>
    </tr>

<xsl:for-each select="configuration//string">
    <tr>
    <xsl:call-template name="tooltip">
        <xsl:with-param name="lookup" select="@Key"/>
    </xsl:call-template>
        <td><xsl:value-of select="@Value"/></td> 
    </tr>
</xsl:for-each>
</table>

<p> </p>

<table border="1">
    <tr bgcolor="#B2BC00">
        <th>Font Name</th>
        <th>Value</th>
    </tr>

<xsl:for-each select="configuration//font">
    <tr>
    <xsl:call-template name="tooltip">
        <xsl:with-param name="lookup" select="@Key"/>
    </xsl:call-template>
        <td><xsl:value-of select="@Value"/></td>
    </tr>
</xsl:for-each>

</table>

<p> </p>

<table border="1">
    <tr bgcolor="#FFC726">
        <th>Name</th>
        <th>File Name</th>
        <th>Image</th>
        <th>Romized</th>
    </tr>

<xsl:for-each select="configuration//image">
    <tr>
    <xsl:call-template name="tooltip">
        <xsl:with-param name="lookup" select="@Key"/>
    </xsl:call-template>

        <td><xsl:value-of select="@Value"/></td> 
        <td> 
            <img>
            <xsl:attribute name="src">
                <xsl:value-of select="concat($skinResourcesDir,@Value,'.png')"/>
            </xsl:attribute>
            </img> 
        </td>
        <td>
        <xsl:if test="@Romized='true'"> 
            <b><xsl:value-of select="@Romized"/>* </b>
        </xsl:if>
        <xsl:if test="@Romized='false'">
            <xsl:value-of select="@Romized"/>
        </xsl:if>
        </td>
    </tr>
</xsl:for-each>
</table>

<p> </p>

<!-- 
    Composite image XML is missing usage information, for example,
    are the images in horizontal sequence like a bar or in a 3x3 block
    like a button composite, or a toggle sequence like a checkbox or a
    repeated animation like a busy cursor. We should consider adding
    an attribute such as 
        layout=replace|horizontal|vertical|table
        rows=  cols=
 -->

<table border="1">
    <tr bgcolor="#FFC726">
        <th>Composite Image</th>
        <th>File Name</th>
        <th>Pieces</th>
        <th>Image</th>
       <th>Romized</th>
    </tr>

<xsl:for-each select="configuration//composite_image">
    <xsl:if test="@Value != ''">
    <tr>
        <xsl:call-template name="tooltip">
            <xsl:with-param name="lookup" select="@Key"/>
        </xsl:call-template>

       <td><xsl:value-of select="@Value"/></td> 
       <td><xsl:value-of select="@Pieces"/></td> 

        <td>
        <xsl:if test="@Pieces=9">
            <xsl:call-template name="compositeImagePieces">
                <xsl:with-param name="from" select="0"/>
                <xsl:with-param name="to" select="2"/>
            </xsl:call-template>
            <br></br>
            <xsl:call-template name="compositeImagePieces">
                <xsl:with-param name="from" select="3"/>
                <xsl:with-param name="to" select="5"/>
            </xsl:call-template>
            <br></br>
            <xsl:call-template name="compositeImagePieces">
                <xsl:with-param name="from" select="6"/>
                <xsl:with-param name="to" select="8"/>
            </xsl:call-template>
        </xsl:if>

        <xsl:if test="@Pieces=2">
            <xsl:call-template name="compositeImagePieces">
                <xsl:with-param name="from" select="0"/>
                <xsl:with-param name="to" select="1"/>
            </xsl:call-template>
        </xsl:if>

        <xsl:if test="@Pieces=3">
            <xsl:call-template name="compositeImagePieces">
                <xsl:with-param name="from" select="0"/>
                <xsl:with-param name="to" select="2"/>
            </xsl:call-template>
        </xsl:if>

        <xsl:if test="@Pieces=7">
            See Below
        </xsl:if>

        <xsl:if test="@Pieces=17">
            See Below
        </xsl:if>
        </td>
    
        <td>     
            <xsl:if test="@Romized='true'"> 
            <b><xsl:value-of select="@Romized"/>* </b>
            </xsl:if>
            <xsl:if test="@Romized='false'">
            <xsl:value-of select="@Romized"/>
            </xsl:if>
        </td>
    </tr>
    </xsl:if>
</xsl:for-each>
</table>

<p> </p>

<table border="1">
    <tr bgcolor="#FFC726">
        <th>File Name</th>
        <th>Image</th>
        <th>Romized</th>
    </tr>

<xsl:for-each select="configuration//composite_image">
    <xsl:choose>
        <xsl:when test="@Value=''">
            <!-- do nothing -->
        </xsl:when>
        <xsl:otherwise>
            <xsl:call-template name="strip">
                <xsl:with-param name="count" select="@Pieces"/>
            </xsl:call-template>
        </xsl:otherwise>
    </xsl:choose>
</xsl:for-each>
</table>

<p> </p>

<table border="1">
    <tr bgcolor="#FFC726">
        <th>Integer Sequence</th>
        <th>Value</th>
    </tr>

<xsl:for-each select="configuration//integer_seq">
    <tr>
    <xsl:call-template name="tooltip">
        <xsl:with-param name="lookup" select="@Key"/>
    </xsl:call-template>
        <td><xsl:value-of select="@Value"/></td> 
    </tr>
</xsl:for-each>
</table>

<p></p>
<i> * If Romized="true", then changes to image cannot be previewed using the "make skin_romization" target. Either re-build source code or set Romized="false".</i>

<p></p>
<hr></hr>
<p></p>

</body>
</html>
</xsl:template>

<xsl:template name="strip">  
<xsl:param name="count" select="1"/>
    <xsl:if test="$count > 0">
        <xsl:call-template name="strip">
            <xsl:with-param name="count" select="$count - 1"/>
        </xsl:call-template>
    <tr>
        <td><xsl:value-of select="concat(@Value,$count - 1,'.png')"/> </td> 
        <td>
            <img>
            <xsl:attribute name="src"> <xsl:value-of select="concat($skinResourcesDir,@Value,$count - 1,'.png')"/>
            </xsl:attribute>
            </img>
        </td>
        <td>
        <xsl:if test="@Romized='true'"> 
            <b><xsl:value-of select="@Romized"/>* </b>
        </xsl:if>
        <xsl:if test="@Romized='false'">
            <xsl:value-of select="@Romized"/>
        </xsl:if>
        </td>
    </tr>
    </xsl:if>  
</xsl:template>


<xsl:template name="tooltip">  
<xsl:param name="lookup"/>
    <td><a>
    <xsl:attribute name="href">javascript: alert("
<xsl:value-of select="normalize-space($constfile//constant[@Name=$lookup]/@Comment)"/>
"); </xsl:attribute>
<!--
    <xsl:attribute name="title">
        <xsl:value-of select="normalize-space($constfile//constant[@Name=$lookup]/@Comment)"/>
    </xsl:attribute>
-->
    <xsl:value-of select="@Key" />
</a></td>
</xsl:template>

<!-- 
    Use this "description" template only if you prefer 
    a table cell for description
    instead of a tooltip 
-->
<xsl:template name="description">  
<xsl:param name="lookup"/>
    <td>
<xsl:value-of select="normalize-space($constfile//constant[@Name=$lookup]/@Comment)"/>
    </td>
</xsl:template>

<xsl:template name="compositeImagePieces">
<xsl:param name="from" />
<xsl:param name="to" />
    <xsl:if test="$from &lt;= $to">
    <img>
        <xsl:attribute name="src"> <xsl:value-of select="concat($skinResourcesDir,@Value,$from,'.png')"/>
        </xsl:attribute>
    </img>
        <xsl:call-template name="compositeImagePieces">
            <xsl:with-param name="from" select="$from+1"/>
            <xsl:with-param name="to" select="$to"/>
        </xsl:call-template>
    </xsl:if>
</xsl:template>

</xsl:stylesheet>
