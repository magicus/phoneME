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
import java.io.Reader;
import java.lang.IllegalArgumentException;

import javax.xml.stream.XMLInputFactory;
import javax.xml.stream.XMLStreamReader;
import javax.xml.stream.XMLResolver;
import javax.xml.stream.StreamFilter;
import javax.xml.stream.XMLStreamException;

/**
 * Defines an implementation of a factory for getting streams.
 * 
 * @see XMLOutputFactory
 * @see XMLStreamReader
 */

public class XMLInputFactoryImp 
	extends XMLInputFactory 
{
	boolean     mIsNSAware;  // namespace aware parser
	boolean     mIsExtEnt;   // parser supports external entities
	XMLResolver mResolver;   // app provided resolver for external entities

	/**
	 * Create a new XMLInputFactory instance.
	 */
	public XMLInputFactoryImp()
	{
		super();

		mIsNSAware = true;  // default is namespace aware
	}

	/**
	 * Create a new XMLStreamReader from a reader
	 *
	 * @param reader the XML data to read from
	 * @throws XMLStreamException 
	 */
	public XMLStreamReader createXMLStreamReader(Reader reader)
		throws XMLStreamException
	{
		ParserStAX parser = new ParserStAX(reader);
		parser.mIsNSAware = mIsNSAware;
		parser.mResolver  = (mIsExtEnt)? mResolver: null;

		return parser;
	}

	/**
	 * Create a new XMLStreamReader from a java.io.InputStream
	 *
	 * @param stream the InputStream to read from
	 * @throws XMLStreamException 
	 */
	public XMLStreamReader createXMLStreamReader(InputStream stream)
		throws XMLStreamException
	{
		ParserStAX parser = new ParserStAX(stream, null);
		parser.mIsNSAware = mIsNSAware;
		parser.mResolver  = (mIsExtEnt)? mResolver: null;

		return parser;
	}

  /**
	 * Create a new XMLStreamReader from a java.io.InputStream
	 *
	 * @param stream the InputStream to read from
	 * @param encoding the character encoding of the stream
	 * @throws XMLStreamException 
	 */
	public XMLStreamReader createXMLStreamReader(
			InputStream stream, String encoding)
		throws XMLStreamException
	{
		if (encoding == null)
			throw new NullPointerException("");

		ParserStAX parser = new ParserStAX(stream, encoding);
		parser.mIsNSAware = mIsNSAware;
		parser.mResolver  = (mIsExtEnt)? mResolver: null;

		return parser;
	}

	/**
	 * The resolver that will be set on any XMLStreamReader created 
	 * by this factory instance.
	 */
	public XMLResolver getXMLResolver()
	{
		return mResolver;
	}

	/**
	 * The resolver that will be set on any XMLStreamReader created 
	 * by this factory instance.
	 *
	 * @param resolver the resolver to use to resolve references
	 */
	public void setXMLResolver(XMLResolver resolver)
	{
		mResolver = resolver;
	}

	/**
	 * Allows the user to set specific feature/property on the underlying 
	 * implementation. The underlying implementation is not required to support 
	 * every setting of every property in the specification and may use 
	 * IllegalArgumentException to signal that an unsupported property may not 
	 * be set with the specified value.
	 *
	 * @param name The name of the property (may not be <code>null</code>)
	 * @param value The value of the property
	 * @throws IllegalArgumentException if the property is not supported or if 
	 *   the supplied value is illegal for the given property.
	 */
	public void setProperty(String name, Object value) 
		throws IllegalArgumentException
	{
		if ("javax.xml.stream.isValidating".equals(name)) {
			if (!(value instanceof Boolean) || 
				((Boolean)value).booleanValue() != false)
				throw new IllegalArgumentException(name);
		} else if ("javax.xml.stream.isNamespaceAware".equals(name)) {
			if (value instanceof Boolean)
				mIsNSAware = ((Boolean)value).booleanValue();
			else
				throw new IllegalArgumentException(name);
		} else if ("javax.xml.stream.isCoalescing".equals(name)) {
			if (!(value instanceof Boolean) || 
				((Boolean)value).booleanValue() != false)
				throw new IllegalArgumentException(name);
		} else if ("javax.xml.stream.isReplacingEntityReferences".equals(name)) {
			if (!(value instanceof Boolean) || 
				((Boolean)value).booleanValue() == false)
				throw new IllegalArgumentException(name);
		} else if ("javax.xml.stream.isSupportingExternalEntities".equals(name)) {
			if (value instanceof Boolean)
				mIsExtEnt = ((Boolean)value).booleanValue();
			else
				throw new IllegalArgumentException(name);
		} else if ("javax.xml.stream.supportDTD".equals(name)) {
			if (!(value instanceof Boolean) || 
				((Boolean)value).booleanValue() == false)
				throw new IllegalArgumentException(name);
		} else if ("javax.xml.stream.resolver".equals(name)) {
			if (value == null)
				mResolver = null;
			else if (value instanceof XMLResolver)
				mResolver = (XMLResolver)value;
			else
				throw new IllegalArgumentException(name);
		} else {
			throw new IllegalArgumentException(name);
		}
	}

	/**
	 * Get the value of a feature/property from the underlying implementation
	 *
	 * @param name The name of the property (may not be <code>null</code>)
	 * @return The value of the property
	 * @throws IllegalArgumentException if the property is not supported
	 */
	public Object getProperty(String name)
		throws IllegalArgumentException
	{
		if ("javax.xml.stream.isValidating".equals(name)) {
			return Boolean.FALSE;
		} else if ("javax.xml.stream.isNamespaceAware".equals(name)) {
			return (mIsNSAware)? Boolean.TRUE: Boolean.FALSE;
		} else if ("javax.xml.stream.isCoalescing".equals(name)) {
			return Boolean.FALSE;
		} else if ("javax.xml.stream.isReplacingEntityReferences".equals(name)) {
			return Boolean.TRUE;
		} else if ("javax.xml.stream.isSupportingExternalEntities".equals(name)) {
			return (mIsExtEnt)? Boolean.TRUE: Boolean.FALSE;
		} else if ("javax.xml.stream.supportDTD".equals(name)) {
			return Boolean.TRUE;
		} else if ("javax.xml.stream.resolver".equals(name)) {
			return mResolver;
		} else {
			throw new IllegalArgumentException(name);
		}
	}


	/**
	 * Query the set of properties that this factory supports.
	 *
	 * @param name The name of the property (may not be <code>null</code>)
	 * @return <code>true</code> if the property is supported and 
	 *    <code>false</code> otherwise
	 */
	public boolean isPropertySupported(String name)
	{
		if ("javax.xml.stream.isValidating".equals(name)) {
			return true;
		} else if ("javax.xml.stream.isNamespaceAware".equals(name)) {
			return true;
		} else if ("javax.xml.stream.isCoalescing".equals(name)) {
			return true;
		} else if ("javax.xml.stream.isReplacingEntityReferences".equals(name)) {
			return true;
		} else if ("javax.xml.stream.isSupportingExternalEntities".equals(name)) {
			return true;
		} else if ("javax.xml.stream.supportDTD".equals(name)) {
			return true;
		} else if ("javax.xml.stream.resolver".equals(name)) {
			return true;
		}
		return false;
	}

}

