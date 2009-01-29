/*
 * @(#)ReflectionClinitTest.java	1.7 06/10/10
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
import java.lang.reflect.Modifier;

/** Test to verify that java bug 4187388 is not present in our system
    (<clinit> for implemented interfaces shows up using
    Class.getMethods()). If "<clinit>" is printed, the test has failed. */

interface TestedInterface {
    String s = System.getProperty("Test");
}
    
public class ReflectionClinitTest {
    public static void main(String args[]) {
        try {
            Class c = Class.forName("TestedInterface");
            Method meth[] = c.getMethods();
            for (int i = 0; i < meth.length; i++)
                System.out.println(Modifier.toString(meth[i].getModifiers()) +
                                   " " + meth[i].getName());
        } catch (Throwable t) {
            t.printStackTrace();
        }
    }
}
