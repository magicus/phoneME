/*
 *  
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

import javax.xml.parsers.SAXParser;
import javax.xml.parsers.SAXParserFactory;
import javax.xml.parsers.ParserConfigurationException;

import org.xml.sax.SAXException;
import org.xml.sax.SAXNotRecognizedException;
import org.xml.sax.SAXNotSupportedException;

/**
 * Implementation of JAXP SAXParserFactory.
 */
public class ParserFactory
	extends SAXParserFactory
{
	public final static String FEATURE_NS   = "http://xml.org/sax/features/namespaces";
	public final static String FEATURE_PREF = "http://xml.org/sax/features/namespace-prefixes";

	private boolean namespaces = false;
	private boolean prefixes   = true;

    /**
     * Creates a new instance of a SAXParser using the currently
     * configured factory parameters.
     *
     * @return A new instance of a SAXParser.
     *
     * @exception ParserConfigurationException if a parser cannot
     * be created which satisfies the requested configuration.
     */
	public SAXParser newSAXParser()
		throws ParserConfigurationException, SAXException
	{
		if ((namespaces == true) && (prefixes == false)) {
			return new Parser(true);
		} else if ((namespaces == false) && (prefixes == true)) {
			return new Parser(false);
		} else {
			throw new ParserConfigurationException("");
		}
	}

    /**
     * Specifies that the parser produced by this code will
     * provide support for XML namespaces. By default the value of this is set
     * to <code>false</code>.
     *
     * @param awareness true if the parser produced by this code will
     *                  provide support for XML namespaces; false otherwise.
     */
    public void setNamespaceAware(boolean awareness)
    {
        super.setNamespaceAware(awareness);
        if (awareness == true) {
        	namespaces = true;
        	prefixes   = false;
        } else {
        	namespaces = false;
        	prefixes   = true;
        }
    }

    /**
     * Sets the particular feature in the underlying implementation of
     * DefaultHandler.
     * A list of the core features and properties can be found at
     * <a href="http://www.saxproject.org/?selected=get-set">
     * http://www.saxproject.org/?selected=get-set </a>
     *
     * @param name The name of the feature to be set.
     * @param value The value of the feature to be set.
     * @exception SAXNotRecognizedException When the underlying DefaultHandler does
     *            not recognize the property name.
     *
     * @exception SAXNotSupportedException When the underlying DefaultHandler
     *            recognizes the property name but doesn't support the
     *            property.
     */
	public void setFeature(String name, boolean value)
		throws ParserConfigurationException, SAXNotRecognizedException, SAXNotSupportedException 
	{
		if (FEATURE_NS.equals(name) == true) {
			namespaces = value;
		} else if (FEATURE_PREF.equals(name) == true) {
			prefixes   = value;
		} else {
			throw new SAXNotRecognizedException(name);
		}
	}

    /**
     * Returns the particular property requested for in the underlying
     * implementation of DefaultHandler.
     *
     * @param name The name of the property to be retrieved.
     * @return Value of the requested property.
     *
     * @exception SAXNotRecognizedException When the underlying DefaultHandler does
     *            not recognize the property name.
     *
     * @exception SAXNotSupportedException When the underlying DefaultHandler
     *            recognizes the property name but doesn't support the
     *            property.
     */
	public boolean getFeature(String name)
		throws ParserConfigurationException, SAXNotRecognizedException, SAXNotSupportedException 
	{
		if (FEATURE_NS.equals(name) == true) {
			return namespaces;
		} else if (FEATURE_PREF.equals(name) == true) {
			return prefixes;
		} else {
			throw new SAXNotRecognizedException(name);
		}
	}
}
