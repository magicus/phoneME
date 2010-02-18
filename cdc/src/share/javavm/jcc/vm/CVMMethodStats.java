/*
 * @(#)CVMMethodStats.java	1.5 06/10/10
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
package vm;
import consts.Const;
import consts.CVMConst;
import components.*;
import vm.*;
import jcc.Util;
import util.*;
import java.util.Arrays;
import java.util.Comparator;
import java.util.Enumeration;
import java.util.Hashtable;
import java.io.OutputStream;

public class CVMMethodStats implements Comparator {

    int tableIdx;
    Entry[] table;
    
    public class Entry {
	// Remember these three
	int argsSize;
	String invoker;
	int access;

	// Index in global table
	int idx;
	
	//
	// And count references. The top referenced guys are going to
	// be at the top of the table
	//
	int refcount;
	
	Entry(int argsSize, String invoker, int access) {
	    this.argsSize = argsSize;
	    this.invoker  = invoker;
	    this.access   = access;
	    this.refcount = 1;
	}
	
	public String toString() 
	{
	    return "["+
		"argsSize = "+argsSize+
		", access = "+access+
		", invoker = "+invoker+
		"]";
	}

	public int getIdx() 
	{
	    return idx;
	}
	
	public String getInvoker() 
	{
	    return invoker;
	}
	
    }
    
    public CVMMethodStats() {
	super();
	// This should be more than enough
	table = new Entry[1024];
	tableIdx = 0;
    }
    
    public void record(int argsSize, String invoker, int access) {
	Entry entry = lookup(argsSize, invoker, access);
	if (entry == null) {
	    entry = new Entry(argsSize, invoker, access);
	    // Created a new one. Add to table
	    entry.idx = tableIdx;
	    table[tableIdx++] = entry;
	} else {
	    // Just increment refcount
	    entry.refcount++;
	}
    }
    
    public Entry lookup(int argsSize, String invoker, int access) {
	for (int i = 0; i < tableIdx; i++) {
	    Entry entry = table[i];
	    if ((entry.argsSize == argsSize) &&
		(entry.access   == access)   &&
		invoker.equals(entry.invoker)) {
		return entry;
	    }
	}
	return null;
    }
    
    public void sort() 
    {
	Entry newTable[] = new Entry[tableIdx];
	System.arraycopy(table, 0, newTable, 0, tableIdx);
	Arrays.sort(newTable, this);
	for (int i = 0; i < tableIdx; i++) {
	    newTable[i].idx = i;
	}
	table = newTable;
    }

    public int compare(java.lang.Object o1, java.lang.Object o2) {
        Entry obj1 = (Entry) o1;
        Entry obj2 = (Entry) o2;

        if (obj1.refcount < obj2.refcount) {
	   return 1;
        } else if (obj1.refcount == obj2.refcount)
           return 0;
        return -1;
    }

    //
    // Dump the first 256 elements at most
    //
    public void dump(runtime.CCodeWriter out) {
	int numElements = Math.min(256, tableIdx);
	out.println();
	out.println("const CVMUint16 argsInvokerAccess[] = {");
	for (int i = 0; i < numElements; i++) {
	    out.print("    /* idx="+i+", refcount="+table[i].refcount+" */ ");
	    out.println(table[i].argsSize + ", "+table[i].invoker+", "+
			table[i].access+",");
	}
	out.println("};");
	out.println();
    }
}
