/*
 *
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.midp.appmanager;

import java.util.Vector;

import com.sun.midp.appmanager.Folder;

import javax.microedition.lcdui.Display;
import javax.microedition.lcdui.Displayable;

import com.sun.lwuit.*;
import com.sun.lwuit.layouts.*;
import com.sun.midp.appmanager.Folder;
import com.sun.midp.appmanager.FolderManager;
import com.sun.midp.main.*;


class AppManagerUIImpl implements AppManagerUI {


    private Form mainMenu;
    private Label label;
    private javax.microedition.lcdui.Display display;
    private Vector folders;
    private Container mainContainer;


    private int elementWidth = 0;
    private int cols = 0;
    private int rows = 0;

    /**
     * Creates the Application Selector Screen.
     * Called if this is the first time AppSelector is being shown.
     *
     * @param manager - The application manager that invoked it
     * @param appManager - The app manager
     * @param display - The display instance associated with the manager
     * @param displayError - The UI used to display error messages
     * @param foldersOn - if folders are used
     */
    AppManagerUIImpl(ApplicationManager manager, AppManagerPeer appManager,
                 Display display, DisplayError displayError, boolean foldersOn) {

	/* init lwuit */
	com.sun.lwuit.Display.init(manager);

	/* calculate cols and rows */
	//int width = com.sun.lwuit.Display.getInstance().getDisplayWidth(); //get the display width

	mainMenu = new Form("Java midlets");

	mainMenu.setLayout(new BorderLayout());
	mainContainer = new Container();
	mainContainer.setLayout(new GridLayout(3, 3));
	mainContainer.addComponent(new Button("label1"));
	mainContainer.addComponent(new Button("label2"));
	mainContainer.addComponent(new Button("label3"));
	mainMenu.addComponent(BorderLayout.CENTER, mainContainer);

	//mainMenu.addComponent(new Label("hello, world!"));	/*works!*/
	mainMenu.show();
    }

    /**
     * Creates the Application Selector Screen.
     * @param manager - The application manager that invoked it
     * @param appManager - The app manager
     * @param display - The display instance associated with the manager
     * @param displayError - The UI used to display error messages
     * @param foldersOn - if folders are used
     * @param askUserIfLaunchMidlet - If true, it is expected that dialog be shown asking
     *             user if last installed midlet should be launched.
     */
    AppManagerUIImpl(ApplicationManager manager, AppManagerPeer appManager,
                 Display display, DisplayError displayError, boolean foldersOn,
                 boolean askUserIfLaunchMidlet) {
    }

    /**
     * The AppManagerPeer manages list of available MIDlet suites
     * and informs AppManagerUI regarding changes in list through
     * itemAppended callback when new item is appended to the list.
     *
     * @param suiteInfo the midlet suite info
     */
    public void itemAppended(RunningMIDletSuiteInfo suiteInfo) {
	/* Convert image */
//         int width = suiteInfo.icon.getWidth();
//         int height = suiteInfo.icon.getHeight();
//         int[] tmp = new int[width * height];
//         suiteInfo.icon.getRGB(tmp, 0, width, 0, 0, width, height);
//         Image tmpIcon = Image.createImage(tmp, width, height);
//
//         /* build button */
//         Button b = new Button(suiteInfo.displayName, tmpIcon){
//             public Image getPressedIcon() {
//                 Image i = getIcon();
//                 return i.scaled((int) (i.getWidth() * 0.8), (int) (i.getHeight() * 0.8));
//             }
//
//             public Image getRolloverIcon() {
//                 Image i = getIcon();
//                 return i.scaled((int) (i.getWidth() * 1.2), (int) (i.getHeight() * 1.2));
//             }
//         };
//         b.getStyle().setBgTransparency(0);
//         b.setBorderPainted(false);
//         b.setAlignment(Label.CENTER);
//         b.setTextPosition(Label.BOTTOM);
	/* Add button */
	//mainContainer.addComponent(b);
	//mainMenu.addComponent(b);

	//elementWidth = Math.max(b.getPreferredW(), elementWidth);

	//TODO:  make dynamic
    }

    /**
     * The AppManagerPeer manages list of available MIDlet suites
     * and informs AppManagerUI regarding changes in list through
     * itemRemoved callback when item is removed from the list.
     *
     * @param suiteInfo the midlet suite info
     */
    public void itemRemoved(RunningMIDletSuiteInfo suiteInfo) {

    }

    /**
     *  Called when a new internal midlet was launched
     * @param midlet proxy of a newly launched MIDlet
     */
    public void notifyInternalMidletStarted(MIDletProxy midlet) {

    }

    /**
     * Called when a new midlet was launched.
     *
     * @param si corresponding midlet suite info
     */
    public void notifyMidletStarted(RunningMIDletSuiteInfo si) {

    }

    /**
     * Called when state of a running midlet has changed.
     *
     * @param si corresponding midlet suite info
     */
    public void notifyMidletStateChanged(RunningMIDletSuiteInfo si) {

    }

    /**
     * Called when a running internal midlet exited.
     * @param midlet
     */
    public void notifyInternalMidletExited(MIDletProxy midlet) {

    }

    /**
     * Called when a running midlet exited.
     * @param si corresponding midlet suite info
     */
    public void notifyMidletExited(RunningMIDletSuiteInfo si) {

    }

    /**
     * Called by AppManagerPeer after a MIDlet suite
     * is successfully installed on the device,
     * to ask the user whether or not to launch
     * a MIDlet from the suite.
     * @param si corresponding suite info
     */
    public void notifySuiteInstalled(RunningMIDletSuiteInfo si) {

    }

    /**
     * Called when a new MIDlet suite is installed externally.
     * @param si corresponding suite info
     */
    public void notifySuiteInstalledExt(RunningMIDletSuiteInfo si) {

    }

    /**
     * Called when a MIDlet suite has been removed externally.
     * @param si corresponding suite info
     */
    public void notifySuiteRemovedExt(RunningMIDletSuiteInfo si) {

    }

    /**
     * Called when MIDlet suite being enabled
     * @param si corresponding suite info
     */
    public void notifyMIDletSuiteEnabled(RunningMIDletSuiteInfo si) {

    }

    /**
     * Called when MIDlet suite icon hase changed
     * @param si corresponding suite info
     */
    public void notifyMIDletSuiteIconChaged(RunningMIDletSuiteInfo si) {

    }

    /**
     * Called when a midlet could not be launched.
     *
     * @param suiteId suite ID of the MIDlet
     * @param className class name of the MIDlet
     * @param errorCode error code
     * @param errorDetails error code details
     */
    public void notifyMidletStartError(int suiteId, String className, int errorCode,
                                String errorDetails) {

    }

    /**
     * Called when state of the midlet changes.
     *
     * @param si corresponding suite info
     * @param newSi new suite info
     */
    public void notifyMIDletSuiteStateChaged(RunningMIDletSuiteInfo si,
                                             RunningMIDletSuiteInfo newSi) {

    }

    /**
     * Requests that the ui element, associated with the specified midlet
     * suite, be visible and active.
     *
     * @param item corresponding suite info
     */
    public void setCurrentItem(RunningMIDletSuiteInfo item) {

    }

    /**
     * Called to determine MidletSuiteInfo of the last selected Item.
     * Is used to restore selection in the app manager.
     *
     * @return last selected MidletSuiteInfo
     */
    public RunningMIDletSuiteInfo getSelectedMIDletSuiteInfo() {
        return null;
    }

    /**
     * Called when midlet selector needed.
     *
     * @param onlyFromLaunchedList true if midlet should
     *        be selected from the list of already launched midlets,
     *        if false then possibility to launch midlet is needed.
     */
    public void showMidletSwitcher(boolean onlyFromLaunchedList) {

    }

    /**
     * Called by Manager when destroyApp happens to clean up data.
     */
    public void cleanUp() {

    }

    /**
     * Returns the main displayable of the AppManagerUI.
     * @return main screen
     */
    public Displayable getMainDisplayable() {
        return null;
    }



}


