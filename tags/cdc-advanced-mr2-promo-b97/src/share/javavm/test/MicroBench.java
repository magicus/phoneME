/*
 * @(#)MicroBench.java	1.11 06/10/10
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
import java.util.*;
import sun.misc.JIT;

class MicroBench {
    /*
     * Benchmarks should be setup to run for about 5 seconds with 5 iterations
     */
    static int iterations = 5;
    public static void main(String[] args) {
	if (args.length < 1 || args.length > 2) {
	    System.err.println("MicroBench {<benchmark>|all} [<iterations>]");
	    return;
	}
	String benchmarkName = args[0];
	if (args.length == 2) {
	    iterations = Integer.parseInt(args[1]);
	}	
	
	/* Make sure all classes use are loaded and initialized */
	new ImplementsMyInterface();
	new Child1();
	new Child2();

	/* Fill methods[] with the benchmarks to run */
	Method[] methods;
	if (!benchmarkName.equals("all")) {
	    methods = new Method[1];
	    try {
		methods[0] = 
		    MicroBench.class.getDeclaredMethod("bench" + benchmarkName,
						       null);
	    } catch(NoSuchMethodException e) {
		methods[0] = null;
	    }
	    if (methods[0] == null) {
		System.err.println("Benchmark \"" + benchmarkName +
				   "\" not found.");
		return;
	    }
	} else {
	    methods = MicroBench.class.getDeclaredMethods();
	}

	/* run all the benchmarks */
	for (int i = 0; i < methods.length; i++) {
	    Method m = methods[i];
	    if (!m.getName().startsWith("bench", 0)) {
		continue;
	    }

	    sun.misc.JIT.compileMethod(m, false);

	    try {
		System.out.println("Starting " + m.getName() + "...");
		long starttime = System.currentTimeMillis();
		for (int j = 0; j < iterations; j++) {
		    m.invoke(null, null);
		}
		long totaltime = System.currentTimeMillis() - starttime;
		System.out.println("	Time spent: " + totaltime + 
				   " milliseconds.");
	    } catch (Throwable e) {
		e.printStackTrace();
		return;
	    }
	}
    }

    /* benchmark simple object allocation */
    public static void benchUByte() {
	int i = 50000000;
	int x;
	byte[] b = new byte[1];
	while (i > 0) {
	    x = b[0] & 0xff;
	    i--;
	}
    }

    /* benchmark simple object allocation */
    public static void benchNew() {
	int i = 4000000;
	while (i > 0) {
	    new Object();
	    i--;
	}
    }

    /* benchmark an String.indexOf() instrinsic*/
    public static void benchStringIndexOf() {
	int i = 600000;
	String str = "Find the very last char in this String";
	while (i > 0) {
	    str.indexOf('g',5);
	    i--;
	}
    }

    /* benchmark an array assignment check when the object type 
       is the element type */
    public static void benchCheckInit() {
	int i = 4000000;
	while (i > 0) {
	    StaticInitClass.staticMethod();
	    i--;
	}
    }

    /* benchmark an array assignment check when the object type 
       is the element type */
    public static void benchArrayAssignmentCheckFastPath() {
	int i = 6600000;
	MicroBench o = new MicroBench();
	MicroBench[] array = new MicroBench[1];
	
	while (i > 0) {
	    array[0] = o;
	    i--;
	}
    }

    /* benchmark an array assignment check when the object type 
       is not the element type */
    public static void benchArrayAssignmentCheckSlowPath() {
	int i = 2200000;
	Child1 o = new Child1();
	Parent[] array = new Parent[1];
	
	while (i > 0) {
	    array[0] = o;
	    i--;
	}
    }

    /* benchmark an array assignment check when the object type 
       is not the element type */
    public static void benchArrayAssignmentCheckObjectPath() {
	int i = 5000000;
	MicroBench o = new MicroBench();
	Object[] array = new Object[1];
	
	while (i > 0) {
	    array[0] = o;
	    i--;
	}
    }

    /* benchmark invoke sync method */
    public static void benchInvokeSync() {
	int i = 1500000;
	ImplementsMyInterface o = new ImplementsMyInterface();
	
	while (i > 0) {
	    o.invokeSync();
	    i--;
	}
    }

    /* benchmark invoke static sync method */
    public static void benchInvokeStaticSync() {
	int i = 1500000;
	ImplementsMyInterface o = new ImplementsMyInterface();
	
	while (i > 0) {
	    o.invokeStaticSync();
	    i--;
	}
    }

    /* benchmark invokeinterface with a successful guess */
    public static void benchInvokeInterface() {
	int i = 2000000;
	MyInterface o = new ImplementsMyInterface();
	
	while (i > 0) {
	    o.invokeinterface();
	    i--;
	}
    }

    /* benchmark invokeinterface with a bad guess */
    public static void benchInvokeInterfaceBadGuess() {
        int i = 2000000;
        MyInterface o1 = new ImplementsMyInterface();
        MyInterface o2 = new ImplementsMyInterface2();

        while (i > 0) {
            MyInterface o = o1;
            o.invokeinterface();
            i--;
            o1 = o2;
            o2 = o; 
        }
    }

    /* benchmark instanceof with a successful guess */
    public static void benchInstanceOf() {
	int i = 10000000;
	Object o = new MicroBench();
	
	while (i > 0) {
	    if (o instanceof MicroBench)
		i--;
	}
    }

    /* benchmark checkcast with a successful guess */
    public static void benchCheckCastGoodGuess() {
	int i = 10000000;
	Object o = new MicroBench();
	
	while (i > 0) {
	    MicroBench m = (MicroBench)o;
	    i--;
	}
    }

    /* benchmark checkcast with a bad guess */
    public static void benchCheckCastBadGuess() {
	int i = 2200000;
	Object o1 = new Child1();
	Object o2 = new Child2();
	
	while (i > 0) {
	    Parent m = (Parent)o1;
	    i--;
	    Object temp = o1;
	    o1 = o2;
	    o2 = temp;
	}
    }

    /* benchmark doing a monitorenter/exit pair */
    public static void benchMonitorEnterExit() {
	int i = 320000;
	Object o = new Object();
	while (i > 0) {
	    synchronized(o) {
		i--;
	    }
	}
    }
}

interface MyInterface {
    public void invokeinterface();
}

interface MyInterface2 {
    public void invokeinterface2();
}

interface MyInterface3 {
    public void invokeinterface3();
}

class ImplementsMyInterface implements MyInterface {
    public void invokeinterface() {
	return;
    }
    public synchronized void invokeSync() {
	return;
    }
    public static synchronized void invokeStaticSync() {
	return;
    }
}

class ImplementsMyInterface2 
    implements MyInterface2, MyInterface, MyInterface3
{
    public void invokeinterface() {
        return;
    }

    public void invokeinterface2() {
        return;
    }

    public void invokeinterface3() {
        return;
    } 
}

class Parent {}
class Child1 extends Parent {}
class Child2 extends Parent {}

class StaticInitClass {
    static {
	new Object();
    }
    static void staticMethod() {}
}
