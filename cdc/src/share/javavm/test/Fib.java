/*
 * @(#)Fib.java	1.9 06/10/10
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

class Fib1 {
    int fib(int x) {
	if (x < 2) {
	    return 1;
	} else {
	    return fib(x - 2) + fib(x - 1);
	}
    }
    synchronized int sfib(int x) {
	if (x < 2) {
	    return 1;
	} else {
	    return sfib(x - 2) + sfib(x - 1);
	}
    }
}

public class Fib {

    public static void main(String args[]) throws NoSuchMethodException {
	Fib1 f = new Fib1();
	int count = Integer.parseInt(args[0]);
	long t2 = System.currentTimeMillis();
	int n0 = f.fib(count);
	long t3 = System.currentTimeMillis();
	Class[] args0 = new Class[] { Integer.TYPE};
	Method m0 = Fib1.class.getDeclaredMethod("fib", args0);
	sun.misc.JIT.compileMethod(m0, true);
	long t4 = System.currentTimeMillis();
	int n1 = f.fib(count);
	long t5 = System.currentTimeMillis();
	int n2 = f.sfib(count);
	long t6 = System.currentTimeMillis();
	Class[] args1 = new Class[] { Integer.TYPE};
	Method m1 = Fib1.class.getDeclaredMethod("sfib", args1);
	sun.misc.JIT.compileMethod(m1, true);
	long t7 = System.currentTimeMillis();
	int n3 = f.sfib(count);
	long t8 = System.currentTimeMillis();
	System.out.println("fib(" + count + ")=" + n0 + " (interpreted) in " +
	    (t3 - t2) + "ms");
	System.out.println("fib(" + count + ")=" + n1 + " (compiled) in " +
	    (t5 - t4) + "ms");
	System.out.println("sfib(" + count + ")=" + n2 + " (interpreted) in " +
	    (t6 - t5) + "ms");
	System.out.println("sfib(" + count + ")=" + n3 + " (compiled) in " +
	    (t8 - t7) + "ms");
    }

}
