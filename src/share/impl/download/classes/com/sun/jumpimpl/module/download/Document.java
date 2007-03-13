/*
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.jumpimpl.module.download;

/*
 * A Document wrapper to hold parsed XML output. In
 * reality all XML is parsed into DocumentElement instances,
 * including the outermost element which is the document
 * itself. This is to make it easier while doing the parsing.
 * This outer class exists mainly to provide a fairly easy
 * way for someone to iterate through the elements (or nodes)
 * in the parsed document.
 */

import java.io.CharArrayWriter;
import java.util.Iterator;

class Document implements Iterator {

    DocumentElement elementHead = null;
    DocumentElement currentElement = null;
    int currentCount = 0;

    Document(String name, DocumentElement head) {
	if (head == null) {
	    throw new IllegalArgumentException("Document cannot have no elements.");
	}
	currentElement = elementHead = head;
    }

    public String toString() {
        String elementStr = elementHead.toString();
        return "document: " + elementHead.name +
	    elementStr;
    }

    // Iterator methods
    public Iterator getIterator() {
	return this;
    }

    public boolean hasNext() {
	return currentElement != null && 
	     currentCount < currentElement.elements.size();
    }

    public Object next() {
	return currentElement.elements.elementAt(currentCount++);
    }

    public void remove() {
	throw new UnsupportedOperationException();
    }
    // End Iterator implementation
}
