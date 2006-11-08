/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */

package com.sun.xlet.mvmixc;

import java.io.File;
import java.net.URI;
import java.rmi.*;
import com.sun.appmanager.*;
import com.sun.appmanager.mtask.*;
import com.sun.appmanager.appmodel.XletAppModelController;

public class XletLauncher implements XletLauncherInterface {

   Client client;
 
   public XletLifecycleInterface launchXlet(String name, 
                                 String[] path, String[] args) {

      if (client == null) {
         client = com.sun.appmanager.impl.CDCAmsAppManager.getMtaskClient();
      }

      int mtaskArgsSize = (8 + (args.length == 0 ? 0 : args.length+1));
      String[] mtaskArgs = new String[mtaskArgsSize];

      int index = 0;
      String tmpPath;
      mtaskArgs[index++] = "-cp";
      for (int i = 0; i < path.length; i++) { 
         tmpPath = path[i];
         try {
            // if the path is in "file:" URL format, convert it.
            tmpPath = new File(new URI(tmpPath)).getAbsolutePath();
         } catch (Exception e) {}  // Ignore.

         if (i == 0) 
            mtaskArgs[index] = tmpPath; 
         else 
            mtaskArgs[index] += File.pathSeparator + tmpPath;
      }
      index++;
      mtaskArgs[index++] = "sun.mtask.xlet.PXletRunner";
      mtaskArgs[index++] = "-loadOnly";
      mtaskArgs[index++] = "-name";
      mtaskArgs[index++] = name;
      mtaskArgs[index++] = "-path";
      mtaskArgs[index++] = mtaskArgs[1];
      if (args.length > 0) {
         mtaskArgs[index++] = "-args";
         for (int i = 0; i < args.length; i++) { 
            mtaskArgs[index++] = args[i];
         }
      }

      String pid = client.launch("JXLET", mtaskArgs); 

      XletLifecycleController controller = 
         new XletLifecycleController(pid, client);

      for (int i = 0; i < 10; i++) {
         try {
            Thread.sleep(1000);
            if (controller.getState() == XletAppModelController.XLET_LOADED) {
		return controller;
            }
         } catch (Exception e) {}
      }

      return null;
   }
}
