/*
 * @(#)ReflectionSecurity.java	1.7 06/10/10
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

/** Verify that we cannot bypass checkMemberAccess checks by
    using reflection. */

public class ReflectionSecurity {


    public static void main(String[] args) {
	try {
	    Class[] x2 = Object.class.getDeclaredClasses();
	    System.err.println("Object.getDeclaredClasses OK: " + x2);
	} catch (Exception e) {
	    e.printStackTrace();
	}
	try {
	    Class[] sig = {};
	    Method m = Class.class.getMethod("getDeclaredClasses", sig);
	    Object[] args0 = {};

	    // Method m = Object.class.getDeclaredClasses();

	    try {
		Object y = m.invoke(Object.class, args0);
		System.err.println("Reflection getDeclaredClasses OK " + y);
		System.err.println("Was java.security.manager set?");
	    } catch (InvocationTargetException e) {
		throw (Exception)e.getTargetException();
	    }
	} catch (Exception e) {
	    e.printStackTrace();
	}
    }
}
