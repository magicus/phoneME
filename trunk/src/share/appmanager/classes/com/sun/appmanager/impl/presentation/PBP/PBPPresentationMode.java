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

package com.sun.appmanager.impl.presentation.PBP;

import com.sun.appmanager.AppManager;
import com.sun.appmanager.impl.CDCAmsAppController;
import com.sun.appmanager.impl.CDCAmsAppManager;
import com.sun.appmanager.apprepository.*;
import com.sun.appmanager.presentation.PresentationMode;
import java.awt.Frame;
import java.awt.Dimension;
import java.awt.FlowLayout;
import java.awt.BorderLayout;
import java.awt.GridLayout;
import java.awt.Container;
import java.awt.Graphics;
import java.awt.Component;
import java.util.*;
import java.awt.FontMetrics;
import java.awt.Font;
import java.awt.Color;
import java.text.SimpleDateFormat;
import com.sun.appmanager.mtask.Client;

public class PBPPresentationMode
    extends PresentationMode {

    static private final int BOTTOM_BAR = 38;

    static private final int SCREENSIZE_WIDTH = 640;
    static private final int SCREENSIZE_HEIGHT = 480 - BOTTOM_BAR;

    static private final int APPMANAGER_HEIGHT = 100;
    static private final int APPMANAGER_WIDTH = SCREENSIZE_WIDTH;

    static private final int DISPLAY_AREA_WIDTH = SCREENSIZE_WIDTH;
    static private final int DISPLAY_AREA_HEIGHT = SCREENSIZE_HEIGHT -
        APPMANAGER_HEIGHT;

    static private final int DISPLAY_AREA_X = 0;
    static private final int DISPLAY_AREA_Y = 0;

    static private final int APPMANAGER_X = 0;
    static private final int APPMANAGER_Y = DISPLAY_AREA_Y +
        DISPLAY_AREA_HEIGHT;

    static private final Color APP_BUTTON_COLOR = new Color(140, 158, 227);
    static private final Color ACTION_BUTTON_COLOR = new Color(186, 169, 203);

    private AppModule[] apps;
    private Vector startupApps = new Vector();
    private Hashtable appNameToAppModule = new Hashtable(13);

    Frame frame = null;
    Container appmanagerContainer = null;
    Container buttonContainer = null;
    Container actionContainer = null;

    Container activeAppsContainer = null;
    Container activeAppsEntryContainer = null;
    Container activeAppsButtonContainer = null;

    CDCAmsAppController controller = null;
    AppRepository appRepository = null;
    Client mtaskClient = null;

    PBPButton suspendButton = null;
    PBPButton resumeButton = null;
    PBPButton terminateButton = null;
    PBPButton showActiveButton = null;
    PBPButton exitButton = null;

    public static final Font DEFAULT_FONT = new Font("sanserif", Font.PLAIN, 12);
    public static final Color DEFAULT_COLOR = Color.blue;

    private ResourceBundle bundle = null;

    public void initialize() {

        this.appRepository = AppManager.getAppRepository();
        this.controller = CDCAmsAppManager.getAppController();
        this.mtaskClient = AppManager.getMtaskClient();

        //
        // Set xlet dimensions in mtask server.  This will be inherited
        // via sun.mtask.xlet.XletFrame in all launched children
        //
        controller.setXletDimensions(DISPLAY_AREA_X, DISPLAY_AREA_Y,
                                     DISPLAY_AREA_WIDTH, DISPLAY_AREA_HEIGHT, 0,
                                     0);

        frame = new Frame();
        frame.setUndecorated(true);

        // container for application buttons
        buttonContainer = new Container();
        buttonContainer.setLayout(new GridLayout(2, 3, 5, 5));

        // container for things like pause, start, kill, etc.
        actionContainer = new Container();
        actionContainer.setLayout(new GridLayout(0, 1));

        suspendButton = new PBPButton(getString("PBPPresentationMode.suspend"));
        suspendButton.setForeground(ACTION_BUTTON_COLOR);
        suspendButton.addPBPButtonListener(new PBPButtonListener() {
            public void buttonPressed(EventObject e) {
                controller.pauseTask(controller.getCurrentApp());
                actionContainer.remove(suspendButton);
                actionContainer.add(resumeButton, 0);
                actionContainer.validate();
                actionContainer.repaint();
            }
        });
        actionContainer.add(suspendButton);

        resumeButton = new PBPButton(getString("PBPPresentationMode.resume"));
        resumeButton.setForeground(ACTION_BUTTON_COLOR);
        resumeButton.addPBPButtonListener(new PBPButtonListener() {
            public void buttonPressed(EventObject e) {
                controller.startTask(controller.getCurrentApp());
                actionContainer.remove(resumeButton);
                actionContainer.add(suspendButton, 0);
                actionContainer.validate();
                actionContainer.repaint();
            }
        });

        terminateButton = new PBPButton(getString(
            "PBPPresentationMode.terminate"));
        terminateButton.addPBPButtonListener(new PBPButtonListener() {
            public void buttonPressed(EventObject e) {
                controller.killTask(controller.getCurrentApp());
                controller.reactivateTopTask();
            }
        });
        terminateButton.setForeground(ACTION_BUTTON_COLOR);
        actionContainer.add(terminateButton);

        showActiveButton = new PBPButton(getString(
            "PBPPresentationMode.showactive"));
        showActiveButton.addPBPButtonListener(new PBPButtonListener() {
            public void buttonPressed(EventObject e) {
                showActiveApps();
            }
        });
        showActiveButton.setForeground(ACTION_BUTTON_COLOR);
        actionContainer.add(showActiveButton);

        exitButton = new PBPButton(getString("PBPPresentationMode.exit"));
        exitButton.addPBPButtonListener(new PBPButtonListener() {
            public void buttonPressed(EventObject e) {
                controller.reboot();
            }
        });
        exitButton.setForeground(ACTION_BUTTON_COLOR);
        actionContainer.add(exitButton);

        activeAppsEntryContainer = new Container();
        activeAppsEntryContainer.setLayout(new GridLayout(0, 1));

        activeAppsButtonContainer = new Container();
        activeAppsButtonContainer.setLayout(new BorderLayout());

        activeAppsContainer = new Container();
        activeAppsContainer.setLayout(new BorderLayout());

        appmanagerContainer = new Container();
        appmanagerContainer.setLayout(new BorderLayout());
        appmanagerContainer.add(buttonContainer, BorderLayout.CENTER);
        appmanagerContainer.add(actionContainer, BorderLayout.EAST);

        frame.add(appmanagerContainer);

    };

    public void loadApps() {
        apps = appRepository.getAppList();
        for (int i = 0; i < apps.length; i++) {
            // add app
            addApp(apps[i]);

            // try to add the app to the menu it belongs to
            String appName = apps[i].getTitle();

            if (apps[i].getIsStartup()) {
                startupApps.add(appName);
            }
        }
    }

    public void addApp(AppModule module) {
        associateAppModule(module.getTitle(), module);
        PBPButton button = createButton(module);
        buttonContainer.add(button);
    }

    public void startAppManager() {
        frame.setBounds(APPMANAGER_X, APPMANAGER_Y, APPMANAGER_WIDTH,
                        APPMANAGER_HEIGHT);
        frame.setVisible(true);
    }

    public void runStartupApps() {
        Object[] apps = startupApps.toArray();
        for (int i = 0; i < apps.length; i++) {
            runStartupApp( (AppModule) getAppModule( (String) apps[i]));
            try {
                Thread.sleep(1000);
            }
            catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    public void runStartupApp(AppModule app) {
        launchApp(app);
    }

    void launchApp(AppModule app) {
        if (!actionContainer.isAncestorOf(suspendButton)) {
            actionContainer.remove(resumeButton);
            actionContainer.add(suspendButton, 0);
            actionContainer.validate();
            actionContainer.repaint();
        }

        String[] apps = controller.getMtaskClient().getLaunchedApps();
        for (int i = 0; i < apps.length; i++) {
            AppModule module = controller.getAppModule(apps[i]);
            if (module.equals( (Object) app)) {
                if (!controller.getCurrentApp().equals(apps[i])) {
                    if (app.getType().equals("XLET")) {
                        controller.activate(apps[i]);
                    }
                }
                return;
            }
        }

        // PBP requires that the PBP_SCREEN_BOUNDS variable be set for apps that we don't
        // want to display full screen.
        String str = "PBP_SCREEN_BOUNDS=" + DISPLAY_AREA_X + "," +
            DISPLAY_AREA_Y + "-" + DISPLAY_AREA_WIDTH + "x" +
            DISPLAY_AREA_HEIGHT;
        mtaskClient.setenv(str);
        String appId = controller.launchApp(app);

    }

    private PBPButton createButton(AppModule app) {
        final AppModule module = app;
        PBPButton button = new PBPButton(app.getTitle());
        button.setForeground(APP_BUTTON_COLOR);
        button.addPBPButtonListener(new PBPButtonListener() {
            public void buttonPressed(EventObject e) {
                launchApp(module);
            }
        });
        return button;
    }

    private AppModule getAppModule(String name) {
        return (AppModule) appNameToAppModule.get(name);
    }

    private void associateAppModule(String name, AppModule module) {
        appNameToAppModule.put(name, module);
    }

    private void disassociateAppModule(String name) {
        appNameToAppModule.remove(name);
    }

    private void showActiveApps() {
        frame.remove(appmanagerContainer);

        String[] activeApps = controller.getMtaskClient().getLaunchedApps();
        for (int i = 0; i < activeApps.length; i++) {
            Date today;
            String dateOutput;
            SimpleDateFormat formatter;
            String pattern = "EEE, MMM d, ''yy h:mm a";
            formatter = new SimpleDateFormat(pattern);
            today = new Date();
            dateOutput = formatter.format(today);

            AppModule module = controller.getAppModule(activeApps[i]);
            String str = module.getTitle() + ": " + dateOutput;
            activeAppsEntryContainer.add(new ActiveAppsEntry(str));
        }

        PBPButton closeButton = new PBPButton("Close");
        closeButton.addPBPButtonListener(new PBPButtonListener() {
            public void buttonPressed(EventObject e) {
                showMainScreen();
            }
        });

        activeAppsButtonContainer.add(closeButton);

        activeAppsContainer.add(activeAppsEntryContainer, BorderLayout.CENTER);
        activeAppsContainer.add(activeAppsButtonContainer, BorderLayout.EAST);

        frame.add(activeAppsContainer, BorderLayout.CENTER);
        frame.validate();
        frame.repaint();
    }

    private void showMainScreen() {
        activeAppsContainer.removeAll();
        activeAppsEntryContainer.removeAll();
        activeAppsButtonContainer.removeAll();
        frame.remove(activeAppsContainer);
        frame.add(appmanagerContainer, BorderLayout.CENTER);
        frame.validate();
        frame.repaint();
    }

    class ActiveAppsLabel
        extends Component {
        String label = null;

        public ActiveAppsLabel(String label) {
            this.label = label;
            setBackground(ACTION_BUTTON_COLOR);
        }

        public void paint(Graphics g) {
            Dimension d = getSize();
            g.setFont(DEFAULT_FONT);
            FontMetrics fm = g.getFontMetrics(DEFAULT_FONT);
            int fw = fm.stringWidth(label);
            int fa = fm.getAscent();
            g.drawString(label, (d.width - fw) / 2, (d.height + fa) / 2 - 1);
        }
    }

    class ActiveAppsEntry
        extends Component {
        String label = null;

        public ActiveAppsEntry(String label) {
            this.label = label;
            setBackground(Color.white);
        }

        public void paint(Graphics g) {
            Dimension d = getSize();
            g.setFont(DEFAULT_FONT);
            FontMetrics fm = g.getFontMetrics(DEFAULT_FONT);
            int fw = fm.stringWidth(label);
            int fa = fm.getAscent();
            g.drawString(label, (d.width - fw) / 2, (d.height + fa) / 2 - 1);
        }
    }

    /**
     * This method returns a string from the demo's resource bundle.
     */
    String getString(String key) {
        String value = null;

        try {
            value = getResourceBundle().getString(key);
        }
        catch (MissingResourceException e) {
            System.out.println("Could not find key for " + key);
	    e.printStackTrace();
        }

        return value;
    }

    int getInt(String key) {
        return Integer.parseInt(getString(key));
    }

    private ResourceBundle getResourceBundle() {
        if (bundle == null) {
            bundle = ResourceBundle.getBundle(
                "com.sun.appmanager.impl.presentation.PBP.resources.pbppresentationmode");
        }

        return bundle;
    }

}
