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
 * Copyright (c) 2006 World Wide Web Consortium,
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

import org.w3c.dom.Element;

/**
 * <p>The ElementTraversal interface is a set of properties on the
 * <code>Element</code> object, which allow an author to easily navigate 
 * between elements. 
 *
 * <p><b>Note: </b> This interface has not yet been accepted for
 * inclusion in the W3C DOM 3 specification. It may be removed from the
 * JSR 280 specification or added to a different namespace in a
 * subsequent version of the specification.</p> 
 *
 * Four of the properties, <code>firstElementChild</code>,
 * <code>lastElementChild</code>, <code>previousElementSibling</code>, and
 * <code>nextElementSibling</code>, each provide a live reference to another
 * element with the defined relationship to the current element, if the related
 * element exists. The fifth property, <code>childElementCount</code>, exposes
 * the number of child elements of an element, for preprocessing before
 * navigation. A conforming implementation must implement all five methods. An 
 * implementation may implement similar interfaces in other specifications, but
 * such implementation is not required for conformance to this specification, 
 * if the implementation is designed for a minimal code footprint. </p>
 *
 * <p>This interface must be implemented on all elements, regardless of their
 * namespace. For the purpose of ElementTraversal, an entity reference node
 * which represents an element must be treated as an element node. Navigation
 * must be irrespective of namespace, e.g. if an element in the HTML namespace
 * is followed by element in the SVG namespace, <code>nextElementSibling</code>
 * will allow you to navigate from the HTML element to the SVG element. </p>
 *
 */
public interface ElementTraversal 
{
    /**
     * Retrieves the number of child elements.
     *
     * @return the current number of element nodes that are immediate children
     * of this element. <code>0</code> if this element has no child elements.
     */
    public int getChildElementCount();

    /**
     * Retrieves the first child element.
     * 
     * @return the first child element node of this element.
     * <code>null</code> if this element has no child elements.
     */
    public Element getFirstElementChild();

    /**
     * Retrieves the last child element.
     *
     * @return the last child element node of this element.
     * <code>null</code> if this element has no child elements.
     */
    public Element getLastElementChild();

    /**
     * Retrieves the next sibling element.
     * 
     * @return the next sibling element node of this element.
     * <code>null</code> if this element has no element sibling nodes
     * that come after this one in the document tree.
     */
    public Element getNextElementSibling();

    /**
     * Retrieves the previous sibling element.
     * 
     * @return the previous sibling element node of this element.
     * <code>null</code> if this element has no element sibling nodes
     * that come before this one in the document tree.
     */
    public Element getPreviousElementSibling();

}
