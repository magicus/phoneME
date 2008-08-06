/*
 * Portions Copyright  2000-2008 Sun Microsystems, Inc. All Rights
 * Reserved.  Use is subject to license terms.
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

/*
 * Copyright (c) 2004 World Wide Web Consortium,
 *
 * (Massachusetts Institute of Technology, European Research Consortium for
 * Informatics and Mathematics, Keio University). All Rights Reserved. This
 * work is distributed under the W3C(r) Software License [1] in the hope that
 * it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * [1] http://www.w3.org/Consortium/Legal/2002/copyright-software-20021231
 */

package org.w3c.dom;

/**
 * The <code>Document</code> interface represents the entire HTML or XML 
 * document. Conceptually, it is the root of the document tree, and provides 
 * the primary access to the document's data.
 * <p>Since elements, text nodes, comments, processing instructions, etc. 
 * cannot exist outside the context of a <code>Document</code>, the 
 * <code>Document</code> interface also contains the factory methods needed 
 * to create these objects. The <code>Node</code> objects created have a 
 * <code>ownerDocument</code> attribute which associates them with the 
 * <code>Document</code> within whose context they were created.
 * <p>See also the <a href='http://www.w3.org/TR/2000/REC-DOM-Level-2-Core-20001113'>Document Object Model (DOM) Level 2 Core Specification</a>
 * and the <a href='http://www.w3.org/TR/2004/REC-DOM-Level-3-Core-20040407'>Document Object Model (DOM) Level 3 Core Specification</a>.
 */
public interface Document extends Node {
    /**
     * The Document Type Declaration (see <code>DocumentType</code>) 
     * associated with this document. For XML documents without a document
     * type declaration this returns <code>null</code>. For HTML documents,
     * a <code>DocumentType</code> object may be returned, independently of
     * the presence or absence of document type declaration in the HTML
     * document.
     * <br>This provides direct access to the <code>DocumentType</code> node, 
     * child node of this <code>Document</code>. This node can be set at 
     * document creation time and later changed through the use of child 
     * nodes manipulation methods, such as <code>Node.insertBefore</code>, 
     * or <code>Node.replaceChild</code>. Note, however, that while some 
     * implementations may instantiate different types of 
     * <code>Document</code> objects supporting additional features than the 
     * "Core", such as "HTML" [<a href='http://www.w3.org/TR/2003/REC-DOM-Level-2-HTML-20030109'>DOM Level 2 HTML</a>]
     * , based on the <code>DocumentType</code> specified at creation time, 
     * changing it afterwards is very unlikely to result in a change of the 
     * features supported.
     * @since DOM Level 3
     * @return the Document Type Declaration associated with this document, or <code>null</code>
     */
    public DocumentType getDoctype();

    /**
     * The <code>DOMImplementation</code> object that handles this document. A 
     * DOM application may use objects from multiple implementations.
     * @return the <code>DOMImplementation</code> object that handles this document
     */
    public DOMImplementation getImplementation();

    /**
     * This is a convenience attribute that allows direct access to the child 
     * node that is the root element of the document. 
     * @return the child node that is the root element of the document
     */
    public Element getDocumentElement();

    /**
     * Creates an element of the type specified. Note that the instance 
     * returned implements the <code>Element</code> interface, so attributes 
     * can be specified directly on the returned object.
     * <br>In addition, if there are known attributes with default values, 
     * <code>Attr</code> nodes representing them are automatically created 
     * and attached to the element.
     * <br>To create an element with a qualified name and namespace URI, use 
     * the <code>createElementNS</code> method.
     * @param tagName The name of the element type to instantiate. For XML, 
     *   this is case-sensitive. otherwise it depends on the 
     *   case-sensitivity of the markup language in use. In that case, the 
     *   name is mapped to the canonical form of that markup by the DOM 
     *   implementation.
     * @return A new <code>Element</code> object with the 
     *   <code>nodeName</code> attribute set to <code>tagName</code>, and 
     *   <code>localName</code>, <code>prefix</code>, and 
     *   <code>namespaceURI</code> set to <code>null</code>.
     * @exception DOMException
     *   INVALID_CHARACTER_ERR: Raised if the specified name is not an XML 
     *   name according to the XML version in use (1.0 for JSR 280).
     */
    public Element createElement(String tagName)
                                 throws DOMException;

    /**
     * Creates an empty <code>DocumentFragment</code> object. 
     * @return A new <code>DocumentFragment</code>.
     */
    public DocumentFragment createDocumentFragment();

    /**
     * Creates a <code>Text</code> node given the specified string.
     * @param data The data for the node.
     * @return The new <code>Text</code> object.
     */
    public Text createTextNode(String data);

    /**
     * Creates a <code>Comment</code> node given the specified string.
     * @param data The data for the node.
     * @return The new <code>Comment</code> object.
     */
    public Comment createComment(String data);

    /**
     * Creates a <code>CDATASection</code> node whose value is the specified 
     * string.
     * @param data The data for the <code>CDATASection</code> contents.
     * @return The new <code>CDATASection</code> object.
     * @exception DOMException
     *   NOT_SUPPORTED_ERR: Raised if this document is an HTML document.
     */
    public CDATASection createCDATASection(String data)
                                           throws DOMException;

    /**
     * Creates a <code>ProcessingInstruction</code> node given the specified 
     * name and data strings.
     * @param target The target part of the processing instruction. Unlike 
     *   <code>Document.createElementNS</code> or 
     *   <code>Document.createAttributeNS</code>, no namespace well-formed 
     *   checking is done on the target name.
     * @param data The data for the node.
     * @return The new <code>ProcessingInstruction</code> object.
     * @exception DOMException
     *   INVALID_CHARACTER_ERR: Raised if the specified target is not an XML 
     *   name according to the XML version in use (1.0 for JSR 280).
     *   <br>NOT_SUPPORTED_ERR: Raised if this document is an HTML document.
     */
    public ProcessingInstruction createProcessingInstruction(String target, 
                                                             String data)
                                                             throws DOMException;

    /**
     * Creates an <code>Attr</code> of the given name. Note that the 
     * <code>Attr</code> instance can then be set on an <code>Element</code> 
     * using the <code>setAttributeNode</code> method. 
     * <br>To create an attribute with a qualified name and namespace URI, use 
     * the <code>createAttributeNS</code> method.
     * @param name The name of the attribute.
     * @return A new <code>Attr</code> object with the <code>nodeName</code> 
     *   attribute set to <code>name</code>, and <code>localName</code>, 
     *   <code>prefix</code>, and <code>namespaceURI</code> set to 
     *   <code>null</code>. The value of the attribute is the empty string.
     * @exception DOMException
     *   INVALID_CHARACTER_ERR: Raised if the specified name is not an XML 
     *   name according to the XML version in use (1.0 for JSR 280).
     */
    public Attr createAttribute(String name)
                                throws DOMException;

    /**
     * Creates an <code>EntityReference</code> object. In addition, if the 
     * referenced entity is known, the child list of the 
     * <code>EntityReference</code> node is made the same as that of the 
     * corresponding <code>Entity</code> node. 
     * <p ><b>Note:</b> If any descendant of the <code>Entity</code> node has 
     * an unbound namespace prefix, the corresponding descendant of the 
     * created <code>EntityReference</code> node is also unbound; (its 
     * <code>namespaceURI</code> is <code>null</code>). The DOM Level 2 and 
     * 3 do not support any mechanism to resolve namespace prefixes in this 
     * case.
     * @param name The name of the entity to reference. Unlike 
     *   <code>Document.createElementNS</code> or 
     *   <code>Document.createAttributeNS</code>, no namespace well-formed 
     *   checking is done on the entity name.
     * @return The new <code>EntityReference</code> object.
     * @exception DOMException
     *   INVALID_CHARACTER_ERR: Raised if the specified name is not an XML 
     *   name according to the XML version in use (1.0 for JSR 280).
     *   <br>NOT_SUPPORTED_ERR: Raised if this document is an HTML document.
     */
    public EntityReference createEntityReference(String name)
                                                 throws DOMException;

    /**
     * Returns a <code>NodeList</code> of all the <code>Elements</code> with a 
     * given tag name in the order in which they are encountered in a 
     * preorder traversal of the <code>Document</code> tree. 
     * @param tagname The name of the tag to match on. The special value "*" 
     *   matches all tags. For XML, the <code>tagname</code> parameter is 
     *   case-sensitive, otherwise it depends on the case-sensitivity of the 
     *   markup language in use.
     * @return A new <code>NodeList</code> object containing all the matched 
     *   <code>Elements</code>.
     */
    public NodeList getElementsByTagName(String tagname);

    /**
     * Imports a node from another document to this document, without altering 
     * or removing the source node from the original document; this method 
     * creates a new copy of the source node. The returned node has no 
     * parent; (<code>parentNode</code> is <code>null</code>).
     * <br>For all nodes, importing a node creates a node object owned by the 
     * importing document, with attribute values identical to the source 
     * node's <code>nodeName</code> and <code>nodeType</code>, plus the 
     * attributes related to namespaces (<code>prefix</code>, 
     * <code>localName</code>, and <code>namespaceURI</code>). As in the 
     * <code>cloneNode</code> operation, the source node is not altered.
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
     * was set to <code>true</code>, the descendants of the source  
     * <code>DocumentFragment</code> are recursively imported and the 
     * resulting nodes reassembled under the imported 
     * <code>DocumentFragment</code> to form the corresponding subtree. 
     * Otherwise, this simply generates an empty 
     * <code>DocumentFragment</code>.</dd>
     * <dt>DOCUMENT_NODE</dt>
     * <dd><code>Document</code> 
     * nodes cannot be imported.</dd>
     * <dt>DOCUMENT_TYPE_NODE</dt>
     * <dd><code>DocumentType</code> 
     * nodes cannot be imported.</dd>
     * <dt>ELEMENT_NODE</dt>
     * <dd><em>Specified</em> attribute nodes of the source element are imported, and the generated 
     * <code>Attr</code> nodes are attached to the generated 
     * <code>Element</code>. Default attributes are <em>not</em> copied, though if the document being imported into defines default 
     * attributes for this element name, those are assigned. If the 
     * <code>importNode</code> <code>deep</code> parameter was set to 
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
     * Note that the <code>deep</code> parameter has no effect on this type
     * of nodes since they cannot have any children.</dd>
     * <dt>
     * PROCESSING_INSTRUCTION_NODE</dt>
     * <dd>The imported node copies its 
     * <code>target</code> and <code>data</code> values from those of the 
     * source node. Note that the <code>deep</code> parameter has no effect 
     * on this type of nodes since they cannot have any children.</dd>
     * <dt>TEXT_NODE, CDATA_SECTION_NODE, COMMENT_NODE</dt>
     * <dd>These three 
     * types of nodes inheriting from <code>CharacterData</code> copy their 
     * <code>data</code> and <code>length</code> attributes from those of 
     * the source node.Note that the <code>deep</code> parameter has no effect 
     * on this type of nodes since they cannot have any children.</dd>
     * </dl> 
     * @param importedNode The node to import.
     * @param deep If <code>true</code>, recursively import the subtree under 
     *   the specified node; if <code>false</code>, import only the node 
     *   itself, as explained above. This has no effect on nodes that cannot 
     *   have any children, and on <code>Attr</code>, and 
     *   <code>EntityReference</code> nodes.
     * @return The imported node that belongs to this <code>Document</code>.
     * @exception DOMException
     *   NOT_SUPPORTED_ERR: Raised if the type of node being imported is not 
     *   supported.
     *   <br>INVALID_CHARACTER_ERR: Raised if one of the imported names is not 
     *   an XML name according to the XML version in use (1.0 for JSR 280).
     *   This may happen when importing an XML 1.1 
     *   [<a href='http://www.w3.org/TR/2004/REC-xml11-20040204/'>XML 1.1</a>]
     *   element into an XML 1.0 document, for instance.
     * @since DOM Level 2
     */
    public Node importNode(Node importedNode, boolean deep)
                                                 throws DOMException;

    /**
     * Creates an element of the given qualified name and namespace URI. 
     * <br>Per [<a href='http://www.w3.org/TR/1999/REC-xml-names-19990114/'>XML Namespaces</a>]
     * , applications must use the value <code>null</code> as the 
     * namespaceURI parameter for methods if they wish to have no namespace.
     * @param namespaceURI The namespace URI of the element to create.
     * @param qualifiedName The qualified name of the element type to 
     *   instantiate.
     * @return A new <code>Element</code> object with the following 
     *   attributes:
     * <table border='1' cellpadding='3'>
     * <tr>
     * <th>Attribute</th>
     * <th>Value</th>
     * </tr>
     * <tr>
     * <td valign='top' rowspan='1' colspan='1'><code>Node.nodeName</code></td>
     * <td valign='top' rowspan='1' colspan='1'>
     *   <code>qualifiedName</code></td>
     * </tr>
     * <tr>
     * <td valign='top' rowspan='1' colspan='1'><code>Node.namespaceURI</code></td>
     * <td valign='top' rowspan='1' colspan='1'>
     *   <code>namespaceURI</code></td>
     * </tr>
     * <tr>
     * <td valign='top' rowspan='1' colspan='1'><code>Node.prefix</code></td>
     * <td valign='top' rowspan='1' colspan='1'>prefix, extracted 
     *   from <code>qualifiedName</code>, or <code>null</code> if there is 
     *   no prefix</td>
     * </tr>
     * <tr>
     * <td valign='top' rowspan='1' colspan='1'><code>Node.localName</code></td>
     * <td valign='top' rowspan='1' colspan='1'>local name, extracted from 
     *   <code>qualifiedName</code></td>
     * </tr>
     * <tr>
     * <td valign='top' rowspan='1' colspan='1'><code>Element.tagName</code></td>
     * <td valign='top' rowspan='1' colspan='1'>
     *   <code>qualifiedName</code></td>
     * </tr>
     * </table>
     * @exception DOMException
     *   INVALID_CHARACTER_ERR: Raised if the specified 
     *   <code>qualifiedName</code> is not an XML name according to the XML 
     *   version in use (1.0 for JSR 280).
     *   <br>NAMESPACE_ERR: Raised if the <code>qualifiedName</code> is a
     *   malformed qualified name, if the <code>qualifiedName</code> has a 
     *   prefix and the <code>namespaceURI</code> is <code>null</code>, or 
     *   if the <code>qualifiedName</code> has a prefix that is "xml" and 
     *   the <code>namespaceURI</code> is different from "<a href='http://www.w3.org/XML/1998/namespace'>
     *   http://www.w3.org/XML/1998/namespace</a>" [<a href='http://www.w3.org/TR/1999/REC-xml-names-19990114/'>XML Namespaces</a>]
     *   , or if the <code>qualifiedName</code> or its prefix is "xmlns" and 
     *   the <code>namespaceURI</code> is different from "<a href='http://www.w3.org/2000/xmlns/'>http://www.w3.org/2000/xmlns/</a>", or if the <code>namespaceURI</code> is "<a href='http://www.w3.org/2000/xmlns/'>http://www.w3.org/2000/xmlns/</a>" and neither the <code>qualifiedName</code> nor its prefix is "xmlns".
     *   <br>NOT_SUPPORTED_ERR: Always thrown if the current document does not 
     *   support the <code>"XML"</code> feature, since namespaces were 
     *   defined by XML.
     * @since DOM Level 2
     */
    public Element createElementNS(String namespaceURI, 
                                   String qualifiedName)
                                   throws DOMException;

    /**
     * Creates an attribute of the given qualified name and namespace URI. 
     * <br>Per [<a href='http://www.w3.org/TR/1999/REC-xml-names-19990114/'>XML Namespaces</a>]
     * , applications must use the value <code>null</code> as the 
     * <code>namespaceURI</code> parameter for methods if they wish to have 
     * no namespace.
     * @param namespaceURI The namespace URI of the attribute to create.
     * @param qualifiedName The qualified name of the attribute to instantiate.
     * @return A new <code>Attr</code> object with the following attributes:
     * <table border='1' cellpadding='3'>
     * <tr>
     * <th>
     *   Attribute</th>
     * <th>Value</th>
     * </tr>
     * <tr>
     * <td valign='top' rowspan='1' colspan='1'><code>Node.nodeName</code></td>
     * <td valign='top' rowspan='1' colspan='1'>qualifiedName</td>
     * </tr>
     * <tr>
     * <td valign='top' rowspan='1' colspan='1'>
     *   <code>Node.namespaceURI</code></td>
     * <td valign='top' rowspan='1' colspan='1'><code>namespaceURI</code></td>
     * </tr>
     * <tr>
     * <td valign='top' rowspan='1' colspan='1'>
     *   <code>Node.prefix</code></td>
     * <td valign='top' rowspan='1' colspan='1'>prefix, extracted from 
     *   <code>qualifiedName</code>, or <code>null</code> if there is no 
     *   prefix</td>
     * </tr>
     * <tr>
     * <td valign='top' rowspan='1' colspan='1'><code>Node.localName</code></td>
     * <td valign='top' rowspan='1' colspan='1'>local name, extracted from 
     *   <code>qualifiedName</code></td>
     * </tr>
     * <tr>
     * <td valign='top' rowspan='1' colspan='1'><code>Attr.name</code></td>
     * <td valign='top' rowspan='1' colspan='1'>
     *   <code>qualifiedName</code></td>
     * </tr>
     * <tr>
     * <td valign='top' rowspan='1' colspan='1'><code>Node.nodeValue</code></td>
     * <td valign='top' rowspan='1' colspan='1'>the empty 
     *   string</td>
     * </tr>
     * </table>
     * @exception DOMException
     *   INVALID_CHARACTER_ERR: Raised if the specified 
     *   <code>qualifiedName</code> is not an XML name according to the XML 
     *   version in use (1.0 for JSR 280).
     *   <br>NAMESPACE_ERR: Raised if the <code>qualifiedName</code> is a
     *   malformed qualified name, if the <code>qualifiedName</code> has a 
     *   prefix and the <code>namespaceURI</code> is <code>null</code>, if 
     *   the <code>qualifiedName</code> has a prefix that is "xml" and the 
     *   <code>namespaceURI</code> is different from "<a href='http://www.w3.org/XML/1998/namespace'>
     *   http://www.w3.org/XML/1998/namespace</a>", if the <code>qualifiedName</code> or its prefix is "xmlns" and the 
     *   <code>namespaceURI</code> is different from "<a href='http://www.w3.org/2000/xmlns/'>http://www.w3.org/2000/xmlns/</a>", or if the <code>namespaceURI</code> is "<a href='http://www.w3.org/2000/xmlns/'>http://www.w3.org/2000/xmlns/</a>" and neither the <code>qualifiedName</code> nor its prefix is "xmlns".
     *   <br>NOT_SUPPORTED_ERR: Always thrown if the current document does not 
     *   support the <code>"XML"</code> feature, since namespaces were 
     *   defined by XML.
     * @since DOM Level 2
     */
    public Attr createAttributeNS(String namespaceURI, 
                                  String qualifiedName)
                                  throws DOMException;

    /**
     * Returns a <code>NodeList</code> of all the <code>Elements</code> with a 
     * given local name and namespace URI in document order.
     * @param namespaceURI The namespace URI of the elements to match on. The 
     *   special value <code>"*"</code> matches all namespaces.
     * @param localName The local name of the elements to match on. The 
     *   special value <code>"*"</code> matches all local names.
     * @return A new <code>NodeList</code> object containing all the matched 
     *   <code>Elements</code>.
     * @since DOM Level 2
     */
    public NodeList getElementsByTagNameNS(String namespaceURI, 
                                           String localName);

    /**
     * Returns the <code>Element</code>that has an ID attribute with the 
     *given value. If no such element exists, this returns <code>null</code>.
     * If more than one element has an ID attribute with that value, what 
     * is returned is undefined.
     * <br>The DOM implementation is expected to use the attribute 
     * <code>Attr.isId</code> to determine if an attribute is of type ID. 
     * <p ><b>Note:</b> Attributes with the name "ID" or "id" are not of type 
     * ID unless so defined. 
     * Implementations that do not know whether attributes are of type ID or 
     * not are expected to return <code>null</code>.
     * @param elementId The unique <code>id</code> value for an element.
     * @return The matching element or <code>null</code> if there is none.
     * @since DOM Level 2
     */
    public Element getElementById(String elementId);

    /**
     *  Attempts to adopt a node from another document to this document. If 
     * supported, it changes the <code>ownerDocument</code> of the source 
     * node, its children, as well as the attached attribute nodes if there 
     * are any. If the source node has a parent it is first removed from the 
     * child list of its parent. This effectively allows moving a subtree 
     * from one document to another (unlike <code>importNode()</code> which 
     * create a copy of the source node instead of moving it). When it 
     * fails, applications should use <code>Document.importNode()</code> 
     * instead. Note that if the adopted node is already part of this 
     * document (i.e. the source and target document are the same), this 
     * method still has the effect of removing the source node from the 
     * child list of its parent, if any. The following list describes the 
     * specifics for each type of node. 
     * <dl>
     * <dt>ATTRIBUTE_NODE</dt>
     * <dd>The 
     * <code>ownerElement</code> attribute is set to <code>null</code>> and 
     * the <code>specified</code> flag is set to <code>true</code> on the 
     * adopted <code>Attr</code>. The descendants of the source 
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
     * <dd><em>Specified</em> attribute nodes of the source element are adopted. Default attributes 
     * are discarded, though if the document being adopted into defines 
     * default attributes for this element name, those are assigned. The 
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
     * <p ><b>Note:</b>  Since it does not create new nodes unlike the 
     * <code>Document.importNode()</code> method, this method does not raise 
     * an <code>INVALID_CHARACTER_ERR</code> exception.
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
     * @since DOM Level 3
     */
    public Node adoptNode(Node source)
                          throws DOMException;
}
