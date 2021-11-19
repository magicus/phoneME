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
import org.w3c.dom.Text;
import org.w3c.dom.Document;
import org.w3c.dom.NodeList;
import org.w3c.dom.DOMException;

/**
 * DOM parent node implementation.
 *
 * @see org.w3c.dom.Node
 */

public abstract class XParent
	extends XNode
{
	/** Default children list capacity. */
	private final static int DEFAULT_CHILD_NUM = 8;
	/** Children list. */
	private XNode[] chlst;
	/** Number of children in the list. */
	private int chnum;

	/**
	 * Constructs node object from other node.
	 */
	 protected XParent(XParent node, boolean deep)
	 {
	 	super(node, deep);
	 	if (deep == true) {
		 	chlst = new XNode[node.chlst.length];
		 	for (int idx = 0; idx < node.chnum; idx++) {
		 		chlst[idx] = (XNode)(node.chlst[idx].cloneNode(deep));
		 		chlst[idx]._setParent(this);
		 	}
		 	chnum = node.chnum;
	 	} else {
		 	chlst = new XNode[DEFAULT_CHILD_NUM];
	 	}
	 }

	/**
	 * Constructs parent node object from its qualified name and namespace URI and 
	 * its owner document.
	 */
	 protected XParent(String namespaceURI, String qName, XDoc ownerDocument)
	 {
	 	super(namespaceURI, qName, ownerDocument);
	 	chlst = new XNode[DEFAULT_CHILD_NUM];
	 }

	/**
	 * A code representing the type of the underlying object, as defined above.
	 */
	public abstract short getNodeType();

	/**
	 * A <code>NodeList</code> that contains all children of this node. If 
	 * there are no children, this is a <code>NodeList</code> containing no 
	 * nodes.
	 */
	public NodeList getChildNodes()
	{
		return this;
	}

	/**
	 * The first child of this node. If there is no such node, this returns 
	 * <code>null</code>.
	 */
	public Node getFirstChild()
	{
		return (chnum > 0)? chlst[0]: null;
	}

	/**
	 * The last child of this node. If there is no such node, this returns 
	 * <code>null</code>.
	 */
	public Node getLastChild()
	{
		return (chnum > 0)? chlst[chnum - 1]: null;
	}

	/**
	 * Inserts the node <code>newChild</code> before the existing child node 
	 * <code>refChild</code>. If <code>refChild</code> is <code>null</code>, 
	 * insert <code>newChild</code> at the end of the list of children.
	 * <br>If <code>newChild</code> is a <code>DocumentFragment</code> object, 
	 * all of its children are inserted, in the same order, before 
	 * <code>refChild</code>. If the <code>newChild</code> is already in the 
	 * tree, it is first removed.
	 *
	 * @param newChild The node to insert.
	 * @param refChild The reference node, i.e., the node before which the new 
	 *   node must be inserted.
	 * @return The node being inserted.
	 * @exception DOMException
	 *   HIERARCHY_REQUEST_ERR: Raised if this node is of a type that does not 
	 *   allow children of the type of the <code>newChild</code> node, or if 
	 *   the node to insert is one of this node's ancestors.
	 *   <br>WRONG_DOCUMENT_ERR: Raised if <code>newChild</code> was created 
	 *   from a different document than the one that created this node.
	 *   <br>NO_MODIFICATION_ALLOWED_ERR: Raised if this node is readonly or 
	 *   if the parent of the node being inserted is readonly.
	 *   <br>NOT_FOUND_ERR: Raised if <code>refChild</code> is not a child of 
	 *   this node.
	 */
	public Node insertBefore(Node newChild, Node refChild)
		throws DOMException
	{
		if (refChild == null)
			return appendChild(newChild);

		if (_isRO())
			throw new DOMException(DOMException.NO_MODIFICATION_ALLOWED_ERR, "");

		int idx = _childIdx(refChild);
		if (idx < 0)
			throw new DOMException(DOMException.NOT_FOUND_ERR, "");

		_insertChild(idx, (XNode)newChild);

		return newChild;
	}

	/**
	 * Replaces the child node <code>oldChild</code> with <code>newChild</code>
	 *  in the list of children, and returns the <code>oldChild</code> node.
	 * <br>If <code>newChild</code> is a <code>DocumentFragment</code> object, 
	 * <code>oldChild</code> is replaced by all of the 
	 * <code>DocumentFragment</code> children, which are inserted in the 
	 * same order. If the <code>newChild</code> is already in the tree, it 
	 * is first removed.
	 *
	 * @param newChild The new node to put in the child list.
	 * @param oldChild The node being replaced in the list.
	 * @return The node replaced.
	 * @exception DOMException
	 *   HIERARCHY_REQUEST_ERR: Raised if this node is of a type that does not 
	 *   allow children of the type of the <code>newChild</code> node, or if 
	 *   the node to put in is one of this node's ancestors.
	 *   <br>WRONG_DOCUMENT_ERR: Raised if <code>newChild</code> was created 
	 *   from a different document than the one that created this node.
	 *   <br>NO_MODIFICATION_ALLOWED_ERR: Raised if this node or the parent of 
	 *   the new node is readonly.
	 *   <br>NOT_FOUND_ERR: Raised if <code>oldChild</code> is not a child of 
	 *   this node.
	 */
	public Node replaceChild(Node newChild, Node oldChild)
		throws DOMException
	{
		if (_isRO())
			throw new DOMException(DOMException.NO_MODIFICATION_ALLOWED_ERR, "");

		if (oldChild.getParentNode() != this)
			throw new DOMException(DOMException.NOT_FOUND_ERR, "");

		if (newChild == oldChild)
			return oldChild;

		XNode nchild = (XNode)newChild;
		XNode ochild = (XNode)oldChild;
		if (nchild.getNodeType() != DOCUMENT_FRAGMENT_NODE) {
			//		Replace old child with new child node.
			_checkNewChild(nchild, true);

			Node pnode = nchild.getParentNode();
			if (pnode != null)
				pnode.removeChild(nchild);

			_childRemoving(ochild);
			chlst[_childIdx(ochild)] = nchild;
			nchild._setParent(this);
			ochild._setParent(null);

			_childAdded(nchild);
		} else {
			//		Content of a DocumenFragment is new child.
			int idx = _childIdx(ochild);
			removeChild(ochild);
			_insertChild(idx, nchild);
		}
		return ochild;
	}

	/**
	 * Removes the child node indicated by <code>oldChild</code> from the list 
	 * of children, and returns it.
	 *
	 * @param oldChild The node being removed.
	 * @return The node removed.
	 * @exception DOMException
	 *   NO_MODIFICATION_ALLOWED_ERR: Raised if this node is readonly.
	 *   <br>NOT_FOUND_ERR: Raised if <code>oldChild</code> is not a child of 
	 *   this node.
	 */
	public Node removeChild(Node oldChild)
		throws DOMException
	{
		if (_isRO())
			throw new DOMException(DOMException.NO_MODIFICATION_ALLOWED_ERR, "");

		int idx = _childIdx(oldChild);
		if (idx < 0)
			throw new DOMException(DOMException.NOT_FOUND_ERR, "");

		return _removeChild(idx);
	}

	/**
	 * Adds the node <code>newChild</code> to the end of the list of children 
	 * of this node. If the <code>newChild</code> is already in the tree, it 
	 * is first removed.
	 *
	 * @param newChild The node to add. If it is a <code>DocumentFragment</code>
	 *    object, the entire contents of the document fragment are moved 
	 *   into the child list of this node
	 * @return The node added.
	 * @exception DOMException
	 *   HIERARCHY_REQUEST_ERR: Raised if this node is of a type that does not 
	 *   allow children of the type of the <code>newChild</code> node, or if 
	 *   the node to append is one of this node's ancestors.
	 *   <br>WRONG_DOCUMENT_ERR: Raised if <code>newChild</code> was created 
	 *   from a different document than the one that created this node.
	 *   <br>NO_MODIFICATION_ALLOWED_ERR: Raised if this node is readonly.
	 */
	public Node appendChild(Node newChild)
		throws DOMException
	{
		if (_isRO())
			throw new DOMException(DOMException.NO_MODIFICATION_ALLOWED_ERR, "");

		XNode nchild = (XNode)newChild;
		if (newChild.getNodeType() != DOCUMENT_FRAGMENT_NODE) {
			//		Append a new child node.
			_appendChild(nchild);
			_childAdded(nchild);
		} else {
			//		Recursive call to appendChild for each child of doc fragment
			while (nchild.getLength() > 0)
				appendChild(nchild.item(0));
		}
		return nchild;
	}

	/**
	 * Returns whether this node has any children.
	 *
	 * @return  <code>true</code> if this node has any children, 
	 *   <code>false</code> otherwise.
	 */
	public boolean hasChildNodes()
	{
		return (chnum > 0);
	}

	/**
	 * This attribute returns the text content of this node and its 
	 * descendants. When it is defined to be <code>null</code>, setting it 
	 * has no effect. 
	 * <br> On getting, no serialization is performed, the returned string 
	 * does not contain any markup. No whitespace normalization is performed 
	 * and the returned string does not contain the white spaces in element 
	 * content.
	 * <br>The string returned is made of the text content of this node 
	 * depending on its type, as defined below: 
	 * <table border='1' cellpadding='3'>
	 * <tr>
	 * <th>Node type</th>
	 * <th>Content</th>
	 * </tr>
	 * <tr>
	 * <td valign='top' rowspan='1' colspan='1'>
	 * ELEMENT_NODE, ATTRIBUTE_NODE, ENTITY_NODE, ENTITY_REFERENCE_NODE, 
	 * DOCUMENT_FRAGMENT_NODE</td>
	 * <td valign='top' rowspan='1' colspan='1'>concatenation of the <code>textContent</code> 
	 * attribute value of every child node, excluding COMMENT_NODE and 
	 * PROCESSING_INSTRUCTION_NODE nodes. This is the empty string if the 
	 * node has no children.</td>
	 * </tr>
	 * <tr>
	 * <td valign='top' rowspan='1' colspan='1'>TEXT_NODE, CDATA_SECTION_NODE, COMMENT_NODE, 
	 * PROCESSING_INSTRUCTION_NODE</td>
	 * <td valign='top' rowspan='1' colspan='1'><code>nodeValue</code></td>
	 * </tr>
	 * <tr>
	 * <td valign='top' rowspan='1' colspan='1'>DOCUMENT_NODE, 
	 * DOCUMENT_TYPE_NODE, NOTATION_NODE</td>
	 * <td valign='top' rowspan='1' colspan='1'><em>null</em></td>
	 * </tr>
	 * </table>
	 *
	 * @exception DOMException
	 *   DOMSTRING_SIZE_ERR: Raised when it would return more characters than 
	 *   fit in a <code>DOMString</code> variable on the implementation 
	 *   platform.
	 *
	 * @since DOM Level 3
	 */
	public String getTextContent()
		throws DOMException
	{
		switch(getNodeType()) {
		case ELEMENT_NODE:
		case ATTRIBUTE_NODE:
		case DOCUMENT_FRAGMENT_NODE:
		case ENTITY_REFERENCE_NODE:
		case ENTITY_NODE:
			StringBuffer text = new StringBuffer();
			_appendText(text);
			return text.toString();

		default:
		}
		return super.getTextContent();
	}

	/**
	 * On setting, any possible children this node may have 
	 * are removed and, if it the new string is not empty or 
	 * <code>null</code>, replaced by a single <code>Text</code> node 
	 * containing the string this attribute is set to. 
	 * <br>No parsing is performed, the input string is taken as pure 
	 * textual content. 
	 *
	 * @exception DOMException
	 *   NO_MODIFICATION_ALLOWED_ERR: Raised when the node is readonly.
	 *
	 * @since DOM Level 3
	 */
	public void setTextContent(String textContent)
		throws DOMException
	{
		if (_isRO())
			throw new DOMException(DOMException.NO_MODIFICATION_ALLOWED_ERR, "");

		switch(getNodeType()) {
		case ELEMENT_NODE:
		case ATTRIBUTE_NODE:
		case DOCUMENT_FRAGMENT_NODE:
		case ENTITY_REFERENCE_NODE:
		case ENTITY_NODE:
			while (chnum > 0)
				_removeChild(0);
			if (textContent != null && textContent.length() > 0) {
				appendChild(_getDoc().createTextNode(textContent));
			}
			break;

		default:
			super.setTextContent(textContent);
		}
	}

	/**
	 * Puts all <code>Text</code> nodes in the full depth of the sub-tree 
	 * underneath this <code>Node</code>, including attribute nodes, into a 
	 * "normal" form where only structure (e.g., elements, comments, 
	 * processing instructions, CDATA sections, and entity references) 
	 * separates <code>Text</code> nodes, i.e., there are neither adjacent 
	 * <code>Text</code> nodes nor empty <code>Text</code> nodes. This can 
	 * be used to ensure that the DOM view of a document is the same as if 
	 * it were saved and re-loaded, and is useful when operations (such as 
	 * XPointer  lookups) that depend on a particular document tree 
	 * structure are to be used.In cases where the document contains 
	 * <code>CDATASections</code>, the normalize operation alone may not be 
	 * sufficient, since XPointers do not differentiate between 
	 * <code>Text</code> nodes and <code>CDATASection</code> nodes.
	 *
	 * @since DOM Level 2
	 */
	public void normalize()
	{
		Text text = null;
		int  idx  = 0;
		while (idx < chnum) {
			Node node = chlst[idx];
			if (node.getNodeType() == Node.TEXT_NODE) {
				if (text == null) {  // first text node
					text = (Text)node;
					if (text.getLength() == 0) {  // empty text node
						_removeChild(idx);
						text = null;
						continue;
					}
				} else {  // second text node
					text.appendData(((Text)node).getData());
					_removeChild(idx);
					continue;
				}
			} else {  // not a text node
				node.normalize();
				text = null;
			}
			idx++;
		}
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
		return ((index >= 0) && (index < chnum))? chlst[index]: null;
	}

	/**
	 * The number of nodes in the list. The range of valid child node indices 
	 * is 0 to <code>length - 1</code> inclusive. 
	 */
	public int getLength()
	{
		return chnum;
	}

	/**
	 * Sets node's owner document.
	 *
	 * @param ownerDoc New owner document.
	 */
	protected void _setDoc(XDoc ownerDoc)
	{
		super._setDoc(ownerDoc);
		for(int i = 0; i < chnum; i++) {
			chlst[i]._setDoc(ownerDoc);
		}
	}

	/**
	 * Return index of child node or a negative number.
	 */
	protected int _childIdx(Node child)
	{
		for(int i = 0; i < chnum; i++) {
			if (child == chlst[i])
				return i;
		}
		return -2147483648;  // the biggest possible negative int.
	}

	/**
	 * Check a new child node. This method throws appropriate exception if this 
	 * node does not like new child. A subclass MUST chain this method with 
	 * implementation provided by its parent class.
	 */
	protected void _checkNewChild(Node newChild, boolean replaceChild)
		throws DOMException
	{
		if (newChild == this || _isAncestor(newChild) == true)
			throw new DOMException(DOMException.HIERARCHY_REQUEST_ERR, "");

		switch(newChild.getNodeType()) {
		case DOCUMENT_TYPE_NODE:
			if (getNodeType() == DOCUMENT_NODE)
				break;
		case DOCUMENT_NODE:
		case DOCUMENT_FRAGMENT_NODE:
		case ATTRIBUTE_NODE:
		case ENTITY_NODE:
		case NOTATION_NODE:
			throw new DOMException(DOMException.HIERARCHY_REQUEST_ERR, "");

		default:
		}

		if (newChild.getOwnerDocument() != _getDoc() && 
			newChild.getNodeType() != DOCUMENT_TYPE_NODE)
			throw new DOMException(DOMException.WRONG_DOCUMENT_ERR, "");
	}

	/**
	 * Appends a child without read-only check.
	 */
	protected void _appendChild(XNode newChild)
		throws DOMException
	{
		_checkNewChild(newChild, false);

		Node oldParent = newChild.getParentNode();
		if (oldParent != null)
			oldParent.removeChild(newChild);

		if (chnum >= chlst.length)
			_extendChlst();

		chlst[chnum++] = newChild;
		if (newChild.getNodeType() == DOCUMENT_TYPE_NODE)
			newChild._setDoc(_getDoc());
		newChild._setParent(this);
	}

	/**
	 * Inserts a child at specified position.
	 */
	private void _insertChild(int index, XNode newChild)
	{
		if (newChild.getNodeType() != DOCUMENT_FRAGMENT_NODE) {
			//		Insert single node.
			_checkNewChild(newChild, false);
			Node pnode = newChild.getParentNode();
			if (pnode != null)
				pnode.removeChild(newChild);

			if (chnum + 1 >= chlst.length)
				_extendChlst();

			System.arraycopy(chlst, index, chlst, index + 1, chnum - index);
			chlst[index] = newChild;
			if (newChild.getNodeType() == DOCUMENT_TYPE_NODE)
				newChild._setDoc(_getDoc());
			newChild._setParent(this);
			chnum += 1;

			_childAdded(newChild);
		} else {
			//		Recursive call to _insertChild for each child of doc fragment.
			for (int idx = index; newChild.getLength() > 0; idx++)
				_insertChild(idx, (XNode)newChild.item(0));
		}
	}

	/**
	 * Removes a child at specified position.
	 */
	private Node _removeChild(int index)
		throws DOMException
	{
		XNode child = chlst[index];
		_childRemoving(child);

		if (index < (chnum - 1))
			System.arraycopy(chlst, index + 1, chlst, index, chnum - index);
		chnum -= 1;
		child._setParent(null);

		_childRemoved(child);
		return child;
	}

	/**
	 * Extends capacity of list of children.
	 */
	private void _extendChlst()
	{
	 	XNode list[] = new XNode[chlst.length + DEFAULT_CHILD_NUM];
		System.arraycopy(chlst, 0, list, 0, chnum);
		chlst = list;
	}

	/**
	 * Lets filter to process each child node.
	 */
	protected void _procEachChild(XList filter)
	{
		super._procEachChild(filter);
		for (int idx = 0; idx < chnum; idx++) {
			chlst[idx]._procEachChild(filter);
		}
	}

	/**
	 * Appends textual content of the node to the buffer.
	 */
	protected void _appendText(StringBuffer buffer)
	{
		switch(getNodeType()) {
		case ELEMENT_NODE:
		case ATTRIBUTE_NODE:
		case DOCUMENT_FRAGMENT_NODE:
		case ENTITY_REFERENCE_NODE:
		case ENTITY_NODE:
			for (int idx = 0; idx < chnum; idx++) {
				chlst[idx]._appendText(buffer);
			}
			break;

		default:
		}
	}
	
	/**
	 * Retrieves next child element. If <code>start</code> parameter 
	 * is null, it returns first child element, if any.
	 */
	protected XElm _nextElm(XElm start) 
	{
		int idx = (start != null) ? _childIdx(start) + 1 : 0;
		for (; idx < chnum; idx++) {
			if (chlst[idx].getNodeType() == ELEMENT_NODE)
				return (XElm) chlst[idx];
		}

		return null;
	}

	/**
	 * Retrieves previous child element. If <code>start</code> parameter 
	 * is null, it returns last child element, if any.
	 */	
	protected XElm _prevElm(XElm start) 
	{
		int idx = ((start != null) ? _childIdx(start) : chnum) - 1;
		for (; idx >= 0 ; idx--) {
			if (chlst[idx].getNodeType() == ELEMENT_NODE)
				return (XElm) chlst[idx];
		}

		return null;
	}
	
	/**
	 * Retrieves the number of child elements.
	 *
	 * @return the current number of element nodes that are children
	 * of this element. <code>0</code> if this element has no child
	 * elements.
	 */
	protected int _countChildElm() {
		int cnt = 0;
		for (int i = 0; i < chnum; i++) {
			if (chlst[i].getNodeType() == ELEMENT_NODE)
				cnt++;
		}
		
		return cnt;
	}

	/**
	 * Returns string representation of all children the node.
	 */
	public String toString()
	{
		StringBuffer out = new StringBuffer();

		for (int i = 0; i < getLength(); i++)
			out.append(item(i).toString());

		return out.toString();
	}
}
