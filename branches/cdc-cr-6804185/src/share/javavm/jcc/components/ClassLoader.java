/*
 * @(#)ClassLoader.java	1.2 06/11/07
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

package components;
import jcc.Util;
import jcc.Str2ID;
import consts.Const;
import util.*;

import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Set;
import java.util.Vector;

/*
 *
 */

public
class ClassLoader
{
    String name;
    ClassLoader parent;
    Hashtable classes;
    static int currentID;
    int id;

    public ClassLoader(String n, ClassLoader p) {
	id = currentID++;
	name = n;
	parent = p;
	classes = new Hashtable();
    }

    public ClassInfo
    lookupClass(String key) {
        Assert.disallowClassloading();
        ClassInfo ci = null;
	if (parent != null) {
	    ci = parent.lookupClass(key);
	}
	if (ci == null) {
	    ci = (ClassInfo)classes.get(key);
	}
        Assert.allowClassloading();
        return ci;
    }

    boolean
    enterClass(ClassInfo c){
	String    className = c.className;
	c.loader = this;
	// check to see if a class of this name is already there...
	if (classes.containsKey( className )){
	    System.err.println(Localizer.getString(
                "classtable.class_table_already_contains", className));
	    return false;
	}
	classes.put( className, c );
	return true;
    }

    public String getName() {
	return name;
    }

    public int getID() {
	return id;
    }

    public ClassLoader getParent() {
	return parent;
    }

    private ClassFileFinder searchPath;

    public void setSearchPath(ClassFileFinder sp) {
	searchPath = sp;
    }

    public ClassFileFinder getSearchPath() {
	return searchPath;
    }

    public String toString() {
	return super.toString() + " name " + name;
    }

}
