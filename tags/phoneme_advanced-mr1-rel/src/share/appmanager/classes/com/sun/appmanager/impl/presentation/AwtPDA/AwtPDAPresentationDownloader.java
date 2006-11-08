/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is furee software; you can redistribute it and/or modify
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

import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.FocusListener;
import java.awt.event.FocusEvent;
import java.awt.event.MouseListener;
import java.awt.event.MouseEvent;
import java.util.*;
import java.net.URL;
import java.io.*;
import com.sun.appmanager.AppManager;
import com.sun.appmanager.preferences.Preferences;
import com.sun.appmanager.apprepository.AppModule;
import com.sun.appmanager.apprepository.AppRepository;
import com.sun.appmanager.ota.*;
import com.sun.appmanager.impl.ota.*;

public class AwtPDAPresentationDownloader
    extends Panel {

    private MediaTracker tracker = null;

    Image backgroundImage = null;
    Image appTitleImage = null;
    Image storeTitleImage = null;
    Image downloadNowImage = null;
    Image progressBarEmptyImage = null;
    Image progressBarFullImage = null;
    Image openFolderImage = null;
    Image closedFolderImage = null;
    Image openTriangleImage = null;
    Image closedTriangleImage = null;
    Image blueDiamondImage = null;
    Image cancelButtonImage = null;

    Container screen1 = null;
    Screen2Container screen2 = null;
    Screen1TopContainer screen1Top = null;
    Screen1BottomContainer screen1Bottom = null;
    ScrollPane screen1ScrollPane = null;

    AwtPDAPresentationMode presentationMode = null;
    private Preferences preferences = null;
    DownloadToWindow downloadToWindow = null;
    DownloadProgressWindow downloadProgressWindow = null;

// OMAOTA stuff
    private static boolean debug = true;
    OTA omaOTA = null;
    String outputFile = null;
    String discoveryUrl = null;
    String[] downloadUris;
    String[] downloadNames;

    Gauge gauge = null;

    private final static Color backgroundColor = new Color(171, 175, 211);

    public AwtPDAPresentationDownloader(AwtPDAPresentationMode presentationMode) {
        super();
        this.presentationMode = presentationMode;
        this.preferences = AppManager.getPreferences();

        setLayout(new BorderLayout());

        gauge = new Gauge(presentationMode,
                          presentationMode.getString(
                              "AwtPDAPresentationMode.downloader.accessingServer"));

        omaOTA = AppManager.getOTAFactory().getImpl("OMA");

        String storeTitleImageFile = "resources/downloader/" +
            presentationMode.getString(
                "AwtPDAPresentationMode.downloaderStoreTitle");
        storeTitleImage = createImage(storeTitleImageFile);

        String appTitleImageFile = "resources/downloader/" +
            presentationMode.getString(
                "AwtPDAPresentationMode.downloaderAppTitleBar");
        appTitleImage = createImage(appTitleImageFile);

        String progressBarEmptyFile = "resources/progressbar/" +
            presentationMode.getString(
                "AwtPDAPresentationMode.progressBarEmpty");
        progressBarEmptyImage = createImage(progressBarEmptyFile);

        String progressBarFullFile = "resources/progressbar/" +
            presentationMode.getString("AwtPDAPresentationMode.progressBarFull");
        progressBarFullImage = createImage(progressBarFullFile);

        String openFolderFile = "resources/downloader/" +
            presentationMode.getString(
                "AwtPDAPresentationMode.downloaderOpenFolder");
        openFolderImage = createImage(openFolderFile);

        String closedFolderFile = "resources/downloader/" +
            presentationMode.getString(
                "AwtPDAPresentationMode.downloaderClosedFolder");
        closedFolderImage = createImage(closedFolderFile);

        String openTriangleFile = "resources/downloader/" +
            presentationMode.getString(
                "AwtPDAPresentationMode.downloaderOpenTriangle");
        openTriangleImage = createImage(openTriangleFile);

        String closedTriangleFile = "resources/downloader/" +
            presentationMode.getString(
                "AwtPDAPresentationMode.downloaderClosedTriangle");
        closedTriangleImage = createImage(closedTriangleFile);

        String blueDiamondFile = "resources/downloader/" +
            presentationMode.getString(
                "AwtPDAPresentationMode.downloaderBlueDiamond");
        blueDiamondImage = createImage(blueDiamondFile);

        String downloadNowFile = "resources/downloader/" +
            presentationMode.getString(
                "AwtPDAPresentationMode.downloaderDownLoadNowButton");
        downloadNowImage = createImage(downloadNowFile);

        String cancelButtonFile = "resources/icons/" +
            presentationMode.getString("AwtPDAPresentationMode.cancelButton");
        cancelButtonImage = createImage(cancelButtonFile);

        tracker = new MediaTracker(this);
        try {
            tracker.addImage(storeTitleImage, 0);
            tracker.addImage(appTitleImage, 1);
            tracker.addImage(progressBarEmptyImage, 2);
            tracker.addImage(progressBarFullImage, 3);

            tracker.addImage(openFolderImage, 4);
            tracker.addImage(closedFolderImage, 5);
            tracker.addImage(openTriangleImage, 6);
            tracker.addImage(closedTriangleImage, 7);
            tracker.addImage(blueDiamondImage, 8);

            tracker.addImage(downloadNowImage, 9);

            tracker.addImage(cancelButtonImage, 10);

            for (int i = 0; i < 11; i++) {
                tracker.waitForID(i);
            }
        }
        catch (InterruptedException e) {
            e.printStackTrace();
            return;
        }
    }

    public void initialize() {
// Screen1 is the screen where the downloadable apps
// are listed.
        initScreen1();

// Screen2 is the screen displayed after selecting
// the app to downlod
        initScreen2();

// Display Screen1 first
        add(screen1);
    }

    public Image getBackgroundImage() {
        return backgroundImage;
    }

    public Image getStoreTitleImage() {
        return storeTitleImage;
    }

    class InitializeAppsListThread
        extends Thread {
        AwtPDAPresentationDownloader downloader = null;

        public InitializeAppsListThread(AwtPDAPresentationDownloader downloader) {
            this.downloader = downloader;
        }

        public void run() {
            gauge.start();
            createDownloadableAppsList();
            if (!downloader.isAncestorOf(screen1)) {
                downloader.removeAll();
                downloader.add(screen1);
                downloader.validate();
            }
            presentationMode.contentPanel.add(downloader, BorderLayout.CENTER);
            presentationMode.contentPanel.validate();
            presentationMode.contentPanel.repaint();
            screen1.repaint();
            gauge.stop();
        }
    }

// It's important that this isn't done in the events thread
    public void initializeAppList() {
        Thread thread = new InitializeAppsListThread(this);
        thread.start();
    }

// This is the container that displays the Download eStore image
// It is separate from the Screen1BottomContainer because we don't
// want any text to be inserted in this container.
    class Screen1TopContainer
        extends Container {

        Image storeTitleImage = null;

        public Screen1TopContainer() {
            super();
            this.storeTitleImage = getStoreTitleImage();
        }

        public Screen1TopContainer(Image title) {
            super();
            this.storeTitleImage = title;
        }

        public void paint(Graphics g) {
            g.drawImage(storeTitleImage, 0, 0, storeTitleImage.getWidth(null),
                        storeTitleImage.getHeight(null), null);
            super.paint(g);
        }

        public Dimension getPreferredSize() {
            if (storeTitleImage != null) {
                return new Dimension(storeTitleImage.getWidth(null),
                                     storeTitleImage.getHeight(null));
            }
            else {
                return null;
            }
        }

    }

    class Screen1BottomContainer
        extends Container {
        Image backgroundImage = null;
        Image storeTitleImage = null;

        public Screen1BottomContainer() {
            super();
            this.backgroundImage = getBackgroundImage();
            this.storeTitleImage = getStoreTitleImage();
            setBackground(backgroundColor);
        }

        public Screen1BottomContainer(Image background) {
            super();
            this.backgroundImage = background;
            this.storeTitleImage = getStoreTitleImage();
            setBackground(backgroundColor);
        }

        public void paint(Graphics g) {
            super.paint(g);
        }
    }

    private void initScreen1() {
        screen1 = new Container();
        screen1.setLayout(new BorderLayout());
        screen1Top = new Screen1TopContainer();
        screen1Bottom = new Screen1BottomContainer();
        screen1Bottom.setLayout(new SlotLayout());
        screen1.add(screen1Top, BorderLayout.NORTH);
        screen1ScrollPane = new ScrollPane(ScrollPane.SCROLLBARS_ALWAYS);
        screen1ScrollPane.add(screen1Bottom);
        screen1ScrollPane.setBackground(backgroundColor);
        screen1.add(screen1ScrollPane, BorderLayout.CENTER);

        ImagePanel bottomPanel = null;

        String bottomBarImageFile = "resources/background/"
            +
            presentationMode.getString("AwtPDAPresentationMode.bottomBarImage");
        bottomPanel = new ImagePanel(bottomBarImageFile);
        bottomPanel.setLayout(new GridLayout(1, 1));
        ImageButton cancelButton = new ImageButton(cancelButtonImage);
        cancelButton.setLabel(presentationMode.getString(
            "AwtPDAPresentationMode.cancelButton.label"));
        cancelButton.setForeground(Color.white);
        cancelButton.setFont(presentationMode.currentUserFont);
        cancelButton.addActionListener(new CancelAction());

        bottomPanel.add(cancelButton);
        screen1.add(bottomPanel, BorderLayout.SOUTH);
    }

    class CancelAction
        implements ActionListener {

        protected CancelAction() {
        }

        public void actionPerformed(ActionEvent e) {
            presentationMode.removeCurrentScreen();
        }
    }

    static void trace(String s) {
        if (false) {
            System.out.println(s);
        }
        return;
    }

    private void createDownloadableAppsList() {

        if (omaOTA == null) {
            return;
        }

        // Determine the discovery URL
        discoveryUrl = preferences.getUserPreference(presentationMode.
            currentUser, "AwtPDAPresentationMode.discoveryURL");
        trace("using discovery URL: " + discoveryUrl);

        // Discover returns a hashtable. Right now we assume it will
        // be keyed by descriptor, and each value will be a textual name
        Hashtable h = omaOTA.discover(discoveryUrl);

        downloadUris = new String[h.size()];
        downloadNames = new String[h.size()];
        int i = 0;
        for (Enumeration e = h.keys(); e.hasMoreElements(); ) {
            String s = (String) e.nextElement();
            downloadUris[i] = s;
            downloadNames[i] = (String) h.get(s);
            trace("key " + downloadUris[i] +
                  ", value " +
                  downloadNames[i]);
            i++;
        }

        // Now, add everything to the screen1 bottom container
        screen1Bottom.removeAll();
        for (i = 0; i < downloadNames.length; i++) {
            ImageButton button = new ImageButton(null, downloadNames[i]) {
                public Dimension getPreferredSize() {
                    return new Dimension(460, 30);
                }

                public Dimension getMinimumSize() {
                    return getPreferredSize();
                }

                public Dimension getMaximumSize() {
                    return getPreferredSize();
                }

            };
            button.setTextColor(Color.black);
            button.setFont(presentationMode.currentUserFont);
            button.addActionListener(new Screen2Action(downloadUris[i]));
            screen1Bottom.add(button);
        }
    }

    private Descriptor getDescriptor(String uri) {
        trace("DEBUG: creating descriptor for " + uri);
        if (omaOTA != null && uri != null) {
            try {
                return omaOTA.createDescriptor(uri);
            }
            catch (OTAException o) {
                o.printStackTrace();
            }
        }
        return null;
    }

    public Image createImage(String
                             filename) {
        URL url = getClass().getResource(filename);
        return Toolkit.getDefaultToolkit().createImage(url);
    }

// This is the Container that holds al of the
// elements for the seconds screen.
    class Screen2Container
        extends Panel {

        Descriptor d = null;
        Application app = null;
        Vector applicationsVector = null;
        String appName = null;
        Image iconImage = null;
        ScreenContentPanel contentPanel = null;

        public Screen2Container() {
            setBackground(backgroundColor);
            setLayout(new BorderLayout());
            contentPanel = new ScreenContentPanel();
            add(contentPanel, BorderLayout.CENTER);
        }

        public void initialize(String uri) {
            this.d = null;
            this.app = null;

            this.d = getDescriptor(uri);
            if (this.d != null) {
                iconImage = getIconImage();
                this.applicationsVector = this.d.getApplications();
                if (applicationsVector == null) {
                    trace("DEBUG: " + uri + " has a null vector!");
                    return;
                }

                if (applicationsVector.size() == 0) {
                    trace("DEBUG: " + uri + " has a vector, but no apps!");
                    return;
                }

                appName = "";
                for (Enumeration e = applicationsVector.elements();
                     e.hasMoreElements(); ) {
                    Application application = (Application) e.nextElement();
                    if (appName.length() == 0) {
                        appName = application.getName();
                    }
                    else {
                        appName = appName.concat(", " + application.getName());
                    }
                }
            }
        }

        Image getIconImage() {
            IconDLIndicator iconDL = null;
            IconDestination iconDest = null;
            Image img = null;
            if (omaOTA != null) {
                try {
                    trace("d.getIconURI() -> " + d.getIconURI());
                    iconDL = new IconDLIndicator();
                    iconDest = new IconDestination();
                    ( (CDCAmsOTA) omaOTA).download(d.getIconURI(), null, 0,
                        iconDest, iconDL);

                }
                catch (OTAException o) {
                    o.printStackTrace();
                }
                finally {
                    byte[] imageData = iconDest.getBuffer();
                    if (imageData == null) {
                        trace("imageData is NULL.");
                    }
                    img = Toolkit.getDefaultToolkit().createImage(
                        imageData);

                }
            }
            return img;

        }

        public Descriptor getCurrentDescriptor() {
            return this.d;
        }

        class ScreenContentPanel
            extends Panel {

            public ScreenContentPanel() {
                setLayout(null);
                setBackground(backgroundColor);

                ImageButton downloadNowButton = new ImageButton(
                    downloadNowImage);
                downloadNowButton.addActionListener(new ActionListener() {
                    public void actionPerformed(ActionEvent e) {
                        downloadToWindow = new DownloadToWindow(
                            presentationMode.
                            getFrame());
                        try {
                            downloadToWindow.setDescriptor(screen2.
                                getCurrentDescriptor());
                        }
                        catch (Exception ex) {}
                        downloadToWindow.pack();
                        downloadToWindow.show();
                    }
                });
                add(downloadNowButton);
                downloadNowButton.setBounds(175, 340, 144, 35);
            }

            public void paint(Graphics g) {
                g.drawImage(storeTitleImage, 0, 0, null);
                g.drawImage(appTitleImage, 5, 90, null);

// We can't let this font change in size since it needs to fit
// within the blue rectangular area
                g.setFont(new Font(presentationMode.currentUserFontStr,
                                   Font.ITALIC | Font.BOLD, 24));
                g.setColor(Color.white);

                if (appName != null) {
                    g.drawString(appName, 80, 125);
                }

                g.setFont(new Font(presentationMode.currentUserFontStr,
                                   Font.PLAIN,
                                   presentationMode.currentUserFontSize));
                g.setColor(Color.black);

                if (d != null && appName != null) {
                    g.drawString(presentationMode.getString(
                        "AwtPDAPresentationMode.downloader.version") +
                                 d.getVersion(), 7, 190);
                    g.drawString(presentationMode.getString(
                        "AwtPDAPresentationMode.downloader.vendor") +
                                 d.getVendor(), 7, 220);
                    g.drawString(presentationMode.getString(
                        "AwtPDAPresentationMode.downloader.size") + d.getSize(),
                                 7, 250);
                    if (d.getDescription() != null) {
                        g.drawString(presentationMode.getString(
                            "AwtPDAPresentationMode.downloader.description") +
                                     d.getDescription(), 7,
                                     280);
                    }
                    else {
                        g.drawString(presentationMode.getString(
                            "AwtPDAPresentationMode.downloader.nodescription"),
                                     7,
                                     280);
                    }
                }
                else {
                    g.drawString(presentationMode.getString(
                        "AwtPDAPresentationMode.downloader.erroraccess"), 7,
                                 160);
                }

                super.paint(g);

                if (iconImage != null) {
                    trace("Drawing icon image!!!");
                    g.drawImage(iconImage, 400, 150, this);
                }
                else {
                    trace("Cannot find iconImage!!!");
                }

            }
        }
    }

    private void initScreen2() {
        screen2 = new Screen2Container();

        ImagePanel bottomPanel = null;

        String bottomBarImageFile = "resources/background/"
            +
            presentationMode.getString("AwtPDAPresentationMode.bottomBarImage");
        bottomPanel = new ImagePanel(bottomBarImageFile);
        bottomPanel.setLayout(new GridLayout(1, 1));
        ImageButton backButton = new ImageButton(presentationMode.
                                                 backButtonImage);
        backButton.setLabel(presentationMode.getString(
            "AwtPDAPresentationMode.backButton.label"));
        backButton.setForeground(Color.white);
        backButton.setFont(presentationMode.currentUserFont);
        backButton.addActionListener(new BackAction());
        bottomPanel.add(backButton);

        screen2.add(bottomPanel, BorderLayout.SOUTH);
        screen2.validate();
    }

    class BackAction
        implements ActionListener {

        public void actionPerformed(ActionEvent e) {
            remove(screen2);
            add(screen1);
            screen1Bottom.validate();
            screen1Bottom.doLayout();
            screen1Bottom.repaint();
            validate();
            repaint();
        }
    }

    class Screen2Action
        implements ActionListener {
        String appUri = null;

        protected Screen2Action(String uri) {
            this.appUri = uri;
        }

        public void actionPerformed(ActionEvent e) {
            remove(screen1);
            screen2.initialize(this.appUri);
            add(screen2);
            validate();
        }
    }

    public Dimension getPreferredSize() {
        return new Dimension(480, 462);
    }

    class DownloadToWindow
        extends Dialog {
        Vector menuList = null;
        Vector menuAppList = null;
        TextField saveInTextField = null;
        private Descriptor descriptor = null;
        String currentSelectionStr = null;
        boolean currentSelectionIsMenu = false;
        boolean currentSelectionIsTopMenu = false;
        TextField fileSaveTextField = null;
        Panel contentPanel = null;
        ScrollPane scrollPane = null;
        DownloadToMenuEntry currentMenuSelection = null;
        ScreenBorder border = new ScreenBorder();

        public DownloadToWindow(Frame frame) {
            super(frame);
            setUndecorated(true);
            setLayout(null);
            setLocation(40, 80);
            saveInTextField = new TextField();
            saveInTextField.setFont(presentationMode.currentUserFont);
            saveInTextField.setBounds(100, 60, 275, 30);
            saveInTextField.setEditable(false);
            add(saveInTextField);

            contentPanel = new Panel();
            contentPanel.setLayout(new SlotLayout());

            contentPanel.setBackground(Color.white);

            scrollPane = new ScrollPane(ScrollPane.
                                        SCROLLBARS_AS_NEEDED);
            scrollPane.add(contentPanel);
            scrollPane.setBounds(20, 100, 360, 250);

            AppModule module = null;

            // Get vector of main menu apps
            menuAppList = presentationMode.getApplicationAppModules("main");

            // Add Main menu
            contentPanel.add(new DownloadToMenuEntry("main", true,
                menuAppList));

            // Get List of SubMenus
            menuList = presentationMode.getMenuAppModules();

            for (Enumeration e = menuList.elements(); e.hasMoreElements(); ) {
                module = (AppModule) e.nextElement();

                // Add Menu apps
                menuAppList = presentationMode.getApplicationAppModules(module.
                    getTitle());
                contentPanel.add(new DownloadToMenuEntry(module.getTitle(), false,
                    menuAppList));
            }

            add(scrollPane);

            // Insert cancel button
            ImageButton cancelButton = new ImageButton(presentationMode.
                mediumButtonImage);
            cancelButton.setLabel(presentationMode.getString(
                "AwtPDAPresentationMode.cancelButtonLabel"), 14, 21);
            cancelButton.setFont(presentationMode.currentUserFont);
            cancelButton.setTextColor(Color.black);
            cancelButton.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent e) {
                    downloadToWindow.setVisible(false);
                }
            });
            add(cancelButton);
            cancelButton.setBounds(25, 360, 88, 40);
            cancelButton.setBackground(Color.white);

            // Insert save buton
            ImageButton saveButton = new ImageButton(presentationMode.
                mediumButtonImage);
            saveButton.setLabel(presentationMode.getString(
                "AwtPDAPresentationMode.saveButtonLabel"), 22, 21);
            saveButton.setFont(presentationMode.currentUserFont);
            saveButton.setTextColor(Color.black);
            saveButton.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent e) {
                    downloadToWindow.setVisible(false);
                    downloadProgressWindow = new DownloadProgressWindow(
                        presentationMode.getFrame(), descriptor,
                        saveInTextField.getText());
                    AwtPDADLIndicatorThread thread = new
                        AwtPDADLIndicatorThread(descriptor,
                                                saveInTextField.getText(), false);
                    thread.start();
                    downloadProgressWindow.pack();
                    downloadProgressWindow.show();
                }
            });

            add(saveButton);
            saveButton.setBounds(130, 360, 88, 40);
            saveButton.setBackground(Color.white);

            // Insert save buton
            ImageButton saveAndRunButton = new ImageButton(
                presentationMode.largeButtonImage);
            saveAndRunButton.setLabel(presentationMode.getString(
                "AwtPDAPresentationMode.saveAndRunButtonLabel"), 15, 21);
            saveAndRunButton.setFont(presentationMode.currentUserFont);
            saveAndRunButton.setTextColor(Color.black);
            saveAndRunButton.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent e) {
                    downloadToWindow.setVisible(false);
                    downloadProgressWindow = new DownloadProgressWindow(
                        presentationMode.getFrame(), descriptor,
                        saveInTextField.getText());
                    AwtPDADLIndicatorThread thread = new
                        AwtPDADLIndicatorThread(descriptor,
                                                saveInTextField.getText(), true);
                    thread.start();
                    downloadProgressWindow.pack();
                    downloadProgressWindow.show();
                }
            });

            add(saveAndRunButton);
            saveAndRunButton.setBounds(230, 360, 126, 40);
            saveAndRunButton.setBackground(Color.white);

        }

        public void setDescriptor(Descriptor d) {
            descriptor = d;
        }

        public Dimension getPreferredSize() {
            return new Dimension(400, 400);
        }

        public void paint(Graphics g) {
            paintBackground(g);

            g.setFont(new Font(presentationMode.currentUserFontStr,
                               Font.ITALIC | Font.BOLD,
                               presentationMode.currentUserFontSize));
            g.setColor(Color.black);

            Dimension size = getSize();
            FontMetrics fm = getFontMetrics(getFont());

            String str = presentationMode.getString(
                "AwtPDAPresentationMode.downloader.saveIn.label");
            int width = (int) fm.stringWidth(str);
            g.drawString(str, (size.width - width) / 2, 30);

            g.setColor(Color.black);

            g.setFont(new Font(presentationMode.currentUserFontStr, Font.BOLD,
                               presentationMode.currentUserFontSize));
            g.drawString(presentationMode.getString(
                "AwtPDAPresentationMode.downloader.saveIn.label"), 10, 75);

            paintComponents(g);

            border.paint(g, getBackground(), 0, 0, size.width,
                         size.height);
        }

        public void paintBackground(Graphics g) {
            Color color1 = new Color(160, 193, 250);
            Color color2 = new Color(148, 176, 239);
            Color color3 = new Color(121, 155, 214);
            Color currentColor = null;

            g.setColor(color3);
            for (int i = 0; i < 45; i++) {
                g.drawLine(0, i, getWidth(), i);
            }
            for (int i = 45; i < getHeight(); i += 5) {
                if (currentColor == color1) {
                    currentColor = color2;
                }
                else {
                    currentColor = color1;
                }
                g.setColor(currentColor);
                for (int j = 0; j < 5; j++) {
                    if (j < getHeight()) {
                        g.drawLine(0, i + j, getWidth(), i + j);
                    }
                }
            }

        }

        public void setCurrentMenuSelection(DownloadToMenuEntry menu) {
            currentMenuSelection = menu;
        }

        public DownloadToMenuEntry getCurrentMenuSelection() {
            return currentMenuSelection;
        }

        class DownloadToMenuEntry
            extends Panel implements FocusListener, MouseListener {

            static final int TOP_MENU_INDENT = 10;
            static final int SUB_MENU_INDENT = 30;
            private boolean selected = false;
            private String name = null;
            private boolean isTopMenu = false;
            private Vector menuAppList = null;
            private Vector downloadEntryVector = null;
            ImageButton closedFolderButton = null;
            ImageButton openFolderButton = null;

            public DownloadToMenuEntry(String str, boolean isTopMenu,
                                       Vector menuAppList) {
                name = str;
                this.isTopMenu = isTopMenu;
                this.menuAppList = menuAppList;
                if (menuAppList != null) {
                    downloadEntryVector = new Vector();
                }

                setLayout(null);

                openFolderButton = new ImageButton(openFolderImage);
                openFolderButton.addActionListener(new
                    CloseFolderActionListener());
                closedFolderButton = new ImageButton(closedFolderImage);
                closedFolderButton.addActionListener(new
                    OpenFolderActionListener());

                add(closedFolderButton);
                if (isTopMenu) {
                    closedFolderButton.setBounds(TOP_MENU_INDENT, 7, 27, 20);
                }
                else {
                    closedFolderButton.setBounds(SUB_MENU_INDENT, 7, 27, 20);
                }

                setFocusable(true);
                addFocusListener(this);
                addMouseListener(this);
            }

            public void setSelected(boolean val) {
                selected = val;
                if (val) {
                    currentSelectionStr = name;
                    currentSelectionIsTopMenu = isTopMenu;
                    setBackground(new Color(154, 179, 223));
                    saveInTextField.setText(name);
                    saveInTextField.repaint();
                    DownloadToMenuEntry previousMenu = getCurrentMenuSelection();
                    if (previousMenu != null && previousMenu != this) {
                        previousMenu.setSelected(false);
                    }
                    setCurrentMenuSelection(this);
                }
                else {
                    currentSelectionStr = "";
                    currentSelectionIsMenu = false;
                    currentSelectionIsTopMenu = false;
                    setBackground(Color.white);
                }
            }

            public boolean isSelected() {
                return selected;
            }

            public void paint(Graphics g) {
                int indentVal;
                if (isTopMenu) {
                    indentVal = TOP_MENU_INDENT;
                }
                else {
                    indentVal = SUB_MENU_INDENT;
                }
                g.setFont(new Font(presentationMode.currentUserFontStr,
                                   Font.PLAIN,
                                   presentationMode.currentUserFontSize));
                g.drawString(name, indentVal + 50, 20);
                super.paint(g);
            }

            public void focusGained(FocusEvent e) {
                if (isSelected()) {
                    return;
                }
                else {
                    setSelected(true);
                }
            }

            public void focusLost(FocusEvent e) {}

            public void mouseClicked(MouseEvent e) {}

            public void mouseEntered(MouseEvent e) {}

            public void mouseExited(MouseEvent e) {}

            public void mousePressed(MouseEvent e) {
                requestFocusInWindow();
            }

            public void mouseReleased(MouseEvent e) {}

            public Dimension getPreferredSize() {
                return new Dimension(300, 30);
            }

            public Dimension getMinimumSize() {
                return getPreferredSize();
            }

            public Dimension getMaximumSize() {
                return getPreferredSize();
            }

            public void openFolder() {
                if (menuAppList != null) {
                    int newIndex = 0;
                    for (newIndex = 0;
                         newIndex < contentPanel.getComponentCount();
                         newIndex++) {
                        if (contentPanel.getComponent(newIndex) == this) {
                            break;
                        }
                    }
                    if (newIndex == contentPanel.getComponentCount()) {
                        trace(
                            "DEBUG: Couldn't find the menu in the contentPanel!");
                        return;
                    }
                    for (Enumeration e2 = menuAppList.elements();
                         e2.hasMoreElements(); ) {
                        AppModule module = (AppModule) e2.nextElement();
                        newIndex++;
                        DownloadToAppEntry entry = new DownloadToAppEntry(this,
                            module.getTitle(), isTopMenu);
                        downloadEntryVector.add(entry);
                        contentPanel.add(entry, newIndex);
                    }
                    contentPanel.validate();
                    contentPanel.doLayout();
                    contentPanel.repaint();
                    scrollPane.validate();
                    scrollPane.repaint();
                }
            }

            public void closeFolder() {
                if (downloadEntryVector != null &&
                    downloadEntryVector.size() > 0) {
                    for (Enumeration e2 = downloadEntryVector.elements();
                         e2.hasMoreElements(); ) {
                        DownloadToAppEntry entry = (DownloadToAppEntry) e2.
                            nextElement();
                        contentPanel.remove(entry);
                    }
                    downloadEntryVector.removeAllElements();
                    contentPanel.validate();
                    contentPanel.doLayout();
                    contentPanel.repaint();
                    scrollPane.validate();
                    scrollPane.repaint();
                }
            }

            class OpenFolderActionListener
                implements ActionListener {

                public void actionPerformed(ActionEvent e) {
                    openFolder();
                    remove(closedFolderButton);
                    add(openFolderButton);
                    if (isTopMenu) {
                        openFolderButton.setBounds(TOP_MENU_INDENT, 7, 27, 20);
                    }
                    else {
                        openFolderButton.setBounds(SUB_MENU_INDENT, 7, 27, 20);
                    }
                    setSelected(true);
                    validate();
                    repaint();
                }
            }

            class CloseFolderActionListener
                implements ActionListener {

                public void actionPerformed(ActionEvent e) {
                    closeFolder();
                    remove(openFolderButton);
                    add(closedFolderButton);
                    if (isTopMenu) {
                        closedFolderButton.setBounds(TOP_MENU_INDENT, 7, 27, 20);
                    }
                    else {
                        closedFolderButton.setBounds(SUB_MENU_INDENT, 7, 27, 20);
                    }
                    validate();
                    repaint();
                }
            }

        }

        class DownloadToAppEntry
            extends Panel {

            static final int TOP_MENU_APP_INDENT = 30;
            static final int SUB_MENU_APP_INDENT = 50;
            DownloadToMenuEntry menu = null;
            private String name = null;
            private boolean isTopMenuApp = false;

            public DownloadToAppEntry(DownloadToMenuEntry menu, String str,
                                      boolean topMenuApp) {
                this.menu = menu;
                name = str;
                isTopMenuApp = topMenuApp;
            }

            public void paint(Graphics g) {
                int indentVal;
                if (isTopMenuApp) {
                    indentVal = TOP_MENU_APP_INDENT;
                    g.drawImage(blueDiamondImage, TOP_MENU_APP_INDENT, 7, null);
                }
                else {
                    indentVal = SUB_MENU_APP_INDENT;
                    g.drawImage(blueDiamondImage, SUB_MENU_APP_INDENT, 7, null);
                }
                g.setFont(presentationMode.currentUserFont);
                g.drawString(name, indentVal + 50, 20);

            }

            public Dimension getPreferredSize() {
                return new Dimension(300, 30);
            }

            public Dimension getMinimumSize() {
                return getPreferredSize();
            }

            public Dimension getMaximumSize() {
                return getPreferredSize();
            }

        }
    }

    class DownloadProgressWindow
        extends Dialog {

        int progressBarWidth = 0;
        int progressBarHeight = 0;
        int percent = 0;
        boolean drawBackground = true;
        String menu = null;
        Descriptor descriptor = null;
        Application app = null;
        boolean cancelled = false;
        ScreenBorder border = new ScreenBorder();
        TextField fromTextField = null;

        public DownloadProgressWindow(Frame frame, Descriptor descriptor,
                                      String menu) {
            super(frame);
            setUndecorated(true);
            setLayout(null);
            setLocation(40, 80);

            this.descriptor = descriptor;
            this.menu = menu;

            Vector applicationsVector = descriptor.getApplications();
            if (applicationsVector == null) {
                trace("DEBUG: " + descriptor +
                      " has a null application vector!");
                return;
            }

            if (applicationsVector.size() == 0) {
                trace("DEBUG: " + descriptor +
                      " has an application vector, but no apps!");
                return;
            }
            this.app = (Application) applicationsVector.get(0);

            progressBarWidth = progressBarEmptyImage.getWidth(null);
            progressBarHeight = progressBarEmptyImage.getHeight(null);

// Insert cancel button
            cancelled = false;
            ImageButton cancelButton = new ImageButton(presentationMode.
                mediumButtonImage);
            cancelButton.setLabel(presentationMode.getString(
                "AwtPDAPresentationMode.cancelButton.label"), 20, 24);
            cancelButton.setFont(presentationMode.currentUserFont);
            cancelButton.setTextColor(Color.black);
            cancelButton.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent e) {
                    cancelled = true;
                    setVisible(false);
                }
            });
            add(cancelButton);
            cancelButton.setBounds(100, 350, 100, 40);

            ImageButton closeButton = new ImageButton(presentationMode.
                mediumButtonImage);
            closeButton.setLabel(presentationMode.getString(
                "AwtPDAPresentationMode.closeButton.label"), 18, 24);
            closeButton.setFont(presentationMode.currentUserFont);
            closeButton.setTextColor(Color.black);
            closeButton.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent e) {
                    setVisible(false);
                }
            });
            add(closeButton);
            closeButton.setBounds(240, 350, 100, 40);

            fromTextField = new TextField();
            add(fromTextField);
            fromTextField.setBounds(70, 95, 285, 40);
            fromTextField.setEditable(false);
        }

        public boolean isCancelled() {
            return cancelled;
        }

        public void setPercent(int value) {
            this.percent = value;
            repaint();
        }

        public Dimension getPreferredSize() {
            return new Dimension(400, 400);
        }

        public Dimension getMinimumSize() {
            return getPreferredSize();
        }

        public Dimension getMaximumSize() {
            return getPreferredSize();
        }

        public void update(Graphics g) {
            paint(g);
        }

        public void paint(Graphics g) {
            if (drawBackground || percent == 100) {

                paintBackground(g);
                g.setColor(Color.black);
                g.setFont(presentationMode.currentUserFont);

                Dimension size = getSize();
                FontMetrics fm = getFontMetrics(getFont());

                String str = presentationMode.getString(
                    "AwtPDAPresentationMode.downloader.saveIn.label");
                int width = (int) fm.stringWidth(str);
                g.drawString(str, (size.width - width) / 2, 30);

                g.setColor(new Color(160, 193, 250));
                g.drawLine(0, 35, 400, 35);

                g.setColor(Color.black);
                if (app == null) {
                    g.drawString(presentationMode.getString(
                        "AwtPDAPresentationMode.downloader.savingUnknown.label"),
                                 50, 75);
                }
                else {
                    g.drawString(presentationMode.getString(
                        "AwtPDAPresentationMode.downloader.saving.label") + ' ' +
                                 descriptor.getName(), 10, 75);
                }

                g.setColor(Color.black);

                g.setFont(presentationMode.currentUserFont);
                if (descriptor == null || descriptor.getObjectURI() == null) {
                    g.drawString(presentationMode.getString(
                        "AwtPDAPresentationMode.downloader.from.label"), 10,
                                 120);
                    fromTextField.setText(presentationMode.getString(
                        "AwtPDAPresentationMode.downloader.fromUnknown.label"));
                }
                else {
                    g.drawString(presentationMode.getString(
                        "AwtPDAPresentationMode.downloader.from.label"), 10,
                                 120);
                    fromTextField.setText(descriptor.getObjectURI());
                }
                g.drawString(presentationMode.getString(
                    "AwtPDAPresentationMode.downloader.to.label") + ' ' + menu,
                             10, 180);

                if (percent == 100) {
                    g.drawString(presentationMode.getString(
                        "AwtPDAPresentationMode.downloader.status.label") + ' ' +
                                 presentationMode.getString(
                                     "AwtPDAPresentationMode.downloader.finished.label"),
                                 10,
                                 240);
                }
                else {
                    g.drawString(presentationMode.getString(
                        "AwtPDAPresentationMode.downloader.status.label") + ' ' +
                                 presentationMode.getString(
                                     "AwtPDAPresentationMode.downloader.downloading.label"),
                                 10, 240);
                }
                g.drawString(presentationMode.getString(
                    "AwtPDAPresentationMode.downloader.progress.label"), 10,
                             315);
                drawBackground = false;
                paintComponents(g);

                border.paint(g, getBackground(), 0, 0, size.width,
                             size.height);

            }

            int percentage = ( (progressBarWidth - 50) * percent) / 100;
            g.drawImage(progressBarEmptyImage, 100, 300, null);
            Shape origclip = g.getClip();
            g.setClip(100, 300, percentage + 50, progressBarHeight);
            g.drawImage(progressBarFullImage, 100, 300, null);
            g.setClip(origclip);
            g.setColor(Color.black);
            g.drawString(Integer.toString(percent) + "%", 270, 315);
        }

        public void paintBackground(Graphics g) {
            Color color1 = new Color(160, 193, 250);
            Color color2 = new Color(148, 176, 239);
            Color color3 = new Color(121, 155, 214);
            Color currentColor = null;

            g.setColor(color3);
            for (int i = 0; i < 35; i++) {
                g.drawLine(0, i, getWidth(), i);
            }

            for (int i = 35; i < getHeight(); i += 5) {
                if (currentColor == color1) {
                    currentColor = color2;
                }
                else {
                    currentColor = color1;
                }
                g.setColor(currentColor);
                for (int j = 0; j < 5; j++) {
                    if (j < getHeight()) {
                        g.drawLine(0, i + j, getWidth(), i + j);
                    }
                }
            }
        }
    }

    class AwtPDADLIndicatorThread
        extends Thread implements DLIndicator {

        private Descriptor descriptor = null;
        private String menu = null;
        private boolean run = false;

        public AwtPDADLIndicatorThread(Descriptor descriptor, String menu,
                                       boolean run) {
            this.descriptor = descriptor;
            this.menu = menu;
            this.run = run;
        }

        // Begin DLIndicator implementation
        public int getGranularity() {
            return 10;
        }

        public void updatePercent(int value) {
            trace("DLIndicator update, value: " + value);
            downloadProgressWindow.setPercent(value);
            return;
        }

        public boolean isCancelled() {
            // What, us cancel the download?
            trace("OTA is checking for a user cancel");

            // check if the cancel button has been pressed
            // maybe use a downloadProgressWindow.isCancelled()?
            if (downloadProgressWindow.isCancelled()) {
                interrupt();
                return true;
            }
            return false;
        }

        Object synchronizationObject = new Object();
        public Object getLockNotifier() {
            return synchronizationObject;
        }

        public void downloadDone() {
            trace("OTA tells us the download is done (but maybe not 100%)");
        }

        // End DLIndicator implementation

        public void run() {
            AppModule app[] = doInstall(descriptor, menu);
            if (run && app != null) {
                downloadProgressWindow.dispose();
                for (int i = 0; i < app.length; i++) {
                    presentationMode.launchApp(app[i]);
                    try {
                        Thread.currentThread().sleep(1000 * 2);
                    }
                    catch (Exception e) {
                        e.printStackTrace();
                    }
                }
            }
            else if (app == null) {
                presentationMode.showInfoDialog(presentationMode.getString(
                    "AwtPDAPresentationMode.downloader.downloaderror"));
                downloadProgressWindow.dispose();
            }
        }

        private AppModule[] doInstall(Descriptor descriptor, String saveInMenu) {
            AppRepository repository = AppManager.getAppRepository();
            Destination dest = repository.getDestination(descriptor);

            // Download the app from the server
            boolean result = doDownload(descriptor, dest);
            if (result) {
                // Install the application into the repository
                AppModule module[] =
                    repository.installApplication(dest, saveInMenu);

                if (module != null) {
                    for (int i = 0; i < module.length; i++) {
                        presentationMode.updateNewApp(module[i]);
                    }
                }

                return module;
            }
            return null;
        }

        private boolean doDownload(Descriptor d, Destination dest) {
            if (d == null || omaOTA == null) {
                return false;
            }
            try {
                omaOTA.download(d, dest, this);
            }
            catch (Exception o) {
                o.printStackTrace();
                return false;
            }

            return true;
        }
    }

    class CDCAmsDestination
        implements Destination {

        byte[] buffer;
        int bufferIndex = 0;
        Descriptor descriptor = null;

        public CDCAmsDestination(Descriptor descriptor) {
            this.descriptor = descriptor;
            buffer = new byte[descriptor.getSize()];
        }

        public byte[] getBuffer() {
            return buffer;
        }

        public Descriptor getDescriptor() {
            return descriptor;
        }

        public void acceptMimeType(String mimeType) throws OTAException {
            trace("saying we handle mimetype " + mimeType);
            return;
        }

        public void start(String sourceURL,
                          String mimeType) throws OTAException, IOException {
            trace("download is about to start from " + sourceURL +
                  ", of type " + mimeType);
            return;
        }

        public int receive(InputStream in, int desiredLength) throws
            OTAException, IOException {

            trace("receiving data ");

            int numRead = in.read(buffer, bufferIndex, desiredLength);
            if (numRead > 0) {
                bufferIndex += numRead;
            }
            return numRead;
        }

        public void finish() throws OTAException, IOException {
            trace("download succeeded. save the results");
            return;
        }

        public void abort() {
            trace("download aborted");
            return;
        }

        public int getMaxChunkSize() {
            trace("saying we'll take any chunksize");
            return 0;
        }
    }

    class IconDLIndicator
        implements DLIndicator {

        public IconDLIndicator() {
        }

        // Begin DLIndicator implementation
        public int getGranularity() {
            return 10;
        }

        public void updatePercent(int value) {
            trace("DLIndicator update, value: " + value);
            return;
        }

        public boolean isCancelled() {
            // What, us cancel the download?
            trace("OTA is checking for a user cancel");
            return false;
        }

        Object synchronizationObject = new Object();
        public Object getLockNotifier() {
            return synchronizationObject;
        }

        public void downloadDone() {
            trace("OTA tells us the download is done (but maybe not 100%)");
        }
    }

    class IconDestination
        implements Destination {

        // Begin Destination implementation
        byte[] buffer1 = null;
        byte[] buffer2 = null;
        int bufferIndex = 0;
        Descriptor descriptor = null;

        public IconDestination() {
        }

        public byte[] getBuffer() {
            if (buffer1 == null) {
                return buffer2;
            }
            else {
                return buffer1;
            }
        }

        public Descriptor getDescriptor() {
            return descriptor;
        }

        public void acceptMimeType(String mimeType) throws OTAException {
            trace("saying we handle mimetype " + mimeType);
            return;
        }

        public void start(String sourceURL,
                          String mimeType) throws OTAException, IOException {
            trace("download is about to start from " + sourceURL +
                  ", of type " + mimeType);
            return;
        }

        public int receive(InputStream in, int desiredLength) throws
            OTAException, IOException {

            trace("receiving data: " + desiredLength + " bytes.");

            // The buffer needs to be at least as big as the desiredLength
            if (buffer1 == null && buffer2 == null) {
                buffer1 = new byte[desiredLength];
            }

            int numRead = 0;
            while (numRead <= desiredLength && numRead != -1) {
                if (buffer1 != null) {
                    numRead = in.read(buffer1, bufferIndex,
                                      desiredLength - bufferIndex);
                    if (numRead > 0) {
                        bufferIndex += numRead;
                    }
                    if (bufferIndex >= buffer1.length) {
                        buffer2 = new byte[buffer1.length * 2];
                        System.arraycopy(buffer1, 0, buffer2, 0, buffer1.length);
                        buffer1 = null;
                    }
                }
                if (buffer2 != null) {
                    numRead = in.read(buffer2, bufferIndex,
                                      desiredLength - bufferIndex);
                    if (numRead > 0) {
                        bufferIndex += numRead;
                    }
                    if (bufferIndex >= buffer2.length) {
                        buffer1 = new byte[buffer2.length * 2];
                        System.arraycopy(buffer2, 0, buffer1, 0, buffer2.length);
                        buffer2 = null;
                    }
                }
            }
            return numRead;
        }

        public void finish() throws OTAException, IOException {
            trace("download succeeded. save the results");
            return;
        }

        public void abort() {
            trace("download aborted");
            return;
        }

        public int getMaxChunkSize() {
            trace("saying we'll take any chunksize");
            return 0;
        }
    }

    class ScreenBorder
        extends Border {

        public ScreenBorder() {
            setBorderThickness(4);
            setType(THREED_IN);
            setMargins(4, 4, 2, 2);
//            setType(THREED_OUT);
//            setMargins(3);
        }
    }

}
