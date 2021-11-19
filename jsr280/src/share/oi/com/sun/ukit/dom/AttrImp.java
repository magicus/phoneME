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
import org.w3c.dom.Attr;
import org.w3c.dom.Element;
import org.w3c.dom.Document;
import org.w3c.dom.DOMException;

/**
 * DOM attribute node implementation.
 *
 * @see org.w3c.dom.Node
 */

/* pkg */ final class AttrImp
	extends XParent
	implements Attr
{
	/** Attribute value. */
	private String value;

	/**
	 * Constructs attribute object from other attribute.
	 */
	 /* pkg */ AttrImp(AttrImp attr, boolean deep)
	 {
	 	super(attr, true);

	 	value = attr.value;
	 }

	/**
	 * Constructs attribute object from its qualified name and namespace URI 
	 * and its owner document.
	 */
	 /* pkg */ AttrImp(String namespaceURI, String qName, XDoc ownerDocument)
	 {
	 	super(namespaceURI, qName, ownerDocument);
		value = "";
	 }

	/**
	 * A code representing the type of the underlying object, as defined above.
	 */
	public short getNodeType()
	{
		return ATTRIBUTE_NODE;
	}

	/**
	 * The parent of this node. All nodes, except <code>Attr</code>, 
	 * <code>Document</code>, <code>DocumentFragment</code>, 
	 * <code>Entity</code>, and <code>Notation</code> may have a parent. 
	 * However, if a node has just been created and not yet added to the 
	 * tree, or if it has been removed from the tree, this is 
	 * <code>null</code>.
	 */
	public Node getParentNode()
	{
		return null;
	}

	/**
	 * Returns the name of this attribute. 
	 */
	public String getName()
	{
		return getNodeName();
	}

	/**
	 * Sets the namespace prefix of this node.
	 * <br>Note that setting this attribute, when permitted, changes the 
	 * <code>nodeName</code> attribute, which holds the qualified name, as 
	 * well as the <code>tagName</code> and <code>name</code> attributes of 
	 * the <code>Element</code> and <code>Attr</code> interfaces, when 
	 * applicable.
	 * <br>Note also that changing the prefix of an attribute that is known to 
	 * have a default value, does not make a new attribute with the default 
	 * value and the original prefix appear, since the 
	 * <code>namespaceURI</code> and <code>localName</code> do not change.
	 *
	 * @param prefix This node namespace prefix.
	 * @exception DOMException
	 *   INVALID_CHARACTER_ERR: Raised if the specified prefix contains an 
	 *   illegal character.
	 *   <br>NO_MODIFICATION_ALLOWED_ERR: Raised if this node is readonly.
	 *   <br>NAMESPACE_ERR: Raised if the specified <code>prefix</code> is 
	 *   malformed, if the <code>namespaceURI</code> of this node is 
	 *   <code>null</code>, if the specified prefix is "xml" and the 
	 *   <code>namespaceURI</code> of this node is different from "
	 *   http://www.w3.org/XML/1998/namespace", if this node is an attribute 
	 *   and the specified prefix is "xmlns" and the 
	 *   <code>namespaceURI</code> of this node is different from "
	 *   http://www.w3.org/2000/xmlns/", or if this node is an attribute and 
	 *   the <code>qualifiedName</code> of this node is "xmlns" .
	 * @since DOM Level 2
	 */
	public void setPrefix(String prefix)
		throws DOMException
	{
		if (_isNS() == false)
			return;  // created with non-NS methods have null prefix

		XElm elm = (XElm)_getParent();
		if (elm != null) {
			int idx = elm._getAttrIdx(this);
			if (elm._isDefAttr(idx))  // preserve default attributes qname
				return;
			_setPrefix(prefix);
			elm.__setAttrQN(idx, getNodeName());
		} else {
			_setPrefix(prefix);
		}
	}

	/**
	 * If this attribute was explicitly given a value in the original 
	 * document, this is <code>true</code>; otherwise, it is 
	 * <code>false</code>. Note that the implementation is in charge of this 
	 * attribute, not the user. If the user changes the value of the 
	 * attribute (even if it ends up having the same value as the default 
	 * value) then the <code>specified</code> flag is automatically flipped 
	 * to <code>true</code>. To re-specify the attribute as the default 
	 * value from the DTD, the user must delete the attribute. The 
	 * implementation will then make a new attribute available with 
	 * <code>specified</code> set to <code>false</code> and the default 
	 * value (if one exists).
	 * <br>In summary:  If the attribute has an assigned value in the document 
	 * then <code>specified</code> is <code>true</code>, and the value is 
	 * the assigned value.  If the attribute has no assigned value in the 
	 * document and has a default value in the DTD, then 
	 * <code>specified</code> is <code>false</code>, and the value is the 
	 * default value in the DTD. If the attribute has no assigned value in 
	 * the document and has a value of #IMPLIED in the DTD, then the 
	 * attribute does not appear in the structure model of the document. If 
	 * the <code>ownerElement</code> attribute is <code>null</code> (i.e. 
	 * because it was just created or was set to <code>null</code> by the 
	 * various removal and cloning operations) <code>specified</code> is 
	 * <code>true</code>. 
	 */
	public boolean getSpecified()
	{
		XElm elm = (XElm)_getParent();

		if (elm == null)
			return true;
		int idx = elm._getAttrIdx(this);
		if (elm._isDefAttr(idx) == false)
			return true;
		String[] attrs = elm._getDefAttrs();
		if (value != attrs[(idx << 2) + 3])
			return true;

		return false;
	}

	/**
	 * Returns the value of this attribute. 
	 * On retrieval, the value of the attribute is returned as a string. 
	 * Character and general entity references are replaced with their 
	 * values. See also the method <code>getAttribute</code> on the 
	 * <code>Element</code> interface.
	 * <br>On setting, this creates a <code>Text</code> node with the unparsed 
	 * contents of the string. I.e. any characters that an XML processor 
	 * would recognize as markup are instead treated as literal text. See 
	 * also the method <code>setAttribute</code> on the <code>Element</code> 
	 * interface.
	 */
	public String getValue()
	{
		return value;
	}

	/**
	 * Sets the value of this attribute. 
	 * On setting, this creates a <code>Text</code> node with the unparsed 
	 * contents of the string. I.e. any characters that an XML processor 
	 * would recognize as markup are instead treated as literal text. See 
	 * also the method <code>setAttribute</code> on the <code>Element</code> 
	 * interface.
	 *
	 * @exception DOMException
	 *   NO_MODIFICATION_ALLOWED_ERR: Raised when the node is readonly.
	 */
	public void setValue(String value)
		throws DOMException
	{
		if (_isRO())
			throw new DOMException(DOMException.NO_MODIFICATION_ALLOWED_ERR, "");

		XElm elm = (XElm)_getParent();
		if (elm != null)
			elm._updateAttr(
				elm._setAttr(getNamespaceURI(), getNodeName(), value));
		else
			_setValue(value);
	}

	/**
	 * The value of this node, depending on its type. 
	 * When it is defined to be <code>null</code>, setting it has no effect.
	 *
	 * @exception DOMException
	 *   DOMSTRING_SIZE_ERR: Raised when it would return more characters than 
	 *   fit in a <code>DOMString</code> variable on the implementation 
	 *   platform.
	 */
	public String getNodeValue()
		throws DOMException
	{
		return value;
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
		setValue(nodeValue);
	}

	/**
	 * The <code>Element</code> node this attribute is attached to or 
	 * <code>null</code> if this attribute is not in use.
	 *
	 * @since DOM Level 2
	 */
	public Element getOwnerElement()
	{
		return (Element)_getParent();
	}

	/**
	 * Returns whether this attribute is known to be of type ID or not. 
	 * In other words, whether this attribute 
	 * contains an identifier for its owner element or not. When it is and 
	 * its value is unique, the <code>ownerElement</code> of this attribute 
	 * can be retrieved using the method <code>Document.getElementById</code>.
	 *
	 * @since DOM Level 3
	 */
	public boolean isId()
	{
		XElm elm = (XElm)_getParent();
		if (elm != null) {
			int idx = elm._getAttrIdx(null, getNodeName());
			if (idx < 0)
				throw new RuntimeException();
			return elm._isIdAttr(idx);
		}
		return false;
	}

	/**
	 * Sets node's value attribute. This method sets the attribute value and 
	 * adjusts list of children. A single child Text node is created unless 
	 * the value is the empty string.
	 */
	protected void _setValue(String value)
	{
		setTextContent(value);
		this.value = value;
	}

	/**
	 * Updates node's value attributes. This method updates attribute node 
	 * internal cache of value and sync up with the parent element if any. 
	 * It is used for the attribute value update when one of the attribute 
	 * children have changed its value.
	 */
	protected void _updateValue(String value)
	{
		this.value = value;

		XElm elm = (XElm)_getParent();
		if (elm != null)
			elm._setAttr(elm._getAttrIdx(this), value);
	}

	/**
	 * Cleans up node.
	 */
	protected void _clear()
	{
		super._clear();
		value = "";
	}

	/**
	 * Check a new child node. This method throws appropriate exception if this 
	 * node does not like new child. A subclass MUST chain this method with 
	 * implementation provided by its parent class.
	 */
	protected void _checkNewChild(Node newChild, boolean replaceChild)
		throws DOMException
	{
		super._checkNewChild(newChild, replaceChild);

		switch(newChild.getNodeType()) {
		case TEXT_NODE:
		case ENTITY_REFERENCE_NODE:
			break;

		default:
			throw new DOMException(DOMException.HIERARCHY_REQUEST_ERR, "");
		}
	}

	/**
	 * Notification of child addition.
	 */
	protected void _childAdded(XNode child)
	{
		super._childAdded(child);

		_updateValue(getTextContent());
	}

	/**
	 * Notification of child removal.
	 */
	protected void _childRemoved(XNode child)
	{
		super._childRemoved(child);

		_updateValue(getTextContent());
	}
}
