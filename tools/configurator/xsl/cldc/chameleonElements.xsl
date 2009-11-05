<?xml version="1.0" encoding="UTF-8"?>
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
<!--
    This stylesheet outputs constants definitions for specified package
    in form of Java class
-->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<!-- stylesheet parameter: name of the package to generate constants for -->
<xsl:strip-space elements="*"/>
<xsl:output method="text"/>
<!-- for pretty printing the comments -->
<xsl:include href="prettyPrint.xsl"/>

<xsl:template match="/">

  /**
  * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
  * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
  *
  * This program is free software; you can redistribute it and/or
  * modify it under the terms of the GNU General Public License version
  * 2 only, as published by the Free Software Foundation.
  *
  * This program is distributed in the hope that it will be useful, but
  * WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  * General Public License version 2 for more details (a copy is
  * included at /legal/license.txt).
  *
  * You should have received a copy of the GNU General Public License
  * version 2 along with this work; if not, write to the Free Software
  * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  * 02110-1301 USA
  *
  * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
  * Clara, CA 95054 or visit www.sun.com if you need additional
  * information or have any questions.
  *
  * These are constant defines both in native and Java layers.
  * NOTE: DO NOT EDIT. THIS FILE IS GENERATED. If you want to
  * edit it, you need to modify the corresponding XML files.
  *
  * Patent pending.
  */

  package com.sun.midp.chameleon.skins.resources;

  import javax.microedition.theme.Element;
  import java.util.Hashtable;

  /**
  * Contains mapping of chameleon skin elemenets to the customizable base
  * and toolkit elements. Is generated from xml at build time.
  * Used by SkinLoader to determin customizable element parameters for
  * particular chameleon resource ID.
  */
  public class ChameleonVsElements {

  /* MIDP2.0 LCDUI toolkit name */
  private static final String LCDUI_TOOLKIT_NAME = "midp20lcdui";

  /* Internal classes for chameleon elements */

  /**
  * Class that represents mapping of particular chameleon resource
  * to the customizable base or toolkit element.
  */
  static class ChamElementInfo {

  /* Сustomizable element feature */
  String feature;

  /* Сustomizable element role */
  String role;

  /**
  *  Toolkit name this element elongs to.
  *  If null, it is considered to be base element.
  */
  String toolkitName;

  /**
  *  constructs toolkit element info.
  * @param feature
  * @param role
  * @param kind
  * @param toolkitName
  */
  public ChamElementInfo(String feature, String role, String kind,
  String toolkitName) {
  this.feature = feature;
  this.role = role;
  this.toolkitName = toolkitName;
  if (kind != null) {
  // add kind to toolkit kind hashtable
  addKind(toolkitName, makeKey(feature, role), new String[] {kind});
  }
  }

  /**
  *  constructs base element info.
  * @param feature
  * @param role
  * @param kind
  */
  public ChamElementInfo(String feature, String role, String kind) {
  this.feature = feature;
  this.role = role;
  this.toolkitName = null;
  if (kind != null) {
  // add kind to base kind hashtable
  addKind(null, makeKey(feature, role), new String[] {kind});
  }
  }

  /**
  * If element is base customizable element
  * @return
  */
  public boolean isBaseElement() {
  return (toolkitName == null);
  }
  }

  /**
  * Class that represents mapping of particular chameleon resource
  * when chameleon element maps to the customizable
  * element presentation.
  */
  static class ChamElementPresentation extends ChamElementInfo {
  /**
  * Сustomizable element presentation hint name.
  */
  String presentationHint;

  /**
  * Constructs toolkit presentation element info.
  * @param feature
  * @param role
  * @param present
  * @param toolkitName
  */
  public ChamElementPresentation(String feature, String role,
  String present, String toolkitName) {
  super(feature, role, null, toolkitName);
  this.presentationHint = present;
  }

  /**
  * Constructs base presentation element info.
  * @param feature
  * @param role
  * @param present
  */
  public ChamElementPresentation(String feature, String role, String present) {
  super(feature, role, null);
  this.presentationHint = present;
  }
  }

  static class ChamElementGroup extends ChamElementInfo {

  ChamElementInfo[] pieces;

  /**
  * Constructs toolkit group element info.
  * @param feature
  * @param role
  * @param child
  * @param toolkitName
  */
  public ChamElementGroup(String feature, String role,
  ChamElementInfo[] child, String toolkitName) {
  super(feature, role, Element.KIND_GROUP, toolkitName);
  this.pieces = child;
  }

  /**
  * Constructs base group element info.
  * @param feature
  * @param role
  * @param child
  */
  public ChamElementGroup(String feature, String role,
  ChamElementInfo[] child) {
  super(feature, role, Element.KIND_GROUP);
  this.pieces = child;
  }
  }

  /**
  * Adds kinds to the hash table. If value for cpecified key already exists,
  * appends kinds to already existing values.
  *
  * @param toolkitName name of the toolkit or null if base toolkit is assumed
  * @param key table key
  * @param kinds values to add
  */
  private static void addKind(String toolkitName, Object key, String[] kinds) {
  Hashtable table = pickKindTable(toolkitName);
  String[] prevKinds = (String[])table.get(key);
  if (prevKinds != null) {
  String[] val = new String[prevKinds.length + kinds.length];
  System.arraycopy(prevKinds, 0, val, 0, prevKinds.length);
  System.arraycopy(kinds, 0, val, prevKinds.length, kinds.length);
  table.put(key, val);
  } else {
  table.put(key, kinds);
  }
  }

  /**
  * Constructs key for kind hashtable
  * @param feature
  * @param role
  * @return uniq key
  */
  private static Object makeKey(String feature, String role) {
  return feature + role;
  }

  /**
  * Returns kind table for specified toolkit
  * @param toolkitName name of the toolkit
  * @return corresponding HashTable
  */
  private static Hashtable pickKindTable(String toolkitName) {
  if (toolkitName == null) {
  return baseKinds;
  } else if (toolkitName.equals(LCDUI_TOOLKIT_NAME)) {
  return lcduiKinds;
  } else {
  // no other toolkits allowed sat the moment
  // toolkit is not supported
  throw new IllegalArgumentException("toolkit not supported: " + toolkitName);
  }
  }

  public static String[] getSupportedToolkits() {
  return new String[] {LCDUI_TOOLKIT_NAME};
  }

  /**
  * Gets the supported content kinds of the customizable element
  * with the specified feature and role from specified toolkit.
  * If toolkit is null, the supported content for base element
  * is returned.
  * @param feature
  * @param role
  * @return array of available kinds or null if no element found
  * @throws java.lang.IllegalArgumentException - if toolkit is not supported
  */
  public static String[] getSupportedKinds(String feature,
  String role, String toolkitName) {
  Hashtable t = pickKindTable(toolkitName);
  if (t != null) {
  return (String[])t.get(makeKey(feature, role));
  } else {
  // toolkit is not supported
  throw new IllegalArgumentException();
  }
  }

  /**
  * Determines if the element with the specified feature and role is
  * supported or not for specified toolkit. If toolkit is null, it determines
  * if element is supported as base element.
  * @param feature
  * @param role
  * @param toolkitName
  * @return
  */
  public static boolean isToolkitElementSupported(String feature,
  String role, String toolkitName) {
  Hashtable t = pickKindTable(toolkitName);
  if (t != null) {
  return t.containsKey(makeKey(feature, role));
  }
  // throw exception?
  return false;
  }


  /**
  * Hash table with supported kinds for base elements.
  */
  static Hashtable baseKinds = new Hashtable();

  /**
  * Hash table with supported kinds for LCDUI toolkit elements.
  */
  static Hashtable lcduiKinds = new Hashtable();


  /**
  * Array of customizable element mapping for supported chameleon resource.
  * Index in this array is equal to corresponding chameleon resource id:
  * elements[id].chamID == id
  * Feature and Role combination is not a uniq key here as there is possible that
  * more than one chameleon resourse maps to one feature-role with different kinds.
  */
  static ChamElementInfo[] elements = {
  <xsl:for-each select="/configuration/constants/constant_class/*[../@Name='SkinPropertiesIDs']">
<xsl:sort data-type="number" select="Value" order="descending"></xsl:sort>
<xsl:variable name="constant_name" select="@Name"/>
<xsl:if test="position()!=1"><xsl:text>,
</xsl:text></xsl:if>
<xsl:choose>
<xsl:when test="/configuration/chamelements/*[@chamID=$constant_name]">
<!-- we take only firs element and ignore others -->
<xsl:apply-templates select="/configuration/chamelements/*[@chamID=$constant_name][1]"/>
</xsl:when>
<xsl:otherwise>
<xsl:text>        null</xsl:text>
</xsl:otherwise>
</xsl:choose>
<!-- add comment with chameleon resource id -->
<xsl:text> /*</xsl:text><xsl:value-of select="$constant_name"/><xsl:text>*/</xsl:text>
</xsl:for-each>
    };
}
</xsl:template>
    
<xsl:template match="/configuration/chamelements/chamelement">
    <xsl:text>        new ChamElementInfo("</xsl:text><xsl:value-of select="@feature" />", "<xsl:value-of select="@role" />", Element.<xsl:value-of select="@kind" />
    <xsl:if test="@toolkit">
    <xsl:text>, "</xsl:text><xsl:value-of select="@toolkit" /><xsl:text>"</xsl:text>
    </xsl:if>
    <xsl:text>)</xsl:text>
</xsl:template>

<xsl:template match="/configuration/chamelements/presentation">
    <xsl:text>        new ChamElementPresentation("</xsl:text><xsl:value-of select="@feature" />", "<xsl:value-of select="@role"/>", "<xsl:value-of select="@hint"/><xsl:text>"</xsl:text>
    <xsl:if test="@toolkit">
    <xsl:text>, "</xsl:text><xsl:value-of select="@toolkit" /><xsl:text>"</xsl:text>
    </xsl:if>
    <xsl:text>)</xsl:text>
</xsl:template>

<xsl:template match="/configuration/chamelements/composite_element">
    <xsl:text>        new ChamElementGroup("</xsl:text><xsl:value-of select="@feature" />", "<xsl:value-of select="@role" /><xsl:text>", new ChamElementInfo[] {
    </xsl:text>
    <xsl:for-each select="child::*">
    <xsl:if test="position()!=1"><xsl:text>,
    </xsl:text></xsl:if>
    <xsl:text>            new ChamElementInfo("</xsl:text><xsl:value-of select="@feature" />", "<xsl:value-of select="@role" />", Element.<xsl:value-of select="@kind" /><xsl:text>)</xsl:text>
    </xsl:for-each>
    <xsl:if test="@toolkit">
    <xsl:text>}, "</xsl:text><xsl:value-of select="@toolkit" /><xsl:text>"</xsl:text>
    </xsl:if>
    <xsl:text>)</xsl:text>
</xsl:template>

</xsl:stylesheet>
