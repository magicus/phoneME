/*
 * @(#)DMFieldRW32Test.java	1.6 06/10/10
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

import java.util.Random;

public class DMFieldRW32Test extends Thread {

   public int publicVar;
   private int privateVar;
   protected int protectedVar;
   public static Object lock;

   public DMFieldRW32Test() {
      publicVar = 0;
      privateVar = 0;
      protectedVar = 0;
   }

   public native void nSetValues(int v1, int v2, int v3);

   public native int [] nGetValues();

   public int [] getValues() {
      int [] intArray = {publicVar, privateVar, protectedVar};
      return intArray;
   }

   public void run() {
      boolean pass=true;
      Random rd = new Random();
      int [] var = {rd.nextInt(), rd.nextInt(), rd.nextInt()};

      System.out.println();

      nSetValues(var[0], var[1], var[2]);

      System.out.println();

      int [] intArray1 = getValues();
      int [] intArray2 = nGetValues();
	
      if(intArray1.length == intArray2.length) {
         for(int i=0; i<intArray1.length; i++) {
            if(intArray1[i] != intArray2[i]) {
               pass = false;	
               break;
            }
         }
      }
      else {
         pass = false;
      }

      System.out.println();

      if(pass)
         System.out.println("PASS: DMFieldRW32Test, Data written and read were same");
      else
         System.out.println("FAIL: DMFieldRW32Test, Data written and read were not same");
   }

   
   public static void main(String args[]) {
      Object lck = new Object();

      DMFieldRW32Test.lock = lck;
      GcThread.lock = lck;

      GcThread gc = new GcThread();
      DMFieldRW32Test test = new DMFieldRW32Test(); 

      gc.setPriority(test.getPriority() + 1);
      gc.start();
      test.start();

      try {
         test.join();
      } catch (Exception e) {}

      gc.interrupt();
   }

}

