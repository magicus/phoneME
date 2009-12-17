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
    THIS STYLESHEET IS OBSOLETE. Do not use or modify it. 
    Its still here for backward compatibility only and will be 
    removed at some time in the future.
    It has been split into several stylesheets - merge_pass1.xsl, 
    merge_pass2.xsl and so on, one stylesheet per one pass. If you 
    want to modify or add some pass, edit these stylesheets instead.
-->

<xsl:stylesheet version="1.0" 
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:xalan="http://xml.apache.org/xalan">

<xsl:output method="xml" indent="yes" />

<!-- Include our new stylesheets -->
<xsl:include href="merge_pass1.xsl"/>
<xsl:include href="merge_pass2.xsl"/>
<xsl:include href="merge_pass3.xsl"/>
<xsl:include href="merge_pass4.xsl"/>


<!-- 
    Merge
-->

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

</xsl:stylesheet>

