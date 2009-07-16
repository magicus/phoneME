/*
 * @(#)IDFieldRWRefTest.java	1.6 06/10/10
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

public class IDFieldRWRefTest extends Thread {

   private String publicString;
   private String protectedString;
   private String privateString;
   public static Object lock;

   public IDFieldRWRefTest() {
      publicString = null;
      protectedString = null;
      privateString = null;
   }

   public native void nSetStrings(String str1, String str2, String str3);

   public native String [] nGetStrings();

   public String [] getStrings() {
      String [] strArray = {publicString, protectedString, privateString};
      return strArray;
   }

   public void run () {
      boolean pass=true;
      Random rd = new Random();
      String [] str = {String.valueOf(rd.nextInt()), String.valueOf(rd.nextInt()),String.valueOf(rd.nextInt())};

      nSetStrings(str[0], str[1], str[2]);

      String [] strArray1 = getStrings();
      String [] strArray2 = nGetStrings();

      if(strArray1.length == strArray2.length) {
         for(int i=0; i<strArray1.length; i++) {
            if(strArray1[i].compareTo(strArray2[i]) != 0) {
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
         System.out.println("PASS: IDFieldRWRefTest, Data written and read were same");
      else
         System.out.println("FAIL: IDFieldRWRefTest, Data written and read were not same");

   }

   public static void main(String args[]) {
      int numGcThreads = 10;
      
      IDFieldRWRefTest test = new IDFieldRWRefTest();

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
