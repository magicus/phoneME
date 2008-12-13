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

package com.sun.ukit.dom;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.XMLConstants;

/**
 * DOM document builder factory implementation.
 *
 * @see javax.xml.parsers.DocumentBuilderFactory
 */

public final class DocBuilderFactory
	extends DocumentBuilderFactory
{
	public final static String FEATURE_NS = 
		"http://xml.org/sax/features/namespaces";

	private boolean secproc = false;

	/** 
	 * Creates a new instance of a {@link javax.xml.parsers.DocumentBuilder}
	 * using the currently configured parameters.
	 *
	 * @exception ParserConfigurationException if a DocumentBuilder
	 * cannot be created which satisfies the configuration requested.
	 * @return A new instance of a DocumentBuilder.
	 */
	public DocumentBuilder newDocumentBuilder()
		throws ParserConfigurationException
	{
		if (isValidating() != false ||
			isIgnoringElementContentWhitespace() != false ||
			isExpandEntityReferences() != true ||
			isIgnoringComments() != false ||
			isCoalescing() != false)
			throw new ParserConfigurationException("");
		return new DocBuilder(isNamespaceAware());
	}

	/** 
	 * Allows the user to set specific attributes on the underlying
	 * implementation.
	 *
	 * @param name The name of the attribute.
	 * @param value The value of the attribute.
	 * @exception IllegalArgumentException thrown if the underlying
	 *   implementation doesn't recognize the attribute.
	 */
	public void setAttribute(String name, Object value)
		throws IllegalArgumentException
	{
		throw new IllegalArgumentException();
	}

	/** 
	 * Allows the user to retrieve specific attributes on the underlying
	 * implementation.
	 *
	 * @param name The name of the attribute.
	 * @return value The value of the attribute.
	 * @exception IllegalArgumentException thrown if the underlying
	 *   implementation doesn't recognize the attribute.
	 */
	public Object getAttribute(String name)
		throws IllegalArgumentException
	{
		throw new IllegalArgumentException();
	}

	/** 
	 * <p>Set a feature for this <code>DocumentBuilderFactory</code> and 
	 * <code>DocumentBuilder</code>s created by this factory.</p>
	 * <p>
	 * Feature names are fully qualified URIs. Implementations may define their 
	 * own features. An {@link ParserConfigurationException} is thrown if this 
	 * <code>DocumentBuilderFactory</code> or the
	 * <code>DocumentBuilder</code>s it creates cannot support the feature.
	 * It is possible for an <code>DocumentBuilderFactory</code> to expose a 
	 * feature value but be unable to change its state.</p>
	 * <p>
	 * All implementations are required to support the 
	 * {@link javax.xml.XMLConstants#FEATURE_SECURE_PROCESSING} feature.
	 * When the feature is:</p>
	 * <ul>
	 *   <li>
	 *     <code>true</code>: the implementation will limit XML processing to 
	 *     conform to implementation limits.
	 *     Examples include entity expansion limits and XML Schema constructs 
	 *     that would consume large amounts of resources.
	 *     If XML processing is limited for security reasons, it will be 
	 *     reported via a call to the registered
	 *     {@link org.xml.sax.ErrorHandler#fatalError(SAXParseException exception)}.
	 *     See 
	 *     {@link DocumentBuilder#setErrorHandler(org.xml.sax.ErrorHandler errorHandler)}.
	 *   </li>
	 *   <li>
	 *     <code>false</code>: the implementation will processing XML according 
	 *     to the XML specifications without regard to possible implementation 
	 *     limits.
	 *   </li>
	 * </ul>
	 * 
	 * @param name Feature name.
	 * @param value Is feature state <code>true</code> or <code>false</code>.
	 * @throws ParserConfigurationException if this 
	 *   <code>DocumentBuilderFactory</code> or the 
	 *   <code>DocumentBuilder</code>s it creates cannot support this feature.
	 * @throws NullPointerException If the <code>name</code> parameter is null.
	 */
	public void setFeature(String name, boolean value)
		throws ParserConfigurationException
	{
		if (name == null)
			throw new NullPointerException("");
		if (FEATURE_NS.equals(name)) {
			setNamespaceAware(value);
		} else if (XMLConstants.FEATURE_SECURE_PROCESSING.equals(name)) {
			secproc = value;
		} else {
			throw new ParserConfigurationException(name);
		}
	}

	/** 
	 * <p>Get the state of the named feature.</p>
	 * <p>
	 * Feature names are fully qualified URIs. Implementations may define their 
	 * own features. An {@link ParserConfigurationException} is thrown if this 
	 * <code>DocumentBuilderFactory</code> or the 
	 * <code>DocumentBuilder</code>s it creates cannot support the feature.
	 * It is possible for an <code>DocumentBuilderFactory</code> to expose a 
	 * feature value but be unable to change its state.</p>
	 * 
	 * @param name Feature name.
	 * 
	 * @return State of the named feature.
	 * 
	 * @throws ParserConfigurationException if this 
	 *   <code>DocumentBuilderFactory</code> or the 
	 *   <code>DocumentBuilder</code>s it creates cannot support this feature.
	 */
	public boolean getFeature(String name)
		throws ParserConfigurationException
	{
		if (name.equals(FEATURE_NS) == true) {
			return isNamespaceAware();
		} else if (XMLConstants.FEATURE_SECURE_PROCESSING.equals(name)) {
			return secproc;
		} else {
			throw new ParserConfigurationException(name);
		}
	}
}
