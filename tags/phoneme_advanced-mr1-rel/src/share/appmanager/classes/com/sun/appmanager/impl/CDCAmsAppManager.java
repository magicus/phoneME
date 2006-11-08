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

package com.sun.appmanager.impl;

import com.sun.appmanager.AppManager;

import com.sun.appmanager.mtask.Client;
import com.sun.appmanager.mtask.TaskListener;
import com.sun.appmanager.presentation.PresentationMode;
import com.sun.appmanager.impl.ota.CDCAmsOTAFactory;
import com.sun.appmanager.impl.preferences.CDCAmsPreferences;
import com.sun.appmanager.impl.store.CDCAmsPersistentStore;
import com.sun.appmanager.impl.apprepository.CDCAmsAppRepository;

import com.sun.xlet.mvmixc.CDCAmsXletContext;
import com.sun.xlet.mvmixc.CDCAmsIxcRegistry;
import com.sun.xlet.mvmixc.ServiceRegistry;
import com.sun.xlet.mvmixc.XletLauncher;
import javax.microedition.xlet.XletContext;
import javax.microedition.xlet.ixc.IxcRegistry;

public class CDCAmsAppManager extends AppManager {

    private final String fileSeparator = System.getProperty("file.separator");
    private static CDCAmsAppController controller = null;
    private static boolean doWarmup = true;
    private static boolean needLauncher = false;

    public CDCAmsAppManager(Client mtaskClient) {
        this.mtaskClient = mtaskClient;
    }

    public static void main(String[] args) {
        Client mtaskClient = null;
        int serverPort = -1;
        String serverName = null;

        for (int i = 0; i < args.length; i++) {
            if (args[i].equals("-port")) {
                String portString = args[++i];

                try {
                    serverPort = Integer.parseInt(portString);
                }
                catch (NumberFormatException ex) {
                }
                if (serverPort == -1) {
                    System.err.println("Invalid port: \"" + portString + "\"");
                    System.exit(1);
                }
            } else if (args[i].equals("-server")) {
                serverName = args[++i];
            } else if (args[i].equals("-nowarmup")) {
		doWarmup = false;
            } else if (args[i].equals("-addLauncher")) {
                needLauncher = true;
            }
        }

        if (serverName != null) {
            if (serverPort == -1) {
                mtaskClient = new Client(serverName);
            }
            else {
                mtaskClient = new Client(serverName, serverPort);
            }
        }
        else {
            if (serverPort == -1) {
                mtaskClient = new Client();
            }
            else {
                mtaskClient = new Client(serverPort);
            }
        }

        // Create AppManager on the default monitor
        CDCAmsAppManager appmanager =
            new CDCAmsAppManager(mtaskClient);

	appmanager.initialize();

    }

    private void setupCDCAmsHome()
    {
	String cdcamsHome = System.getProperty("cdcams.home");
        if (cdcamsHome == null) {
	    cdcamsHome = System.getProperty("java.home");
	    System.setProperty("cdcams.home", cdcamsHome);
	}
    }

    private void setupSecurity()
    {
	//
	// Before installing the security manager, set the right policy
	//
        String cdcamsHome = System.getProperty("cdcams.home");
	if (cdcamsHome == null) {
	    throw new RuntimeException("Need cdcams.home");
	}

	String policy = cdcamsHome + fileSeparator + "lib" + fileSeparator + "security" + fileSeparator + "appmanager.security.permissive";
	System.setProperty("java.security.policy", policy);

	// Now we install a security manager.
	System.setSecurityManager(new SecurityManager());
    }

    private void initialize()
    {
	// Home
	setupCDCAmsHome();

	// First off, security
	setupSecurity();

	// Start with persistent store.
	this.persistentStore = new CDCAmsPersistentStore();

        this.controller = new CDCAmsAppController(mtaskClient);

        // Add a lifecycle listener for the controller
        mtaskClient.addListener((TaskListener)this.controller);

        this.otaFactory = new CDCAmsOTAFactory();
        this.appRepository = new CDCAmsAppRepository();
        this.preferences = new CDCAmsPreferences();
        XletContext context = new CDCAmsXletContext();
        IxcRegistry mainRegistry  = CDCAmsIxcRegistry.getRegistry(context);
	this.registry = ServiceRegistry.getRegistry(context);

        presentationMode = PresentationMode.createPresentation();
        presentationMode.initialize();

        LoadAppsThread demoLoader = new LoadAppsThread();

        demoLoader.start();

        try {
            demoLoader.join();
        }
        catch (Exception e) {
            e.printStackTrace();
        }

        if (needLauncher) {
           try {
               this.registry.bind("cdcams/XletLauncher", new XletLauncher());
           } catch (Exception e) { e.printStackTrace(); }
        }

        // Add a lifecycle listener for the ixc implementation
        TaskListener listener = (TaskListener)mainRegistry;
        mtaskClient.addListener(listener);

     
        // 6349239 This method may never return!
        presentationMode.startAppManager();
    }

    public static CDCAmsAppController getAppController() {
        return controller;
    }

    private void startWarmupLists() {
        //
        // And warmup MVM server
        // Note that the paths here are relative to the server's invocation
        // location, and not the AppManager!!
        //
	if (doWarmup) {
	    mtaskClient.warmupLists(null,
		persistentStore.absolutePathOf("profiles/classesList.txt"),
		persistentStore.absolutePathOf("profiles/methodsList.txt"));
	}
    }

    /**
     * Loads all known apps
     */
    private void loadApps() {
        presentationMode.loadApps();
    }

    /*
     * Load and run the apps that must be run upon
     * reboot of the system
     */
    private void runStartupApps() {
        presentationMode.runStartupApps();
    }

    class LoadAppsThread
        extends Thread {

        public LoadAppsThread() {
        }

        public void run() {
            startWarmupLists();
            loadApps();
            runStartupApps();
        }

    }

}
