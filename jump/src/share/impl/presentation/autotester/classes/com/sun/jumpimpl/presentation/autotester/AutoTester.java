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

package com.sun.jumpimpl.presentation.autotester;

import com.sun.jump.command.JUMPIsolateLifecycleRequest;
import com.sun.jump.common.JUMPApplication;
import com.sun.jump.common.JUMPContent;
import com.sun.jump.common.JUMPAppModel;
import com.sun.jump.executive.JUMPApplicationProxy;
import com.sun.jump.executive.JUMPExecutive;
import com.sun.jump.executive.JUMPIsolateFactory;
import com.sun.jump.executive.JUMPIsolateProxy;
import com.sun.jump.message.JUMPMessage;
import com.sun.jump.message.JUMPMessageDispatcher;
import com.sun.jump.message.JUMPMessageDispatcherTypeException;
import com.sun.jump.message.JUMPMessageHandler;
import com.sun.jump.module.installer.JUMPInstallerModule;
import com.sun.jump.module.installer.JUMPInstallerModuleFactory;
import com.sun.jump.module.download.JUMPDownloadDescriptor;
import com.sun.jump.module.download.JUMPDownloader;
import com.sun.jump.module.download.JUMPDownloadModule;
import com.sun.jump.module.download.JUMPDownloadModuleFactory;
import com.sun.jump.module.presentation.JUMPPresentationModule;

import com.sun.jumpimpl.module.download.DownloadDestinationImpl;

import java.net.URL;
import java.util.Map;

/**
 * A simple JUMP launcher for auto testing.
 */
public class AutoTester implements JUMPPresentationModule, JUMPMessageHandler {
    
    String nextDescriptor = null;
    static final String DESCRIPTOR_KEY  = "jump.descriptor.url";

    int currentIsolateId;

    public void load(Map map) {
       nextDescriptor = (String) System.getProperty(DESCRIPTOR_KEY);
       if (nextDescriptor == null)
          nextDescriptor = (String) map.get(DESCRIPTOR_KEY);
    }
    
    public void stop() {}
    
    public void unload() {}
    
    public void start() {

        System.err.println("*** Starting AutoTester ***");

	if (nextDescriptor == null) {
             System.err.println(DESCRIPTOR_KEY + " value not found, exiting");
	     return;
	} 

	installAndPerformTests(nextDescriptor);
        
        // shut down the server
        new com.sun.jumpimpl.os.JUMPOSInterfaceImpl().shutdownServer();
        System.exit(0);
    }

    /**
     * Installs/Updates a test suite, runs the first MIDlets in the suite,
     * uninstall the suite, and install the next suite, until there is no more midlet 
     * suite and HTTP 404 not found error is thrown during the download time.
     */
    public void installAndPerformTests(String descriptorUrl) {

        JUMPExecutive executive = JUMPExecutive.getInstance();

	JUMPDownloadModule downloadModule;
	JUMPInstallerModule installerModule;

        JUMPIsolateFactory lcm = executive.getIsolateFactory();

	// Get the download and install modules based on the URL
	if (descriptorUrl.trim().toLowerCase().endsWith(".jad"))  {
            downloadModule = JUMPDownloadModuleFactory.getInstance().getModule(JUMPDownloadModuleFactory.PROTOCOL_MIDP_OTA);
            installerModule = JUMPInstallerModuleFactory.getInstance().getModule(JUMPAppModel.MIDLET);
	} else {
            downloadModule = JUMPDownloadModuleFactory.getInstance().getModule(JUMPDownloadModuleFactory.PROTOCOL_OMA_OTA);
            installerModule = JUMPInstallerModuleFactory.getInstance().getModule(JUMPAppModel.XLET);
        } 

        try {

	   // Start the message dispatcher first, for the lifecycle events.
           JUMPMessageDispatcher md = executive.getMessageDispatcher();
           md.registerHandler(JUMPIsolateLifecycleRequest.MESSAGE_TYPE, this); 

           while (true) { // Exits when the exception happens.

              JUMPDownloadDescriptor descriptor = 
                  downloadModule.createDescriptor(descriptorUrl);
      	      JUMPDownloader downloader = 
      	          downloadModule.createDownloader(descriptor);
      
      	      // downloader.start is synchronous right now
      	      URL contentUrl = downloader.start(new DownloadDestinationImpl(descriptor));
      	      JUMPContent[] contents =  installerModule.install(contentUrl, descriptor);
      
      	      if (contents == null || contents.length == 0) {
      		   // Problem with the installation, break.
      		   break;
              }            
      
              JUMPApplication app = (JUMPApplication)contents[0];

	      synchronized(this) { 

                 // Launch the isolate and record the isolate id	
                 JUMPIsolateProxy ip = lcm.newIsolate(app.getAppType());
		 setLaunchedIsolateId(ip.getIsolateId());

		 // Launch the app inside the newly created isolate
                 System.err.println("*** Isolate " + ip.getIsolateId() + " trying to launch: " + app.getTitle() + "...");
                 JUMPApplicationProxy proxy = ip.startApp(app, null);
     
                 if (proxy == null) { 
 		  // failure, uninstall the bundle and exit.
                    System.err.println("Launching failed " + app);
                    installerModule.uninstall(app);
		    return;
                 } 
             
                 trace("*** Launch of " + app.getTitle() + " successed");
		   
		 // Launch succeeded.
		 // Wait for the handleMessage() to deliver the death notification.
		 wait();  
              }
	      // All contents from the bundle is executed, uninstall.
              installerModule.uninstall(contents[0]);
           }

        } catch (Exception e) { 
  	   e.printStackTrace(); 
	}
    }

    /**
     * A message handling routine.
     * Waits for the isolate destroyed notification, and if the message
     * is about the isolate we're waiting for, notify the blocking 
     * thread in start() method.
     */
    public void handleMessage(JUMPMessage message) {
        if (JUMPIsolateLifecycleRequest.MESSAGE_TYPE.equals(message.getType())) {
	   trace("==== MESSAGE RECEIVED: JUMPIsolateLifecycleRequest");
	   JUMPIsolateLifecycleRequest cmd = 
  	      (JUMPIsolateLifecycleRequest)JUMPIsolateLifecycleRequest.fromMessage(message);
	   if (JUMPIsolateLifecycleRequest.ID_ISOLATE_DESTROYED.equals
	                  (cmd.getCommandId())) {

              synchronized(this) {
                  if (getLaunchedIsolateId() == cmd.getIsolateId())
                     notify(); 
              }
           }
        }
    }

    public void setLaunchedIsolateId(int isolateId) {
        currentIsolateId = isolateId;
    }

    public int getLaunchedIsolateId() {
        return currentIsolateId;
    }

    void trace(String str) {
        if (false) {
            System.out.println(str);
        }
    }
}
