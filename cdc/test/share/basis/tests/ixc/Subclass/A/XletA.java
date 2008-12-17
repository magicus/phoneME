/*
 * @(#)XletA.java	1.3 06/10/10
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

import javax.microedition.xlet.ixc.*;
import javax.microedition.xlet.*;

public class XletA implements Xlet {
    IxcRegistry regis;

    public void initXlet(XletContext context) {
       regis = IxcRegistry.getRegistry(context);
    }

    public void startXlet() {
       try {
          MyRemote r = new A2();
          System.out.println("XletA binding objects");
          regis.bind("A", r);
          regis.bind("B",new B2());
          System.out.println("XletA finished binding");
       } catch (Exception e) { e.printStackTrace(); }
    }

    public void pauseXlet() {}
    public void destroyXlet(boolean isforced) {}

    class A2 extends A {}
    class A implements MyRemote { 
       public String getString() throws java.rmi.RemoteException { 
          return "A"; 
       }
    }
    class B2 extends B {}
    class B implements MyRemote{
       public String getString() throws java.rmi.RemoteException { 
          return "B"; 
       }
    }

}
