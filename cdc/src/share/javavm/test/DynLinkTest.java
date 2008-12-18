/*
 * @(#)DynLinkTest.java	1.8 06/10/10
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
/** Test dynamic linking functionality. Since we don't have full
    dynamic classloading support, call the dynamic linking routines
    from a native method. This test takes two string arguments: the
    first is the pathname of the shared object to load, and the second
    is the name of the function in it to call. The function must
    return void and take no arguments. */

public class DynLinkTest {
    public static void usage() {
	System.out.println("usage: cvm DynLinkTest <pathname of shared object to load> <name of function in it to call>");
	System.out.println("The function must return void and take no arguments.");
    }

    private static native void callDynLinkedFunc(String sharedObjectPathname,
						 String functionName);

    public static void main(String[] args) {
	if (args.length != 2) {
	    usage();
	    return;
	}
	
	callDynLinkedFunc(args[0], args[1]);
    }
}
