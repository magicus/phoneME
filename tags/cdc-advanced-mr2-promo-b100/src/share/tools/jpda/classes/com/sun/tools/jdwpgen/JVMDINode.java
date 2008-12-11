/*
 * @(#)JVMDINode.java	1.12 06/10/10
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.tools.jdwpgen;

import java.util.*;
import java.io.*;
import java.text.Collator;

/**
 * Will get converted to Constant nodes during prune.
 */
class JVMDINode extends AbstractNamedNode {
            
    void addConstants(List addons) {
	List addList = new ArrayList();
        int prefixLength = name.length();

	// Place the name map entries that match into addList
        for (Iterator it = Main.nameMap.entrySet().iterator(); it.hasNext();) {
            Map.Entry entry = (Map.Entry)it.next();
            String nm = (String)entry.getKey();
            if (nm.startsWith(name)) {
                NameValueNode nv = (NameValueNode)entry.getValue();
                nv.name = nm.substring(prefixLength);
		addList.add(nv);
	    }
	}

	// Sort JVMDI defs to be added, by numeric value (if possible)
	Collections.sort(addList, new Comparator() {
	    public int compare(Object o1, Object o2) {
		String s1 = ((NameValueNode)o1).val;
		String s2 = ((NameValueNode)o2).val;
		try {
		    return Integer.parseInt(s1) - Integer.parseInt(s2);
		} catch (NumberFormatException exc) {
		    return Collator.getInstance().compare(s1, s2);
		}
	    }
	});
	
	// Wrap in constant nodes and add to addons
        for (Iterator it = addList.iterator(); it.hasNext();) {
	    NameValueNode nv = (NameValueNode)it.next();
	    List constName = new ArrayList();
	    constName.add(nv);
	    ConstantNode cn = new ConstantNode(constName);
	    addons.add(cn);
        }
    }        
}
