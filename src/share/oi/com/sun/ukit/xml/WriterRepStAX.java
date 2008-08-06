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
import java.io.OutputStreamWriter;
import java.io.IOException;

import javax.xml.stream.XMLOutputFactory;
import javax.xml.stream.XMLStreamWriter;
import javax.xml.stream.XMLStreamException;

/**
 * XML stream writer which supports namespace repairing.
 *
 * @see XMLOutputFactory
 * @see XMLStreamWriter
 */
/* pkg */ final class WriterRepStAX 
	extends WriterStAX
{
	private int mAuto = 'a' - 1;  // automatic prefix

	/**
	 * Create a new instance of XMLStreamWriter implementation.
	 *
	 * @param writer the output writer
	 */
	/* pkg */ WriterRepStAX(Writer writer)
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
		mElm.id = (isEmpty)? ELM_EMPTY: 0;

		String pref = findName(mPref, namespaceURI);
		if (pref == null) {
			//		Add new mapping for app defined prefix
			pref = verifyPref(prefix);
			writeNamespace(pref, namespaceURI);
		}
		//		Use the prefix already defined for the NS even if prefix 
		//		is different form one provided by app. (replace the prefix)
		mElm.name  = ((pref.length() != 0)? pref + ':': "") + localName;
		mElm.value = namespaceURI;
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

		mElm = pair(mElm);
		mElm.id = (isEmpty)? ELM_EMPTY: 0;

		String pref = findName(mPref, namespaceURI);
		if (pref == null) {
			//		Prefix from hints (bindings)
			pref = findName(mHint, namespaceURI);
			if (pref == null) {
				//		Auto prefix
				pref = getAuto();
			}
			writeNamespace(pref, namespaceURI);
		}
		mElm.name  = ((pref.length() != 0)? pref + ':': "") + localName;
		mElm.value = namespaceURI;
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
		mElm.id = (isEmpty)? ELM_EMPTY: 0;
		dropPref(mElm, localName);
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

		mElm.list = pair(mElm.list);
		dropPref(mElm.list, localName);
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

		String pref = findName(mPref, namespaceURI);
		if (pref == null) {
			//		Add new mapping for app defined prefix
			pref = verifyPref(prefix);
			writeNamespace(pref, namespaceURI);
		}
		//		Use the prefix already defined for the NS even if prefix 
		//		is different form one provided by app. (replace the prefix)
		mElm.list = pair(mElm.list);
		mElm.list.name  = ((pref.length() != 0) ? pref + ':' : "") + lname;
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

		String pref = findName(mPref, namespaceURI);
		if (pref == null) {
			//		Prefix from hints (bindings)
			pref = findName(mHint, namespaceURI);
			if (pref == null) {
				//		Auto prefix
				pref = getAuto();
			}
			writeNamespace(pref, namespaceURI);
		}
		mElm.list = pair(mElm.list);
		mElm.list.name  = ((pref.length() != 0) ? pref + ':' : "") + lname;
		mElm.list.value = escValue(value);
	}

	/**
	 * Writes a namespace to the output stream.
	 * If the prefix argument to this method is the empty string,
	 * "xmlns", or null this method will delegate to writeDefaultNamespace
	 *
	 * @param prefix the prefix to bind this namespace to
	 * @param namespaceURI the uri to bind the prefix to, may not be null
	 * @throws IllegalStateException if the current state does not allow 
	 *   Namespace writing
	 * @throws XMLStreamException 
	 * @throws NullPointerException
	 */ 
	public void writeNamespace(String prefix, String namespaceURI) 
		throws XMLStreamException
	{
		String pref = findName(mPref, namespaceURI);
		if (pref == null)
			super.writeNamespace(verifyPref(prefix), namespaceURI);
	}

	/**
	 * Writes the default namespace to the stream.
	 *
	 * @param namespaceURI the uri to bind the default namespace to, may not be 
	 *   null
	 * @throws IllegalStateException if the current state does not allow 
	 *   Namespace writing
	 * @throws XMLStreamException 
	 * @throws NullPointerException
	 */ 
	public void writeDefaultNamespace(String namespaceURI)
		throws XMLStreamException
	{
		for (Pair pref = mPref; pref != null; pref = pref.next) {
			if (pref.name.length() != 0)  // search for first default NS
				continue;
			if (pref.list == mElm) { // is default NS on current element?
				// is provided namespaceURI equal to default NS?
				if (pref.list.value.equals(namespaceURI))
					return;
			
				throw new IllegalStateException(INVALID_XML);
			}
			super.writeDefaultNamespace(namespaceURI);
			return;
		}
		super.writeDefaultNamespace(namespaceURI);
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
		return Boolean.TRUE;
	}

	/**
	 * Drops prefix if it is not in current stack.
	 *
	 * @param pair element or attribute descriptor.
	 * @param name qualified name.
	 */
	/* pkg */ void dropPref(Pair pair, String name)
		throws XMLStreamException
	{
		int off = name.indexOf(':');
		if (off < 0) {
			//		Name without prefix
			pair.name = name.trim();
			if (pair.name.length() == 0)  // qname is empty string
				throw new XMLStreamException(INVALID_XML);
			return;
		}
		//		Name with prefix
		String pref = name.substring(0, off).trim();
		pair.value  = findValue(mPref, pref);
		if (pair.value != null) {
			//		Keep the prefix (it is valid)
			pair.name = pref + ':' + name.substring(off + 1).trim();
			if (pair.name.length() <= pref.length() + 1)  // no local name
				throw new XMLStreamException(INVALID_XML);
		} else {
			//		Remove the prefix (cannot find URI)
			pair.name = name.substring(off + 1).trim();
			if (pair.name.length() == 0)  // no local name
				throw new XMLStreamException(INVALID_XML);
		}
	}

	/**
	 * Returns unique prefix if provided prefix is on current element. 
	 * Otherwise the same prefix is returned.
	 *
	 * @param prefix prefix to check.
	 * @return unique prefix.
	 */
	/* pkg */ String verifyPref(String prefix)
	{
		String pref = prefix.trim();
		for (Pair elm = mPref; elm != null && elm.list == mElm; elm =  elm.next) {
			if (pref.equals(elm.name))
				return getAuto();
		}
		return pref;
	}

	/**
	 * Creates a new prefix. Generates strings from "a" to "z" and then "zNNN", 
	 * where NNN is a number starting with 1 so the first string is "z1".
	 *
	 * @return unique prefix which shadows no prefix in current stack.
	 */
	/* pkg */ String getAuto()
	{
		String auto;
		if (mAuto < 'z') {
			mAuto++;
			auto = String.valueOf((char)mAuto);
		} else {
			mAuto += 0x100;
			auto = "z" + String.valueOf(mAuto >> 8);
		}
		return (findValue(mPref, auto) == null)? auto: getAuto();
	}
}
