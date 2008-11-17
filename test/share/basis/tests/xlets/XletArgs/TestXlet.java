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

// CR 4840254 
//    XletContext.getXletProperty(ARGS) return null instead of 0 length array  

import javax.microedition.xlet.*;
import com.sun.xlet.*;

public class TestXlet implements Xlet {
   XletContext context;
   public void initXlet(XletContext context) {
      this.context = context;
      String[] args = (String[])context.getXletProperty(XletContext.ARGS);
      if (args == null)
         System.out.println("FAILED!, got null array");
      else if (args.length == 0) 
         System.out.println("Test passed");
      else 
         System.out.println("Test failed (don't pass in any args to this test xlet)");
   }
   public void startXlet() {}
   public void pauseXlet() {}
   public void destroyXlet(boolean b) {}
}
