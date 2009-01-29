/*
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
 */

// Run the xlet that throws XletStateChangeException
// from initXlet().  It should be destroyed immediately.

import com.sun.xlet.*;
public class TestRunner {

   XletLifecycleHandler handler;

   public static void main(String[] args) {
      String[] path;
      if (args == null || args.length == 0) {
         System.out.println("TestRunner - no argument given.");
         System.out.println("Searching for the test xlet in the current directory (\".\")");
         path = new String[]{"."};
      } else {
          path = new String[]{args[0]};
      }

      new TestRunner(path);
   }


   public TestRunner(String[] path) {
      try {
         System.out.println("Starting test");

         handler = XletManager.createXlet("TestXlet", path, null);

         Thread.sleep(500);

         int state = handler.getState();
         if (state != 1) { 
            System.out.println("Test failed, expecting 1 (loaded), received " + state);
            return;
         }

         handler.postInitXlet(); 
         handler.postStartXlet(); 
         Thread.sleep(500);

         System.out.println("After initializing, getting the state");
         state = handler.getState();
         if (state != 4 && state != 0) {
            System.out.println("Test failed, expecting 4 (destroyed) or 0 (unknown), received " + state);
            return;
         }

         System.out.println("Test passed");

      } catch (Exception e) { e.printStackTrace(); }
   }
}
