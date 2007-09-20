/*
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
 */
package com.sun.jump.common;

import junit.framework.*;

import java.util.HashSet;

public class JUMPApplicationTest extends TestCase {

/*
 * Just a simple test to verify equals() and hashcode().
 */
   
   JUMPApplication[] set1; // different app type

   HashSet hash = new HashSet();

   public void setUp() {

      set1 = new JUMPApplication[4];
      set1[0] = new JUMPApplication("app1", null, JUMPAppModel.XLET, 1);
      set1[1] = new JUMPApplication("app1", null, JUMPAppModel.MAIN, 1);
      set1[2] = new JUMPApplication("app1", null, JUMPAppModel.MIDLET, 1);
      set1[3] = new JUMPApplication("app1", null, JUMPAppModel.MIDLET, -1);
   }
  
   public void testSet1_equals() { 
      for (int i = 0; i < (set1.length-1); i++) {
	  assertFalse(set1[i].equals(set1[i+1])); 
      }
   }

   public void testSet1_hash() { 
      hash.clear();
      for (int i = 0; i < set1.length; i++) {
         hash.add(set1[i]);
      }

      assert(hash.size() == set1.length);

      for (int i = 0; i < (set1.length-1); i++) {
         hash.remove(set1[i]);
      }

      assert(hash.size() == 1);
      assert(hash.contains(set1[set1.length-1]));
   }

   public void testSet1_copy() { 
      for (int i = 0; i < set1.length; i++) {
         JUMPApplication app = set1[i];

	 byte[] appArray = app.toByteArray();
   
         JUMPApplication app2 = JUMPApplication.fromByteArray(appArray);
   
         assert(!(app==app2));
         assert(app.equals(app2));
         assert(app.hashCode() == app2.hashCode());
      }
   }

   public void test_bytearray() { 
      try {
         JUMPApplication app = JUMPApplication.fromByteArray(new byte[0]);
	 fail("JUMPApplication created from an empty byte array" + app);
      } catch (IllegalArgumentException e) {
	 //success       
      }
   }
}
