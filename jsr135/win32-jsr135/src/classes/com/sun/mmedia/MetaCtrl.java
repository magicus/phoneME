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
package com.sun.mmedia;

import javax.microedition.media.*;
import javax.microedition.media.control.*;
import java.util.Hashtable;
import java.util.Enumeration;

/**
 * This class implements a MetaDataControl and is usable
 * by any class that provides a MetaDataControl.
 * <p>
 * A translation map is used to be able to pass native keys
 * to <code>put()</code>. The key passed to it will be mapped
 * using the translation map.<br>
 * Translation mappings are of type Hashtable and are provided
 * by the owner in the constructor. This enables
 * the use of static translation maps, which is expected
 * for most applications, thus minimizing memory footprint during
 * operation.
 *
 */
public class MetaCtrl implements MetaDataControl {
    private Hashtable data = null;
    private Hashtable map;

    // JAVADOC COMMENT ELIDED
    public MetaCtrl(Hashtable map) {
	this.map = map;
    }

    // JAVADOC COMMENT ELIDED
    public void put(String k, String v) {
	Object o = null;
	if (map!=null) {
	    o=map.get(k);
	}
	if (data==null) {
	    data=new Hashtable();
	}
	if (o!=null) {
	    data.put(o, v);
	    //System.out.println("putting translated Key="+o+"  value="+v);
	} else {
	    data.put(k, v);
	    //System.out.println("Putting Key="+k+"  value="+v);
	}
    }

    // JAVADOC COMMENT ELIDED
    public String[] getKeys() {
	if (data == null) {
	    return new String[0];
	}
	int size = data.size();
	String[] keys = new String[size];
	Enumeration en = data.keys();
	for (int i = 0; i < size; i++) {
	    keys[i] = (String) en.nextElement();
	}
	return keys;
    }

    // JAVADOC COMMENT ELIDED
    public String getKeyValue(String key) {
	if ( (key == null) || (data == null) ||
	     (!(data.containsKey(key))) ) {
	    throw new IllegalArgumentException("invalid key");
	}
	return (String) data.get(key);
    }
 
}
