/*
 * @(#)DMFieldRWFloatTest.java	1.6 06/10/10
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

public class DMFieldRWFloatTest extends Thread {

   public float cvar;
   private float evar;
   protected float dvar;
   public static Object lock;

   public DMFieldRWFloatTest() {
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

      System.out.println();

      nSetValues(var[0], var[1], var[2]);

      System.out.println();

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
         System.out.println("PASS: DMFieldRWFloatTest, Data written and read were same");
      else
         System.out.println("FAIL: DMFieldRWFloatTest, Data written and read were not same");

   }

   
   public static void main(String args[]) {
      Object lck = new Object();

      DMFieldRWFloatTest.lock = lck;
      GcThread.lock = lck;

      GcThread gc = new GcThread();
      DMFieldRWFloatTest test = new DMFieldRWFloatTest();

      gc.setPriority(test.getPriority() + 1);
      gc.start();
      test.start();

      try {
         test.join();
      } catch (Exception e) {}

      gc.interrupt();
   }

}

