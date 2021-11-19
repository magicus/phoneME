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
 * The <code>NodeList</code> interface provides the abstraction of an ordered 
 * collection of nodes, without defining or constraining how this collection 
 * is implemented. <code>NodeList</code> objects in the DOM are live.
 * <p>The items in the <code>NodeList</code> are accessible via an integral 
 * index, starting from 0.
 * <p>See also the <a href='http://www.w3.org/TR/2000/REC-DOM-Level-2-Core-20001113'>Document Object Model (DOM) Level 2 Core Specification</a>.
 */
public interface NodeList {
    /**
     * Returns the <code>index</code>th item in the collection. If 
     * <code>index</code> is greater than or equal to the number of nodes in 
     * the list, this returns <code>null</code>. The range of valid child
     *  node indices is 0 to <code>length-1</code> inclusive.
     * @param index Index into the collection.
     * @return The node at the <code>index</code>th position in the 
     *   <code>NodeList</code>, or <code>null</code> if that is not a valid 
     *   index.
     */
    public Node item(int index);

    /**
     * The number of nodes in the list. The range of valid child node indices 
     * is 0 to <code>length-1</code> inclusive. 
     * @return The number of nodes in the list
     */
    public int getLength();

}
