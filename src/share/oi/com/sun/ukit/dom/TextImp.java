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

import org.w3c.dom.Text;
import org.w3c.dom.CDATASection;
import org.w3c.dom.CharacterData;
import org.w3c.dom.Node;
import org.w3c.dom.Document;
import org.w3c.dom.DOMException;

/**
 * DOM text node implementation.
 *
 * @see org.w3c.dom.Node
 */

/* pkg */ final class TextImp
	extends XCharData
	implements Text, CDATASection
{
	/**
	 * Constructs text object from other text.
	 */
	 /* pkg */ TextImp(TextImp node, boolean deep)
	 {
	 	super(node, deep);
	 }

	/**
	 * Constructs text object from data and its owner document.
	 */
	 /* pkg */ TextImp(int type, String data, XDoc ownerDocument)
	 {
	 	super((type == TEXT_NODE)? "#text": "#cdata-section", ownerDocument, data);
	 }

	/**
	 * A code representing the type of the underlying object, as defined above.
	 */
	public short getNodeType()
	{
		return (getNodeName().equals("#text"))? TEXT_NODE: CDATA_SECTION_NODE;
	}

	/**
	 * Breaks this node into two nodes at the specified <code>offset</code>, 
	 * keeping both in the tree as siblings. After being split, this node 
	 * will contain all the content up to the <code>offset</code> point. A 
	 * new node of the same type, which contains all the content at and 
	 * after the <code>offset</code> point, is returned. If the original 
	 * node had a parent node, the new node is inserted as the next sibling 
	 * of the original node. When the <code>offset</code> is equal to the 
	 * length of this node, the new node has no data.
	 *
	 * @param offset The 16-bit unit offset at which to split, starting from 
	 *   <code>0</code>.
	 * @return The new node, of the same type as this node.
	 * @exception DOMException
	 *   INDEX_SIZE_ERR: Raised if the specified offset is negative or greater 
	 *   than the number of 16-bit units in <code>data</code>.
	 *   <br>NO_MODIFICATION_ALLOWED_ERR: Raised if this node is readonly.
	 */
	public Text splitText(int offset)
		throws DOMException
	{
		String tail = substringData(offset, getLength() - offset);
		deleteData(offset, getLength() - offset);

		Text node = (getNodeName().equals("#text"))?
			getOwnerDocument().createTextNode(tail): 
			getOwnerDocument().createCDATASection(tail);

		Node parent = getParentNode();
		if (parent != null) {
			Node next = getNextSibling();
			if (next != null)
				node = (Text)parent.insertBefore(node, next);
			else
				node = (Text)parent.appendChild(node);
		}

		return node;
	}

	/**
	 * Returns string representation of the text or CDATA.
	 */
	public String toString()
	{
		if (getNodeName().equals("#text"))
			return getData();

		StringBuffer out = new StringBuffer();

		out.append("<![CDATA[");
		out.append(getData());
		out.append("]]>");
		
		return out.toString();
	}
}
