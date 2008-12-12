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

import org.w3c.dom.CharacterData;
import org.w3c.dom.Document;
import org.w3c.dom.NodeList;
import org.w3c.dom.DOMException;

/**
 * DOM character data node implementation.
 *
 * @see org.w3c.dom.Node
 */

public abstract class XCharData
	extends XNode
	implements CharacterData
{
	/** Empty node list */
	private final static NodeList empty = new XList();

	/** Character data. */
	private String chdat;

	/**
	 * Constructs element object from other element.
	 */
	 protected XCharData(XCharData node, boolean deep)
	 {
	 	super(node, deep);

	 	chdat = node.chdat;
	 }

	/**
	 * Constructs character data object from node name and its owner document.
	 */
	 protected XCharData(String nodename, XDoc ownerDocument, String data)
	 {
	 	super(null, nodename, ownerDocument);
	 	
	 	chdat = data;
	 }

	/**
	 * A code representing the type of the underlying object, as defined above.
	 */
	public abstract short getNodeType();

	/**
	 * The value of this node, depending on its type. 
	 * When it is defined to be <code>null</code>, setting it has no effect.
	 *
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
	 *
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
	 * A <code>NodeList</code> that contains all children of this node. If 
	 * there are no children, this is a <code>NodeList</code> containing no 
	 * nodes.
	 */
	public NodeList getChildNodes()
	{
		return empty;
	}

	/**
	 * The character data of the node that implements this interface. The DOM 
	 * implementation may not put arbitrary limits on the amount of data 
	 * that may be stored in a <code>CharacterData</code> node. However, 
	 * implementation limits may mean that the entirety of a node's data may 
	 * not fit into a single <code>DOMString</code>. In such cases, the user 
	 * may call <code>substringData</code> to retrieve the data in 
	 * appropriately sized pieces.
	 *
	 * @exception DOMException
	 *   DOMSTRING_SIZE_ERR: Raised when it would return more characters than 
	 *   fit in a <code>DOMString</code> variable on the implementation 
	 *   platform.
	 */
	public String getData()
		throws DOMException
	{
		return chdat;
	}

	/**
	 * The character data of the node that implements this interface. The DOM 
	 * implementation may not put arbitrary limits on the amount of data 
	 * that may be stored in a <code>CharacterData</code> node. However, 
	 * implementation limits may mean that the entirety of a node's data may 
	 * not fit into a single <code>DOMString</code>. In such cases, the user 
	 * may call <code>substringData</code> to retrieve the data in 
	 * appropriately sized pieces.
	 *
	 * @exception DOMException
	 *   NO_MODIFICATION_ALLOWED_ERR: Raised when the node is readonly.
	 * @exception DOMException
	 *   DOMSTRING_SIZE_ERR: Raised when it would return more characters than 
	 *   fit in a <code>DOMString</code> variable on the implementation 
	 *   platform.
	 */
	public void setData(String data)
		throws DOMException
	{
		if (_isRO())
			throw new DOMException(DOMException.NO_MODIFICATION_ALLOWED_ERR, "");

		String olddat = chdat;

		chdat = (data != null)? data: "";

		_dataChanged(olddat, chdat);
	}

	/**
	 * The number of 16-bit units that are available through <code>data</code> 
	 * and the <code>substringData</code> method below. This may have the 
	 * value zero, i.e., <code>CharacterData</code> nodes may be empty.
	 */
	public int getLength()
	{
		return chdat.length();
	}

	/**
	 * Extracts a range of data from the node.
	 *
	 * @param offset Start offset of substring to extract.
	 * @param count The number of 16-bit units to extract.
	 * @return The specified substring. If the sum of <code>offset</code> and 
	 *   <code>count</code> exceeds the <code>length</code>, then all 16-bit 
	 *   units to the end of the data are returned.
	 * @exception DOMException
	 *   INDEX_SIZE_ERR: Raised if the specified <code>offset</code> is 
	 *   negative or greater than the number of 16-bit units in 
	 *   <code>data</code>, or if the specified <code>count</code> is 
	 *   negative.
	 *   <br>DOMSTRING_SIZE_ERR: Raised if the specified range of text does 
	 *   not fit into a <code>DOMString</code>.
	 */
	public String substringData(int offset, int count)
		throws DOMException
	{
		try {
			int tail = offset + count;
			if (tail > chdat.length())
				tail = chdat.length();
			return chdat.substring(offset, tail);
		} catch (IndexOutOfBoundsException iob) {
			throw new DOMException(DOMException.INDEX_SIZE_ERR, "");
		}
	}

	/**
	 * Append the string to the end of the character data of the node. Upon 
	 * success, <code>data</code> provides access to the concatenation of 
	 * <code>data</code> and the <code>DOMString</code> specified.
	 *
	 * @param arg The <code>DOMString</code> to append.
	 * @exception DOMException
	 *   NO_MODIFICATION_ALLOWED_ERR: Raised if this node is readonly.
	 */
	public void appendData(String arg)
		throws DOMException
	{
		if (_isRO())
			throw new DOMException(DOMException.NO_MODIFICATION_ALLOWED_ERR, "");

		String olddat = chdat;

		if (arg != null && arg.length() > 0)
			chdat = chdat.concat(arg);

		_dataChanged(olddat, chdat);
	}

	/**
	 * Insert a string at the specified 16-bit unit offset.
	 *
	 * @param offset The character offset at which to insert.
	 * @param arg The <code>DOMString</code> to insert.
	 * @exception DOMException
	 *   INDEX_SIZE_ERR: Raised if the specified <code>offset</code> is 
	 *   negative or greater than the number of 16-bit units in 
	 *   <code>data</code>.
	 *   <br>NO_MODIFICATION_ALLOWED_ERR: Raised if this node is readonly.
	 */
	public void insertData(int offset, String arg)
		throws DOMException
	{
		if (_isRO())
			throw new DOMException(DOMException.NO_MODIFICATION_ALLOWED_ERR, "");

		String olddat = chdat;

		try {
			String head = chdat.substring(0, offset);
			chdat = head + arg + chdat.substring(offset);
		} catch (IndexOutOfBoundsException iob) {
			throw new DOMException(DOMException.INDEX_SIZE_ERR, "");
		}

		_dataChanged(olddat, chdat);
	}

	/**
	 * Remove a range of 16-bit units from the node. Upon success, 
	 * <code>data</code> and <code>length</code> reflect the change.
	 *
	 * @param offset The offset from which to start removing.
	 * @param count The number of 16-bit units to delete. If the sum of 
	 *   <code>offset</code> and <code>count</code> exceeds 
	 *   <code>length</code> then all 16-bit units from <code>offset</code> 
	 *   to the end of the data are deleted.
	 * @exception DOMException
	 *   INDEX_SIZE_ERR: Raised if the specified <code>offset</code> is 
	 *   negative or greater than the number of 16-bit units in 
	 *   <code>data</code>, or if the specified <code>count</code> is 
	 *   negative.
	 *   <br>NO_MODIFICATION_ALLOWED_ERR: Raised if this node is readonly.
	 */
	public void deleteData(int offset, int count)
		throws DOMException
	{
		if (_isRO())
			throw new DOMException(DOMException.NO_MODIFICATION_ALLOWED_ERR, "");
		if (offset < 0 || count < 0 || offset > chdat.length())
			throw new DOMException(DOMException.INDEX_SIZE_ERR, "");

		String olddat = chdat;

		try {
			String head = chdat.substring(0, offset);
			if ((offset + count) < chdat.length())
				chdat = head + chdat.substring(offset + count);
			else
				chdat = head;
		} catch (IndexOutOfBoundsException iob) {
			throw new DOMException(DOMException.INDEX_SIZE_ERR, "");
		}

		_dataChanged(olddat, chdat);
	}

	/**
	 * Replace the characters starting at the specified 16-bit unit offset 
	 * with the specified string.
	 *
	 * @param offset The offset from which to start replacing.
	 * @param count The number of 16-bit units to replace. If the sum of 
	 *   <code>offset</code> and <code>count</code> exceeds 
	 *   <code>length</code>, then all 16-bit units to the end of the data 
	 *   are replaced; (i.e., the effect is the same as a <code>remove</code>
	 *    method call with the same range, followed by an <code>append</code>
	 *    method invocation).
	 * @param arg The <code>DOMString</code> with which the range must be 
	 *   replaced.
	 * @exception DOMException
	 *   INDEX_SIZE_ERR: Raised if the specified <code>offset</code> is 
	 *   negative or greater than the number of 16-bit units in 
	 *   <code>data</code>, or if the specified <code>count</code> is 
	 *   negative.
	 *   <br>NO_MODIFICATION_ALLOWED_ERR: Raised if this node is readonly.
	 */
	public void replaceData(int offset, int count, String arg)
		throws DOMException
	{
		if (_isRO())
			throw new DOMException(DOMException.NO_MODIFICATION_ALLOWED_ERR, "");
		if (offset < 0 || count < 0 || offset > chdat.length())
			throw new DOMException(DOMException.INDEX_SIZE_ERR, "");

		String olddat = chdat;

		try {
			String head = chdat.substring(0, offset);
			int num = (offset + count <= chdat.length())? 
				count: chdat.length() - offset;
			chdat = head + arg + chdat.substring(offset + num);
		} catch (IndexOutOfBoundsException iob) {
			throw new DOMException(DOMException.INDEX_SIZE_ERR, "");
		}

		_dataChanged(olddat, chdat);
	}

	/**
	 * Notification of data change.
	 */
	protected void _dataChanged(String oldData, String newData)
	{
		super._dataChanged(oldData, newData);

		XNode attr = (XNode)_getParent();
		if (attr != null && attr.getNodeType() == ATTRIBUTE_NODE)
			attr._updateValue(attr.getTextContent());
	}
}
