<?xml version="1.0" encoding="UTF-8"?>
<!--
          

        Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved.
        DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
-->
<!--
    This stylesheet outputs constants definitions for specified package 
    in form of Java class
-->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<!-- stylesheet parameter: name of the package to generate constants for -->
<xsl:strip-space elements="*"/>
<xsl:output method="text"/>
<xsl:template match="/">
<!-- constant_class nodes with given package name -->
<xsl:for-each select="/configuration/constants/constant_class">
<xsl:value-of select="concat(@Package,'.',@Name,' ')"/>
</xsl:for-each>
</xsl:template>
</xsl:stylesheet>
