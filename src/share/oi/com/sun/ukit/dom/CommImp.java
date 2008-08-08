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

import org.w3c.dom.Comment;
import org.w3c.dom.CharacterData;
import org.w3c.dom.Document;
import org.w3c.dom.DOMException;

/**
 * DOM comment node implementation.
 *
 * @see org.w3c.dom.Node
 */

/* pkg */ final class CommImp
	extends XCharData
	implements Comment
{
	/**
	 * Constructs comment object from other comment.
	 */
	 /* pkg */ CommImp(CommImp node, boolean deep)
	 {
	 	super(node, deep);
	 }

	/**
	 * Constructs text object from data and its owner document.
	 */
	 /* pkg */ CommImp(String data, XDoc ownerDocument)
	 {
	 	super("#comment", ownerDocument, data);
	 }

	/**
	 * A code representing the type of the underlying object, as defined above.
	 */
	public short getNodeType()
	{
		return COMMENT_NODE;
	}

	/**
	 * Returns string representation of the comment.
	 */
	public String toString()
	{
		StringBuffer out = new StringBuffer();

		out.append("<!-- ");
		out.append(getData());
		out.append(" -->");

		return out.toString();
	}
}
