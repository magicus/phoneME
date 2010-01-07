<?xml version="1.0" ?>
<!--
Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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

    <xsl:output
        method="text"
        omit-xml-declaration="yes"
        encoding="utf8"/>

    <xsl:param name="package-name"/>

    <xsl:param name="output-java-dir"   select="translate($package-name,'.','/')"/>
    <xsl:param name="output-xml-dir"    select="."/>
    <xsl:param name="with-unit-tests"   select="'yes'"/>


    <xsl:template match="/*">
        <xsl:apply-templates select="." mode="main"/>
    </xsl:template>


    <xsl:template match="*" mode="main">
        <!--
            This is just a check to ensure that proper XSLT2.0 complient processor runs.
            The check relays on the fact that current-date() is not a part of XSLT1.0 spec.

            Saxon is free XSLT2.0 complient processor (http://saxon.sourceforge.net/).
            By default Ant uses Xalan, so you need to twik it: copy Saxon jars into $ANT_HOME/lib dir.
        -->
        <xsl:if test="current-date()"/>

        <xsl:call-template name="top-I18N"/>
        <xsl:call-template name="top-Screen"/>
        <xsl:if test="$with-unit-tests='yes'">
            <xsl:call-template name="top-UTest"/>
        </xsl:if>
    </xsl:template>


    <xsl:include href="utils.xsl"/>
    <xsl:include href="traits.xsl"/>
    <xsl:include href="i18n.xsl"/>
    <xsl:include href="screen.xsl"/>
    <xsl:include href="utest.xsl"/>


    <xsl:template match="*">
        <xsl:call-template name="error-unexpected"/>
    </xsl:template>

    <xsl:template match="text()"/>

</xsl:stylesheet>
