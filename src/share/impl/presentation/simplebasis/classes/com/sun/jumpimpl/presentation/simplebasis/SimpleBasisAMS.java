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

package com.sun.jumpimpl.presentation.simplebasis;

import com.sun.jump.command.JUMPIsolateLifecycleRequest;
import com.sun.jump.command.JUMPIsolateWindowRequest;
import com.sun.jump.common.JUMPWindow;
import com.sun.jump.executive.JUMPExecutive;
import com.sun.jump.message.JUMPMessageDispatcher;
import com.sun.jump.message.JUMPMessageDispatcherTypeException;
import com.sun.jump.module.lifecycle.JUMPApplicationLifecycleModule;
import com.sun.jump.module.lifecycle.JUMPApplicationLifecycleModuleFactory;
import com.sun.jump.module.presentation.JUMPPresentationModule;
import com.sun.jump.common.JUMPApplication;
import com.sun.jump.common.JUMPContent;
import com.sun.jump.executive.JUMPApplicationProxy;
import com.sun.jump.executive.JUMPIsolateProxy;
import com.sun.jump.message.JUMPMessage;
import com.sun.jump.message.JUMPMessageHandler;
import com.sun.jump.module.installer.JUMPInstallerModule;
import com.sun.jump.module.installer.JUMPInstallerModuleFactory;
import com.sun.jump.module.isolatemanager.JUMPIsolateManagerModule;
import com.sun.jump.module.isolatemanager.JUMPIsolateManagerModuleFactory;
import com.sun.jump.module.windowing.JUMPWindowingModule;
import com.sun.jump.module.windowing.JUMPWindowingModuleFactory;
import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Container;
import java.awt.Dimension;
import java.awt.Frame;
import java.awt.GridLayout;
import java.awt.Image;
import java.awt.Toolkit;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.IOException;
import java.net.URL;
import java.util.Map;
import java.util.Vector;

/**
 * A simple JUMP launcher that uses Personal Basis components.
 */
public class SimpleBasisAMS implements JUMPPresentationModule, JUMPMessageHandler {
    
    private Frame frame = null;
    private Container commandContainer = null;
    private Container screenContainer = null;
    private JUMPWindowingModuleFactory wmf = null;
    private JUMPWindowingModule wm = null;
    private JUMPApplicationLifecycleModuleFactory almf = null;
    private JUMPApplicationLifecycleModule alm = null;
    private JUMPIsolateManagerModuleFactory lcmf = null;
    private JUMPIsolateManagerModule lcm = null;
    private JUMPApplication currentApp = null;
    private Object timeoutObject = null;
    private boolean appWindowDisplayState = false;
    private static final int TIMEOUT_VAL = 2000;
    protected static final int MAX_TITLE_CHARS = 15;
    private boolean verbose = false;
    /**
     * Creates a new instance of SimpleBasisAMS
     */
    public SimpleBasisAMS() {
    }
    
    /**
     * load the presentation module
     * @param map the configuration data required for loading this module.
     */
    public void load(Map map) {
        // check if verbose mode is used
        String verboseStr = System.getProperty("jump.presentation.verbose");
        if (verboseStr == null && map != null) {
            verboseStr = (String) map.get("jump.presentation.verbose");
        }
        if (verboseStr != null && verboseStr.toLowerCase().equals("true")) {
            verbose = true;
        }
    }
    
    public void stop() {
    }
    
    public void unload() {
    }
    
    private boolean setup() {
        frame = new Frame();
        frame.setLayout(new BorderLayout());
        
        commandContainer = new Container() {
            public Dimension getPreferredSize() {
                return new Dimension(480, 50);
            }
        };
        
        commandContainer.setLayout(new GridLayout(1, 3));
        addCommandButton("Apps", new ApplicationsScreenActionListener());
        addCommandButton("Switch", new SwitchToScreenActionListener());
        addCommandButton("Kill", new KillScreenActionListener());
        addCommandButton("Exit", new ExitActionListener());
        
        screenContainer = new Container();
        screenContainer.setLayout(new GridLayout(0, 2));
        
        frame.add(commandContainer, BorderLayout.NORTH);
        frame.add(screenContainer, BorderLayout.CENTER);
        
        wmf = JUMPWindowingModuleFactory.getInstance();
        wm = wmf.getModule();
        
        almf = JUMPApplicationLifecycleModuleFactory.getInstance();
        alm = almf.getModule(JUMPApplicationLifecycleModuleFactory.POLICY_ONE_LIVE_INSTANCE_ONLY);
        
        lcmf = JUMPIsolateManagerModuleFactory.getInstance();
        lcm = lcmf.getModule();
        
        JUMPExecutive e = JUMPExecutive.getInstance();
        JUMPMessageDispatcher md = e.getMessageDispatcher();
        try {
            md.registerHandler(JUMPIsolateWindowRequest.MESSAGE_TYPE, this);
            md.registerHandler(JUMPIsolateLifecycleRequest.MESSAGE_TYPE, this);
        } catch (JUMPMessageDispatcherTypeException ex) {
            ex.printStackTrace();
        } catch (IOException ex) {
            ex.printStackTrace();
        }
        
        return true;
    }
    
    class LaunchThread extends Thread {
        
        JUMPApplication app = null;
        
        public LaunchThread(JUMPApplication app) {
            this.app = app;
        }
        public void run() {
            timeoutObject = new Object();
            launchApp(app);
        }
    }
    
    public void handleMessage(JUMPMessage message) {
        if (JUMPIsolateWindowRequest.MESSAGE_TYPE.equals(message.getType())) {
            trace("==== MESSAGE RECEIVED: JUMPIsolateWindowRequest.MESSAGE_TYPE");
            JUMPIsolateWindowRequest cmd =
                    (JUMPIsolateWindowRequest)
                    JUMPIsolateWindowRequest.fromMessage(message);
            
            int isolateID = cmd.getIsolateId();
            int windowID = cmd.getWindowId();
            JUMPWindow window = wm.idToWindow(isolateID);
            if (window != null) {
                JUMPApplication app = window.getApplication();
            }
            if (JUMPIsolateWindowRequest.ID_REQUEST_FOREGROUND.equals
                    (cmd.getCommandId())) {
                trace("====== COMMAND RECEIVED: JUMPIsolateWindowRequest.ID_REQUEST_FOREGROUND");
                appWindowDisplayState = true;
                synchronized(timeoutObject) {
                    System.out.println("****** Calling notify() on timout object. ******");
                    timeoutObject.notify();
                }
            } else if(JUMPIsolateWindowRequest.ID_REQUEST_BACKGROUND.equals(
                    cmd.getCommandId())) {
                trace("====== COMMAND RECEIVED: JUMPIsolateWindowRequest.ID_REQUEST_BACKGROUND");
            }
        } else if(JUMPIsolateLifecycleRequest.MESSAGE_TYPE.equals(
                message.getType())) {
            trace("==== MESSAGE RECEIVED: JUMPIsolateLifecycleRequest.MESSAGE_TYPE");
            JUMPIsolateLifecycleRequest cmd =
                    (JUMPIsolateLifecycleRequest)
                    JUMPIsolateLifecycleRequest.fromMessage(message);
            
            int isolateID = cmd.getIsolateId();
            int appID = cmd.getAppId();
            JUMPWindow window = wm.idToWindow(isolateID);
            JUMPApplication app = null;
            if (window != null) {
                app = window.getApplication();
            }
            
            if (JUMPIsolateLifecycleRequest.ID_ISOLATE_DESTROYED.equals
                    (cmd.getCommandId())) {
                
                trace("====== COMMAND RECEIVED: JUMPIsolateLifecycleRequest.ID_ISOLATE_DESTROYED");
                
                // the killApp(app) call below may not be needed and needs
                // to undergo testing to see if this is necessary.  In fact,
                // it may be the case that the 'app' value returned by
                // window application is null.  If killApp(app) is not needed,
                // then it should be replaced with a "currentApp = null".
                killApp(app);
            }
        }
    }
    
    /**
     * Display the screen containing application icons.
     */
    public void doApplicationsScreen() {
        trace("ENTERING SCREEN: APPLICATIONS");
        if (currentApp != null) {
            pauseApp(currentApp);
            bringWindowToBack(currentApp);
        }
        clearScreen();
        JUMPApplication apps[] = getInstalledApps();
        for (int i = 0; i < apps.length; i++) {
            addScreenButton(apps[i], new LaunchAppActionListener(apps[i]), Color.yellow);
        }
        refreshScreen();
    }
    
    /**
     * Display the switch-to screen, consisting of icons
     * pertaining to currently running applications.
     */
    public void doSwitchToScreen() {
        trace("ENTERING SCREEN: SWITCH");
        if (currentApp == null) {
            trace("***currentApp is NULL.***");
        } else {
            trace("*** currentApp -> " + currentApp.getTitle() + " ***");
        }
        if (currentApp != null) {
            pauseApp(currentApp);
            bringWindowToBack(currentApp);
        }
        clearScreen();
        JUMPApplication apps[] = getRunningApps();
        for (int i = 0; i < apps.length; i++) {
            addScreenButton(apps[i], new SwitchToActionListener(apps[i]), Color.green);
        }
        refreshScreen();
    }
    
    /**
     * Display the kill screen, consisting of icons pertaining to currently
     * running applications that can be killed .
     */
    public void doKillScreen() {
        trace("ENTERING SCREEN: KILL");
        if (currentApp != null) {
            pauseApp(currentApp);
            bringWindowToBack(currentApp);
        }
        clearScreen();
        JUMPApplication apps[] = getRunningApps();
        for (int i = 0; i < apps.length; i++) {
            addScreenButton(apps[i], new KillActionListener(apps[i]), Color.red);
        }
        refreshScreen();
    }
    
    class ApplicationsScreenActionListener
            implements ActionListener {
        
        public ApplicationsScreenActionListener() {
        }
        
        public void actionPerformed(ActionEvent e) {
            doApplicationsScreen();
        }
    }
    
    class SwitchToScreenActionListener
            implements ActionListener {
        
        public SwitchToScreenActionListener() {
        }
        
        public void actionPerformed(ActionEvent e) {
            doSwitchToScreen();
        }
    }
    
    class KillScreenActionListener
            implements ActionListener {
        
        public KillScreenActionListener() {
        }
        
        public void actionPerformed(ActionEvent e) {
            doKillScreen();
        }
    }
    
    class LaunchAppActionListener
            implements ActionListener {
        JUMPApplication app;
        
        public LaunchAppActionListener(JUMPApplication app) {
            this.app = app;
        }
        
        public void actionPerformed(ActionEvent e) {
            Thread launchThread = new LaunchThread(app);
            launchThread.start();
        }
    }
    
    class SwitchToActionListener
            implements ActionListener {
        JUMPApplication app;
        
        public SwitchToActionListener(JUMPApplication app) {
            this.app = app;
        }
        
        public void actionPerformed(ActionEvent e) {
            switchToApp(app);
        }
    }
    
    class KillActionListener
            implements ActionListener {
        JUMPApplication app;
        
        public KillActionListener(JUMPApplication app) {
            this.app = app;
        }
        
        public void actionPerformed(ActionEvent e) {
            killApp(app);
        }
    }
    
    class ExitActionListener
            implements ActionListener {
        
        public void actionPerformed(ActionEvent e) {
            killAllApps();
            System.exit(0);
        }
    }
    
    private Image getIconImage(JUMPApplication app) {
        URL iconPath = app.getIconPath();
        if (iconPath != null) {
            return Toolkit.getDefaultToolkit().createImage(iconPath);
        } else {
            return null;
        }
    }
    
    private SimpleBasisAMSImageButton addCommandButton(String label, ActionListener action) {
        
        SimpleBasisAMSImageButton button = new SimpleBasisAMSImageButton();
        if (button == null) {
            return null;
        }
        button.addActionListener(action);
        button.setEnabled(true);
        button.setForeground(Color.blue);
        button.setTextShadow(true);
        button.setPaintBorders(true);
        
        String str = null;
        if (label.trim().length() > MAX_TITLE_CHARS) {
            str = label.trim().substring(0, MAX_TITLE_CHARS - 3);
            str += "...";
        } else {
            str = label.trim();
        }
        button.setLabel(str);
        commandContainer.add(button);
        return button;
    }
    
    private void clearScreen() {
        screenContainer.removeAll();
    }
    
    private void refreshScreen() {
        screenContainer.validate();
        screenContainer.repaint();
    }
    
    private SimpleBasisAMSImageButton addScreenButton(JUMPApplication app, ActionListener action, Color color) {
        Image image = getIconImage(app);
        SimpleBasisAMSImageButton button = null;
        if (image != null) {
            button = new SimpleBasisAMSImageButton(getIconImage(app));
        } else {
            button = new SimpleBasisAMSImageButton();
        }
        if (button == null) {
            return null;
        }
        button.setEnabled(true);
        button.setForeground(color);
        button.setTextShadow(true);
        button.setPaintBorders(true);
        
        // Trim strings that are too long to fit.
        String str = null;
        if (app.getTitle().trim().length() > MAX_TITLE_CHARS) {
            str = app.getTitle().trim().substring(0, MAX_TITLE_CHARS - 3);
            str += "...";
        } else {
            str = app.getTitle().trim();
        }
        
        button.setLabel(str);
        button.addActionListener(action);
        
        screenContainer.add(button);
        return button;
    }
    
    private JUMPApplicationProxy launchApp(JUMPApplication app) {
        // There currently isn't a way to extract application arguments.
        // Will use null for now.
        
        JUMPApplicationProxy appProxy = null;
        synchronized(timeoutObject) {
            appProxy = alm.launchApplication(app, null);
            try {
                // Use a timeout to detect whether or not a JUMPWindow is created
                // after launching the application.  The detection is done in
                // handleMessage().  If a JUMPWindow isn't detected during the
                // timeout, it is assumed that there is a problem.
                timeoutObject.wait(TIMEOUT_VAL);
            } catch (InterruptedException ex) {
                ex.printStackTrace();
            }
        }
        
        if (appWindowDisplayState) {
            bringWindowToFront(app);
            currentApp = app;
        }
        return appProxy;
    }
    
    private void killAllApps() {
        JUMPApplication apps[] = getRunningApps();
        for (int i = 0; i < apps.length; i++) {
            killApp(apps[i]);
        }
    }
    
    private void killApp(JUMPApplication app) {
        destroyApp(app);
        currentApp = null;
    }
    
    private void bringWindowToFront(JUMPApplication app) {
        if (app == null) {
            trace("ERROR:  Cannot do a bringWindowToFront... app is null.");
            return;
        }
        
        // find window owned by the application and make it foreground
        JUMPWindow windows[] = wm.getWindows();
        trace("**************** bringWindowToFront begin ***********************");
        trace("Length of wm.getWindows(): " + windows.length);
        for(int i = 0; i != windows.length; ++i) {
            trace("-> Window: " + i + ": " + windows[i]);
            if(windows[i].getApplication().equals(app)) {
                // make window foreground
                trace("FOUND Window to bring to front: " + app.getTitle());
                wm.setForeground(windows[i]);
                break;
            }
        }
        trace("**************** bringWindowToFront end ***********************");
        
    }
    
    private void bringWindowToBack(JUMPApplication app) {
        if (app == null) {
            trace("ERROR:  Cannot do a bringWindowToBack... app is null.");
            return;
        }
        // find window owned by the application and make it foreground
        JUMPWindow windows[] = wm.getWindows();
        trace("************bringWindowToBack begin ***************************");
        trace("Length of wm.getWindows(): " + windows.length);
        for(int i = 0; i != windows.length; ++i) {
            trace("-> Window: " + i + ": " + windows[i]);
            if(windows[i].getApplication().equals(app)) {
                // make window foreground
                trace("FOUND Window to bring to back: " + app.getTitle());
                wm.setBackground(windows[i]);
                break;
            }
        }
        trace("************bringWindowToBack end ***************************");
    }
    
    private void switchToApp(JUMPApplication app) {
        resumeApp(app);
        bringWindowToFront(app);
        currentApp = app;
    }
    
    private void resumeApp(JUMPApplication app) {
        if (app == null) {
            return;
        }
        trace("*** Trying to resume: " + app.getTitle());
        // find app's proxy and resume it
        JUMPApplicationProxy appProxies[] = alm.getApplications(app);
        for(int i = 0; i != appProxies.length /* 1 */; ++i) {
            appProxies[i].resumeApp();
        }
    }
    
    private void destroyApp(JUMPApplication app) {
        if (app == null) {
            return;
        }
        trace("*** Trying to kill: " + app.getTitle());
        // find app's proxy and destroy it's apps
        JUMPApplicationProxy appProxies[] = alm.getApplications(app);
        if (appProxies == null) {
            return;
        } else {
            for(int i = 0; i != appProxies.length /* 1 */; ++i) {
                appProxies[i].destroyApp();
            }
        }
    }
    
    private void pauseApp(JUMPApplication app) {
        if (app == null) {
            return;
        }
        trace("*** Trying to pause: " + app.getTitle());
        // find app's proxy and resume it
        JUMPApplicationProxy appProxies[] = alm.getApplications(app);
        if (appProxies == null) {
            return;
        } else {
            trace("*** alm.getApplications() returns the length: " + appProxies.length);
            for(int i = 0; i != appProxies.length /* 1 */; ++i) {
                appProxies[i].pauseApp();
            }
        }
    }
    
    private JUMPApplication[] getRunningApps() {
        JUMPIsolateProxy[] ips = lcm.getActiveIsolates();
        Vector appsVector = new Vector();
        for (int i = 0; i < ips.length; i++) {
            JUMPIsolateProxy ip = ips[i];
            JUMPApplicationProxy appProxy[] = ip.getApps();
            if (appProxy == null) {
                return null;
            } else {
                for (int j = 0; j < appProxy.length; j++) {
                    appsVector.add(appProxy[j].getApplication());
                }
            }
        }
        
        return (JUMPApplication[]) appsVector.toArray(new JUMPApplication[]{});
    }
    
    private JUMPApplication[] getInstalledApps() {
        JUMPInstallerModule installers[] = JUMPInstallerModuleFactory.getInstance().getAllInstallers();
        Vector appsVector = new Vector();
        if (installers == null) {
            return null;
        }
        for (int i = 0; i < installers.length; i++) {
            JUMPContent[] content = installers[i].getInstalled();
            if (content != null) {
                for(int j = 0; j < content.length; j++) {
                    appsVector.add((JUMPApplication)content[j]);
                }
            }
        }
        
        return (JUMPApplication[]) appsVector.toArray(new JUMPApplication[]{});
    }
    
    void trace(String str) {
        if (verbose) {
            System.out.println(str);
        }
    }
    
    /**
     * Implementation of the interface's start() method.
     */
    public void start() {
        System.err.println("*** Starting SimpleBasisAMS ***");
        if (setup()) {
            doApplicationsScreen();
            if (frame != null) {
                frame.setVisible(true);
            }
        } else {
            System.err.println("*** Setup of SimpleBasisAMS failed. ***");
        }
    }
}
