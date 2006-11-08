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

import java.awt.*;
import java.util.*;
import java.net.*;
import java.awt.event.ActionListener;
import java.awt.event.ActionEvent;
import com.sun.appmanager.impl.CDCAmsAppController;
import com.sun.appmanager.impl.CDCAmsAppManager;

public class AwtPDAPresentationLogin
    extends Panel {

    Image loginPromptImage = null;

    ImageButton loginExitButton = null;
    ImageButton loginLoginButton = null;

    MediaTracker tracker = null;

    TextField password = null;
    TextField confirm = null;
    TextField user = null;

    String loginUserText = null;
    String loginNewUserText = null;
    String loginPasswordText = null;
    String loginConfirmPasswordText = null;

    CDCAmsAppController controller = null;
    AwtPDAPresentationMode presentationMode = null;

    String loggedInUser = null;

    boolean createUserScreen = false;

    NewUserActionListener newUserAction = null;
    LoginActionListener loginAction = null;

    public AwtPDAPresentationLogin(AwtPDAPresentationMode presentationMode) {
        setLayout(null);
        setBackground(presentationMode.backgroundColor);

        this.controller = CDCAmsAppManager.getAppController();
        this.presentationMode = presentationMode;

        Color textColor = Color.black;

        String loginExitButtonFile = presentationMode.getString(
            "AwtPDAPresentationMode.loginExitButton");
        Image loginExitButtonImage = createImage(loginExitButtonFile);
        loginExitButton = new ImageButton(loginExitButtonImage);
        loginExitButton.addActionListener(new ExitActionListener());
        loginExitButton.setLabel(presentationMode.getString(
            "AwtPDAPresentationMode.exitButtonText"));
        loginExitButton.setTextColor(textColor);

        String loginLoginButtonFile = presentationMode.getString(
            "AwtPDAPresentationMode.loginLoginButton");
        Image loginLoginButtonImage = createImage(loginLoginButtonFile);
        loginLoginButton = new ImageButton(loginLoginButtonImage);
        loginLoginButton.setLabel(presentationMode.getString(
            "AwtPDAPresentationMode.loginButtonText"));
        loginLoginButton.setTextColor(textColor);

        loginUserText = presentationMode.getString(
            "AwtPDAPresentationMode.loginUserText");
        loginNewUserText = presentationMode.getString(
            "AwtPDAPresentationMode.loginNewUserText");
        loginPasswordText = presentationMode.getString(
            "AwtPDAPresentationMode.loginPasswordText");
        loginConfirmPasswordText = presentationMode.getString(
            "AwtPDAPresentationMode.loginConfirmPasswordText");

        tracker = new MediaTracker(this);

        String loginPromptImageFile = presentationMode.getString(
            "AwtPDAPresentationMode.loginPrompt");
        loginPromptImage = createImage(loginPromptImageFile);

        tracker.addImage(loginPromptImage, 0);

        try {
            tracker.waitForID(0);
        }
        catch (InterruptedException e) {
            e.printStackTrace();
            return;
        }

        user = new TextField();
        user.setBackground(Color.white);
        password = new TextField();
        password.setEchoChar('*');
        password.setBackground(Color.white);
        confirm = new TextField();
        confirm.setEchoChar('*');
        confirm.setBackground(Color.white);

        newUserAction = new NewUserActionListener();
        loginAction = new LoginActionListener();
    }

    int currentLoginFontSize = 0;
    String currentLoginFontStr = null;
    Font currentLoginFont = null;

    void refresh() {

        currentLoginFontSize = Integer.parseInt(presentationMode.preferences.
                                                getSystemPreference(
            "AwtPDAPresentationMode.loginFontSize"));
        currentLoginFontStr = presentationMode.preferences.getSystemPreference(
            "AwtPDAPresentationMode.loginFont");
        currentLoginFont = new Font(currentLoginFontStr, Font.PLAIN,
                                    currentLoginFontSize);
        user.setFont(currentLoginFont);
        password.setFont(currentLoginFont);
        confirm.setFont(currentLoginFont);
        loginLoginButton.setFont(currentLoginFont);
        loginExitButton.setFont(currentLoginFont);
    }

    void createNewUserScreen() {
        refresh();
        createUserScreen = true;
        removeAll();

        user.setText("");
        user.setBounds(249, 207 - 30, 170, 30);
        password.setBounds(249, 240 - 30, 170, 30);
        confirm.setBounds(249, 273 - 30, 170, 30);

        loginExitButton.setBounds(176, 280, 80, 70);
        loginLoginButton.setBounds(350, 280, 80, 70);
        loginLoginButton.removeActionListener(loginAction);
        loginLoginButton.addActionListener(newUserAction);

        user.setVisible(true);
        password.setVisible(true);
        confirm.setVisible(true);

        add(user);
        add(password);
        add(confirm);
        add(loginExitButton);
        add(loginLoginButton);

        giveTextFieldFocus();
    }

    void createLoginScreen() {

// Check to see if a user is already installed in the system
        String userName = presentationMode.preferences.
            getSystemPreference("AwtPDAPresentationMode.user");

// If there isn't a user on the system, go to the create new user screen
        if (userName == null || userName.equals("")) {
            createNewUserScreen();
            return;
        }

        refresh();
        createUserScreen = false;

        user.setBounds(249, 190, 170, 30);
        password.setBounds(249, 230, 170, 30);
        loginExitButton.setBounds(176, 280, 80, 70);
        loginLoginButton.setBounds(350, 280, 80, 70);
        loginLoginButton.removeActionListener(newUserAction);
        loginLoginButton.addActionListener(loginAction);

        user.setVisible(true);
        password.setVisible(true);

        add(user);
        add(password);
        add(loginExitButton);
        add(loginLoginButton);

        validate();
        giveTextFieldFocus();
    }

    String getUserName() {
        return loggedInUser;
    }

    void resetUserName() {
        presentationMode.preferences.
            setSystemPreference("AwtPDAPresentationMode.user", "");
        presentationMode.preferences.
            setSystemPreference("AwtPDAPresentationMode.password", "");
        presentationMode.preferences.saveSystemPreferences();
        loggedInUser = null;
    }

    public void giveTextFieldFocus() {
        user.requestFocusInWindow();
    }

    public void showAppManagerScreen() {
        user.setVisible(false);
        password.setVisible(false);
        confirm.setVisible(false);
        presentationMode.showAppManagerScreen();
        loginPromptImage = null;
    }

    public void update(Graphics g) {
        paint(g);
    }

    public void paint(Graphics g) {
        g.setFont(currentLoginFont);
        g.setColor(Color.black);
        g.drawImage(loginPromptImage, 10, 68, loginPromptImage.getWidth(null),
                    loginPromptImage.getHeight(null), null);
        FontMetrics fm = getFontMetrics(currentLoginFont);

        /*
        if (!user.isVisible()) {
            user.setVisible(true);
        }

        if (!password.isVisible()) {
            password.setVisible(true);
        }
        */

        if (createUserScreen) {
            g.drawString(loginNewUserText, 135, 170 + fm.getHeight());
            g.drawString(loginPasswordText, 135, 204 + fm.getHeight());
            g.drawString(loginConfirmPasswordText, 135, 235 + fm.getHeight());
            /*
            if (!confirm.isVisible()) {
                confirm.setVisible(true);
            }
            */
        }
        else {
            g.drawString(loginUserText, 175, 185 + fm.getHeight());
            g.drawString(loginPasswordText, 135, 225 + fm.getHeight());
        }

        super.paint(g);
    }

    private Image createImage(String
                              filename) {
        String path = "resources/login/" + filename;
        URL url = getClass().getResource(path);
        return Toolkit.getDefaultToolkit().createImage(url);
    }

    class ExitActionListener
        implements ActionListener {

        public void actionPerformed(ActionEvent e) {
            controller.reboot();
        }
    }

    class NewUserActionListener
        implements ActionListener {

        public void actionPerformed(ActionEvent e) {
            String userName = user.getText();
            String passwd = password.getText();
            String conf = confirm.getText();
            if (!passwd.equals(conf)) {
                presentationMode.showInfoDialog(presentationMode.getString(
                    "AwtPDAPresentationMode.message.mismatchPasswords"));
            }
            else {
                presentationMode.preferences.
                    setSystemPreference("AwtPDAPresentationMode.user", userName);
                presentationMode.preferences.
                    setSystemPreference("AwtPDAPresentationMode.password",
                                        passwd);
                presentationMode.preferences.saveSystemPreferences();
                loggedInUser = userName;
                showAppManagerScreen();
            }
        }
    }

    class LoginActionListener
        implements ActionListener {

        public void actionPerformed(ActionEvent e) {
            String userName = presentationMode.preferences.getSystemPreference(
                "AwtPDAPresentationMode.user");
            String typedUserName = user.getText();
            String passwd = presentationMode.preferences.getSystemPreference(
                "AwtPDAPresentationMode.password");
            String typedPassword = password.getText();

            if (userName.equals(typedUserName) && passwd.equals(typedPassword)) {
                loggedInUser = userName;
                showAppManagerScreen();
            }
            else {
                presentationMode.showInfoDialog(presentationMode.getString(
                    "AwtPDAPresentationMode.message.wrongPassword"));
            }
        }
    }
}
