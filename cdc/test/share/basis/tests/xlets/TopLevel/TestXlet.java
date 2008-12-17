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

//
// 6208460
// Checks for the accessibility of the toplevel frame
//

import java.awt.Container;
import javax.microedition.xlet.*;

public class TestXlet implements Xlet {
   XletContext context;
   public void initXlet(XletContext context) {
      System.out.println("Checking for the accessibility of the toplevel frame");
      this.context = context;
      try {
         Container c = context.getContainer();
         int i = 0; 
         while (c != null) { 
            System.out.println((i++) + ":" + c);
            if (c instanceof java.awt.Window) {
               System.out.println("Warning: This xlet can access the toplevel!");
            }
            c = c.getParent(); 
         }

         c = context.getContainer();
         i = 0; 
         while (c != null) { 
            System.out.println((i++) + ":" + c.getFocusCycleRootAncestor());
            if (c.getFocusCycleRootAncestor() instanceof java.awt.Window) {
               System.out.println("Warning: This xlet can access the toplevel!");
            }
            c = c.getParent(); 
         }
         
      } catch (Exception e) { e.printStackTrace(); }

      System.out.println("done.");
   }
   public void startXlet() {}
   public void pauseXlet() {}
   public void destroyXlet(boolean b) {}
}
