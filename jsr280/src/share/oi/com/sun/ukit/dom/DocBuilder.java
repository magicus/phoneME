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

package com.sun.ukit.dom;

import javax.xml.parsers.DocumentBuilder;

import java.io.InputStream;
import java.io.IOException;

import org.w3c.dom.Document;
import org.w3c.dom.DOMImplementation;

import org.xml.sax.InputSource;
import org.xml.sax.EntityResolver;
import org.xml.sax.ErrorHandler;
import org.xml.sax.SAXException;

/**
 * DOM document builder implementation.
 *
 * @see javax.xml.parsers.DocumentBuilder
 */

/* pkg */ final class DocBuilder
	extends DocumentBuilder
{
	/** Namespace aware flag for parser. */
	private boolean namespaces;

	/** DOM implementation instance. */
	private static DOMImpImp domimp;

	/** SAX error handler. */
	private ErrorHandler   handErr;
	/** SAX entity resolver. */
	private EntityResolver handEnt;

	/**
	 * Constructor.
	 */
	/* pkg */ DocBuilder(boolean isNSAware)
	{
		namespaces = isNSAware;
	}

	/** 
	 * Parse the content of the given input source as an XML document
	 * and return a new DOM {@link Document} object.
	 * An <code>IllegalArgumentException</code> is thrown if the
	 * <code>InputSource</code> is <code>null</code> null.
	 *
	 * @param is InputSource containing the content to be parsed.
	 * @exception IOException If any IO errors occur.
	 * @exception SAXException If any parse errors occur.
	 * @return A new DOM Document object.
	 */
	public Document parse(InputSource is)
		throws SAXException, IOException
	{
		if (is == null)
			throw new IllegalArgumentException();

		ParserDOM parser = new ParserDOM(namespaces, (XDoc)newDocument());
		parser.mHandErr = handErr;
		parser.mHandEnt = handEnt;
		return parser.parse(is);
	}

	/** 
	 * Indicates whether or not this parser is configured to
	 * understand namespaces.
	 *
	 * @return <code>true</code> if this parser is configured to understand
	 *         namespaces; <code>false</code> otherwise.
	 */
	public boolean isNamespaceAware()
	{
		return namespaces;
	}

	/** 
	 * Indicates whether or not this parser is configured to
	 * validate XML documents.
	 *
	 * @return <code>true</code> if this parser is configured to validate
	 *         XML documents; <code>false</code> otherwise.
	 */
	public boolean isValidating()
	{
		return false;
	}

	/** 
	 * Specify the {@link EntityResolver} to be used to resolve
	 * entities present in the XML document to be parsed. Setting
	 * this to <code>null</code> will result in the underlying
	 * implementation using it's own default implementation and
	 * behavior.
	 *
	 * @param er The <code>EntityResolver</code> to be used to resolve entities
	 *           present in the XML document to be parsed.
	 */
	public void setEntityResolver(EntityResolver er)
	{
		handEnt = er;
	}

	/** 
	 * Specify the {@link ErrorHandler} to be used by the parser.
	 * Setting this to <code>null</code> will result in the underlying
	 * implementation using it's own default implementation and
	 * behavior.
	 *
	 * @param eh The <code>ErrorHandler</code> to be used by the parser.
	 */
	public void setErrorHandler(ErrorHandler eh)
	{
		handErr = eh;
	}

	/** 
	 * Obtain a new instance of a DOM {@link Document} object
	 * to build a DOM tree with.
	 *
	 * @return A new instance of a DOM Document object.
	 */
	public Document newDocument()
	{
		return getDOMImplementation().createDocument(null, null, null);
	}

	/** 
	 * Obtain an instance of a {@link DOMImplementation} object.
	 *
	 * @return A new instance of a <code>DOMImplementation</code>.
	 */
	public DOMImplementation getDOMImplementation()
	{
		if (domimp == null)
			domimp = new DOMImpImp();

		return domimp;
	}
}
