/*
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved. 
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
import java.io.IOException;

import org.xml.sax.XMLReader;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;
import org.xml.sax.helpers.DefaultHandler;

import javax.xml.parsers.SAXParser;

/* pkg */ final class SAXParserImp
	extends SAXParser
{
	private ParserSAX parser;

	/* pkg */ SAXParserImp()
	{
		super();
		parser = new ParserSAX();
	}

	/**
	 * Returns the {@link org.xml.sax.XMLReader} that is encapsulated by the
	 * implementation of this class.
	 *
	 * @return The XMLReader that is encapsulated by the
	 *         implementation of this class.
	 * 
	 * @throws SAXException If any SAX errors occur during processing.
	 */

	public XMLReader getXMLReader()
		throws SAXException
	{
		return parser;
	}

	/**
	 * Indicates whether or not this parser is configured to
	 * understand namespaces.
	 *
	 * @return true if this parser is configured to
	 *         understand namespaces; false otherwise.
	 */
	public boolean isNamespaceAware()
	{
		boolean awareness = false;
		try {
			awareness = parser.getFeature(ParserSAX.FEATURE_NS);
		} catch (SAXException e) {
			// this exception cannot be thrown with argument above
		}
		return awareness;
	}

	/**
	 * Indicates whether or not this parser is configured to validate
	 * XML documents.
	 *
	 * @return true if this parser is configured to validate XML
	 *          documents; false otherwise.
	 */
	public boolean isValidating()
	{
		return false;
	}

	/**
	 * Parse the content of the given {@link java.io.InputStream}
	 * instance as XML using the specified
	 * {@link org.xml.sax.helpers.DefaultHandler}.
	 *
	 * @param src InputStream containing the content to be parsed.
	 * @param handler The SAX DefaultHandler to use.
	 * @exception IOException If any IO errors occur.
	 * @exception IllegalArgumentException If the given InputStream or handler is null.
	 * @exception SAXException If the underlying parser throws a
	 * SAXException while parsing.
	 * @see org.xml.sax.helpers.DefaultHandler
	 */
	public void parse(InputStream src, DefaultHandler handler)
		throws SAXException, IOException
	{
		if ((src == null) || (handler == null))
			throw new IllegalArgumentException("");

		parse(new InputSource(src), handler);
	}

	/**
	 * Parse the content given {@link org.xml.sax.InputSource}
	 * as XML using the specified
	 * {@link org.xml.sax.helpers.DefaultHandler}.
	 *
	 * @param is The InputSource containing the content to be parsed.
	 * @param handler The SAX DefaultHandler to use.
	 * @exception IOException If any IO errors occur.
	 * @exception IllegalArgumentException If the InputSource or handler is null.
	 * @exception SAXException If the underlying parser throws a
	 * SAXException while parsing.
	 * @see org.xml.sax.helpers.DefaultHandler
	 */
	public void parse(InputSource is, DefaultHandler handler)
		throws SAXException, IOException
	{
		if ((is == null) || (handler == null))
			throw new IllegalArgumentException("");

		//		Set up the handler
		parser.setContentHandler(handler);
		parser.setDTDHandler(handler);
		parser.setErrorHandler(handler);
		parser.setEntityResolver(handler);

		parser.parse(is);
	}
}
