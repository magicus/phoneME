/*
 * @(#)DMArrayCopyRefTest.java	1.6 06/10/10
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

public class DMArrayCopyRefTest extends Thread {

   private String [] privateArray;
   public static Object lock;

   public DMArrayCopyRefTest(int num) {
      privateArray = new String[num];
   }

   private native void nSetArray(String [] dstArray, String [] srcArray);

   public void setArray(String [] strArray) {
	this.nSetArray(this.privateArray, strArray);
   }

   public String [] getArray() {
      int arrLength = getArrayLength();
      String [] strArray = new String[arrLength];
 
      for(int i=0; i<arrLength; i++)
         strArray[i] = privateArray[i];

       return strArray;
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
         if(outArray[i] != inArray[i]) {
            pass = false;	
            break;
         }
      }

      System.out.println();

      if(pass)
         System.out.println("PASS: DMArrayCopyRefTest, Data written and read were same");
      else
         System.out.println("FAIL: DMArrayCopyRefTest, Data written and read were not same");
   }
   
   public static void main(String args[]) {
      Random rd = new Random();
      Object lck = new Object();

      DMArrayCopyRefTest.lock = lck;
      GcThread.lock = lck;

      GcThread gc = new GcThread();
      DMArrayCopyRefTest test = new DMArrayCopyRefTest(rd.nextInt(100));

      gc.setPriority(test.getPriority() + 1);
      gc.start();
      test.start();

      try {
         test.join();
      } catch (Exception e) {}

      gc.interrupt();
   }

}
