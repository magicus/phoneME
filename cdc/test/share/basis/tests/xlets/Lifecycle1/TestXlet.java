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


/*
 * Verifies proper lifecycle change by the
 * xlet's request via XletContext methods.
 */

import javax.microedition.xlet.*;
import com.sun.xlet.*;

public class TestXlet implements Xlet, Runnable {
   XletContext context;
   boolean started = false;
   public void initXlet(XletContext context) {
      this.context = context;
   }
   public void startXlet() {
      if (!started) { 
         started = true;
         new Thread(this).start();
      } 
   }
   public void pauseXlet() {
      System.out.println("pauseXlet called");
   }

   public void destroyXlet(boolean b) {
      System.out.println("destroyXlet called");
   }

   public void run() {
      System.out.println("Starting the test");
      try {
         Thread.sleep(500);
      } catch (Exception e) {}
      int state = XletManager.getState(context);
      if (state != 3) { // should be active at this point
         System.out.println("Test failed, expecting 3 (active), received " + state);
         return;
      }

      context.notifyPaused();  // xlet is paused
      try {
         Thread.sleep(500);
      } catch (Exception e) {}
      state = XletManager.getState(context);
      if (state != 2) {  
         System.out.println("Test failed, expecting 2 (paused), received " + state);
         return;
      }

      context.resumeRequest();  // xlet wants to be active
      try {
         Thread.sleep(500);
      } catch (Exception e) {}
      state = XletManager.getState(context);
      if (state != 2 && state != 3) {
         System.out.println("Test failed, expecting 2 (paused) or 3 (active), received " + state);
         return;
      }

      context.notifyDestroyed();  // xlet is destroyed
      try {
         Thread.sleep(500);
      } catch (Exception e) {}
      state = XletManager.getState(context);
      if (state != 4 && state != 0) {
         System.out.println("Test failed, expecting 4 (destroyed) or 0 (unknown), received " + state);
         return;
      }

      context.notifyPaused();  // destroyed xlet can't be paused
      try {
         Thread.sleep(500);
      } catch (Exception e) {}
      state = XletManager.getState(context);
      if (state != 4 && state != 0) {
         System.out.println("Test failed, expecting 4 (destroyed) or 0 (unknown), received " + state);
         return;
      }

      context.resumeRequest();  // destroyed xlet can't be activated
      try {
         Thread.sleep(500);
      } catch (Exception e) {}
      state = XletManager.getState(context);
      if (state != 4 && state != 0) {
         System.out.println("Test failed, expecting 4 (destroyed) or 0 (unknown), received " + state);
         return;
      }

      System.out.println("Test passed");
   }
}
