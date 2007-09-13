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

 
/**
 * A test presentation module that starts basis ColorDemo on the isolate.
 * To test this, one can start the executive with 
 * "--config-file simplepresentation.properties" option. like below.
 *
 * ./bin/cvmc -Xbootclasspath/a:lib/jump.jar -Xserver &
 * ./bin/cvmc -target <serverPID> -command JDETACH -Xbootclasspath/a:lib/executive-jump.jar com.sun.jumpimpl.executive.JUMPExecutiveImpl --config-file simplepresentation.properties
 */

import java.util.Map;
import com.sun.jump.module.presentation.*;
import com.sun.jump.common.JUMPApplication;
import com.sun.jump.common.JUMPAppModel;
import com.sun.jump.executive.JUMPApplicationProxy;
import com.sun.jump.executive.JUMPIsolateProxy;
import com.sun.jump.module.lifecycle.JUMPApplicationLifecycleModule;
import com.sun.jump.module.lifecycle.JUMPApplicationLifecycleModuleFactory;

public class SimplePresentation implements JUMPPresentationModule {
   public void load(Map map) {}
   public void unload() {}
   public void stop() {}

   public void start() {

      // For simplicity, just create a JUMPApp off the fly. 
      JUMPApplication app = new JUMPApplication("", null, JUMPAppModel.MAIN, 1);
      app.addProperty("MAINApplication_classpath", System.getProperty("java.home") + "/democlasses.jar");
      app.addProperty("MAINApplication_initialClass", "basis.DemoFrame");

      // Application argument 
      String[] args = { "-d", "basis.demos.ColorDemo" };  

      // Let's create an isolate

      JUMPApplicationLifecycleModule lcm =
           JUMPApplicationLifecycleModuleFactory.getInstance().getModule(
           JUMPApplicationLifecycleModuleFactory.POLICY_ONE_LIVE_INSTANCE_ONLY);

      System.out.println("*** Isolate trying to launch: " + app.getProperty("MAINApplication_initialClass") + "...");

      // Launch an app in the isolate
      JUMPApplicationProxy appProxy = lcm.launchApplication(app, args);

      if (appProxy != null)
         System.out.println("*** Application  launched");
      else 
         System.out.println("*** Error launching the application");
   }
}
