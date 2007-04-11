<?xml version="1.0" encoding="UTF-8"?>
<!--
           

        Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved.
        DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
-->
<!--
    This stylesheet overrides constants values in input file 
    by values from file specified as parameter
-->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method="xml" indent="yes"/>

<!-- stylesheet parameter: name of file with overrides -->
<xsl:param name="overrideFile"></xsl:param>

<!-- map from constant name to constant node -->
<xsl:key 
    name="constantOverrides" 
    match="/configuration/constants/constant_class/constant" 
    use="@Name"/>

<xsl:template match="@* | node()">
    <xsl:copy>
        <xsl:apply-templates select="@* | node()" />
    </xsl:copy>
</xsl:template>

<xsl:template match="constant">
    <!-- constant's name  -->
    <xsl:variable name="constantName" select="@Name"/>

    <!-- lookup for overridden value  -->
    <xsl:variable name="newValue">
        <xsl:for-each select="document($overrideFile)">
            <xsl:value-of select="key('constantOverrides', $constantName)[1]/@Value"/>
        </xsl:for-each>
    </xsl:variable>

    <!-- output 'Value' attribute -->
    <xsl:copy>
        <!-- check if value has been overridden -->
        <xsl:choose>
            <!-- if so, output new value -->
            <xsl:when test="string-length($newValue)">
                <xsl:attribute name="Value">
                    <xsl:value-of select="$newValue"/>
                </xsl:attribute>
            </xsl:when>
            <!-- otherwise output original value -->
            <xsl:otherwise>
                <xsl:attribute name="Value">
                    <xsl:value-of select="@Value"/>
                </xsl:attribute>
            </xsl:otherwise>
        </xsl:choose>
        
        <!-- output all other attributes -->
        <xsl:copy-of select="@*[local-name() != 'Value']" />
   </xsl:copy>
</xsl:template>

</xsl:stylesheet>

