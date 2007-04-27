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

package com.sun.jumpimpl.module.lifecycle;

import com.sun.jump.module.lifecycle.JUMPApplicationLifecycleModule;
import com.sun.jump.module.lifecycle.JUMPApplicationLifecycleModuleFactory;
import com.sun.jumpimpl.module.lifecycle.remote.ApplicationLifecycleModuleRemoteImpl;
import java.util.Map;

public class ApplicationLifecycleModuleFactoryImpl extends JUMPApplicationLifecycleModuleFactory {
 
   private JUMPApplicationLifecycleModule oneLiveOnly;
   private Map config;
   
   public void load(Map config) {
       this.config = config;

       boolean shouldExport = 
	  new Boolean((String)config.get("lifecycle.remote.export")).booleanValue();

       if (shouldExport) {
	  JUMPApplicationLifecycleModule module = 
	       getModule(POLICY_ONE_LIVE_INSTANCE_ONLY);

	  new ApplicationLifecycleModuleRemoteImpl(module);
       }
   }

   public void unload() {
       synchronized(this) {  
           if (oneLiveOnly != null) { 
              oneLiveOnly.unload();
           }
       }
   }

   private synchronized JUMPApplicationLifecycleModule getOneLiveOnly() {
       if (oneLiveOnly == null) {
           oneLiveOnly = new OneLiveOnlyApplicationLifecycleModule();
           oneLiveOnly.load(config); 
       }
       return oneLiveOnly;
   }

   public JUMPApplicationLifecycleModule getModule(String policy) {
        if (policy.equals(JUMPApplicationLifecycleModuleFactory.POLICY_ONE_LIVE_INSTANCE_ONLY))
	    return getOneLiveOnly();
return null;
   }
}
