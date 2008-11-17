/*
 * @(#)BarrierTest.java	1.6 06/10/10
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

public class BarrierTest {

  // The string array here is three cards long. This assures that no
  // matter how the cards fall in memory, the middle element of this array
  // will have no other elements but testStringArray elements around it.

   private String[] testStringArray = new String[128 * 3];
   private int midpoint = 128 * 3 / 2;
   public static final int CARD_DIRTY_BYTE = 0;
   public static final int CARD_CLEAN_BYTE = 1;
   public static final int CARD_SUMMARIZED_BYTE = 2;

   public BarrierTest() {
   }

   public native int checkCardMarking();
   public static native void doYoungGC();

   private void setString() {
      testStringArray[midpoint] = null;
   }

   private void setString(String str) {
      testStringArray[midpoint] = str;
   }

   public String getString() {
      String str = testStringArray[midpoint];
      return str;
   }

   public static void main(String args[]) {
      Random rd = new Random();
      BarrierTest test = new BarrierTest();
      String str = String.valueOf(rd.nextInt());

      System.out.println();

      //Case 1: Before setting the field, card should be clean
      //
      // Age 'test' so that it ends up in the old generation.
      //
      doYoungGC();
      doYoungGC();
      doYoungGC();
      doYoungGC();
      if(test.checkCardMarking() == CARD_CLEAN_BYTE)
         System.out.println("PASS: Card is clean before setting the object field");
      else
         System.out.println("FAIL: Card is not clean before setting the object field");


      //Case 2: After setting the field, card should be dirty
      test.setString(str);
      if(test.checkCardMarking() == CARD_DIRTY_BYTE)
         System.out.println("PASS: Card is dirty after setting the object field");
      else
         System.out.println("FAIL: Card is not dirty after setting the object field");


      //Case 3: Card should be summarized after GC
      //Create a old->young pointer to keep the card "unclean".
      test.setString(new String("test"));
      doYoungGC();
      if(test.checkCardMarking() == CARD_SUMMARIZED_BYTE)
         System.out.println("PASS: Card is summarized after GC");
      else
         System.out.println("FAIL: Card is not summarized after GC");


      //Case 4: Card should be clean after GC
      doYoungGC();
      doYoungGC();
      if(test.checkCardMarking() == CARD_CLEAN_BYTE)
         System.out.println("PASS: Card is clean after GC");
      else
         System.out.println("FAIL: Card is not clean after GC");


      //Case 5: doYoungGC, after reference to the string object is
      // removed and the card should be clean.
      test.setString();
      doYoungGC();
      if(test.checkCardMarking() == CARD_CLEAN_BYTE)
         System.out.println("PASS: Card is clean with GC after reference to the object is removed");
      else
         System.out.println("FAIL: Card is not clean with GC after reference to the object is removed");
   }
}
