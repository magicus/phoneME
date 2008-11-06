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
 *
 */

import java.rmi.Remote;
import com.sun.jump.common.JUMPApplication;
import com.sun.jump.common.JUMPAppModel;
import com.sun.jump.isolate.jvmprocess.JUMPIsolateProcess;
import com.sun.jump.module.lifecycle.remote.JUMPApplicationLifecycleModuleRemote;
import com.sun.jump.module.lifecycle.remote.JUMPApplicationProxyRemote;

/*
 * A super simple test for the lifecycle module remote.  Need to be launched
 * as a JUMPApplication main type itself.
 *
 * The test launches a pbp DemoXlet through remote interface and then destroy it.
 * One should see the new isolate launch and the destroy message on the console.
 */

public class Test implements Runnable {

   public static void main(String[] args) {
       new Thread(new Test()).start();
   }

   public void run() {

       try {

          // First find the lifecycle module remote.
          Remote remote = JUMPIsolateProcess.getInstance().getRemoteService(
                                  JUMPApplicationLifecycleModuleRemote.class.getName());
   
          System.out.println("Test received " + remote);
   
          JUMPApplicationLifecycleModuleRemote almr = 
   	       (JUMPApplicationLifecycleModuleRemote) remote;

	  // Create an JUMPApplication to launch.
          JUMPApplication app = new JUMPApplication("", null, JUMPAppModel.XLET, 1);
          app.addProperty("XLETApplication_classpath", 
                          System.getProperty("java.home") + "/democlasses.jar");
          app.addProperty("XLETApplication_initialClass", "basis.DemoXlet");

          JUMPApplicationProxyRemote proxy = 
		  almr.launchApplication(app, new String[]{"-d", "basis.demos.ColorDemo"} );

          if (proxy == null) {
              System.out.println("Test: Launching failed, returning"); 
              return;
          } else {
	      System.out.println("Test: AppProxyRemote received, " + proxy.getApplication());
          }

	  Thread.sleep(500);

	  System.out.println("Test: Killing the xlet");

	  proxy.destroyApp();

	  System.out.println("Test: Done with the test, returning");  

      } catch (Exception e) {  
         e.printStackTrace(); 
      }
   }
}

