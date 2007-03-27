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

import com.sun.jump.module.serviceregistry.JUMPServiceRegistryFactory;
import com.sun.jump.module.serviceregistry.JUMPServiceRegistry;

import java.util.Map;
import java.util.HashMap;

import javax.microedition.xlet.ixc.IxcRegistry;
import com.sun.jumpimpl.ixc.JumpExecServiceRegistry; 
import com.sun.jumpimpl.ixc.JumpExecIxcRegistry; 
import com.sun.jumpimpl.ixc.XletContextFactory;

public class ServiceRegistryFactoryImpl extends JUMPServiceRegistryFactory {

   Map initdata;
   HashMap modules = new HashMap(); // IxcRegistry, ServiceModule

   public void load(Map map) {
      initdata = map;

      // If the system property is not set and the value is passed in as 
      // the parameter, set the property.  
      if (System.getProperty("serviceregistry.ixcport") == null) {
         String serverPortNumber = (String) map.get("serviceregistry.ixcport");
         if (serverPortNumber != null) {
            System.setProperty("serviceregistry.ixcport", serverPortNumber); 
         }
      }

      JumpExecIxcRegistry.getRegistry(XletContextFactory.getXletContext(null));
   }

   public void unload() {
   }

   public JUMPServiceRegistry getModule(ClassLoader loader) {
  
      IxcRegistry regis = 
          JumpExecServiceRegistry.getRegistry(XletContextFactory.getXletContext(loader));
   
      return getServiceRegistryModule(regis); 
   }

   private synchronized JUMPServiceRegistry getServiceRegistryModule(IxcRegistry regis) {
      JUMPServiceRegistry registryModule = (JUMPServiceRegistry)modules.get(regis);

      if (registryModule == null) {
           registryModule = new ServiceRegistryImpl(regis);
           registryModule.load(initdata);
           modules.put(regis, registryModule);
      }
      return registryModule;
   }
}
