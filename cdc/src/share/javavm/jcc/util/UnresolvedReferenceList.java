/*
 * @(#)UnresolvedReferenceList.java	1.4 06/10/10
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
package util;
import components.*;
import java.io.PrintStream;
import java.util.Enumeration;
import java.util.Hashtable;
/*
 * UnresolvedReferenceList tracks ConstantObjects
 * which could not be resolved, as well as the
 * classes in which they occur.
 * It can print the results in a readable form.
 */
public class
UnresolvedReferenceList{
    Hashtable refs = new Hashtable();

    public boolean
    hasUnresolvedReferences(){
	return !refs.isEmpty();
    }

    public void
    add(ConstantObject reference, String container){
	Hashtable containerList;
	containerList = (Hashtable)refs.get(reference);
	if (containerList == null){
	    /* not in list. Make new entry */
	    containerList = new Hashtable();
	    refs.put(reference, containerList);
	}
	/* put container in container list */
	containerList.put(container, container);
    }

    public void print(PrintStream out){
	if (refs.isEmpty())
	    return;
	Enumeration missingRefs = refs.keys();
	out.println(
	    Localizer.getString("javacodecompact.unresolved_references"));
	while (missingRefs.hasMoreElements()){
	    ConstantObject obj = (ConstantObject)(missingRefs.nextElement());
	    out.print("	"+(obj.prettyPrint()));
	    out.println(" "+
		Localizer.getString("javacodecompact.from"));
	    Hashtable containerList = (Hashtable)(refs.get(obj));
	    Enumeration containers = containerList.elements();
	    while (containers.hasMoreElements()){
		String c = (String)(containers.nextElement());
		out.println("	    "+c);
	    }
	}
    }
}
