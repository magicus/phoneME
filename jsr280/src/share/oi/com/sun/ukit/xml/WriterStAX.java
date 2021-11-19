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

import java.io.IOException;
import java.io.Writer;

import javax.xml.stream.XMLOutputFactory;
import javax.xml.stream.XMLStreamException;
import javax.xml.stream.XMLStreamWriter;

/**
 * The XMLStreamWriter interface specifies how to write XML. The XMLStreamWriter 
 * does not perform well formedness checking on its input.  However
 * the writeCharacters method is required to escape &amp; , &lt; and &gt;
 * For attribute values the writeAttribute method will escape the 
 * above characters plus &quot; to ensure that all character content
 * and attribute values are well formed. 
 *
 * Each NAMESPACE and ATTRIBUTE must be individually written.
 *
 * If javax.xml.stream.isPrefixDefaulting is set to <code>false</code> it is a 
 * fatal error if an element is written with namespace URI that has not been 
 * bound to a prefix.
 *
 * If javax.xml.stream.isPrefixDefaulting is set to <code>true</code> the 
 * XMLStreamWriter implementation must write a prefix for each unbound URI that 
 * it encounters in the current scope.  
 *
 * @see XMLOutputFactory
 * @see XMLStreamWriter
 */
/* pkg */ abstract class WriterStAX 
	implements XMLStreamWriter
{
	/* pkg */ static String INVALID_XML = "Invalid XML";

	/* pkg */ static int    ELM_EMPTY   = 0x01;
	/* pkg */ static int    ELM_INCOMP  = 0x02;

	/* pkg */ Writer  mOut;

	/* pkg */ int mPh;  // current phase of document writing
	/* pkg */ final static int PH_BEFORE_DOC  = -1;  // before writing
	/* pkg */ final static int PH_DOC_START   = 0;   // document start
	/* pkg */ final static int PH_MISC_DTD    = 1;   // misc before DTD
	/* pkg */ final static int PH_DTD         = 2;   // DTD
	/* pkg */ final static int PH_DTD_MISC    = 3;   // misc after DTD
	/* pkg */ final static int PH_DOCELM      = 4;   // document's element
	/* pkg */ final static int PH_DOCELM_MISC = 5;   // misc after element
	/* pkg */ final static int PH_AFTER_DOC   = 6;   // after parsing

	// true until first non attr/namespace output after start/empty element
	/* pkg */ boolean mIsElm;
	// name - prefix; value - namespace
	/* pkg */ Pair    mHint;   // prefix hints set by setPrefix
	/* pkg */ Pair    mPref;   // stack of prefixes
	// name - lname/qname; value - namespace
	/* pkg */ Pair    mElm;    // stack of elements

	private Pair      mDltd;   // deleted objects for reuse

	/**
	 * Create a new instance of XMLStreamWriter implementation.
	 *
	 * @param writer the output writer
	 */
	/* pkg */ WriterStAX(Writer writer)
	{
		if (writer == null)
			throw new NullPointerException("");
		mOut = writer;

		mPh    = PH_BEFORE_DOC;
		mIsElm = false;

		//		XML namespace
		mPref = pair(mPref);
		mPref.name  = "xml";
		mPref.value = "http://www.w3.org/XML/1998/namespace";
		//		XML Namespace namespace
		mPref = pair(mPref);
		mPref.name  = "xmlns";
		mPref.value = "http://www.w3.org/2000/xmlns/";
	}

	/**
	 * Writes a start tag to the output.  All writeStartElement methods
	 * open a new scope in the internal namespace context.  Writing the
	 * corresponding EndElement causes the scope to be closed.
	 *
	 * @param localName local name of the tag, may not be null
	 * @throws XMLStreamException
	 */ 
	public void writeStartElement(String localName) 
		throws XMLStreamException
	{
		if (localName == null)
			throw new NullPointerException("");

		newElm(false, localName);
	}

	/**
	 * Writes a start tag to the output
	 *
	 * @param namespaceURI the namespaceURI of the prefix to use, may not be null
	 * @param localName local name of the tag, may not be null
	 * @throws XMLStreamException if the namespace URI has not been bound to a 
	 *   prefix and javax.xml.stream.isPrefixDefaulting has not been set to 
	 *   <code>true</code>
	 */ 
	public void writeStartElement(String namespaceURI, String localName) 
		throws XMLStreamException
	{
		if (localName == null || namespaceURI == null)
			throw new NullPointerException("");

		newElm(false, namespaceURI, localName);
	}

	/**
	 * Writes a start tag to the output
	 *
	 * @param localName local name of the tag, may not be null
	 * @param prefix the prefix of the tag, may not be null
	 * @param namespaceURI the uri to bind the prefix to, may not be null
	 * @throws XMLStreamException
	 */ 
	public void writeStartElement(
			String prefix, String localName, String namespaceURI) 
		throws XMLStreamException
	{
		if (prefix == null || localName == null || namespaceURI == null)
			throw new NullPointerException("");

		newElm(false, prefix, namespaceURI, localName);
	}

	/**
	 * Writes an empty element tag to the output
	 *
	 * @param namespaceURI the uri to bind the tag to, may not be null
	 * @param localName local name of the tag, may not be null
	 * @throws XMLStreamException if the namespace URI has not been bound to a 
	 *   prefix and javax.xml.stream.isPrefixDefaulting has not been set to 
	 *   <code>true</code>
	 */ 
	public void writeEmptyElement(String namespaceURI, String localName) 
		throws XMLStreamException
	{
		if (localName == null || namespaceURI == null)
			throw new NullPointerException("");

		newElm(true, namespaceURI, localName);
	}

	/**
	 * Writes an empty element tag to the output
	 *
	 * @param prefix the prefix of the tag, may not be null
	 * @param localName local name of the tag, may not be null
	 * @param namespaceURI the uri to bind the tag to, may not be null
	 * @throws XMLStreamException
	 */ 
	public void writeEmptyElement(
			String prefix, String localName, String namespaceURI) 
		throws XMLStreamException
	{
		if (prefix == null || localName == null || namespaceURI == null)
			throw new NullPointerException("");

		newElm(true, prefix, namespaceURI, localName);
	}

	/**
	 * Writes an empty element tag to the output
	 *
	 * @param localName local name of the tag, may not be null
	 * @throws XMLStreamException
	 */ 
	public void writeEmptyElement(String localName) 
		throws XMLStreamException
	{
		if (localName == null)
			throw new NullPointerException("");

		newElm(true, localName);
	}

	/**
	 * Writes an end tag to the output relying on the internal 
	 * state of the writer to determine the prefix and local name
	 * of the event.
	 *
	 * @throws XMLStreamException 
	 */ 
	public void writeEndElement() 
		throws XMLStreamException
	{
		if (mElm == null)
			return;

		if (mIsElm != false) {
			if ((mElm.id & ELM_EMPTY) != 0) {
				writeElm();
				return;
			}
			writeElm();
		}
		try {
			mOut.write("</" + mElm.name + '>');
		} catch (IOException ioe) {
			throw new XMLStreamException(ioe);
		}
		delElm();
	}

	/**
	 * Closes any start tags and writes corresponding end tags.
	 *
	 * @throws XMLStreamException 
	 */ 
	public void writeEndDocument() 
		throws XMLStreamException
	{
		while (mElm != null)
			writeEndElement();
	}

	/**
	 * Close this writer and free any resources associated with the 
	 * writer.  This must not close the underlying output stream.
	 *
	 * @throws XMLStreamException 
	 */ 
	public void close() 
		throws XMLStreamException
	{
		writeEndDocument();
		flush();
	}

	/**
	 * Write any cached data to the underlying output mechanism.
	 *
	 * @throws XMLStreamException 
	 */ 
	public void flush() 
		throws XMLStreamException
	{
		if (mIsElm != false)
			writeElm();
		try {
			mOut.flush();
		} catch (IOException ioe) {
			throw new XMLStreamException(ioe);
		}
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
	public abstract void writeAttribute(String localName, String value) 
		throws XMLStreamException;

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

	public abstract void writeAttribute(
			String prefix, String namespaceURI, String localName, String value) 
		throws XMLStreamException;

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
	public abstract void writeAttribute(
			String namespaceURI, String localName, String value) 
		throws XMLStreamException;

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
		if (prefix == null || prefix.length() == 0 || prefix.equals("xmlns")) {
			writeDefaultNamespace(namespaceURI);
			return;
		}

		if (mIsElm == false)
			throw new IllegalStateException(INVALID_XML);
		if (namespaceURI == null)
			throw new NullPointerException("");

		mPref = pair(mPref);
		mPref.name  = prefix;
		mPref.value = namespaceURI;
		mPref.list  = mElm;
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
		if (mIsElm == false)
			throw new IllegalStateException(INVALID_XML);
		if (namespaceURI == null)
			throw new NullPointerException("");

		mPref = pair(mPref);
		mPref.name  = "";
		mPref.value = namespaceURI;
		mPref.list  = mElm;
	}

	/**
	 * Writes an xml comment with the data enclosed.
	 *
	 * @param data the data contained in the comment, may be null
	 * @throws XMLStreamException 
	 */ 
	public void writeComment(String data) 
		throws XMLStreamException
	{
		if (mIsElm != false)
			writeElm();
		if (mPh < PH_DOC_START)
			mPh = PH_DOC_START;
		try {
			mOut.write("<!--" + ((data != null) ? data : "") + "-->");
		} catch (IOException ioe) {
			throw new XMLStreamException(ioe);
		}
	}

	/**
	 * Writes a processing instruction
	 *
	 * @param target the target of the processing instruction, may not be null
	 * @throws XMLStreamException 
	 * @throws NullPointerException
	 */ 
	public void writeProcessingInstruction(String target) 
		throws XMLStreamException
	{
		if (target == null)
			throw new NullPointerException();

		if (mIsElm != false)
			writeElm();
		if (mPh < PH_DOC_START)
			mPh = PH_DOC_START;
		try {
			mOut.write("<?" + target + "?>");
		} catch (IOException ioe) {
			throw new XMLStreamException(ioe);
		}
	}

	/**
	 * Writes a processing instruction
	 *
	 * @param target the target of the processing instruction, may not be null
	 * @param data the data contained in the processing instruction, may not be null
	 * @throws XMLStreamException 
	 * @throws NullPointerException
	 */ 
	public void writeProcessingInstruction(String target, String data)
		throws XMLStreamException
	{
		if (target == null || data == null)
			throw new NullPointerException();

		if (mIsElm != false)
			writeElm();
		if (mPh < PH_DOC_START)
			mPh = PH_DOC_START;
		try {
			mOut.write("<?" + target + ' ' + data + "?>");
		} catch (IOException ioe) {
			throw new XMLStreamException(ioe);
		}
	}

	/**
	 * Writes a CData section
	 *
	 * @param data the data contained in the CData Section, may not be null
	 * @throws XMLStreamException 
	 * @throws NullPointerException
	 */ 
	public void writeCData(String data)
		throws XMLStreamException
	{
		if (data == null)
			throw new NullPointerException();

		if (mIsElm != false)
			writeElm();
		if (mPh != PH_DOCELM)
			throw new IllegalStateException(INVALID_XML);
		try {
			mOut.write("<![CDATA[" + data + "]]>");
		} catch (IOException ioe) {
			throw new XMLStreamException(ioe);
		}
	}

	/**
	 * Write a DTD section.  This string represents the entire doctypedecl 
	 * production from the XML 1.0 specification.
	 *
	 * @param dtd the DTD to be written, may not be null
	 * @throws XMLStreamException 
	 * @throws NullPointerException
	 */ 
	public void writeDTD(String dtd)
		throws XMLStreamException
	{
		if (mPh >= PH_DTD)
			throw new IllegalStateException(INVALID_XML);
		mPh = PH_DTD;
		try {
			mOut.write(dtd);
		} catch (IOException ioe) {
			throw new XMLStreamException(ioe);
		}
	}

	/**
	 * Writes an entity reference.
	 *
	 * @param name the name of the entity, may not be null
	 * @throws XMLStreamException 
	 * @throws NullPointerException
	 */ 
	public void writeEntityRef(String name)
		throws XMLStreamException
	{
		if (mIsElm != false)
			writeElm();
		if (name == null)
			throw new NullPointerException("");
		if (mPh != PH_DOCELM)
			throw new IllegalStateException(INVALID_XML);
		try {
			mOut.write('&' + name + ';');
		} catch (IOException ioe) {
			throw new XMLStreamException(ioe);
		}
	}

	/**
	 * Write the XML Declaration. Defaults the XML version to 1.0, and the 
	 * encoding to UTF-8.
	 *
	 * @throws XMLStreamException 
	 */ 
	public void writeStartDocument()
		throws XMLStreamException
	{
		writeStartDocument("1.0");
	}

	/**
	 * Write the XML Declaration. Defaults the XML version to 1.0
	 * @param version version of the xml document, may not be null
	 * @throws XMLStreamException 
	 * @throws NullPointerException
	 */ 
	public void writeStartDocument(String version)
		throws XMLStreamException
	{
		writeStartDocument("UTF-8", version);
	}

	/**
	 * Write the XML Declaration.  Note that the encoding parameter does
	 * not set the actual encoding of the underlying output.  That must 
	 * be set when the instance of the XMLStreamWriter is created using the
	 * XMLOutputFactory
	 *
	 * @param encoding encoding of the xml declaration, may not be null
	 * @param version version of the xml document, may not be null
	 * @throws XMLStreamException 
	 * @throws NullPointerException
	 */ 
	public void writeStartDocument(String encoding, String version)
		throws XMLStreamException
	{
		if (encoding == null || version == null)
			throw new NullPointerException("");
		if (mPh >= PH_DOC_START)
			throw new IllegalStateException(INVALID_XML);

		mPh = PH_DOC_START;
		try {
			mOut.write("<?xml version=\"" + 
				version + "\" encoding=\"" + encoding + "\"?>");
		} catch (IOException ioe) {
			throw new XMLStreamException(ioe);
		}
	}

	/**
	 * Write text to the output.
	 *
	 * @param text the value to write, may not be null
	 * @throws XMLStreamException 
	 * @throws NullPointerException
	 */ 
	public void writeCharacters(String text)
		throws XMLStreamException
	{
		char[] chars = text.toCharArray();
		writeCharacters(chars, 0, chars.length);
	}

	/**
	 * Write text to the output
	 *
	 * @param text the value to write, may not be null
	 * @param start the starting position in the array
	 * @param len the number of characters to write
	 * @throws XMLStreamException 
	 * @throws NullPointerException
	 */ 
	public void writeCharacters(char[] text, int start, int len)
		throws XMLStreamException
	{
		if (mIsElm != false)
			writeElm();
		if (mPh != PH_DOCELM)
			throw new IllegalStateException(INVALID_XML);
		if (len < 0)
			throw new IndexOutOfBoundsException("");

		try {
			for (int idx = 0; idx < len; idx++) {
				char ch = text[start + idx];
				switch (ch) {
				case '&':
					mOut.write("&amp;");
					break;
	
				case '<':
					mOut.write("&lt;");
					break;
	
				case '>':
					mOut.write("&gt;");
					break;
	
				default:
					mOut.write(ch);
				}
			}
		} catch (IOException ioe) {
			throw new XMLStreamException(ioe);
		}
	}

	/**
	 * Gets the prefix the uri is bound to.
	 *
	 * @return the prefix or null
	 * @throws XMLStreamException 
	 */ 
	public String getPrefix(String uri)
		throws XMLStreamException
	{
		String pref = findName(mPref, uri);
		if (pref == null)
			pref = findName(mHint, uri);
		return pref;
	}

	/**
	 * Sets the prefix the uri is bound to.  This prefix is bound
	 * in the scope of the current START_ELEMENT / END_ELEMENT pair.
	 * If this method is called before a START_ELEMENT has been written
	 * the prefix is bound in the root scope.
	 *
	 * @param prefix the prefix to bind to the uri, may not be null
	 * @param uri the uri to bind to the prefix, may be null
	 * @throws XMLStreamException 
	 * @throws NullPointerException
	 */ 
	public void setPrefix(String prefix, String uri)
		throws XMLStreamException
	{
		String pref = prefix.trim();

		mHint = pair(mHint);
		mHint.name  = pref;
		mHint.value = uri;
		mHint.list  = mElm;
	}

	/**
	 * Binds a URI to the default namespace. This URI is bound
	 * in the scope of the current START_ELEMENT / END_ELEMENT pair.
	 * If this method is called before a START_ELEMENT has been written
	 * the uri is bound in the root scope.
	 *
	 * @param uri the uri to bind to the default namespace, may be null
	 * @throws XMLStreamException 
	 */ 
	public void setDefaultNamespace(String uri)
		throws XMLStreamException
	{
		setPrefix("", uri);
	}

	/**
	 * Get the value of a feature/property from the underlying implementation
	 *
	 * @param name The name of the property, may not be null
	 * @return The value of the property
	 * @throws IllegalArgumentException if the property is not supported
	 * @throws NullPointerException if the name is null
	 */ 
	public abstract Object getProperty(String name)
		throws IllegalArgumentException;

	/**
	 * Creates a new element.
	 *
	 * @param isEmpty If <code>true</code> the element is empty.
	 * @param prefix the prefix of the tag
	 * @param localName local name of the tag
	 * @param namespaceURI the uri to bind the tag to
	 */
	/* pkg */ abstract void newElm(boolean isEmpty, 
			String prefix, String namespaceURI, String localName)
		throws XMLStreamException;

	/**
	 * Creates a new element.
	 *
	 * @param isEmpty If <code>true</code> the element is empty.
	 * @param localName local name of the tag
	 * @param namespaceURI the uri to bind the tag to
	 */
	/* pkg */ abstract void newElm(boolean isEmpty, 
			String namespaceURI, String localName)
		throws XMLStreamException;

	/**
	 * Creates a new element.
	 *
	 * @param isEmpty If <code>true</code> the element is empty.
	 * @param localName local name of the tag
	 */
	/* pkg */ abstract void newElm(boolean isEmpty, String localName)
		throws XMLStreamException;

	/**
	 * Writes an element created by <code>newElm</code> method.
	 */
	/* pkg */ void writeElm()
		throws XMLStreamException
	{
		try {
			mOut.write('<' + mElm.name);
			Pair attr;
			for (attr = mPref; attr != null; attr = attr.next) {
				if (attr.list != mElm)
					break;
				mOut.write(" xmlns");
				if (attr.name.length() != 0)
					mOut.write(':' + attr.name);
				mOut.write("=\"" + attr.value + "\"");
			}
			for (attr = mElm.list; attr != null; attr = attr.next) {
				mOut.write(' ' + attr.name + "=\"" + attr.value + "\"");
			}
			if ((mElm.id & ELM_EMPTY) != 0) {
				mOut.write('/');
				delElm();
			}
			mOut.write('>');
		} catch (IOException ioe) {
			throw new XMLStreamException(ioe);
		}

		mIsElm = false;
	}

	/**
	 * Removes <code>mElm</code> element.
	 */
	/* pkg */ void delElm()
	{
		Pair attr = mHint;
		while (attr != null && attr.list == mElm) {
			attr = del(attr);
		}
		attr = mPref;
		while (attr != null && attr.list == mElm) {
			attr = del(attr);
		}
		mPref = attr;
		attr = mElm.list;
		while (attr != null) {
			attr = del(attr);
		}
		mElm = del(mElm);
		if (mElm == null)
			mPh = PH_DOCELM_MISC;
	}

	/**
	 * Escapes an attribute value. Characters which will be escaped are: '"', 
	 * '&', '<', '>'.
	 */
	/* pkg */ String escValue(String value)
	{
		if (value.indexOf('"') < 0 && value.indexOf('&') < 0 && 
			value.indexOf('<') < 0 && value.indexOf('>') < 0)
			return value;

		int len = value.length();
		StringBuffer val = new StringBuffer(len + 20);
		for (int idx = 0; idx < len; idx++) {
			char ch = value.charAt(idx);
			switch(ch) {
			case '"':
				val.append("&quot;");
				break;

			case '&':
				val.append("&amp;");
				break;

			case '<':
				val.append("&lt;");
				break;

			case '>':
				val.append("&gt;");
				break;

			default:
				val.append(ch);
			}
		}

		return val.toString();
	}

	/**
	 * Retrieves name by value.
	 *
	 * @return name or null if there is no equal value in the list
	 */
	/* pkg */ String findName(Pair list, String value)
	{
		for (Pair elm = list; elm != null; elm = elm.next) {
			if (value.equals(elm.value))
				return elm.name;
		}
		return null;
	}

	/**
	 * Retrieves value by name.
	 *
	 * @return value or null if there is no equal name in the list
	 */
	/* pkg */ String findValue(Pair list, String name)
	{
		for (Pair elm = list; elm != null; elm = elm.next) {
			if (name.equals(elm.name))
				return elm.value;
		}
		return null;
	}

	/**
	 * Provides an instance of a pair.
	 *
	 * @param next The reference to a next pair.
	 * @return An instance of a pair.
	 */
	/* pkg */ Pair pair(Pair next)
	{
		Pair pair;

		if (mDltd != null) {
			pair  = mDltd;
			mDltd = pair.next;
		} else {
			pair  = new Pair();
		}
		pair.next = next;

		return pair;
	}

	/**
	 * Deletes an instance of a pair.
	 *
	 * @param pair The pair to delete.
	 * @return A reference to the next pair in a chain.
	 */
	/* pkg */ Pair del(Pair pair)
	{
		Pair next = pair.next;

		pair.name  = null;
		pair.value = null;
		pair.chars = null;
		pair.list  = null;
		pair.next  = mDltd;
		mDltd      = pair;

		return next;
	}
}
