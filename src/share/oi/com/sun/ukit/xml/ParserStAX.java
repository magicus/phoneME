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

import java.io.InputStream;
import java.io.Reader;
import java.io.InputStreamReader;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.util.NoSuchElementException;
import java.lang.IllegalArgumentException;
import java.lang.IllegalStateException;

import javax.xml.stream.XMLStreamReader;
import javax.xml.stream.XMLResolver;
import javax.xml.stream.Location;
import javax.xml.stream.XMLStreamException;

import org.xml.sax.InputSource;
import org.xml.sax.ext.Attributes2;

/**
 * XML non-validating pull parser.
 *
 * @see javax.xml.stream.XMLStreamReader
 */

/* pkg */ final class ParserStAX
	extends Parser
	implements XMLStreamReader, Location
{
	private Pair mEQhead;  // internal event queue
	private Pair mEQtail;

	private int  mNsNum;  // number of namespaces defined on current element

	private String mEnc;  // encoding if provided by factory

	/* pkg */ DTDStreamReaderImp mDTD;  // DTD reader or null

	/* pkg */ XMLResolver mResolver;
	

	/**
	 * Constructor.
	 *
	 * @param stream Document input stream.
	 * @param encoding Document encoding or <code>null</code>.
	 */
	/* pkg */ ParserStAX(InputStream stream, String encoding)
		throws XMLStreamException
	{
		super();
		//		Set up the document
		if (stream == null)
			throw new NullPointerException();
		mInp = new Input(BUFFSIZE_READER);
		mPh  = PH_BEFORE_DOC;  // before parsing
		try {
			mEnc = encoding;
			InputSource is = new InputSource(stream);
			is.setEncoding(encoding);
			setinp(is);
		} catch(XMLStreamException xmlse) {
			throw xmlse;
		} catch(IOException ioe) {
			throw new XMLStreamException(ioe);
		} catch(RuntimeException rte) {
			throw rte;
		} catch(Exception e) {
			panic(e.toString());
		}
		init();
		mIsNSAware = true;
		//		Set initial event
		Pair evt = pair(null);
		evt.id = XMLStreamReader.START_DOCUMENT;
		eqAdd(evt);
	}

	/**
	 * Constructor.
	 *
	 * @param stream Document reader.
	 */
	/* pkg */ ParserStAX(Reader reader)
		throws XMLStreamException
	{
		super();
		//		Set up the document
		if (reader == null)
			throw new NullPointerException();
		mInp = new Input(BUFFSIZE_READER);
		mPh  = PH_BEFORE_DOC;  // before parsing
		try {
			setinp(new InputSource(reader));
		} catch(XMLStreamException xmlse) {
			throw xmlse;
		} catch(IOException ioe) {
			throw new XMLStreamException(ioe);
		} catch(RuntimeException rte) {
			throw rte;
		} catch(Exception e) {
			panic(e.toString());
		}
		init();
		mIsNSAware = true;
		//		Set initial event
		Pair evt = pair(null);
		evt.id = XMLStreamReader.START_DOCUMENT;
		eqAdd(evt);
	}

	/**
	 * Get the value of a feature/property from the underlying implementation
	 *
	 * @param name The name of the property, may not be <code>null</code>
	 * @return The value of the property
	 * @throws IllegalArgumentException if name is <code>null</code>
	 */
	public Object getProperty(String name) 
		throws IllegalArgumentException
	{
		if (name == null)
			throw new IllegalArgumentException(name);

		if ("javax.xml.stream.isValidating".equals(name)) {
			return Boolean.FALSE;
		} else if ("javax.xml.stream.isNamespaceAware".equals(name)) {
			return (mIsNSAware)? Boolean.TRUE: Boolean.FALSE;
		} else if ("javax.xml.stream.isCoalescing".equals(name)) {
			return Boolean.FALSE;
		} else if ("javax.xml.stream.isReplacingEntityReferences".equals(name)) {
			return Boolean.TRUE;
		} else if ("javax.xml.stream.isSupportingExternalEntities".equals(name)) {
			return (mResolver != null)? Boolean.TRUE: Boolean.FALSE;
		} else if ("javax.xml.stream.supportDTD".equals(name)) {
			return Boolean.TRUE;
		} else if ("javax.xml.stream.resolver".equals(name)) {
			return mResolver;
		} else if ("javax.xml.stream.DTDStreamReader".equals(name)) {
			if (getEventType() == XMLStreamReader.DTD) {
				if (mDTD == null) {
					Pair evt  = pair(null);
					evt.name  = mEQhead.name;
					evt.chars = null;
					evt.list  = pair(null);
					evt.list.name  = mEQhead.list.name;
					evt.list.value = mEQhead.list.value;
					mDTD = new DTDStreamReaderImp(this, evt);
				}
				return mDTD;
			}
			return null;
		}

		return null;
	}

	/**
	 * Get next parsing event - a processor may return all contiguous
	 * character data in a single chunk, or it may split it into several chunks.
	 * If the property javax.xml.stream.isCoalescing is set to <code>true</code>
	 * element content must be coalesced and only one CHARACTERS event
	 * must be returned for contiguous element content or
	 * CDATA Sections.  
	 *
	 * By default entity references must be 
	 * expanded and reported transparently to the application.
	 * An exception will be thrown if an entity reference cannot be expanded.
	 * If element content is empty (i.e. content is "") then no CHARACTERS event 
	 * will be reported.
	 *
	 * <p>Given the following XML:<br>
	 * &lt;foo>&lt;!--description-->content text&lt;![CDATA[&lt;greeting>Hello
	 * &lt;/greeting>]]>other content&lt;/foo><br>
	 * The behavior of calling next() when being on foo will be:<br>
	 * 1- the comment (COMMENT)<br>
	 * 2- then the characters section (CHARACTERS)<br>
	 * 3- then the CDATA section (another CHARACTERS)<br>
	 * 4- then the next characters section (another CHARACTERS)<br>
	 * 5- then the END_ELEMENT<br>
	 *
	 * <p><b>NOTE:</b> empty element (such as &lt;tag/>) will be reported
	 *  with  two separate events: START_ELEMENT, END_ELEMENT - This preserves
	 *   parsing equivalency of empty element to &lt;tag>&lt;/tag>.
	 *
	 * @return the integer code corresponding to the current parse event
	 * @throws NoSuchElementException if this is called when hasNext() returns 
	 *   <code>false</code>
	 * @throws XMLStreamException  if there is an error processing the underlying 
	 *   XML source
	 */
	public int next()
		throws XMLStreamException
	{
		try {
			if (mEQhead != null) {
				if (mEQhead.id == XMLStreamReader.END_ELEMENT) {
					//		Remove all element's namespace mappings
					while(mPref.list == mElm)
						mPref = del(mPref);
					//		Remove the top element tag
					mElm = del(mElm);
				}
				//		Remove the head of the queue
				del(eqGet());
				if (mEQhead != null)
					return mEQhead.id;
			}
			//		Event queue is empty. Get more events.
			mDTD     = null;  // make sure new events go into local queue
			Pair evt = null;
			int  eid = (mPh < PH_AFTER_DOC)? step(): 0;

			switch(mPh) {
			case PH_DOC_START:    // document start
				mPh = PH_MISC_DTD;
			case PH_MISC_DTD:     // misc before DTD
			case PH_DTD_MISC:     // misc after DTD
				switch (eid) {
				case EV_ELM:
					mNsNum = -1;  // recalculate the number if needed
					evt = pair(null);
					evt.id   = XMLStreamReader.START_ELEMENT;
					evt.list = mElm;
					eqAdd(evt);
					if (mAttrs.getLength() != 0) {
						evt = pair(null);
						evt.id   = XMLStreamReader.ATTRIBUTE;
						eqAdd(evt);
					}
					evt = pair(null);
					evt.id   = XMLStreamReader.END_ELEMENT;
					evt.list = mElm;
					eqAdd(evt);
					//		There is only one element in this document.
					mPh = (wsskip() != EOS)? PH_DOCELM_MISC: PH_AFTER_DOC;
					if (mPh == PH_AFTER_DOC) {
						evt = pair(null);
						evt.id = XMLStreamReader.END_DOCUMENT;
						eqAdd(evt);
					}
					break;

				case EV_ELMS:
					mNsNum = -1;  // recalculate the number if needed
					evt = pair(null);
					evt.id   = XMLStreamReader.START_ELEMENT;
					evt.list = mElm;
					eqAdd(evt);
					if (mAttrs.getLength() != 0) {
						evt = pair(null);
						evt.id   = XMLStreamReader.ATTRIBUTE;
						eqAdd(evt);
					}
					mPh = PH_DOCELM;       // document's element
					break;

				case EV_DTDS:
					if (mPh >= PH_DTD)
						panic(FAULT);
					mPh = PH_DTD;  // DTD
					break;

				case EV_COMM:
				case EV_PI:
					break;

				default:
					panic(FAULT);
				}
				break;

			case PH_DTD:                   // DTD
				switch (eid) {
				case EV_COMM:
				case EV_PI:
				case EV_PENT:
				case EV_UENT:
				case EV_NOT:
					do {
						if (mEQhead != null)
							del(eqGet());
					} while (step() != EV_DTDE);
				case EV_DTDE:
					mPh = PH_DTD_MISC;  // misc after DTD
					next();
					break;

				default:
					panic(FAULT);
				}
				break;

			case PH_DOCELM:                // document's element
				switch (eid) {
				case EV_ELM:
					mNsNum = -1;  // recalculate the number if needed
					evt = pair(null);
					evt.id   = XMLStreamReader.START_ELEMENT;
					evt.list = mElm;
					eqAdd(evt);
					if (mAttrs.getLength() != 0) {
						evt = pair(null);
						evt.id   = XMLStreamReader.ATTRIBUTE;
						eqAdd(evt);
					}
					evt = pair(null);
					evt.id   = XMLStreamReader.END_ELEMENT;
					evt.list = mElm;
					eqAdd(evt);
					break;

				case EV_ELMS:
					mNsNum = -1;  // recalculate the number if needed
					evt = pair(null);
					evt.id   = XMLStreamReader.START_ELEMENT;
					evt.list = mElm;
					eqAdd(evt);
					if (mAttrs.getLength() != 0) {
						evt = pair(null);
						evt.id   = XMLStreamReader.ATTRIBUTE;
						eqAdd(evt);
					}
					break;

				case EV_ELME:
					mNsNum = -1;  // recalculate the number if needed
					evt = pair(null);
					evt.id   = XMLStreamReader.END_ELEMENT;
					evt.list = mElm;
					eqAdd(evt);
					if (mElm.next == null) {
						mPh = (wsskip() != EOS)? PH_DOCELM_MISC: PH_AFTER_DOC;
						if (mPh == PH_AFTER_DOC) {
							evt = pair(null);
							evt.id = XMLStreamReader.END_DOCUMENT;
							eqAdd(evt);
						}
					}
					break;

				case EV_CDAT:
					mEQtail.id = XMLStreamReader.CDATA;
					break;

				case EV_TEXT:
				case EV_WSPC:
				case EV_COMM:
				case EV_PI:
				case EV_ENT:
					break;

				default:
					panic(FAULT);
				}
				break;

			case PH_DOCELM_MISC:           // misc after element
				switch (eid) {
				case EV_COMM:
				case EV_PI:
					if (wsskip() == EOS) {
						mPh = PH_AFTER_DOC;
						evt = pair(null);
						evt.id = XMLStreamReader.END_DOCUMENT;
						eqAdd(evt);
					}
					break;

				default:
					panic(FAULT);
				}
				break;

			case PH_BEFORE_DOC:            // before parsing
			case PH_AFTER_DOC:             // after parsing
				if (mDoc != null)
					throw new NoSuchElementException();
			default:
				throw new XMLStreamException();
			}

		} catch(XMLStreamException xmlse) {
			throw xmlse;
		} catch(IOException ioe) {
			throw new XMLStreamException(ioe);
		} catch(RuntimeException rte) {
			throw rte;
		} catch(Exception e) {
			panic(e.toString());
		}

		return mEQhead.id;
	}

	/**
	 * Test if the current event is of the given type and if the namespace and 
	 * name match the current namespace and name of the current event.  If the 
	 * namespaceURI is <code>null</code> it is not checked for equality, if the 
	 * localName is <code>null</code> it is not checked for equality.
	 *
	 * @param type the event type
	 * @param namespaceURI the uri of the event, may be <code>null</code>
	 * @param localName the localName of the event, may be <code>null</code>
	 * @throws XMLStreamException if the required values are not matched.
	 */
	public void require(int type, String namespaceURI, String localName)
		throws XMLStreamException
	{
		try {
			if (type != getEventType())
				throw new XMLStreamException(FAULT);
	
			if (namespaceURI != null && !namespaceURI.equals(getNamespaceURI()))
				throw new XMLStreamException(FAULT);
	
			if (localName != null && !localName.equals(getLocalName()))
				throw new XMLStreamException(FAULT);
		} catch (IllegalStateException ise) {
			throw new XMLStreamException(ise);
		}
	}

	/**
	 * Reads the content of a text-only element, an exception is thrown if this 
	 * is not a text-only element.
	 * Regardless of value of javax.xml.stream.isCoalescing this method always 
	 * returns coalesced content.
	 * <br /> Precondition: the current event is START_ELEMENT.
	 * <br /> Postcondition: the current event is the corresponding END_ELEMENT.
	 *
	 * <br />The method does the following (implementations are free to optimize 
	 * but must do equivalent processing):
	 * <pre>
	 * if(getEventType() != XMLStreamConstants.START_ELEMENT) {
	 *     throw new XMLStreamException(
	 *         "parser must be on START_ELEMENT to read next text", 
	 *         getLocation());
	 * }
	 * int eventType = next();
	 * StringBuffer content = new StringBuffer();
	 * while(eventType != XMLStreamConstants.END_ELEMENT ) {
	 *     if(eventType == XMLStreamConstants.CHARACTERS
	 *             || eventType == XMLStreamConstants.CDATA
	 *             || eventType == XMLStreamConstants.SPACE
	 *             || eventType == XMLStreamConstants.ENTITY_REFERENCE) {
	 *         content.append(getText());
	 *     } else if(eventType == XMLStreamConstants.PROCESSING_INSTRUCTION
	 *             || eventType == XMLStreamConstants.COMMENT) {
	 *             // skipping
	 *     } else if(eventType == XMLStreamConstants.END_DOCUMENT) {
	 *         throw new XMLStreamException(
	 *             "unexpected end of document when reading element text content",
	 *             this);
	 *     } else if(eventType == XMLStreamConstants.START_ELEMENT) {
	 *         throw new XMLStreamException(
	 *             "element text content may not contain START_ELEMENT", 
	 *             getLocation());
	 *     } else {
	 *         throw new XMLStreamException(
	 *             "Unexpected event type "+eventType, getLocation());
	 *     }
	 *     eventType = next();
	 * }
	 * return content.toString();
	 * </pre>
	 *
	 * @throws XMLStreamException if the current event is not a START_ELEMENT 
	 * or if a non text element is encountered
	 */
	public String getElementText()
		throws XMLStreamException
	{
		int evt = getEventType();
		if (evt != XMLStreamReader.START_ELEMENT)
			throw new XMLStreamException(FAULT);

		StringBuffer content = new StringBuffer();
		evt = next();
		while (evt != XMLStreamReader.END_ELEMENT) {
			switch (evt) {
			case XMLStreamReader.CHARACTERS:
			case XMLStreamReader.SPACE:
			case XMLStreamReader.CDATA:
				content.append(getText());
				break;

			case XMLStreamReader.ENTITY_REFERENCE:
				content.append((getText() != null)? getText(): "");
				break;

			case XMLStreamReader.PROCESSING_INSTRUCTION:
			case XMLStreamReader.COMMENT:
			case XMLStreamReader.ATTRIBUTE:
			case XMLStreamReader.NAMESPACE:
				break;

			default:
				throw new XMLStreamException(FAULT);
			}
			evt = next();
		}
		return content.toString();
	}

	/**
	 * Skips any white space (isWhiteSpace() returns <code>true</code>), COMMENT,
	 * or PROCESSING_INSTRUCTION, until a START_ELEMENT or END_ELEMENT is 
	 * reached.
	 * If other than white space characters, COMMENT, PROCESSING_INSTRUCTION, 
	 * START_ELEMENT, END_ELEMENT are encountered, an exception is thrown. This 
	 * method should be used when processing element-only content separated by 
	 * white space. 
	 *
	 * <br /> Precondition: none
	 * <br /> Postcondition: the current event is START_ELEMENT or END_ELEMENT
	 * and cursor may have moved over any whitespace event.
	 *
	 * <br />Essentially it does the following (implementations are free to 
	 * optimize but must do equivalent processing):
	 * <pre>
	 * int eventType = next();
	 * // skip whitespace
	 * while((eventType == XMLStreamConstants.CHARACTERS 
	 *         &amp;&amp; isWhiteSpace())
	 *         || (eventType == XMLStreamConstants.CDATA &amp;&amp; isWhiteSpace()) 
	 *         // skip whitespace
	 *         || eventType == XMLStreamConstants.SPACE
	 *         || eventType == XMLStreamConstants.PROCESSING_INSTRUCTION
	 *         || eventType == XMLStreamConstants.COMMENT) {
	 *     eventType = next();
	 * }
	 * if (eventType != XMLStreamConstants.START_ELEMENT 
	 *         &amp;&amp; eventType != XMLStreamConstants.END_ELEMENT) {
	 *     throw new String XMLStreamException(
	 *         "expected start or end tag", getLocation());
	 * }
	 * return eventType;
	 * </pre>
	 *
	 * @return the event type of the element read (START_ELEMENT or END_ELEMENT)
	 * @throws XMLStreamException if the current event is not white space, 
	 *   PROCESSING_INSTRUCTION, START_ELEMENT or END_ELEMENT
	 * @throws NoSuchElementException if this is called when hasNext() returns 
	 *   <code>false</code>
	 */
	public int nextTag()
		throws XMLStreamException
	{
		int evt = 0;
		do {
			evt = next();
			switch (evt) {
			case XMLStreamReader.SPACE:
			case XMLStreamReader.ATTRIBUTE:
			case XMLStreamReader.NAMESPACE:
				break;

			case XMLStreamReader.PROCESSING_INSTRUCTION:
			case XMLStreamReader.COMMENT:
				break;

			case XMLStreamReader.START_ELEMENT:
			case XMLStreamReader.END_ELEMENT:
				evt = -1;
				break;

			case XMLStreamReader.CHARACTERS:
			case XMLStreamReader.CDATA:
				if (isWhiteSpace())
					break;
			default:
				throw new XMLStreamException(FAULT);
			}
		} while (evt > 0);

		return getEventType();
	}

	/**
	 * Returns <code>true</code> if there are more parsing events and 
	 * <code>false</code> if there are no more events.  This method will return
	 * <code>false</code> if the current state of the XMLStreamReader is
	 * END_DOCUMENT
	 *
	 * @return <code>true</code> if there are more events, <code>false</code> 
	 *   otherwise
	 * @throws XMLStreamException if there is a fatal error detecting the next 
	 *   state
	 */
	public boolean hasNext()
		throws XMLStreamException
	{
		mDTD = null;
		return (mEQhead != null && mEQhead.next != null) || 
			(mPh >= PH_DOC_START && mPh < PH_AFTER_DOC);
	}

	/**
	 * Frees any resources associated with this Reader.  This method does not 
	 * close the underlying input source.
	 *
	 * @throws XMLStreamException if there are errors freeing associated resources
	 */
	public void close()
		throws XMLStreamException
	{
		mDTD = null;
		cleanup();
	}

	/**
	 * Returns <code>true</code> if the cursor points to a character data event
	 * that consists of all whitespace
	 *
	 * @return <code>true</code> if the cursor points to all whitespace, 
	 *   <code>false</code> otherwise
	 */
	public boolean isWhiteSpace()
	{
		switch (getEventType()) {
		case XMLStreamReader.CHARACTERS:
			return (mEQhead.name == null);  // hint from bflash and bflash_ws

		case XMLStreamReader.SPACE:
			return true;

		case XMLStreamReader.CDATA:
		default:
			return false;
		}
	}

	/**
	 * Returns the normalized attribute value of the attribute with the namespace 
	 * and localName. If the namespaceURI is <code>null</code> the namespace
	 * is not checked for equality. A value of "" (empty String) is interpreted 
	 * to mean 'no namespace'.
	 *
	 * @param namespaceURI the namespace of the attribute
	 * @param localName the local name of the attribute, cannot be 
	 *   <code>null</code>
	 * @return returns the value of the attribute , returns <code>null</code> if 
	 *   not found
	 * @throws IllegalStateException if this is not a START_ELEMENT or ATTRIBUTE
	 */
	public String getAttributeValue(String namespaceURI, String localName)
	{
		switch (getEventType()) {
		case XMLStreamReader.START_ELEMENT:
		case XMLStreamReader.ATTRIBUTE:
			if (namespaceURI != null && namespaceURI.length() == 0)
				return mAttrs.getValue(localName);
			if (mIsNSAware) {
				return mAttrs.getValue(
					mAttrs.getIndexNullNS(namespaceURI, localName));
			} else {
				return (namespaceURI == null)?
					mAttrs.getValue(localName): null;
			}

		default:
			throw new IllegalStateException(FAULT);
		}
	}

	/**
	 * Returns the count of attributes on this START_ELEMENT,
	 * this method is only valid on a START_ELEMENT or ATTRIBUTE.  This
	 * count excludes namespace definitions.  Attribute indices are
	 * zero-based.
	 *
	 * @return returns the number of attributes
	 * @throws IllegalStateException if this is not a START_ELEMENT or ATTRIBUTE
	 */
	public int getAttributeCount()
	{
		switch (getEventType()) {
		case XMLStreamReader.START_ELEMENT:
		case XMLStreamReader.ATTRIBUTE:
			return mAttrs.getLength();

		default:
			throw new IllegalStateException(FAULT);
		}
	}

	/**
	 * Returns the namespace of the attribute at the provided index.
	 *
	 * @param index the position of the attribute
	 * @return the namespace URI (can be <code>null</code>)
	 * @throws IllegalStateException if this is not a START_ELEMENT or ATTRIBUTE
	 */
	public String getAttributeNamespace(int index)
	{
		switch (getEventType()) {
		case XMLStreamReader.START_ELEMENT:
		case XMLStreamReader.ATTRIBUTE:
			String ns = mAttrs.getURI(index);
			if (ns == null)
				throw new IndexOutOfBoundsException();
			return (ns.length() != 0)? ns: null;

		default:
			throw new IllegalStateException(FAULT);
		}
	}

	/**
	 * Returns the localName of the attribute at the provided index.
	 *
	 * @param index the position of the attribute
	 * @return the localName of the attribute
	 * @throws IllegalStateException if this is not a START_ELEMENT or ATTRIBUTE
	 */
	public String getAttributeLocalName(int index)
	{
		switch (getEventType()) {
		case XMLStreamReader.START_ELEMENT:
		case XMLStreamReader.ATTRIBUTE:
			String name = (mIsNSAware != false)? 
				mAttrs.getLocalName(index): mAttrs.getQName(index);
			if (name == null)
				throw new IndexOutOfBoundsException();
			return name;

		default:
			throw new IllegalStateException(FAULT);
		}
	}

	/**
	 * Returns the prefix of this attribute at the provided index.
	 *
	 * @param index the position of the attribute
	 * @return the prefix of the attribute
	 * @throws IllegalStateException if this is not a START_ELEMENT or ATTRIBUTE
	 */
	public String getAttributePrefix(int index)
	{
		switch (getEventType()) {
		case XMLStreamReader.START_ELEMENT:
		case XMLStreamReader.ATTRIBUTE:
			//		See Attrs.java and Parser.attr() for details.
			String qname  = mAttrs.getQName(index);  // qName of the attribute
			if (qname == null)
				throw new IndexOutOfBoundsException();
			int    offset = qname.indexOf(':');
			return (offset >= 0)? qname.substring(0, offset): null;

		default:
			throw new IllegalStateException(FAULT);
		}
	}

	/**
	 * Returns the XML type of the attribute at the provided index.
	 *
	 * @param index the position of the attribute
	 * @return the XML type of the attribute
	 * @throws IllegalStateException if this is not a START_ELEMENT or ATTRIBUTE
	 */
	public String getAttributeType(int index)
	{
		switch (getEventType()) {
		case XMLStreamReader.START_ELEMENT:
		case XMLStreamReader.ATTRIBUTE:
			String type = mAttrs.getType(index);
			if (type == null)
				throw new IndexOutOfBoundsException();
			return type;

		default:
			throw new IllegalStateException(FAULT);
		}
	}

	/**
	 * Returns the value of the attribute at the index.
	 *
	 * @param index the position of the attribute
	 * @return the attribute value
	 * @throws IllegalStateException if this is not a START_ELEMENT or ATTRIBUTE
	 */
	public String getAttributeValue(int index)
	{
		switch (getEventType()) {
		case XMLStreamReader.START_ELEMENT:
		case XMLStreamReader.ATTRIBUTE:
			String value = mAttrs.getValue(index);
			if (value == null)
				throw new IndexOutOfBoundsException();
			return value;

		default:
			throw new IllegalStateException(FAULT);
		}
	}

	/**
	 * Returns a boolean which indicates if this attribute was created by default.
	 *
	 * @param index the position of the attribute
	 * @return <code>true</code> if this is a default attribute
	 * @throws IllegalStateException if this is not a START_ELEMENT or ATTRIBUTE
	 */
	public boolean isAttributeSpecified(int index)
	{
		switch (getEventType()) {
		case XMLStreamReader.START_ELEMENT:
		case XMLStreamReader.ATTRIBUTE:
			if (index < 0 || index >= mAttrs.getLength())
				throw new IndexOutOfBoundsException();
			return !((Attributes2)mAttrs).isSpecified(index);

		default:
			throw new IllegalStateException(FAULT);
		}
	}

	/**
	 * Returns the count of namespaces declared on this START_ELEMENT or 
	 * END_ELEMENT, this method is only valid on a START_ELEMENT, END_ELEMENT or 
	 * NAMESPACE. On an END_ELEMENT the count is of the namespaces that are about 
	 * to go out of scope.  This is the equivalent of the information reported
	 * by SAX callback for an end element event.
	 *
	 * @return returns the number of namespace declarations on this specific 
	 *   element
	 * @throws IllegalStateException if this is not a START_ELEMENT, END_ELEMENT 
	 *   or NAMESPACE
	 */
	public int getNamespaceCount()
	{
		switch (getEventType()) {
		case XMLStreamReader.START_ELEMENT:
		case XMLStreamReader.END_ELEMENT:
		case XMLStreamReader.NAMESPACE:
			return nsNum();

		default:
			throw new IllegalStateException(FAULT);
		}
	}

	/**
	 * Returns the prefix for the namespace declared at the index. Returns 
	 * <code>null</code> if this is the default namespace declaration
	 *
	 * @param index the position of the namespace declaration
	 * @return returns the namespace prefix
	 * @throws IllegalStateException if this is not a START_ELEMENT, END_ELEMENT 
	 *   or NAMESPACE
	 */
	public String getNamespacePrefix(int index)
	{
		switch (getEventType()) {
		case XMLStreamReader.START_ELEMENT:
		case XMLStreamReader.END_ELEMENT:
		case XMLStreamReader.NAMESPACE:
			break;

		default:
			throw new IllegalStateException(FAULT);
		}
		if (index < 0 || index >= nsNum())
			throw new IndexOutOfBoundsException();

		Pair pref = mPref;
		for (int idx = 0; idx < index; idx++) {
			pref = pref.next;
		}

		return (pref.name.length() != 0)? pref.name: null;
	}
  
	/**
	 * Returns the uri for the namespace declared at the index.
	 *
	 * @param index the position of the namespace declaration
	 * @return returns the namespace uri
	 * @throws IllegalStateException if this is not a START_ELEMENT, END_ELEMENT 
	 *   or NAMESPACE
	 */
	public String getNamespaceURI(int index)
	{
		switch (getEventType()) {
		case XMLStreamReader.START_ELEMENT:
		case XMLStreamReader.END_ELEMENT:
		case XMLStreamReader.NAMESPACE:
			break;

		default:
			throw new IllegalStateException(FAULT);
		}
		if (nsNum() == 0)
			return null;
		if (index < 0 || index >= nsNum())
			throw new IndexOutOfBoundsException();

		Pair pref = mPref;
		for (int idx = 0; idx < index; idx++) {
			pref = pref.next;
		}

		return pref.value;
	}

	/**
	 * Return the uri for the given prefix.
	 * The uri returned depends on the current state of the processor.
	 *
	 * <p><strong>NOTE:</strong>The 'xml' prefix is bound as defined in
	 * <a href="http://www.w3.org/TR/REC-xml-names/#ns-using">
	 * Namespaces in XML</a> specification to 
	 * "http://www.w3.org/XML/1998/namespace".
	 *
	 * <p><strong>NOTE:</strong> The 'xmlns' prefix must be resolved to following
	 * namespace
	 * <a href="http://www.w3.org/2000/xmlns/">http://www.w3.org/2000/xmlns/</a>
	 *
	 * @param prefix The prefix to lookup, may not be <code>null</code>
	 * @return the uri bound to the given prefix or <code>null</code> if it is 
	 *   not bound
	 * @throws IllegalArgumentException if the prefix is <code>null</code>
	 */
	public String getNamespaceURI(String prefix)
	{
		if (prefix == null)
			throw new IllegalArgumentException("");

		Pair pref = mPref;
		while (pref != null) {
			if (prefix.equals(pref.name))
				return pref.value;
			pref = pref.next;
		}

		if (prefix.equals("xml"))
			return "http://www.w3.org/XML/1998/namespace";
		if (prefix.equals("xmlns"))
			return "http://www.w3.org/2000/xmlns/";

		return null;
	}

	/**
	 * Returns an integer code that indicates the type of the event the cursor 
	 * is pointing to.
	 */
	public int getEventType()
	{
		return mEQhead.id;
	}

	/**
	 * Returns the current value of the parse event as a string, this returns the
	 * string value of a CHARACTERS event, returns the value of a COMMENT, the 
	 * replacement value for an ENTITY_REFERENCE, the string value of a CDATA 
	 * section, the string value for a SPACE event, or the String value of the 
	 * internal subset of the DTD. If an ENTITY_REFERENCE has been resolved, any 
	 * character data will be reported as CHARACTERS events.
	 *
	 * @return the current text or <code>null</code>
	 * @throws java.lang.IllegalStateException if this state is not a valid
	 *   text state.
	 */
	public String getText()
	{
		switch (getEventType()) {
		case XMLStreamReader.COMMENT:
		case XMLStreamReader.CHARACTERS:
		case XMLStreamReader.SPACE:
		case XMLStreamReader.CDATA:
			if (mEQhead.value == null)
				mEQhead.value = new String(mEQhead.chars, 0, mEQhead.num);
			return mEQhead.value;

		case XMLStreamReader.DTD:
			if (mDTD != null)
				throw new IllegalStateException(FAULT);
			return (mEQhead.chars != null)? String.valueOf(mEQhead.chars): null;

		case XMLStreamReader.ENTITY_REFERENCE:
			return mEQhead.value;

		default:
			throw new IllegalStateException(FAULT);
		}
	}

	/**
	 * Returns an array which contains the characters from this event.
	 * This array should be treated as read-only and transient. I.e. the array 
	 * will contain the text characters until the XMLStreamReader moves on to the
	 * next event. Attempts to hold onto the character array beyond that time or 
	 * modify the contents of the array are breaches of the contract for this 
	 * interface.
	 *
	 * @return the current text or an empty array
	 * @throws java.lang.IllegalStateException if this state is not a valid 
	 *   text state.
	 */
	public char[] getTextCharacters()
	{
		switch (getEventType()) {
		case XMLStreamReader.COMMENT:
		case XMLStreamReader.CHARACTERS:
		case XMLStreamReader.SPACE:
		case XMLStreamReader.CDATA:
			return mEQhead.chars;

		default:
			throw new IllegalStateException(FAULT);
		}
	}

	/**
	 * Returns the offset into the text character array where the first character
	 * (of this text event) is stored.
	 *
	 * @throws java.lang.IllegalStateException if this state is not a valid text
	 *   state.
	 */
	public int getTextStart()
	{
		switch (getEventType()) {
		case XMLStreamReader.COMMENT:
		case XMLStreamReader.CHARACTERS:
		case XMLStreamReader.SPACE:
		case XMLStreamReader.CDATA:
			return 0;

		default:
			throw new IllegalStateException(FAULT);
		}
	}

	/**
	 * Returns the length of the sequence of characters for this Text event 
	 * within the text character array.
	 *
	 * @throws java.lang.IllegalStateException if this state is not a valid text
	 *   state.
	 */
	public int getTextLength()
	{
		switch (getEventType()) {
		case XMLStreamReader.COMMENT:
		case XMLStreamReader.CHARACTERS:
		case XMLStreamReader.SPACE:
		case XMLStreamReader.CDATA:
			return mEQhead.num;

		default:
			throw new IllegalStateException(FAULT);
		}
	}

	/**
	 * Return input encoding if known or <code>null</code> if unknown.
	 *
	 * @return the encoding of this instance or <code>null</code>
	 */
	public String getEncoding()
	{
		return mEnc;
	}

	/**
	 * Return the current location of the processor.
	 * If the Location is unknown the processor should return an implementation 
	 * of Location that returns -1 for the location and <code>null</code> for the 
	 * publicId and systemId. The location information is only valid until next() 
	 * is called.
	 */
	public Location getLocation()
	{
		return this;
	}

	/**
	 * Returns the (local) name of the current event. 
	 * For START_ELEMENT or END_ELEMENT returns the (local) name of the current 
	 * element. For ENTITY_REFERENCE it returns entity name.  The current event 
	 * must be START_ELEMENT or END_ELEMENT or ENTITY_REFERENCE.
	 *
	 * @return the localName
	 * @throws IllegalStateException if this not a START_ELEMENT, END_ELEMENT 
	 *   or ENTITY_REFERENCE
	 */
	public String getLocalName()
	{
		switch (getEventType()) {
		case XMLStreamReader.START_ELEMENT:
		case XMLStreamReader.END_ELEMENT:
			return mEQhead.list.name;

		case XMLStreamReader.ENTITY_REFERENCE:
			return mEQhead.name;

		default:
			throw new IllegalStateException(FAULT);
		}
	}

	/**
	 * If the current event is a START_ELEMENT or END_ELEMENT  this method
	 * returns the URI of the prefix or the default namespace.
	 * Returns <code>null</code> if the event does not have a prefix.
	 *
	 * @return the URI bound to this elements prefix, the default namespace, or 
	 *   <code>null</code>
	 */
	public String getNamespaceURI()
	{
		switch (getEventType()) {
		case XMLStreamReader.START_ELEMENT:
		case XMLStreamReader.END_ELEMENT:
			Pair elm = mEQhead.list;
			return (elm.ns != null && elm.ns.length() != 0)? 
				elm.ns: null;

		case XMLStreamReader.NAMESPACE:
			return mEQhead.list.value;

		default:
			return null;
		}
	}

	/**
	 * Returns the prefix of the current event or <code>null</code> if the event 
	 * does not have a prefix.
	 *
	 * @return the prefix or <code>null</code>
	 */
	public String getPrefix()
	{
		switch (getEventType()) {
		case XMLStreamReader.START_ELEMENT:
		case XMLStreamReader.END_ELEMENT:
			Pair elm = mEQhead.list;
			return (elm.chars[0] > 0)? 
				new String(elm.chars, 1, elm.chars[0] - 1):
				null;

		default:
			throw new IllegalStateException(FAULT);
		}
	}

	/**
	 * Get the xml version declared on the xml declaration.  Returns 
	 * <code>null</code> if none was declared.
	 *
	 * @return the XML version or <code>null</code>
	 */
	public String getVersion()
	{
		//		Current version of the parser does not support XML versions 
		//		other then 1.0.
		switch (mInp.xmlver) {
		case 0x0100:
			return "1.0";

		default:
			return null;
		}
	}

	/**
	 * Get the standalone declaration from the xml declaration.
	 *
	 * @return <code>true</code> if this is standalone, or <code>false</code> 
	 *   otherwise
	 */
	public boolean isStandalone()
	{
		return mIsSAlone;
	}

	/**
	 * Checks if standalone was set in the document.
	 *
	 * @return <code>true</code> if standalone was set in the document, or 
	 *   <code>false</code> otherwise
	 */
	public boolean standaloneSet()
	{
		return mIsSAloneSet;
	}

	/**
	 * Returns the character encoding declared on the xml declaration.
	 * Returns <code>null</code> if none was declared.
	 *
	 * @return the encoding declared in the document or <code>null</code>
	 */
	public String getCharacterEncodingScheme()
	{
		//		Returns encoding from XML declaration of current entity. 
		//		Therefore it might be encoding of the document entity or an 
		//		external parsed entity.
		return mInp.xmlenc;
	}

	/**
	 * Get the target of a processing instruction.
	 *
	 * @return the target or <code>null</code>
	 */
	public String getPITarget()
	{
		if (getEventType() != XMLStreamReader.PROCESSING_INSTRUCTION)
			throw new IllegalStateException(FAULT);
		return mEQhead.name;
	}

	/**
	 * Get the data section of a processing instruction.
	 *
	 * @return the data or <code>null</code>
	 */
	public String getPIData()
	{
		if (getEventType() != XMLStreamReader.PROCESSING_INSTRUCTION)
			throw new IllegalStateException(FAULT);
		return mEQhead.value;
	}

	/**
	 * Return the line number where the current event ends,
	 * returns -1 if none is available.
	 *
	 * @return the current line number
	 */
	public int getLineNumber()
	{
		return -1;
	}

	/**
	 * Return the column number where the current event ends,
	 * returns -1 if none is available.
	 *
	 * @return the current column number
	 */
	public int getColumnNumber()
	{
		return -1;
	}

	/**
	 * Return the byte or character offset into the input source this location
	 * is pointing to. If the input source is a file or a byte stream then 
	 * this is the byte offset into that stream, but if the input source is 
	 * a character media then the offset is the character offset. 
	 * Returns -1 if there is no offset available.
	 *
	 * @return the current offset
	 */
	public int getCharacterOffset()
	{
		return -1;
	}

	/**
	 * Returns the public ID of the XML
	 *
	 * @return the public ID, or null if not available
	 */
	public String getPublicId()
	{
		return (mInp != null)? mInp.pubid: null;
	}

	/**
	 * Returns the system ID of the XML
	 *
	 * @return the system ID, or null if not available
	 */
	public String getSystemId()
	{
		return (mInp != null)? mInp.sysid: null;
	}

	/**
	 * Reports document type.
	 *
	 * @param name The name of the entity.
	 * @param pubid The public identifier of the DTD or <code>null</code>.
	 * @param sysid The system identifier of the DTD or <code>null</code>.
	 * @param dtdint The DTD internal subset or <code>null</code>.
	 */
	protected void docType(
		String name, String pubid, String sysid, char[] dtdint)
	{
		Pair evt  = pair(null);
		evt.id    = XMLStreamReader.DTD;
		evt.name  = name;
		evt.chars = dtdint;
		evt.list  = pair(null);
		evt.list.name  = pubid;
		evt.list.value = sysid;
		eqAdd(evt);
	}

	/**
	 * Reports a comment.
	 *
	 * @param text The comment text starting from first character.
	 * @param length The number of characters in comment.
	 */
	protected void comm(char[] text, int length)
	{
		Pair evt  = pair(null);
		evt.id    = XMLStreamReader.COMMENT;
		evt.chars = text;
		evt.num   = length;
		if (mDTD != null)
			mDTD.eqAdd(evt);
		else
			eqAdd(evt);
	}

	/**
	 * Reports a processing instruction.
	 *
	 * @param target The processing instruction target name.
	 * @param body The processing instruction body text.
	 */
	protected void pi(String target, String body)
		throws Exception
	{
		Pair evt  = pair(null);
		evt.id    = XMLStreamReader.PROCESSING_INSTRUCTION;
		evt.name  = target;
		evt.value = (body.length() != 0)? body: null;
		if (mDTD != null)
			mDTD.eqAdd(evt);
		else
			eqAdd(evt);
	}

	/**
	 * Reports new namespace prefix. 
	 * The Namespace prefix (<code>mPref.name</code>) being declared and 
	 * the Namespace URI (<code>mPref.value</code>) the prefix is mapped 
	 * to. An empty string is used for the default element namespace, 
	 * which has no prefix.
	 */
	protected void newPrefix()
		throws Exception
	{
		Pair evt = pair(null);
		evt.id   = XMLStreamReader.NAMESPACE;
		evt.list = mPref;
		eqAdd(evt);
	}

	/**
	 * Reports skipped entity name.
	 *
	 * @param name The entity name.
	 */
	protected void skippedEnt(String name)
		throws Exception
	{
		if (name.charAt(0) != '%' && !name.equals("[dtd]"))
			panic(name);
	}

	/**
	 * Returns an <code>InputSource</code> for specified entity or 
	 * <code>null</code>.
	 *
	 * @param name The name of the entity.
	 * @param pubid The public identifier of the entity.
	 * @param sysid The system identifier of the entity.
	 */
	protected InputSource resolveEnt(String name, String pubid, String sysid)
		throws XMLStreamException, IOException
	{
		if (mResolver != null) {
			InputStream is = (InputStream)mResolver.resolveEntity(
				pubid, sysid, null, null);
			return new InputSource(is);
		}
		return null;
	}

	/**
	 * Reports internal parsed entity.
	 *
	 * @param name The entity name.
	 * @param value The entity replacement text.
	 */
	protected void intparsedEntDecl(String name, char[] value)
		throws Exception
	{
		if (mDTD != null) {
			Pair evt  = pair(null);
			evt.id    = XMLStreamReader.ENTITY_DECLARATION;
			evt.name  = name;
			evt.num   = value.length;
			evt.chars = value;
			mDTD.eqAdd(evt);
		}
	}

	/**
	 * Reports external parsed entity.
	 *
	 * @param name The entity name.
	 * @param pubid The entity public identifier, may be null.
	 * @param name The entity system identifier, may be null.
	 */
	protected void extparsedEntDecl(String name, String pubid, String sysid)
		throws Exception
	{
		if (mDTD != null) {
			Pair evt  = pair(null);
			evt.id    = XMLStreamReader.ENTITY_DECLARATION;
			evt.name  = name;
			evt.num   = -1;
			evt.chars = new char[0];
			evt.list  = pair(null);
			evt.list.name  = pubid;
			evt.list.value = sysid;
			mDTD.eqAdd(evt);
		}
	}

	/**
	 * Reports notation declaration.
	 *
	 * @param name The notation's name.
	 * @param pubid The notation's public identifier, or <code>null</code> 
	 *   if none was given.
	 * @param sysid The notation's system identifier, or <code>null</code> 
	 *   if none was given.
	 */
	protected void notDecl(String name, String pubid, String sysid)
		throws Exception
	{
		if (mDTD != null) {
			Pair evt  = pair(null);
			evt.id    = XMLStreamReader.NOTATION_DECLARATION;
			evt.name  = name;
			evt.list  = pair(null);
			evt.list.name  = pubid;
			evt.list.value = sysid;
			mDTD.eqAdd(evt);
		}
	}

	/**
	 * Reports unparsed entity name.
	 *
	 * @param name The unparsed entity's name.
	 * @param pubid The entity's public identifier, or null if none was given.
	 * @param sysid The entity's system identifier.
	 * @param notation The name of the associated notation.
	 */
	protected void unparsedEntDecl(
			String name, String pubid, String sysid, String notation)
		throws Exception
	{
		if (mDTD != null) {
			Pair evt  = pair(null);
			evt.id    = XMLStreamReader.ENTITY_DECLARATION;
			evt.name  = name;
			evt.value = notation;
			evt.list  = pair(null);
			evt.list.name  = pubid;
			evt.list.value = sysid;
			mDTD.eqAdd(evt);
		}
	}

	/**
	 * Notifies the handler about fatal parsing error.
	 *
	 * @param msg The problem description message.
	 */
	protected void panic(String msg)
		throws XMLStreamException
	{
		throw new XMLStreamException(msg);
	}

	/**
	 * Reports characters and empties the parser's buffer.
	 * This method is called only if parser is going to return control to 
	 * the main loop. This means that this method may use parser buffer 
	 * to report white space without copying characters to temporary 
	 * buffer.
	 */
	protected void bflash()
		throws Exception
	{
		if (mBuffIdx < 0)  // if the parser buffer is empty
			return;

		Pair evt  = pair(null);
		evt.id    = XMLStreamReader.CHARACTERS;
		evt.chars = mBuff;         // character data
		evt.num   = mBuffIdx + 1;  // number of characters
		evt.name  = "";            // non white space hint for isWhiteSpace
		eqAdd(evt);
		//		Empty parser's buffer.
		mBuffIdx = -1;
	}

	/**
	 * Reports white space characters and empties the parser's buffer. 
	 * This method is called only if parser is going to return control to 
	 * the main loop. This means that this method may use parser buffer 
	 * to report white space without copying characters to temporary 
	 * buffer.
	 */
	protected void bflash_ws()
		throws Exception
	{
		if (mBuffIdx < 0)  // if the parser buffer is empty
			return;

		Pair evt  = pair(null);
		evt.id    = ((mElm.id & FLAG_XMLSPC_PRESERVE) != 0) ? 
			XMLStreamReader.CHARACTERS : XMLStreamReader.SPACE;
		evt.chars = mBuff;         // white space characters
		evt.num   = mBuffIdx + 1;  // number of characters
		evt.name  = null;          // white space hint for isWhiteSpace
		eqAdd(evt);
		//		Empty parser's buffer.
		mBuffIdx = -1;
	}

	/**
	 * Adds an event to the head of internal event queue.
	 *
	 * @param evt An event to add to the tail of the queue.
	 */
	private void eqAdd(Pair evt)
	{
		evt.next = null;
		if (mEQtail != null)
			mEQtail.next = evt;
		else
			mEQhead = evt;
		mEQtail = evt;
	}

	/**
	 * Retrieves an event from internal event queue.
	 *
	 * @return An event or <code>null</code> if queue is empty.
	 */
	private Pair eqGet()
	{
		Pair evt = mEQhead;
		if (evt != null) {
			mEQhead  = evt.next;
			evt.next = null;
		}
		if (mEQhead == null)
			mEQtail = null;

		return evt;
	}

	/**
	 * Calculates number of namespaces defined on current element.
	 *
	 * @return A number of namespaces defined on current element.
	 */
	private int nsNum()
	{
		if (mNsNum < 0) {
			int count = 0;
			for(Pair nsdec = mPref; nsdec != null; nsdec = nsdec.next) {
				if (nsdec.list == mElm)
					count++;
				else
					break;
			}
			mNsNum = count;
		}

		return mNsNum;
	}
}
