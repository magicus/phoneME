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
import java.awt.AWTEvent;
import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Component;
import java.awt.Container;
import java.awt.Dimension;
import java.awt.Frame;
import java.awt.Graphics;
import java.awt.GridLayout;
import java.awt.Image;
import java.awt.KeyEventDispatcher;
import java.awt.KeyboardFocusManager;
import java.awt.Toolkit;
import java.awt.event.AWTEventListener;
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
    private JUMPIsolateManagerModuleFactory lcmf = null;
    private JUMPIsolateManagerModule lcm = null;
    private JUMPApplicationProxy currentApp = null;
    private Object timeoutObject = null;
    private boolean appWindowDisplayState = false;
    private static final int TIMEOUT_VAL = 2000;
    protected static final int MAX_TITLE_CHARS = 15;
    private SimpleBasisAMSImageButton applicationsScreenButtons[] = null;
    private SimpleBasisAMSImageButton switchScreenButtons[] = null;
    private SimpleBasisAMSImageButton killScreenButtons[] = null;
    
    private static final int SCREEN_ROWS = 3;
    private static final int SCREEN_COLUMNS = 3;
    private static final int SCREEN_DISPLAY_ICONS = SCREEN_ROWS * SCREEN_COLUMNS;
    
    private int CURRENT_SCREEN = 0;
    private static final int APPLICATIONS_SCREEN = 1;
    private static final int SWITCHTO_SCREEN = 2;
    private static final int KILL_SCREEN = 3;
    private static final int HELP_SCREEN = 4;
    
    int applicationsScreenPageNumber = 0;
    int switchToScreenPageNumber = 0;
    int killScreenPageNumber = 0;
    
    static boolean verbose;
    
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
    
    class ScreenContainer extends Container implements FocusListener, KeyListener {
        
        public ScreenContainer() {
            addKeyListener(this);
            addFocusListener(this);
        }
        
        public boolean isFocusable() {
            return true;
        }
        
        public void focusGained(FocusEvent e) {
            trace("--> Screen Container has Focus!!!");
            transferFocus();
        }
        
        public void focusLost(FocusEvent e) {
            trace("--> Screen Container lost Focus!!!");
        }
        
        public void keyTyped(KeyEvent e) {
            trace("--> ScreenContainer keyTyped: " + e.toString());
        }
        
        public void keyPressed(KeyEvent e) {
            trace("--> ScreenContainer keyPressed: " + e.toString());
        }
        
        public void keyReleased(KeyEvent e) {
            trace("--> ScreenContainer Released: " + e.toString());
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
            trace("--> Command Container has Focus!!!");
            transferFocus();
        }
        
        public void focusLost(FocusEvent e) {
            trace("--> Command Container lost Focus!!!");
        }
        
        public void keyTyped(KeyEvent e) {
            trace("--> CommandContainer keyTyped: " + e.toString());
        }
        
        public void keyPressed(KeyEvent e) {
            trace("--> CommandContainer keyPressed: " + e.toString());
        }
        
        public void keyReleased(KeyEvent e) {
            trace("--> CommandContainer Released: " + e.toString());
        }
    }
    
    private boolean setup() {
        
        frame = new Frame();
        frame.setLayout(new BorderLayout());
        
        commandContainer = new CommandContainer();
        
        commandContainer.setLayout(new GridLayout(1, 5));
        addCommandButton("Apps", commandContainer, new ApplicationsScreenActionListener());
        addCommandButton("Switch", commandContainer, new SwitchToScreenActionListener());
        addCommandButton("Kill", commandContainer, new KillScreenActionListener());
        addCommandButton("Help", commandContainer, new HelpScreenActionListener());
        addCommandButton("Exit", commandContainer, new ExitActionListener());
        
        screenContainer = new ScreenContainer();
        screenContainer.setLayout(new GridLayout(SCREEN_ROWS, SCREEN_COLUMNS));
        
        frame.add(screenContainer, BorderLayout.CENTER);
        frame.add(commandContainer, BorderLayout.NORTH);
        
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
        
        JUMPApplication apps[] = getInstalledApps();
        applicationsScreenButtons = new SimpleBasisAMSImageButton[apps.length];
        for (int i = 0; i < apps.length; i++) {
            applicationsScreenButtons[i] = createScreenButton(apps[i], new LaunchAppActionListener(apps[i]), Color.yellow);
        }
        
//        try {
//            Toolkit.getDefaultToolkit().addAWTEventListener(new AMSEventListener(), AWTEvent.FOCUS_EVENT_MASK | AWTEvent.KEY_EVENT_MASK);
//        } catch (Exception ex) {
//            ex.printStackTrace();
//        }
        
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
    
    private void displayDialog(String str, ActionListener okActionListener,
            ActionListener cancelActionListener) {
        trace("ENTERING DIALOG SCREEN");
        if (currentApp != null) {
            pauseApp(currentApp);
            bringWindowToBack(currentApp);
        }
        Container helpContainer = new Container();
        helpContainer.setBackground(Color.white);
        helpContainer.setLayout(new BorderLayout());
        
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
        
        helpContainer.add(textContainer, BorderLayout.CENTER);
        helpContainer.add(buttonContainer, BorderLayout.SOUTH);
        
        screenContainer.add(helpContainer);
        refreshScreen();
    }
    
    public class AMSEventListener implements AWTEventListener {
        
        public void eventDispatched(AWTEvent e) {
            System.out.println("Event: " + e.toString());
//            KeyEvent k = (KeyEvent) e;
//            System.out.println("KeyEvent: " + k.toString());
        }
    }
    private void pageUp() {
        if (CURRENT_SCREEN == APPLICATIONS_SCREEN) {
            // Determine if there is possibly more icons to display
            // beyond this page
            if (applicationsScreenPageNumber > 0) {
                applicationsScreenPageNumber--;
                doApplicationsScreen();
            }
            
        } else if (CURRENT_SCREEN == SWITCHTO_SCREEN) {
            // Determine if there is possibly more icons to display
            // beyond this page
            if (switchToScreenPageNumber > 0) {
                switchToScreenPageNumber--;
                doSwitchToScreen();
            }
        } else if (CURRENT_SCREEN == KILL_SCREEN) {
            // Determine if there is possibly more icons to display
            // beyond this page
            if (killScreenPageNumber > 0) {
                killScreenPageNumber--;
                doKillScreen();
            }
        }
    }
    
    private void pageDown() {
        if (CURRENT_SCREEN == APPLICATIONS_SCREEN) {
            // Determine if there is possibly more icons to display
            // beyond this page
            
            // Find out number of total screen pages
            int totalApplicationsScreenPages = 0;
            int div = applicationsScreenButtons.length / SCREEN_DISPLAY_ICONS;
            int mod = applicationsScreenButtons.length % SCREEN_DISPLAY_ICONS;
            if (mod == 0) {
                totalApplicationsScreenPages = div;
            } else {
                totalApplicationsScreenPages = div + 1;
            }
            
            // Don't scroll beyond the last page
            if (applicationsScreenPageNumber < (totalApplicationsScreenPages - 1)) {
                applicationsScreenPageNumber++;
                doApplicationsScreen();
            }
            
        } else if (CURRENT_SCREEN == SWITCHTO_SCREEN) {
            
            // Find out number of total screen pages
            int totalSwitchToScreenPages = 0;
            int div = switchScreenButtons.length / SCREEN_DISPLAY_ICONS;
            int mod = switchScreenButtons.length % SCREEN_DISPLAY_ICONS;
            if (mod == 0) {
                totalSwitchToScreenPages = div;
            } else {
                totalSwitchToScreenPages = div + 1;
            }
            
            // Don't scroll beyond the last page
            if (switchToScreenPageNumber < (totalSwitchToScreenPages - 1)) {
                switchToScreenPageNumber++;
                doSwitchToScreen();
            }
            
        } else if (CURRENT_SCREEN == KILL_SCREEN) {
            
            // Find out number of total screen pages
            int totalKillScreenPages = 0;
            int div = killScreenButtons.length / SCREEN_DISPLAY_ICONS;
            int mod = killScreenButtons.length % SCREEN_DISPLAY_ICONS;
            if (mod == 0) {
                totalKillScreenPages = div;
            } else {
                totalKillScreenPages = div + 1;
            }
            
            // Don't scroll beyond the last page
            if (killScreenPageNumber < (totalKillScreenPages - 1)) {
                killScreenPageNumber++;
                doKillScreen();
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
        int firstPositionIndex = applicationsScreenPageNumber * SCREEN_DISPLAY_ICONS;
        for (int i = firstPositionIndex;
        i < (applicationsScreenPageNumber * SCREEN_DISPLAY_ICONS + SCREEN_DISPLAY_ICONS); i++) {
            if (i < applicationsScreenButtons.length) {
                screenContainer.add(applicationsScreenButtons[i]);
            } else {
                screenContainer.add(createScreenButton(null, null, Color.yellow));
            }
        }
        refreshScreen();
        CURRENT_SCREEN = APPLICATIONS_SCREEN;
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
            System.out.println(button.getLabel() + " screen button KeyEvent: " + e.toString());
            System.out.println(button.getLabel() + " screen button keyTyped: " + keyCode);
            if (keyCode == KeyEvent.VK_PAGE_UP || keyCode == KeyEvent.VK_F1) {
                System.out.println("*** PAGE UP ***");
                pageUp();
            } else if (keyCode == KeyEvent.VK_PAGE_DOWN || keyCode == KeyEvent.VK_F2) {
                System.out.println("*** PAGE DOWN ***");
                pageDown();
            } else if (keyCode == KeyEvent.VK_ENTER) {
                button.doAction();
            } else if (keyCode == KeyEvent.VK_UP || keyCode == KeyEvent.VK_LEFT ) {
                button.transferFocusBackward();
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
            System.out.println(button.getLabel() + " command button KeyEvent: " + e.toString());
            System.out.println(button.getLabel() + " command button keyTyped: " + keyCode);
            
            if (keyCode == KeyEvent.VK_PAGE_UP || keyCode == KeyEvent.VK_F1) {
                System.out.println("*** PAGE UP ***");
                pageUp();
            } else if (keyCode == KeyEvent.VK_PAGE_DOWN || keyCode == KeyEvent.VK_F2) {
                System.out.println("*** PAGE DOWN ***");
                pageDown();
            } else if (keyCode == KeyEvent.VK_ENTER) {
                button.doAction();
            } else if (keyCode == KeyEvent.VK_DOWN) {
                screenContainer.requestFocusInWindow();
            } else if (keyCode == KeyEvent.VK_LEFT) {
                button.transferFocusBackward();
                //button.transferFocus();
            } else if (keyCode == KeyEvent.VK_RIGHT) {
                //button.transferFocusUpCycle();
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
    public void doSwitchToScreen() {
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
        clearScreen();
        JUMPApplicationProxy apps[] = getRunningApps();
        switchScreenButtons = new SimpleBasisAMSImageButton[apps.length];
        for (int i = 0; i < apps.length; i++) {
            switchScreenButtons[i] = createScreenButton(apps[i].getApplication(), new SwitchToActionListener(apps[i]), Color.green);
        }
        int firstPositionIndex = switchToScreenPageNumber * SCREEN_DISPLAY_ICONS;
        for (int i = firstPositionIndex;
        i < (switchToScreenPageNumber * SCREEN_DISPLAY_ICONS + SCREEN_DISPLAY_ICONS); i++) {
            if (i < switchScreenButtons.length) {
                screenContainer.add(switchScreenButtons[i]);
            } else {
                screenContainer.add(createScreenButton(null, null, Color.green));
            }
        }
        refreshScreen();
        CURRENT_SCREEN = SWITCHTO_SCREEN;
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
        JUMPApplicationProxy apps[] = getRunningApps();
        killScreenButtons = new SimpleBasisAMSImageButton[apps.length];
        for (int i = 0; i < apps.length; i++) {
            killScreenButtons[i] = createScreenButton(apps[i].getApplication(), new KillActionListener(apps[i]), Color.red);
        }
        int firstPositionIndex = switchToScreenPageNumber * SCREEN_DISPLAY_ICONS;
        for (int i = firstPositionIndex;
        i < (killScreenPageNumber * SCREEN_DISPLAY_ICONS + SCREEN_DISPLAY_ICONS) ; i++) {
            if (i < killScreenButtons.length) {
                screenContainer.add(killScreenButtons[i]);
            } else {
                createScreenButton(null, null, Color.red);
            }
        }
        refreshScreen();
        CURRENT_SCREEN = KILL_SCREEN;
    }
    
    public void doHelpScreen() {
        trace("ENTERING SCREEN: KILL");
        if (currentApp != null) {
            pauseApp(currentApp);
            bringWindowToBack(currentApp);
        }
        clearScreen();
        Container helpContainer = new Container() {
            public void paint(Graphics g) {
                super.paint(g);
                g.drawString(" F1  : PAGE UP", 10, 20);
                g.drawString(" F2  : PAGE DOWN", 10, 50);
            }
        };
        helpContainer.setBackground(Color.white);
        screenContainer.add(helpContainer);
        refreshScreen();
        CURRENT_SCREEN = HELP_SCREEN;
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
    
    class HelpScreenActionListener
            implements ActionListener {
        
        public HelpScreenActionListener() {
        }
        
        public void actionPerformed(ActionEvent e) {
            doHelpScreen();
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
    
    private SimpleBasisAMSImageButton addCommandButton(String label, Container container, ActionListener action) {
        
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
    
    private void clearScreen() {
        screenContainer.removeAll();
    }
    
    private void refreshScreen() {
        screenContainer.validate();
        screenContainer.repaint();
    }
    
    private SimpleBasisAMSImageButton createScreenButton(JUMPApplication app, ActionListener action, Color color) {
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
            
            // Trim strings that are too long to fit.
            String str = null;
            if (app.getTitle().trim().length() > MAX_TITLE_CHARS) {
                str = app.getTitle().trim().substring(0, MAX_TITLE_CHARS - 3);
                str += "...";
            } else {
                str = app.getTitle().trim();
            }
            button.setLabel(str);
            button.setTextShadow(true);
            button.addActionListener(action);
            
            //screenContainer.add(button);
        } else {
            button = new SimpleBasisAMSImageButton();
            if (button == null) {
                return null;
            }
            button.setEnabled(true);
            button.setForeground(color);
            button.setPaintBorders(true);
        }
        
        button.setEnabled(true);
        button.setForeground(color);
        button.setPaintBorders(true);
        
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
    
}
