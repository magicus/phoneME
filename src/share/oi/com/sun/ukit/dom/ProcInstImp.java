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

import org.w3c.dom.ProcessingInstruction;
import org.w3c.dom.Node;
import org.w3c.dom.Document;
import org.w3c.dom.DOMException;

/**
 * DOM processing instruction node implementation.
 *
 * @see org.w3c.dom.Node
 */

/* pkg */ final class ProcInstImp
	extends XNode
	implements ProcessingInstruction
{
	/** Character data. */
	private String pidat;

	/**
	 * Constructs text object from other text.
	 */
	 /* pkg */ ProcInstImp(ProcInstImp node, boolean deep)
	 {
	 	super(node, deep);

	 	pidat = node.pidat;
	 }

	/**
	 * Constructs processing instruction object from its owner document.
	 */
	 /* pkg */ ProcInstImp(String target, String data, XDoc ownerDocument)
	 {
	 	super(null, (target != null)? target: "", ownerDocument);
	 	setData(data);
	 }

	/**
	 * A code representing the type of the underlying object, as defined above.
	 */
	public short getNodeType()
	{
		return PROCESSING_INSTRUCTION_NODE;
	}

	/**
	 * The value of this node, depending on its type. 
	 * When it is defined to be <code>null</code>, setting it has no effect.
	 * @exception DOMException
	 *   NO_MODIFICATION_ALLOWED_ERR: Raised when the node is readonly.
	 * @exception DOMException
	 *   DOMSTRING_SIZE_ERR: Raised when it would return more characters than 
	 *   fit in a <code>DOMString</code> variable on the implementation 
	 *   platform.
	 */
	public String getNodeValue()
		throws DOMException
	{
		return getData();
	}

	/**
	 * The value of this node, depending on its type. 
	 * When it is defined to be <code>null</code>, setting it has no effect.
	 * @exception DOMException
	 *   NO_MODIFICATION_ALLOWED_ERR: Raised when the node is readonly.
	 * @exception DOMException
	 *   DOMSTRING_SIZE_ERR: Raised when it would return more characters than 
	 *   fit in a <code>DOMString</code> variable on the implementation 
	 *   platform.
	 */
	public void setNodeValue(String nodeValue)
		throws DOMException
	{
		setData(nodeValue);
	}
	
	/**
	 * The target of this processing instruction. XML defines this as being 
	 * the first token following the markup that begins the processing 
	 * instruction.
	 */
	public String getTarget()
	{
		return getNodeName();
	}

	/**
	 * The content of this processing instruction. This is from the first non 
	 * white space character after the target to the character immediately 
	 * preceding the <code>?&gt;</code>.
	 */
	public String getData()
	{
		return pidat;
	}

	/**
	 * The content of this processing instruction. This is from the first non 
	 * white space character after the target to the character immediately 
	 * preceding the <code>?&gt;</code>.
	 * @exception DOMException
	 *   NO_MODIFICATION_ALLOWED_ERR: Raised when the node is readonly.
	 */
	public void setData(String data)
		throws DOMException
	{
		if (_isRO())
			throw new DOMException(DOMException.NO_MODIFICATION_ALLOWED_ERR, "");

		String olddat = pidat;

		pidat = (data != null)? data: "";

		_dataChanged(olddat, pidat);
	}

	/**
	 * Returns string representation of the text or CDATA.
	 */
	public String toString()
	{
		StringBuffer out = new StringBuffer();

		out.append("<?");
		out.append(getNodeName());
		out.append(" ");
		out.append(getData());
		out.append("?>");
		
		return out.toString();
	}
}
