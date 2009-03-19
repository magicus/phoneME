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
import java.awt.Panel;
import java.awt.Frame;

import sun.awt.AppContext;
import sun.awt.SunToolkit;

public class XletContainer extends Panel {

    public Container getParent() {
       Container c = super.getParent();
       if (AppContext.getAppContext() != SunToolkit.targetToAppContext(c)) {
          ClassLoader loader = sun.misc.CVM.getCallerClass(1).getClassLoader();
          if (loader != null) { // call not by the system library
             return null;
          }
       }
       return c;
    }

    public Container getFocusCycleRootAncestor () {
       Container c = super.getFocusCycleRootAncestor();
       if (AppContext.getAppContext() != SunToolkit.targetToAppContext(c)) {
          return null;
       }
       return c;
    }
                                                                              
    public boolean isFocusCycleRoot() {
       return true;
    }
}
