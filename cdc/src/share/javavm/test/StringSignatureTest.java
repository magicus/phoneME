/*
 * @(#)StringSignatureTest.java	1.11 06/10/10
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
/** Exercises the regression test for the bug that used to be in the 
    TypeID system:

    1. Reflection code makes full signature iterator for method's
    TypeID.

    2. Full signature iterator runs until it returns
    CVM_TYPEID_ENDFUNC.

    3. Terse signature contains CVM_TYPEID_OBJ for String argument to
    method. Detail for that argument, though, contains a field TypeID
    of 12, which is confused with CVM_TYPEID_ENDFUNC even though
    conceptually these two tokens are in different namespaces.

    This caused an assertion failure in CVMreflectGetParameterTypes on
    debug builds.
*/

import java.lang.reflect.*;

public class StringSignatureTest {
    public void methodTakingString(String x) {
	System.out.println("methodTakingString(" + x + ")");
    }

    public static void main(String[] args) {
	try {
	    StringSignatureTest test = new StringSignatureTest();
	    // Next line causes assertion failure
	    Method method =
		test.getClass().getMethod("methodTakingString",
					  new Class[] { String.class });
	    method.invoke(test, new Object[] { "Hello, world" });
	    System.err.println("Test passed.");
	}
	catch (Exception e) {
	    e.printStackTrace();
	    System.err.println("TEST FAILED");
	}
    }
}
