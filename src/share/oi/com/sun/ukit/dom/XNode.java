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

import org.w3c.dom.Node;
import org.w3c.dom.Document;

/**
 * Implementation of DOM events propagation layer.
 */

public abstract class XNode
	extends XBase
{
	/**
	 * Constructs node object from its owner document.
	 */
	protected XNode(XDoc ownerDocument)
	{
	 	super(ownerDocument);
	}

	/**
	 * Constructs node object from other node.
	 */
	 protected XNode(XNode node, boolean deep)
	 {
		super(node, deep);
	 }

	/**
	 * Constructs node object from its qualified name and namespace URI and 
	 * its owner document.
	 */
	 protected XNode(String namespaceURI, String qName, XDoc ownerDocument)
	 {
		super(namespaceURI, qName, ownerDocument);
	 }

	/**
	 * A code representing the type of the underlying object, as defined above.
	 */
	public abstract short getNodeType();

	/**
	 * Notification of child addition.
	 */
	protected void _childAdded(XNode child)
	{
	}

	/**
	 * Notification of child removal.
	 */
	protected void _childRemoving(XNode child)
	{
	}

	/**
	 * Notification of a child have been removed.
	 */
	protected void _childRemoved(XNode child)
	{
	}

	/**
	 * Notification of attribute added.
	 */
	protected void _attrAdded(String name, String value)
	{
	}

	/**
	 * Notification of attribute change.
	 */
	protected void _attrChanged(String name, String oldValue, String newValue)
	{
	}

	/**
	 * Notification of attribute removed.
	 */
	protected void _attrRemoved(String name, String value)
	{
	}

	/**
	 * Notification of data change.
	 */
	protected void _dataChanged(String oldData, String newData)
	{
	}
}
