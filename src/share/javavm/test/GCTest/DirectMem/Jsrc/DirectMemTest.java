/*
 * @(#)DirectMemTest.java	1.6 06/10/10
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

public class DirectMemTest extends Thread {

   public static void main(String args[]) {
      Object lck = new Object();

      DMFieldRWIntTest.lock = lck;
      DMFieldRW32Test.lock = lck;
      DMFieldRW64Test.lock = lck;
      DMFieldRWLongTest.lock = lck;
      DMFieldRWFloatTest.lock = lck;
      DMFieldRWDoubleTest.lock = lck;
      //DMFieldRWRefTest.lock = lck;
      GcThread.lock = lck;

      GcThread gc = new GcThread();
      DMFieldRWIntTest test1 = new DMFieldRWIntTest();
      DMFieldRW32Test test2 = new DMFieldRW32Test();
      DMFieldRW64Test test3 = new DMFieldRW64Test();
      DMFieldRWLongTest test4 = new DMFieldRWLongTest();
      DMFieldRWFloatTest test5 = new DMFieldRWFloatTest();
      DMFieldRWDoubleTest test6 = new DMFieldRWDoubleTest();
      //DMFieldRWRefTest test7 = new DMFieldRWRefTest();

      gc.setPriority(test1.getPriority() + 1);
      gc.start();
      test1.start();
      test2.start();
      test3.start();
      test4.start();
      test5.start();
      test6.start();
      //test7.start();

      try {
         test1.join();
         test2.join();
         test3.join();
         test4.join();
         test5.join();
         test6.join();
         //test7.join();
      } catch (Exception e) {}

      gc.interrupt();
   }
}
