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

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.util.Hashtable;
import java.util.Vector;

import javax.microedition.io.Connector;
import javax.microedition.lcdui.Display;
import javax.microedition.lcdui.Displayable;

import com.sun.lwuit.*;
import com.sun.lwuit.animations.CommonTransitions;
import com.sun.lwuit.animations.Transition;
import com.sun.lwuit.events.ActionEvent;
import com.sun.lwuit.events.ActionListener;
import com.sun.lwuit.events.FocusListener;
import com.sun.lwuit.geom.Dimension;
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
import com.sun.midp.installer.DiscoveryApp;
import com.sun.midp.io.j2me.storage.File;
import com.sun.midp.io.j2me.storage.RandomAccessStream;
import com.sun.midp.log.LogChannels;
import com.sun.midp.log.Logging;
import com.sun.midp.main.*;
import com.sun.midp.midletsuite.*;
import com.sun.midp.payment.*;


/**
 * The Graphical MIDlet selector Screen.
 * <p>
 * It displays a list or grid of currently installed
 * MIDlets/MIDlet suites (including the Installer MIDlet). Each MIDlet or
 * MIDlet suite is represented by an icon with a name under it.
 * An icon from a jad file for the MIDlet/MIDlet suite representation
 * is used if possible, otherwise a default icon is used.
 *
 * There is a a set of commands per MIDlet/MIDlet suite. Note that
 * the set of commands can change depending on the corresponding MIDlet state.
 * For MIDlets/MIDlet suites that are not running the following commands are
 * available:
 * <ul>
 * <li><b>Launch</b>: Launch the MIDlet or the MIDlet Selector
 *      if it is a suite.
 * <li><b>Remove</b>: Remove the MIDlet/MIDlet suite teh user selected
 *      (with confirmation). </li>
 * <li><b>Update</b>: Update the MIDlet/MIDlet suite the user selected.</li>
 * <li><b>Info</b>: Show the user general information
 *    of the selected MIDlet/MIdlet suite. </li>
 * <li><b>Settings</b>: Let the user change the manager's settings.
 * </ul>
 *
 * For MIDlets/MIDlet suites that are running the following commands are
 * available:
 * <ul>
 * <li><b>Bring to foreground</b>: Bring the running MIDlet to foreground
 * <li><b>End</b>: Terminate the running MIDlet
 * <li><b>Remove</b>: Remove the MIDlet/MIDlet suite teh user selected
 *      (with confirmation). </li>
 * <li><b>Update</b>: Update the MIDlet/MIDlet suite the user selected.</li>
 * <li><b>Info</b>: Show the user general information
 *    of the selected MIDlet/MIdlet suite. </li>
 * <li><b>Settings</b>: Let the user change the manager's settings.
 * </ul>
 *
 */
public class AppManagerUIImpl implements AppManagerUI, ActionListener {
    /* Forms */
    private Form mainMenu;
    private ThemesForm themesForm;
    private AppInfoForm appInfoForm;
    private AppSettingsForm appSettingsForm;
    private SelectorForm selectorForm;
    private SplashScreen splashScreen;
    private DiscoveryApp discoveryApp;

    private Container buttonsContainer;

    /* hard coded button size parameters */
    private static final int GRID_CELL_SIZE = 80;
    private static final int LIST_ROW_HEIGHT = 20;
    private static final int LARGE_ICON_SIZE = 40;
    private static final int SMALL_ICON_SIZE = 20;

    private int cols = 0;
    private int gridRows = 0;
    private int listRows = 0;
    private int displayWidth = 0;
    private int displayHeight = 0;

    /* button width:  can be screen wide, or square */
    private int currentWidth = 0;
    private int currentHeight = 0;

    private final int GRID_STYLE = 0;
    private final int LIST_STYLE = 1;

    public static final String[] LAYOUTS = {"Grid", "List"};

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

    /** Command object for "Application settings". */
    private Command appSettingsCmd =
	new Command(Resource.
		    getString(ResourceConstants.APPLICATION_SETTINGS),
		    ResourceConstants.APPLICATION_SETTINGS);

    /** Command object for "Ok" command for the remove form. */
    private Command okCmd =
	new Command(Resource.getString(ResourceConstants.OK),
		    ResourceConstants.OK);

    /** Command object for "Cancel" command for the remove form. */
    private Command cancelCmd =
	new Command(Resource.getString(ResourceConstants.CANCEL),
		    ResourceConstants.CANCEL);


    /** Command object for "Bring to foreground". */
    private Command fgCmd = new Command(Resource.getString
					(ResourceConstants.FOREGROUND),
					ResourceConstants.FOREGROUND);

    /** Command object for "End" midlet. */
    private Command endCmd = new Command(Resource.getString
					 (ResourceConstants.END),
					 ResourceConstants.END);


    /** Command object for "Toggle icons" command. */
    private Command changeStyleCmd = new Command(Resource.getString
					   (ResourceConstants.AMS_CHANGE_STYLE),
					   ResourceConstants.AMS_CHANGE_STYLE);



    /** Display for the Manager MIDlet. */
    private ApplicationManager manager;

    /** task manager */
    private AppManagerPeer appManager;

    /** MIDlet Suite storage object. */
    private MIDletSuiteStorage midletSuiteStorage;

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

    private Transition dialogTransition;

    /* transition speed */
    private final int RUN_SPEED = 500;


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
	initTheme();


	midletSuiteStorage = MIDletSuiteStorage.getMIDletSuiteStorage();
	createMainMenu();
	mainMenu.setCommandListener(this);

	dialogTransition = CommonTransitions.createSlide(
	    CommonTransitions.SLIDE_VERTICAL, true, RUN_SPEED);

	/* create forms */
	appInfoForm = new AppInfoForm(mainMenu);
	themesForm = new ThemesForm(mainMenu, this);
	selectorForm = new SelectorForm(mainMenu, manager);
	splashScreen = new SplashScreen(mainMenu);
	discoveryApp = new DiscoveryApp(mainMenu);

	splashScreen.show();
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
	System.out.println(">>>AppManagerUIImpl with askUserIfLaunchMidlet() " + askUserIfLaunchMidlet);
    }


    /**
     * The AppManagerPeer manages list of available MIDlet suites
     * and informs AppManagerUI regarding changes in list through
     * itemAppended callback when new item is appended to the list.
     *
     * @param suiteInfo the midlet suite info
     */
    public void itemAppended(RunningMIDletSuiteInfo suiteInfo) {
	System.out.println(">>>itemAppended()");
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
	buttonsContainer.addComponent(button);
	/* add button to hash tables */
	midletsHash.put(button, suiteInfo);
	reverseHash.put(suiteInfo, button);
	largeIconsHash.put(button, tmpIcon);
	smallIconsHash.put(button, tmpIcon.scaled(SMALL_ICON_SIZE, SMALL_ICON_SIZE));


	button.getStyle().setBgTransparency(0x00);
	button.getStyle().setScaleImage(true);
	button.setBorderPainted(false);
	button.setAlignment(currentIconAlignment);	    /*set horizontal alignment*/
        button.setVerticalAlignment(Label.BOTTOM);  	    /*set vertical alignment*/
	button.setTextPosition(currentTextAlignment);
	button.setIcon((Image)currentIconsHash.get(button));

	/* set button width */
	Dimension newSize = new Dimension(currentWidth, currentHeight);
	button.setPreferredSize(newSize);

	button.addActionListener(bAListener);
	button.addFocusListener(buttonsFocusListener);

	/* Create commands list */
	Vector commandsVector = new Vector();

	/* Add commands */
	commandsVector.addElement(exitCommand);
	commandsVector.addElement(changeStyleCmd);

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
            commandsVector.addElement(appSettingsCmd);
            if (suiteInfo.enabled) {
                // setDefaultCommand will add default command first
                commandsVector.addElement(launchCmd);
            }
        }
	commandsHash.put(button, commandsVector);
	System.out.println("<<<itemAppended()");
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
	System.out.println(">>>itemRemoved()");

	midletsHash.remove(button);
	reverseHash.remove(suiteInfo);
	smallIconsHash.remove(button);
	largeIconsHash.remove(button);
	commandsHash.remove(button);
	buttonsContainer.removeComponent(button);

	Dialog.show(Resource.getString(	//title
		    ResourceConstants.AMS_INFORMATION),
	    Resource.getString(//text
		ResourceConstants.AMS_GRA_INTLR_REM_COMPL),
	    new Command[]{okCmd},//commands
	    Dialog.TYPE_INFO,//type
	    null,//icon
	    0,//timeout,
	    dialogTransition);//transition

	mainMenu.show();
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
	System.out.println(">>>notifyMidletStarted()");
	Button b = (Button)reverseHash.get(si);
	Vector v = (Vector)commandsHash.get(b);

	/* set started style */
	markStarted(b);

	/* remove nonrelevant commands */
	int i = 0;
	while (i < v.size()) {
	    if ((Command)v.elementAt(i) == launchCmd) {
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
	System.out.println(">>>notifyMidletStateChanged()  State is " +
			   si.proxy.getMidletState());
    }

    /**
     * Called when a running internal midlet exited.
     * @param midlet
     */
    public void notifyInternalMidletExited(MIDletProxy midlet) {
	System.out.println(">>>notifyInternalMidletExited()");
	com.sun.lwuit.Display.init(manager);
	mainMenu.show();
    }

    /**
     * Called when a running midlet exited.
     * @param si corresponding midlet suite info
     */
    public void notifyMidletExited(RunningMIDletSuiteInfo si) {
	System.out.println(">>>notifyMidletExited()");
	Button b = (Button)reverseHash.get(si);
	markExited(b);

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
	System.out.println(">>>notifySuiteInstalled()");

	mainMenu.show();

	String msg = new String();
	msg += si.displayName +
	    Resource.getString(ResourceConstants.AMS_GRA_INTLR_SUCC_INSTALLED);

	/* IMPL_NOTE:  if uncommenting, image freezes */
//         Dialog.show(Resource.getString( //title
//                     ResourceConstants.AMS_INFORMATION),
//                     msg,//text
//                     new Command[]{okCmd},//commands
//                     Dialog.TYPE_INFO,//type
//                     null,//icon
//                     0,//timeout,
//                     dialogTransition);//transition
    }

    /**
     * Called when a new MIDlet suite is installed externally.
     * @param si corresponding suite info
     */
    public void notifySuiteInstalledExt(RunningMIDletSuiteInfo si) {
	System.out.println(">>>notifySuiteInstalledExt()");
    }

    /**
     * Called when a MIDlet suite has been removed externally.
     * @param si corresponding suite info
     */
    public void notifySuiteRemovedExt(RunningMIDletSuiteInfo si) {
	System.out.println(">>>notifySuiteRemovedExt()");
    }

    /**
     * Called when MIDlet suite being enabled
     * @param si corresponding suite info
     */
    public void notifyMIDletSuiteEnabled(RunningMIDletSuiteInfo msi) {

	System.out.println(">>>notifyMIDletSuiteEnabled()");
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
	System.out.println(">>>notifyMIDletSuiteIconChaged()");
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
	System.out.println(">>>notifyMidletStartError()");
    }

    /**
     * Called when state of the midlet changes.
     *
     * @param si corresponding suite info
     * @param newSi new suite info
     */
    public void notifyMIDletSuiteStateChaged(RunningMIDletSuiteInfo si,
					     RunningMIDletSuiteInfo newSi) {
	System.out.println(">>>notifyMIDletSuiteStateChaged()");
    }

    /**
     * Requests that the ui element, associated with the specified midlet
     * suite, be visible and active.
     *
     * @param item corresponding suite info
     */
    public void setCurrentItem(RunningMIDletSuiteInfo item) {
	System.out.println(">>>setCurrentItem()");
    }

    /**
     * Called to determine MidletSuiteInfo of the last selected Item.
     * Is used to restore selection in the app manager.
     *
     * @return last selected MidletSuiteInfo
     */
    public RunningMIDletSuiteInfo getSelectedMIDletSuiteInfo() {
	System.out.println(">>>getSelectedMIDletSuiteInfo()");
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
	System.out.println(">>>showMidletSwitcher()");
	mainMenu.show();
    }

    /**
     * Called when midlet selector is needed. Should show a list of
     * midlets present in the given suite and allow to select one.
     *
     * @param msiToRun a suite from which a midlet must be selected
     */
    public void showMidletSelector(RunningMIDletSuiteInfo msiToRun){
	System.out.println(">>>showMidletSelector()");
    }



    /**
     * Called by Manager when destroyApp happens to clean up data.
     *
     */
    public void cleanUp() {
	System.out.println(">>>cleanUp()");
    }

    /**
     * Returns the main displayable of the AppManagerUI.
     * @return main screen
     */
    public Displayable getMainDisplayable() {
	System.out.println(">>>getMainDisplayable()");
	return null;
    }


    /**
     * Commands handler dispatcher.
     *
     * @param evt: action event with the command ID
     */
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

	case ResourceConstants.INFO:
	    handlerInfo();
	    break;

	case ResourceConstants.AMS_CHANGE_STYLE:
	    handlerChangeStyle();
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

    /**
     * Getter for current style
     */
    public int getCurrentStyle() {
	return currentStyle;
    }

    /**
     * Interface method to switch layouts
     *
     * @param layoutName:  new layout name
     * Currently "List" and "Grid" are supported.
     * IMPL_NOTE:  refactor to support more layouts
     */
    public void setIconsStyle(String styleName){
	if ((currentStyle == GRID_STYLE && "List".equals(styleName))||
	    (currentStyle == LIST_STYLE && "Grid".equals(styleName))) {
		handlerToggleIconsView();
	}
    }

    /**
     * Class needed to intercept button pressed events
     *
     */
    private class ButtonActionListener implements ActionListener {

        public void actionPerformed(ActionEvent evt) {
	    RunningMIDletSuiteInfo msi = ((RunningMIDletSuiteInfo)
					 midletsHash.get(evt.getSource()));

	    if (msi.midletToRun.equals(AppManagerPeer.DISCOVERY_APP)) {
		handlerInstall();
	    }
	    else if (msi.midletToRun.equals(AppManagerPeer.CA_MANAGER)) {
		manager.launchCaManager();
	    }
	    else if (msi.midletToRun.equals(AppManagerPeer.ODT_AGENT)) {
		manager.launchODTAgent();
	    }
	    else {
		handlerLaunch();
	    }

        }
    }

    /**
     * Converts from javax Image to Lwuit image
     *
     * @param sourceImage image in javax format
     * @return Image in Lwuit format
     */
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



    /**
     * Launches focused midlet/midlet suite
     *
     */
    private void handlerLaunch() {
	RunningMIDletSuiteInfo msi = ((RunningMIDletSuiteInfo)
				     (midletsHash.get(mainMenu.getFocused())));

	if (msi.midletToRun.equals(AppManagerPeer.DISCOVERY_APP)) {
	    handlerInstall();
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



    /**
     * Switches between list and grid views
     *
     * IMPL_NOTE:  refactor to support more than 2 view types
     */
    private void handlerToggleIconsView() {
	if (currentStyle == GRID_STYLE) {
	    /* transition to List style */
	    currentIconAlignment = Label.LEFT;
	    currentTextAlignment = Label.RIGHT;
	    currentRows = listRows;
	    currentCols = 1;
	    currentStyle = LIST_STYLE;
	    currentIconsHash = smallIconsHash;
	    currentWidth = displayWidth;
	    currentHeight = LARGE_ICON_SIZE;
	}
	else {
	    /* transition to grid style */
	    currentIconAlignment = Label.CENTER;
	    currentTextAlignment = Label.BOTTOM;
	    currentRows = gridRows;
	    currentCols = cols;
	    currentStyle = GRID_STYLE;
	    currentIconsHash = largeIconsHash;
	    currentWidth = GRID_CELL_SIZE;
	    currentHeight = GRID_CELL_SIZE;
	}

	Dimension newSize = new Dimension(currentWidth, currentHeight);
	int numOfButtons = buttonsContainer.getComponentCount();
	for (int i = 0; i < numOfButtons; i++) {
	    Button currentButton = (Button)buttonsContainer.getComponentAt(i);
	    currentButton.setAlignment(currentIconAlignment);
	    currentButton.setTextPosition(currentTextAlignment);
	    currentButton.setIcon((Image)currentIconsHash.get(currentButton));
	    currentButton.setPreferredSize(newSize);
	}
	buttonsContainer.setLayout(new GridLayout(currentRows, currentCols));
	buttonsContainer.revalidate();
    }

    /**
     * Removes the focused midlet/midlet suite
     *
     */
    private void handlerRemove() {
        String confirmStr;
        String tmpStr;
        String[] midletsNames = new String[1];
        MIDletSuiteImpl midletSuite = null;
	RunningMIDletSuiteInfo msi;


	msi = ((RunningMIDletSuiteInfo)
			     (midletsHash.get(mainMenu.getFocused())));

	confirmStr = new String();

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
		       ResourceConstants.AMS_MGR_REMOVE_QUE, midletsNames) + "\n";

	    tmpStr = new String();

	    /* add missed transactions */
//IMPL_NOTE:  this code does not work
//             tmpStr = PAPICleanUp.checkMissedTransactions(
//                 midletSuite.getID());
//             if (tmpStr != null) {
//                 confirmStr += tmpStr;
//             }

            //tmpStr +=  midletSuite.getProperty("MIDlet-Delete-Confirm");
	    //if (tmpStr != null) {
		//IMPL_NOTE:  bug here
		//confirmStr += tmpStr;
	    //}


	    /* if more than one midlet, add "this suite contains" string */
	    if (!msi.hasSingleMidlet()) {
		confirmStr += Resource.getString
			    (ResourceConstants.AMS_MGR_SUITE_CONTAINS) + ": " + "\n";
	    }

	    /* add midlets names strings */
	    String[] recordStores =
		//IMPL_NOTE:  always returns null, bug here
		midletSuiteStorage.listRecordStores(msi.suiteId);
	    if (recordStores != null) {
		confirmStr += Resource.getString
			(ResourceConstants.AMS_MGR_SUITE_RECORD_STORES) + ": ";
		for (int i = 0; i < recordStores.length; i++) {
		    confirmStr += recordStores[i] + "\n";
		}
	    }

	    /* add reinstall warning string */
	    confirmStr += Resource.getString
			(ResourceConstants.AMS_MGR_REM_REINSTALL, midletsNames);

	    Command cmd = Dialog.show(Resource.getString(	//title
				ResourceConstants.AMS_INFORMATION),
				confirmStr,	//text
				new Command[]{okCmd, cancelCmd},//commands
				Dialog.TYPE_CONFIRMATION,//type
				null,//icon
				0,  //timeout,
				dialogTransition);//transition

	    if (cmd == okCmd) {
		try {
		    appManager.remove(msi);
		    /* removal complete dialog is in itemRemoved() */
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
		    0,  //timeout,
		    dialogTransition);//transition
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

    /**
     * Shows the info form relevant to the currently focused item
     *
     */
    private void handlerInfo() {
	RunningMIDletSuiteInfo msi = ((RunningMIDletSuiteInfo)
				     (midletsHash.get(mainMenu.getFocused())));
	try {
	    appInfoForm.setContents(msi.suiteId);
	} catch ( Throwable t ) {
	    System.out.println(">>>Error updating info form");
	}

	appInfoForm.show();
    }

    /**
     * Displays the change style form
     */
    private void handlerChangeStyle() {
	themesForm.show();
    }


    /**
     * Shows the settings form relevant to the currently focused item
     *
     */
    private void handlerSettings() {
	RunningMIDletSuiteInfo msi = ((RunningMIDletSuiteInfo)
				     (midletsHash.get(mainMenu.getFocused())));
	appSettingsForm = new AppSettingsForm(mainMenu, msi.suiteId);
	appSettingsForm.show();
    }

    /**
     * Brings to foreground currently focused item
     *
     */
    private void handlerForeground() {
	RunningMIDletSuiteInfo msi = ((RunningMIDletSuiteInfo)
		                     (midletsHash.get(mainMenu.getFocused())));
	Button b = (Button)reverseHash.get(msi);

	manager.moveToForeground(msi);
    }

    /**
     * Ends the currently focused item
     *
     */
    private void handlerEnd() {
	RunningMIDletSuiteInfo msi = ((RunningMIDletSuiteInfo)
				     (midletsHash.get(mainMenu.getFocused())));

	manager.exitMidlet(msi);
    }

    /**
     * Shows main install form
     *
     */
    private void handlerInstall() {
	discoveryApp.showMainForm();
    }

    /**
     * Extracts command from vector v and adds them to the main menu
     *
     * @param v Vector containing the commands
     */
    private void addCommands(Vector v) {
	Command c;
	for (int i = 0; i < v.size(); i++) {
	    c = (Command)v.elementAt(i);
	    mainMenu.addCommand(c);
	}
    }

    /**
     * Removes all commands from main menu, adds commands from v and
     * exit command.
     *
     * @param v Vector containing the commands
     */
    private void refreshCommands(Vector v) {
	mainMenu.removeAllCommands();
	addCommands(v);
	mainMenu.setBackCommand(exitCommand);
    }

    /**
     * Marks midlet corresponding to b as started
     *
     * @param b:  button midlet of which is started
     */
    private void markStarted(Button b) {
	b.setBorderPainted(true);
    }


    /**
     * Cancels the started milet mark
     *
     * @param b:  button midlet of which is exited
     */
    private void markExited(Button b) {
	//b.getStyle().setBgTransparency(0x00);
	b.setBorderPainted(false);
    }


    /**
     * Initializes lwuit theme
     *
     * IMPL_NOTE:  make theme name dynamic
     */
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
	} catch (IOException ioe) {
	    ioe.printStackTrace();
	}
    }

    /**
     * Creaetes and initializes the main menu form.  The form is
     * created in list style
     */
    private void createMainMenu() {
	/* calculate cols and rows */
	displayWidth = com.sun.lwuit.Display.getInstance().getDisplayWidth();
	displayHeight = com.sun.lwuit.Display.getInstance().getDisplayHeight();

	cols = displayWidth / GRID_CELL_SIZE;
	gridRows = displayHeight / GRID_CELL_SIZE;
	listRows = displayHeight / LIST_ROW_HEIGHT;

	/* set List style */
	currentIconAlignment = Label.LEFT;
	currentTextAlignment = Label.RIGHT;
	currentRows = listRows;
	currentCols = 1;
	currentStyle = LIST_STYLE;
	currentIconsHash = smallIconsHash;
	currentWidth = displayWidth;
	currentHeight = LARGE_ICON_SIZE;

	buttonsContainer = new Container();
	buttonsContainer.setLayout(new GridLayout(currentRows, currentCols));

	mainMenu = new Form("Java midlets");
	mainMenu.setLayout(new FlowLayout());
	mainMenu.addComponent(buttonsContainer);
    }


    /**
     * Focus listener class needed to process buttons clicks events
     */
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

	    if (v == null) {
		/* v can be null when midlet is removed */
		System.out.println("focusLost(): no commands vector found.");
		return;
	    }

	    for (int i = 0; i < v.size(); i++) {
		mainMenu.removeCommand((Command)v.elementAt(i));
	    }
	}
    }

}
