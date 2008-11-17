/*
 * @(#)DMArrayRWBodyCharTest.java	1.6 06/10/10
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

public class DMArrayRWBodyCharTest extends Thread {

   private char [] privateArray;
   public static Object lock;

   public DMArrayRWBodyCharTest(int num) {
      privateArray = new char[num];
   }

   private native void nSetArray(char[] dstArray, char[] srcArray, int arrayLength);

   public void setArray(char [] srcArray) {
        this.nSetArray(this.privateArray, srcArray, getArrayLength());
   }

   public char [] getArray() {
      int arrLength = getArrayLength();
      char [] charArray = new char[arrLength];

      for(int i=0; i<arrLength; i++)
         charArray[i] = privateArray[i];

       return charArray;
   }

   public int getArrayLength() {
	return this.privateArray.length;
   }

   public void run() {
      int arrayLength;
      boolean pass=true;
      Random rd = new Random();

      arrayLength = getArrayLength();

      char [] inArray = new char[arrayLength];
      for(int i=0; i<arrayLength; i++)
         inArray[i] = Character.forDigit(rd.nextInt(Character.MAX_RADIX), Character.MAX_RADIX);

      setArray(inArray);
      char [] outArray = getArray();

      for(int i=0; i<arrayLength; i++) {
         if(inArray[i] != outArray[i]) {
            pass = false;
            break;
         }
      }

      System.out.println();

      if(pass)
         System.out.println("PASS: DMArrayRWBodyCharTest, Data written and read were same");
      else
         System.out.println("FAIL: DMArrayRWBodyCharTest, Data written and read were not same");
   }
   
   public static void main(String args[]) {
      Random rd = new Random();
      Object lck = new Object();

      DMArrayRWBodyCharTest.lock = lck;
      GcThread.lock = lck;

      GcThread gc = new GcThread();
      DMArrayRWBodyCharTest test = new DMArrayRWBodyCharTest(rd.nextInt(100));

      gc.setPriority(test.getPriority() + 1);
      gc.start();
      test.start();

      try {
         test.join();
      } catch (Exception e) {}

      gc.interrupt();
   }

}
