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

import org.w3c.dom.NamedNodeMap;
import org.w3c.dom.Node;
import org.w3c.dom.DOMException;

/**
 * Named node map.
 *
 * @see org.w3c.dom.NamedNodeMap
 */
public final class NodeMapImp
	extends XList
	implements NamedNodeMap
{
	/**
	 * Constructs named node map.
	 */
	public NodeMapImp()
	{
		super();
	}

	/**
	 * Retrieves a node specified by name.
	 *
	 * @param name The <code>nodeName</code> of a node to retrieve.
	 * @return A <code>Node</code> (of any type) with the specified 
	 *   <code>nodeName</code>, or <code>null</code> if it does not identify 
	 *   any node in this map.
	 */
	public Node getNamedItem(String name)
	{
		if (name == null)
			throw new NullPointerException();

		return item(_find(name));
	}

	/**
	 * Retrieves a node specified by local name and namespace URI. HTML-only 
	 * DOM implementations do not need to implement this method.
	 *
	 * @param namespaceURI The namespace URI of the node to retrieve.
	 * @param localName The local name of the node to retrieve.
	 * @return A <code>Node</code> (of any type) with the specified local 
	 *   name and namespace URI, or <code>null</code> if they do not 
	 *   identify any node in this map.
	 *
	 * @since DOM Level 2
	 */
	public Node getNamedItemNS(String namespaceURI, String localName)
	{
		if (namespaceURI == null || localName == null)
			throw new NullPointerException();

		return item(_find(namespaceURI, localName));
	}

	/**
	 * Adds a node using its <code>nodeName</code> attribute. If a node with 
	 * that name is already present in this map, it is replaced by the new 
	 * one.
	 * <br>As the <code>nodeName</code> attribute is used to derive the name 
	 * which the node must be stored under, multiple nodes of certain types 
	 * (those that have a "special" string value) cannot be stored as the 
	 * names would clash. This is seen as preferable to allowing nodes to be 
	 * aliased.
	 *
	 * @param arg A node to store in this map. The node will later be 
	 *   accessible using the value of its <code>nodeName</code> attribute.
	 * @return If the new <code>Node</code> replaces an existing node the 
	 *   replaced <code>Node</code> is returned, otherwise <code>null</code> 
	 *   is returned.
	 * @exception DOMException
	 *   WRONG_DOCUMENT_ERR: Raised if <code>arg</code> was created from a 
	 *   different document than the one that created this map.
	 *   <br>NO_MODIFICATION_ALLOWED_ERR: Raised if this map is readonly.
	 *   <br>INUSE_ATTRIBUTE_ERR: Raised if <code>arg</code> is an 
	 *   <code>Attr</code> that is already an attribute of another 
	 *   <code>Element</code> object. The DOM user must explicitly clone 
	 *   <code>Attr</code> nodes to re-use them in other elements.
	 */
	public Node setNamedItem(Node arg)
		throws DOMException
	{
		throw new DOMException(DOMException.NO_MODIFICATION_ALLOWED_ERR, "");
	}

	/**
	 * Adds a node using its <code>namespaceURI</code> and 
	 * <code>localName</code>. If a node with that namespace URI and that 
	 * local name is already present in this map, it is replaced by the new 
	 * one.
	 * <br>HTML-only DOM implementations do not need to implement this method.
	 *
	 * @param arg A node to store in this map. The node will later be 
	 *   accessible using the value of its <code>namespaceURI</code> and 
	 *   <code>localName</code> attributes.
	 * @return If the new <code>Node</code> replaces an existing node the 
	 *   replaced <code>Node</code> is returned, otherwise <code>null</code> 
	 *   is returned.
	 * @exception DOMException
	 *   WRONG_DOCUMENT_ERR: Raised if <code>arg</code> was created from a 
	 *   different document than the one that created this map.
	 *   <br>NO_MODIFICATION_ALLOWED_ERR: Raised if this map is readonly.
	 *   <br>INUSE_ATTRIBUTE_ERR: Raised if <code>arg</code> is an 
	 *   <code>Attr</code> that is already an attribute of another 
	 *   <code>Element</code> object. The DOM user must explicitly clone 
	 *   <code>Attr</code> nodes to re-use them in other elements.
	 *
	 * @since DOM Level 2
	 */
	public Node setNamedItemNS(Node arg)
		throws DOMException
	{
		throw new DOMException(DOMException.NO_MODIFICATION_ALLOWED_ERR, "");
	}

	/**
	 * Removes a node specified by name. When this map contains the attributes 
	 * attached to an element, if the removed attribute is known to have a 
	 * default value, an attribute immediately appears containing the 
	 * default value as well as the corresponding namespace URI, local name, 
	 * and prefix when applicable.
	 *
	 * @param name The <code>nodeName</code> of the node to remove.
	 * @return The node removed from this map if a node with such a name 
	 *   exists.
	 * @exception DOMException
	 *   NOT_FOUND_ERR: Raised if there is no node named <code>name</code> in 
	 *   this map.
	 *   <br>NO_MODIFICATION_ALLOWED_ERR: Raised if this map is readonly.
	 */
	public Node removeNamedItem(String name)
		throws DOMException
	{
		throw new DOMException(DOMException.NO_MODIFICATION_ALLOWED_ERR, "");
	}

	/**
	 * Removes a node specified by local name and namespace URI. A removed 
	 * attribute may be known to have a default value when this map contains 
	 * the attributes attached to an element, as returned by the attributes 
	 * attribute of the <code>Node</code> interface. If so, an attribute 
	 * immediately appears containing the default value as well as the 
	 * corresponding namespace URI, local name, and prefix when applicable.
	 * <br>HTML-only DOM implementations do not need to implement this method.
	 *
	 * @param namespaceURI The namespace URI of the node to remove.
	 * @param localName The local name of the node to remove.
	 * @return The node removed from this map if a node with such a local 
	 *   name and namespace URI exists.
	 * @exception DOMException
	 *   NOT_FOUND_ERR: Raised if there is no node with the specified 
	 *   <code>namespaceURI</code> and <code>localName</code> in this map.
	 *   <br>NO_MODIFICATION_ALLOWED_ERR: Raised if this map is readonly.
	 *
	 * @since DOM Level 2
	 */
	public Node removeNamedItemNS(String namespaceURI, String localName)
		throws DOMException
	{
		throw new DOMException(DOMException.NO_MODIFICATION_ALLOWED_ERR, "");
	}
}
