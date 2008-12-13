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

import org.w3c.dom.Entity;
import org.w3c.dom.DOMException;

/**
 * DOM entity node implementation.
 *
 * @see org.w3c.dom.Entity
 */

/* pkg */ final class EntImp
	extends XParent
	implements Entity
{
	private String pubid;
	private String sysid;
	private String notation;

	/**
	 * Constructs entity object from other entity.
	 */
	 /* pkg */ EntImp(EntImp node, boolean deep)
	 {
	 	super(node, deep);

	 	pubid    = node.pubid;
	 	sysid    = node.sysid;
	 	notation = node.notation;
	 }

	/**
	 * Constructs entity node object from its name and owner document.
	 */
	/* pkg */ EntImp(String name, XDoc ownerDocument)
	{
	 	super(null, name, ownerDocument);
	}

	/**
	 * Constructs entity node object from its name and owner document.
	 */
	/* pkg */ EntImp(String name, 
		String pubid, String sysid, String notation, XDoc ownerDocument)
	{
	 	super(null, name, ownerDocument);

	 	this.pubid    = pubid;
	 	this.sysid    = sysid;
	 	this.notation = notation;
	}

	/**
	 * A code representing the type of the underlying object, as defined above.
	 */
	public short getNodeType()
	{
		return ENTITY_NODE;
	}

	/**
	 * The public identifier associated with the entity, if specified. If the 
	 * public identifier was not specified, this is <code>null</code>.
	 * @return The public identifier associated with the entity, or <code>null</code>
	 */
	public String getPublicId()
	{
		return pubid;
	}

	/**
	 * The system identifier associated with the entity, if specified. If the 
	 * system identifier was not specified, this is <code>null</code>.
	 * @return The system identifier associated with the entity or <code>null</code>
	 */
	public String getSystemId()
	{
		return sysid;
	}

	/**
	 * For unparsed entities, the name of the notation for the entity. For 
	 * parsed entities, this is <code>null</code>. 
	 * @return the name of the notation for the entity or <code>null</code>
	 */
	public String getNotationName()
	{
		return notation;
	}

	/**
	 * Return true if node or one of its parents is read only.
	 */
	protected boolean _isRO()
	{
		return true;
	}
}
