/*
 * @(#)ReflectionBug.java	1.7 06/10/10
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
import java.lang.reflect.*;

/** Illustrates a bug in Java 1.2's specification of
  Field.set{Boolean,Int,...}. These and the Field.get{primitive}
  operations should NOT be specified in terms of Field.set and
  Field.get because (a) it's confusing and (b) as shown here,
  Field.set wasn't precise enough and is therefore wrong. */

public class ReflectionBug {
    public Boolean field = new Boolean(false);

    public static void main(String[] args) {
	ReflectionBug bug = new ReflectionBug();
	Class bugClass = bug.getClass();
	try {
	    Field bugField = bugClass.getField("field");
	    bugField.set(bug, new Boolean(true));
	    if (bug.field.booleanValue() == false) {
		System.out.println("Internal error in test");
	    }
	    bug.field = new Boolean(false);
	    bugField.setBoolean(bug, true);
	    if (bug.field.booleanValue() == true) {
		System.out.println("No bug found.");
	    }
	}
	catch (Exception e) {
	    System.out.println("Bug in JDK 1.2 reflection spec");
	}
    }
}
