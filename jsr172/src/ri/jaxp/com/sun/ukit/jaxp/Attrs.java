/*
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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
 */

package com.sun.ukit.jaxp;

import org.xml.sax.Attributes;

/**
 * SAX Attributes interface implementation.
 */

/* package */ class Attrs
	implements org.xml.sax.Attributes
{
	/**
	 * Attributes string array. Each individual attribute is reprecented by 
	 * four strings: namespace URL(+0), qname(+1), local name(+2), value(+3),
	 * type(+4). 
	 * In order to find attribute by the attrubute index, the attribute 
	 * index MUST be multiplied by 8. The result will point to the attribute 
	 * namespace URL. 
	 */
	/* package */ String[]	mItems;

	/**
	 * Number of attributes in the attributes string array.
	 */
	private char			mLength;

	/**
	 * Constructor.
	 */
	/* package */ Attrs()
	{
		//		The default number of attributies capacity is 8.
		mItems	= new String[(8 << 3)];
	}

	/**
	 * Sets up the number of attributes and ensure the capacity of 
	 * the attribute string array.
	 *
	 * @param length The number of attributes in the object.
	 */
	/* package */ void setLength(char length)
	{
		if (length > ((char)(mItems.length >> 3))) {
			mItems	= new String[length << 3];
		}
		mLength	= length;
	}

	/**
	 * Return the number of attributes in the list.
	 *
	 * <p>Once you know the number of attributes, you can iterate
	 * through the list.</p>
	 *
	 * @return The number of attributes in the list.
	 * @see #getURI(int)
	 * @see #getLocalName(int)
	 * @see #getQName(int)
	 * @see #getType(int)
	 * @see #getValue(int)
	 */
	public int getLength()
	{
		return mLength;
	}

	/**
	 * Look up an attribute's Namespace URI by index.
	 *
	 * @param index The attribute index (zero-based).
	 * @return The Namespace URI, or the empty string if none
	 *	is available, or null if the index is out of
	 *	range.
	 * @see #getLength
	 */
	public String getURI(int index)
	{
		return ((index >= 0) && (index < mLength))? 
			(mItems[index << 3]): 
			null;
	}

	/**
	 * Look up an attribute's local name by index.
	 *
	 * @param index The attribute index (zero-based).
	 * @return The local name, or the empty string if Namespace
	 *	processing is not being performed, or null
	 *	if the index is out of range.
	 * @see #getLength
	 */
	public String getLocalName(int index)
	{
		return ((index >= 0) && (index < mLength))? 
			(mItems[(index << 3) + 2]):
			null;
	}

	/**
	 * Look up an attribute's XML 1.0 qualified name by index.
	 *
	 * @param index The attribute index (zero-based).
	 * @return The XML 1.0 qualified name, or the empty string
	 *	if none is available, or null if the index
	 *	is out of range.
	 * @see #getLength
	 */
	public String getQName(int index)
	{
		if ((index < 0) || (index >= mLength))
			return null;
		return mItems[(index << 3) + 1];
	}

	/**
	 * Look up an attribute's type by index.
	 *
	 * <p>The attribute type is one of the strings "CDATA", "ID",
	 * "IDREF", "IDREFS", "NMTOKEN", "NMTOKENS", "ENTITY", "ENTITIES",
	 * or "NOTATION" (always in upper case).</p>
	 *
	 * <p>If the parser has not read a declaration for the attribute,
	 * or if the parser does not report attribute types, then it must
	 * return the value "CDATA" as stated in the XML 1.0 Recommentation
	 * (clause 3.3.3, "Attribute-Value Normalization").</p>
	 *
	 * <p>For an enumerated attribute that is not a notation, the
	 * parser will report the type as "NMTOKEN".</p>
	 *
	 * @param index The attribute index (zero-based).
	 * @return The attribute's type as a string, or null if the
	 *         index is out of range.
	 * @see #getLength
	 */
	public String getType(int index)
	{
		return ((index >= 0) && (index < (mItems.length >> 3)))? 
			(mItems[(index << 3) + 4]): 
			null;
	}

	/**
	 * Look up an attribute's value by index.
	 *
	 * <p>If the attribute value is a list of tokens (IDREFS,
	 * ENTITIES, or NMTOKENS), the tokens will be concatenated
	 * into a single string with each token separated by a
	 * single space.</p>
	 *
	 * @param index The attribute index (zero-based).
	 * @return The attribute's value as a string, or null if the
	 *         index is out of range.
	 * @see #getLength
	 */
	public String getValue(int index)
	{
		return ((index >= 0) && (index < mLength))? 
			(mItems[(index << 3) + 3]):
			null;
	}

	/**
	 * Look up the index of an attribute by Namespace name.
	 *
	 * @param uri The Namespace URI, or the empty string if
	 *	the name has no Namespace URI.
	 * @param localName The attribute's local name.
	 * @return The index of the attribute, or -1 if it does not
	 *	appear in the list.
	 */
	public int getIndex(String uri, String localName)
	{
		char	len	= mLength;
		char	idx	= 0;
		while (idx < len) {
			if ((mItems[idx << 3]).equals(uri) &&
				mItems[(idx << 3) + 2].equals(localName))
				return idx;
			idx++;
		}
		return -1;
	}

	/**
	 * Look up the index of an attribute by XML 1.0 qualified name.
	 *
	 * @param qName The qualified (prefixed) name.
	 * @return The index of the attribute, or -1 if it does not
	 *	appear in the list.
	 */
	public int getIndex(String qName)
	{
		char	len	= mLength;
		char	idx	= 0;
		while (idx < len) {
			if (getQName(idx).equals(qName))
				return idx;
			idx++;
		}
		return -1;
	}

	/**
	 * Look up an attribute's type by Namespace name.
	 *
	 * <p>See {@link #getType(int) getType(int)} for a description
	 * of the possible types.</p>
	 *
	 * @param uri The Namespace URI, or the empty String if the
	 *	name has no Namespace URI.
	 * @param localName The local name of the attribute.
	 * @return The attribute type as a string, or null if the
	 *	attribute is not in the list or if Namespace
	 *	processing is not being performed.
	 */
	public String getType(String uri, String localName)
	{
		int	idx	= getIndex(uri, localName);
		return (idx >= 0)? (mItems[(idx << 3) + 4]): null;
	}

	/**
	 * Look up an attribute's type by XML 1.0 qualified name.
	 *
	 * <p>See {@link #getType(int) getType(int)} for a description
	 * of the possible types.</p>
	 *
	 * @param qName The XML 1.0 qualified name.
	 * @return The attribute type as a string, or null if the
	 *	attribute is not in the list or if qualified names
	 *	are not available.
	 */
	public String getType(String qName)
	{
		int	idx	= getIndex(qName);
		return (idx >= 0)? (mItems[(idx << 3) + 4]): null;
	}

	/**
	 * Look up an attribute's value by Namespace name.
	 *
	 * <p>See {@link #getValue(int) getValue(int)} for a description
	 * of the possible values.</p>
	 *
	 * @param uri The Namespace URI, or the empty String if the
	 *	name has no Namespace URI.
	 * @param localName The local name of the attribute.
	 * @return The attribute value as a string, or null if the
	 *	attribute is not in the list.
	 */
	public String getValue(String uri, String localName)
	{
		int	idx	= getIndex(uri, localName);
		return (idx >= 0)? (mItems[(idx << 3) + 3]): null;
	}

	/**
	 * Look up an attribute's value by XML 1.0 qualified name.
	 *
	 * <p>See {@link #getValue(int) getValue(int)} for a description
	 * of the possible values.</p>
	 *
	 * @param qName The XML 1.0 qualified name.
	 * @return The attribute value as a string, or null if the
	 *	attribute is not in the list or if qualified names
	 *	are not available.
	 */
	public String getValue(String qName)
	{
		int	idx	= getIndex(qName);
		return (idx >= 0)? (mItems[(idx << 3) + 3]): null;
	}
}
