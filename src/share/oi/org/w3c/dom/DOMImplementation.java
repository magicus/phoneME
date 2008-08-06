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
 * Copyright (c) 2000 World Wide Web Consortium,
 * (Massachusetts Institute of Technology, Institut National de
 * Recherche en Informatique et en Automatique, Keio University). All
 * Rights Reserved. This program is distributed under the W3C's Software
 * Intellectual Property License. This program is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.
 * See W3C License http://www.w3.org/Consortium/Legal/ for more details.
 */

package org.w3c.dom;

/**
 * The <code>DOMImplementation</code> interface provides a number of methods 
 * for performing operations that are independent of any particular instance 
 * of the document object model.
 * <p>See also the <a href='http://www.w3.org/TR/2000/REC-DOM-Level-2-Core-20001113'>Document Object Model (DOM) Level 2 Core Specification</a>.
 */
public interface DOMImplementation {
    /**
     * Test if the DOM implementation implements a specific feature.
     * @param feature The name of the feature to test (case-insensitive). The 
     *   values used by DOM features are defined throughout the DOM Level 2 
     *   specifications and listed in the  section. The name must be an XML 
     *   name. To avoid possible conflicts, as a convention, names referring 
     *   to features defined outside the DOM specification should be made 
     *   unique by reversing the name of the Internet domain name of the 
     *   person (or the organization that the person belongs to) who defines 
     *   the feature, component by component, and using this as a prefix. 
     *   For instance, the W3C SVG Working Group defines the feature 
     *   "org.w3c.dom.svg".
     * @param version This is the version number of the feature to test. In 
     *   Level 2, the string can be either "2.0" or "1.0". If the version is 
     *   not specified (parameter is either null or the empty string, as per 
     *   http://www.w3.org/TR/DOM-Level-3-Events/events.html#Conformance), 
     *   supporting any version of the feature causes the 
     *   method to return <code>true</code>.
     * @return <code>true</code> if the feature is implemented in the 
     *   specified version, <code>false</code> otherwise.
     */
    public boolean hasFeature(String feature, 
                              String version);

    /**
     * Creates an empty <code>DocumentType</code> node. Entity declarations 
     * and notations are not made available. Entity reference expansions and 
     * default attribute additions do not occur. It is expected that a 
     * future version of the DOM will provide a way for populating a 
     * <code>DocumentType</code>.
     * @param qualifiedName The qualified name of the document type to be 
     *   created. 
     * @param publicId The external subset public identifier.
     * @param systemId The external subset system identifier.
     * @return A new <code>DocumentType</code> node with 
     *   <code>Node.ownerDocument</code> set to <code>null</code>.
     * @exception DOMException
     *   INVALID_CHARACTER_ERR: Raised if the specified qualified name 
     *   contains an illegal character.
     *   <br>NAMESPACE_ERR: Raised if the <code>qualifiedName</code> is 
     *   malformed.
     * @since DOM Level 2
     */
    public DocumentType createDocumentType(String qualifiedName, 
                                           String publicId, 
                                           String systemId)
                                           throws DOMException;

    /**
     * Creates an XML <code>Document</code> object of the specified type with 
     * its document element. 
     * @param namespaceURI The namespace URI of the document element to create.
     * @param qualifiedName The qualified name of the document element to be 
     *   created.
     * @param doctype The type of document to be created or <code>null</code>.
     *   When <code>doctype</code> is not <code>null</code>, its 
     *   <code>Node.ownerDocument</code> attribute is set to the document 
     *   being created.
     * @return A new <code>Document</code> object.
     * @exception DOMException
     *   INVALID_CHARACTER_ERR: Raised if the specified qualified name 
     *   contains an illegal character.
     *   <br>NAMESPACE_ERR: Raised if the <code>qualifiedName</code> is 
     *   malformed, if the <code>qualifiedName</code> has a prefix and the 
     *   <code>namespaceURI</code> is <code>null</code>, or if the 
     *   <code>qualifiedName</code> has a prefix that is "xml" and the 
     *   <code>namespaceURI</code> is different from "
     *   http://www.w3.org/XML/1998/namespace" .
     *   <br>WRONG_DOCUMENT_ERR: Raised if <code>doctype</code> has already 
     *   been used with a different document or was created from a different 
     *   implementation.
     * @since DOM Level 2
     */
    public Document createDocument(String namespaceURI, 
                                   String qualifiedName, 
                                   DocumentType doctype)
                                   throws DOMException;

    /**
     *  This method returns a specialized object which implements the 
     * specialized APIs of the specified feature and version, as specified 
     * in <a href="http://www.w3.org/TR/DOM-Level-3-Core/core.html#DOMFeatures"
     * >DOM Features</a>. The specialized object may also be obtained by using 
     * binding-specific casting methods but is not necessarily expected to, 
     * as discussed in 
     * <a href="http://www.w3.org/TR/DOM-Level-3-Core/core.html#Embedded-DOM"
     * >Mixed DOM Implementations</a>.
     * This method also allow the implementation to 
     * provide specialized objects which do not support the 
     * <code>DOMImplementation</code> interface. 
     * <p><b>Note:</b> when using the methods that take a feature and a 
     * version as parameters, applications can use null or empty string 
     * for the version parameter if they don't wish to specify a particular
     * version for the specified feature.
     * @param feature  The name of the feature requested. Note that any plus 
     *   sign "+" prepended to the name of the feature will be ignored since 
     *   it is not significant in the context of this method. 
     * @param version  This is the version number of the feature to test. 
     * @return  Returns an object which implements the specialized APIs of 
     *   the specified feature and version, if any, or <code>null</code> if 
     *   there is no object which implements interfaces associated with that 
     *   feature. If the <code>DOMObject</code> returned by this method 
     *   implements the <code>DOMImplementation</code> interface, it must 
     *   delegate to the primary core <code>DOMImplementation</code> and not 
     *   return results inconsistent with the primary core 
     *   <code>DOMImplementation</code> such as <code>hasFeature</code>, 
     *   <code>getFeature</code>, etc. 
     * @since DOM Level 3
     */
    public Object getFeature(String feature, 
                             String version);
}
