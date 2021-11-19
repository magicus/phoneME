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
import org.w3c.dom.NodeList;

/**
 * DOM node list implementation.
 *
 * @see org.w3c.dom.NodeList
 */

public class XList 
	implements NodeList
{
	/** Default children list capacity. */
	private final static int DEFAULT_NODE_NUM = 32;
	/** Children list. */
	private XNode[] nlst;
	/** Number of children in the list. */
	private int nnum;
	/** Link for single linked list */
	/* pkg */ XList next;

	/**
	 * Constructs default node list object. 
	 */
	public XList()
	{
		nlst = new XNode[DEFAULT_NODE_NUM];
	}

	/**
	 * Returns the <code>index</code>th item in the collection. If 
	 * <code>index</code> is greater than or equal to the number of nodes in 
	 * the list, this returns <code>null</code>.
	 *
	 * @param index Index into the collection.
	 * @return The node at the <code>index</code>th position in the 
	 *   <code>NodeList</code>, or <code>null</code> if that is not a valid 
	 *   index.
	 */
	public Node item(int index)
	{
		return (index >= 0 && index < nnum)? nlst[index]: null;
	}

	/**
	 * The number of nodes in the list. The range of valid child node indices 
	 * is 0 to <code>length - 1</code> inclusive. 
	 */
	public int getLength()
	{
		return nnum;
	}

	/**
	 * Processes a node.
	 *
	 * @parm node A node to be processed.
	 */
	protected void _proc(XNode node)
	{
	}

	/**
	 * Processes an element node.
	 *
	 * @parm node An element to be processed.
	 */
	protected void _proc(XElm elm)
	{
	}

	/**
	 * Appends a node to the list.
	 *
	 * @parm node A node to append to the list.
	 */
	protected final void _append(XNode node)
	{
		if (nnum >= nlst.length) {
			XNode list[] = new XNode[nlst.length + DEFAULT_NODE_NUM];
			System.arraycopy(nlst, 0, list, 0, nnum);
			nlst = list;
		}
		nlst[nnum++] = node;
	}

	/**
	 * Removes a node from the list.
	 *
	 * @parm index A node to append to the list.
	 * @return Node which was removed or null if index is out of boundary.
	 */
	protected final XNode _remove(int index)
	{
		if (index < 0 || index >= nnum)
			return null;

		XNode node = nlst[index];
		if (index < (nnum - 1))
			System.arraycopy(nlst, index + 1, nlst, index, nnum - index);
		nnum -= 1;

		return node;
	}

	/**
	 * Removes all nodes from the list.
	 */
	protected final void _empty()
	{
		while (nnum > 0)
			nlst[--nnum] = null;
	}

	/**
	 * Searches a node in the list. 
	 *
	 * @parm node A node to find in the list.
	 * @return Index of the matching node or -1 if no match was found.
	 */
	protected final int _find(Node node)
	{
		for (int idx = 0; idx < nnum; idx++) {
			if (nlst[idx] == node)
				return idx;
		}

		return -1;
	}

	/**
	 * Searches a node in the list. This method uses Node.getNodeName method 
	 * to find a match with provided name argument.
	 *
	 * @parm name A node name to find in the list.
	 * @return Index of the matching node or -1 if no match was found.
	 */
	protected final int _find(String name)
	{
		for (int idx = 0; idx < nnum; idx++) {
			if (name.equals(nlst[idx].getNodeName()))
				return idx;
		}

		return -1;
	}

	/**
	 * Searches a node in the list. This method uses Node.getLocalName and 
	 * Node.getNamespaceURI methods to find a match with provided nsuri and 
	 * lname arguments.
	 *
	 * @parm nsuri A node namespace URI to find in the list.
	 * @parm lname A node local name to find in the list.
	 * @return Index of the matching node or -1 if no match was found.
	 */
	protected final int _find(String nsuri, String lname)
	{
		for (int idx = 0; idx < nnum; idx++) {
			XNode node = nlst[idx];
			if (lname.equals(node.getLocalName()) && 
				nsuri.equals(node.getNamespaceURI()))
				return idx;
		}

		return -1;
	}
}
