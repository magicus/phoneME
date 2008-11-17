/*
 * @(#)DMArrayCopyBooleanTest.java	1.6 06/10/10
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

public class DMArrayCopyBooleanTest extends Thread {

   private boolean [] privateArray;
   public static Object lock;

   public DMArrayCopyBooleanTest(int num) {
      privateArray = new boolean[num];
   }

   private native void nSetArray(boolean [] srcArray, boolean [] dstArray);

   public void setArray(boolean [] srcArray) {
	this.nSetArray(srcArray, this.privateArray);
   }

   public boolean getArrayElement(int index) {
        return (privateArray[index]);
   }

   public int getArrayLength() {
	return this.privateArray.length;
   }

   public void run() {
      int arrayLength;
      boolean pass=true;

      arrayLength = getArrayLength();

      System.out.println();

      boolean [] boolArray = new boolean[arrayLength];
      for(int i=0; i<arrayLength; i++) {
         if(i%2 == 0)
            boolArray[i] = true;
         else
            boolArray[i] = false;
      }

      setArray(boolArray);

      for(int i=0; i<arrayLength; i++) {
         if(getArrayElement(i) != boolArray[i]) {
            pass = false;	
            break;
         }
      }

      System.out.println();

      if(pass)
         System.out.println("PASS: DMArrayCopyBooleanTest, Data written and read were same");
      else
         System.out.println("FAIL: DMArrayCopyBooleanTest, Data written and read were not same");
   }

   public static void main(String args[]) {
      Random rd = new Random();
      Object lck = new Object();

      DMArrayCopyBooleanTest.lock = lck;
      GcThread.lock = lck;

      DMArrayCopyBooleanTest test = new DMArrayCopyBooleanTest(rd.nextInt(100));
      GcThread gc = new GcThread();

      gc.setPriority(test.getPriority() + 1);
      gc.start();
      test.start();

      try {
         test.join();
      } catch (Exception e) {}

      gc.interrupt();
   }
}
