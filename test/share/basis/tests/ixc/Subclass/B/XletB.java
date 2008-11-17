/*
 * @(#)XletB.java	1.3 06/10/10
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

import javax.microedition.xlet.*;
import javax.microedition.xlet.ixc.*;

public class XletB implements Xlet{
    IxcRegistry regis;
    XletContext context;
    public XletB() {}
    public void initXlet(XletContext context) {
       regis = IxcRegistry.getRegistry(context);
       this.context = context;
    }
    public void startXlet() {
       try {
          Thread.sleep(100);
          System.out.println("XletB starting ixc");
          String[] s = regis.list();
          for (int i = 0; i < s.length; i++) {
             System.out.println("XletB found method " + s[i]);
          }
          for (int i = 0; i < s.length; i++) {
             MyRemote r = (MyRemote) regis.lookup(s[i]);
             String str = r.getString();
             if (!s[i].equals(str)) {
                System.out.println("FAILED");
                return;
             } else {
                System.out.println(s[i] + " invocation successful");
             }
          }
          System.out.println("Test OK.");
       } catch (Exception e) { e.printStackTrace(); }
       context.notifyDestroyed();
    }
    public void pauseXlet() {
    }
    public void destroyXlet(boolean isforced) {}
}
