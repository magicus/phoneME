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
 *  The <code>Attr</code> interface represents an attribute in an 
 * <code>Element</code> object. Typically the allowable values for the 
 * attribute are defined in a document type definition.
 * <p><code>Attr</code> objects inherit the <code>Node</code> interface, but 
 * since they are not actually child nodes of the element they describe, the 
 * DOM does not consider them part of the document tree. Thus, the 
 * <code>Node</code> attributes <code>parentNode</code>, 
 * <code>previousSibling</code>, and <code>nextSibling</code> have a 
 * <code>null</code> value for <code>Attr</code> objects. The DOM takes the 
 * view that attributes are properties of elements rather than having a 
 * separate identity from the elements they are associated with; this should 
 * make it more efficient to implement such features as default attributes 
 * associated with all elements of a given type. Furthermore, 
 * <code>Attr</code> nodes may not be immediate children of a 
 * <code>DocumentFragment</code>. However, they can be associated with 
 * <code>Element</code> nodes contained within a 
 * <code>DocumentFragment</code>. In short, users and implementors of the 
 * DOM need to be aware that <code>Attr</code> nodes have some things in 
 * common with other objects inheriting the <code>Node</code> interface, but 
 * they also are quite distinct.
 * <p> The attribute's effective value is determined as follows: if this 
 * attribute has been explicitly assigned any value, that value is the 
 * attribute's effective value; otherwise, if there is a declaration for 
 * this attribute, and that declaration includes a default value, then that 
 * default value is the attribute's effective value; otherwise, the 
 * attribute does not exist on this element in the structure model until it 
 * has been explicitly added. Note that the <code>Node.nodeValue</code> 
 * attribute on the <code>Attr</code> instance can also be used to retrieve
 * the string version of the attribute's value(s). 
 * <p>In XML, where the value of an attribute can contain entity references, 
 * the child nodes of the <code>Attr</code> node may be either 
 * <code>Text</code> or <code>EntityReference</code> nodes (when these are 
 * in use; see the description of <code>EntityReference</code> for 
 * discussion).
 * <p>The DOM Core represents all attribute values as simple strings, even if 
 * the DTD or schema associated with the document declares them of some 
 * specific type such as tokenized.  
 * <p>See also the <a href='http://www.w3.org/TR/2000/REC-DOM-Level-2-Core-20001113'>Document Object Model (DOM) Level 2 Core Specification</a> and the <a href='http://www.w3.org/TR/2004/REC-DOM-Level-3-Core-20040407'>Document Object Model (DOM) Level 3 Core Specification</a>.
 */
public interface Attr extends Node {
    /**
     * Returns the name of this attribute. If <code>Node.localName</code> is 
     * different from <code>null</code>, this attribute is a qualified name.
     * @return the attribute name
     */
    public String getName();

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
     * @return <code>true</code> if this attribute was explicitly specified, otherwise <code>false</code>
     */
    public boolean getSpecified();

    /**
     * Returns the value of this attribute. 
     * On retrieval, the value of the attribute is returned as a string. 
     * Character and general entity references are replaced with their 
     * values. See also the method <code>getAttribute</code> on the 
     * <code>Element</code> interface.
     * <br>On setting, this creates a <code>Text</code> node with the unparsed 
     * contents of the string. I.e. any characters that an XML processor 
     * would recognize as markup are instead treated as literal text. See 
     * also the method <code>Element.setAttribute</code>.
     * <br> Some specialized implementations, such as some [<a href='http://www.w3.org/TR/2003/REC-SVG11-20030114/'>SVG 1.1</a>] 
     * implementations, may do normalization automatically, even after 
     * mutation; in such case, the value on retrieval may differ from the 
     * value on setting.
     * @return a String containing the value of this attribute
     */
    public String getValue();

    /**
     * Sets the value of this attribute. 
     * On setting, this creates a <code>Text</code> node with the unparsed 
     * contents of the string. I.e. any characters that an XML processor 
     * would recognize as markup are instead treated as literal text. See 
     * also the method <code>Element.setAttribute</code>.
     * <br> Some specialized implementations, such as some [<a href='http://www.w3.org/TR/2003/REC-SVG11-20030114/'>SVG 1.1</a>] 
     * implementations, may do normalization automatically, even after 
     * mutation; in such case, the value on retrieval may differ from the 
     * value on setting.
     * @param value a String containing the value of this attribute
     * @exception DOMException
     *   NO_MODIFICATION_ALLOWED_ERR: Raised when the node is readonly.
     */
    public void setValue(String value)
                            throws DOMException;

    /**
     * The <code>Element</code> node this attribute is attached to or 
     * <code>null</code> if this attribute is not in use.
     * @return the <code>Element</code> node this attribute is attached to, or <code>null</code>
     * @since DOM Level 2
     */
    public Element getOwnerElement();

    /**
     * Returns whether this attribute is known to be of type ID or not. 
     * In other words, whether this attribute 
     * contains an identifier for its owner element or not. When it is and 
     * its value is unique, the <code>ownerElement</code> of this attribute 
     * can be retrieved using the method <code>Document.getElementById</code>.
     * <p>Note: The JSR 280 DOM subset does not support XML schema or
     * <code>Document.normalizeDocument()</code>, and thus supports only 
     * a subset of the DOM 3 mechanisms for identifying ID attributes:
     * <ul>
     * <li> the use of the methods <code>Element.setIdAttribute()</code>, 
     * <code>Element.setIdAttributeNS()</code>, or 
     * <code>Element.setIdAttributeNode()</code>, i.e. it is an 
     * user-determined ID attribute.</li>
     * </ul>
     * @return <code>true</code> if the attribute is of type ID, otherwise <code>false</code>
     * @since DOM Level 3
     */
    public boolean isId();
}
