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

import com.sun.jump.common.JUMPApplication;
import com.sun.jump.common.JUMPContent;
import com.sun.jump.module.installer.JUMPInstallerModule;
import com.sun.jump.module.installer.JUMPInstallerModuleFactory;
import com.sun.jumpimpl.module.download.OTADiscovery;
import com.sun.jumpimpl.module.installer.JUMPInstallerTool;
import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Container;
import java.awt.Dimension;
import java.awt.Font;
import java.awt.FontMetrics;
import java.awt.Graphics;
import java.awt.GridLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Vector;

public class SimpleBasisAMSInstall {
    
    private String downloadNames[] = null;
    private String downloadURIs[] = null;
    private SimpleBasisAMS ams = null;
    
    private final int INSTALL_SCREEN = 10;
    private final int INSTALL_APPS_LIST_SCREEN = 11;
    private final int REMOVE_SCREEN = 12;
    private final int REMOVE_APPS_LIST_SCREEN = 13;
    private final int STATUS_SCREEN = 14;
    
    private static final Color INSTALL_SCREEN_COLOR = new Color(209, 191, 217);
    private static final Color REMOVE_SCREEN_COLOR = new Color(201, 158, 119);
    private static final Color STATUS_SCREEN_COLOR = new Color(219, 241, 153);
    
    private static final Color BUTTON_BLUE_COLOR = new Color(86, 135, 248);
    private static final Color BUTTON_YELLOW_COLOR = new Color(229, 231, 44);
    private static final Color BUTTON_RED_COLOR = new Color(188, 85, 68);
    
    private static final int APPS_LIST_SCREEN_ROWS = 7;
    private static final int APPS_LIST_SCREEN_COLUMNS = 1;
    private static final int APPS_LIST_SCREEN_NUM_ENTRIES = APPS_LIST_SCREEN_ROWS * APPS_LIST_SCREEN_COLUMNS;
    
    int installAppsListScreenPageNumber = 0;
    int removeAppsListScreenPageNumber = 0;
    InstallationStatusScreen installationStatusScreen = null;
    
    private JUMPInstallerTool tool = null;
    private Map map = null;
    
    private final Font HEADER_FONT = new Font("Arial", Font.BOLD | Font.ITALIC, 24);
    
    private String provisionURL = null;
    
    public SimpleBasisAMSInstall(SimpleBasisAMS ams) {
        this.ams = ams;
        this.map = ams.map;
        tool = new JUMPInstallerTool(parseToolProperties(map));
    }
    
    private void trace(String str) {
        ams.trace(str);
    }
    
    private String[] parseToolProperties(Map map) {
        
        // check if verbose mode is used
        provisionURL = System.getProperty("jump.installer.provisionURL");
        if (provisionURL == null && map != null) {
            provisionURL = (String) map.get("jump.installer.provisionURL");
        }
        String str[] = new String[1];
        str[0] = "-ProvisioningServerURL " + provisionURL;
        return str;
    }
    
    public void pageUp() {
        int currentScreen = ams.getCurrentScreen();
        
        if (currentScreen == INSTALL_APPS_LIST_SCREEN) {
            // Determine if there is possibly more icons to display
            // beyond this page
            if (installAppsListScreenPageNumber > 0) {
                installAppsListScreenPageNumber--;
                showInstallAppListScreen();
            }
        } else if (currentScreen == REMOVE_APPS_LIST_SCREEN) {
            // Determine if there is possibly more icons to display
            // beyond this page
            if (removeAppsListScreenPageNumber > 0) {
                removeAppsListScreenPageNumber--;
                showRemoveAppListScreen();
            }
        }
    }
    
    public void pageDown() {
        int currentScreen = ams.getCurrentScreen();
        
        if (currentScreen == INSTALL_APPS_LIST_SCREEN) {
            // Find out number of total screen pages
            int totalInstallAppsListScreenPages = ams.getTotalScreenPages(installAppsListScreenButtons.length, APPS_LIST_SCREEN_ROWS * APPS_LIST_SCREEN_COLUMNS);
            
            // Don't scroll beyond the last page
            if (installAppsListScreenPageNumber < (totalInstallAppsListScreenPages - 1)) {
                installAppsListScreenPageNumber++;
                showInstallAppListScreen();
            }
        } else if (currentScreen == REMOVE_APPS_LIST_SCREEN) {
            // Find out number of total screen pages
            int totalRemoveAppsListScreenPages = ams.getTotalScreenPages(removeAppsListScreenButtons.length, ams.SCREEN_DISPLAY_ICONS);
            
            // Don't scroll beyond the last page
            if (removeAppsListScreenPageNumber < (totalRemoveAppsListScreenPages - 1)) {
                removeAppsListScreenPageNumber++;
                showRemoveAppListScreen();
            }
        }
    }
    
    /****************************** DISCOVERY ****************************/
    
    private void discover() {
        HashMap applist = new OTADiscovery().discover(provisionURL);
        
        downloadURIs = new String[ applist.size() ];
        downloadNames = new String[ applist.size() ];
        
        int i = 0;
        for ( Iterator e = applist.keySet().iterator(); e.hasNext(); ) {
            String s = (String)e.next();
            downloadURIs[ i ] = s;
            downloadNames[ i ] = (String)applist.get( s );
            i++;
        }
    }
    
    class DiscoveryInstallAllActionListener implements ActionListener {
        /* Because this actionPeformed() will be run within the AWT thread,
         * we need to run the following in a different thread as it is
         * not a good idea to draw within the AWT thread.
         */
        public void actionPerformed(ActionEvent e) {
            new Thread() {
                public void run() {
                    discover();
                    showInstallationStatusScreen();
                    for (int i = 0; i < downloadNames.length; i++) {
                        try {
                            installationStatusScreen.drawStatusString("Attemping to install: ", downloadNames[i], "INSTALLING");
                            Thread.sleep(100);
                            JUMPContent content[] = tool.doInstall(downloadNames[i], downloadURIs[i]);
                            if (content == null || content.length == 0) {
                                installationStatusScreen.drawStatusString("Completed install: ", downloadNames[i], "FAILED");
                            } else {
                                installationStatusScreen.drawStatusString("Completed install: ", downloadNames[i], "SUCCESS");
                            }
                            Thread.sleep(100);
                        } catch (InterruptedException ex) {
                            ex.printStackTrace();
                        }
                    }
                    installationStatusScreen.drawStatusDone("Install completed.", new ShowInstallScreenActionListener(), new ApplicationsScreenActionListener());
                    try {
                        Thread.sleep(100);
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                    ams.refreshApplicationsScreen();
                }
            }.start();
        }
    }
    
    class DiscoveryInstall1ActionListener implements ActionListener {
        public void actionPerformed(ActionEvent e) {
            discover();
            showInstallAppListScreen();
        }
    }
    
    /****************************** STATUS SCREEN ****************************/
    
    private void showInstallationStatusScreen() {
        ams.setCurrentScreen(STATUS_SCREEN);
        ams.showScreen(createInstallationStatusScreen());
    }
    
    private Container createInstallationStatusScreen() {
        installationStatusScreen = new InstallationStatusScreen();
        return installationStatusScreen;
    }
    
    /*
     * This is the status screen seen during installation and removal
     */
    class InstallationStatusScreen extends Container {
        static final int START_X = 5;
        static final int START_Y = 5;
        int currentX = 0;
        int currentY = 0;
        String label = null;
        String appName = null;
        String status = null;
        boolean clearFlag = false;
        boolean done = false;
        Container buttonContainer = null;
        Container textContainer = null;
        final Color darkGreen = new Color(72, 142, 87);
        
        public InstallationStatusScreen() {
            currentX = START_X;
            currentY = START_Y;
            clearFlag = true;
            removeAll();
            setLayout(new BorderLayout());
            
            buttonContainer = new Container() {
                public Dimension getPreferredSize() {
                    return new Dimension(240, 50);
                }
            };
            buttonContainer.setLayout(new GridLayout(1, 2));
            
            textContainer = new Container() {
                public void paint(Graphics g) {
                    Dimension d = getSize();
                    g.setColor(STATUS_SCREEN_COLOR);
                    if (clearFlag) {
                        g.fillRect(0, 0, d.width, d.height);
                        g.setColor(Color.black);
                        g.fillRect(0, 0, d.width, d.height / 4);
                        g.setColor(Color.white);
                        g.setFont(HEADER_FONT);
                        FontMetrics fm = getFontMetrics(HEADER_FONT);
                        int fheight = (int)fm.getHeight();
                        g.drawString("STATUS WINDOW", 5, ((d.height / 4) / 2) + (fheight / 2));
                    }
                    g.setColor(Color.black);
                    g.setFont(new Font("Helvetica", Font.PLAIN, 20));
                    if (!done) {
                        if (label != null) {
                            g.drawString(label, 5, 120);
                        }
                        g.setColor(BUTTON_BLUE_COLOR);
                        if (appName != null) {
                            g.drawString(appName, 20, 150);
                        } else {
                            g.drawString("<untitled>", 20, 150);
                        }
                        if (status != null) {
                            g.setColor(darkGreen);
                            g.drawString("Status: " + status, 5, 180);
                        } else {
                            g.setColor(Color.red);
                            g.drawString("Status: UNKNOWN", 5, 180);
                        }
                    } else {
                        if (label != null) {
                            g.drawString(label, 5, 80);
                        }
                    }
                }
                
                public void update(Graphics g) {
                    paint(g);
                }
            };
            add(textContainer, BorderLayout.CENTER);
        }
        
        private void drawStatusDone(String label, ActionListener backAction,
                ActionListener okAction) {
            this.label = label;
            buttonContainer.add(createButton("Back", backAction));
            buttonContainer.add(createButton("Done", okAction));
            add(buttonContainer, BorderLayout.SOUTH);
            done = true;
            validate();
            repaint();
        }
        
        private SimpleBasisAMSImageButton createButton(String label, ActionListener action) {
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
            return button;
        }
        
        synchronized public void drawStatusString(String label, String appName, String status) {
            this.label = label;
            this.appName = appName;
            this.status = status;
            done = false;
            repaint();
        }
        
    }
    
    
    /****************************** INSTALL SCREEN ****************************/
    
    public void showInstallScreen() {
        ams.showScreen(createInstallScreen());
        ams.setCurrentScreen(INSTALL_SCREEN);
    }
    
    Container createInstallScreen() {
        Container showURLContainer = new InstallScreen();
        return showURLContainer;
    }
    
    class ShowInstallScreenActionListener implements ActionListener {
        public void actionPerformed(ActionEvent e) {
            showInstallScreen();
        }
    }
    
    class ApplicationsScreenActionListener implements ActionListener {
        public void actionPerformed(ActionEvent e) {
            ams.doApplicationsScreen();
        }
    }
    
    class ShowInstallApplistScreenActionListener implements ActionListener {
        public void actionPerformed(ActionEvent e) {
            showInstallAppListScreen();
        }
    }
    
        /*
         * This is the screen the user sees when selecting the Install command button
         */
    class InstallScreen extends Container {
        private static final String message1 = "Choose All to install all apps.";
        private static final String message2 = "Choose One to install from a list of apps.";
        private static final String message3 = "Choose Cancel to exit without installing.";
        
        public InstallScreen() {
            
            setLayout(new BorderLayout());
            Container buttonContainer = new Container();
            if (provisionURL != null) {
                buttonContainer.setLayout(new GridLayout(1, 3));
                buttonContainer.add(createButton("All", new DiscoveryInstallAllActionListener()));
                buttonContainer.add(createButton("One", new DiscoveryInstall1ActionListener()));
                buttonContainer.add(createButton("Cancel", new ApplicationsScreenActionListener()));
            }
            
            Container textContainer = new Container() {
                public void paint(Graphics g) {
                    super.paint(g);
                    Dimension d = getSize();
                    g.setColor(INSTALL_SCREEN_COLOR);
                    g.fillRect(0, 0, d.width, d.height);
                    g.setColor(Color.black);
                    g.fillRect(0, 0, d.width, d.height / 4);
                    g.setColor(Color.white);
                    g.setFont(HEADER_FONT);
                    FontMetrics fm = getFontMetrics(HEADER_FONT);
                    int fheight = (int)fm.getHeight();
                    g.drawString("Install Applications", 5, ((d.height / 4) / 2) + (fheight / 2));
                    g.setColor(Color.black);
                    g.setFont(new Font("Helvetica", Font.PLAIN, 20));
                    g.drawString("Discover URL:", 5, 80);
                    g.setFont(new Font("Monospaced", Font.PLAIN, 12));
                    if (provisionURL != null) {
                        g.drawString(provisionURL, 5, 100);
                        g.setFont(new Font("Helvetica", Font.PLAIN, 12));
                        g.drawString(message1, 5, 140);
                        g.drawString(message2, 5, 160);
                        g.drawString(message3, 5, 180);
                    } else {
                        g.drawString("No Provisioning URL provided.", 5, 100);
                        g.drawString("The property jump.installer.provisionURL", 5, 140);
                        g.drawString("must be provided to the JUMP executive", 5, 160);
                        g.drawString("with a valid provisioning server URL.", 5, 180);
                    }
                    
                }
            };
            textContainer.setBackground(INSTALL_SCREEN_COLOR);
            add(textContainer, BorderLayout.CENTER);
            add(buttonContainer, BorderLayout.SOUTH);
        }
        
        private SimpleBasisAMSImageButton createButton(String label, ActionListener action) {
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
            return button;
        }
    }
    
    private void showInstallAppListScreen() {
        Container installAppsListScreen = createInstallAppsListScreen();
        ams.showScreen(installAppsListScreen);
        ams.setCurrentScreen(INSTALL_APPS_LIST_SCREEN);
    }
    
    
    private SimpleBasisAMSImageButton installAppsListScreenButtons[] = null;
    Container createInstallAppsListScreen() {
        Container appsListScreen = new Container();
        appsListScreen.setLayout(new GridLayout(APPS_LIST_SCREEN_ROWS, APPS_LIST_SCREEN_COLUMNS));
        installAppsListScreenButtons = new SimpleBasisAMSImageButton[downloadNames.length];
        for (int i = 0; i < downloadNames.length; i++) {
            installAppsListScreenButtons[i] = ams.createScreenButton(downloadNames[i], new InstallOneActionListener(i), BUTTON_YELLOW_COLOR);
        }
        int firstPositionIndex = installAppsListScreenPageNumber * APPS_LIST_SCREEN_NUM_ENTRIES;
        for (int i = firstPositionIndex;
        i < (installAppsListScreenPageNumber * APPS_LIST_SCREEN_NUM_ENTRIES + APPS_LIST_SCREEN_NUM_ENTRIES); i++) {
            if (i < installAppsListScreenButtons.length) {
                appsListScreen.add(installAppsListScreenButtons[i]);
            } else {
                appsListScreen.add(ams.createScreenButton(null, BUTTON_YELLOW_COLOR));
            }
        }
        return appsListScreen;
    }
    
    class InstallOneActionListener
            implements ActionListener {
        int index;
        
        public InstallOneActionListener(int i) {
            this.index = i;
        }
        
        public void actionPerformed(ActionEvent e) {
            new Thread() {
                public void run() {
                    try {
                        showInstallationStatusScreen();
                        installationStatusScreen.drawStatusString("Attempting to install: ", downloadNames[index], "INSTALLING");
                        Thread.sleep(100);
                        JUMPContent content[] = tool.doInstall(downloadNames[index], downloadURIs[index]);
                        if (content == null || content.length == 0) {
                            installationStatusScreen.drawStatusString("Installing Application: ", downloadNames[index], "PROCESSING...");
                        } else {
                            installationStatusScreen.drawStatusString("Installing Application: ", downloadNames[index], "DONE");
                        }
                        Thread.sleep(100);
                    } catch (InterruptedException ex) {
                        ex.printStackTrace();
                    }
                    installationStatusScreen.drawStatusDone("Finished installing: " + downloadNames[index],
                            new ShowInstallApplistScreenActionListener(), new ApplicationsScreenActionListener());
                    try {
                        Thread.sleep(100);
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                    ams.refreshApplicationsScreen();
                }
            }.start();
        }
    }
    
    /****************************** REMOVE SCREEN ****************************/
    
    public void showRemoveScreen() {
        ams.showScreen(createRemoveScreen());
        ams.setCurrentScreen(REMOVE_SCREEN);
    }
    
    Container createRemoveScreen() {
        Container removeContainer = new RemoveScreen();
        return removeContainer;
    }
    
    /*
     * This the screen the user sees after selecting the Remove command button
     */
    class RemoveScreen extends Container {
        
        public RemoveScreen() {
            
            setLayout(new BorderLayout());
            Container buttonContainer = new Container();
            buttonContainer.setLayout(new GridLayout(1, 3));
            buttonContainer.add(createButton("All", new RemoveAllActionListener()));
            buttonContainer.add(createButton("One", new Remove1ScreenActionListener()));
            buttonContainer.add(createButton("Cancel", new ApplicationsScreenActionListener()));
            
            Container textContainer = new Container() {
                public void paint(Graphics g) {
                    super.paint(g);
                    Dimension d = getSize();
                    g.setColor(REMOVE_SCREEN_COLOR);
                    g.fillRect(0, 0, d.width, d.height);
                    g.setColor(Color.black);
                    g.fillRect(0, 0, d.width, d.height / 4);
                    g.setColor(Color.white);
                    g.setFont(HEADER_FONT);
                    FontMetrics fm = getFontMetrics(HEADER_FONT);
                    int fheight = (int)fm.getHeight();
                    g.drawString("Remove Applications", 5, ((d.height / 4) / 2) + (fheight / 2));
                    g.setColor(Color.black);
                    g.setFont(new Font("Helvetica", Font.PLAIN, 12));
                    g.drawString("Remove Applications Screen:", 5, 110);
                    g.drawString("Remove all, Remove 1, or Cancel?", 5, 130);
                }
            };
            textContainer.setBackground(REMOVE_SCREEN_COLOR);
            add(textContainer, BorderLayout.CENTER);
            add(buttonContainer, BorderLayout.SOUTH);
        }
        
        private SimpleBasisAMSImageButton createButton(String label, ActionListener action) {
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
            return button;
        }
    }
    
    class Remove1ScreenActionListener implements ActionListener {
        public void actionPerformed(ActionEvent e) {
            showRemoveAppListScreen();
        }
    }
    
    class ShowRemoveApplistScreenActionListener implements ActionListener {
        public void actionPerformed(ActionEvent e) {
            showRemoveAppListScreen();
        }
    }
    
    class ShowRemoveScreenActionListener implements ActionListener {
        public void actionPerformed(ActionEvent e) {
            showRemoveScreen();
        }
    }
    
    class RemoveOneActionListener implements ActionListener {
        JUMPApplication app = null;
        public RemoveOneActionListener(JUMPApplication app) {
            this.app = app;
        }
        
        /* Because this actionPeformed() will be run within the AWT thread,
         * we need to run the following in a different thread as it is
         * not a good idea to draw within the AWT thread.
         */
        public void actionPerformed(ActionEvent e) {
            new Thread() {
                public void run() {
                    try {
                        showInstallationStatusScreen();
                        installationStatusScreen.drawStatusString("Removing Application: ", app.getTitle(), "PROCESSING...");
                        Thread.sleep(100);
                        if (!ams.isRunningApp(app)) {
                            tool.doUninstall(app);
                            installationStatusScreen.drawStatusString("Removing Application: ", app.getTitle(), "DONE");
                        } else {
                            installationStatusScreen.drawStatusString("Removing Application: ", app.getTitle(), "ERROR - application running");
                        }
                        Thread.sleep(100);
                    } catch (InterruptedException ex) {
                        ex.printStackTrace();
                    }
                    installationStatusScreen.drawStatusDone("Finished removing application: " + app.getTitle(),
                            new ShowRemoveApplistScreenActionListener(), new ApplicationsScreenActionListener());
                    try {
                        Thread.sleep(100);
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                    ams.refreshApplicationsScreen();
                }
            }.start();
        }
    }
    
    class RemoveAllActionListener implements ActionListener {
        public void actionPerformed(ActionEvent e) {
        /* Because this actionPeformed() will be run within the AWT thread,
         * we need to run the following in a different thread as it is
         * not a good idea to draw within the AWT thread.
         */
            new Thread() {
                public void run() {
                    try {
                        showInstallationStatusScreen();
                        JUMPInstallerModule installers[] = JUMPInstallerModuleFactory.getInstance().getAllInstallers();
                        for (int i = 0; i < installers.length; i++) {
                            JUMPContent[] content = installers[i].getInstalled();
                            while (content != null && content.length > 0) {
                                if (content[0] != null) {
                                    JUMPApplication app = (JUMPApplication)content[0];
                                    installationStatusScreen.drawStatusString("Removing Application: ", app.getTitle(), "PROCESSING...");
                                    Thread.sleep(100);
                                    if (!ams.isRunningApp(app)) {
                                        tool.doUninstall(app);
                                        installationStatusScreen.drawStatusString("Removing Application: ", app.getTitle(), "DONE");
                                    } else {
                                        installationStatusScreen.drawStatusString("Removing Application: ", app.getTitle(), "ERROR - application running");
                                    }
                                    Thread.sleep(100);
                                }
                                content = installers[i].getInstalled();
                            }
                        }
                    } catch (InterruptedException ex) {
                        ex.printStackTrace();
                    }
                    installationStatusScreen.drawStatusDone("Remove completed.",
                            new ShowRemoveScreenActionListener(), new ApplicationsScreenActionListener());
                    try {
                        Thread.sleep(100);
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                    ams.refreshApplicationsScreen();
                }
            }.start();
        }
    }
    
    private void showRemoveAppListScreen() {
        Container removeAppsListScreen = createRemoveAppsListScreen();
        ams.showScreen(removeAppsListScreen);
        ams.setCurrentScreen(REMOVE_APPS_LIST_SCREEN);
    }
    
    private SimpleBasisAMSImageButton removeAppsListScreenButtons[] = null;
    Container createRemoveAppsListScreen() {
        Container removeAppsListScreen = new Container();
        removeAppsListScreen.setLayout(new GridLayout(ams.SCREEN_ROWS, ams.SCREEN_COLUMNS));
        
        JUMPInstallerModule installers[] = JUMPInstallerModuleFactory.getInstance().getAllInstallers();
        Vector allApps = new Vector();
        for (int i = 0; i < installers.length; i++) {
            JUMPContent[] apps = installers[i].getInstalled();
            for (int j = 0; j < apps.length; j++) {
                allApps.add(apps[j]);
            }
        }
        JUMPContent content[] = (JUMPContent[])allApps.toArray(new JUMPContent[]{});
        removeAppsListScreenButtons = new SimpleBasisAMSImageButton[content.length];
        for (int i = 0; i < content.length; i++) {
            JUMPApplication app = (JUMPApplication)content[i];
            removeAppsListScreenButtons[i] = ams.createScreenButton(app, new RemoveOneActionListener(app), BUTTON_RED_COLOR);
        }
        int firstPositionIndex = removeAppsListScreenPageNumber * ams.SCREEN_DISPLAY_ICONS;
        for (int i = firstPositionIndex;
        i < (removeAppsListScreenPageNumber * ams.SCREEN_DISPLAY_ICONS + ams.SCREEN_DISPLAY_ICONS); i++) {
            if (i < removeAppsListScreenButtons.length) {
                removeAppsListScreen.add(removeAppsListScreenButtons[i]);
            } else {
                removeAppsListScreen.add(ams.createScreenButton(null, BUTTON_RED_COLOR));
            }
        }
        return removeAppsListScreen;
    }
    
}

