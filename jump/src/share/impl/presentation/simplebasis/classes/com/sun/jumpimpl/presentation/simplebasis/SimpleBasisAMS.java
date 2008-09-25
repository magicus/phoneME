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
import com.sun.jump.executive.JUMPIsolateFactory;
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
import com.sun.jump.module.windowing.JUMPWindowingModule;
import com.sun.jump.module.windowing.JUMPWindowingModuleFactory;
import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Container;
import java.awt.Dimension;
import java.awt.Font;
import java.awt.Frame;
import java.awt.Graphics;
import java.awt.GridLayout;
import java.awt.Image;
import java.awt.Toolkit;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.FocusEvent;
import java.awt.event.FocusListener;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;
import java.io.IOException;
import java.net.URL;
import java.util.Map;
import java.util.Vector;

/**
 * A simple JUMP launcher that uses Personal Basis components.
 */
public class SimpleBasisAMS implements JUMPPresentationModule, JUMPMessageHandler {
    
    private Frame frame = null;
    private CommandContainer commandContainer = null;
    private ScreenContainer screenContainer = null;
    private JUMPWindowingModuleFactory wmf = null;
    private JUMPWindowingModule wm = null;
    private JUMPApplicationLifecycleModuleFactory almf = null;
    private JUMPApplicationLifecycleModule alm = null;
    private JUMPIsolateFactory lcm = null;
    private JUMPApplicationProxy currentApp = null;
    private Object timeoutObject = null;
    private boolean appWindowDisplayState = false;
    private static final int TIMEOUT_VAL = 2000;
    protected static final int MAX_TITLE_CHARS = 15;
    private SimpleBasisAMSImageButton applicationsScreenButtons[] = null;
    private SimpleBasisAMSImageButton switchScreenButtons[] = null;
    private SimpleBasisAMSImageButton killScreenButtons[] = null;
    
    static final int SCREEN_ROWS = 3;
    static final int SCREEN_COLUMNS = 3;
    static final int SCREEN_DISPLAY_ICONS = SCREEN_ROWS * SCREEN_COLUMNS;
    
    private int CURRENT_SCREEN = 0;
    private static final int APPLICATIONS_SCREEN = 1;
    private static final int SWITCHTO_SCREEN = 2;
    private static final int KILL_SCREEN = 3;
    private static final int HELP_SCREEN = 4;
    private static final int PREFS_SCREEN = 5;
    
    
    private static final Color APPLICATIONS_SCREEN_COLOR = new Color(179, 229, 188);
    private static final Color SWITCHTO_SCREEN_COLOR = new Color(87, 188, 132);
    private static final Color KILL_SCREEN_COLOR = new Color(229, 183, 179);
    private static final Color HELP_SCREEN_COLOR = new Color(225, 227, 187);
    private static final Color PREFS_SCREEN_COLOR = new Color(255, 188, 157);
    
    private static final Color BUTTON_BLUE_COLOR = new Color(86, 135, 248);
    
    int applicationsScreenPageNumber = 0;
    int switchToScreenPageNumber = 0;
    int killScreenPageNumber = 0;
    
    SimpleBasisAMSInstall installer = null;
    static boolean verbose;
    Map map = null;
    
    JUMPMessageDispatcher md = null;
    Object isolateWindowHandler = null;
    Object lifecycleHandler = null;
    
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
        this.map = map;
        
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
    
    class ScreenContainer extends Container implements FocusListener, KeyListener {
        
        public ScreenContainer() {
            addKeyListener(this);
            addFocusListener(this);
        }
        
        public boolean isFocusable() {
            return true;
        }
        
        public void focusGained(FocusEvent e) {
            transferFocus();
        }
        
        public void focusLost(FocusEvent e) {
        }
        
        public void keyTyped(KeyEvent e) {
        }
        
        public void keyPressed(KeyEvent e) {
        }
        
        public void keyReleased(KeyEvent e) {
        }
    }
    
    class CommandContainer extends Container implements FocusListener, KeyListener {
        public CommandContainer() {
            addKeyListener(this);
            addFocusListener(this);
        }
        
        public Dimension getPreferredSize() {
            return new Dimension(480, 50);
        }
        
        public boolean isFocusable() {
            return true;
        }
        
        public void focusGained(FocusEvent e) {
            transferFocus();
        }
        
        public void focusLost(FocusEvent e) {
        }
        
        public void keyTyped(KeyEvent e) {
        }
        
        public void keyPressed(KeyEvent e) {;
        }
        
        public void keyReleased(KeyEvent e) {
        }
    }
    
    private boolean setup() {
        
        // the loading of apps will happen in its own thread
        LoadAppsThread loadAppsThread = new LoadAppsThread();
        loadAppsThread.start();
        
        frame = new Frame();
        frame.setLayout(new BorderLayout());
        
        commandContainer = new CommandContainer();
        
        commandContainer.setLayout(new GridLayout(0, 4));
        addCommandButton("Apps", commandContainer, new ApplicationsScreenActionListener());
        addCommandButton("Switch", commandContainer, new SwitchToScreenActionListener());
        addCommandButton("Kill", commandContainer, new KillScreenActionListener());
        addCommandButton("Help", commandContainer, new HelpScreenActionListener());
        addCommandButton("Install", commandContainer, new InstallScreenActionListener());
        addCommandButton("Remove", commandContainer, new RemoveScreenActionListener());
        addCommandButton("Prefs", commandContainer, new PrefsScreenActionListener());
        addCommandButton("Exit", commandContainer, new ExitActionListener());
        
        screenContainer = new ScreenContainer();
        screenContainer.setLayout(new BorderLayout());
        
        frame.add(screenContainer, BorderLayout.CENTER);
        frame.add(commandContainer, BorderLayout.NORTH);
        
        wmf = JUMPWindowingModuleFactory.getInstance();
        wm = wmf.getModule();
        
        almf = JUMPApplicationLifecycleModuleFactory.getInstance();
        alm = almf.getModule(JUMPApplicationLifecycleModuleFactory.POLICY_ONE_LIVE_INSTANCE_ONLY);
        
        
        JUMPExecutive e = JUMPExecutive.getInstance();
        md = e.getMessageDispatcher();

        lcm = e.getIsolateFactory();
        
        try {
            isolateWindowHandler = md.registerHandler(JUMPIsolateWindowRequest.MESSAGE_TYPE, this);
            lifecycleHandler = md.registerHandler(JUMPIsolateLifecycleRequest.MESSAGE_TYPE, this);
        } catch (JUMPMessageDispatcherTypeException ex) {
            ex.printStackTrace();
        } catch (IOException ex) {
            ex.printStackTrace();
        }
        
        try {
            loadAppsThread.join();
        } catch (InterruptedException ex) {
            ex.printStackTrace();
        }
        
        installer = new SimpleBasisAMSInstall(this);
        return true;
    }
    
    public void refreshApplicationsScreen() {
        // the loading of apps will happen in its own thread
        LoadAppsThread loadAppsThread = new LoadAppsThread();
        loadAppsThread.start();
        try {
            loadAppsThread.join();
        } catch (InterruptedException ex) {
            ex.printStackTrace();
        }
    }
    
    class LoadAppsThread extends Thread {
        public void run() {
            JUMPApplication apps[] = getInstalledApps();
            applicationsScreenButtons = new SimpleBasisAMSImageButton[apps.length];
            for (int i = 0; i < apps.length; i++) {
                trace("Loading: " + apps[i].getTitle());
                applicationsScreenButtons[i] = createScreenButton(apps[i], new LaunchAppActionListener(apps[i]), APPLICATIONS_SCREEN_COLOR);
            }
        }
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
            
            if (JUMPIsolateWindowRequest.ID_NOTIFY_WINDOW_FOREGROUND.equals
                    (cmd.getCommandId())) {
                trace("====== COMMAND RECEIVED: JUMPIsolateWindowRequest.ID_NOTIFY_WINDOW_FOREGROUND");
                appWindowDisplayState = true;
                synchronized(timeoutObject) {
                    System.out.println("****** Calling notify() on timout object. ******");
                    timeoutObject.notify();
                }
            } else if(JUMPIsolateWindowRequest.ID_NOTIFY_WINDOW_BACKGROUND.equals(
                    cmd.getCommandId())) {
                trace("====== COMMAND RECEIVED: JUMPIsolateWindowRequest.ID_NOTIFY_WINDOW_BACKGROUND");
            }
        } else if(JUMPIsolateLifecycleRequest.MESSAGE_TYPE.equals(
                message.getType())) {
            trace("==== MESSAGE RECEIVED: JUMPIsolateLifecycleRequest.MESSAGE_TYPE");
            JUMPIsolateLifecycleRequest cmd =
                    (JUMPIsolateLifecycleRequest)
                    JUMPIsolateLifecycleRequest.fromMessage(message);
            
            int isolateID = cmd.getIsolateId();
            
            if (JUMPIsolateLifecycleRequest.ID_ISOLATE_DESTROYED.equals
                    (cmd.getCommandId())) {
                
                trace("====== COMMAND RECEIVED: JUMPIsolateLifecycleRequest.ID_ISOLATE_DESTROYED");
                
                JUMPIsolateProxy isolateProxy = lcm.getIsolate(isolateID);
                JUMPApplicationProxy apps[] = isolateProxy.getApps();
                
                // the killApp(app) call below may not be needed and needs
                // to undergo testing to see if this is necessary.  In fact,
                // it may be the case that the 'app' value returned by
                // window application is null.  If killApp(app) is not needed,
                // then it should be replaced with a "currentApp = null".
                if(apps != null) {
                    for(int i = 0; i < apps.length; ++i) {
                        trace("====== killApp( + " + apps[i].getApplication() + ")");
                        killApp(apps[i]);
                    }
                }
            }
        }
    }
    
    public void displayDialog(String str, ActionListener okActionListener,
            ActionListener cancelActionListener) {
        trace("ENTERING DIALOG SCREEN");
        if (currentApp != null) {
            pauseApp(currentApp);
            bringWindowToBack(currentApp);
        }
        Container dialogContainer = new Container();
        dialogContainer.setBackground(Color.white);
        dialogContainer.setLayout(new BorderLayout());
        
        final String displayStr = str;
        Container textContainer = new Container() {
            public void paint(Graphics g) {
                super.paint(g);
                g.drawString(displayStr, 10, 20);
            }
        };
        textContainer.setBackground(Color.white);
        
        Container buttonContainer = new Container();
        buttonContainer.setLayout(new GridLayout(1, 2));
        addDialogButton("OK", buttonContainer, okActionListener);
        addDialogButton("Cancel", buttonContainer, cancelActionListener);
        
        dialogContainer.add(textContainer, BorderLayout.CENTER);
        dialogContainer.add(buttonContainer, BorderLayout.SOUTH);
        
        showScreen(dialogContainer);
    }
    
    private void pageUp() {
        int currentScreen = getCurrentScreen();
        
        if (currentScreen == APPLICATIONS_SCREEN) {
            // Determine if there is possibly more icons to display
            // beyond this page
            if (applicationsScreenPageNumber > 0) {
                applicationsScreenPageNumber--;
                doApplicationsScreen();
            }
            
        } else if (currentScreen == SWITCHTO_SCREEN) {
            // Determine if there is possibly more icons to display
            // beyond this page
            if (switchToScreenPageNumber > 0) {
                switchToScreenPageNumber--;
                doSwitchToScreen();
            }
        } else if (currentScreen == KILL_SCREEN) {
            // Determine if there is possibly more icons to display
            // beyond this page
            if (killScreenPageNumber > 0) {
                killScreenPageNumber--;
                doKillScreen();
            }
        } else {
            installer.pageUp();
        }
    }
    
    private void pageDown() {
        int currentScreen = getCurrentScreen();
        if (currentScreen == APPLICATIONS_SCREEN) {
            // Find out number of total screen pages
            int totalApplicationsScreenPages = getTotalScreenPages(applicationsScreenButtons.length, SCREEN_DISPLAY_ICONS);
            
            // Don't scroll beyond the last page
            if (applicationsScreenPageNumber < (totalApplicationsScreenPages - 1)) {
                applicationsScreenPageNumber++;
                doApplicationsScreen();
            }
            
        } else if (currentScreen == SWITCHTO_SCREEN) {
            // Find out number of total screen pages
            int totalSwitchToScreenPages = getTotalScreenPages(switchScreenButtons.length, SCREEN_DISPLAY_ICONS);
            // Don't scroll beyond the last page
            if (switchToScreenPageNumber < (totalSwitchToScreenPages - 1)) {
                switchToScreenPageNumber++;
                doSwitchToScreen();
            }
            
        } else if (currentScreen == KILL_SCREEN) {
            // Find out number of total screen pages
            int totalKillScreenPages = getTotalScreenPages(killScreenButtons.length, SCREEN_DISPLAY_ICONS);
            // Don't scroll beyond the last page
            if (killScreenPageNumber < (totalKillScreenPages - 1)) {
                killScreenPageNumber++;
                doKillScreen();
            }
            
        } else {
            installer.pageDown();
        }
    }
    
    /*
     * Utility method to determine the number of pages to store
     * a total number of icons
     */
    int getTotalScreenPages(int numIcons, int screenIconCapacity) {
        int totalScreenPages = 0;
        int div = numIcons / screenIconCapacity;
        int mod = numIcons % screenIconCapacity;
        if (mod == 0) {
            totalScreenPages = div;
        } else {
            totalScreenPages = div + 1;
        }
        return totalScreenPages;
    }
    
    
    /**
     * Display the screen containing application icons.
     */
    void doApplicationsScreen() {
        trace("ENTERING SCREEN: APPLICATIONS");
        if (currentApp != null) {
            pauseApp(currentApp);
            bringWindowToBack(currentApp);
        }
        showScreen(createApplicationsScreen());
        setCurrentScreen(APPLICATIONS_SCREEN);
    }
    
    private Container createApplicationsScreen() {
        Container applicationsScreen = new Container();
        applicationsScreen.setLayout(new DisplayLayout());
        int firstPositionIndex = applicationsScreenPageNumber * SCREEN_DISPLAY_ICONS;
        for (int i = firstPositionIndex;
        i < (applicationsScreenPageNumber * SCREEN_DISPLAY_ICONS + SCREEN_DISPLAY_ICONS); i++) {
            if (i < applicationsScreenButtons.length) {
                applicationsScreen.add(applicationsScreenButtons[i]);
            } else {
                applicationsScreen.add(createScreenButton(null, APPLICATIONS_SCREEN_COLOR));
            }
        }
        return applicationsScreen;
    }
    
    class ScreenButtonsKeyListener implements KeyListener {
        SimpleBasisAMSImageButton button = null;
        
        public ScreenButtonsKeyListener(SimpleBasisAMSImageButton button) {
            this.button = button;
        }
        
        public void keyTyped(KeyEvent e) {
        }
        
        public void keyPressed(KeyEvent e) {
            int keyCode = e.getKeyCode();
            if (keyCode == KeyEvent.VK_PAGE_UP || keyCode == KeyEvent.VK_F1) {
                pageUp();
            } else if (keyCode == KeyEvent.VK_PAGE_DOWN || keyCode == KeyEvent.VK_F2) {
                pageDown();
            } else if (keyCode == KeyEvent.VK_ENTER) {
                button.doAction();
            } else if (keyCode == KeyEvent.VK_UP || keyCode == KeyEvent.VK_LEFT ) {
                if (e.getComponent() == screenContainer.getComponent(0)) {
                    commandContainer.requestFocusInWindow();
                } else {
                    button.transferFocusBackward();
                }
            } else if (keyCode == KeyEvent.VK_DOWN | keyCode == KeyEvent.VK_RIGHT) {
                button.transferFocus();
            }
        }
        
        public void keyReleased(KeyEvent e) {
        }
    }
    
    class CommandButtonsKeyListener implements KeyListener {
        SimpleBasisAMSImageButton button = null;
        
        public CommandButtonsKeyListener(SimpleBasisAMSImageButton button) {
            this.button = button;
        }
        public void keyTyped(KeyEvent e) {
        }
        
        public void keyPressed(KeyEvent e) {
            int keyCode = e.getKeyCode();
            if (keyCode == KeyEvent.VK_PAGE_UP || keyCode == KeyEvent.VK_F1) {
                pageUp();
            } else if (keyCode == KeyEvent.VK_PAGE_DOWN || keyCode == KeyEvent.VK_F2) {
                pageDown();
            } else if (keyCode == KeyEvent.VK_ENTER) {
                button.doAction();
            } else if (keyCode == KeyEvent.VK_DOWN || keyCode == KeyEvent.VK_UP) {
                screenContainer.requestFocusInWindow();
            } else if (keyCode == KeyEvent.VK_LEFT) {
                button.transferFocusBackward();
            } else if (keyCode == KeyEvent.VK_RIGHT) {
                button.transferFocus();
            }
        }
        
        public void keyReleased(KeyEvent e) {
        }
    }
    
    /**
     * Display the switch-to screen, consisting of icons
     * pertaining to currently running applications.
     */
    private void doSwitchToScreen() {
        trace("ENTERING SCREEN: SWITCH");
        if (currentApp == null) {
            trace("*** currentApp is NULL. ***");
        } else {
            trace("*** currentApp -> " + currentApp.getApplication().getTitle() + " ***");
        }
        if (currentApp != null) {
            pauseApp(currentApp);
            bringWindowToBack(currentApp);
        }
        showScreen(createSwitchToScreen());
        setCurrentScreen(SWITCHTO_SCREEN);
    }
    
    private Container createSwitchToScreen() {
        Container switchToScreen = new Container();
        switchToScreen.setLayout(new DisplayLayout());
        JUMPApplicationProxy apps[] = getRunningApps();
        switchScreenButtons = new SimpleBasisAMSImageButton[apps.length];
        for (int i = 0; i < apps.length; i++) {
            switchScreenButtons[i] = createScreenButton(apps[i].getApplication(), new SwitchToActionListener(apps[i]), SWITCHTO_SCREEN_COLOR);
        }
        int firstPositionIndex = switchToScreenPageNumber * SCREEN_DISPLAY_ICONS;
        for (int i = firstPositionIndex;
        i < (switchToScreenPageNumber * SCREEN_DISPLAY_ICONS + SCREEN_DISPLAY_ICONS); i++) {
            if (i < switchScreenButtons.length) {
                switchToScreen.add(switchScreenButtons[i]);
            } else {
                switchToScreen.add(createScreenButton(null, SWITCHTO_SCREEN_COLOR));
            }
        }
        return switchToScreen;
    }
    
    /**
     * Display the kill screen, consisting of icons pertaining to currently
     * running applications that can be killed .
     */
    private void doKillScreen() {
        trace("ENTERING SCREEN: KILL");
        if (currentApp != null) {
            pauseApp(currentApp);
            bringWindowToBack(currentApp);
        }
        showScreen(createKillScreen());
        setCurrentScreen(KILL_SCREEN);
    }
    
    private Container createKillScreen() {
        Container killScreen = new Container();
        killScreen.setLayout(new DisplayLayout());
        JUMPApplicationProxy apps[] = getRunningApps();
        killScreenButtons = new SimpleBasisAMSImageButton[apps.length];
        for (int i = 0; i < apps.length; i++) {
            killScreenButtons[i] = createScreenButton(apps[i].getApplication(), new KillActionListener(apps[i]), KILL_SCREEN_COLOR);
        }
        int firstPositionIndex = killScreenPageNumber * SCREEN_DISPLAY_ICONS;
        for (int i = firstPositionIndex;
        i < (killScreenPageNumber * SCREEN_DISPLAY_ICONS + SCREEN_DISPLAY_ICONS) ; i++) {
            if (i < killScreenButtons.length) {
                killScreen.add(killScreenButtons[i]);
            } else {
                killScreen.add(createScreenButton(null, KILL_SCREEN_COLOR));
            }
        }
        return killScreen;
    }
    
    private void doRemoveScreen() {
        trace("ENTERING SCREEN: INSTALLER");
        if (currentApp != null) {
            pauseApp(currentApp);
            bringWindowToBack(currentApp);
        }
        installer.showRemoveScreen();
    }
    
    private void doInstallScreen() {
        trace("ENTERING SCREEN: INSTALLER");
        if (currentApp != null) {
            pauseApp(currentApp);
            bringWindowToBack(currentApp);
        }
        installer.showInstallScreen();
    }
    
    private void doHelpScreen() {
        trace("ENTERING SCREEN: KILL");
        if (currentApp != null) {
            pauseApp(currentApp);
            bringWindowToBack(currentApp);
        }
        Container helpContainer = new Container() {
            public void paint(Graphics g) {
                super.paint(g);
                g.drawString(" NOTE: When there is no mouse support,", 10, 20);
                g.drawString("  the TAB key must be pressed to initially", 10, 40);
                g.drawString("  bring focus to this window.", 10, 60);
                g.drawString(" F1  : PAGE UP", 10, 100);
                g.drawString(" F2  : PAGE DOWN", 10, 120);
            }
        };
        helpContainer.setBackground(HELP_SCREEN_COLOR);
        showScreen(helpContainer);
        setCurrentScreen(HELP_SCREEN);
    }
    
    private void doPrefsScreen() {
        trace("ENTERING SCREEN: PREFS");
        if (currentApp != null) {
            pauseApp(currentApp);
            bringWindowToBack(currentApp);
        }
        Container prefsContainer = new Container() {
            public void paint(Graphics g) {
                super.paint(g);
                g.drawString(" NOTE: This screen is currently,", 10, 20);
                g.drawString("  unimplemented.", 10, 40);
            }
        };
        prefsContainer.setBackground(PREFS_SCREEN_COLOR);
        showScreen(prefsContainer);
        setCurrentScreen(PREFS_SCREEN);
    }
    
    class InstallScreenActionListener implements ActionListener {
        public void actionPerformed(ActionEvent e) {
            doInstallScreen();
        }
    }
    
    class RemoveScreenActionListener implements ActionListener {
        public void actionPerformed(ActionEvent e) {
            doRemoveScreen();
        }
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
        public void actionPerformed(ActionEvent e) {
            doKillScreen();
        }
    }
    
    class HelpScreenActionListener
            implements ActionListener {
        public void actionPerformed(ActionEvent e) {
            doHelpScreen();
        }
    }
    
    class PrefsScreenActionListener
            implements ActionListener {
        public void actionPerformed(ActionEvent e) {
            doPrefsScreen();
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
        JUMPApplicationProxy app;
        
        public SwitchToActionListener(JUMPApplicationProxy app) {
            this.app = app;
        }
        
        public void actionPerformed(ActionEvent e) {
            switchToApp(app);
        }
    }
    
    class KillActionListener
            implements ActionListener {
        JUMPApplicationProxy app;
        
        public KillActionListener(JUMPApplicationProxy app) {
            this.app = app;
        }
        
        public void actionPerformed(ActionEvent e) {
            killApp(app);
            doKillScreen();
        }
    }
    
    class ExitActionListener
            implements ActionListener {
        
        public void actionPerformed(ActionEvent e) {
            killAllApps();
            // shut down the server
            new com.sun.jumpimpl.os.JUMPOSInterfaceImpl().shutdownServer();
            try {
                md.cancelRegistration(isolateWindowHandler);
                md.cancelRegistration(lifecycleHandler);                
            } catch (Exception ex) {
                System.out.println("ERROR: " + ex.getMessage());
                ex.printStackTrace();
            }
            finally {
            }
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
    
    private SimpleBasisAMSImageButton addCommandButton(String label, Container container, ActionListener action) {
        
        SimpleBasisAMSImageButton button = new SimpleBasisAMSImageButton();
        if (button == null) {
            return null;
        }
        button.addActionListener(action);
        button.setEnabled(true);
        button.setForeground(BUTTON_BLUE_COLOR);
        button.setTextShadow(false);
        button.setPaintBorders(true);
        button.setLabel(label);
        button.setFont(new Font("Monospaced", Font.PLAIN, 12));
        CommandButtonsKeyListener keyListener = new CommandButtonsKeyListener(button);
        button.addKeyListener(keyListener);
        
        container.add(button);
        return button;
    }
    
    private SimpleBasisAMSImageButton addDialogButton(String label, Container container, ActionListener action) {
        
        SimpleBasisAMSImageButton button = new SimpleBasisAMSImageButton();
        if (button == null) {
            return null;
        }
        button.addActionListener(action);
        button.setEnabled(true);
        button.setForeground(BUTTON_BLUE_COLOR);
        button.setTextShadow(true);
        button.setPaintBorders(true);
        button.setLabel(label);
        
        container.add(button);
        return button;
    }
    
    private void clearCommandScreen() {
        commandContainer.removeAll();
    }
    
    private void refreshCommandScreen() {
        commandContainer.validate();
        commandContainer.repaint();
    }
    
    public void clearScreen() {
        screenContainer.removeAll();
    }
    
    public void showScreen(Container container) {
        clearScreen();
        screenContainer.add(container, BorderLayout.CENTER);
        refreshScreen();
    }
    
    public void refreshScreen() {
        screenContainer.validate();
        screenContainer.repaint();
    }
    
    
    SimpleBasisAMSImageButton createScreenButton(JUMPApplication app, ActionListener action, Color color) {
        SimpleBasisAMSImageButton button = null;
        // When app is null, create an empty button
        if (app != null) {
            Image image = getIconImage(app);
            if (image != null) {
                button = new SimpleBasisAMSImageButton(getIconImage(app));
            } else {
                button = new SimpleBasisAMSImageButton();
            }
            if (button == null) {
                return null;
            }
            
            button.setLabel(app.getTitle().trim());
            button.setTextShadow(true);
            button.addActionListener(action);
            
        } else {
            button = new SimpleBasisAMSImageButton();
            if (button == null) {
                return null;
            }
            button.setEnabled(true);
            button.setForeground(color);
            button.setPaintBorders(true);
            button.setFocusable(false);
        }
        
        button.setEnabled(true);
        button.setForeground(color);
        button.setPaintBorders(true);
        
        ScreenButtonsKeyListener keyListener = new ScreenButtonsKeyListener(button);
        button.addKeyListener(keyListener);
        
        return button;
    }
    
    SimpleBasisAMSImageButton createScreenButton(String title, ActionListener action, Color color) {
        SimpleBasisAMSImageButton button = null;
        // Create an empty button
        button = new SimpleBasisAMSImageButton();
        if (button == null) {
            return null;
        }
        button.setEnabled(true);
        button.setForeground(color);
        button.setPaintBorders(true);
        button.setLabel(title);
        button.setTextShadow(true);
        button.addActionListener(action);
        ScreenButtonsKeyListener keyListener = new ScreenButtonsKeyListener(button);
        button.addKeyListener(keyListener);
        
        return button;
    }
    
    SimpleBasisAMSImageButton createScreenButton(ActionListener action, Color color) {
        SimpleBasisAMSImageButton button = null;
        // Create an empty button
        button = new SimpleBasisAMSImageButton();
        if (button == null) {
            return null;
        }
        button.setEnabled(true);
        button.setForeground(color);
        button.setPaintBorders(true);
        button.addActionListener(action);
        ScreenButtonsKeyListener keyListener = new ScreenButtonsKeyListener(button);
        button.addKeyListener(keyListener);
        
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
            bringWindowToFront(appProxy);
            currentApp = appProxy;
        }
        refreshCommandScreen();
        
        return appProxy;
    }
    
    private void killAllApps() {
        JUMPApplicationProxy apps[] = getRunningApps();
        for (int i = 0; i < apps.length; i++) {
            killApp(apps[i]);
        }
    }
    
    private void killApp(JUMPApplicationProxy app) {
        destroyApp(app);
        currentApp = null;
    }
    
    private void bringWindowToFront(JUMPApplicationProxy app) {
        if (app == null) {
            trace("ERROR:  Cannot do a bringWindowToFront... app is null.");
            return;
        }
        
        JUMPWindow[] windows = app.getIsolateProxy().getWindows();
        if(windows != null) {
            for (int i = 0; i < windows.length; i++) {
                wm.setForeground(windows[i]);
            }
        }
    }
    
    private void bringWindowToBack(JUMPApplicationProxy app) {
        if (app == null) {
            trace("ERROR:  Cannot do a bringWindowToBack... app is null.");
            return;
        }
        
        JUMPWindow[] windows = app.getIsolateProxy().getWindows();
        if(windows != null) {
            for (int i = 0; i < windows.length; i++) {
                wm.setBackground(windows[i]);
            }
        }
    }
    
    private void switchToApp(JUMPApplicationProxy app) {
        resumeApp(app);
        bringWindowToFront(app);
        currentApp = app;
    }
    
    private void resumeApp(JUMPApplicationProxy app) {
        if (app == null) {
            return;
        }
        trace("*** Trying to resume: " + app.getApplication().getTitle());
        app.resumeApp();
    }
    
    private void destroyApp(JUMPApplicationProxy app) {
        if (app == null) {
            return;
        }
        trace("*** Trying to kill: " + app.getApplication().getTitle());
        app.destroyApp();
    }
    
    private void pauseApp(JUMPApplicationProxy app) {
        if (app == null) {
            return;
        }
        trace("*** Trying to pause: " + app.getApplication().getTitle());
        app.pauseApp();
    }
    
    boolean isRunningApp(JUMPApplication app) {
        JUMPApplicationProxy proxy[] = getRunningApps();
        for (int i = 0; i < proxy.length; i++) {
            if (proxy[i].getApplication().equals(app)) {
                return true;
            }
        }
        return false;
    }
    
    private JUMPApplicationProxy[] getRunningApps() {
        JUMPIsolateProxy[] ips = lcm.getActiveIsolates();
        Vector appsVector = new Vector();
        for (int i = 0; i < ips.length; i++) {
            JUMPIsolateProxy ip = ips[i];
            JUMPApplicationProxy appProxy[] = ip.getApps();
            if (appProxy != null) {
                for (int j = 0; j < appProxy.length; j++) {
                    appsVector.add(appProxy[j]);
                }
            }
        }
        
        return (JUMPApplicationProxy[]) appsVector.toArray(new JUMPApplicationProxy[]{});
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
    
    static void trace(String str) {
        if (verbose) {
            System.out.println(str);
        }
    }
    
    class DisplayLayout extends GridLayout {
        public DisplayLayout() {
            super(SCREEN_ROWS, SCREEN_COLUMNS);
        }
    }
    
    /**
     * Implementation of the interface's start() method.
     */
    public void start() {
        System.err.println("*** Starting SimpleBasisAMS ***");
        if (setup()) {
            if (frame != null) {
                frame.setVisible(true);
            }
            doApplicationsScreen();
        } else {
            System.err.println("*** Setup of SimpleBasisAMS failed. ***");
        }
    }
    
    void setCurrentScreen(int screen) {
        CURRENT_SCREEN = screen;
    }
    
    int getCurrentScreen() {
        return CURRENT_SCREEN;
    }
    
}
