/*
 * @(#)ClassTable.java	1.4 06/10/10
 *
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
 * The symbol table for classes.
 * Currently contains two distince tables: the main table
 * and the alternate table.
 * The choice of which table to insert a class in depends
 * on the altNametable flag in the ClassInfo.flags field.
 * The choice of lookup depends on the useAltTable flag.
 * If the alt table is searched, it is searched before the
 * main table, which is searched if the entry was not in the alt.
 */

public
class ClassTable
{

    /**
     * We keep track of classes by hashing them by name when
     * we read them. They can be looked up using lookupClass,
     * which will take a classname string as parameter.
     */
    static Hashtable mainClasstable = new Hashtable();
    static Hashtable altClasstable  = new Hashtable();

    public static boolean
    enterClass(ClassInfo c){
	Hashtable classtable;
	String    className = c.className;
	// check to see if a class of this name is already there...
	classtable = c.altNametable ? altClasstable : mainClasstable;
	if (classtable.containsKey( className )){
	    System.err.println(Localizer.getString("classtable.class_table_already_contains", className));
	    return false;
	}
	classtable.put( className, c );
	// Make sure a classvector hasn't been created yet.
	// (used to add, now we just assert that it isn't necessary).
	if (vm.ClassClass.hasClassVector()){
	    System.err.println(Localizer.getString("classtable.class_vector_in_place",
				className));
	    return false;
	}
	return true;
    }

    public static boolean
    enterClasses(Enumeration e, boolean useAltTable){
	while(e.hasMoreElements()){
	    ClassInfo c = (ClassInfo)(e.nextElement());
	    c.altNametable = useAltTable;
	    if (!enterClass(c))
		return false;
	}
	return true;
    }

    public static ClassInfo
    lookupClass(String key, boolean useAltTable){
	Object result;
	if (useAltTable){
	    result = altClasstable.get(key);
	    if (result != null)
		return (ClassInfo)result;
	}
	return (ClassInfo)mainClasstable.get(key);
    }

    public static int size(){
	return mainClasstable.size() + altClasstable.size();
    }

    public static Enumeration elements(){
	return new TupleEnumeration(mainClasstable.elements(),
				    altClasstable.elements());
    }

    public static ClassInfo[]
    allClasses()
    {
	Enumeration classEnum = elements();
	int nclasses = size();
	ClassInfo ary[] = new ClassInfo[ nclasses ];
	for (int i= 0; i < nclasses; i++){
	    ary[i] = (ClassInfo) classEnum.nextElement();
	}
	return ary;
    }

    public static Set setOfClassnames(){
	// this one is a problem. 
	return mainClasstable.keySet();
    }

}
