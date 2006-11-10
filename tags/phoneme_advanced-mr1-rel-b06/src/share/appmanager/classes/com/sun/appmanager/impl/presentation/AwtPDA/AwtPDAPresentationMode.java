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

package com.sun.appmanager.impl.presentation.AwtPDA;

import com.sun.appmanager.AppManager;
import com.sun.appmanager.mtask.TaskListener;
import com.sun.appmanager.impl.CDCAmsAppController;
import com.sun.appmanager.impl.CDCAmsAppManager;
import com.sun.appmanager.apprepository.*;
import com.sun.appmanager.presentation.PresentationMode;
import com.sun.appmanager.preferences.Preferences;
import java.awt.*;
import java.awt.event.*;
import java.util.*;
import java.net.*;

public class AwtPDAPresentationMode
    extends PresentationMode {

    AppManagerFrame topFrame = null;
    ContentFrame bottomFrame = null;
    private AppRepository appRepository = null;
    private final Rectangle topFrameMaximizedSize = new Rectangle(0, 0, 480,
        552);
    private final Rectangle topFrameMinimizedSize = new Rectangle(0, 0, 480, 81);
    final Color backgroundColor = new Color(75, 75, 86);
    private ResourceBundle bundle = null;
    private AwtPDAScrollPane scrollPane = null;
    private ImageButton exitButton = null;
    private ImageButton backButton = null;
    private ImageButton downloaderButton = null;
    private ImageButton prefsButton = null;
    CDCAmsAppController controller = null;
    private Hashtable appNameToButton = new Hashtable(13);
    private Hashtable appNameToAppModule = new Hashtable(13);
    private Hashtable menuNameToAppNames = new Hashtable(13);
    private AwtPDAPresentationTaskbar taskbar = null;
    private AppModule[] apps;
    private Vector startupSystemApps = new Vector();
    private Vector startupUserApps = new Vector();
    private ToolbarPanel tb = null;
    private Font font = null;
    private Stack menuStack = new Stack();
    private Stack scrollPositionStack = new Stack();
    private int currentScrollPosition = 0;
    private String currentMenu = null;
    private IconPanel iconPanel = null;
    ContentPanel contentPanel = null;
    TransparentPanel outerMostPanel = null;
    private ImageButton taskBarButton = null;
    private ImageButton leftScrollButton = null;
    private ImageButton leftGrayedScrollButton = null;
    private ImageButton rightScrollButton = null;
    private ImageButton rightGrayedScrollButton = null;
    private ImageButton systemMenuButton = null;
    private ImagePanel backButtonPanel = null;
    private ImagePanel scrollButtonsPanel = null;
    private AwtPDAPresentationPrefs prefsScreen = null;
    private AwtPDAPresentationDownloader downloader = null;
    AwtPDAPresentationLogin login = null;
    private AwtPDAPresentationSplash splash = null;
    Preferences preferences = null;
    private Gauge initializationGauge = null;

    String currentUser = null;
    int currentUserFontSize = 16;
    String currentUserFontStr = null;
    Font currentUserFont = null;
    GetAppListThread appListThread = null;
    InitDuringLoginThread initDuringLoginThread = null;

    /*********************************** Images ***********************************/
    MediaTracker tracker = null;

    private String backButtonPanelBackgroundImageFile = "resources/background/"
        + getString("AwtPDAPresentationMode.leftRaisedPlatform");

    private String scrollPanelBackgroundImageFile = "resources/background/"
        + getString("AwtPDAPresentationMode.rightRaisedPlatform");

    private String toolbarPanelBackgroundImageFile = "resources/background/"
        + getString("AwtPDAPresentationMode.toolbarBackground");
    Image toolbarPanelBackgroundImage = null;

    private String smallButtonFile = "resources/downloader/" +
        getString("AwtPDAPresentationMode.downloaderSmallButton");
    Image smallButtonImage = null;

    private String mediumButtonFile = "resources/downloader/" +
        getString("AwtPDAPresentationMode.downloaderMediumButton");
    Image mediumButtonImage = null;

    private String largeButtonFile = "resources/downloader/" +
        getString("AwtPDAPresentationMode.downloaderLargeButton");
    Image largeButtonImage = null;

    private String backButtonImageFile = getString(
        "AwtPDAPresentationMode.backButton");
    Image backButtonImage = null;

    /******************************************************************************/

    public class ToolbarPanel
        extends Panel {
        Image backgroundImage = null;
        ScrollPane scrollPane = null;

        public ToolbarPanel(Image image, ScrollPane scrollPane) {
            super();
            this.scrollPane = scrollPane;
            backgroundImage = image;
        }

        public void paint(Graphics g) {
            int clipWidthOffset = 0;
            if (iconPanel.isAncestorOf(scrollButtonsPanel)) {
                clipWidthOffset += scrollButtonsPanel.getWidth();
            }
            if (iconPanel.isAncestorOf(backButtonPanel)) {
                clipWidthOffset += backButtonPanel.getWidth();
            }

            if (clipWidthOffset > 0) {
                g.setClip(scrollPane.getHAdjustable().getValue(), 0,
                          480 - clipWidthOffset, 81);
                g.drawImage(backgroundImage,
                            scrollPane.getHAdjustable().getValue(),
                            0, 480, 81, null);
                super.paint(g);

            }
            else {
                g.drawImage(backgroundImage,
                            scrollPane.getHAdjustable().getValue(),
                            0, 480, 81, null);
                super.paint(g);
            }
        }

        public void update(Graphics g) {
        }
    }

    class IconPanel
        extends Container {

        Border border = null;

        public IconPanel() {
            super();
        }

        public void setBorder(Border newBorder) {
            if (border != newBorder) {
                border = newBorder;
                repaint();
            }
        }

        public Dimension getPreferredSize() {
            return new Dimension(480, 81);
        }

        public Dimension getMinimumSize() {
            return getPreferredSize();
        }

        public Dimension getMaximumSize() {
            return getPreferredSize();
        }

    };

    class ContentPanel
        extends Panel {
        public ContentPanel() {
            super();
        }

        public Dimension getPreferredSize() {
            return new Dimension(480, 552 - 81);
        }

        public Dimension getMinimumSize() {
            return getPreferredSize();
        }

        public Dimension getMaximumSize() {
            return getPreferredSize();
        }
    }

    class ContentFrame
        extends Frame {
        public ContentFrame() {
            super();
            setUndecorated(true);
        }
    }

// The intent here is to load images that may be used in other classes.
// Therefore, these images should be shared images.
    public void loadImages() {

        tracker = new MediaTracker(getFrame());

        toolbarPanelBackgroundImage = createImage(
            toolbarPanelBackgroundImageFile);

        smallButtonImage = createImage(smallButtonFile);
        mediumButtonImage = createImage(mediumButtonFile);
        largeButtonImage = createImage(largeButtonFile);
        backButtonImage = createIconImage(backButtonImageFile);

        try {
            tracker.addImage(toolbarPanelBackgroundImage, 0);
            tracker.addImage(smallButtonImage, 2);
            tracker.addImage(mediumButtonImage, 3);
            tracker.addImage(largeButtonImage, 4);
            tracker.addImage(backButtonImage, 5);

            for (int i = 0; i < 6; i++) {
                tracker.waitForID(i);
            }
        }
        catch (InterruptedException e) {
            e.printStackTrace();
            return;
        }

    }

    public void initialize() {
        topFrame = new AppManagerFrame();
        topFrame.setBackground(backgroundColor);
        topFrame.setLayout(new BorderLayout());

        // Create and show splash screen
        splash = new AwtPDAPresentationSplash(this);

        // Create the animation used during initialization
        initializationGauge = new Gauge(this,
                                        getString(
                                            "AwtPDAPresentationMode.initializing"));

        showSplashScreen();

        this.appRepository = AppManager.getAppRepository();

        // The actions associated with searching for repository applications
        // will be done in another thread since this can be time consuming.
        appListThread = new GetAppListThread();
        appListThread.start();

        loadImages();

        this.controller = CDCAmsAppManager.getAppController();

        this.preferences = AppManager.getPreferences();

        //
        // Set xlet dimensions in mtask server.  This will be inherited
        // via sun.mtask.xlet.XletFrame in all launched children
        //
        controller.setXletDimensions(0, 81, 480, 472, 0, 0);

        // Create additional screens
        login = new AwtPDAPresentationLogin(this);
        taskbar = new AwtPDAPresentationTaskbar(controller, this);
        prefsScreen = new AwtPDAPresentationPrefs(this);
        downloader = new AwtPDAPresentationDownloader(this);

        // Create scrollpane for icon toolbar
        scrollPane = new AwtPDAScrollPane(ScrollPane.SCROLLBARS_NEVER);

        // Create icon toolbar
        tb = new ToolbarPanel(toolbarPanelBackgroundImage, scrollPane);
        tb.setLayout(new FlowLayout(FlowLayout.LEFT));

        scrollPane.add(tb);

        /***************** Back Button *****************/
        backButton = new ImageButton(backButtonImage);
        backButton.setSize(50, 75);
        backButton.setLabel(getString("AwtPDAPresentationMode.backButton.label"));
        backButton.setTextShadow(true);
        backButton.setForeground(Color.white);
        backButton.addActionListener(new BackAction());

        /***************** Back Button Panel *****************/
        backButtonPanel = new ImagePanel(backButtonPanelBackgroundImageFile);
        backButtonPanel.setLayout(null);
        backButtonPanel.add(backButton);
        backButton.setVisible(false);
        backButton.setLocation(new Point(5, 0));

        /***************** Exit Button *****************/
        exitButton = new ImageButton(createIconImage(getString(
            "AwtPDAPresentationMode.exitButton")));
        exitButton.setLabel(getString("AwtPDAPresentationMode.exitButton.label"));
        exitButton.setFont(font);
        exitButton.setForeground(Color.white);
        exitButton.addActionListener(new ExitActionListener());

        /***************** Taskbar Button *****************/
        taskBarButton = new ImageButton(createIconImage(getString(
            "AwtPDAPresentationMode.taskbarButton")));
        taskBarButton.setDepressedImage(createIconImage(getString(
            "AwtPDAPresentationMode.taskbarButton.depressed")));
        taskBarButton.setFont(font);
        taskBarButton.setForeground(Color.white);
        taskBarButton.setLabel(getString(
            "AwtPDAPresentationMode.taskbarButton.label"));
        taskBarButton.addActionListener(new DisplayTaskbarActionListener());

        /***************** Downloader Button *****************/
        downloaderButton = new ImageButton(createIconImage(getString(
            "AwtPDAPresentationMode.downloaderButton")));
        downloaderButton.setLabel(getString(
            "AwtPDAPresentationMode.downloaderButton.label"));
        downloaderButton.setFont(font);
        downloaderButton.setForeground(Color.white);
        downloaderButton.addActionListener(new DisplayDownloaderActionListener());

        /***************** Preferences Button *****************/
        prefsButton = new ImageButton(createIconImage(getString(
            "AwtPDAPresentationMode.prefsButton")));
        prefsButton.setLabel(getString(
            "AwtPDAPresentationMode.prefsButton.label"));
        prefsButton.setFont(font);
        prefsButton.setForeground(Color.white);
        prefsButton.addActionListener(new DisplayPrefsScreenActionListener());

        /***************** System Button *****************/
        systemMenuButton = new ImageButton(createIconImage(getString(
            "AwtPDAPresentationMode.systemMenuButton")));
        systemMenuButton.setLabel("System");
        systemMenuButton.setFont(font);
        systemMenuButton.setForeground(Color.white);
        systemMenuButton.addActionListener(new SystemSubListActionListener());

        scrollButtonsPanel = new ImagePanel(scrollPanelBackgroundImageFile);
        scrollButtonsPanel.setLayout(null);
        scrollButtonsPanel.setFont(currentUserFont);
        scrollButtonsPanel.setLabel(getString(
            "AwtPDAPresentationMode.scrollButton.label"), 13, 70);

        rightScrollButton = new ImageButton(createIconImage(getString(
            "AwtPDAPresentationMode.rightScrollButton")));
        rightScrollButton.addActionListener(new ScrollRightActionListener());
        rightScrollButton.setDepressedImage(createIconImage(getString(
            "AwtPDAPresentationMode.rightScrollButton.pressed")));

        rightGrayedScrollButton = new ImageButton(createIconImage(getString(
            "AwtPDAPresentationMode.rightGrayedScrollButton")));

        leftScrollButton = new ImageButton(createIconImage(getString(
            "AwtPDAPresentationMode.leftScrollButton")));
        leftScrollButton.addActionListener(new ScrollLeftActionListener());
        leftScrollButton.setDepressedImage(createIconImage(getString(
            "AwtPDAPresentationMode.leftScrollButton.pressed")));

        leftGrayedScrollButton = new ImageButton(createIconImage(getString(
            "AwtPDAPresentationMode.leftGrayedScrollButton")));

        // this is the container that holds the toolbar + scrollbuttons
        iconPanel = new IconPanel();
        iconPanel.setLayout(new BorderLayout());
        iconPanel.add(scrollPane, BorderLayout.CENTER);
        iconPanel.add(scrollButtonsPanel, BorderLayout.EAST);
        iconPanel.add(backButtonPanel, BorderLayout.WEST);

        leftScrollButton.setBounds(15, 10, 18, 50);
        rightScrollButton.setBounds(35, 10, 21, 50);
        leftGrayedScrollButton.setBounds(15, 10, 18, 50);
        rightGrayedScrollButton.setBounds(35, 10, 21, 50);

        scrollButtonsPanel.setBounds(480 - 57, 0, 57, 81);
        scrollPane.setBounds(0, 0, 480, 81);

        contentPanel = new ContentPanel();
        contentPanel.setBackground(backgroundColor);

        System.setProperty("sun.textarea.wordwrap", "true");
    };

    void updateFonts() {
        currentUserFont = new Font(currentUserFontStr, Font.PLAIN,
                                   currentUserFontSize);
        displayMenu(currentMenu, currentScrollPosition);
    }

    public AwtPDAPresentationLogin getLoginScreen() {
        return login;
    }

    public Frame getFrame() {
        return topFrame;
    }

    public void showAppManagerScreen() {
        try {
            initDuringLoginThread.join();
        }
        catch (Exception e) {
            e.printStackTrace();
        }

        topFrame.removeAll();
        topFrame.add(iconPanel, BorderLayout.NORTH);

        bottomFrame = new ContentFrame();
        bottomFrame.setBackground(backgroundColor);

        contentPanel.removeAll();

        bottomFrame.add(contentPanel, BorderLayout.NORTH);
        bottomFrame.validate();

        currentUser = login.getUserName();
        currentUserFontSize = Integer.parseInt(preferences.getUserPreference(
            currentUser, "AwtPDAPresentationMode.fontSize"));
        currentUserFontStr = preferences.getUserPreference(
            currentUser, "AwtPDAPresentationMode.font");
        currentUserFont = new Font(currentUserFontStr, Font.PLAIN,
                                   currentUserFontSize);

        // Make sure we are aware of tasks ending in the background
        controller.getMtaskClient().addListener(new TaskEndedListener());

        // hopefully, the login screen will be garbage collected
        login = null;
        setFrameMinimized();

        displayMenu("main");

        Rectangle rect = topFrame.getBounds();
        Rectangle contentFrameBounds = new Rectangle(0, rect.height + rect.y,
            480, 552 - rect.height);
        bottomFrame.setBounds(contentFrameBounds);
        bottomFrame.show();

// Run all user startup apps now
        Object[] apps = startupUserApps.toArray();
        for (int i = 0; i < apps.length; i++) {
            AppModule module = (AppModule) apps[i];

            launchApp(module);

            // We don't need to sleep 2 seconds if there is only 1 startup
            // app, i.e., there won't be a possible clash.
            if (i == 0 && apps.length == 1) {
                break;
            }

            try {
                Thread.sleep(1000 * 2);
            }
            catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    void showLoginScreen() {

        if (bottomFrame != null) {
// This is the case where reset settings
// is selected from the Preferences screen
            bottomFrame.removeAll();
            bottomFrame.hide();
            bottomFrame = null;
            topFrame.removeAll();
            setFrameMaximized();
        }
        login.createLoginScreen();
        topFrame.add(login, BorderLayout.CENTER);
        topFrame.validate();
        login.giveTextFieldFocus();
    }

    public Component getOuterMostPanel() {
        return contentPanel;
    };

    private void showSplashScreen() {
        topFrame.add(splash, BorderLayout.CENTER);
        setFrameMaximized();
        topFrame.setVisible(true);
        // start the animated initialization gauge
        initializationGauge.start();
    }

    private void hideSplashScreen() {
        // stop the animated initialization gauge
        initializationGauge.stop();
        initializationGauge.dispose();
        initializationGauge = null;

        topFrame.remove(splash);
        splash.dispose();
        // hopefully, the splash screen will be garbage collected
        splash = null;
    }

    public void startAppManager() {
        hideSplashScreen();

        // The actions associated with initing some screens
        // will be done in another thread since this can be time consuming.
        // We'll start this right before the login screen appears waiting
        // for the user's input.
        initDuringLoginThread = new InitDuringLoginThread();
        initDuringLoginThread.start();

        showLoginScreen();
    }

    void showInfoDialog(String str) {
        InfoDialog dialog = new InfoDialog(this, str);
        dialog.pack();
        dialog.show();
    }

    public void setFrameMaximized() {
        if ( (topFrame.getWidth() != topFrameMinimizedSize.width) ||
            (topFrame.getHeight() != topFrameMinimizedSize.height)) {
            topFrame.setPreferredSize(topFrameMaximizedSize.width,
                                      topFrameMaximizedSize.height);
            topFrame.pack();
        }
    }

    public void setFrameMinimized() {
        if ( (topFrame.getWidth() != topFrameMinimizedSize.width) ||
            (topFrame.getHeight() != topFrameMinimizedSize.height)) {
            topFrame.setPreferredSize(topFrameMinimizedSize.width,
                                      topFrameMinimizedSize.height);
            topFrame.pack();
        }
    }

    public void loadApps() {

//        apps = appRepository.getAppList();

        try {
            appListThread.join();
        }
        catch (Exception e) {
            e.printStackTrace();
        }

        for (int i = 0; i < apps.length; i++) {
            // add app
            addApp(apps[i]);

            // try to add the app to the menu it belongs to
            String appName = apps[i].getTitle();

            String menu = apps[i].getMenu();
            if (menu == null) {
                menu = "main";
            }
            Vector menuNames = getMenuApps(menu);
            if (menuNames == null) {
                menuNames = new Vector();
                menuNames.add(appName);
                associateMenuApps(menu, menuNames);
            }
            else {
                menuNames.add(appName);
            }

            if (!apps[i].isSublist() && apps[i].getIsStartup()) {
                if (apps[i].isSystemApp()) {
                    startupSystemApps.add(apps[i]);
                }
                else {
                    startupUserApps.add(apps[i]);
                }
            }
        }
    }

    public void updateNewApp(AppModule module) {

        // add app
        addApp(module);

        // try to add the app to the menu it belongs to
        String appName = module.getTitle();

        String menu = module.getMenu();
        if (menu == null) {
            menu = "main";
        }
        Vector menuNames = getMenuApps(menu);
        if (menuNames == null) {
            menuNames = new Vector();
            menuNames.add(appName);
            associateMenuApps(menu, menuNames);
        }
        else {
            menuNames.add(appName);
        }

        if (!menu.equals("main")) {
            menuStack.push("main");
        }

        displayMenu(menu, -1);
    }

    private void associateMenuApps(String menu, Vector menuNames) {
        menuNameToAppNames.put(menu, menuNames);
    }

    private void disassociateMenuApps(String menu, String appName) {
        if (menu == null) {
            menu = "main";
        }
        Vector menuNames = (Vector) menuNameToAppNames.get(menu);
        menuNames.remove(appName);
        if (menuNames.isEmpty()) {
            menuNameToAppNames.remove(menu);
        }
    }

    private Vector getMenuApps(String menu) {
        return (Vector) menuNameToAppNames.get(menu);
    }

    private void addBackButton() {
        backButton.setFont(currentUserFont);
        backButton.setVisible(true);
    }

    private void removeBackButton() {
        backButton.setVisible(false);
    }

    private void displayMenu(String menu) {
        displayMenu(menu, 0);
    }

    private void displayMenu(String menu, int scrollPosition) {
        Vector menuNames = null;

        // Don't check the hash table for the system menu.
        // The system menu is a special case menu only recognized
        // internally by the appmanager.
        if (!menu.equals("system")) {
            if (!menuNameToAppNames.containsKey( (Object) menu)) {
                System.out.println(getString(
                    "AwtPDAPresentationMode.noApps") + menu + "!");
            }
            menuNames = (Vector) menuNameToAppNames.get( (Object) menu);
            if (menuNames == null) {
                System.out.println(getString(
                    "AwtPDAPresentationMode.noApps") + menu + "!");
            }
        }

        // Get rid of everything
        removeBackButton();
        tb.removeAll();

        // Special case stuff here... add the back button if you're in
        // a submenu, or add the system menu if you're in the main menu.
        if (!menu.equals("main")) {
            addBackButton();
        }
        else {
            addSystemMenu();
        }

        // add the system buttons if you're in the system menu, otherwise
        // add the application icons for the menu you are in.
        if (menu.equals("system")) {
            addSystemButtons();
        }
        else {
            if (menuNames != null) {
                Object[] names = menuNames.toArray();
                for (int i = 0; i < names.length; i++) {
                    ImageButton button = getAppButton( (String) names[i]);
                    button.setFont(currentUserFont);
                    tb.add(button);
                }
            }
        }

        tb.validate();
        scrollPane.validate();
        scrollPane.doLayout();

        tb.doLayout();
        tb.repaint();

        displayScrollbars(scrollPosition);

        currentScrollPosition = scrollPosition;
        currentMenu = menu;
    }

    public void runStartupApps() {
// We want only the ticker to start before the login screen.
// We'll launch the other startup apps right after the login.
        Object[] apps = startupSystemApps.toArray();
        for (int i = 0; i < apps.length; i++) {
            AppModule module = (AppModule) apps[i];
            runStartupApp(module);
            try {
                Thread.sleep(1000 * 2);
            }
            catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    public void runStartupApp(AppModule app) {
        String appId = null;
        if (app.getIsStartup()) {
            String[] apps = controller.getMtaskClient().getLaunchedApps();
            String numMaxApps = preferences.getSystemPreference(
                "AwtPDAPresentationMode.maxVMs");
            if (apps.length < Integer.parseInt(numMaxApps)) {
                appId = controller.launchApp(app, app.getIsSwitchable());
                if (appId != null) {
                    addTaskbarButton(app, appId);
                }
            }
        }
        else {
            showInfoDialog(getString(
                "AwtPDAPresentationMode.message.maxConcurrentApps"));
        }

    }

    private void associateAppButton(String name, ImageButton button) {
        appNameToButton.put(name, button);
    }

    private void disassociateAppButton(String name) {
        appNameToButton.remove(name);
    }

    private ImageButton getAppButton(String name) {
        return (ImageButton) appNameToButton.get(name);
    }

    private void associateAppModule(String name, AppModule module) {
        appNameToAppModule.put(name, module);
    }

    private void disassociateAppModule(String name) {
        appNameToAppModule.remove(name);
    }

    private AppModule getAppModule(String name) {
        return (AppModule) appNameToAppModule.get(name);
    }

    public Vector getMenuAppModules() {
        Vector v = new Vector();
        for (Enumeration e = appNameToAppModule.elements(); e.hasMoreElements(); ) {
            AppModule module = (AppModule) e.nextElement();
            if (module.isSublist()) {
                v.add(module);
            }
        }
        return v;
    }

    public Vector getApplicationAppModules() {
        Vector v = new Vector();
        for (Enumeration e = appNameToAppModule.elements(); e.hasMoreElements(); ) {
            AppModule module = (AppModule) e.nextElement();
            if (!module.isSublist()) {
                v.add(module);
            }
        }
        return v;
    }

    public Vector getApplicationAppModules(String menu) {
        Vector v = new Vector();
        for (Enumeration e = appNameToAppModule.elements(); e.hasMoreElements(); ) {
            AppModule module = (AppModule) e.nextElement();
            if (!module.isSublist()) {
                if (module.getMenu() == null) {
                    if (menu.equals("main")) {
                        v.add(module);
                    }
                }
                else if (module.getMenu().equals(menu)) {
                    v.add(module);
                }
            }
        }
        return v;
    }

    private void addTaskbarButton(AppModule app, String appId) {

        if (appId != null) {
            // Add to task panel here
            ImageButton button = new ImageButton();

            button.setUnpressedImage(getIconImage(app));
            button.setDepressedImage(getDepressedIconImage(app));

            String str = app.getTitle();
            button.setLabel(str);

            // Remember this association
            associateAppButton(appId, button);
            taskbar.addTaskBarButton(button, appId, str);
        }

    }

    public void addApp(AppModule module) {
        ActionListener action;
        if (module.isSublist()) {
            action = new SubListActionListener(module);
        }
        else {
            action = new LaunchAppActionListener(module);
        }

        associateAppModule(module.getTitle(), module);
        addButton(action, module);
    }

    void removeApp(AppModule module) {
        disassociateAppModule(module.getTitle());
        removeButton(module);
        disassociateMenuApps(module.getMenu(), module.getTitle());
    }

    private ImageButton addButton(ActionListener a, AppModule app) {

        ImageButton button = new ImageButton(getIconImage(app));
        button.addActionListener(a);
        button.setFont(font);
        button.setForeground(Color.white);

        String str = null;

        if (app.getTitle().trim().length()
            > getInt("AwtPDAPresentationMode.appButtonTitle.maxLength")) {
            str =
                app.getTitle().trim().substring(0,
                                                getInt(
                "AwtPDAPresentationMode.appButtonTitle.maxLength")
                                                - 3);
            str += "...";
        }
        else {
            str = app.getTitle().trim();
        }

        button.setLabel(str);
        button.setDepressedImage(getDepressedIconImage(app));
        button.setTextShadow(true);

        associateAppButton(app.getTitle(), button);

        return button;
    }

    private void removeButton(AppModule app) {
        ImageButton button = (ImageButton) appNameToButton.get(app.getTitle());
        if (tb.isAncestorOf(button)) {
            tb.remove(button);
            tb.repaint();
        }
        disassociateAppButton(app.getTitle());
    }

    void launchApp(AppModule app) {
        if (contentPanel.getComponentCount() > 0) {
            contentPanel.removeAll();
            contentPanel.validate();
        }
        setFrameMinimized();
        String[] apps = controller.getMtaskClient().getLaunchedApps();
        String numMaxApps = preferences.getSystemPreference(
            "AwtPDAPresentationMode.maxVMs");
        if (apps.length < Integer.parseInt(numMaxApps)) {
            String appId = controller.launchApp(app);
            if (appId != null) {
                addTaskbarButton(app, appId);
            }
        }
        else {
            showInfoDialog(getString(
                "AwtPDAPresentationMode.message.maxConcurrentApps"));
        }
    }

    private Image getIconImage(AppModule app) {
        return Toolkit.getDefaultToolkit().createImage(app.getIconURL());
    }

    private Image getDepressedIconImage(AppModule app) {
        return Toolkit.getDefaultToolkit().createImage(app.getIconDepressedURL());
    }

    private void addSystemButtons() {
        exitButton.setFont(currentUserFont);
        tb.add(exitButton);

        taskBarButton.setFont(currentUserFont);
        tb.add(taskBarButton);

        // check the canDownloadApps user property to see if the user is
        // allowed to download apps
        String value = preferences.getUserPreference(currentUser,
            "AwtPDAPresentationMode.canDownloadApps");
        if (value.equals("true")) {
            downloaderButton.setFont(currentUserFont);
            tb.add(downloaderButton);
        }

        prefsButton.setFont(currentUserFont);
        tb.add(prefsButton);
    }

    private void addSystemMenu() {
        systemMenuButton.setFont(currentUserFont);
        tb.add(systemMenuButton);
    }

    private void displayScrollbars() {
        displayScrollbars(0);
    }

    private void displayScrollbars(int scrollPosition) {

        int extent = scrollPane.getHAdjustable().getVisibleAmount();
        // if the scrollPosition equals -1, set the scroll position to the max
        // this scenario pertains to when a new app has been installed and
        // the appmanager should show immediately show the icon of the
        // newly installed app.
        if (scrollPosition == -1) {
            scrollPane.getHAdjustable().setValue(scrollPane.getHAdjustable().
                                                 getMaximum());
        }
        else if (scrollPosition != 0) {
            scrollPane.getHAdjustable().setValue(scrollPosition);
        }
        int value = scrollPane.getHAdjustable().getValue();
        int max = scrollPane.getHAdjustable().getMaximum();

        scrollButtonsPanel.removeAll();
// if we can scroll right
        if (value + extent < max) {
            scrollButtonsPanel.add(rightScrollButton);
        }
        else {
            scrollButtonsPanel.add(rightGrayedScrollButton);
        }
// if we can scroll left
        if (value > 0) {
            scrollButtonsPanel.add(leftScrollButton);
        }
        else {
            scrollButtonsPanel.add(leftGrayedScrollButton);
        }

        scrollButtonsPanel.setFont(currentUserFont);
        scrollButtonsPanel.validate();
        scrollButtonsPanel.repaint();

    }

    private void displayTaskbar() {
        controller.deactivateTopTask();
        contentPanel.removeAll();
        taskbar.refresh();
        contentPanel.add(taskbar, BorderLayout.CENTER);
        contentPanel.validate();
        contentPanel.repaint();
        taskbar.setFocusable(true);
        taskbar.requestFocusInWindow();
    }

    private void displayPrefsScreen() {
        controller.deactivateTopTask();
        contentPanel.removeAll();
        prefsScreen.gotoFirstScreen();
        contentPanel.add(prefsScreen, BorderLayout.CENTER);
        contentPanel.validate();
        contentPanel.repaint();
    }

    private void displayDownloader() {
        controller.deactivateTopTask();
        contentPanel.removeAll();
        // This is a special case.  The initalizing of the apps list,
        // which occurs for screen 1, may take a long time to complete.
        // Therefore, the actions must be put into a different thread
        // so it doesn't hog the event thread.  In addition, we cannot
        // add the download screen to the content panel until the actions
        // is complete.  Therefore, the add, validate, and repaint calls
        // also happen as part of this separate thread.  Please see the
        // initializeAppList() method for more details.
        downloader.initializeAppList();
    }

    void displayScreen(Panel screen) {
        controller.deactivateTopTask();
        contentPanel.removeAll();
        contentPanel.add(screen, BorderLayout.CENTER);
        contentPanel.validate();
        contentPanel.repaint();
    }

    void removeCurrentScreen() {
        contentPanel.removeAll();
        contentPanel.validate();
        contentPanel.repaint();
    }

    private Image createIconImage(String filename) {
        String path = "resources/icons/" + filename;
        URL url = getClass().getResource(path);
        return Toolkit.getDefaultToolkit().createImage(url);
    }

    private Image createImage(String filename) {
        URL url = getClass().getResource(filename);
        return Toolkit.getDefaultToolkit().createImage(url);
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
                "com.sun.appmanager.impl.presentation.AwtPDA.resources.awtpdapresentationmode");
        }

        return bundle;
    }

    class TransparentPanel
        extends Container {
        int prefWidth, prefHeight;

        public TransparentPanel(int width, int height) {
            this.prefWidth = width;
            this.prefHeight = height;
        }

        public void paint(Graphics g) {
            super.paint(g);
        };

        public Dimension getPreferredSize() {
            return new Dimension(prefWidth, prefHeight);
        }
    }

    class SubListActionListener
        implements ActionListener {
        AppModule app;

        public SubListActionListener(AppModule app) {
            this.app = app;
        }

        public void actionPerformed(ActionEvent e) {

            menuStack.push(currentMenu);
            int scrollValue = scrollPane.getHAdjustable().getValue();
            scrollPositionStack.push(new Integer(scrollValue));
            String menu = app.getTitle();
            displayMenu(menu);
        }
    }

    class SystemSubListActionListener
        implements ActionListener {

        public void actionPerformed(ActionEvent e) {

            menuStack.push(currentMenu);
            int scrollValue = scrollPane.getHAdjustable().getValue();
            scrollPositionStack.push(new Integer(scrollValue));
            displayMenu("system");
        }
    }

    class BackAction
        implements ActionListener {

        public void actionPerformed(ActionEvent e) {

            String menu = null;
            Integer scrollVal = null;
            if (!menuStack.empty()) {
                menu = (String) menuStack.pop();
            }
            if (!scrollPositionStack.empty()) {
                scrollVal = (Integer) scrollPositionStack.pop();
            }
            if (menu == null) {
                return;
            }
            if (scrollVal == null) {
                displayMenu(menu, 0);
            }
            else {
                displayMenu(menu, scrollVal.intValue());
            }
        }

    }

    class LaunchAppActionListener
        implements ActionListener {
        AppModule app;

        public LaunchAppActionListener(AppModule app) {
            this.app = app;
        }

        public void actionPerformed(ActionEvent e) {
            launchApp(app);
        }
    }

    class ScrollRightActionListener
        implements ActionListener {
        public void actionPerformed(ActionEvent e) {
            int currentValue = scrollPane.getHAdjustable().getValue();
            int newValue = currentValue += 120;
            scrollPane.getHAdjustable().setValue(newValue);
            displayScrollbars();
        }
    }

    class ScrollLeftActionListener
        implements ActionListener {

        public void actionPerformed(ActionEvent e) {
            int currentValue = scrollPane.getHAdjustable().getValue();
            int newValue = currentValue -= 120;
            scrollPane.getHAdjustable().setValue(newValue);
            displayScrollbars();
        }
    }

    class DisplayTaskbarActionListener
        implements ActionListener {

        public void actionPerformed(ActionEvent e) {
            if (contentPanel.isAncestorOf(taskbar)) {
                removeCurrentScreen();
                controller.reactivateTopTask();
            }
            else {
                displayTaskbar();
            }
        }
    }

    class TaskEndedListener
        implements TaskListener {

        public void taskEvent(String appId, int what) {
            if (what != TaskListener.CDCAMS_TASK_KILLED) {
                // Not intended for us
                return;
            }

            System.out.println(getString("AwtPDAPresentationMode.taskEnded") +
                               appId);
            taskbar.removeAppEntry(appId);
            if (contentPanel.isAncestorOf(taskbar)) {
                taskbar.refresh();
            }
            else {
                controller.reactivateTopTask();
            }
        }
    }

    class DisplayPrefsScreenActionListener
        implements ActionListener {

        public void actionPerformed(ActionEvent e) {
            if (contentPanel.isAncestorOf(prefsScreen)) {
                removeCurrentScreen();
                controller.reactivateTopTask();
            }
            else {
                displayPrefsScreen();
            }
        }
    }

    class DisplayDownloaderActionListener
        implements ActionListener {

        public void actionPerformed(ActionEvent e) {
            if (contentPanel.isAncestorOf(downloader)) {
                removeCurrentScreen();
                controller.reactivateTopTask();
            }
            else {
                displayDownloader();
            }
        }
    }

    class ExitActionListener
        implements ActionListener {

        public void actionPerformed(ActionEvent e) {
            controller.reboot();
        }
    }

    class AppManagerFrame
        extends Frame {
        private int width;
        private int height;

        public AppManagerFrame() {
            super();
            setUndecorated(true);
        }

        public Dimension getPreferredSize() {
            return new Dimension(width, height);
        }

        public void setPreferredSize(int width, int height) {
            this.width = width;
            this.height = height;
        }
    }

    /**
     * DefaultImageButtonBorder - a Border, subclassed to set the default border values.
     */
    class IconPanelBorder
        extends Border {

        public IconPanelBorder() {
            setBorderThickness(1);
            setType(THREED_OUT);
            setMargins(2, 2, 2, 2);
        }
    }

    class AwtPDAScrollPane
        extends ScrollPane {

        public AwtPDAScrollPane(int scrollbarDisplayPolicy) {
            super(scrollbarDisplayPolicy);
        }

        public void update(Graphics g) {
            paint(g);
        }

        public Dimension getPreferredSize() {
            return new Dimension(430, topFrameMinimizedSize.height);
        }

    }

    class GetAppListThread
        extends Thread {

        public GetAppListThread() {
        }

        public void run() {
            apps = appRepository.getAppList();
        }

    }

    class InitDuringLoginThread
        extends Thread {

        public InitDuringLoginThread() {
        }

        public void run() {
            prefsScreen.initialize();
            downloader.initialize();
        }

    }
}
