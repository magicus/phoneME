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

import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.UnsupportedEncodingException;
import java.io.Writer;

import javax.xml.stream.XMLOutputFactory;
import javax.xml.stream.XMLStreamException;
import javax.xml.stream.XMLStreamWriter;

/**
 * XMLOutputFactory implementation.
 *
 * @see XMLOutputFactory
 * @see XMLStreamWriter
 */
public class XMLOutputFactoryImp
	extends XMLOutputFactory
{
	private Boolean mIsRepNs = Boolean.FALSE;

	/**
	 * Constructor
	 */
	public XMLOutputFactoryImp()
	{
		super();
	}

	/**
	 * Create a new XMLStreamWriter that writes to a writer
	 *
	 * @param stream the writer to write to, may not be null
	 * @throws XMLStreamException
	 * @throws NullPointerException
	 */
	public XMLStreamWriter createXMLStreamWriter(Writer stream)
		throws XMLStreamException
	{
		return (mIsRepNs.booleanValue() == false)? 
			(XMLStreamWriter)(new WriterNoRepStAX(stream)): 
			(XMLStreamWriter)(new WriterRepStAX(stream));
	}

	/**
	 * Create a new XMLStreamWriter that writes to a stream
	 *
	 * @param stream the stream to write to, may not be null
	 * @throws XMLStreamException
	 * @throws NullPointerException
	 */
	public XMLStreamWriter createXMLStreamWriter(OutputStream stream)
		throws XMLStreamException
	{
		Writer wr = new OutputStreamWriter(stream);

		return (mIsRepNs.booleanValue() == false)? 
			(XMLStreamWriter)(new WriterNoRepStAX(wr)): new WriterRepStAX(wr);
	}

	/**
	 * Create a new XMLStreamWriter that writes to a stream
	 *
	 * @param stream the stream to write to, may not be null
	 * @param encoding the encoding to use
	 * @throws XMLStreamException
	 * @throws NullPointerException
	 */
	public XMLStreamWriter createXMLStreamWriter(
			OutputStream stream, String encoding)
		throws XMLStreamException
	{
		try {
			Writer wr = new OutputStreamWriter(stream, encoding);
	
			return (mIsRepNs.booleanValue() == false)? 
				(XMLStreamWriter)(new WriterNoRepStAX(wr)): new WriterRepStAX(wr);
		} catch (UnsupportedEncodingException uee) {
			throw new XMLStreamException(uee);
		}
	}

	/**
	 * Allows the user to set specific features/properties on the underlying 
	 * implementation. 
	 *
	 * @param name The name of the property, may not be null
	 * @param value The value of the property
	 * @throws IllegalArgumentException if the property is not supported, or if 
	 *   the specified value is not supported for a (valid) specified property
	 * @throws NullPointerException
	 */
	public void setProperty(String name, Object value)
		throws IllegalArgumentException
	{
		if (isPropertySupported(name) == false)
			throw new IllegalArgumentException(name);
		//		XMLOutputFactory.IS_REPAIRING_NAMESPACES
		if (value instanceof Boolean)
			mIsRepNs = (Boolean)value;
		else
			throw new IllegalArgumentException(name);
	}

	/**
	 * Get a feature/property on the underlying implementation
	 *
	 * @param name The name of the property, may not be null
	 * @return The value of the property
	 * @throws IllegalArgumentException if the property is not supported
	 * @throws NullPointerException
	 */
	public Object getProperty(String name)
		throws IllegalArgumentException
	{
		if (isPropertySupported(name) == false)
			throw new IllegalArgumentException(name);
		return mIsRepNs;
	}

	/**
	 * Query the set of properties that this factory supports.
	 *
	 * @param name The name of the property (may not be null)
	 * @return <code>true</code> if the property is supported and 
	 *   <code>false</code> otherwise
	 * @throws NullPointerException
	 */
	public boolean isPropertySupported(String name)
	{
		return name.equals(XMLOutputFactory.IS_REPAIRING_NAMESPACES);
	}
}
