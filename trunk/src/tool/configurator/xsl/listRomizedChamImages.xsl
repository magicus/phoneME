<?xml version="1.0" encoding="UTF-8"?>
<!--
          

        Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved.
        DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
-->

<!--
    This stylesheet outputs the file names of romized Chameleon images
-->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method="text"/>

<xsl:template match="/">

<!-- for each image property node -->
<xsl:for-each 
select="/configuration/skin/skin_properties/image[@Romized='true']">
    <!-- file name: only .png images can be romized currently -->
    <xsl:variable name="fileName" select="concat(@Value,'.png')"/>
<!-- output file name -->
<xsl:value-of select="$fileName"/>
<xsl:text> 
</xsl:text>
</xsl:for-each>

<!-- for each composite image property node -->
<xsl:for-each 
select="/configuration/skin/skin_properties/composite_image[@Romized='true']">
    <!-- image file name prefix -->
    <xsl:variable name="baseFileName" select="@Value"/>
    <!-- number of image files in composite image -->
    <xsl:variable name="totalImages" select="@Pieces"/>

    <!-- output file names -->
    <xsl:call-template name="listCompositeImage">
        <xsl:with-param name="baseFileName" select="$baseFileName"/>
        <xsl:with-param name="imageIdx" select="$totalImages - 1"/>
    </xsl:call-template>
</xsl:for-each>
</xsl:template>


<!--
    This template prints names of image files for composite image
-->
<xsl:template name="listCompositeImage">
<!-- template parameter: image file name prefix -->
<xsl:param name="baseFileName"/>
<!-- template parameter: image file number -->
<xsl:param name="imageIdx"/>

<xsl:if test="$imageIdx >= 0">
    <!-- file name: only .png images can be romized currently -->
    <xsl:variable name="fileName" 
    select="concat($baseFileName,$imageIdx,'.png')"/>
<!-- output file name -->
<xsl:value-of select="$fileName"/>
<xsl:text> 
</xsl:text>
    <xsl:call-template name="listCompositeImage">
        <xsl:with-param name="baseFileName" select="$baseFileName"/>
        <xsl:with-param name="imageIdx" select="$imageIdx - 1"/>
    </xsl:call-template>
</xsl:if>

</xsl:template>

</xsl:stylesheet>
