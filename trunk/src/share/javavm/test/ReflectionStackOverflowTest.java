/*
 * @(#)ReflectionStackOverflowTest.java	1.8 06/10/10
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

/** Verify that we throw StackOverflowErrors properly when
  Method.invoke() is used recursively, as per Java bug #4217960 which
  was filed against JDK 1.2 in March. */

public class ReflectionStackOverflowTest {

    static int depth = 0;
    static Method m;

    public static void test(Method m) throws Throwable {
        ++depth;
	if ((depth % 1000) == 0)
	    System.err.println("Test at depth " + depth);

        Object[] args = {m};
        try {
            m.invoke(null, args);
        } catch (InvocationTargetException e) {
            throw e.getTargetException();
        }
    }

    public static void main(String[] args) {
	try {
            Class c = ReflectionStackOverflowTest.class;
            Class[] sig = {Method.class};
            m = c.getMethod("test", sig);
            test(m);
	}
	catch (StackOverflowError e) {
	    System.out.println("Stack overflow error successfully detected");
	}
	catch (Throwable e) {
	    e.printStackTrace();
	    System.out.println("ERROR: Some other exception caught");
	}
    }
}
