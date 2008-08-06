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
import org.w3c.dom.Document;
import org.w3c.dom.NodeList;
import org.w3c.dom.NamedNodeMap;
import org.w3c.dom.UserDataHandler;
import org.w3c.dom.DOMException;

import java.util.Hashtable;
import java.util.Enumeration;

/**
 * Base DOM node functionality implementation.
 *
 * @see org.w3c.dom.Node
 */

public abstract class XBase
	implements Node, NodeList
{
	/** XML namespace URI. */
	public final static String URI_XML = "http://www.w3.org/XML/1998/namespace";
	/** XML namespace definition namespace URI. */
	public final static String URI_XMLNS = "http://www.w3.org/2000/xmlns/";

	/** Owner document */
	private XDoc    owner;
	/** Parent node */
	private XParent parent;

	/** Qualified name. */
	private String qname;
	/** Local name. */
	private String lname;
	/** Namespace URI. */
	private String nsuri;

	/** User data. */
	private Hashtable usrdat;

	/**
	 * XML name character type array.
	 * This array maps an ASCII (7 bit) character to the character type.<br /> 
	 * Defined character types are:<br /> 
	 * - 0 for underscore ('_') or any lower and upper case alphabetical character value;<br /> 
	 * - 1 for colon (':') character;<br /> 
	 * - 2 for dash ('-') and dot ('.') or any decimal digit character value;<br /> 
	 * - 3 for any kind of white space character<br /> 
	 * An ASCII (7 bit) character which does not fall in any category listed 
	 * above is mapped to 0xff.
	 */
	private static final byte chtyp[];

	/**
	 * Static constructor.
	 *
	 * Sets up the XML name character type array which is used by 
	 * {@link #_chekName _chekName} method.
	 */
	static {
		short i = 0;
		chtyp   = new byte[0x80];
		for (i = 0; i < '0'; i++)
			chtyp[i]   = (byte)0xff;
		while (i <= '9')
			chtyp[i++] = (byte)2;  // digits
		while (i < 'A')
			chtyp[i++] = (byte)0xff;
		// skipped upper case alphabetical character are already 0
		for (i = '['; i < 'a'; i++)
			chtyp[i]   = (byte)0xff;
		// skipped lower case alphabetical character are already 0
		for (i = '{'; i < 0x80; i++)
			chtyp[i]   = (byte)0xff;
		chtyp['_']  = 0;
		chtyp[':']  = 1;
		chtyp['.']  = 2;
		chtyp['-']  = 2;
		chtyp[' ']  = 3;
		chtyp['\t'] = 3;
		chtyp['\r'] = 3;
		chtyp['\n'] = 3;
	}

	/**
	 * Constructs node object from its owner document.
	 */
	protected XBase(XDoc ownerDocument)
	{
	 	owner = (XDoc)ownerDocument;
	}

	/**
	 * Constructs node object from other node.
	 */
	 protected XBase(XNode node, boolean deep)
	 {
	 	owner = ((XBase)node).owner;
	 	qname = ((XBase)node).qname;
	 	lname = ((XBase)node).lname;
	 	nsuri = ((XBase)node).nsuri;
	 }

	/**
	 * Constructs node object from its qualified name and namespace URI and 
	 * its owner document.
	 */
	 protected XBase(String namespaceURI, String qName, XDoc ownerDocument)
	 {
	 	owner = (XDoc)ownerDocument;
		qname = (owner != null)? owner._intern(qName): qName;
		if (namespaceURI != null && qname.charAt(0) != '#') {
			nsuri = owner._intern(namespaceURI);
			lname = owner._intern(XNode._getLocalName(qname));
		} else {
			nsuri = null;
			lname = null;
		}
	 }

	/**
	 * A code representing the type of the underlying object, as defined above.
	 */
	public abstract short getNodeType();

	/**
	 * The namespace URI of this node, or <code>null</code> if it is 
	 * unspecified.
	 * <br>This is not a computed value that is the result of a namespace 
	 * lookup based on an examination of the namespace declarations in 
	 * scope. It is merely the namespace URI given at creation time.
	 * <br>For nodes of any type other than <code>ELEMENT_NODE</code> and 
	 * <code>ATTRIBUTE_NODE</code> and nodes created with a DOM Level 1 
	 * method, such as <code>createElement</code> from the 
	 * <code>Document</code> interface, this is always <code>null</code>.Per 
	 * the Namespaces in XML Specification  an attribute does not inherit 
	 * its namespace from the element it is attached to. If an attribute is 
	 * not explicitly given a namespace, it simply has no namespace.
	 *
	 * @since DOM Level 2
	 */
	public String getNamespaceURI()
	{
		return (nsuri != null && nsuri.length() == 0)? null: nsuri;
	}

	/**
	 * The namespace prefix of this node, or <code>null</code> if it is 
	 * unspecified. When it is defined to be <code>null</code>, setting it 
	 * has no effect, including if the node is read-only.
	 * <br>Note that setting this attribute, when permitted, changes the 
	 * <code>nodeName</code> attribute, which holds the qualified name, as 
	 * well as the <code>tagName</code> and <code>name</code> attributes of 
	 * the <code>Element</code> and <code>Attr</code> interfaces, when 
	 * applicable.
	 * <br>Setting the prefix to <code>null</code> makes it unspecified, 
	 * setting it to an empty string is implementation dependent.
	 * <br>Note also that changing the prefix of an attribute that is known to 
	 * have a default value, does not make a new attribute with the default 
	 * value and the original prefix appear, since the 
	 * <code>namespaceURI</code> and <code>localName</code> do not change.
	 * <br>For nodes of any type other than <code>ELEMENT_NODE</code> and 
	 * <code>ATTRIBUTE_NODE</code> and nodes created with a DOM Level 1 
	 * method, such as <code>createElement</code> from the 
	 * <code>Document</code> interface, this is always <code>null</code>.
	 * @return The namespace prefix of this node, or <code>null</code>
	 *
	 * @since DOM Level 2
	 */
	public String getPrefix()
	{
		return (nsuri != null && qname.length() != lname.length())? 
			qname.substring(0, qname.length() - lname.length() - 1): null;
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
	 *
	 * @since DOM Level 2
	 */
	public void setPrefix(String prefix)
		throws DOMException
	{
		//		This method is implemented on Element and Attr only.
		return;
	}

	/**
	 * Returns the local part of the qualified name of this node.
	 * <br>For nodes of any type other than <code>ELEMENT_NODE</code> and 
	 * <code>ATTRIBUTE_NODE</code> and nodes created with a DOM Level 1 
	 * method, such as <code>createElement</code> from the 
	 * <code>Document</code> interface, this is always <code>null</code>.
	 *
	 * @since DOM Level 2
	 */
	public String getLocalName()
	{
		return (nsuri != null)? lname: null;
	}

	/**
	 * The name of this node, depending on its type; see the table above. 
	 */
	public String getNodeName()
	{
		return qname;
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
	public String getNodeValue()
		throws DOMException
	{
		return null;
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
		return parent;
	}

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
		return null;
	}

	/**
	 * The last child of this node. If there is no such node, this returns 
	 * <code>null</code>.
	 */
	public Node getLastChild()
	{
		return null;
	}

	/**
	 * The node immediately preceding this node. If there is no such node, 
	 * this returns <code>null</code>.
	 */
	public Node getPreviousSibling()
	{
		return (parent != null)? 
			parent.item(parent._childIdx(this) - 1): null;
	}

	/**
	 * The node immediately following this node. If there is no such node, 
	 * this returns <code>null</code>.
	 */
	public Node getNextSibling()
	{
		return (parent != null)? 
			parent.item(parent._childIdx(this) + 1): null;
	}

	/**
	 * A <code>NamedNodeMap</code> containing the attributes of this node (if 
	 * it is an <code>Element</code>) or <code>null</code> otherwise. 
	 */
	public NamedNodeMap getAttributes()
	{
		return null;
	}

	/**
	 * The <code>Document</code> object associated with this node. This is 
	 * also the <code>Document</code> object used to create new nodes. When 
	 * this node is a <code>Document</code> or a <code>DocumentType</code> 
	 * which is not used with any <code>Document</code> yet, this is 
	 * <code>null</code>.
	 *
	 * @since DOM Level 2
	 */
	public Document getOwnerDocument()
	{
		return owner;
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
		throw new DOMException(DOMException.HIERARCHY_REQUEST_ERR, "");
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
    	throw new DOMException(DOMException.NOT_FOUND_ERR, "");
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
    	throw new DOMException(DOMException.NOT_FOUND_ERR, "");
	}

	/**
	 * Adds the node <code>newChild</code> to the end of the list of children 
	 * of this node. If the <code>newChild</code> is already in the tree, it 
	 * is first removed.
	 *
	 * @param newChild The node to add.If it is a <code>DocumentFragment</code>
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
    	throw new DOMException(DOMException.HIERARCHY_REQUEST_ERR, "");
	}

	/**
	 * Returns whether this node has any children.
	 *
	 * @return  <code>true</code> if this node has any children, 
	 *   <code>false</code> otherwise.
	 */
	public boolean hasChildNodes()
	{
		return false;
	}

	/**
	 * Returns a duplicate of this node, i.e., serves as a generic copy 
	 * constructor for nodes. The duplicate node has no parent; (
	 * <code>parentNode</code> is <code>null</code>.).
	 * <br>Cloning an <code>Element</code> copies all attributes and their 
	 * values, including those generated by the XML processor to represent 
	 * defaulted attributes, but this method does not copy any text it 
	 * contains unless it is a deep clone, since the text is contained in a 
	 * child <code>Text</code> node. Cloning an <code>Attribute</code> 
	 * directly, as opposed to be cloned as part of an <code>Element</code> 
	 * cloning operation, returns a specified attribute (
	 * <code>specified</code> is <code>true</code>). Cloning any other type 
	 * of node simply returns a copy of this node.
	 * <br>Note that cloning an immutable subtree results in a mutable copy, 
	 * but the children of an <code>EntityReference</code> clone are readonly
	 * . In addition, clones of unspecified <code>Attr</code> nodes are 
	 * specified. And, cloning <code>Document</code>, 
	 * <code>DocumentType</code>, <code>Entity</code>, and 
	 * <code>Notation</code> nodes is implementation dependent.
	 *
	 * @param deep If <code>true</code>, recursively clone the subtree under 
	 *   the specified node; if <code>false</code>, clone only the node 
	 *   itself (and its attributes, if it is an <code>Element</code>). 
	 * @return The duplicate node.
	 */
	public Node cloneNode(boolean deep)
	{
		XNode node = owner._clone(this, deep);
		_notifyUDH(UserDataHandler.NODE_CLONED, node);
		return node;
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
	}

	/**
	 * Tests whether the DOM implementation implements a specific feature and 
	 * that feature is supported by this node.
	 *
	 * @param feature The name of the feature to test. This is the same name 
	 *   which can be passed to the method <code>hasFeature</code> on 
	 *   <code>DOMImplementation</code>.
	 * @param version This is the version number of the feature to test. In 
	 *   Level 2, version 1, this is the string "2.0". If the version is not 
	 *   specified, supporting any version of the feature will cause the 
	 *   method to return <code>true</code>.
	 * @return Returns <code>true</code> if the specified feature is 
	 *   supported on this node, <code>false</code> otherwise.
	 *
	 * @since DOM Level 2
	 */
	public boolean isSupported(String feature, String version)
	{
		return _getDoc()._isSupported(feature, version);
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
		return false;
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
		case TEXT_NODE:
		case CDATA_SECTION_NODE:
		case COMMENT_NODE:
		case PROCESSING_INSTRUCTION_NODE:
		case ATTRIBUTE_NODE:
			return getNodeValue();

		case DOCUMENT_NODE:
		case DOCUMENT_TYPE_NODE:
		case NOTATION_NODE:
			break;

		default:
		}
		return null;
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

		switch(getNodeType()) {
		case TEXT_NODE:
		case CDATA_SECTION_NODE:
		case COMMENT_NODE:
		case PROCESSING_INSTRUCTION_NODE:
		case ATTRIBUTE_NODE:
			if (_isRO())
				throw new DOMException(
					DOMException.NO_MODIFICATION_ALLOWED_ERR, "");
			setNodeValue((textContent != null)? textContent: "");
			break;

		case DOCUMENT_NODE:
		case DOCUMENT_TYPE_NODE:
		case NOTATION_NODE:
			break;

		default:
		}
	}

	/**
	 *  This method returns a specialized object which implements the 
	 * specialized APIs of the specified feature and version, as specified 
	 * in . The specialized object may also be obtained by using 
	 * binding-specific casting methods but is not necessarily expected to, 
	 * as discussed in <a href="http://www.w3.org/TR/DOM-Level-3-Core/core.html#Embedded-DOM">
	 * Mixed DOM Implementations</a>. This method also allow the implementation 
	 * to provide specialized objects which do not support the <code>Node</code>
	 * interface. 
	 *
	 * @param feature  The name of the feature requested. Note that any plus 
	 *   sign "+" prepended to the name of the feature will be ignored since 
	 *   it is not significant in the context of this method. 
	 * @param version  This is the version number of the feature to test. 
	 * @return  Returns an object which implements the specialized APIs of 
	 *   the specified feature and version, if any, or <code>null</code> if 
	 *   there is no object which implements interfaces associated with that 
	 *   feature. If the <code>DOMObject</code> returned by this method 
	 *   implements the <code>Node</code> interface, it must delegate to the 
	 *   primary core <code>Node</code> and not return results inconsistent 
	 *   with the primary core <code>Node</code> such as attributes, 
	 *   childNodes, etc. 
	 *
	 * @since DOM Level 3
	 */
	public Object getFeature(String feature, String version)
	{
		return (_getDoc()._isSupported(feature, version))? this: null;
	}

	/**
	 * Associate an object to a key on this node. The object can later be 
	 * retrieved from this node by calling <code>getUserData</code> with the 
	 * same key.
	 *
	 * @param key The key to associate the object to.
	 * @param data The object to associate to the given key, or 
	 *   <code>null</code> to remove any existing association to that key.
	 * @param handler The handler to associate to that key, or 
	 *   <code>null</code>.
	 * @return Returns the <code>DOMUserData</code> previously associated to 
	 *   the given key on this node, or <code>null</code> if there was none.
	 *
	 * @since DOM Level 3
	 */
	public final Object setUserData(
			String key, Object data, UserDataHandler handler)
	{
		Object[] olddat = null;

		if (data == null && usrdat == null)
			return null;

		if (usrdat != null)
			olddat = (Object[])usrdat.get(key);
		else
			usrdat = new Hashtable(4);

		if (data != null) {
			Object[] kdh = new Object[3];
			kdh[0] = key;
			kdh[1] = data;
			kdh[2] = handler;
			usrdat.put(key, kdh);
		} else {
			usrdat.remove(key);
		}

		return (olddat != null)? olddat[1]: null;
	}

	/**
	 * Retrieves the object associated to a key on a this node. The object 
	 * must first have been set to this node by calling 
	 * <code>setUserData</code> with the same key.
	 *
	 * @param key The key the object is associated to.
	 * @return Returns the <code>DOMUserData</code> associated to the given 
	 *   key on this node, or <code>null</code> if there was none.
	 *
	 * @since DOM Level 3
	 */
	public final Object getUserData(String key)
	{
		if (usrdat == null)
			return null;
		Object[] pair = (Object[])usrdat.get(key);
		return (pair != null)? pair[1]: null;
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
		return null;
	}

	/**
	 * The number of nodes in the list. The range of valid child node indices 
	 * is 0 to <code>length - 1</code> inclusive. 
	 */
	public int getLength()
	{
		return 0;
	}

	/**
	 * Sets node's name attributes.
	 */
	protected final void _set(
			String namespaceURI, String qName, String localName)
	{
		nsuri = namespaceURI;
		qname = qName;
		lname = localName;
	}

	/**
	 * Sets node's name prefix.
	 */
	protected final void _setPrefix(String prefix)
		throws DOMException
	{
		if (_isRO())
			throw new DOMException(DOMException.NO_MODIFICATION_ALLOWED_ERR, "");

		String name = (prefix != null)? (prefix + ':' + lname): lname;
		XBase._checkNameNS(nsuri, name);
		qname = owner._intern(name);
	}

	/**
	 * Sets node's value attributes.
	 */
	protected void _setValue(String value)
	{
	}

	/**
	 * Updates node's value attributes.
	 */
	protected void _updateValue(String value)
	{
	}

	/**
	 * Cleans up node.
	 */
	protected void _clear()
	{
		_set(null, null, null);
	}

	/**
	 * Retrieves node's parent node. Note, for an attribute node this method 
	 * retrieves element owner.
	 */
	protected final XParent _getParent()
	{
		return parent;
	}

	/**
	 * Sets node's parent node.
	 */
	protected void _setParent(XParent parentNode)
	{
		parent = parentNode;
	}

	/**
	 * Retrieves node's owner document.
	 */
	protected final XDoc _getDoc()
	{
		return (getNodeType() != DOCUMENT_NODE)? owner: (XDoc)this;
	}

	/**
	 * Sets node's owner document.
	 *
	 * @param ownerDoc New owner document.
	 */
	protected void _setDoc(XDoc ownerDoc)
	{
		if (owner == ownerDoc)
			return;
		if (getNodeType() != DOCUMENT_NODE)
			owner = ownerDoc;
		if (owner != null) {
			qname = owner._intern(qname);
			nsuri = owner._intern(nsuri);
			lname = owner._intern(lname);
		}
	}

	/**
	 * Return true if node or one of its parents is read only.
	 */
	protected boolean _isRO()
	{
		return (parent != null)? parent._isRO(): false;
	}

	/**
	 * Return true if node created by namespace-aware methods.
	 */
	protected final boolean _isNS()
	{
		return (nsuri != null);
	}

	/**
	 * Returns <code>true</code> if node is one of ancestors of this node.
	 */
	protected final boolean _isAncestor(Node node)
	{
		if (parent == null)
			return false;
		if (parent == node)
			return true;
		return parent._isAncestor(node);
	}

	/**
	 * Notifies all user data handlers on this node.
	 */
	protected void _notifyUDH(short action, XNode node)
	{
 		if (usrdat == null)
			return;

		Enumeration elms = usrdat.elements();
		while (elms.hasMoreElements()) {
			Object[] data = (Object[])elms.nextElement();
			UserDataHandler udh = (UserDataHandler)data[2];
			if (udh != null)
				udh.handle(action, (String)data[0], data[1], this, node);
		}
	}

	/**
	 * Lets filter to process each child node.
	 */
	protected void _procEachChild(XList filter)
	{
		filter._proc((XNode)this);
	}

	/**
	 * Appends textual content of the node to the buffer.
	 */
	protected void _appendText(StringBuffer buffer)
	{
		switch(getNodeType()) {
		case TEXT_NODE:
		case CDATA_SECTION_NODE:
			buffer.append(getNodeValue());
			break;

		case COMMENT_NODE:
		case PROCESSING_INSTRUCTION_NODE:
		default:
		}
	}

	/**
	 * Checks qualified name.
	 *
	 * @param qName The XML name to check.
	 * @param ns The true value turns namespace conformance on.
	 * @return an index to colon (prefix separator) or -1
	 *
	 * @exception DOMException
	 *   INVALID_CHARACTER_ERR: Raised when qName is invalid XML name or 
	 *   an empty string.
	 * @exception NullPointerException Raised when qName is <code>null</code>.
	 */
	protected final static int _checkName(String qName, boolean ns)
		throws DOMException
	{
		char  ch;
		char  type;
		int   colon = -1;

		int st  = (ns == true)? 0: 2;
		int idx = 0;
		chloop: while (idx < qName.length()) {
			//		Read next character
			ch   = qName.charAt(idx++);
			type = (char)0;  // any char above 0x7f
			if (ch < 0x80)
				type = (char)chtyp[ch];
			//		Parse QName
			switch (st) {
			case 0:     // read the first char of the prefix
			case 2:     // read the first char of the suffix
				switch (type) {
				case 0:  // [aA_X]
					st++;      // (st == 0)? 1: 3;
					break;

				case 1:  // [:]
					if (st == 2) { // read the first char of the suffix					
						idx--;
						st = 3;    // read the suffix
						break;
					}

				default:
					idx = -1;
					break chloop;  // invalid character
				}
				break;

			case 1:     // read the prefix
			case 3:     // read the suffix
				switch (type) {
				case 0:  // [aA_X]
				case 2:  // [.-d]
					break;

				case 1:  // [:]
					if (ns == true) {
						//		It must be only one colon and it may not be 
						//		the last character of qName
						if ((colon >= 0) || (idx >= qName.length()))
							throw new DOMException(
								DOMException.NAMESPACE_ERR, qName);
						colon = idx - 1;
						if (st == 1)
							st = 2;
					}
					break;

				default:
					idx = -1;
					break chloop;  // invalid character
				}
				break;

			default:
				idx = -1;
				break chloop;  // invalid character
			}
		}
		if (idx <= 0)
			throw new DOMException(DOMException.INVALID_CHARACTER_ERR, qName);

		return colon;
	}

	/**
	 * Checks qualified name and namespace URI.
	 */
	protected final static int _checkNameNS(String uri, String qName)
		throws DOMException
	{
		int idx = XBase._checkName(qName, true);

		//		There is a prefix but there is no NS URI
		if (idx >= 0 && (uri == null || (uri != null && uri.length() == 0)))
			throw new DOMException(DOMException.NAMESPACE_ERR, "");
		//		Identify 'xml' and 'xmlns' prefixes
		int xmlns = 0;
		if (qName.charAt(0) == 'x' && 
			qName.charAt(1) == 'm' && 
			qName.charAt(2) == 'l') {
			xmlns = (idx == 3)? 1: 0;  // 'xml'
			if (qName.charAt(3) == 'n' && 
				qName.charAt(4) == 's') {
				if (idx == 5 || qName.length() == 5) 
					xmlns = 2;  // 'xmlns' prefix or qName
			}
		}
		//		There is 'xml' prefix and namespace URI is not URI_XML
		if (xmlns == 1 && uri != null && !uri.equals(URI_XML))
			throw new DOMException(DOMException.NAMESPACE_ERR, URI_XML);

		if (URI_XMLNS.equals(uri) == true) {
			//		NS URI is URI_XMLNS but qName or prefix is not 'xmlns'
			if (xmlns != 2)
				throw new DOMException(DOMException.NAMESPACE_ERR, URI_XMLNS);
		} else {
			//		There is 'xmlns' qName or prefix and NS URI is not URI_XMLNS
			if (xmlns == 2)
				throw new DOMException(DOMException.NAMESPACE_ERR, URI_XMLNS);
		}

		return idx;
	}

	/**
	 * Returns local name substring of qualified name string.
	 */
	protected final static String _getLocalName(String qName)
	{
		int idx = qName.indexOf(':');
		return (idx >= 0)? qName.substring(idx + 1): qName;
	}
}
