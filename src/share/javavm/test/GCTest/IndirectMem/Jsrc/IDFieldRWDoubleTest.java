/*
 * @(#)IDFieldRWDoubleTest.java	1.6 06/10/10
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

public class IDFieldRWDoubleTest extends Thread {

   public double publicVar;
   private double privateVar;
   protected double protectedVar;

   public IDFieldRWDoubleTest() {
      publicVar = 0.0;
      privateVar = 0.0;
      protectedVar = 0.0;
   }

   public native void nSetValues(double v1, double v2, double v3);

   public native double [] nGetValues();

   public double [] getValues() {
      double [] doubleArray = {publicVar, privateVar, protectedVar};
      return doubleArray;
   }

   public void run() {
      boolean pass=true;
      Random rd = new Random();
      double [] var = {rd.nextDouble(), rd.nextDouble(), rd.nextDouble()};

      nSetValues(var[0], var[1], var[2]);

      double [] doubleArray1 = getValues();
      double [] doubleArray2 = nGetValues();

      if(doubleArray1.length == doubleArray2.length) {
         for(int i=0; i<doubleArray1.length; i++) {
            if(doubleArray1[i] != doubleArray2[i]) {
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
         System.out.println("PASS: IDFieldRWDoubleTest, Data written and read were same");
      else
         System.out.println("FAIL: IDFieldRWDoubleTest, Data written and read were not same");

   }

   public static void main(String args[]) {
      int numGcThreads = 10;

      IDFieldRWDoubleTest test = new IDFieldRWDoubleTest();

      GcThread.sleepCount = numGcThreads;
      GcThread [] gc = new GcThread[numGcThreads];
      for(int i=0; i<gc.length; i++) {
         gc[i] = new GcThread();
         gc[i].start();
      }

      test.start();

      try {
         test.join();
      } catch (Exception e) {}

      for(int i=0; i<gc.length; i++) {
         gc[i].interrupt();
      }

      try {
         for(int i=0; i<gc.length; i++) {
            gc[i].join();
         }
      } catch (Exception e) {}

   }

}

