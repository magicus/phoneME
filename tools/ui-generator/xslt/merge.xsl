<?xml version="1.0" ?>
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

<xsl:stylesheet version="2.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:xs="http://www.w3.org/2001/XMLSchema"
                xmlns:uig="foo://sun.me.ui-generator.net/">


    <xsl:output method="xml" encoding="utf8" indent="yes"/>

    <xsl:template match="/*">
        <xsl:copy>
            <xsl:apply-templates select="*"/>
        </xsl:copy>
    </xsl:template>

    <xsl:template match="include">
        <xsl:apply-templates select="document(@href)/screens/*"/>
    </xsl:template>

    <xsl:template match="@*|node()|comment()">
        <xsl:copy>
            <xsl:apply-templates select="@*|node()|comment()"/>
        </xsl:copy>
  </xsl:template>
</xsl:stylesheet>
