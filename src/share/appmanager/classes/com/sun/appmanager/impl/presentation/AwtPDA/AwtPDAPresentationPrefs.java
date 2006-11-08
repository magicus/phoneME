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
import com.sun.appmanager.preferences.Preferences;
import java.awt.*;
import java.awt.event.*;
import java.util.*;
import java.net.URL;

public class AwtPDAPresentationPrefs
    extends Panel {

    AwtPDAPresentationMode presentationMode = null;

    private Preferences preferences = null;

// screens
    protected GeneralPreferencesScreen generalPrefsScreen = null;
    private AwtPDAPresentationMoveRemove moveRemoveScreen = null;

// Preferences buttons
    private ImageButton moveRemoveButton = null;
    private ImageButton generalPrefsButton = null;
    private ImageButton cancelButton = null;
    private ImageButton resetUserPrefsButton = null;
    Image saveButtonImage = null;
    Image cancelButtonImage = null;

    private ImagePanel bottomPanel = null;
    ImagePanel titleBarHeader = null;
    Panel prefsPanel = null;

    private Color slotsColor = new Color(176, 177, 203);

    public AwtPDAPresentationPrefs(AwtPDAPresentationMode presentationMode) {
        this.presentationMode = presentationMode;
        this.preferences = AppManager.getPreferences();
    }

    public void initialize() {
        setLayout(new BorderLayout());

        String titleBarImageFile = "resources/background/"
            + presentationMode.getString("AwtPDAPresentationMode.titleBarImage");
        titleBarHeader = new ImagePanel(
            titleBarImageFile);
        titleBarHeader.setLabel(presentationMode.getString(
            "AwtPDAPresentationMode.preferencesHeader.label"), -1, 17);
        add(titleBarHeader, BorderLayout.NORTH);

        prefsPanel = new Panel();
        prefsPanel.setLayout(new GridLayout(0, 1));

        for (int i = 0; i < 3; i++) {
            Panel slot = new Panel(new FlowLayout(FlowLayout.LEFT));
            if (i % 2 == 0) {
                slot.setBackground(slotsColor);
            }
            else {
                slot.setBackground(Color.white);
            }
            prefsPanel.add(slot, i);
        }

        add(prefsPanel, BorderLayout.CENTER);

        String saveButtonFile = "resources/downloader/" +
            presentationMode.getString(
                "AwtPDAPresentationMode.downloaderMediumButton");
        saveButtonImage = createImage(saveButtonFile);

        String cancelButtonFile = "resources/downloader/" +
            presentationMode.getString(
                "AwtPDAPresentationMode.downloaderMediumButton");
        cancelButtonImage = createImage(cancelButtonFile);

        MediaTracker tracker = new MediaTracker(this);
        try {
            tracker.addImage(saveButtonImage, 0);
            tracker.addImage(cancelButtonImage, 1);
            for (int i = 0; i < 2; i++) {
                tracker.waitForID(i);
            }
        }
        catch (InterruptedException e) {
            e.printStackTrace();
            return;
        }

        /***************** User Buttons *****************/
        Panel p = (Panel) prefsPanel.getComponent(0);

        generalPrefsScreen = new GeneralPreferencesScreen();
        generalPrefsButton = new ImageButton(createIconImage(presentationMode.
            getString(
                "AwtPDAPresentationMode.generalPrefsButton")));

        generalPrefsButton.setLabel(presentationMode.getString(
            "AwtPDAPresentationMode.generalPrefsButton.label"));
        generalPrefsButton.setTextColor(Color.black);
        generalPrefsButton.addActionListener(new
                                             DisplayGeneralPreferencesScreenAction());
        p.add(generalPrefsButton);

        /***************** Move/Remove Screen & Button *****************/
        p = (Panel) prefsPanel.getComponent(1);
        moveRemoveButton = new ImageButton(createIconImage(presentationMode.
            getString(
                "AwtPDAPresentationMode.moveRemoveButton")));
        moveRemoveButton.setLabel(presentationMode.getString(
            "AwtPDAPresentationMode.moveRemoveButton.label"));
        moveRemoveButton.setTextColor(Color.black);
        moveRemoveButton.addActionListener(new DisplayRemoveApplicationsAction());
        p.add(moveRemoveButton);

        /***************** Network Screen & Button *****************/
        p = (Panel) prefsPanel.getComponent(2);
        resetUserPrefsButton = new ImageButton(createIconImage(presentationMode.
            getString(
                "AwtPDAPresentationMode.resetUserPrefsButton")));
        resetUserPrefsButton.setLabel(presentationMode.getString(
            "AwtPDAPresentationMode.resetUserPrefsButton.label"));
        resetUserPrefsButton.addActionListener(new ResetUserPrefsAction());
        resetUserPrefsButton.setTextColor(Color.black);
        p.add(resetUserPrefsButton);

        String bottomBarImageFile = "resources/background/"
            +
            presentationMode.getString("AwtPDAPresentationMode.bottomBarImage");
        bottomPanel = new ImagePanel(bottomBarImageFile);
        bottomPanel.setLayout(new GridLayout(1, 1));

        cancelButton = new ImageButton(createImage(
            "resources/icons/" + presentationMode.getString(
                "AwtPDAPresentationMode.cancelButton")));
        cancelButton.addActionListener(new CancelActionListener());
        cancelButton.setLabel(presentationMode.getString(
            "AwtPDAPresentationMode.closeButton.label"));

        bottomPanel.add(cancelButton);
        add(bottomPanel, BorderLayout.SOUTH);
    }

    public void refresh() {
        titleBarHeader.setFont(presentationMode.currentUserFont);
        resetUserPrefsButton.setFont(presentationMode.currentUserFont);
        moveRemoveButton.setFont(presentationMode.currentUserFont);
        generalPrefsButton.setFont(presentationMode.currentUserFont);
        cancelButton.setFont(presentationMode.currentUserFont);
    }

    void gotoFirstScreen() {
        refresh();

        if (!isAncestorOf(titleBarHeader) && !isAncestorOf(prefsPanel)) {
            removeAll();
            add(titleBarHeader, BorderLayout.NORTH);
            add(prefsPanel, BorderLayout.CENTER);
            add(bottomPanel, BorderLayout.SOUTH);
            validate();
        }
    }

    private Image createImage(String
                              filename) {
        URL url = getClass().getResource(filename);
        return Toolkit.getDefaultToolkit().createImage(url);
    }

    /**************** General Preferences Screen **************************/
    private class GeneralPreferencesScreen
        extends Panel {
        TextField discoveryURLTextField = null;
        Choice fontSizeChoice = null;
        Choice fontChoice = null;
        Choice loginFontChoice = null;
        Choice loginFontSizeChoice = null;
        Choice maxAppsChoice = null;
        Label discoveryURLLabel = null;
        Label fontLabel = null;
        Label fontSizeLabel = null;
        Label loginFontLabel = null;
        Label loginFontSizeLabel = null;
        Label maxAppsLabel = null;
        Button saveButton = null;
        Button cancelButton = null;
        Panel prefsPanel = new Panel();
        Panel buttonPanel = new Panel();

        public GeneralPreferencesScreen() {
            setLayout(new BorderLayout());
            prefsPanel = new Panel(new GridLayout(0, 1));
            buttonPanel = new Panel(new GridLayout(1, 2)) {
                    public Dimension getPreferredSize() {
                        return new Dimension(480, 50);
                    }
                };

            setBackground(Color.white);
            discoveryURLLabel = new Label(presentationMode.getString(
                "AwtPDAPresentationMode.discoveryURL.label"));
            prefsPanel.add(discoveryURLLabel);
            discoveryURLLabel.setFont(presentationMode.currentUserFont);
            discoveryURLLabel.setBackground(slotsColor);
            discoveryURLLabel.setForeground(Color.black);
            discoveryURLTextField = new TextField();
            prefsPanel.add(discoveryURLTextField);
            discoveryURLTextField.setBackground(Color.white);
            discoveryURLTextField.setForeground(Color.black);
            discoveryURLTextField.setFont(presentationMode.currentUserFont);

            String[] fontFamily = GraphicsEnvironment.
                getLocalGraphicsEnvironment().getAvailableFontFamilyNames();

            fontLabel = new Label(presentationMode.getString(
                "AwtPDAPresentationMode.fontPreference.label"));
            fontLabel.setFont(presentationMode.currentUserFont);
            prefsPanel.add(fontLabel);
            fontLabel.setBackground(slotsColor);
            fontLabel.setForeground(Color.black);
            fontChoice = new Choice();
            for (int i = 0; i < fontFamily.length; i++) {
                fontChoice.add(fontFamily[i]);
            }
            prefsPanel.add(fontChoice);
            fontChoice.setForeground(Color.black);
            fontChoice.setBackground(Color.white);
            fontChoice.setFont(presentationMode.currentUserFont);

            fontSizeLabel = new Label(presentationMode.getString(
                "AwtPDAPresentationMode.fontSizePreference.label"));
            fontSizeLabel.setFont(presentationMode.currentUserFont);

            prefsPanel.add(fontSizeLabel);
            fontSizeLabel.setBackground(slotsColor);
            fontSizeLabel.setForeground(Color.black);
            fontSizeChoice = new Choice();
            fontSizeChoice.add("12");
            fontSizeChoice.add("14");
            fontSizeChoice.add("16");
            fontSizeChoice.setFont(presentationMode.currentUserFont);
            prefsPanel.add(fontSizeChoice);
            fontSizeChoice.setForeground(Color.black);
            fontSizeChoice.setBackground(Color.white);

            loginFontLabel = new Label(presentationMode.getString(
                "AwtPDAPresentationMode.loginFontPreference.label"));
            loginFontLabel.setFont(presentationMode.currentUserFont);
            prefsPanel.add(loginFontLabel);
            loginFontLabel.setForeground(Color.black);
            loginFontLabel.setBackground(slotsColor);
            loginFontChoice = new Choice();
            for (int i = 0; i < fontFamily.length; i++) {
                loginFontChoice.add(fontFamily[i]);
            }
            prefsPanel.add(loginFontChoice);
            loginFontChoice.setForeground(Color.black);
            loginFontChoice.setBackground(Color.white);
            loginFontChoice.setFont(presentationMode.currentUserFont);

            loginFontSizeLabel = new Label(presentationMode.getString(
                "AwtPDAPresentationMode.loginFontSizePreference.label"));
            loginFontSizeLabel.setFont(presentationMode.currentUserFont);
            prefsPanel.add(loginFontSizeLabel);
            loginFontSizeLabel.setForeground(Color.black);
            loginFontSizeLabel.setBackground(slotsColor);
            loginFontSizeChoice = new Choice();
            loginFontSizeChoice.add("12");
            loginFontSizeChoice.add("14");
            loginFontSizeChoice.add("16");
            loginFontSizeChoice.setFont(presentationMode.currentUserFont);
            prefsPanel.add(loginFontSizeChoice);
            loginFontSizeChoice.setForeground(Color.black);
            loginFontSizeChoice.setBackground(Color.white);

            maxAppsLabel = new Label(presentationMode.getString(
                "AwtPDAPresentationMode.maxAppsPreference.label"));
            maxAppsLabel.setFont(presentationMode.currentUserFont);
            maxAppsLabel.setForeground(Color.black);
            maxAppsLabel.setBackground(slotsColor);
            prefsPanel.add(maxAppsLabel);
            maxAppsChoice = new Choice();
            for (int i = 5; i < 20; i++) {
                maxAppsChoice.add(Integer.toString(i));
            }
            maxAppsChoice.setFont(presentationMode.currentUserFont);
            prefsPanel.add(maxAppsChoice);
            maxAppsChoice.setForeground(Color.black);
            maxAppsChoice.setBackground(Color.white);

            // Insert cancel button
            cancelButton = new Button();
            cancelButton.setLabel(presentationMode.getString(
                "AwtPDAPresentationMode.cancelButtonLabel"));
            cancelButton.setFont(presentationMode.currentUserFont);
            cancelButton.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent e) {
                    gotoFirstScreen();
                }
            });
            buttonPanel.add(cancelButton);
            cancelButton.setBackground(slotsColor.darker());
            cancelButton.setForeground(Color.black);

            // Insert save buton
            saveButton = new Button();
            saveButton.setLabel(presentationMode.getString(
                "AwtPDAPresentationMode.saveButtonLabel"));
            saveButton.setFont(presentationMode.currentUserFont);
            saveButton.addActionListener(new SaveActionListener());
            buttonPanel.add(saveButton);
            saveButton.setBackground(slotsColor.darker());
            saveButton.setForeground(Color.black);

            add(prefsPanel, BorderLayout.CENTER);
            add(buttonPanel, BorderLayout.SOUTH);

        }

        class SaveActionListener
            implements ActionListener {

            protected SaveActionListener() {
            }

            public void actionPerformed(ActionEvent e) {
                preferences.setUserPreference(presentationMode.currentUser,
                                              "AwtPDAPresentationMode.discoveryURL",
                                              discoveryURLTextField.getText());
                preferences.setUserPreference(presentationMode.currentUser,
                                              "AwtPDAPresentationMode.fontSize",
                                              fontSizeChoice.getSelectedItem());
                preferences.setUserPreference(presentationMode.currentUser,
                                              "AwtPDAPresentationMode.font",
                                              fontChoice.getSelectedItem());

                preferences.setSystemPreference(
                    "AwtPDAPresentationMode.loginFontSize",
                    loginFontSizeChoice.getSelectedItem());
                preferences.setSystemPreference(
                    "AwtPDAPresentationMode.loginFont",
                    loginFontChoice.getSelectedItem());

                preferences.setSystemPreference("AwtPDAPresentationMode.maxVMs",
                                                maxAppsChoice.getSelectedItem());

                boolean updateFonts = false;
                if (fontSizeChoice.getSelectedItem() !=
                    Integer.toString(presentationMode.currentUserFontSize)) {
                    presentationMode.currentUserFontSize = Integer.parseInt(
                        fontSizeChoice.getSelectedItem());
                    updateFonts = true;
                }

                if (fontChoice.getSelectedItem() !=
                    presentationMode.currentUserFontStr) {
                    presentationMode.currentUserFontStr = presentationMode.
                        currentUserFontStr;
                    updateFonts = true;
                }
                if (updateFonts) {
                    presentationMode.updateFonts();
                }

                preferences.saveSystemPreferences();
                preferences.saveUserPreferences(presentationMode.currentUser);
                gotoFirstScreen();
            }

        }

        public void refresh() {
            String DISCOVERY_URL = preferences.getUserPreference(
                presentationMode.currentUser,
                "AwtPDAPresentationMode.discoveryURL");
            discoveryURLTextField.setText(DISCOVERY_URL);

            String fontStr = preferences.getUserPreference(presentationMode.
                currentUser, "AwtPDAPresentationMode.font");
            fontChoice.select(fontStr);

            String fontSizeStr = preferences.getUserPreference(presentationMode.
                currentUser, "AwtPDAPresentationMode.fontSize");
            fontSizeChoice.select(fontSizeStr);

            String loginFontStr = preferences.getSystemPreference(
                "AwtPDAPresentationMode.loginFont");
            loginFontChoice.select(loginFontStr);

            String loginFontSizeStr = preferences.getSystemPreference(
                "AwtPDAPresentationMode.loginFontSize");
            loginFontSizeChoice.select(loginFontSizeStr);

            String maxAppsSizeStr = preferences.getSystemPreference(
                "AwtPDAPresentationMode.maxVMs");
            maxAppsChoice.select(maxAppsSizeStr);

            discoveryURLLabel.setFont(presentationMode.currentUserFont);
            discoveryURLTextField.setFont(presentationMode.currentUserFont);
            fontLabel.setFont(presentationMode.currentUserFont);
            fontChoice.setFont(presentationMode.currentUserFont);
            fontSizeLabel.setFont(presentationMode.currentUserFont);
            fontSizeChoice.setFont(presentationMode.currentUserFont);
            loginFontLabel.setFont(presentationMode.currentUserFont);
            loginFontChoice.setFont(presentationMode.currentUserFont);
            loginFontSizeLabel.setFont(presentationMode.currentUserFont);
            loginFontSizeChoice.setFont(presentationMode.currentUserFont);
            cancelButton.setFont(presentationMode.currentUserFont);
            saveButton.setFont(presentationMode.currentUserFont);
            maxAppsChoice.setFont(presentationMode.currentUserFont);
            maxAppsLabel.setFont(presentationMode.currentUserFont);

            repaint();
        }
    }

    class DisplayGeneralPreferencesScreenAction
        implements ActionListener {
        public void actionPerformed(ActionEvent e) {
            remove(prefsPanel);
            remove(titleBarHeader);
            remove(bottomPanel);
            generalPrefsScreen.refresh();
            add(generalPrefsScreen);
            generalPrefsScreen.refresh();
            validate();
            repaint();
        }
    }

    /**************** Move/Remove Applications Screen **************************/
    class DisplayRemoveApplicationsAction
        implements ActionListener {

        public void actionPerformed(ActionEvent e) {
            remove(prefsPanel);
            remove(titleBarHeader);
            remove(bottomPanel);
            moveRemoveScreen = new AwtPDAPresentationMoveRemove(
                presentationMode);
            moveRemoveScreen.initialize();
            moveRemoveScreen.refresh();
            add(moveRemoveScreen);
            validate();
            repaint();
        }
    }

    class ResetUserPrefsAction
        implements ActionListener {

        public void actionPerformed(ActionEvent e) {
            showOkCancelDialog(presentationMode.getString(
                "AwtPDAPresentationMode.message.resettingPrefs"));
        }
    }

    class DeleteUserPrefsAction
        implements ActionListener {
        public void actionPerformed(ActionEvent e) {
            preferences.deleteUserPreferences(presentationMode.currentUser);
            presentationMode.controller.reboot();
        }
    }

    class CancelActionListener
        implements ActionListener {

        protected CancelActionListener() {
        }

        public void actionPerformed(ActionEvent e) {
            presentationMode.removeCurrentScreen();
        }
    }

    private Image createIconImage(String filename) {
        String path = "resources/preferences/" + filename;
        URL url = getClass().getResource(path);
        return Toolkit.getDefaultToolkit().createImage(url);
    }

    public Dimension getPreferredSize() {
        return new Dimension(480, 462);
    }

    void showOkCancelDialog(String str) {
        DeleteUserPrefsAction deleteUserPrefsAction = new DeleteUserPrefsAction();
        OkCancelDialog dialog = new OkCancelDialog(presentationMode, str,
            deleteUserPrefsAction);
        dialog.pack();
        dialog.show();
    }

    class OkCancelDialog
        extends Dialog {

        AwtPDAPresentationMode presentationMode = null;
        String str = null;
        TextArea textArea = null;

        public OkCancelDialog(AwtPDAPresentationMode presentationMode,
                              String string, ActionListener okAction) {
            super(presentationMode.getFrame(), true);

            setLayout(new BorderLayout());

            this.presentationMode = presentationMode;
            this.str = string;

            textArea = new TextArea(str, -1, -1,
                                    TextArea.SCROLLBARS_VERTICAL_ONLY);

            Font font = presentationMode.currentUserFont;

            if (font == null) {
                int fontSize = 0;
                String fontStr = null;
                fontSize = Integer.parseInt(presentationMode.preferences.
                                            getSystemPreference(
                                                "AwtPDAPresentationMode.loginFontSize"));
                fontStr = presentationMode.preferences.getSystemPreference(
                    "AwtPDAPresentationMode.loginFont");
                font = new Font(fontStr, Font.PLAIN,
                                fontSize);
            }
            textArea.setFont(font);
            textArea.setEditable(false);

            add(textArea, BorderLayout.CENTER);

            // Ok Button
            ImageButton okButton = new ImageButton(presentationMode.
                mediumButtonImage) {
                public Dimension getPreferredSize() {
                    return new Dimension(120, 50);
                }
            };

            okButton.setLabel(presentationMode.getString(
                "AwtPDAPresentationMode.okButton.label"), 50, 26);
            okButton.setTextColor(Color.black);
            okButton.setFont(font);
            okButton.addActionListener(okAction);

            // Cancel Button
            ImageButton cancelButton = new ImageButton(presentationMode.
                mediumButtonImage) {
                public Dimension getPreferredSize() {
                    return new Dimension(120, 50);
                }
            };

            cancelButton.setLabel(presentationMode.getString(
                "AwtPDAPresentationMode.cancelButton.label"), 35, 26);
            cancelButton.setTextColor(Color.black);
            cancelButton.setFont(font);
            cancelButton.addActionListener(new CancelAction());

            Panel buttonPanel = new Panel(new GridLayout(1, 2));
            buttonPanel.add(okButton);
            buttonPanel.add(cancelButton);

            add(buttonPanel, BorderLayout.SOUTH);

            setBackground(Color.white);
            setForeground(Color.black);

            setLocation(100, 150);
        }

        public Dimension getPreferredSize() {
            return new Dimension(250, 300);
        }

        class CancelAction
            implements ActionListener {

            public void actionPerformed(ActionEvent e) {
                dispose();
            }
        }
    }

}
