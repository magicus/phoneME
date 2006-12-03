<?xml version="1.0" encoding="UTF-8"?> 
<!--
          

        Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved.
        DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
-->
<!--
    This stylesheet outputs properties with given scope in form of
    "name: value" lines
-->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method="text"/>
<!-- stylesheet parameter: properties scope -->
<xsl:param name="propsScope">error</xsl:param>

<xsl:template match="/">
<!-- for each property with given scope and non-empty value -->
<xsl:for-each select="/configuration/properties/property[@Scope=$propsScope and @Value != '']">
<!-- output "name: value" pair -->
<xsl:value-of select="@Key"/>: <xsl:value-of select="@Value"/>
<xsl:text>
</xsl:text>
</xsl:for-each>
</xsl:template>
</xsl:stylesheet>
