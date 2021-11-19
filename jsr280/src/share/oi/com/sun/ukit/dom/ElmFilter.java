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

/**
 * Element filter.
 *
 * @see com.sun.ukit.dom.XList
 */

public final class ElmFilter 
	extends XList
{
	private String name;
	private String nsuri;

	// 0 - nsuri name
	// 1 - nsuri '*'
	// 2 - '*'   name
	// 3 - '*'   '*'
	// 4 - null  name
	// 5 - null  '*'
	private int type;

	/**
	 * Constructs filter from name.
	 *
	 * @parm qname A name to match or "*" to match any.
	 */
	public ElmFilter(String qname)
	{
		type = (qname.equals("*"))? 5: 4;
		name = qname;
	}

	/**
	 * Constructs filter from namespace URI and local name.
	 *
	 * @parm namespaceURI A namespace URI to match or "*" to match any.
	 * @parm localName A name to match or "*" to match any.
	 */
	public ElmFilter(String namespaceURI, String localName)
	{
		type += (localName.equals("*"))? 0x1: 0x0;
		type += (namespaceURI.equals("*"))? 0x2: 0x0;
		name  = localName;
		nsuri = namespaceURI;
	}

	/**
	 * Processes an element node.
	 *
	 * @parm node An element to be processed.
	 */
	protected void _proc(XElm elm)
	{
		switch (type) {
		case 0:  // namespaceURI localName
			if (nsuri.equals(elm.getNamespaceURI()) && name.equals(elm.getLocalName()))
				_append(elm);
			break;

		case 1:  // namespaceURI '*'
			if (nsuri.equals(elm.getNamespaceURI()))
				_append(elm);
			break;

		case 2:  // '*' localName
			if (name.equals(elm.getLocalName()))
				_append(elm);
			break;

		case 3:  // '*' '*'
		case 5:  // null '*'
			_append(elm);
			break;

		case 4:  // null qName
			if (name.equals(elm.getNodeName()))
				_append(elm);
			break;

		default:
		}
	}
}
