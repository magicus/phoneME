/*
 * @(#)ClassnameFilterList.java	1.8 06/10/10
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
 *
 */

package util;

import java.util.Vector;

/*
 * A ClassnameFilterList stores a list of ClassnameFilter objects.
 * It is used for mapping a classname into group, such as "JNI"
 */

public
class ClassnameFilterList {

    public ClassnameFilterList() {
	groups = new Vector();
	filters = new Vector();
    }

    public void addTypePatterns(String group, String patterns ) {
	boolean conditional;
	// If a pattern starts with '-', it matches only if there
	// were no earlier matches
	if (patterns.charAt(0) == '-') {
	    conditional = true;
	    patterns = patterns.substring(1);
	} else {
	    conditional = false;
	}
	ClassnameFilter f = new ClassnameFilter(conditional);
	parseClassList( patterns, f );
	groups.addElement(group.intern());
	filters.addElement(f);
    }

    public String[] getTypes(String classname) {
	Vector types = new Vector(1);
	int l = groups.size();
	for (int i = 0; i < l; ++i) {
	    String name = (String)groups.elementAt(i);
	    ClassnameFilter f = (ClassnameFilter)filters.elementAt(i);
	    // If a pattern is conditional, it matches only if there
	    // were no earlier matches
	    if (types.size() == 0 || !f.conditional) {
		if (f.accept(null, classname)) {
		    types.addElement(name);
		}
	    }
	}
	String[] strings = new String[types.size()];
	types.copyInto(strings);
	return strings;
    }

    public boolean isType(String classname, String type) {
	String[] types = getTypes(classname);
	for (int i = 0; i < types.length; ++i) {
	    if (types[i] == type.intern()) {
		return true;
	    }
	}
	return false;
    }

    // Parse the rest of the string as a list of classes.
    private void parseClassList(String val, ClassnameFilter filter) {
	java.util.StringTokenizer tkn =
	    new java.util.StringTokenizer(val, " ,", false );
	while ( tkn.hasMoreTokens() ){
	    String classname =
		util.LinkerUtil.sanitizeClassname( tkn.nextToken() );
	    filter.includeName(classname);
	}
    }

    private Vector groups;
    private Vector filters;

}
