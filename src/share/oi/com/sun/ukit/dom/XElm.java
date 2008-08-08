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

import java.lang.ref.Reference;
import java.lang.ref.WeakReference;

import org.w3c.dom.Node;
import org.w3c.dom.Attr;
import org.w3c.dom.Element;
import org.w3c.dom.Document;
import org.w3c.dom.NodeList;
import org.w3c.dom.NamedNodeMap;
import org.w3c.dom.DOMException;
import org.w3c.dom.ElementTraversal;

import com.sun.ukit.xml.Pair;
import com.sun.ukit.xml.Parser;

/**
 * DOM element node implementation.
 *
 * @see org.w3c.dom.Node
 */

public abstract class XElm
	extends XParent
	implements Element, ElementTraversal
{
	/** Default attribute list capacity. */
	private final static int ATTR_ALLOC_UNIT = 4;
	/** List of attributes assigned to this element. Four strings represent 
	 *  single attribute: 0 - qname, 1 - localname, 2 - namespace, 3 - value */
	private String[] attlst;
	/** Number of attributes assigned to this element. */
	private int attnum;
	/** Index of id attribute or negative value if element has no id. */
	private int attid;
	/** Number of default attributes. First <code>defnum</code> attributes in 
	 *  <code>attlst</code> are default with values copied from list of default 
	 *  attributes for this element or assigned values. */
	private int defnum;
	/** Attr objects created to represent attributes of this element. */
	private Reference[] attobj;

	/**
	 * Constructs element object from other element.
	 */
	 protected XElm(XElm element, boolean deep)
	 {
	 	super(element, deep);
	 	attlst = new String[element.attlst.length];
	 	System.arraycopy(element.attlst, 0, attlst, 0, element.attnum << 2);
	 	attnum = element.attnum;
	 	attid  = element.attid;
	 	defnum = element.defnum;
	 }

	/**
	 * Constructs element object from its qualified name and namespace URI and 
	 * its owner document.
	 */
	 protected XElm(String namespaceURI, String tagName, XDoc ownerDocument)
	 {
	 	super(namespaceURI, tagName, ownerDocument);
		// Copy default attributes from document
		String[] dattrs = _getDefAttrs();
		defnum = (dattrs != null)? dattrs.length >> 2: 0;
		attlst = new String[(defnum + ATTR_ALLOC_UNIT) << 2];
		attid  = -1;
		for (int idx = 0; idx < defnum; idx++) {
			int base = idx << 2;
			attlst[base] = dattrs[base];
			attlst[base + 1] = dattrs[base + 1];
			attlst[base + 2] = dattrs[base + 2];
			attlst[base + 3] = dattrs[base + 3];
			if ("xml:id".equals(attlst[base])) {
				attid = idx;
			}
		}
		attnum = defnum;
	 }

	/**
	 * Constructs element object from parser element.
	 */
	 protected XElm(Pair element, int defcnt, XDoc ownerDocument)
	 {
	 	super(
	 		(element.ns == null && (element.id & Parser.FLAG_NSAWARE) != 0)? 
	 			"": element.ns, 
	 		element.qname(), 
	 		ownerDocument);
	 	attlst = new String[element.num << 2];
		attid  = -1;
		//		Set attributes from last to first. This order ensures the 
		//		default attributes will be in the beginning of the list.
		boolean nsaware = ((element.id & Parser.FLAG_NSAWARE) != 0);
		Pair    attr    = element.list;
		for (int idx = element.num - 1; idx >= 0; idx--) {
			int base = idx << 2;
			attlst[base] = ownerDocument._intern(attr.qname());
			attlst[base + 1] = ownerDocument._intern(attr.local());
			attlst[base + 2] = ownerDocument._intern(
				(attr.ns == null && nsaware == true)? "": attr.ns);
			attlst[base + 3] = attr.value;
			if (attr.id == 'i')
				attid = idx;
			attr = attr.next;
		}
		defnum = defcnt;
		attnum = element.num;
	 }

	/**
	 * A code representing the type of the underlying object, as defined above.
	 */
	public abstract short getNodeType();

	/**
	 * The name of the element. For example, in: 
	 * <pre> &lt;elementExample 
	 * id="demo"&gt; ... &lt;/elementExample&gt; , </pre>
	 *  <code>tagName</code> has 
	 * the value <code>"elementExample"</code>. Note that this is 
	 * case-preserving in XML, as are all of the operations of the DOM. The 
	 * HTML DOM returns the <code>tagName</code> of an HTML element in the 
	 * canonical uppercase form, regardless of the case in the source HTML 
	 * document. 
	 */ 
	public String getTagName()
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

		_setPrefix(prefix);
	}

	/**
	 * Returns whether this node (if it is an element) has any attributes.
	 *
	 * @return <code>true</code> if this node has any attributes, 
	 *   <code>false</code> otherwise.
	 *
	 * @since DOM Level 2
	 */
	public boolean hasAttributes()
	{
		return (attnum > 0);
	}

	/**
	 * Returns <code>true</code> when an attribute with a given name is 
	 * specified on this element or has a default value, <code>false</code> 
	 * otherwise.
	 *
	 * @param name The name of the attribute to look for.
	 * @return <code>true</code> if an attribute with the given name is 
	 *   specified on this element or has a default value, <code>false</code>
	 *    otherwise.
	 *
	 * @since DOM Level 2
	 */ 
	public boolean hasAttribute(String name)
	{
		return (_getAttrIdx(null, name) >= 0);
	}

	/**
	 * Returns <code>true</code> when an attribute with a given local name and 
	 * namespace URI is specified on this element or has a default value, 
	 * <code>false</code> otherwise. HTML-only DOM implementations do not 
	 * need to implement this method.
	 *
	 * @param namespaceURI The namespace URI of the attribute to look for.
	 * @param localName The local name of the attribute to look for.
	 * @return <code>true</code> if an attribute with the given local name 
	 *   and namespace URI is specified or has a default value on this 
	 *   element, <code>false</code> otherwise.
	 *
	 * @since DOM Level 2
	 */ 
	public boolean hasAttributeNS(String namespaceURI, String localName)
	{
		return (_getAttrIdx(namespaceURI, localName) >= 0);
	}

	/**
	 * A <code>NamedNodeMap</code> containing the attributes of this node (if 
	 * it is an <code>Element</code>) or <code>null</code> otherwise. 
	 */
	public NamedNodeMap getAttributes()
	{
		return new AttrMapImp(this);
	}

	/**
	 * Retrieves an attribute value by name.
	 *
	 * @param name The name of the attribute to retrieve.
	 * @return The <code>Attr</code> value as a string, or the empty string 
	 *   if that attribute does not have a specified or default value.
	 */ 
	public String getAttribute(String name)
	{
		int idx = _getAttrIdx(null, name);

		//		uDOM #text attribute support
		if (idx < 0 && name.equals("#text"))
			return getTextContent();

		return (idx >= 0)? attlst[(idx << 2) + 3]: "";
	}

	/**
	 * Retrieves an attribute value by local name and namespace URI. HTML-only 
	 * DOM implementations do not need to implement this method.
	 *
	 * @param namespaceURI The namespace URI of the attribute to retrieve.
	 * @param localName The local name of the attribute to retrieve.
	 * @return The <code>Attr</code> value as a string, or the empty string 
	 *   if that attribute does not have a specified or default value.
	 *
	 * @since DOM Level 2
	 */ 
	public String getAttributeNS(String namespaceURI, String localName)
	{
		int idx = _getAttrIdx(namespaceURI, localName);
		return (idx >= 0)? attlst[(idx << 2) + 3]: "";
	}

	/**
	 * Retrieves an attribute node by name.
	 * <br>To retrieve an attribute node by qualified name and namespace URI, 
	 * use the <code>getAttributeNodeNS</code> method.
	 *
	 * @param name The name (<code>nodeName</code>) of the attribute to 
	 *   retrieve.
	 * @return The <code>Attr</code> node with the specified name (
	 *   <code>nodeName</code>) or <code>null</code> if there is no such 
	 *   attribute.
	 */ 
	public Attr getAttributeNode(String name)
	{
		return (Attr)_getAttrObj(_getAttrIdx(null, name));
	}

	/**
	 * Retrieves an <code>Attr</code> node by local name and namespace URI. 
	 * HTML-only DOM implementations do not need to implement this method.
	 *
	 * @param namespaceURI The namespace URI of the attribute to retrieve.
	 * @param localName The local name of the attribute to retrieve.
	 * @return The <code>Attr</code> node with the specified attribute local 
	 *   name and namespace URI or <code>null</code> if there is no such 
	 *   attribute.
	 *
	 * @since DOM Level 2
	 */ 
	public Attr getAttributeNodeNS(String namespaceURI, String localName)
	{
		return (Attr)_getAttrObj(_getAttrIdx(namespaceURI, localName));
	}

	/**
	 * Adds a new attribute. If an attribute with that name is already present 
	 * in the element, its value is changed to be that of the value 
	 * parameter. This value is a simple string; it is not parsed as it is 
	 * being set. So any markup (such as syntax to be recognized as an 
	 * entity reference) is treated as literal text, and needs to be 
	 * appropriately escaped by the implementation when it is written out. 
	 * In order to assign an attribute value that contains entity 
	 * references, the user must create an <code>Attr</code> node plus any 
	 * <code>Text</code> and <code>EntityReference</code> nodes, build the 
	 * appropriate subtree, and use <code>setAttributeNode</code> to assign 
	 * it as the value of an attribute.
	 * <br>To set an attribute with a qualified name and namespace URI, use 
	 * the <code>setAttributeNS</code> method.
	 *
	 * @param name The name of the attribute to create or alter.
	 * @param value Value to set in string form.
	 * @exception DOMException
	 *   INVALID_CHARACTER_ERR: Raised if the specified name contains an 
	 *   illegal character.
	 *   <br>NO_MODIFICATION_ALLOWED_ERR: Raised if this node is readonly.
	 */ 
	public void setAttribute(String name, String value)
		throws DOMException
	{
		if (value == null)
			throw new NullPointerException("");

		if (name.equals("#text")) {
			setTextContent(value);  // uDOM #text attribute support
		} else {
			XNode._checkName(name, false);
			_updateAttr(_setAttr(null, name, value));
		}
	}

	/**
	 * Adds a new attribute. If an attribute with the same local name and 
	 * namespace URI is already present on the element, its prefix is 
	 * changed to be the prefix part of the <code>qualifiedName</code>, and 
	 * its value is changed to be the <code>value</code> parameter. This 
	 * value is a simple string; it is not parsed as it is being set. So any 
	 * markup (such as syntax to be recognized as an entity reference) is 
	 * treated as literal text, and needs to be appropriately escaped by the 
	 * implementation when it is written out. In order to assign an 
	 * attribute value that contains entity references, the user must create 
	 * an <code>Attr</code> node plus any <code>Text</code> and 
	 * <code>EntityReference</code> nodes, build the appropriate subtree, 
	 * and use <code>setAttributeNodeNS</code> or 
	 * <code>setAttributeNode</code> to assign it as the value of an 
	 * attribute.
	 * <br>HTML-only DOM implementations do not need to implement this method.
	 *
	 * @param namespaceURI The namespace URI of the attribute to create or 
	 *   alter.
	 * @param qualifiedName The qualified name of the attribute to create or 
	 *   alter.
	 * @param value The value to set in string form.
	 * @exception DOMException
	 *   INVALID_CHARACTER_ERR: Raised if the specified qualified name 
	 *   contains an illegal character.
	 *   <br>NO_MODIFICATION_ALLOWED_ERR: Raised if this node is readonly.
	 *   <br>NAMESPACE_ERR: Raised if the <code>qualifiedName</code> is 
	 *   malformed, if the <code>qualifiedName</code> has a prefix and the 
	 *   <code>namespaceURI</code> is <code>null</code>, if the 
	 *   <code>qualifiedName</code> has a prefix that is "xml" and the 
	 *   <code>namespaceURI</code> is different from "
	 *   http://www.w3.org/XML/1998/namespace", or if the 
	 *   <code>qualifiedName</code> is "xmlns" and the 
	 *   <code>namespaceURI</code> is different from "
	 *   http://www.w3.org/2000/xmlns/".
	 *
	 * @since DOM Level 2
	 */ 
	public void setAttributeNS(
			String namespaceURI, String qualifiedName, String value)
		throws DOMException
	{
		if (value == null)
			throw new NullPointerException("");

		XNode._checkNameNS(namespaceURI, qualifiedName);
		_updateAttr(_setAttr(namespaceURI, qualifiedName, value));
	}

	/**
	 * Adds a new attribute node. If an attribute with that name (
	 * <code>nodeName</code>) is already present in the element, it is 
	 * replaced by the new one.
	 * <br>To add a new attribute node with a qualified name and namespace 
	 * URI, use the <code>setAttributeNodeNS</code> method.
	 *
	 * @param newAttr The <code>Attr</code> node to add to the attribute list.
	 * @return If the <code>newAttr</code> attribute replaces an existing 
	 *   attribute, the replaced <code>Attr</code> node is returned, 
	 *   otherwise <code>null</code> is returned.
	 * @exception DOMException
	 *   WRONG_DOCUMENT_ERR: Raised if <code>newAttr</code> was created from a 
	 *   different document than the one that created the element.
	 *   <br>NO_MODIFICATION_ALLOWED_ERR: Raised if this node is readonly.
	 *   <br>INUSE_ATTRIBUTE_ERR: Raised if <code>newAttr</code> is already an 
	 *   attribute of another <code>Element</code> object. The DOM user must 
	 *   explicitly clone <code>Attr</code> nodes to re-use them in other 
	 *   elements.
	 */ 
	public Attr setAttributeNode(Attr newAttr)
		throws DOMException
	{
		XNode attr = (XNode)newAttr;

		if (attr._getParent() == this)
			return (Attr)attr;
		if (getOwnerDocument() != attr.getOwnerDocument())
			throw new DOMException(DOMException.WRONG_DOCUMENT_ERR, "");
		if (attr._getParent() != null)
			throw new DOMException(DOMException.INUSE_ATTRIBUTE_ERR, "");

		if (attobj == null)
			attobj = new WeakReference[attlst.length >> 2];

		XNode ex  = null;
		int   idx = _getAttrIdx(null, attr.getNodeName());
		if (idx < 0) {
			//		Create attribute
			idx = _setAttr(null, attr.getNodeName(), attr.getNodeValue());
		} else {
			//		Release old object
			ex  = _getAttrObj(idx);
			attobj[idx].clear();
			ex._setParent(null);
			//		Replace attribute
			_setAttr(idx, attr.getNodeValue());
		}
		//		Set new object
		attobj[idx] = new WeakReference(attr);
		attr._setParent(this);

		return (Attr)ex;
	}

	/**
	 * Adds a new attribute. If an attribute with that local name and that 
	 * namespace URI is already present in the element, it is replaced by 
	 * the new one.
	 * <br>HTML-only DOM implementations do not need to implement this method.
	 *
	 * @param newAttr The <code>Attr</code> node to add to the attribute list.
	 * @return If the <code>newAttr</code> attribute replaces an existing 
	 *   attribute with the same local name and namespace URI, the replaced 
	 *   <code>Attr</code> node is returned, otherwise <code>null</code> is 
	 *   returned.
	 * @exception DOMException
	 *   WRONG_DOCUMENT_ERR: Raised if <code>newAttr</code> was created from a 
	 *   different document than the one that created the element.
	 *   <br>NO_MODIFICATION_ALLOWED_ERR: Raised if this node is readonly.
	 *   <br>INUSE_ATTRIBUTE_ERR: Raised if <code>newAttr</code> is already an 
	 *   attribute of another <code>Element</code> object. The DOM user must 
	 *   explicitly clone <code>Attr</code> nodes to re-use them in other 
	 *   elements.
	 *
	 * @since DOM Level 2
	 */ 
	public Attr setAttributeNodeNS(Attr newAttr)
		throws DOMException
	{
		XNode attr = (XNode)newAttr;

		if (attr._getParent() == this)
			return null;
		if (getOwnerDocument() != attr.getOwnerDocument())
			throw new DOMException(DOMException.WRONG_DOCUMENT_ERR, "");
		if (attr._getParent() != null)
			throw new DOMException(DOMException.INUSE_ATTRIBUTE_ERR, "");

		XNode ex  = null;
		int   idx = _getAttrIdx(attr.getNamespaceURI(), attr.getLocalName());
		if (idx >= 0) {
			ex = _getAttrObj(idx);
			//		Release old object
			attobj[idx].clear();
			ex._setParent(null);
		}

		idx = _setAttr(
				attr.getNamespaceURI(), attr.getNodeName(), attr.getNodeValue());

		if (_isDefAttr(idx)) {  // preserve default attribute qname
			int base = idx << 2;
			attr._set(attlst[base + 2], attlst[base], attlst[base + 1]);
		}
		if (attobj == null)
			attobj = new WeakReference[attlst.length >> 2];
		//		Set new object
		attobj[idx] = new WeakReference(attr);
		attr._setParent(this);

		return (Attr)ex;
	}

	/**
	 *  If the parameter <code>isId</code> is <code>true</code>, this method 
	 * declares the specified attribute to be a user-determined ID attribute
	 * . This affects the value of <code>Attr.isId</code> and the behavior 
	 * of <code>Document.getElementById</code>.
	 * Use the value <code>false</code> for the parameter 
	 * <code>isId</code> to undeclare an attribute for being a 
	 * user-determined ID attribute. 
	 * <br> To specify an attribute by local name and namespace URI, use the 
	 * <code>setIdAttributeNS</code> method. 
	 *
	 * @param name The name of the attribute.
	 * @param isId Whether the attribute is a of type ID.
	 * @exception DOMException
	 *   NO_MODIFICATION_ALLOWED_ERR: Raised if this node is readonly.
	 *   <br>NOT_FOUND_ERR: Raised if the specified node is not an attribute 
	 *   of this element.
	 *
	 * @since DOM Level 3
	 */ 
	public void setIdAttribute(String name, boolean isId)
		throws DOMException
	{
		_setIdAttr(_getAttrIdx(null, name), isId);
	}

	/**
	 *  If the parameter <code>isId</code> is <code>true</code>, this method 
	 * declares the specified attribute to be a user-determined ID attribute
	 * . This affects the value of <code>Attr.isId</code> and the behavior 
	 * of <code>Document.getElementById</code>.
	 * Use the value <code>false</code> for the parameter 
	 * <code>isId</code> to undeclare an attribute for being a 
	 * user-determined ID attribute. 
	 *
	 * @param namespaceURI The namespace URI of the attribute.
	 * @param localName The local name of the attribute.
	 * @param isId Whether the attribute is a of type ID.
	 * @exception DOMException
	 *   NO_MODIFICATION_ALLOWED_ERR: Raised if this node is readonly.
	 *   <br>NOT_FOUND_ERR: Raised if the specified node is not an attribute 
	 *   of this element.
	 *
	 * @since DOM Level 3
	 */ 
	public void setIdAttributeNS(
			String namespaceURI, String localName, boolean isId)
		throws DOMException
	{
		_setIdAttr(_getAttrIdx(namespaceURI, localName), isId);
	}

	/**
	 *  If the parameter <code>isId</code> is <code>true</code>, this method 
	 * declares the specified attribute to be a user-determined ID attribute
	 * . This affects the value of <code>Attr.isId</code> and the behavior 
	 * of <code>Document.getElementById</code>.
	 * Use the value <code>false</code> for the parameter 
	 * <code>isId</code> to undeclare an attribute for being a 
	 * user-determined ID attribute. 
	 *
	 * @param idAttr The attribute node.
	 * @param isId Whether the attribute is a of type ID.
	 * @exception DOMException
	 *   NO_MODIFICATION_ALLOWED_ERR: Raised if this node is readonly.
	 *   <br>NOT_FOUND_ERR: Raised if the specified node is not an attribute 
	 *   of this element.
	 *
	 * @since DOM Level 3
	 */ 
	public void setIdAttributeNode(Attr idAttr, boolean isId)
		throws DOMException
	{
		XNode attr = (XNode)idAttr;
		if (attr._getParent() != this)
			throw new DOMException(DOMException.NOT_FOUND_ERR, "");

		_setIdAttr(_getAttrIdx(attr), isId);
	}

	/**
	 * Removes an attribute by name. If the removed attribute is known to have 
	 * a default value, an attribute immediately appears containing the 
	 * default value as well as the corresponding namespace URI, local name, 
	 * and prefix when applicable.
	 * <br>To remove an attribute by local name and namespace URI, use the 
	 * <code>removeAttributeNS</code> method.
	 *
	 * @param name The name of the attribute to remove.
	 * @exception DOMException
	 *   NO_MODIFICATION_ALLOWED_ERR: Raised if this node is readonly.
	 */ 
	public void removeAttribute(String name)
		throws DOMException
	{
		_rmAttr(_getAttrIdx(null, name));
	}

	/**
	 * Removes an attribute by local name and namespace URI. If the removed 
	 * attribute has a default value it is immediately replaced. The 
	 * replacing attribute has the same namespace URI and local name, as 
	 * well as the original prefix.
	 * <br>HTML-only DOM implementations do not need to implement this method.
	 *
	 * @param namespaceURI The namespace URI of the attribute to remove.
	 * @param localName The local name of the attribute to remove.
	 * @exception DOMException
	 *   NO_MODIFICATION_ALLOWED_ERR: Raised if this node is readonly.
	 *
	 * @since DOM Level 2
	 */ 
	public void removeAttributeNS(String namespaceURI, String localName)
		throws DOMException
	{
		_rmAttr(_getAttrIdx(namespaceURI, localName));
	}

	/**
	 * Removes the specified attribute node. If the removed <code>Attr</code> 
	 * has a default value it is immediately replaced. The replacing 
	 * attribute has the same namespace URI and local name, as well as the 
	 * original prefix, when applicable.
	 *
	 * @param oldAttr The <code>Attr</code> node to remove from the attribute 
	 *   list.
	 * @return The <code>Attr</code> node that was removed.
	 * @exception DOMException
	 *   NO_MODIFICATION_ALLOWED_ERR: Raised if this node is readonly.
	 *   <br>NOT_FOUND_ERR: Raised if <code>oldAttr</code> is not an attribute 
	 *   of the element.
	 */ 
	public Attr removeAttributeNode(Attr oldAttr)
		throws DOMException
	{
		XNode attr = (XNode)oldAttr;
		int   idx  = _getAttrIdx(attr);
		if (attr._getParent() != this || idx < 0)
			throw new DOMException(DOMException.NOT_FOUND_ERR, "");

		_rmAttr(idx);

		return (Attr)attr;
	}

	/**
	 * Returns a <code>NodeList</code> of all descendant <code>Elements</code> 
	 * with a given tag name, in the order in which they are encountered in 
	 * a preorder traversal of this <code>Element</code> tree.
	 *
	 * @param name The name of the tag to match on. The special value "*" 
	 *   matches all tags.
	 * @return A list of matching <code>Element</code> nodes.
	 */ 
	public NodeList getElementsByTagName(String name)
	{
		ElmFilter filter = new ElmFilter(name);
		super._procEachChild(filter);
		return filter;
	}

	/**
	 * Returns a <code>NodeList</code> of all the descendant 
	 * <code>Elements</code> with a given local name and namespace URI in 
	 * the order in which they are encountered in a preorder traversal of 
	 * this <code>Element</code> tree.
	 * <br>HTML-only DOM implementations do not need to implement this method.
	 *
	 * @param namespaceURI The namespace URI of the elements to match on. The 
	 *   special value "*" matches all namespaces.
	 * @param localName The local name of the elements to match on. The 
	 *   special value "*" matches all local names.
	 * @return A new <code>NodeList</code> object containing all the matched 
	 *   <code>Elements</code>.
	 *
	 * @since DOM Level 2
	 */ 
	public NodeList getElementsByTagNameNS(String namespaceURI, String localName)
	{
		ElmFilter filter = new ElmFilter(namespaceURI, localName);
		super._procEachChild(filter);
		return filter;
	}

	/**
	 * Retrieves the number of child elements.
	 *
	 * @return the current number of element nodes that are children
	 * of this element. <code>0</code> if this element has no child
	 * elements.
	 */
	public int getChildElementCount() {
		return _countChildElm();
	}

	/**
	 * Retrieves the first child element.
	 * 
	 * @return the first child element or null
	 */
	public Element getFirstElementChild() {
		return _nextElm(null);
	}

	/**
	 * Retrieves the last child element.
	 * 
	 * @return the last child element or null
	 */
	public Element getLastElementChild() {
		return _prevElm(null);
	}

	/**
	 * Retrieves the next sibling element.
	 * 
	 * @return the next sibling element or null
	 */
	public Element getNextElementSibling() {
		XParent parent = _getParent();
		return (parent != null) ? parent._nextElm(this) : null;
	}

	/**
	 * Retrieves the previous sibling element.
	 * 
	 * @return the previous sibling element or null
	 */
	public Element getPreviousElementSibling() {
		XParent parent = _getParent();
		return (parent != null) ? parent._prevElm(this) : null;
	}

	/**
	 * Extends capacity of list of attributes.
	 */
	private final void _extAttlst()
	{
	 	String list[] = new String[attlst.length + (ATTR_ALLOC_UNIT << 2)];
		System.arraycopy(attlst, 0, list, 0, attnum << 2);
		attlst = list;
		if (attobj != null) {
			Reference objs[] = new WeakReference[attobj.length + ATTR_ALLOC_UNIT];
			System.arraycopy(attobj, 0, objs, 0, attnum);
			attobj = objs;
		}
	}

	/**
	 * Returns total number of attributes. 
	 */
	/* pkg */ final int _getAttrNum()
	{
		return attnum;
	}

	/**
	 * Returns attribute index. If namespace is not <code>null</code> or the 
	 * empty string the name is local name otherwise the name is qualified name.
	 * 
	 * @param namespace the namespace associated with attribute; <code>null</code> 
	 *  is used for non-namespace aware mode; the empty string is used to indicate 
	 *  that the attribute has no associated namespace.
	 * @param name attribute's local or qualified name.
	 */
	/* pkg */ final int _getAttrIdx(String namespace, String name)
	{
		if (namespace == null || namespace.length() == 0) {
			for (int i = 0; i < attnum; i++) {
				if (name.equals(attlst[i << 2]))
					return i;
			}
		} else {
			for (int i = 0; i < attnum; i++) {
				if (name.equals(attlst[(i << 2) + 1]) == true && 
					namespace.equals(attlst[(i << 2) + 2]) == true)
					return i;
			}
		}
		return -1;
	}

	/**
	 * Returns attribute index for an attribute node. 
	 */
	/* pkg */ final int _getAttrIdx(XNode attr)
	{
		if (attr != null && attobj != null) {
			for (int i = 0; i < attnum; i++) {
				Reference ref = attobj[i];
				if (ref != null && ref.get() == attr)
					return i;
			}
		}
		return -1;
	}

	/**
	 * Returns attribute object or <code>null</code> if index is negative.
	 */
	/* pkg */ final XNode _getAttrObj(int idx)
	{
		if (idx >= 0 && idx < attnum) {
			if (attobj == null)
				attobj = new WeakReference[attlst.length >> 2];
			if (attobj[idx] == null || attobj[idx].get() == null) {
				int   base = idx << 2;
				XNode attr = (XNode)((attlst[base + 2] != null)? 
					_getDoc().createAttributeNS(attlst[base + 2], attlst[base]):
					_getDoc().createAttribute(attlst[base]));
				attr._setValue(attlst[base + 3]);
				attr._setParent(this);
				attobj[idx] = new WeakReference(attr);
			}
			return (XNode)attobj[idx].get();
		}
		return null;
	}

	/**
	 * Removes attribute.
	 */
	private final XNode _rmAttr(int idx)
		throws DOMException
	{
		if (_isRO())
			throw new DOMException(DOMException.NO_MODIFICATION_ALLOWED_ERR, "");

		if (idx < 0 || idx >= attnum)
			return null;

		String attName  = attlst[idx << 2];
		String attValue = attlst[(idx << 2) + 3];
		if (idx == attid)
			attid = -1;
		XNode attr = null;
		//		Release attribute object
		if (attobj != null && attobj[idx] != null) {
			attr = (XNode)attobj[idx].get();
			if (attr != null) {
				attr._setParent(null);
				attobj[idx].clear();
				attobj[idx] = null;
			}
		}
		if (_isDefAttr(idx)) {
			//		Restore default attribute value
			String[] attrs = _getDefAttrs();
			attlst[(idx << 2) + 3] = attrs[(idx << 2) + 3];
			if ("xml:id".equals(attlst[idx << 2]))
				attid = idx;
			_attrChanged(attName, attValue, attlst[(idx << 2) + 3]);
		} else {
			if (idx < (attnum - 1)) {
				System.arraycopy(
					attlst, (idx + 1) << 2, attlst, idx << 2, (attnum - idx) << 2);
				if (attobj != null)
					System.arraycopy(attobj, idx + 1, attobj, idx, attnum - idx);
			}
			attnum -= 1;
			_attrRemoved(attName, attValue);
		}

		return attr;
	}

	/**
	 * Sets attribute value or add an attribute and returns its index.
	 * 
	 * @param namespace the namespace associated with attribute; <code>null</code> 
	 *  is used for non-namespace aware mode; the empty string is used to indicate 
	 *  that the attribute has no associated namespace.
	 * @param qname attribute's qualified name.
	 * @param value attribute's value.
	 */
	/* pkg */ final int _setAttr(String namespace, String qname, String value)
		throws DOMException
	{
		if (_isRO())
			throw new DOMException(DOMException.NO_MODIFICATION_ALLOWED_ERR, "");

		XDoc doc = _getDoc();
		//		Check attribute name before add it.
		String name = (namespace != null)? XNode._getLocalName(qname): qname;
		int idx = _getAttrIdx(namespace, name);
		if (idx < 0) {
			if ("xml:id".equals(qname)) {
				attid = attnum;
			}
			//		Set new attribute.
			if (attnum >= (attlst.length >> 2))
				_extAttlst();
	
			int base = attnum << 2;
			attlst[base]     = doc._intern(qname);
			attlst[base + 1] = doc._intern(name);
			attlst[base + 2] = doc._intern(namespace);
			attlst[base + 3] = value;
			idx = attnum;
			attnum++;
			_attrAdded(attlst[base], value);
		} else {
			int base = idx << 2;
			if (!qname.equals(attlst[base]) || value != attlst[base + 3]) {
				String oldValue  = attlst[base + 3];
				attlst[base + 3] = value;
				if (!_isDefAttr(idx))  // preserve default attributes qname
					attlst[base] = doc._intern(qname);
				_attrChanged(attlst[base], oldValue, value);
			}
		}

		return idx;
	}

	/**
	 * Sets attribute value and returns its index.
	 */
	/* pkg */ final int _setAttr(int idx, String value)
		throws DOMException
	{
		if (_isRO())
			throw new DOMException(DOMException.NO_MODIFICATION_ALLOWED_ERR, "");

		if (idx < 0 || idx >= attnum)
			return -1;

		if (value != attlst[(idx << 2) + 3]) {
			String oldValue = attlst[(idx << 2) + 3];
			attlst[(idx << 2) + 3] = value;
			_attrChanged(attlst[idx << 2], oldValue, value);
		}

		return idx;
	}

	/**
	 * Sets attribute value by the attribute index. Note, this method 
	 * is here to allow parser to assign values to default attributes.
	 */
	/* pkg */ final void __setAttrV(int idx, String value)
	{
		if (idx < 0 || idx >= attnum)
			throw new IndexOutOfBoundsException();

		attlst[(idx << 2) + 3] = value;
	}

	/**
	 * Sets attribute namespace URI by the attribute index. Note, this method 
	 * is here to allow parser to assign namespace URIs to default attributes.
	 */
	/* pkg */ final void __setAttrNS(int idx, String namespace)
	{
		if (idx < 0 || idx >= attnum)
			throw new IndexOutOfBoundsException();

		attlst[(idx << 2) + 2] = _getDoc()._intern(namespace);
	}

	/**
	 * Sets attribute qualified name. Note, this method is here to allow an 
	 * attribute node to update its qualified name after setPrefix call.
	 */
	/* pkg */ final void __setAttrQN(int idx, String qname)
	{
		if (idx < 0 || idx >= attnum)
			throw new IndexOutOfBoundsException();

		attlst[idx << 2] = qname;
	}

	/**
	 * Updates attribute object if it exists.
	 */
	/* pkg */ final void _updateAttr(int idx)
	{
		if (idx >= 0 && idx < attnum && attobj != null && attobj[idx] != null) {
			XNode attr = (XNode)attobj[idx].get();
			if (attr != null) {
				int base = idx << 2;
				attr._set(attlst[base + 2], attlst[base], attlst[base + 1]);
				attr._setValue(attlst[base + 3]);
			}
		}
	}

	/**
	 * Sets attribute to be identifier.
	 */
	private final void _setIdAttr(int idx, boolean isId)
	{
		if (_isRO())
			throw new DOMException(DOMException.NO_MODIFICATION_ALLOWED_ERR, "");
		if (idx < 0)
			throw new DOMException(DOMException.NOT_FOUND_ERR, "");
		String value = attlst[(idx << 2) + 3];
		if (isId == true) {
			if (attid >= 0 && "xml:id".equals(attlst[attid << 2]))
				return;  // cannot remove id from 'xml:id' attribute
			attid = idx;
		} else {
			if (idx != attid || "xml:id".equals(attlst[attid << 2]))
				return;  // cannot remove id from 'xml:id' attribute
			attid = -1;
		}
	}

	/**
	 * Returns <code>true</code> if attribute is identifier.
	 */
	/* pkg */ final boolean _isIdAttr(int idx)
	{
		return (idx == attid);
	}

	/**
	 * Returns <code>true</code> if attribute is default.
	 */
	/* pkg */ final boolean _isDefAttr(int idx)
	{
		return (idx >= 0 && idx < defnum);
	}

	/**
	 * Returns number of default attributes.
	 */
	/* pkg */ final int _getDefAttrsCount()
	{
		return defnum;
	}

	/**
	 * Returns default attributes for this element or <code>null</code>.
	 */
	/* pkg */ final String[] _getDefAttrs()
	{
		return _getDoc()._getDefAttrs(getNodeName());
	}

	/**
	 * Returns element with specified identifier or <code>null</code>.
	 */
	/* pkg */ final Element _getElmById(String elmId)
	{
		if (attid >= 0 && attlst[(attid << 2) + 3].equals(elmId))
			return this;

		Element elm = null;
		for (int idx = 0; idx < getLength(); idx++) {
			Node node = item(idx);
			if (node.getNodeType() == ELEMENT_NODE) {
				if ((elm = ((XElm)node)._getElmById(elmId)) != null)
					break;
			}
		}

		return elm;
	}

	/**
	 * Sets node's owner document.
	 *
	 * @param ownerDoc New owner document.
	 */
	protected void _setDoc(XDoc ownerDoc)
	{
		String[]    old_attlst = attlst;
		int         old_attnum = attnum;
		String[]    old_deflst = _getDefAttrs();
		int         old_defnum = defnum;
		Reference[] old_attobj = attobj;
		//		Detach all default attribute objects if any
		if (old_attobj != null) {
			for (int idx = 0; idx < old_defnum; idx++) {
				if (old_attobj[idx] != null) {
					XNode attr = (XNode)old_attobj[idx].get();
					if (attr != null) {
						if (attr.getNodeValue() != old_deflst[(idx << 2) + 3])
							continue;  // default value had been replaced
						attr._setParent(null);
						attr._clear();
						old_attobj[idx].clear();
						old_attobj[idx] = null;
					}
				}
			}
		}
		//		Release id if any
		attid  = -1;  // ids from old doc are invalid
		//		Get default attributes for this element in the new document
		String dattrs[] = ownerDoc._getDefAttrs(getNodeName());
		attnum = (dattrs != null)? (dattrs.length >> 2): 0;
		defnum = attnum;
		//		Allocate arrays for new set of attributes
		attlst = new String[(defnum + ATTR_ALLOC_UNIT) << 2];
		attobj = (old_attobj != null)? 
			new WeakReference[attlst.length >> 2]: null;
		//		Copy new default attributes
		for (int idx = 0; idx < defnum; idx++) {
			int base = idx << 2;
			attlst[base] = dattrs[base];
			attlst[base + 1] = dattrs[base + 1];
			attlst[base + 2] = dattrs[base + 2];
			attlst[base + 3] = dattrs[base + 3];
			if ("xml:id".equals(attlst[base])) {
				attid = idx;
			}
		}
		//		Copy attributes defined on this element
		super._setDoc(ownerDoc);  // new owner is used by _setAttr
		for (int idx = 0; idx < old_attnum; idx++) {
			int base = idx << 2;
			//		Skip default attributes with original value
			if (idx < old_defnum && old_attlst[base + 3] == old_deflst[base + 3])
				continue;
			int nidx = _setAttr(
				old_attlst[base + 2], old_attlst[base], old_attlst[base + 3]);
			//		Copy attribute objects
			if (old_attobj != null && old_attobj[idx] != null) {
				XNode attr = (XNode)old_attobj[idx].get();
				if (attr != null) {
					//		Copy only defined attribute objects
					attobj[nidx] = new WeakReference(attr);
					attr._setDoc(ownerDoc);
				}
			}
		}
	}

	/**
	 * Lets filter to process each child node.
	 */
	protected void _procEachChild(XList filter)
	{
		filter._proc(this);
		super._procEachChild(filter);
	}

	/**
	 * Returns string representation of the element.
	 */
	public String toString()
	{
		StringBuffer out = new StringBuffer();
		out.append("<");
		out.append(getNodeName());
		for (int idx = 0; idx < attnum; idx++) {
			out.append(" ");
			out.append(attlst[idx << 2]);
			out.append("=\"");
			out.append(attlst[(idx << 2) + 3]);
			out.append("\"");
		}
		if (getLength() > 0) {
			out.append(">");
			out.append(super.toString());
			out.append("</");
			out.append(getNodeName());
			out.append(">");
		} else {
			out.append("/>");
		}
		return out.toString();
	}
}
