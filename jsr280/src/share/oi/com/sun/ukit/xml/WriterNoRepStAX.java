/*
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved. 
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

package com.sun.ukit.xml;

import java.io.Writer;

import javax.xml.stream.XMLOutputFactory;
import javax.xml.stream.XMLStreamException;
import javax.xml.stream.XMLStreamWriter;

/**
 * XML stream writer which does not support namespace repairing.
 *
 * @see XMLOutputFactory
 * @see XMLStreamWriter
 */
/* pkg */ final class WriterNoRepStAX 
	extends WriterStAX
{
	/**
	 * Create a new instance of XMLStreamWriter implementation.
	 *
	 * @param writer the output writer
	 */
	/* pkg */ WriterNoRepStAX(Writer writer)
	{
		super(writer);
	}

	/**
	 * Creates a new element.
	 *
	 * @param isEmpty If <code>true</code> the element is empty.
	 * @param prefix the prefix of the tag
	 * @param localName local name of the tag
	 * @param namespaceURI the uri to bind the tag to
	 */
	/* pkg */ void newElm(boolean isEmpty, 
			String prefix, String namespaceURI, String localName)
		throws XMLStreamException
	{
		if (mIsElm != false)
			writeElm();
		if (mPh > PH_DOCELM)
			throw new IllegalStateException(INVALID_XML);
		mPh    = PH_DOCELM;
		mIsElm = true;

		mElm = pair(mElm);
		mElm.name  = prefix + ':' + localName;
		mElm.value = namespaceURI;
		mElm.id    = (isEmpty)? ELM_EMPTY: 0;
	}

	/**
	 * Creates a new element.
	 *
	 * @param isEmpty If <code>true</code> the element is empty.
	 * @param localName local name of the tag
	 * @param namespaceURI the uri to bind the tag to
	 */
	/* pkg */ void newElm(boolean isEmpty, 
			String namespaceURI, String localName)
		throws XMLStreamException
	{
		if (mIsElm != false)
			writeElm();
		if (mPh > PH_DOCELM)
			throw new IllegalStateException(INVALID_XML);
		mPh    = PH_DOCELM;
		mIsElm = true;

		String pref = findPrefix(namespaceURI);

		mElm = pair(mElm);
		mElm.name  = pref + ':' + localName;
		mElm.value = namespaceURI;
		mElm.id    = (isEmpty)? ELM_EMPTY: 0;
	}

	/**
	 * Creates a new element.
	 *
	 * @param isEmpty If <code>true</code> the element is empty.
	 * @param localName local name of the tag
	 */
	/* pkg */ void newElm(boolean isEmpty, String localName)
		throws XMLStreamException
	{
		if (mIsElm != false)
			writeElm();
		if (mPh > PH_DOCELM)
			throw new IllegalStateException(INVALID_XML);
		mPh    = PH_DOCELM;
		mIsElm = true;

		mElm = pair(mElm);
		mElm.name  = localName;
		mElm.value = null;
		mElm.id    = (isEmpty)? ELM_EMPTY: 0;
	}
  
	/**
	 * Writes an attribute to the output stream without a prefix.
	 *
	 * @param localName the local name of the attribute, may not be null
	 * @param value the value of the attribute
	 * @throws IllegalStateException if the current state does not allow 
	 *   Attribute writing
	 * @throws XMLStreamException 
	 * @throws NullPointerException
	 */ 
	public void writeAttribute(String localName, String value) 
		throws XMLStreamException
	{
		if (localName == null)
			throw new NullPointerException("");
		if (mIsElm == false)
			throw new IllegalStateException(INVALID_XML);
		String lname = localName.trim();
		if (lname.length() == 0)
			throw new IllegalStateException(INVALID_XML);

		mElm.list = pair(mElm.list);
		mElm.list.name  = lname;
		mElm.list.value = escValue(value);
	}

	/**
	 * Writes an attribute to the output stream.
	 *
	 * @param prefix the prefix for this attribute, may not be null
	 * @param namespaceURI the uri of the prefix for this attribute, may not be 
	 *   null
	 * @param localName the local name of the attribute, may not be null
	 * @param value the value of the attribute
	 * @throws IllegalStateException if the current state does not allow 
	 *   Attribute writing
	 * @throws XMLStreamException if the namespace URI has not been bound to a 
	 *   prefix and javax.xml.stream.isPrefixDefaulting has not been set to 
	 *   <code>true</code>
	 * @throws NullPointerException
	 */ 

	public void writeAttribute(
			String prefix, String namespaceURI, String localName, String value) 
		throws XMLStreamException
	{
		if (prefix == null || localName == null || namespaceURI == null)
			throw new NullPointerException("");
		if (mIsElm == false)
			throw new IllegalStateException(INVALID_XML);
		String lname = localName.trim();
		if (lname.length() == 0)
			throw new IllegalStateException(INVALID_XML);
		
		String pref = findPrefix(namespaceURI);
		if (prefix.equals(pref) == false) {
			throw new XMLStreamException(INVALID_XML);
		}

		mElm.list = pair(mElm.list);
		mElm.list.name  = prefix.trim() + ':' + lname;
		mElm.list.value = escValue(value);
	}

	/**
	 * Writes an attribute to the output stream.
	 * 
	 * @param namespaceURI the uri of the prefix for this attribute, may not be 
	 *   null
	 * @param localName the local name of the attribute, may not be null
	 * @param value the value of the attribute
	 * @throws IllegalStateException if the current state does not allow 
	 *   Attribute writing
	 * @throws XMLStreamException if the namespace URI has not been bound to a 
	 *   prefix and javax.xml.stream.isPrefixDefaulting has not been set to 
	 *   <code>true</code>
	 * @throws NullPointerException
	 */ 
	public void writeAttribute(
			String namespaceURI, String localName, String value) 
		throws XMLStreamException
	{
		if (localName == null || namespaceURI == null)
			throw new NullPointerException("");
		if (mIsElm == false)
			throw new IllegalStateException(INVALID_XML);
		String lname = localName.trim();
		if (lname.length() == 0)
			throw new IllegalStateException(INVALID_XML);

		String pref = findPrefix(namespaceURI);

		mElm.list = pair(mElm.list);
		mElm.list.name  = pref + ':' + lname;
		mElm.list.value = escValue(value);
	}

	/**
	 * Get the value of a feature/property from the underlying implementation
	 *
	 * @param name The name of the property, may not be null
	 * @return The value of the property
	 * @throws IllegalArgumentException if the property is not supported
	 * @throws NullPointerException if the name is null
	 */ 
	public Object getProperty(String name)
		throws IllegalArgumentException
	{
		if (name.equals(XMLOutputFactory.IS_REPAIRING_NAMESPACES) == false)
			throw new IllegalArgumentException(name);
		return Boolean.FALSE;
	}

	/**
	 * Retrieves a prefix by namespace URI
	 *
	 * @param namespaceURI The URI of the prefix
	 * @return prefix 
	 * @throws XMLStreamException if the prefix was not found
	 */
	private String findPrefix(String namespaceURI)
		throws XMLStreamException
	{
		String pref = findName(mPref, namespaceURI);
		if (pref == null) {
			pref = findName(mHint, namespaceURI);
			if (pref == null)
				throw new XMLStreamException(INVALID_XML);
		}

		return pref;
	}
}
