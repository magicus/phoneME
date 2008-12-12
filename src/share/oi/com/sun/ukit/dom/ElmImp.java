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

import org.w3c.dom.Document;

import com.sun.ukit.xml.Pair;

/**
 * DOM element node implementation.
 *
 * @see org.w3c.dom.Node
 */

/* pkg */ final class ElmImp
	extends XElm
{
	/**
	 * Constructs element object from other element.
	 */
	 /* pkg */ ElmImp(ElmImp element, boolean deep)
	 {
	 	super(element, deep);
	 }

	/**
	 * Constructs element object from parser element.
	 */
	 /* pkg */ ElmImp(Pair element, int defcnt, XDoc ownerDocument)
	 {
	 	super(element, defcnt, ownerDocument);
	 }

	/**
	 * Constructs element object from its qualified name and namespace URI and 
	 * its owner document.
	 */
	 /* pkg */ ElmImp(String namespaceURI, String tagName, XDoc ownerDocument)
	 {
	 	super(namespaceURI, tagName, ownerDocument);
	 }

	/**
	 * A code representing the type of the underlying object, as defined above.
	 */
	public short getNodeType()
	{
		return ELEMENT_NODE;
	}
}
