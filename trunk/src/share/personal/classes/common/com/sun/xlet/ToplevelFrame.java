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


package com.sun.xlet;

import java.awt.Container;
import java.awt.Frame;
import java.awt.FlowLayout;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.util.Enumeration;

public class ToplevelFrame extends Frame {

   // No real reason, some random values.
   int width  = 400;
   int height = 300;

   public ToplevelFrame(String title, XletManager manager) {
      super(title);
      setSize(width, height);
      addWindowListener(new XletFrameListener());

      addNotify(); // Do we need this for pp?
   }

   class XletFrameListener extends WindowAdapter {
      public void windowClosing(WindowEvent e) {
         XletManager.exit();
      }
      public void windowIconified(WindowEvent e) {
         for (Enumeration enu = XletManager.activeContexts.elements();
              enu.hasMoreElements(); ) {
            XletLifecycleHandler handler = 
                (XletLifecycleHandler) enu.nextElement();
            handler.postPauseXlet();
         }
      }
      public void windowDeiconified(WindowEvent e) {
         for (Enumeration enu = XletManager.activeContexts.elements();
              enu.hasMoreElements(); ) {
            XletLifecycleHandler handler = 
                (XletLifecycleHandler) enu.nextElement();
            handler.postStartXlet();
         }
      }
   }

   // A request from XletContextImpl to create a new Container.
   public Container getXletContainer() {
       XletContainer c = new XletContainer();
       if (getComponentCount() > 0) {
          setLayout(new FlowLayout());  // If more than one xlet, FlowLayout
       }
       add(c);
       validate();
       setVisible(true);
       c.setVisible(false);
       return c;
   }
}
