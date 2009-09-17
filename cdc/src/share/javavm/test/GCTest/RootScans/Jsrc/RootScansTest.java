/*
 * @(#)RootScansTest.java	1.9 06/10/10
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

import java.lang.*;
import java.lang.ref.*;
import java.util.*;
import java.io.*;

public class RootScansTest extends Thread {
   private Object obj;
   protected Double [] dbl;
   public FileOutputStream fo = null;
   public static String s1 = new String("Hello World");

   public RootScansTest() {
      obj = new Object();
      dbl = new Double[10];
      try {
         fo = new FileOutputStream("temp");
      } catch (Exception e) {
      }
   }

   private native static void scanRootsTest1();

   private native static void scanRootsTest2();

   private void createJavaRoots() {
      String [] s2 = {"This", "is", "a", "RootScans", "test"};
      String [] s3 = new String[s2.length];
      Class clazz;
      FileInputStream in = null;
      FileOutputStream out = null;
      byte[] buf = {'h', 'e', 'l', 'l', 'o', '\n'};
      int max_count = 100;
      Object [] wobj = new Object[max_count] ;
      WeakReference [] ref;
      ReferenceQueue aRefQ = new ReferenceQueue();

      // Create Intern Strings
      for(int i = 0; i < s2.length; i++)
         s3[i] = s2[i].intern();

      // Create Finalizable objects and Java locals
      try {
         out = new FileOutputStream("temp"); 
         out.write(buf);
         in = new FileInputStream("temp"); 
      } catch (Exception e) {
         System.out.println("Class not found Exception");
      }

      // Create Weak References
      ref = new WeakReference[max_count];

      for(int count = 0; count < max_count; count++) {
         wobj[count] = new Object();

         ref[count] = new WeakReference(wobj[count], aRefQ);

         ref[count].enqueue();

         // Break the strong reference to the object
         wobj[count] = null;
      }

      System.gc();
      
   }

   public void run() {
      //RootScansTest1 tests if all the roots were scanned when a gc happens
      System.out.println();
      scanRootsTest1();

      //RootScansTest2 tests if all the roots were scanned only once when a 
      //gc happens
      System.out.println();
      scanRootsTest2();
   }

   public static void main(String [] args) {
      RootScansTest rs = new RootScansTest();

      rs.start();

      try {
         rs.join();
      } catch(Exception e) {
      }
   }
}
