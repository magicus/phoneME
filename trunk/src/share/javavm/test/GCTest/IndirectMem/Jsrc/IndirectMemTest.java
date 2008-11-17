/*
 * @(#)IndirectMemTest.java	1.6 06/10/10
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

public class IndirectMemTest extends Thread {

   public static void main(String args[]) {
      Random rd = new Random();
      int numGcThreads = 100;

      IDArrayRWCharTest test1 = new IDArrayRWCharTest(rd.nextInt(100));
      IDArrayRWByteTest test2 = new IDArrayRWByteTest(rd.nextInt(100));
      IDArrayRWBooleanTest test3 = new IDArrayRWBooleanTest(rd.nextInt(100));
      IDArrayRWLongTest test4 = new IDArrayRWLongTest(rd.nextInt(100));
      IDArrayRWShortTest test5 = new IDArrayRWShortTest(rd.nextInt(100));
      IDArrayRWIntTest test6 = new IDArrayRWIntTest(rd.nextInt(100));
      IDArrayRWFloatTest test7 = new IDArrayRWFloatTest(rd.nextInt(100));
      IDArrayRWDoubleTest test8 = new IDArrayRWDoubleTest(rd.nextInt(100));
      IDArrayRWRefTest test9 = new IDArrayRWRefTest(rd.nextInt(100));
      IDFieldRW32Test test10 = new IDFieldRW32Test();
      IDFieldRWDoubleTest test11 = new IDFieldRWDoubleTest();
      IDFieldRWFloatTest test12 = new IDFieldRWFloatTest();
      IDFieldRWIntTest test13 = new IDFieldRWIntTest();
      IDFieldRWLongTest test14 = new IDFieldRWLongTest();
      IDFieldRWRefTest test15 = new IDFieldRWRefTest();

      GcThread.sleepCount = numGcThreads;
      GcThread [] gc = new GcThread[numGcThreads];
      for(int i=0; i<gc.length; i++) {
         gc[i] = new GcThread();
         gc[i].start();
      }

      test1.start();
      test2.start();
      test3.start();
      test4.start();
      test5.start();
      test6.start();
      test7.start();
      test8.start();
      test9.start();
      test10.start();
      test11.start();
      test12.start();
      test13.start();
      test14.start();
      test15.start();

      try {
         test1.join();
         test2.join();
         test3.join();
         test4.join();
         test5.join();
         test6.join();
         test7.join();
         test8.join();
         test9.join();
         test10.join();
         test11.join();
         test12.join();
         test13.join();
         test14.join();
         test15.join();
         System.out.println("All tests joined");
      } catch (Exception e) {}

      for(int i=0; i<gc.length; i++) {
         gc[i].interrupt();
      }

      try {
         for(int i=0; i<gc.length; i++) {
            gc[i].join();
            System.out.println("All GC threads joined");
         }
      } catch (Exception e) {}
   }

}
