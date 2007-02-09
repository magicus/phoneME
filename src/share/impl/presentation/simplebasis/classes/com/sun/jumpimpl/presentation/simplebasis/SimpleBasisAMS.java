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

import com.sun.jump.module.presentation.JUMPPresentationModule;
import com.sun.jump.common.JUMPApplication;
import com.sun.jump.common.JUMPContent;
import com.sun.jump.executive.JUMPApplicationProxy;
import com.sun.jump.executive.JUMPIsolateProxy;
import com.sun.jump.module.installer.JUMPInstallerModule;
import com.sun.jump.module.installer.JUMPInstallerModuleFactory;
import com.sun.jump.module.isolatemanager.JUMPIsolateManagerModule;
import com.sun.jump.module.isolatemanager.JUMPIsolateManagerModuleFactory;
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
import java.util.HashMap;
import java.util.Map;
import java.util.Vector;

/**
 * A simple JUMP launcher that uses Personal Basis components.
 */
public class SimpleBasisAMS implements JUMPPresentationModule {
    
    private Frame frame = null;
    private Container commandContainer = null;
    private Container screenContainer = null;
    private HashMap appToProxyHash = null;
    
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
        
        screenContainer = new Container();
        screenContainer.setLayout(new GridLayout(0, 2));
        
        frame.add(commandContainer, BorderLayout.NORTH);
        frame.add(screenContainer, BorderLayout.CENTER);
        
        appToProxyHash = new HashMap();
        
        return true;
    }
    
    /**
     * Display the screen containing application icons.
     */
    public void doApplicationsScreen() {
        clearScreen();
        JUMPApplication apps[] = getInstalledApps();
        for (int i = 0; i < apps.length; i++) {
            trace("Loading " + apps[i].getTitle() + "...");
            addScreenButton(apps[i], new LaunchAppActionListener(apps[i]), Color.yellow);
        }
        refreshScreen();
    }
    
    
    /**
     * Display the switch-to screen, consisting of icons
     * pertaining to currently running applications.
     */
    public void doSwitchToScreen() {
        clearScreen();
        JUMPApplication apps[] = getRunningApps();
        for (int i = 0; i < apps.length; i++) {
            trace("Running app: " + apps[i].getTitle());
            addScreenButton(apps[i], new SwitchToActionListener(apps[i]), Color.green);
        }
        refreshScreen();
    }
    
    /**
     * Display the kill screen, consisting of icons pertaining to currently
     * running applications that can be killed .
     */
    public void doKillScreen() {
        clearScreen();
        JUMPApplication apps[] = getRunningApps();
        for (int i = 0; i < apps.length; i++) {
            trace("Running app: " + apps[i].getTitle());
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
            launchApp(app);
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
    
    private Image getIconImage(JUMPApplication app) {
        trace("Icon Path: " + app.getIconPath().getFile());
        return Toolkit.getDefaultToolkit().createImage(app.getIconPath());
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
        if (label.trim().length() > 15) {
            str = label.trim().substring(0, 12);
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
        SimpleBasisAMSImageButton button = new SimpleBasisAMSImageButton(getIconImage(app));
        if (button == null) {
            return null;
        }
        button.setEnabled(true);
        button.setForeground(color);
        button.setTextShadow(true);
        button.setPaintBorders(true);
        
        // Trim strings that are too long to fit.
        String str = null;
        if (app.getTitle().trim().length()
        > 15) {
            str =
                    app.getTitle().trim().substring(0, 12);
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
        JUMPIsolateManagerModuleFactory lcmf =
                JUMPIsolateManagerModuleFactory.getInstance();
        JUMPIsolateManagerModule lcm = lcmf.getModule();
        JUMPIsolateProxy ip = lcm.newIsolate(app.getAppType());
        System.err.println("*** New isolate created="+ip);
        System.err.println("*** Isolate trying to launch: " + app.getTitle() + "...");
        JUMPApplicationProxy appProxy = ip.startApp(app, null);
        if (appProxy != null) {
            appToProxyHash.put(app, appProxy);
            trace("*** Launch of " + app.getTitle() + " returns proxy: " + appProxy);
        }
        return appProxy;
    }
    
    private void killApp(JUMPApplication app) {
        if (app == null) {
            return;
        }
        JUMPApplicationProxy appProxy = (JUMPApplicationProxy)appToProxyHash.get(app);
        if (appProxy == null) {
            return;
        }
        appProxy.destroyApp();
        appToProxyHash.remove(app);
    }
    
    private void switchToApp(JUMPApplication app) {
        if (app == null) {
            return;
        }
        JUMPApplicationProxy appProxy = (JUMPApplicationProxy)appToProxyHash.get(app);
        if (appProxy == null) {
            return;
        }
        appProxy.resumeApp();
    }
    
    private JUMPApplication[] getRunningApps() {
        
        JUMPIsolateManagerModuleFactory lcmf =
                JUMPIsolateManagerModuleFactory.getInstance();
        JUMPIsolateManagerModule lcm = lcmf.getModule();
        JUMPIsolateProxy[] ips = lcm.getActiveIsolates();
        Vector appsVector = new Vector();
        for (int i = 0; i < ips.length; i++) {
            JUMPIsolateProxy ip = ips[i];
            JUMPApplicationProxy appProxy[] = ip.getApps();
            for (int j = 0; j < appProxy.length; j++) {
                appsVector.add(appProxy[j].getApplication());
            }
        }
        
        return (JUMPApplication[]) appsVector.toArray(new JUMPApplication[]{});
    }
    
    private JUMPApplication[] getInstalledApps() {
        JUMPInstallerModule installers[] = JUMPInstallerModuleFactory.getInstance().getAllInstallers();
        Vector appsVector = new Vector();
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
        if (true) {
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
