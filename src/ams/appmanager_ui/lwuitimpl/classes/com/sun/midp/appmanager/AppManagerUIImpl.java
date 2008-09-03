/*
 *
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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

import java.io.IOException;
import java.io.ByteArrayInputStream;

import java.util.Hashtable;
import java.util.Vector;

import javax.microedition.lcdui.Display;
import javax.microedition.lcdui.Displayable;
import javax.microedition.io.Connector;

import com.sun.lwuit.*;
import com.sun.lwuit.events.ActionEvent;
import com.sun.lwuit.events.ActionListener;
import com.sun.lwuit.events.FocusListener;
import com.sun.lwuit.layouts.*;
import com.sun.lwuit.list.DefaultListModel;
import com.sun.lwuit.list.ListModel;
import com.sun.lwuit.plaf.Style;
import com.sun.lwuit.plaf.UIManager;
import com.sun.lwuit.util.Resources;
import com.sun.midp.appmanager.Folder;
import com.sun.midp.appmanager.FolderManager;
import com.sun.midp.configurator.Constants;
import com.sun.midp.i18n.Resource;
import com.sun.midp.i18n.ResourceConstants;
import com.sun.midp.log.LogChannels;
import com.sun.midp.log.Logging;
import com.sun.midp.main.*;
import com.sun.midp.midletsuite.*;
import com.sun.midp.payment.*;

import com.sun.midp.io.j2me.storage.RandomAccessStream;
import com.sun.midp.io.j2me.storage.File;

class AppManagerUIImpl implements AppManagerUI, ActionListener {

    private Form mainMenu;
    private AppInfoForm appInfoForm;
    private AppSettingsForm appSettingsForm;
    private SelectorForm selectorForm;

    private Label label;
    private javax.microedition.lcdui.Display display;
    private Vector folders;
    private Container mainContainer;

    private static final int GRID_CELL_SIZE = 80;
    private static final int LIST_ROW_HEIGHT = 20;
    private static final int LARGE_ICON_SIZE = 40;
    private static final int SMALL_ICON_SIZE = 20;


    private int elementWidth = 0;
    private int cols = 0;
    private int gridRows = 0;
    private int listRows = 0;

    private final int GRID_STYLE = 0;
    private final int LIST_STYLE = 1;

    private int currentStyle = LIST_STYLE;

    private int currentIconAlignment;
    private int currentTextAlignment;
    private int currentRows;
    private int currentCols;

    /* Currently displayed hash table */
    private Hashtable currentIconsHash;

    /** Command object for "Exit" command for splash screen. */
    private Command exitCommand =
	new Command(Resource.getString(ResourceConstants.EXIT),
		    ResourceConstants.EXIT);

    /** Command object for "Launch" install app. */
    private Command launchInstallCmd =
        new Command(Resource.getString(ResourceConstants.LAUNCH),
                    ResourceConstants.LAUNCH);

    /** Command object for "Launch" CA manager app. */
    private Command launchCaManagerCmd =
        new Command(Resource.getString(ResourceConstants.LAUNCH),
                    ResourceConstants.LAUNCH);

    /** Command object for "Launch" ODT Agent app. */
    private Command launchODTAgentCmd =
        new Command(Resource.getString(ResourceConstants.LAUNCH),
                    ResourceConstants.LAUNCH);

    /** Command object for "Launch" */
    private Command launchCmd =
	new Command(Resource.getString(ResourceConstants.LAUNCH),
		    ResourceConstants.LAUNCH);

    /** Command object for "Info". */
    private Command infoCmd =
	new Command(Resource.getString(ResourceConstants.INFO),
		    ResourceConstants.INFO);
    /** Command object for "Remove". */
    private Command removeCmd =
	new Command(Resource.getString(ResourceConstants.REMOVE),
		    ResourceConstants.REMOVE);
    /** Command object for "Update". */
    private Command updateCmd =
	new Command(Resource.getString(ResourceConstants.UPDATE),
		    ResourceConstants.UPDATE);
    /** Command object for "Application settings". */
    private Command appSettingsCmd =
	new Command(Resource.
		    getString(ResourceConstants.APPLICATION_SETTINGS),
		    ResourceConstants.APPLICATION_SETTINGS);
    /** Command object for moving to internal storage. */
    private Command moveToInternalStorageCmd =
	new Command(Resource.
		    getString(ResourceConstants.AMS_MOVE_TO_INTERNAL_STORAGE),
		    ResourceConstants.AMS_MOVE_TO_INTERNAL_STORAGE);
    /** Command object for "Ok" command for the remove form. */
    private Command okCmd =
	new Command(Resource.getString(ResourceConstants.OK),
		    ResourceConstants.OK);
    /** Command object for "Cancel" command for the remove form. */
    private Command cancelCmd =
	new Command(Resource.getString(ResourceConstants.CANCEL),
		    ResourceConstants.CANCEL);
    /** Command object for "Remove" command for the remove form. */
    private Command removeOkCmd =
	new Command(Resource.getString(ResourceConstants.REMOVE),
		    ResourceConstants.REMOVE);

    /** Command object for "Back" command for back to the AppSelector. */
    private Command backCmd =
	new Command(Resource.getString(ResourceConstants.BACK),
		    ResourceConstants.BACK);


    /** Command object for "Bring to foreground". */
    private Command fgCmd = new Command(Resource.getString
					(ResourceConstants.FOREGROUND),
					ResourceConstants.FOREGROUND);

    /** Command object for "End" midlet. */
    private Command endCmd = new Command(Resource.getString
					 (ResourceConstants.END),
					 ResourceConstants.END);

    /** Command object for "Yes" command. */
    private Command runYesCmd = new Command(Resource.getString
					    (ResourceConstants.YES),
					    ResourceConstants.YES);

    /** Command object for "No" command. */
    private Command runNoCmd = new Command(Resource.getString
					   (ResourceConstants.NO),
					   ResourceConstants.NO);

    /** Command object for "Select" command in folder list. */
    private Command selectCmd = new Command(Resource.getString
					   (ResourceConstants.SELECT),
					   ResourceConstants.SELECT);


    /** Command object for "Change folder" command. */
    private Command changeFolderCmd = new Command(Resource.getString
					   (ResourceConstants.AMS_CHANGE_FOLDER),
					   ResourceConstants.AMS_CHANGE_FOLDER);

    /** Command object for "Toggle icons" command. */
    private Command toggleIconsCmd = new Command(Resource.getString
					   (ResourceConstants.AMS_TOGGLE_ICONS_VIEW),
					   ResourceConstants.AMS_TOGGLE_ICONS_VIEW);

    /** Display for the Manager MIDlet. */
    private ApplicationManager manager;

    /** task manager */
    private AppManagerPeer appManager;

    /** MIDlet Suite storage object. */
    private MIDletSuiteStorage midletSuiteStorage;

    /** If there are folders */
    private boolean foldersOn;

    /* mapping:  button->suite info */
    private Hashtable midletsHash = new Hashtable();
    /* mapping:  suite info->button */
    private Hashtable reverseHash = new Hashtable();
    /* mapping:  button->small button icon */
    private Hashtable smallIconsHash = new Hashtable();
    /* mapping:  button->large button icon */
    private Hashtable largeIconsHash = new Hashtable();
    /* mapping:  button->commands list */
    private Hashtable commandsHash = new Hashtable();

    private ButtonsFocusListener buttonsFocusListener =
	new ButtonsFocusListener();


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

	this.manager = manager;
        this.appManager = appManager;

	/* init lwuit */
	com.sun.lwuit.Display.init(manager);
	midletSuiteStorage = MIDletSuiteStorage.getMIDletSuiteStorage();

	initTheme();

	createMainMenu();
	mainMenu.setCommandListener(this);

	appInfoForm = new AppInfoForm(mainMenu);
	selectorForm = new SelectorForm(mainMenu);
	SplashScreen splashScreen = new SplashScreen(mainMenu);
	splashScreen.show();
	//com.sun.lwuit.Display.getInstance().getCurrent().refreshTheme();
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
	System.out.println("AppManagerUIImpl with askUserIfLaunchMidlet called " + askUserIfLaunchMidlet);
    }


    /**
     * The AppManagerPeer manages list of available MIDlet suites
     * and informs AppManagerUI regarding changes in list through
     * itemAppended callback when new item is appended to the list.
     *
     * @param suiteInfo the midlet suite info
     */
    public void itemAppended(RunningMIDletSuiteInfo suiteInfo) {

	/* Convert image to lwuit format*/
	Image tmpIcon = convertImage(suiteInfo.icon);

	/* resize image */
	tmpIcon = tmpIcon.scaled(LARGE_ICON_SIZE, LARGE_ICON_SIZE);

	ButtonActionListener bAListener = new ButtonActionListener();

	/* build button */
	Button button = new Button(suiteInfo.displayName){
	    public Image getPressedIcon() {
		Image i = getIcon();
		return i.scaled((int) (i.getWidth() * 0.8), (int) (i.getHeight() * 0.8));
	    }

	    public Image getRolloverIcon() {
		Image i = getIcon();
		return i.scaled((int) (i.getWidth() * 1.2), (int) (i.getHeight() * 1.2));
	    }
	};

	/* Add button */
	mainContainer.addComponent(button);
	/* add button to hash tables */
	midletsHash.put(button, suiteInfo);
	reverseHash.put(suiteInfo, button);
	largeIconsHash.put(button, tmpIcon);
	smallIconsHash.put(button, tmpIcon.scaled(SMALL_ICON_SIZE, SMALL_ICON_SIZE));


	button.getStyle().setBgTransparency(0x00);
	button.getStyle().setScaleImage(true);
	button.setAlignment(currentIconAlignment);	    /*set horizontal alignment*/
        button.setVerticalAlignment(Label.BOTTOM);  	    /*set vertical alignment*/
	button.setTextPosition(currentTextAlignment);
	button.setIcon((Image)currentIconsHash.get(button));

	button.addActionListener(bAListener);
	button.addFocusListener(buttonsFocusListener);

	/* Create commands list */
	Vector commandsVector = new Vector();

	/* Add commands */
	commandsVector.addElement(exitCommand);
	commandsVector.addElement(toggleIconsCmd);
	commandsVector.addElement(exitCommand);

	if (suiteInfo.midletToRun != null &&
            suiteInfo.midletToRun.equals(AppManagerPeer.DISCOVERY_APP)) {
            /* setDefaultCommand will add default command first */
            commandsVector.addElement(launchCmd);
        } else if (appManager.caManagerIncluded() && suiteInfo.midletToRun != null &&
                   suiteInfo.midletToRun.equals(AppManagerPeer.CA_MANAGER)) {
            commandsVector.addElement(launchCmd);
        } else if (appManager.oddEnabled() && suiteInfo.midletToRun != null &&
                   suiteInfo.midletToRun.equals(AppManagerPeer.ODT_AGENT)) {
            commandsVector.addElement(launchCmd);
        } else {
            commandsVector.addElement(infoCmd);
            commandsVector.addElement(removeCmd);
            commandsVector.addElement(updateCmd);
            commandsVector.addElement(appSettingsCmd);
            if (suiteInfo.storageId != Constants.INTERNAL_STORAGE_ID) {
                commandsVector.addElement(moveToInternalStorageCmd);
            }
            if (suiteInfo.enabled) {
                // setDefaultCommand will add default command first
                commandsVector.addElement(launchCmd);
            }
            if (foldersOn) {
                commandsVector.addElement(changeFolderCmd);
            }
        }
	commandsHash.put(button, commandsVector);
    }

    /**
     * The AppManagerPeer manages list of available MIDlet suites
     * and informs AppManagerUI regarding changes in list through
     * itemRemoved callback when item is removed from the list.
     *
     * @param suiteInfo the midlet suite info
     */
    public void itemRemoved(RunningMIDletSuiteInfo suiteInfo) {

	Button button = (Button)reverseHash.get(suiteInfo);
	System.out.println("itemRemoved():  enter.");

	midletsHash.remove(button);
	reverseHash.remove(suiteInfo);
	smallIconsHash.remove(button);
	largeIconsHash.remove(button);
	commandsHash.remove(button);
	mainContainer.removeComponent(button);

	Dialog.show(Resource.getString(	//title
		    ResourceConstants.AMS_INFORMATION),
	    Resource.getString(//text
		ResourceConstants.AMS_GRA_INTLR_REM_COMPL),
	    new Command[]{okCmd},//commands
	    Dialog.TYPE_INFO,//type
	    null,//icon
	    5000,//timeout,
	    null);//transition


	/* TODO:  remove the commands list */
	//mainMenu.repaint(mainContainer);
    }

    /**
     *  Called when a new internal midlet was launched
     * @param midlet proxy of a newly launched MIDlet
     */
    public void notifyInternalMidletStarted(MIDletProxy midlet) {
	/* nothing to do */
    }

    /**
     * Called when a new midlet was launched.
     *
     * @param si corresponding midlet suite info
     */
    public void notifyMidletStarted(RunningMIDletSuiteInfo si) {
	Button b = (Button)reverseHash.get(si);
	Vector v = (Vector)commandsHash.get(b);

	/* set started style */
	b.getStyle().setBgTransparency(0xff);

	/* remove nonrelevant commands */
	int i = 0;
	while (i < v.size()) {
	    if ((Command)v.elementAt(i) == launchCmd ||
		(Command)v.elementAt(i) == launchInstallCmd ||
		(Command)v.elementAt(i) == launchCaManagerCmd ||
		(Command)v.elementAt(i) == launchODTAgentCmd) {
		v.removeElementAt(i);
	    }
	    else {
		i++;
	    }
	}
	v.addElement(endCmd);
	v.addElement(fgCmd);
        v.removeElement(launchCmd);
	refreshCommands(v);
    }

    /**
     * Called when state of a running midlet has changed.
     *
     * @param si corresponding midlet suite info
     */
    public void notifyMidletStateChanged(RunningMIDletSuiteInfo si) {
	System.out.println("notifyMidletStateChanged called");
	/* TODO:  add fg command */
    }

    /**
     * Called when a running internal midlet exited.
     * @param midlet
     */
    public void notifyInternalMidletExited(MIDletProxy midlet) {
	System.out.println("notifyInternalMidletExited called");
    }

    /**
     * Called when a running midlet exited.
     * @param si corresponding midlet suite info
     */
    public void notifyMidletExited(RunningMIDletSuiteInfo si) {
	Button b = (Button)reverseHash.get(si);
	b.getStyle().setBgTransparency(0x00);
	
	Vector v = (Vector)commandsHash.get(b);
        v.removeElement(fgCmd);
	v.removeElement(endCmd);
	v.addElement(launchCmd);
	refreshCommands(v);
    }

    /**
     * Called by AppManagerPeer after a MIDlet suite
     * is successfully installed on the device,
     * to ask the user whether or not to launch
     * a MIDlet from the suite.
     * @param si corresponding suite info
     */
    public void notifySuiteInstalled(RunningMIDletSuiteInfo si) {
	askUserIfLaunchMidlet();
	System.out.println("notifySuiteInstalled called");
    }

    /**
     * Called when a new MIDlet suite is installed externally.
     * @param si corresponding suite info
     */
    public void notifySuiteInstalledExt(RunningMIDletSuiteInfo si) {
	System.out.println("notifySuiteInstalledExt called");
    }

    /**
     * Called when a MIDlet suite has been removed externally.
     * @param si corresponding suite info
     */
    public void notifySuiteRemovedExt(RunningMIDletSuiteInfo si) {
	System.out.println("notifySuiteRemovedExt called");
    }

    /**
     * Called when MIDlet suite being enabled
     * @param si corresponding suite info
     */
    public void notifyMIDletSuiteEnabled(RunningMIDletSuiteInfo msi) {

	System.out.println("notifyMIDletSuiteEnabled called");
	Button b = (Button)reverseHash.get(msi);
	Vector v = (Vector)commandsHash.get(b);

	if (msi.enabled) {
	    /* add launch command */
	    v.addElement(launchCmd);
	}
	else {
	    /* remove launch command */
	    v.removeElement(launchCmd);
	}
    }

    /**
     * Called when MIDlet suite icon hase changed
     * @param si corresponding suite info
     */
    public void notifyMIDletSuiteIconChaged(RunningMIDletSuiteInfo msi) {
	System.out.println("notifyMIDletSuiteIconChaged called");
	Button b = (Button)reverseHash.get(msi);

	Image tmpIcon = convertImage(msi.icon);
	largeIconsHash.put(b, tmpIcon);
	smallIconsHash.put(b, tmpIcon.scaled(SMALL_ICON_SIZE, SMALL_ICON_SIZE));
	b.setIcon(tmpIcon);
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
	System.out.println("notifyMidletStartError called");
    }

    /**
     * Called when state of the midlet changes.
     *
     * @param si corresponding suite info
     * @param newSi new suite info
     */
    public void notifyMIDletSuiteStateChaged(RunningMIDletSuiteInfo si,
					     RunningMIDletSuiteInfo newSi) {
	System.out.println("notifyMIDletSuiteStateChaged called");
    }

    /**
     * Requests that the ui element, associated with the specified midlet
     * suite, be visible and active.
     *
     * @param item corresponding suite info
     */
    public void setCurrentItem(RunningMIDletSuiteInfo item) {
	System.out.println("setCurrentItem called");
    }

    /**
     * Called to determine MidletSuiteInfo of the last selected Item.
     * Is used to restore selection in the app manager.
     *
     * @return last selected MidletSuiteInfo
     */
    public RunningMIDletSuiteInfo getSelectedMIDletSuiteInfo() {
	System.out.println("getSelectedMIDletSuiteInfo called");
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
	System.out.println("showMidletSwitcher called");
    }

    /**
     * Called when midlet selector is needed. Should show a list of
     * midlets present in the given suite and allow to select one.
     *
     * @param msiToRun a suite from which a midlet must be selected
     */
    public void showMidletSelector(RunningMIDletSuiteInfo msiToRun){
	System.out.println("showMidletSelector called");
    }



    /**
     * Called by Manager when destroyApp happens to clean up data.
     */
    public void cleanUp() {
	System.out.println("cleanUp called");
    }

    /**
     * Returns the main displayable of the AppManagerUI.
     * @return main screen
     */
    public Displayable getMainDisplayable() {
	System.out.println("getMainDisplayable called");
	return null;
    }



    public void actionPerformed(ActionEvent evt) {
	Command cmd = evt.getCommand();
	RunningMIDletSuiteInfo msi;

	switch (cmd.getId()) {
	case ResourceConstants.AMS_MGR_SETTINGS:
	    handlerSettings();
	    break;

	case ResourceConstants.LAUNCH:
	    handlerLaunch();
	    break;

	case ResourceConstants.REMOVE:
	    handlerRemove();
	    break;

	case ResourceConstants.AMS_TOGGLE_ICONS_VIEW:
	    handlerToggleIconsView();
	    break;

	case ResourceConstants.INFO:
	    handlerInfo();
	    break;

	case ResourceConstants.FOREGROUND:
	    handlerForeground();
	    break;

	case ResourceConstants.END:
	    handlerEnd();
	    break;


	case ResourceConstants.EXIT:
		manager.shutDown();
	    break;

	}
    }

    private class ButtonActionListener implements ActionListener {

        public void actionPerformed(ActionEvent evt) {
	    RunningMIDletSuiteInfo msi = ((RunningMIDletSuiteInfo)
					 midletsHash.get(evt.getSource()));

	    //TODO: refactor to avoid code redundancy with main menu
	    if (msi.midletToRun.equals(AppManagerPeer.DISCOVERY_APP)) {
		manager.installSuite();
	    }
	    else if (msi.midletToRun.equals(AppManagerPeer.CA_MANAGER)) {
		manager.launchCaManager();
	    }
	    else if (msi.midletToRun.equals(AppManagerPeer.ODT_AGENT)) {
		manager.launchODTAgent();
	    }
	    else {
		appManager.launchMidlet(msi);
	    }

        }
    }

    public static Image convertImage(javax.microedition.lcdui.Image sourceImage) {
	int width, height;
	int[] tmp;

	if (sourceImage == null) {
	    return null;
	}

	width = sourceImage.getWidth();
	height = sourceImage.getHeight();
	tmp = new int[width * height];
	sourceImage.getRGB(tmp, 0, width, 0, 0, width, height);
	Image tmpImage = Image.createImage(tmp, width, height);
	return tmpImage;
    }



    private void handlerLaunch() {
	RunningMIDletSuiteInfo msi = ((RunningMIDletSuiteInfo)
				     (midletsHash.get(mainMenu.getFocused())));

	if (msi.midletToRun.equals(AppManagerPeer.DISCOVERY_APP)) {
	    manager.installSuite();
	}
	else if (msi.midletToRun.equals(AppManagerPeer.CA_MANAGER)) {
	    manager.launchCaManager();
	}
	else if (msi.midletToRun.equals(AppManagerPeer.ODT_AGENT)) {
	    manager.launchODTAgent();
	}
	else if (msi.hasSingleMidlet()) {
	    appManager.launchMidlet(msi);
	}
	else {
	    selectorForm.setContents(msi);
	    selectorForm.show();
	}
    }



    private void handlerToggleIconsView() {
	if (currentStyle == GRID_STYLE) {
	    /* transition to List style */
	    currentIconAlignment = Label.LEFT;
	    currentTextAlignment = Label.RIGHT;
	    currentRows = listRows;
	    currentCols = 1;
	    currentStyle = LIST_STYLE;
	    currentIconsHash = smallIconsHash;
	}
	else {
	    /* transition to grid style */
	    currentIconAlignment = Label.CENTER;
	    currentTextAlignment = Label.BOTTOM;
	    currentRows = gridRows;
	    currentCols = cols;
	    currentStyle = GRID_STYLE;
	    currentIconsHash = largeIconsHash;
	}

	int numOfButtons = mainContainer.getComponentCount();
	for (int i = 0; i < numOfButtons; i++) {
	    Button currentButton = (Button)mainContainer.getComponentAt(i);
	    currentButton.setAlignment(currentIconAlignment);
	    currentButton.setTextPosition(currentTextAlignment);
	    currentButton.setIcon((Image)currentIconsHash.get(currentButton));
	}
	mainContainer.setLayout(new GridLayout(currentRows, currentCols));
    }

    private void handlerRemove() {
        Dialog confirmDialog;
        StringBuffer temp = new StringBuffer(40);
        String confirmStr;
        String tmpStr;
        String[] midletsNames = new String[1];
	String newLineSeparator;
        MIDletSuiteImpl midletSuite = null;
	RunningMIDletSuiteInfo msi;
	Container confirmContainer;


	msi = ((RunningMIDletSuiteInfo)
			     (midletsHash.get(mainMenu.getFocused())));

	confirmContainer = new Container();
	confirmStr = new String();

	newLineSeparator = System.getProperty("line.separator");
	if (newLineSeparator == null) {
	    newLineSeparator = new String("\n");
	}

        try {
	    /* IMPL_NOTE:  the below code causes an error while trying to remove
	       the midlet:  the midlet seems to be running */
//             midletSuite = midletSuiteStorage.getMIDletSuite(msi.suiteId,
//                                                             false);
	    /* get midlet/midlet suite name */
//             if (msi.hasSingleMidlet()) {
//                 midletsNames[0] = msi.displayName;
//             } else {
//                 midletsNames[0] =
//                     midletSuite.getProperty(MIDletSuiteImpl.SUITE_NAME_PROP);
//             }

	    /* midlet suite removal string */
	    confirmStr += Resource.getString(
		       ResourceConstants.AMS_MGR_REMOVE_QUE, midletsNames);
	    confirmStr += newLineSeparator;

	    tmpStr = new String();

	    /* add missed transactions */
//             tmpStr = PAPICleanUp.checkMissedTransactions(
//                 midletSuite.getID());
//             if (tmpStr != null) {
//                 //IMPL_NOTE:  bug here
//                 //confirmStr += tmpStr;
//             }

            //tmpStr +=  midletSuite.getProperty("MIDlet-Delete-Confirm");
	    if (tmpStr != null) {
		//IMPL_NOTE:  bug here
		//confirmStr += tmpStr;
	    }


	    /* if more than one midlet, add "this suite contains" string */
	    if (!msi.hasSingleMidlet()) {
		confirmStr += Resource.getString
			    (ResourceConstants.AMS_MGR_SUITE_CONTAINS) + ": ";
	    }
	    confirmStr += newLineSeparator;

	    /* add midlets names strings */
	    String[] recordStores =
		//IMPL_NOTE:  always returns null, bug here
		midletSuiteStorage.listRecordStores(msi.suiteId);
	    if (recordStores != null) {
		confirmStr += Resource.getString
			(ResourceConstants.AMS_MGR_SUITE_RECORD_STORES) + ": ";
		for (int i = 0; i < recordStores.length; i++) {
		    confirmStr += recordStores[i] + newLineSeparator;
		}
	    }

	    /* add reinstall warning string */
	    confirmStr += Resource.getString
			(ResourceConstants.AMS_MGR_REM_REINSTALL, midletsNames);

	    TextArea confirmArea = new TextArea(confirmStr);

	    boolean result = Dialog.show(Resource.getString(
			    ResourceConstants.AMS_CONFIRMATION),
			    confirmStr,
                            Dialog.TYPE_CONFIRMATION,
			    null, //convertImage(msi.icon),
		            Resource.getString(
				ResourceConstants.YES),
			    Resource.getString(
				ResourceConstants.NO));

	    if (result == true) {
		try {
		    appManager.remove(msi);
		} catch (Throwable t) {
		    t.printStackTrace();
                }
	    }
	    else {

		Dialog.show(Resource.getString(	//title
			ResourceConstants.AMS_INFORMATION),
		Resource.getString(//text
		    ResourceConstants.AMS_GRA_INTLR_REM_CAN),
		new Command[]{okCmd},//commands
		Dialog.TYPE_INFO,//type
		null,//icon
		5000,//timeout,
		null);//transition

	    }

        } catch (Throwable t) {
	    t.printStackTrace();
            return;
        } finally {
            if (midletSuite != null) {
                midletSuite.close();
            }
	}
    }


    private void handlerInfo() {
	RunningMIDletSuiteInfo msi;

	//TODO:  resolve double code with launch
	msi = ((RunningMIDletSuiteInfo)
				     (midletsHash.get(mainMenu.getFocused())));
	try {
	    appInfoForm.setContents(msi.suiteId);
	} catch ( Throwable t ) {
	    System.out.println("Error updating info form");
	}

	appInfoForm.show();
    }

    private void handlerSettings() {
	RunningMIDletSuiteInfo msi = ((RunningMIDletSuiteInfo)
		(midletsHash.get(mainMenu.getFocused())));
	appSettingsForm = new AppSettingsForm(mainMenu, msi.suiteId);
	appSettingsForm.show();
    }

    private void handlerForeground() {
	RunningMIDletSuiteInfo msi = ((RunningMIDletSuiteInfo)
		(midletsHash.get(mainMenu.getFocused())));
	Button b = (Button)reverseHash.get(msi);
		
	manager.moveToForeground(msi);
    }

    private void handlerEnd() {
	RunningMIDletSuiteInfo msi = ((RunningMIDletSuiteInfo)
		(midletsHash.get(mainMenu.getFocused())));

	manager.exitMidlet(msi);
    }

    private void addCommands(Vector v) {
	Command c;
	for (int i = 0; i < v.size(); i++) {
	    c = (Command)v.elementAt(i);
	    mainMenu.addCommand(c);
	}
    }

    private void refreshCommands(Vector v) {
	mainMenu.removeAllCommands();
	addCommands(v);
	mainMenu.setBackCommand(exitCommand);
    }

    private void askUserIfLaunchMidlet(){
        // Ask the user if he wants to run a midlet from
        // the newly installed midlet suite
	// Ask the user if he wants to run a midlet from
	// the newly installed midlet suite

	System.out.println("askUserIfLaunchMidlet() invoked.");
	String title = Resource.getString(
	    ResourceConstants.AMS_MGR_RUN_THE_NEW_SUITE_TITLE, null);
	String msg = Resource.getString(
	    ResourceConstants.AMS_MGR_RUN_THE_NEW_SUITE, null);

	Dialog.show(title,
		    new Label(msg),
		    new Command[] {launchCmd, cancelCmd});
    }

    private void initTheme() {
	try {
	    RandomAccessStream storage = new RandomAccessStream();
	    storage.connect(File.getStorageRoot(Constants.INTERNAL_STORAGE_ID) +
                "javaTheme.res", Connector.READ);
	    int length = storage.getSizeOf();
	    byte[] resourceData = new byte[length];
	    storage.readBytes(resourceData, 0,length);
	    storage.disconnect();
	    Resources r = Resources.open(new ByteArrayInputStream(resourceData));
	    UIManager.getInstance().setThemeProps(r.getTheme("javaTheme"));
	    //com.sun.lwuit.Display.getInstance().getCurrent().refreshTheme();
	} catch (IOException ioe) {
	    ioe.printStackTrace();
	}
    }

    private void createMainMenu() {
	/* calculate cols and rows */
	int width = com.sun.lwuit.Display.getInstance().getDisplayWidth();
	int height = com.sun.lwuit.Display.getInstance().getDisplayHeight();

	cols = width / GRID_CELL_SIZE;
	gridRows = height / GRID_CELL_SIZE;
	listRows = height / LIST_ROW_HEIGHT;

	/* set List style */
	currentIconAlignment = Label.LEFT;
	currentTextAlignment = Label.RIGHT;
	currentRows = listRows;
	currentCols = 1;
	currentStyle = LIST_STYLE;
	currentIconsHash = smallIconsHash;

	mainContainer = new Container();
	mainContainer.setLayout(new GridLayout(currentRows, currentCols));

	mainMenu = new Form("Java midlets");
	//mainMenu.setLayout(new BorderLayout());
	mainMenu.setLayout(new FlowLayout());
	//mainMenu.addComponent(BorderLayout.CENTER, mainContainer);
	mainMenu.addComponent(mainContainer);
	currentStyle = LIST_STYLE;
    }


    private class ButtonsFocusListener implements FocusListener {
	/**
	 * Invoked when component gains focus
	 * @param cmp the component that gains focus
	 */
	public void focusGained(Component cmp) {
	    Vector v = (Vector)(commandsHash.get((Button)cmp));
	    addCommands(v);
	}
	/**
	 * Invoked when component loses focus
	 * @param cmp the component that lost focus
	 */
	public void focusLost(Component cmp) {
	    Vector v = (Vector)(commandsHash.get((Button)cmp));

	    for (int i = 0; i < v.size(); i++) {
		mainMenu.removeCommand((Command)v.elementAt(i));
	    }
	}
    }

}
