/*
 * @(#)Version.java	1.14 06/10/10
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

package sun.misc;
import java.io.PrintStream;

public class Version {
    /*
    static {
	init();
    }
    */

    public static void init() {
	/* 
	 * java.version is already set in 
	 * Java_java_lang_System_initProperties() 
	 *
	 * System.setProperty("java.version", java_version);
	 * 
	 * In CVM, "java.runtime.*" properties are not defined since 
	 * they are not officially required.
	 *
	 * System.setProperty("java.runtime.version", java_runtime_version);
	 * System.setProperty("java.runtime.name", java_runtime_name);
	 */
    }

    /**
     * In case you were wondering this method is called by java -version.
     * Sad that it prints to stderr; would be nicer if default printed on
     * stdout.
     */
    public static void print(boolean longVersion) {
	print(System.err, longVersion);
    }

    /**
     * Give a stream, it will print version info on it.
     */
    public static void print(PrintStream ps, boolean longVersion) {
	String productName = System.getProperty("sun.misc.product") + " (" +
	    System.getProperty("java.version") + ")";
	if (!longVersion) {
	    ps.println(productName);
	} else {
	    /* First line: Product. */
	    ps.println("Product: " + productName);
	    
	    /* Second line: J2ME Profile. */
	    ps.println("Profile: " +
		       System.getProperty("java.specification.name") + " " +
		       System.getProperty("java.specification.version"));
	    
	    /* Third line: JVM information. */
	    String java_vm_name    =
		(String)System.getProperty("java.vm.name");
	    String java_vm_version    =
		System.getProperty("java.vm.version");
	    String java_vm_info    =
		System.getProperty("java.vm.info");
	    ps.println("JVM:     " + java_vm_name + " " +
                       java_vm_version + " (" +
		       java_vm_info + ")");
	}
    }
}
