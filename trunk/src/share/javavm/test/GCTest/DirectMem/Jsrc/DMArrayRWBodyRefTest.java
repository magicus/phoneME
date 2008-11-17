/*
 * @(#)DMArrayRWBodyRefTest.java	1.6 06/10/10
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

public class DMArrayRWBodyRefTest extends Thread {

   private String [] privateArray;
   public static Object lock;

   public DMArrayRWBodyRefTest(int num) {
      privateArray = new String[num];
   }

   private native void nSetArray(String[] dstArray, String[] srcArray, int arrayLength);

   public void setArray(String [] srcArray) {
        this.nSetArray(this.privateArray, srcArray, getArrayLength());
   }

   public String [] getArray() {
      int arrLength = getArrayLength();
      String [] stringArray = new String[arrLength];

      for(int i=0; i<arrLength; i++)
         stringArray[i] = privateArray[i];

       return stringArray;
   }

   public int getArrayLength() {
	return this.privateArray.length;
   }

   public void run() {
      int arrayLength;
      boolean pass=true;
      Random rd = new Random();

      arrayLength = getArrayLength();

      String [] inArray = new String[arrayLength];
      for(int i=0; i<arrayLength; i++)
         inArray[i] = String.valueOf(rd.nextInt());

      setArray(inArray);
      String [] outArray = getArray();

      for(int i=0; i<arrayLength; i++) {
         if(inArray[i] != outArray[i]) {
            pass = false;
            break;
         }
      }

      System.out.println();

      if(pass)
         System.out.println("PASS: DMArrayRWBodyRefTest, Data written and read were same");
      else
         System.out.println("FAIL: DMArrayRWBodyRefTest, Data written and read were not same");
   }
   
   public static void main(String args[]) {
      Random rd = new Random();
      Object lck = new Object();

      DMArrayRWBodyRefTest.lock = lck;
      GcThread.lock = lck;

      GcThread gc = new GcThread();
      DMArrayRWBodyRefTest test = new DMArrayRWBodyRefTest(rd.nextInt(100));

      gc.setPriority(test.getPriority() + 1);
      gc.start();
      test.start();

      try {
         test.join();
      } catch (Exception e) {}

      gc.interrupt();
   }

}
