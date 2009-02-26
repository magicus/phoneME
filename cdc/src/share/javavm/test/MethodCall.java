/*
 * @(#)MethodCall.java	1.7 06/10/10
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

import java.lang.reflect.Method;
import sun.misc.JIT;

class C1 {
    boolean printTrace = false;
    void xxx() {
	if (printTrace) {
	    new Exception("backtrace").printStackTrace();
	}
    }

    int zoo(int x) {
	if ((x & 0xff) == 0xff) {
	    xxx();
	}
	return x + 3;
    }
    int goo(int x) {
	return zoo(x + 2);
    }
    int foo(int x) {
	return goo(x + 1);
    }
}

public class MethodCall {

    private static void testMethod(C1 c, int i) {
	c.foo(i);
    }

    public static void main(String args[]) throws NoSuchMethodException {
	C1 c = new C1();
	int iterations = Integer.parseInt(args[0]);
	long t1 = System.currentTimeMillis();
	for (int i = 0; i < iterations; ++i) {
	}
	long t2 = System.currentTimeMillis();
	long nullTime = t2 - t1;
	t2 = System.currentTimeMillis();
	for (int i = 0; i < iterations; ++i) {
	    testMethod(c, i);
	}
	long t3 = System.currentTimeMillis();
	t3 = System.currentTimeMillis();
	Class[] args0 = new Class[] { Integer.TYPE};
	Method m0 = C1.class.getDeclaredMethod("goo", args0);
	sun.misc.JIT.compileMethod(m0, true);
	long t4 = System.currentTimeMillis();
	for (int i = 0; i < iterations; ++i) {
	    testMethod(c, i);
	}
	long t5 = System.currentTimeMillis();
	t5 = System.currentTimeMillis();
	Class[] args1 = new Class[] { Integer.TYPE};
	Method m1 = C1.class.getDeclaredMethod("zoo", args1);
	sun.misc.JIT.compileMethod(m1, true);
	long t6 = System.currentTimeMillis();
	for (int i = 0; i < iterations; ++i) {
	    testMethod(c, i);
	}
	long t7 = System.currentTimeMillis();
	c.printTrace = true;
	for (int i = 0; i < iterations; ++i) {
	    testMethod(c, i);
	}
	System.out.println("null loop " + iterations + " iterations in " +
	    nullTime + "ms");
	System.out.println("goo interpreted " + iterations + " iterations in " +
	    (t3 - t2 - nullTime) + "ms");
	System.out.println("goo compiled " + iterations + " iterations in " +
	    (t5 - t4 - nullTime) + "ms");
	System.out.println("goo,zoo compiled " + iterations +
	    " iterations in " + (t7 - t6 - nullTime) + "ms");
    }

}
