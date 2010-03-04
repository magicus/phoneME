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

import java.io.CharArrayWriter;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Vector;

import org.xml.sax.Attributes;
import org.xml.sax.SAXException;
import org.xml.sax.helpers.DefaultHandler;

class DocumentElement extends DefaultHandler {
    String name = null;
    String nameSpace = null;  // If a Document
    Hashtable attributes = new Hashtable();
    Vector elements = new Vector();
    DocumentElement parent = null;
    boolean isDocument = false;

    // Have we encountered an error?
    boolean fastForward = false;
    int cdepth = 0;

    // Accumulate contents
    private CharArrayWriter contents = new CharArrayWriter();

    DocumentElement(String s, boolean isDoc, DocumentElement parent){
	name = s;
	isDocument = isDoc;
        this.parent = parent;
    }

    String getName() {
	return name;
    }

    void setFastForward(boolean trueFalse) {
        fastForward = trueFalse;
        return;
    }

    void addAttribute(String attrName, String value) {
        attributes.put(attrName, value);
        return;
    }

    String getAttribute(String attrName) {
	return (String)attributes.get(attrName);
    }

    void addElement(DocumentElement e) {
	elements.add(e);
	return;
    }

    String getValue() {
        return contents.toString().trim();
    }

    public String toString() {
        String s = "attributes:\n";
        Enumeration e = attributes.keys();
        while (e.hasMoreElements()) {
            String attrName = (String)e.nextElement();
            s += attrName +  " -> " + (String)attributes.get(attrName) +
              "\n";
        }
        if (!elements.isEmpty()) {
            s += "elements: [ ";
            e = elements.elements();
            while (e.hasMoreElements()) {
                DocumentElement de = (DocumentElement)e.nextElement();
                s += de.toString();
            }
            s += "] ";
        }
        s += "value: " + getValue();
        return "\n[ Document Element:\nname: " + name +
          "\n" + s + " ]";
    }

    public void addCharacters(char[] ch, int start, int length) {
        // accumulate
        contents.write(ch, start, length);
        return;
    }

    public void warning(SAXException se) {
        setFastForward(true);
        //        parent.setFastForward( true );
        se.printStackTrace();
        return;
    }
}

