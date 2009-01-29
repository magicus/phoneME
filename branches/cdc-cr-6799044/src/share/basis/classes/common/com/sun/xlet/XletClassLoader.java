/*
 * @(#)XletClassLoader.java	1.5 06/10/10
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

package com.sun.xlet;

import java.net.URLClassLoader;
import java.net.URL;

import sun.awt.AppContext;

public class XletClassLoader extends URLClassLoader {
   XletManager xletManager;

   protected XletClassLoader(URL[] paths) {
      super(paths, null); 
   }

   void setXletManager(XletManager manager) {
       xletManager = manager;
   }

   AppContext getAppContext() {
      return xletManager.appContext;
   }

   ThreadGroup getThreadGroup() {
      return xletManager.threadGroup;
   }

   /**
    * Override loadClass so we can checkPackageAccess.
    */
   public synchronized Class loadClass(String name, boolean resolve) 
    throws ClassNotFoundException {
           int i = name.lastIndexOf('.');
           if (i != -1) {
           SecurityManager sm = System.getSecurityManager();
           if (sm != null) {
                sm.checkPackageAccess(name.substring(0, i));
           }
       }
       return (super.loadClass(name, resolve));
   }
}
