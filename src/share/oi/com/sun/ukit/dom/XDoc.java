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

import java.util.Hashtable;
import java.util.Enumeration;

import org.w3c.dom.DOMImplementation;
import org.w3c.dom.Node;
import org.w3c.dom.Attr;
import org.w3c.dom.Element;
import org.w3c.dom.Text;
import org.w3c.dom.Comment;
import org.w3c.dom.Document;
import org.w3c.dom.DocumentType;
import org.w3c.dom.DocumentFragment;
import org.w3c.dom.EntityReference;
import org.w3c.dom.ProcessingInstruction;
import org.w3c.dom.CDATASection;
import org.w3c.dom.NodeList;
import org.w3c.dom.UserDataHandler;
import org.w3c.dom.DOMException;

/**
 * DOM document node implementation.
 *
 * @see org.w3c.dom.Node
 */

public abstract class XDoc
	extends XParent
	implements Document
{
	/** Document's type. */
	private DocumentType type;
	/** DOM implementation this document belongs to. */
	private DOMImplementation impl;
	/** Document's root element. */
	private Element root;
	/** Document's default attributes. Map of qnames to string arrays. */
	private Hashtable attrs;
	/** Document's names. */
	private Hashtable names;

	/* pkg */ final static char FLAG_BUILD = 0x0001;  /* set during build */
	/* pkg */ char flags;

	/**
	 * Constructs document object from its document type and DOM implementation 
	 * object.
	 */
	 protected XDoc(DOMImplementation implementation)
	 {
	 	super(null, "#document", null);

		attrs = new Hashtable();
		names = new Hashtable();

	 	impl = implementation;
	 }

	/**
	 * A code representing the type of the underlying object, as defined above.
	 */
	public abstract short getNodeType();

	/**
	 * The Document Type Declaration (see <code>DocumentType</code>) 
	 * associated with this document. For HTML documents as well as XML 
	 * documents without a document type declaration this returns 
	 * <code>null</code>. The DOM Level 2 does not support editing the 
	 * Document Type Declaration. <code>docType</code> cannot be altered in 
	 * any way, including through the use of methods inherited from the 
	 * <code>Node</code> interface, such as <code>insertNode</code> or 
	 * <code>removeNode</code>.
	 */
	public DocumentType getDoctype()
	{
		return type;
	}

	/**
	 * The <code>DOMImplementation</code> object that handles this document. A 
	 * DOM application may use objects from multiple implementations.
	 */
	public DOMImplementation getImplementation()
	{
		return impl;
	}

	/**
	 * This is a convenience attribute that allows direct access to the child 
	 * node that is the root element of the document. For HTML documents, 
	 * this is the element with the tagName "HTML".
	 */
	public Element getDocumentElement()
	{
		return root;
	}

	/**
	 * Creates an element of the type specified. Note that the instance 
	 * returned implements the <code>Element</code> interface, so attributes 
	 * can be specified directly on the returned object.
	 * <br>In addition, if there are known attributes with default values, 
	 * <code>Attr</code> nodes representing them are automatically created 
	 * and attached to the element.
	 * <br>To create an element with a qualified name and namespace URI, use 
	 * the <code>createElementNS</code> method.
	 *
	 * @param tagName The name of the element type to instantiate. For XML, 
	 *   this is case-sensitive. For HTML, the <code>tagName</code> 
	 *   parameter may be provided in any case, but it must be mapped to the 
	 *   canonical uppercase form by the DOM implementation. 
	 * @return A new <code>Element</code> object with the 
	 *   <code>nodeName</code> attribute set to <code>tagName</code>, and 
	 *   <code>localName</code>, <code>prefix</code>, and 
	 *   <code>namespaceURI</code> set to <code>null</code>.
	 * @exception DOMException
	 *   INVALID_CHARACTER_ERR: Raised if the specified name contains an 
	 *   illegal character.
	 */
	public Element createElement(String tagName)
		throws DOMException
	{
		XNode._checkName(tagName, false);
		return new ElmImp(null, tagName, this);
	}

	/**
	 * Creates an element of the given qualified name and namespace URI. 
	 * HTML-only DOM implementations do not need to implement this method.
	 *
	 * @param namespaceURI The namespace URI of the element to create.
	 * @param qualifiedName The qualified name of the element type to 
	 *   instantiate.
	 * @return A new <code>Element</code> object with the following 
	 *   attributes:AttributeValue<code>Node.nodeName</code>
	 *   <code>qualifiedName</code><code>Node.namespaceURI</code>
	 *   <code>namespaceURI</code><code>Node.prefix</code>prefix, extracted 
	 *   from <code>qualifiedName</code>, or <code>null</code> if there is 
	 *   no prefix<code>Node.localName</code>local name, extracted from 
	 *   <code>qualifiedName</code><code>Element.tagName</code>
	 *   <code>qualifiedName</code>
	 * @exception DOMException
	 *   INVALID_CHARACTER_ERR: Raised if the specified qualified name 
	 *   contains an illegal character.
	 *   <br>NAMESPACE_ERR: Raised if the <code>qualifiedName</code> is 
	 *   malformed, if the <code>qualifiedName</code> has a prefix and the 
	 *   <code>namespaceURI</code> is <code>null</code>, or if the 
	 *   <code>qualifiedName</code> has a prefix that is "xml" and the 
	 *   <code>namespaceURI</code> is different from "
	 *   http://www.w3.org/XML/1998/namespace" .
	 *
	 * @since DOM Level 2
	 */
	public Element createElementNS(String namespaceURI, String qualifiedName)
		throws DOMException
	{
		int off = XNode._checkNameNS(namespaceURI, qualifiedName);
		if (off == 5 && qualifiedName.startsWith("xmlns"))
			throw new DOMException(DOMException.NAMESPACE_ERR, URI_XML);
		String nsuri = (namespaceURI != null)? namespaceURI: "";
		return new ElmImp(nsuri, qualifiedName, this);
	}

	/**
	 * Creates an <code>Attr</code> of the given name. Note that the 
	 * <code>Attr</code> instance can then be set on an <code>Element</code> 
	 * using the <code>setAttributeNode</code> method. 
	 * <br>To create an attribute with a qualified name and namespace URI, use 
	 * the <code>createAttributeNS</code> method.
	 *
	 * @param name The name of the attribute.
	 * @return A new <code>Attr</code> object with the <code>nodeName</code> 
	 *   attribute set to <code>name</code>, and <code>localName</code>, 
	 *   <code>prefix</code>, and <code>namespaceURI</code> set to 
	 *   <code>null</code>. The value of the attribute is the empty string.
	 * @exception DOMException
	 *   INVALID_CHARACTER_ERR: Raised if the specified name contains an 
	 *   illegal character.
	 */
	public Attr createAttribute(String name)
		throws DOMException
	{
		XNode._checkName(name, false);
		return new AttrImp(null, name, this);
	}

	/**
	 * Creates an attribute of the given qualified name and namespace URI. 
	 * HTML-only DOM implementations do not need to implement this method.
	 *
	 * @param namespaceURI The namespace URI of the attribute to create.
	 * @param qualifiedName The qualified name of the attribute to instantiate.
	 * @return A new <code>Attr</code> object with the following attributes:
	 *   AttributeValue<code>Node.nodeName</code>qualifiedName
	 *   <code>Node.namespaceURI</code><code>namespaceURI</code>
	 *   <code>Node.prefix</code>prefix, extracted from 
	 *   <code>qualifiedName</code>, or <code>null</code> if there is no 
	 *   prefix<code>Node.localName</code>local name, extracted from 
	 *   <code>qualifiedName</code><code>Attr.name</code>
	 *   <code>qualifiedName</code><code>Node.nodeValue</code>the empty 
	 *   string
	 * @exception DOMException
	 *   INVALID_CHARACTER_ERR: Raised if the specified qualified name 
	 *   contains an illegal character.
	 *   <br>NAMESPACE_ERR: Raised if the <code>qualifiedName</code> is 
	 *   malformed, if the <code>qualifiedName</code> has a prefix and the 
	 *   <code>namespaceURI</code> is <code>null</code>, if the 
	 *   <code>qualifiedName</code> has a prefix that is "xml" and the 
	 *   <code>namespaceURI</code> is different from "
	 *   http://www.w3.org/XML/1998/namespace", or if the 
	 *   <code>qualifiedName</code> is "xmlns" and the 
	 *   <code>namespaceURI</code> is different from "
	 *   http://www.w3.org/2000/xmlns/".
	 * @since DOM Level 2
	 */
	public Attr createAttributeNS(String namespaceURI, String qualifiedName)
		throws DOMException
	{
		XNode._checkNameNS(namespaceURI, qualifiedName);
		String nsuri = (namespaceURI != null)? namespaceURI: "";
		return new AttrImp(nsuri, qualifiedName, this);
	}

	/**
	 * Creates a <code>Text</code> node given the specified string.
	 *
	 * @param data The data for the node.
	 * @return The new <code>Text</code> object.
	 */
	public Text createTextNode(String data)
	{
		return new TextImp(TEXT_NODE, data, this);
	}

	/**
	 * Creates a <code>Comment</code> node given the specified string.
	 *
	 * @param data The data for the node.
	 * @return The new <code>Comment</code> object.
	 */
	public Comment createComment(String data)
	{
		return new CommImp(data, this);
	}

	/**
	 * Creates a <code>CDATASection</code> node whose value is the specified 
	 * string.
	 *
	 * @param data The data for the <code>CDATASection</code> contents.
	 * @return The new <code>CDATASection</code> object.
	 * @exception DOMException
	 *   NOT_SUPPORTED_ERR: Raised if this document is an HTML document.
	 */
	public CDATASection createCDATASection(String data)
		throws DOMException
	{
		if (getDocumentElement() != null && 
			"HTML".equals(getDocumentElement().getTagName()))
			throw new DOMException(DOMException.NOT_SUPPORTED_ERR, "");

		return new TextImp(CDATA_SECTION_NODE, data, this);
	}

	/**
	 * Creates a <code>ProcessingInstruction</code> node given the specified 
	 * name and data strings.
	 *
	 * @param target The target part of the processing instruction.
	 * @param data The data for the node.
	 * @return The new <code>ProcessingInstruction</code> object.
	 * @exception DOMException
	 *   INVALID_CHARACTER_ERR: Raised if the specified target contains an 
	 *   illegal character.
	 *   <br>NOT_SUPPORTED_ERR: Raised if this document is an HTML document.
	 */
	public ProcessingInstruction createProcessingInstruction(
			String target, String data)
		throws DOMException
	{
		if (getDocumentElement() != null && 
			"HTML".equals(getDocumentElement().getTagName()))
			throw new DOMException(DOMException.NOT_SUPPORTED_ERR, "");

		XNode._checkName(target, false);
		return new ProcInstImp(target, data, this);
	}

	/**
	 * Creates an <code>EntityReference</code> object. In addition, if the 
	 * referenced entity is known, the child list of the 
	 * <code>EntityReference</code> node is made the same as that of the 
	 * corresponding <code>Entity</code> node.If any descendant of the 
	 * <code>Entity</code> node has an unbound namespace prefix, the 
	 * corresponding descendant of the created <code>EntityReference</code> 
	 * node is also unbound; (its <code>namespaceURI</code> is 
	 * <code>null</code>). The DOM Level 2 does not support any mechanism to 
	 * resolve namespace prefixes.
	 *
	 * @param name The name of the entity to reference. 
	 * @return The new <code>EntityReference</code> object.
	 * @exception DOMException
	 *   INVALID_CHARACTER_ERR: Raised if the specified name contains an 
	 *   illegal character.
	 *   <br>NOT_SUPPORTED_ERR: Raised if this document is an HTML document.
	 */
	public EntityReference createEntityReference(String name)
		throws DOMException
	{
		XNode._checkName(name, false);
		return new EntRefImp(name, this);
	}

	/**
	 * Creates an empty <code>DocumentFragment</code> object. 
	 *
	 * @return A new <code>DocumentFragment</code>.
	 */
	public DocumentFragment createDocumentFragment()
	{
		return new DocFragImp(this);
	}

	/**
	 * Returns a <code>NodeList</code> of all the <code>Elements</code> with a 
	 * given tag name in the order in which they are encountered in a 
	 * preorder traversal of the <code>Document</code> tree. 
	 *
	 * @param tagname The name of the tag to match on. The special value "*" 
	 *   matches all tags.
	 * @return A new <code>NodeList</code> object containing all the matched 
	 *   <code>Elements</code>.
	 */
	public NodeList getElementsByTagName(String tagname)
	{
		ElmFilter filter = new ElmFilter(tagname);
		if (getDocumentElement() != null)
			((XElm)getDocumentElement())._procEachChild(filter);
		return filter;
	}

	/**
	 * Returns a <code>NodeList</code> of all the <code>Elements</code> with a 
	 * given local name and namespace URI in the order in which they are 
	 * encountered in a preorder traversal of the <code>Document</code> tree.
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
		if (getDocumentElement() != null)
			((XElm)getDocumentElement())._procEachChild(filter);
		return filter;
	}

	/**
	 * Returns the <code>Element</code> whose <code>ID</code> is given by 
	 * <code>elementId</code>. If no such element exists, returns 
	 * <code>null</code>. Behavior is not defined if more than one element 
	 * has this <code>ID</code>. The DOM implementation must have 
	 * information that says which attributes are of type ID. Attributes 
	 * with the name "ID" are not of type ID unless so defined. 
	 * Implementations that do not know whether attributes are of type ID or 
	 * not are expected to return <code>null</code>.
	 *
	 * @param elementId The unique <code>id</code> value for an element.
	 * @return The matching element.
	 *
	 * @since DOM Level 2
	 */
	public Element getElementById(String elementId)
	{
		return ((XElm)getDocumentElement())._getElmById(elementId);
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
		XDoc doc = new DocImp(impl);
		//		Copy all default attributes
		Enumeration elms = attrs.keys();
		while (elms.hasMoreElements()) {
			String elm      = (String)elms.nextElement();
			String dattrs[] = _getDefAttrs(elm);
			for (int idx = 0; idx < (dattrs.length >> 2); idx++) {
				int base = idx << 2;
				doc._setDefAttr(elm, dattrs[base], dattrs[base + 3]);
			}
		}
		if (deep == true) {
			//		Add clones of all children
			for (int idx = 0; idx < getLength(); idx++) {
				XNode node = (XNode)(item(idx).cloneNode(true));
				node._setDoc(doc);
				doc._appendChild(node);
				doc._childAdded(node);
			}
		}
		_notifyUDH(UserDataHandler.NODE_CLONED, doc);
		return doc;
	}

	/**
	 * Imports a node from another document to this document. The returned 
	 * node has no parent; (<code>parentNode</code> is <code>null</code>). 
	 * The source node is not altered or removed from the original document; 
	 * this method creates a new copy of the source node.
	 * <br>For all nodes, importing a node creates a node object owned by the 
	 * importing document, with attribute values identical to the source 
	 * node's <code>nodeName</code> and <code>nodeType</code>, plus the 
	 * attributes related to namespaces (<code>prefix</code>, 
	 * <code>localName</code>, and <code>namespaceURI</code>). As in the 
	 * <code>cloneNode</code> operation on a <code>Node</code>, the source 
	 * node is not altered.
	 * <br>Additional information is copied as appropriate to the 
	 * <code>nodeType</code>, attempting to mirror the behavior expected if 
	 * a fragment of XML or HTML source was copied from one document to 
	 * another, recognizing that the two documents may have different DTDs 
	 * in the XML case. The following list describes the specifics for each 
	 * type of node. 
	 * <dl>
	 * <dt>ATTRIBUTE_NODE</dt>
	 * <dd>The <code>ownerElement</code> attribute 
	 * is set to <code>null</code> and the <code>specified</code> flag is 
	 * set to <code>true</code> on the generated <code>Attr</code>. The 
	 * descendants of the source <code>Attr</code> are recursively imported 
	 * and the resulting nodes reassembled to form the corresponding subtree.
	 * Note that the <code>deep</code> parameter has no effect on 
	 * <code>Attr</code> nodes; they always carry their children with them 
	 * when imported.</dd>
	 * <dt>DOCUMENT_FRAGMENT_NODE</dt>
	 * <dd>If the <code>deep</code> option 
	 * was set to <code>true</code>, the descendants of the source element 
	 * are recursively imported and the resulting nodes reassembled to form 
	 * the corresponding subtree. Otherwise, this simply generates an empty 
	 * <code>DocumentFragment</code>.</dd>
	 * <dt>DOCUMENT_NODE</dt>
	 * <dd><code>Document</code> 
	 * nodes cannot be imported.</dd>
	 * <dt>DOCUMENT_TYPE_NODE</dt>
	 * <dd><code>DocumentType</code> 
	 * nodes cannot be imported.</dd>
	 * <dt>ELEMENT_NODE</dt>
	 * <dd>Specified attribute nodes of the 
	 * source element are imported, and the generated <code>Attr</code> 
	 * nodes are attached to the generated <code>Element</code>. Default 
	 * attributes are not copied, though if the document being imported into 
	 * defines default attributes for this element name, those are assigned. 
	 * If the <code>importNode</code> <code>deep</code> parameter was set to 
	 * <code>true</code>, the descendants of the source element are 
	 * recursively imported and the resulting nodes reassembled to form the 
	 * corresponding subtree.</dd>
	 * <dt>ENTITY_NODE</dt>
	 * <dd><code>Entity</code> nodes can be 
	 * imported, however in the current release of the DOM the 
	 * <code>DocumentType</code> is readonly. Ability to add these imported 
	 * nodes to a <code>DocumentType</code> will be considered for addition 
	 * to a future release of the DOM.On import, the <code>publicId</code>, 
	 * <code>systemId</code>, and <code>notationName</code> attributes are 
	 * copied. If a <code>deep</code> import is requested, the descendants 
	 * of the the source <code>Entity</code> are recursively imported and 
	 * the resulting nodes reassembled to form the corresponding subtree.</dd>
	 * <dt>
	 * ENTITY_REFERENCE_NODE</dt>
	 * <dd>Only the <code>EntityReference</code> itself is 
	 * copied, even if a <code>deep</code> import is requested, since the 
	 * source and destination documents might have defined the entity 
	 * differently. If the document being imported into provides a 
	 * definition for this entity name, its value is assigned.</dd>
	 * <dt>NOTATION_NODE</dt>
	 * <dd>
	 * <code>Notation</code> nodes can be imported, however in the current 
	 * release of the DOM the <code>DocumentType</code> is readonly. Ability 
	 * to add these imported nodes to a <code>DocumentType</code> will be 
	 * considered for addition to a future release of the DOM.On import, the 
	 * <code>publicId</code> and <code>systemId</code> attributes are copied.
	 * Note that the <code>deep</code> parameter has no effect on 
	 * <code>Notation</code> nodes since they never have any children.</dd>
	 * <dt>
	 * PROCESSING_INSTRUCTION_NODE</dt>
	 * <dd>The imported node copies its 
	 * <code>target</code> and <code>data</code> values from those of the 
	 * source node.</dd>
	 * <dt>TEXT_NODE, CDATA_SECTION_NODE, COMMENT_NODE</dt>
	 * <dd>These three 
	 * types of nodes inheriting from <code>CharacterData</code> copy their 
	 * <code>data</code> and <code>length</code> attributes from those of 
	 * the source node.</dd>
	 *  
	 * @param importedNode The node to import.
	 * @param deep If <code>true</code>, recursively import the subtree under 
	 *   the specified node; if <code>false</code>, import only the node 
	 *   itself, as explained above. This has no effect on <code>Attr</code>
	 *   , <code>EntityReference</code>, and <code>Notation</code> nodes.
	 * @return The imported node that belongs to this <code>Document</code>.
	 * @exception DOMException
	 *   NOT_SUPPORTED_ERR: Raised if the type of node being imported is not 
	 *   supported.
	 *
	 * @since DOM Level 2
	 */
	public Node importNode(Node importedNode, boolean deep)
		throws DOMException
	{
		switch(importedNode.getNodeType()) {
		case DOCUMENT_NODE:
		case DOCUMENT_TYPE_NODE:
			throw new DOMException(DOMException.NOT_SUPPORTED_ERR, "");

		default:
		}
		XNode node = _clone(importedNode, deep);
		node._setDoc(this);
		((XNode)importedNode)._notifyUDH(UserDataHandler.NODE_IMPORTED, node);
		return node;
	}

	/**
	 *  Attempts to adopt a node from another document to this document. If 
	 * supported, it changes the <code>ownerDocument</code> of the source 
	 * node, its children, as well as the attached attribute nodes if there 
	 * are any. If the source node has a parent it is first removed from the 
	 * child list of its parent. This effectively allows moving a subtree 
	 * from one document to another. Note that if the adopted node is already part of this 
	 * document (i.e. the source and target document are the same), this 
	 * method still has the effect of removing the source node from the 
	 * child list of its parent, if any. The following list describes the 
	 * specifics for each type of node. 
	 * <dl>
	 * <dt>ATTRIBUTE_NODE</dt>
	 * <dd>The 
	 * <code>ownerElement</code> attribute is set to <code>null</code>. The 
	 * descendants of the source 
	 * <code>Attr</code> are recursively adopted.</dd>
	 * <dt>DOCUMENT_FRAGMENT_NODE</dt>
	 * <dd>The 
	 * descendants of the source node are recursively adopted.</dd>
	 * <dt>DOCUMENT_NODE</dt>
	 * <dd>
	 * <code>Document</code> nodes cannot be adopted.</dd>
	 * <dt>DOCUMENT_TYPE_NODE</dt>
	 * <dd>
	 * <code>DocumentType</code> nodes cannot be adopted.</dd>
	 * <dt>ELEMENT_NODE</dt>
	 * <dd>The 
	 * descendants of the source element are recursively adopted.</dd>
	 * <dt>ENTITY_NODE</dt>
	 * <dd>
	 * <code>Entity</code> nodes cannot be adopted.</dd>
	 * <dt>ENTITY_REFERENCE_NODE</dt>
	 * <dd>Only 
	 * the <code>EntityReference</code> node itself is adopted, the 
	 * descendants are discarded, since the source and destination documents 
	 * might have defined the entity differently. If the document being 
	 * imported into provides a definition for this entity name, its value 
	 * is assigned.</dd>
	 * <dt>NOTATION_NODE</dt>
	 * <dd><code>Notation</code> nodes cannot be 
	 * adopted.</dd>
	 * <dt>PROCESSING_INSTRUCTION_NODE, TEXT_NODE, CDATA_SECTION_NODE, 
	 * COMMENT_NODE</dt>
	 * <dd>These nodes can all be adopted. No specifics.</dd>
	 * </dl> 
	 *
	 * @param source The node to move into this document.
	 * @return The adopted node, or <code>null</code> if this operation 
	 *   fails, such as when the source node comes from a different 
	 *   implementation.
	 * @exception DOMException
	 *   NOT_SUPPORTED_ERR: Raised if the source node is of type 
	 *   <code>DOCUMENT</code>, <code>DOCUMENT_TYPE</code>, 
	 *   <code>ENTITY_NODE</code>, <code>NOTATION_NODE</code>.
	 *   <br>NO_MODIFICATION_ALLOWED_ERR: Raised when the source node is 
	 *   readonly.
	 *
	 * @since DOM Level 3
	 */
	public Node adoptNode(Node source)
		throws DOMException
	{
		switch(source.getNodeType()) {
		case DOCUMENT_NODE:
		case DOCUMENT_TYPE_NODE:
		case ENTITY_NODE:
		case NOTATION_NODE:
			throw new DOMException(DOMException.NOT_SUPPORTED_ERR, "");

		default:
		}
		XNode src = (XNode)source;
		if (src._isRO())
			throw new DOMException(DOMException.NO_MODIFICATION_ALLOWED_ERR, "");
		//		Detach source form its parent
		if (src._getParent() != null) {
			if (src.getNodeType() == ATTRIBUTE_NODE) {
				Element elm = (Element)src._getParent();
				src = (XNode)elm.removeAttributeNode((Attr)src);
			} else
				src = (XNode)src._getParent().removeChild(src);
		}
		if (src.getOwnerDocument() == this)
			return src;
		src._setDoc(this);
		src._notifyUDH(UserDataHandler.NODE_ADOPTED, src);
		return src;
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

		XNode child = (XNode)newChild;

		switch(child.getNodeType()) {
		case PROCESSING_INSTRUCTION_NODE:
		case COMMENT_NODE:
			break;

		case DOCUMENT_TYPE_NODE:
			if (child._getDoc() != null && child._getDoc() != this)
				throw new DOMException(DOMException.WRONG_DOCUMENT_ERR, "");
			if (type != null)
				throw new DOMException(DOMException.HIERARCHY_REQUEST_ERR, "");
			break;

		case ELEMENT_NODE:
			if ((getDocumentElement() != null) && (replaceChild == false))
				throw new DOMException(DOMException.HIERARCHY_REQUEST_ERR, "");
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
		switch(child.getNodeType()) {
		case DOCUMENT_TYPE_NODE:
			type = (DocumentType)child;
			break;

		case ELEMENT_NODE:
			root = (Element)child;
			break;

		default:
			break;
		}

		super._childAdded(child);
	}

	/**
	 * Notification of child removal.
	 */
	protected void _childRemoving(XNode child)
	{
		super._childRemoving(child);

		switch(child.getNodeType()) {
		case DOCUMENT_TYPE_NODE:
			child._setDoc(null);
			type = null;
			break;

		case ELEMENT_NODE:
			root = null;
			break;

		default:
			break;
		}
	}

	/** 
	 * Returns default attributes for specified element or <code>null</code>. 
	 * Four strings represent single attribute: 0 - qname, 1 - localname, 
	 * 2 - namespace, 3 - value. Therefore, the array size divided by four is 
	 * the number of default attributes.
	 */
	/* pkg */ final String[] _getDefAttrs(String qname)
	{
		if (attrs == null || qname == null)
			return null;

		return (String[])attrs.get(qname);
	}

	/** 
	 * Adds a default attribute to the document.
	 */
	/* pkg */ final void _setDefAttr(String tag, String qname, String value)
	{
		qname = _intern(qname);
		String   lname = _intern(XNode._getLocalName(qname));
		String[] defs  = (String[])attrs.get(tag);
		if (defs == null) {
			//		No default attributes on this element
			defs = new String[1 << 2];
			defs[0] = qname;
			defs[1] = lname;
			defs[2] = null;
			defs[3] = value;
		} else {
			//		Add one more default attribute to this element
			String[] list = new String[defs.length + (1 << 2)];
			System.arraycopy(defs, 0, list, 0, defs.length);
			list[defs.length + 0] = qname;
			list[defs.length + 1] = lname;
			list[defs.length + 2] = null;
			list[defs.length + 3] = value;
			defs = list;
		}
		attrs.put(tag, defs);
	}

	/**
	 * Insures the document has only one instance of string with content equal 
	 * to string provided as an argument.
	 */
	protected final String _intern(String str)
	{
		if (str == null)
			return null;

		String name = (String)names.get(str);
		if (name == null) {
			names.put(str, str);
			name = str;
		}

		return name;
	}

	/**
	 * Returns clone of the node or <code>null</code>.
	 */
	protected XNode _clone(Node node, boolean deep)
	{
		switch(node.getNodeType()) {
		case DOCUMENT_FRAGMENT_NODE:
			return new DocFragImp((DocFragImp)node, deep);

		case ELEMENT_NODE:
			return new ElmImp((ElmImp)node, deep);

		case ATTRIBUTE_NODE:
			return new AttrImp((AttrImp)node, deep);

		case TEXT_NODE:
		case CDATA_SECTION_NODE:
			return new TextImp((TextImp)node, deep);

		case PROCESSING_INSTRUCTION_NODE:
			return new ProcInstImp((ProcInstImp)node, deep);

		case COMMENT_NODE:
			return new CommImp((CommImp)node, deep);

		case ENTITY_REFERENCE_NODE:
			return new EntRefImp((EntRefImp)node, deep);

		case ENTITY_NODE:
			return new EntImp((EntImp)node, deep);

		case NOTATION_NODE:
			return new NotImp((NotImp)node, deep);

		case DOCUMENT_TYPE_NODE:
			//		See method cloneNode of DocTypeImp class.
		case DOCUMENT_NODE:
			//		See method cloneNode of this class.
		default:
		}
		return null;
	}

	/**
	 * Returns list of supported features.
	 */
	protected abstract String[] _getFeatures();

	/**
	 * Checks is feature supported.
	 */
	protected boolean _isSupported(String feature, String version)
	{
		String name = (feature.charAt(0) == '+')? feature.substring(1): feature;
		String ver  = ("".equals(version))? null: version;
		String[] features = _getDoc()._getFeatures();
		for (int idx = 0; idx < features.length; idx++) {
			if (features[idx].equalsIgnoreCase(name)) {
				if (ver == null)
					return true;
				if ("1.0".equals(ver) || "2.0".equals(ver))
					return true;
			}
		}
		return false;
	}

	/**
	 * Sets/resets build mode flag. This method reduces amount of work during 
	 * document construction by a DOM parser.
	 */
	public void _setBuild(boolean build)
	{
		if (build == false)
			flags &= ~FLAG_BUILD;
		else
			flags |= FLAG_BUILD;
	}

	/**
	 * Returns string representation of the element.
	 */
	public String toString()
	{
		StringBuffer out = new StringBuffer();

		if (type != null)
			out.append(type.toString());
		for (int i = 0; i < getLength(); i++) {
			if (item(i).getNodeType() != DOCUMENT_TYPE_NODE)
				out.append(item(i).toString());
		}

		return out.toString();
	}
}
