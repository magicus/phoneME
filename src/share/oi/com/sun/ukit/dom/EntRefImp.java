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

import java.lang.ref.Reference;
import java.lang.ref.WeakReference;

import org.w3c.dom.EntityReference;
import org.w3c.dom.Node;
import org.w3c.dom.Document;
import org.w3c.dom.DocumentType;
import org.w3c.dom.DOMException;

/**
 * DOM entity reference node implementation.
 *
 * @see org.w3c.dom.Node
 */

/* pkg */ final class EntRefImp
	extends XParent
	implements EntityReference
{
	/**
	 * Constructs entity reference object from other entity reference.
	 */
	 /* pkg */ EntRefImp(EntRefImp node, boolean deep)
	 {
	 	super(node, deep);
	 }

	/**
	 * Constructs element object from its qualified name and namespace URI and 
	 * its owner document.
	 */
	 /* pkg */ EntRefImp(String entName, XDoc ownerDocument)
	 {
	 	super(null, entName, ownerDocument);

		DocumentType doctyp = _getDoc().getDoctype();
		if (doctyp != null) {
			//		Get corresponding entity node
			Node ent = doctyp.getEntities().getNamedItem(getNodeName());
			if (ent != null && ent.getChildNodes().getLength() != 0) {
				//		There should be only one child node - Text
				Node txt = ent.getChildNodes().item(0);
				_appendChild((XNode)_getDoc().createTextNode(txt.getNodeValue()));
			}
		}
	 }

	/**
	 * A code representing the type of the underlying object, as defined above.
	 */
	public short getNodeType()
	{
		return ENTITY_REFERENCE_NODE;
	}

	/**
	 * Return true if node or one of its parents is read only.
	 */
	protected boolean _isRO()
	{
		return true;
	}

	/**
	 * Appends textual content of the node to the buffer.
	 */
	protected void _appendText(StringBuffer buffer)
	{
		if (getLength() != 0)
			super._appendText(buffer);
		else
			buffer.append('&' + getNodeName() + ';');
	}

	/**
	 * Returns string representation of the element.
	 */
	public String toString()
	{
		return '&' + getNodeName() + ';';
	}
}
