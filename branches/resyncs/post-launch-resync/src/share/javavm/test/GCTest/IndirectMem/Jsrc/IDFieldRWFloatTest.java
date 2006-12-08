/*
 * @(#)IDFieldRWFloatTest.java	1.6 06/10/10
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.  
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

public class IDFieldRWFloatTest extends Thread {

   public float cvar;
   private float evar;
   protected float dvar;

   public IDFieldRWFloatTest() {
      cvar = 0.0f;
      evar = 0.0f;
      dvar = 0.0f;
   }

   public native void nSetValues(float v1, float v2, float v3);

   public native float [] nGetValues();

   public float [] getValues() {
      float [] floatArray = {cvar, evar, dvar};
      return floatArray;
   }

   public void run() {
      boolean pass=true;
      Random rd = new Random();
      float [] var = {rd.nextFloat(), rd.nextFloat(), rd.nextFloat()};

      nSetValues(var[0], var[1], var[2]);

      float [] floatArray1 = getValues();
      float [] floatArray2 = nGetValues();

      if(floatArray1.length == floatArray2.length) {
         for(int i=0; i<floatArray1.length; i++) {
            if(floatArray1[i] != floatArray2[i]) {
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
         System.out.println("PASS: IDFieldRWFloatTest, Data written and read were same");
      else
         System.out.println("FAIL: IDFieldRWFloatTest, Data written and read were not same");

   }

   
   public static void main(String args[]) {
      int numGcThreads = 10;

      IDFieldRWFloatTest test = new IDFieldRWFloatTest();

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

