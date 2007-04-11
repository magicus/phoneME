<?xml version="1.0" encoding="UTF-8"?>
<!--
          

        Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved.
        DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
-->
<!--
    This stylesheet outputs names of the packages for which constants
    definitions should be generated.
-->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method="text"/>

<xsl:template match="/">

<!-- for each node corresponding to package -->
<xsl:for-each 
    select="/configuration/constants/constant_class | 
        /configuration/localized_strings">
<xsl:variable name="package" select="concat(@Package,'.',@Name)"/>
<!-- output package name -->
<xsl:value-of select="@Package"/>.<xsl:value-of select="@Name"/>
<xsl:text> 
</xsl:text>
</xsl:for-each>

</xsl:template>
</xsl:stylesheet>
