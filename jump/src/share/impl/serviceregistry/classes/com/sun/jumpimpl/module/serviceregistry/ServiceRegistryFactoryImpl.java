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

package com.sun.jumpimpl.module.serviceregistry;

import com.sun.jump.executive.JUMPExecutive;
import com.sun.jump.module.serviceregistry.JUMPServiceRegistryModuleFactory;
import com.sun.jump.module.serviceregistry.JUMPServiceRegistryModule;

import java.util.Map;
import java.util.HashMap;

import javax.microedition.xlet.ixc.IxcRegistry;

import com.sun.jumpimpl.ixc.XletContextFactory;
import com.sun.jumpimpl.ixc.executive.JUMPExecIxcRegistryWrapper; 
import com.sun.jumpimpl.ixc.executive.JUMPExecIxcRegistry; 

public class ServiceRegistryFactoryImpl extends JUMPServiceRegistryModuleFactory {

   Map initdata;
   JUMPServiceRegistryModule module;

   public void load(Map map) {
      initdata = map;

      JUMPExecIxcRegistry.startExecVMService();
   }

   public void unload() {
   }

   public synchronized JUMPServiceRegistryModule getModule() {
  
      if (module == null)   {
         ClassLoader loader = ClassLoader.getSystemClassLoader();

         IxcRegistry regis = 
             JUMPExecIxcRegistryWrapper.getRegistry(XletContextFactory.getXletContext(loader));
         module = new ServiceRegistryImpl(regis);
      }

      return module;
   }
}
