/*
 * @(#)DMArrayCopyCharTest.java	1.6 06/10/10
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

public class DMArrayCopyCharTest extends Thread {

   private char [] privateArray;
   public static Object lock;

   public DMArrayCopyCharTest(int num) {
      privateArray = new char[num];
   }

   private native void nSetArray(char [] srcArray, char [] dstArray);

   public void setArray(char [] srcArray) {
	this.nSetArray(srcArray, this.privateArray);
   }

   public char getArrayElement(int index) {
      return(privateArray[index]);
   }

   public int getArrayLength() {
	return this.privateArray.length;
   }

   public void run() {
      int arrayLength;
      boolean pass=true;
      Random rd = new Random();

      arrayLength = getArrayLength();

      System.out.println();

      char [] charArray = new char[arrayLength];
      for(int i=0; i<arrayLength; i++)
         charArray[i] = Character.forDigit(rd.nextInt(Character.MAX_RADIX), Character.MAX_RADIX);

      setArray(charArray);

      for(int i=0; i<arrayLength; i++) {
         if(getArrayElement(i) != charArray[i]) {
            pass = false;	
            break;
         }
      }

      System.out.println();

      if(pass)
         System.out.println("PASS: DMArrayCopyCharTest, Data written and read were same");
      else
         System.out.println("FAIL: DMArrayCopyCharTest, Data written and read were not same");
   }

   public static void main(String args[]) {
      Object lck = new Object();
      Random rd = new Random();

      DMArrayCopyCharTest.lock = lck;
      GcThread.lock = lck;

      GcThread gc = new GcThread();
      DMArrayCopyCharTest test = new DMArrayCopyCharTest(rd.nextInt(100));

      gc.setPriority(test.getPriority() + 1);
      gc.start();
      test.start();

      try {
         test.join();
      } catch (Exception e) {}

      gc.interrupt();
   }

}
