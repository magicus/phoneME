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

import javax.microedition.lcdui.*;

import com.sun.midp.configurator.Constants;

import com.sun.midp.installer.*;
import com.sun.midp.main.*;
import com.sun.midp.midletsuite.*;
import com.sun.midp.midlet.MIDletSuite;
import com.sun.midp.io.j2me.push.PushRegistryInternal;

import com.sun.midp.i18n.Resource;
import com.sun.midp.i18n.ResourceConstants;

import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;

import com.sun.midp.payment.PAPICleanUp;

import java.io.*;
import javax.microedition.rms.*;
import java.util.*;

/**
 * The Graphical MIDlet selector Screen.
 * <p>
 * It displays a list (or grid to be exact) of currently installed
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
 * Exactly one MIDlet from a MIDlet suite could be run at the same time.
 * Each MIDlet/MIDlet suite representation corresponds to an instance of
 * MidletCustomItem which in turn maintains a reference to a MIDletSuiteInfo
 * object (that contains info about this MIDlet/MIDlet suite).
 * When a MIDlet is launched or a MIDlet form a MIDlet suite is launched
 * the proxy instance in the corresponding MidletCustomItem is set to
 * a running MIDletProxy value. It is set back to null when MIDlet exits.
 *
 * Running midlets can be distinguished from non-running MIdlets/MIDlet suites
 * by the color of their name.
 */
class AppManagerUI extends Form
    implements ItemCommandListener, CommandListener {

    /** Constant for the discovery application class name. */
    private static final String DISCOVERY_APP =
        "com.sun.midp.installer.DiscoveryApp";

    /** Constant for the certificate manager class name */
    private static final String CA_MANAGER =
        "com.sun.midp.appmanager.CaManager";

    /** Constant for the graphical installer class name. */
    private static final String INSTALLER =
        "com.sun.midp.installer.GraphicalInstaller";

    /** Constant for the ODT Agent class name. */
    private static final String ODT_AGENT =
        "com.sun.midp.odd.ODTAgentMIDlet";

    /** True if On Device Debug is enabled. */
    private static boolean oddEnabled = false;

    /**
     * The font used to paint midlet names in the AppSelector.
     * Inner class cannot have static variables thus it has to be here.
     */
    private static final Font ICON_FONT = Font.getFont(Font.FACE_SYSTEM,
                                                         Font.STYLE_BOLD,
                                                         Font.SIZE_SMALL);

    /**
     * The image used to draw background for the midlet representation.
     * IMPL NOTE: it is assumed that background image is larger or equal
     * than all other images that are painted over it
     */
    private static final Image ICON_BG =
        GraphicalInstaller.getImageFromInternalStorage("_ch_hilight_bg");

    /**
     * Cashed background image width.
     */
    private static final int bgIconW = ICON_BG.getWidth();

    /**
     * Cashed background image height.
     */
    private static final int bgIconH = ICON_BG.getHeight();

    /**
     * The icon used to display that user attention is requested
     * and that midlet needs to brought into foreground.
     */
    private static final Image FG_REQUESTED =
        GraphicalInstaller.getImageFromInternalStorage("_ch_fg_requested");

    /**
     * The image used to draw disable midlet representation.
     */
    private static final Image DISABLED_IMAGE =
        GraphicalInstaller.getImageFromInternalStorage("_ch_disabled");

    /**
     * The color used to draw midlet name
     * for the hilighted non-running running midlet representation.
     */
    private static final int ICON_HL_TEXT = 0x000B2876;

    /**
     * The color used to draw the shadow of the midlet name
     * for the non hilighted non-running midlet representation.
     */
    private static final int ICON_TEXT = 0x003177E2;

    /**
     * The color used to draw the midlet name
     * for the non hilighted running midlet representation.
     */
    private static final int ICON_RUNNING_TEXT = 0xbb0000;

    /**
     * The color used to draw the midlet name
     * for the hilighted running midlet representation.
     */
    private static final int ICON_RUNNING_HL_TEXT = 0xff0000;

    /**
     * Tha pad between custom item's icon and text
     */
    private static final int ITEM_PAD = 2;

    /**
     * Cashed truncation mark
     */
    private static final char truncationMark =
        Resource.getString(ResourceConstants.TRUNCATION_MARK).charAt(0);


    /** Command object for "Exit" command for splash screen. */
    private Command exitCmd =
        new Command(Resource.getString(ResourceConstants.EXIT),
                    Command.BACK, 1);

    /** Command object for "Launch" install app. */
    private Command launchInstallCmd =
        new Command(Resource.getString(ResourceConstants.LAUNCH),
                    Command.ITEM, 1);

    /** Command object for "Launch" CA manager app. */
    private Command launchCaManagerCmd =
        new Command(Resource.getString(ResourceConstants.LAUNCH),
                    Command.ITEM, 1);

    /** Command object for "Launch" ODT Agent app. */
    private Command launchODTAgentCmd =
        new Command(Resource.getString(ResourceConstants.LAUNCH),
                    Command.ITEM, 1);

    /** Command object for "Launch". */
    private Command launchCmd =
        new Command(Resource.getString(ResourceConstants.LAUNCH),
                    Command.ITEM, 1);
    /** Command object for "Info". */
    private Command infoCmd =
        new Command(Resource.getString(ResourceConstants.INFO),
                    Command.ITEM, 2);
    /** Command object for "Remove". */
    private Command removeCmd =
        new Command(Resource.getString(ResourceConstants.REMOVE),
                    Command.ITEM, 3);
    /** Command object for "Update". */
    private Command updateCmd =
        new Command(Resource.getString(ResourceConstants.UPDATE),
                    Command.ITEM, 4);
    /** Command object for "Application settings". */
    private Command appSettingsCmd =
        new Command(Resource.
                    getString(ResourceConstants.APPLICATION_SETTINGS),
                    Command.ITEM, 5);
    /** Command object for moving to internal storage. */
    private Command moveToInternalStorageCmd =
        new Command(Resource.
                    getString(ResourceConstants.AMS_MOVE_TO_INTERNAL_STORAGE),
                    Command.ITEM, 6);


    /** Command object for "Cancel" command for the remove form. */
    private Command cancelCmd =
        new Command(Resource.getString(ResourceConstants.CANCEL),
                    Command.CANCEL, 1);
    /** Command object for "Remove" command for the remove form. */
    private Command removeOkCmd =
        new Command(Resource.getString(ResourceConstants.REMOVE),
                    Command.SCREEN, 1);

    /** Command object for "Back" command for back to the AppSelector. */
    Command backCmd =
        new Command(Resource.getString(ResourceConstants.BACK),
                    Command.BACK, 1);


    /** Command object for "Bring to foreground". */
    private Command fgCmd = new Command(Resource.getString
                                        (ResourceConstants.FOREGROUND),
                                        Command.ITEM, 1);

    /** Command object for "End" midlet. */
    private Command endCmd = new Command(Resource.getString
                                         (ResourceConstants.END),
                                         Command.ITEM, 1);

    /** Command object for "Yes" command. */
    private Command runYesCmd = new Command(Resource.getString
                                            (ResourceConstants.YES),
                                            Command.OK, 1);

    /** Command object for "No" command. */
    private Command runNoCmd = new Command(Resource.getString
                                           (ResourceConstants.NO),
                                           Command.BACK, 1);

    /** Command object for "Select" command in folder list. */
    private Command selectFolderCmd = new Command(Resource.getString
                                           (ResourceConstants.SELECT),
                                           Command.OK, 1);

    /** Command object for "Select" command in folder list. */
    private Command selectItemFolderCmd = new Command(Resource.getString
                                           (ResourceConstants.SELECT),
                                           Command.OK, 1);

    /** Command object for "Change folder" command. */
    private Command changeFolderCmd = new Command(Resource.getString
                                           (ResourceConstants.AMS_CHANGE_FOLDER),
                                           Command.ITEM, 1);

    /** Command object for "Yes, enable on device debug" command. */
    private Command enableOddYesCmd = new Command(Resource.getString
                                            (ResourceConstants.YES),
                                            Command.OK, 1);

    /** Command object for "No, don't enable on device debug" command. */
    private Command enableOddNoCmd = new Command(Resource.getString
                                           (ResourceConstants.NO),
                                           Command.BACK, 1);

    /** Display for the Manager MIDlet. */
    ApplicationManager manager;

    /** MIDlet Suite storage object. */
    private MIDletSuiteStorage midletSuiteStorage;

    /** Display for the Manager MIDlet. */
    Display display; // = null

    /** Keeps track of when the display last changed, in milliseconds. */
    private long lastDisplayChange;

    /** MIDlet to be removed after confirmation screen was accepted */
    private RunningMIDletSuiteInfo removeMsi;

    /** last Item that was selected */
    private RunningMIDletSuiteInfo lastSelectedMsi;

   /** vector of existing MidletCustomItems */
    private Vector mciVector;

    /**
     * There are several Application Manager
     * midlets from the same "internal" midlet suite
     * that should not be running in the background.
     * appManagerMidlet helps to destroy them
     * (see MidletCustomItem.showNotify).
     */
    private MIDletProxy appManagerMidlet;

    /** UI used to display error messages. */
    private DisplayError displayError;

    /** True, if the CA manager is included. */
    private boolean caManagerIncluded;

    private MIDletSwitcher midletSwitcher;

    /** List of available folders */
    private FolderList folderList;

    /** currently selected folder' id */
    private int currentFolderId;

    /** If there are folders */
    private boolean foldersOn;

    /** custom item for which command to change folder was activated */
    private MidletCustomItem mciToChangeFolder;


    /**
     * Creates and populates the Application Selector Screen.
     * @param manager - The application manager that invoked it
     * @param displayError - The UI used to display error messages
     * @param display - The display instance associated with the manager
     * @param first - true if this is the first time AppSelector is being
     *                shown
     * @param ms - MidletSuiteInfo that should be selected. For the internal
     *             suites midletToRun should be set, for the other suites
     *             suiteId is enough to find the corresponding item.
     */
    AppManagerUI(ApplicationManager manager, Display display,
                 DisplayError displayError, boolean first,
                 MIDletSuiteInfo ms) {
        super(null);

        mciToChangeFolder = null;
        
        folderList = new FolderList();
        folderList.addCommand(exitCmd);
        folderList.addCommand(selectFolderCmd);
        folderList.setSelectCommand(selectFolderCmd);
        folderList.setCommandListener(this);
        foldersOn = folderList.hasItems();

        mciVector = new Vector();

        try {
            caManagerIncluded = Class.forName(CA_MANAGER) != null;
        } catch (ClassNotFoundException e) {
            // keep caManagerIncluded false
        }

        this.manager = manager;
        this.display = display;
        this.displayError = displayError;

        midletSwitcher = new MIDletSwitcher(this, manager, display);

        midletSuiteStorage = MIDletSuiteStorage.getMIDletSuiteStorage();

        setTitle(Resource.getString(ResourceConstants.AMS_MGR_TITLE));
        updateContent();

        if (foldersOn) {
            addCommand(backCmd);
        } else {
            addCommand(exitCmd);
        }
        
        setCommandListener(this);

        if (first) {
            if (foldersOn) {
                display.setCurrent(new SplashScreen(display, folderList));
            } else {
                display.setCurrent(new SplashScreen(display, this));
            }
        } else {
            // if a MIDlet was just installed
            // getLastInstalledMidletItem() will return MidletCustomItem
            // corresponding to this suite, then we have to prompt
            // the user if he want to launch a midlet from the suite.
            MidletCustomItem mci = getLastInstalledMidletItem();
            if (mci != null) {
                if (foldersOn) {
                    // move it to default folder
                    try {
                        midletSuiteStorage.moveSuiteToFolder(mci.msi.suiteId,
                                FolderManager.getDefaultFolderId());
                        mci.msi.folderId = FolderManager.getDefaultFolderId();
                        setFolder(this.currentFolderId);
                    } catch (Throwable t) {
                        displayError.showErrorAlert(mci.msi.displayName, t, null, null);
                    }
                }

                askUserIfLaunchMidlet();
            } else {
                display.setCurrent(this);
                if (ms != null) {
                    // Find item to select
                    if (ms.suiteId == MIDletSuite.INTERNAL_SUITE_ID) {
                        for (int i = 0; i < mciVector.size(); i++) {
                            MidletCustomItem mi =
                                    (MidletCustomItem)mciVector.elementAt(i);
                            if ((mi.msi.suiteId ==
                                    MIDletSuite.INTERNAL_SUITE_ID) &&
                                    mi.msi.midletToRun.equals(ms.midletToRun)) {
                                selectItem(mi);
                                break;
                            }
                        }
                    } else {
                        for (int i = 0; i < mciVector.size(); i++) {
                            MidletCustomItem mi =
                                    (MidletCustomItem)mciVector.elementAt(i);
                            if (mi.msi.suiteId == ms.suiteId) {
                                selectItem(mi);
                                break;
                            }
                        }
                    }
                } // ms != null
            }
        }
    }


    /**
     * Selects specified item. If necessary changes folder to items folder. 
     */
    private void selectItem(MidletCustomItem mi) {
        if (foldersOn) {
            if (currentFolderId != mi.msi.folderId) {
                setFolder(mi.msi.folderId);
            }
        }
        display.setCurrentItem(mi);
    }

    /**
     * Changes current folder and refreshes items on the
     * form accordingly.
     *
     * @param fid new current folder id
     */
    private void setFolder(int fid) {
        if (foldersOn) {
            currentFolderId = fid;
            deleteAll();
            for (int i = 0; i < mciVector.size(); i++) {
                MidletCustomItem mci = (MidletCustomItem)mciVector.elementAt(i);
                if (currentFolderId == mci.msi.folderId) {
                    append(mci);
                }
            }
        } else {
            if (Logging.REPORT_LEVEL <= Logging.ERROR) {
                Logging.report(Logging.ERROR, LogChannels.LC_AMS,
                               "Folders are not supported");
            }
        }
    }
    
    /**
     * Shows ODT Agent midlet in the midlet list. 
     */
    public void showODTAgent() {
        try {
            // check if the ODTAgent midlet is included into the build
            Class.forName(ODT_AGENT);
        } catch (ClassNotFoundException e) {
            // return if the agent is not included
            return;
        }

        // return if ODD is already enabled
        if (oddEnabled) {
            return;
        }

        // warn the user that he enables a potentially dangerous feature
        String title = Resource.getString(
            ResourceConstants.AMS_MGR_ENABLE_ON_DEVICE_DEBUG_TITLE, null);
        String msg = Resource.getString(
            ResourceConstants.AMS_MGR_ENABLE_ON_DEVICE_DEBUG, null);

        Alert alert = new Alert(title, msg, null, AlertType.WARNING);
        alert.addCommand(enableOddNoCmd);
        alert.addCommand(enableOddYesCmd);
        alert.setCommandListener(this);
        alert.setTimeout(Alert.FOREVER);

        display.setCurrent(alert);
    }

    /**
     * Called when midlet selector needed.
     *
     * @param onlyFromLaunchedList true if midlet should
     *        be selected from the list of already launched midlets,
     *        if false then possibility to launch midlet is needed.
     */
    public void showMidletSwitcher(boolean onlyFromLaunchedList) {
        if (onlyFromLaunchedList && midletSwitcher.hasItems()) {
            display.setCurrent(midletSwitcher);
        } else {
            display.setCurrent(this);
        }
    }

    /**
     * Called to determine MidletSuiteInfo of the last selected Item.
     *
     * @return last selected MidletSuiteInfo
     */
    public RunningMIDletSuiteInfo getSelectedMIDletSuiteInfo() {
        return lastSelectedMsi;
    }

    /**
     * Respond to a command issued on any Screen.
     *
     * @param c command activated by the user
     * @param s the Displayable the command was on.
     */
    public void commandAction(Command c, Displayable s) {
        if (c == exitCmd) {
            if ((s == this) || s == folderList) {
                manager.shutDown();
            }
            return;
        }

        // for the rest of the commands
        // we will have to request AppSelector to be displayed
        if (c == removeOkCmd) {

            // suite to remove was set in confirmRemove()
            try {
                remove(removeMsi);
            } catch (Throwable t) {
                if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                    Logging.report(Logging.WARNING, LogChannels.LC_AMS,
                                   "Throwable in removeSuitee");
                }
            }
            return;

        } else if (c == cancelCmd) {

            // null out removeMsi in remove confirmation screen
            removeMsi = null;

        } else if (c == runYesCmd) {

            // user decided run the midlet suite after installation
            MidletCustomItem mciToRun = getLastInstalledMidletItem();
            if (mciToRun != null) {
                display.setCurrentItem(mciToRun);
                launchMidlet(mciToRun.msi);
                return;
            }

        } else if (c == runNoCmd) {

            /*
             * user decided not to run the newly installed midlet suite
             *
             * if a MIDlet was just installed
             * displayLastInstalledMidlet() will return true and
             * make "this" visible with
             * the right MIDlet icon hilighted.
             */
            if (displayLastInstalledMidlet()) {
                // Last installed midlet was set as the current item
                return;
            }

        } else if (c == enableOddYesCmd) {
            // user has confirmed that he wants to enable on device debug
            oddEnabled = true;
            updateContent();
        } else if (c == enableOddNoCmd) {
            // user doesn't want to enable on device debug, do nothing
        } else if (c == selectFolderCmd) {
            if (foldersOn) {
                Folder f = folderList.getSelectedFolder();
                setFolder(f.getId());
            }
        } else if (c == selectItemFolderCmd) {
            if (foldersOn && (null != mciToChangeFolder)) {
                Folder f = folderList.getSelectedFolder();
                int folderId = f.getId();

                if (mciToChangeFolder.msi.folderId != folderId) {
                    try {
                        midletSuiteStorage.moveSuiteToFolder(
                                mciToChangeFolder.msi.suiteId, folderId);
                        mciToChangeFolder.msi.folderId = folderId;
                        if (currentFolderId != folderId) {
                            /*
                             * IMPL_NOTE: optimization possible - we can remove only
                             *  one item and avoid full refresh
                             */
                            setFolder(currentFolderId);
                        }
                    } catch (Throwable t) {
                        displayError.showErrorAlert(
                                mciToChangeFolder.msi.displayName,
                                    t, null, null);
                    }
                    mciToChangeFolder = null;
                }
            }
            display.setCurrent(this);
        } else if (c == backCmd) {
            if ((display.getCurrent() == this) && foldersOn) {
                folderList.removeCommand(selectItemFolderCmd);
                folderList.addCommand(exitCmd);
                folderList.addCommand(selectFolderCmd);
                folderList.setSelectCommand(selectFolderCmd);
                folderList.setSelectedFolder(currentFolderId);
                display.setCurrent(folderList);
                return;
            }
        } else {
            return;
        }

        // for back we just need to display AppSelector
        display.setCurrent(this);
    }

    /**
     * Respond to a command issued on an Item in AppSelector
     *
     * @param c command activated by the user
     * @param item the Item the command was on.
     */
    public void commandAction(Command c, Item item) {
        RunningMIDletSuiteInfo msi = ((MidletCustomItem)item).msi;
        if (msi == null) {
            return;
        }

        if (c == launchInstallCmd) {

            manager.installSuite();

        } else if (c == launchCaManagerCmd) {

            manager.launchCaManager();

        } else if (c == launchODTAgentCmd) {

            manager.launchODTAgent();

        } if (c == launchCmd) {

            launchMidlet(msi);

        } else if (c == infoCmd) {

            try {
                AppInfo appInfo = new AppInfo(msi.suiteId);
                appInfo.addCommand(backCmd);
                appInfo.setCommandListener(this);
                display.setCurrent(appInfo);
            } catch (Throwable t) {
                displayError.showErrorAlert(msi.displayName, t, null, null);
            }

        } else if (c == removeCmd) {

            confirmRemove(msi);

        } else if (c == updateCmd) {

            if (!isInstallerRunning()) {
                manager.updateSuite(msi);
                display.setCurrent(this);
            } else {
                String alertMessage = Resource.getString(
                    ResourceConstants.AMS_MGR_INSTALLER_IS_RUNNING);

                displayError.showErrorAlert(null, null,
                    Resource.getString(ResourceConstants.ERROR),
                    alertMessage);
            }

        } else if (c == appSettingsCmd) {

            try {
                AppSettings appSettings = new AppSettings(msi.suiteId, display,
                                                          displayError, this);
                display.setCurrent(appSettings);

            } catch (Throwable t) {
                displayError.showErrorAlert(msi.displayName, t, null, null);
            }
        } else if (c == moveToInternalStorageCmd) {
            try {
                midletSuiteStorage.changeStorage(msi.suiteId,
                        Constants.INTERNAL_STORAGE_ID);
                
                /*
                 * According to MIDP Spec security requirements we don't allow
                 *  to copy non DRM-protected MIDlet suite to external storage.
                 */
                
                ((MidletCustomItem)item).removeCommand(moveToInternalStorageCmd);
                msi.storageId = Constants.INTERNAL_STORAGE_ID;
                displaySuccessMessage(Resource.getString(
                    ResourceConstants.APPLICATION) + Resource.getString(
                        ResourceConstants.AMS_MGR_SUCC_SUITE_STORAGE_CHANGED));
            } catch (Throwable t) {
                displayError.showErrorAlert(msi.displayName, t, null, null);
            }

        } else if (c == fgCmd) {

            manager.moveToForeground(msi);
            display.setCurrent(this);

        } else if (c == endCmd) {
            manager.exitMidlet(msi);
            display.setCurrent(this);
        } else if (c == changeFolderCmd) {
            if (foldersOn) {
                folderList.removeCommand(exitCmd);
                folderList.removeCommand(selectFolderCmd);
                folderList.addCommand(selectItemFolderCmd);
                folderList.setSelectCommand(selectItemFolderCmd);
                mciToChangeFolder = (MidletCustomItem)item;
                folderList.setSelectedFolder(currentFolderId);
                display.setCurrent(folderList);
            }
        }
    }

    /**
     * Called when a new midlet was launched.
     *
     * @param midlet proxy of a newly added MIDlet
     */
    void notifyMidletStarted(MIDletProxy midlet) {
        String midletClassName = midlet.getClassName();

        if (midletClassName.equals(manager.getClass().getName())) {
            return;
        }

        if (midlet.getSuiteId() == MIDletSuite.INTERNAL_SUITE_ID &&
                !midletClassName.equals(DISCOVERY_APP) &&
                !midletClassName.equals(INSTALLER) &&
                !midletClassName.equals(CA_MANAGER) &&
                !midletClassName.equals(ODT_AGENT)) {
            appManagerMidlet = midlet;
        } else {
            MidletCustomItem ci;
            for (int i = 0; i < mciVector.size(); i++) {
                ci = (MidletCustomItem)mciVector.elementAt(i);

                if (ci.msi.equals(midlet)) {
                    ci.removeCommand(launchCmd);
                    ci.removeCommand(launchInstallCmd);

                    if (caManagerIncluded) {
                        ci.removeCommand(launchCaManagerCmd);
                    }

                    if (oddEnabled) {
                        ci.removeCommand(launchODTAgentCmd);
                    }

                    ci.setDefaultCommand(fgCmd);
                    ci.addCommand(endCmd);
                    if (ci.msi.proxy == null) {
                        // add item to midlet switcher
                        midletSwitcher.append(ci.msi);
                    }
                    ci.msi.proxy = midlet;
                    return;
                }
            }
        }
    }

    /**
     * Called when state of a running midlet was changed.
     *
     * @param midlet proxy of a newly added MIDlet
     */
    void notifyMidletStateChanged(MIDletProxy midlet) {
        MidletCustomItem mci = null;

        for (int i = 0; i < mciVector.size(); i++) {
            mci = (MidletCustomItem)mciVector.elementAt(i);
            if (mci.msi.proxy == midlet) {
                mci.update();
            }
        }
    }

    /**
     * Called when a running midlet exited.
     *
     * @param midlet proxy of a newly added MIDlet
     */
    void notifyMidletExited(MIDletProxy midlet) {
        String midletClassName = midlet.getClassName();

        if (midlet.getSuiteId() == MIDletSuite.INTERNAL_SUITE_ID &&
                !midletClassName.equals(DISCOVERY_APP) &&
                !midletClassName.equals(INSTALLER) &&
                !midletClassName.equals(CA_MANAGER) &&
                !midletClassName.equals(ODT_AGENT)) {
            appManagerMidlet = null;
        } else {
            MidletCustomItem ci;

            for (int i = 0; i < mciVector.size(); i++) {
                ci = (MidletCustomItem)mciVector.elementAt(i);

                if (ci.msi.equals(midlet)) {
                    ci.removeCommand(fgCmd);
                    ci.removeCommand(endCmd);

                    if (ci.msi.midletToRun != null &&
                        ci.msi.midletToRun.equals(DISCOVERY_APP)) {
                        ci.setDefaultCommand(launchInstallCmd);
                    } else if (caManagerIncluded &&
                        ci.msi.midletToRun != null &&
                        ci.msi.midletToRun.equals(CA_MANAGER)) {
                        ci.setDefaultCommand(launchCaManagerCmd);
                    } else if (oddEnabled &&
                        ci.msi.midletToRun != null &&
                        ci.msi.midletToRun.equals(ODT_AGENT)) {
                        ci.setDefaultCommand(launchODTAgentCmd);
                    } else {
                        if (ci.msi.enabled) {
                            ci.setDefaultCommand(launchCmd);
                        }
                    }

                    midletSwitcher.remove(ci.msi);
                    ci.msi.proxy = null;

                    if (removeMsi != null && removeMsi.equals(midlet)) {
                        remove(removeMsi);
                    }

                    /*
                     * When the Installer midlet quites
                     * (it is removed from the running apps list)
                     * this is a good time to see if any new MIDlet suites
                     * where added
                     * Also the CA manager could have disabled a MIDlet.
                     */
                    if (INSTALLER.equals(midletClassName)) {
                        updateContent();
                        /*
                        * After a MIDlet suite is successfully installed on the
                        * device, ask the user whether or not to launch
                        * a MIDlet from the suite.
                        */
                        MidletCustomItem mci = getLastInstalledMidletItem();
                        if (mci != null) {
                            askUserIfLaunchMidlet();
                            return;
                        }
                    } else {
                        if (CA_MANAGER.equals(midletClassName)) {
                            updateContent();
                        }
                        ci.update();
                    }

                    return;
                }
            }
        }

        // Midlet quited; display the application Selector
        display.setCurrent(this);
    }

    /**
     * Called when a midlet could not be launched.
     *
     * @param suiteId suite ID of the MIDlet
     * @param className class name of the MIDlet
     * @param errorCode error code
     * @param errorDetails error code details
     */
    void notifyMidletStartError(int suiteId, String className, int errorCode,
                                String errorDetails) {
        Alert a;
        String errorMsg;

        switch (errorCode) {
        case Constants.MIDLET_SUITE_NOT_FOUND:
            errorMsg = Resource.getString(
                ResourceConstants.AMS_MIDLETSUITELDR_MIDLETSUITE_NOTFOUND);
            break;

        case Constants.MIDLET_CLASS_NOT_FOUND:
            errorMsg = Resource.getString(
              ResourceConstants.AMS_MIDLETSUITELDR_CANT_LAUNCH_MISSING_CLASS);
            break;

        case Constants.MIDLET_INSTANTIATION_EXCEPTION:
            errorMsg = Resource.getString(
              ResourceConstants.AMS_MIDLETSUITELDR_CANT_LAUNCH_ILL_OPERATION);
            break;

        case Constants.MIDLET_ILLEGAL_ACCESS_EXCEPTION:
            errorMsg = Resource.getString(
              ResourceConstants.AMS_MIDLETSUITELDR_CANT_LAUNCH_ILL_OPERATION);
            break;

        case Constants.MIDLET_OUT_OF_MEM_ERROR:
            errorMsg = Resource.getString(
                ResourceConstants.AMS_MIDLETSUITELDR_QUIT_OUT_OF_MEMORY);
            break;

        case Constants.MIDLET_RESOURCE_LIMIT:
        case Constants.MIDLET_ISOLATE_RESOURCE_LIMIT:
            errorMsg = Resource.getString(
                ResourceConstants.AMS_MIDLETSUITELDR_RESOURCE_LIMIT_ERROR);
            break;

        case Constants.MIDLET_ISOLATE_CONSTRUCTOR_FAILED:
            errorMsg = Resource.getString(
                ResourceConstants.AMS_MIDLETSUITELDR_CANT_EXE_NEXT_MIDLET);
            break;

        case Constants.MIDLET_SUITE_DISABLED:
            errorMsg = Resource.getString(
                ResourceConstants.AMS_MIDLETSUITELDR_MIDLETSUITE_DISABLED);
            break;

        case Constants.MIDLET_INSTALLER_RUNNING:
            String[] values = new String[1];
            values[0] = className;
            errorMsg = Resource.getString(
                           ResourceConstants.AMS_MGR_UPDATE_IS_RUNNING,
                           values);
            break;

        default:
            errorMsg = Resource.getString(
                ResourceConstants.AMS_MIDLETSUITELDR_UNEXPECTEDLY_QUIT);
        }

        if (errorDetails != null) {
             errorMsg += "\n\n" + errorDetails;
        }

        displayError.showErrorAlert(null, null,
                                    Resource.getString
                                    (ResourceConstants.EXCEPTION),
                                    errorMsg);
    }

    // ------------------------------------------------------------------

    /**
     * Read in and create a MIDletInfo for newly added MIDlet suite and
     * check enabled state of currently added MIDlet suites.
     */
    private void updateContent() {
        int[] suiteIds;
        RunningMIDletSuiteInfo msi = null;
        boolean newlyAdded;

        suiteIds = midletSuiteStorage.getListOfSuites();

        // Add the Installer as the first installed midlet
        if (mciVector.size() > 0) {
            msi = ((MidletCustomItem)mciVector.elementAt(0)).msi;
        }

        if (msi == null || msi.midletToRun == null ||
            !msi.midletToRun.equals(DISCOVERY_APP)) {

            msi = new RunningMIDletSuiteInfo(MIDletSuite.INTERNAL_SUITE_ID,
                DISCOVERY_APP,
                Resource.getString(ResourceConstants.INSTALL_APPLICATION),
                true) {
                    public boolean equals(MIDletProxy midlet) {
                        if (super.equals(midlet)) {
                            return true;
                        }

                        // there is one exception when 2 midlets belong to the
                        // same icon: Discovery app & Graphical installer.
                        // Graphical Installer can be launched by Discover app
                        // or when MIdlet update is needed.
                        // In such cases we simply need to set the proxy on
                        // corresponding icon (MidletCustomItem).
                        // Note that when Discovery app exits and
                        // Installer is launched
                        // notifyMidletExited() will not find corresponding
                        // icon in the list of MidletCustomItems.
                        // (that midlet exit will be ignored).
                        return (INSTALLER.equals(midlet.getClassName()));
                    }
                };

            append(msi);
        }

        if (caManagerIncluded) {
            // Add the CA manager as the second installed midlet
            if (mciVector.size() > 1) {
                msi = ((MidletCustomItem)mciVector.elementAt(1)).msi;
            }

            if (msi == null || msi.midletToRun == null ||
                !msi.midletToRun.equals(CA_MANAGER)) {
                msi = new RunningMIDletSuiteInfo(MIDletSuite.INTERNAL_SUITE_ID,
                  CA_MANAGER,
                  Resource.getString(ResourceConstants.CA_MANAGER_APP), true);
                append(msi);
            }
        }

        if (oddEnabled) {
            // Add the ODT Agent midlet as the third installed midlet
            if (mciVector.size() > 2) {
                msi = ((MidletCustomItem)mciVector.elementAt(2)).msi;
            }

            if (msi == null || msi.midletToRun == null ||
                !msi.midletToRun.equals(ODT_AGENT)) {
                msi = new RunningMIDletSuiteInfo(MIDletSuite.INTERNAL_SUITE_ID,
                  ODT_AGENT,
                  Resource.getString(ResourceConstants.ODT_AGENT_MIDLET), true);
                append(msi);
            }
        }

        // Add the rest of the installed midlets
        for (int lowest, i = 0; i < suiteIds.length; i++) {

            lowest = i;

            for (int k = i + 1; k < suiteIds.length; k++) {
                if (suiteIds[k] < suiteIds[lowest]) {
                    lowest = k;
                }
            }

            try {
                MIDletSuiteInfo temp =
                    midletSuiteStorage.getMIDletSuiteInfo(suiteIds[lowest]);

                RunningMIDletSuiteInfo suiteInfo =
                    new RunningMIDletSuiteInfo(temp, midletSuiteStorage);

                newlyAdded = true;
                for (int k = 0; k < mciVector.size(); k++) {
                    MidletCustomItem mci = (MidletCustomItem)mciVector.elementAt(k);

                    if (suiteIds[lowest] == mci.msi.suiteId) {
                        newlyAdded = false;
                        boolean isEnabled = suiteInfo.enabled;

                        if (mci.msi.enabled != isEnabled) {
                            mci.msi.enabled = isEnabled;

                            // MIDlet suite being enabled
                            if (isEnabled) {
                                mci.setDefaultCommand(launchCmd);
                            } else { // MIDlet suite is being disabled

                                if (mci.msi.proxy == null) { // Not running
                                    mci.removeCommand(launchCmd);
                                }

                                // running MIDlets will continue to run
                                // even when disabled
                            }
                        }

                        // Update all information about the suite;
                        // if the suite's icon was changed, reload it.
                        String oldIconName = mci.msi.iconName;
                        int oldNumberOfMidlets = mci.msi.numberOfMidlets;
                        MIDletProxy oldProxy = mci.msi.proxy;

                        midletSwitcher.update(mci.msi, suiteInfo);
                        mci.msi = suiteInfo;
                        mci.msi.proxy = oldProxy;

                        if ((suiteInfo.iconName != null &&
                                !suiteInfo.iconName.equals(oldIconName)) ||
                            (suiteInfo.iconName == null &&
                                suiteInfo.numberOfMidlets != oldNumberOfMidlets)
                        ) {
                            mci.msi.icon = null;
                            mci.msi.loadIcon(midletSuiteStorage);
                            mci.icon = mci.msi.icon;
                        }

                        break;
                    }
                }

                if (newlyAdded) {
                    append(suiteInfo);
                }

            } catch (Exception e) {
                // move on to the next suite
            }

            suiteIds[lowest] = suiteIds[i];
        }
    }

    /**
     * Appends a MidletCustomItem to the App Selector Screen
     *
     * @param suiteInfo the midlet suite info
     *                  of the recently started midlet
     */
    private void append(RunningMIDletSuiteInfo suiteInfo) {
    
        MidletCustomItem ci = new MidletCustomItem(suiteInfo);

        if (suiteInfo.midletToRun != null &&
            suiteInfo.midletToRun.equals(DISCOVERY_APP)) {
            // setDefaultCommand will add default command first
            ci.setDefaultCommand(launchInstallCmd);
        } else if (caManagerIncluded && suiteInfo.midletToRun != null &&
                   suiteInfo.midletToRun.equals(CA_MANAGER)) {
            // setDefaultCommand will add default command first
            ci.setDefaultCommand(launchCaManagerCmd);
        } else if (oddEnabled && suiteInfo.midletToRun != null &&
                   suiteInfo.midletToRun.equals(ODT_AGENT)) {
            ci.setDefaultCommand(launchODTAgentCmd);
        } else {
            ci.addCommand(infoCmd);
            ci.addCommand(removeCmd);
            ci.addCommand(updateCmd);
            ci.addCommand(appSettingsCmd);
            if (suiteInfo.storageId != Constants.INTERNAL_STORAGE_ID) {
                ci.addCommand(moveToInternalStorageCmd);
            }
            if (suiteInfo.enabled) {
                // setDefaultCommand will add default command first
                ci.setDefaultCommand(launchCmd);
            }
            if (foldersOn) {
                ci.addCommand(changeFolderCmd);
            }
        }

        ci.setItemCommandListener(this);
        ci.setOwner(this);

        mciVector.addElement(ci);

        if (foldersOn) {
            /* check if suiteInfo corresponds to current folder */
            if (currentFolderId == suiteInfo.folderId) {
                append(ci);
            }
        }  else {
            append(ci);
        }
    }

    /**
     * Removes a midlet from the App Selector Screen
     *
     * @param suiteInfo the midlet suite info of a recently removed MIDlet
     */
    private void remove(RunningMIDletSuiteInfo suiteInfo) {
        if (suiteInfo == null) {
            // Invalid parameter, should not happen.
            return;
        }

        // the last item in AppSelector is time
        for (int i = 0; i < mciVector.size(); i++) {
            MidletCustomItem mci = (MidletCustomItem)mciVector.elementAt(i);
            if (mci.msi == suiteInfo) {
                PAPICleanUp.removeMissedTransaction(suiteInfo.suiteId);

                if (mci.msi.proxy != null) {
                    mci.msi.proxy.destroyMidlet();
                }

                try {
                    midletSuiteStorage.remove(suiteInfo.suiteId);
                } catch (Throwable t) {
                    if (t instanceof MIDletSuiteLockedException) {
                        String[] val = new String[1];
                        val[0] = suiteInfo.displayName;
                        displayError.showErrorAlert(suiteInfo.displayName,
                            null,
                            Resource.getString(ResourceConstants.ERROR),
                            Resource.getString(
                                ResourceConstants.AMS_MGR_REMOVE_LOCKED_SUITE,
                                    val),
                            this);
                    } else {
                        displayError.showErrorAlert(suiteInfo.displayName,
                            t,
                            Resource.getString(ResourceConstants.ERROR),
                            null, this);
                    }

                    return;
                }


                try {
                    PushRegistryInternal.unregisterConnections(
                        suiteInfo.suiteId);
                } catch (Throwable t) {
                    // Intentionally ignored: suite has been removed already,
                    // we can't do anything meaningful at this point.
                }


                mciVector.removeElementAt(i);
                if (foldersOn) {
                    if (mci.msi.folderId == currentFolderId) {
                        for (int j = 0; j < size(); j++) {
                            if (get(j).equals(mci)) {
                                delete(j);
                                break;
                            }
                        }
                    }
                } else {
                    delete(i);
                }
                
                removeMsi = null;
                break;
            }
        }
        

        display.setCurrent(this);
    }

    /**
     * Alert the user that an action was successful.
     * @param successMessage message to display to user
     */
    private void displaySuccessMessage(String successMessage) {
        Image icon;
        Alert successAlert;

        icon = GraphicalInstaller.getImageFromInternalStorage("_dukeok8");

        successAlert = new Alert(null, successMessage, icon, null);

        successAlert.setTimeout(GraphicalInstaller.ALERT_TIMEOUT);

        // We need to prevent "flashing" on fast development platforms.
        while (System.currentTimeMillis() - lastDisplayChange <
               GraphicalInstaller.ALERT_TIMEOUT);

        display.setCurrent(successAlert, this);
        lastDisplayChange = System.currentTimeMillis();
    }

    /**
     * Confirm the removal of a suite.
     *
     * @param suiteInfo information for suite to remove
     */
    private void confirmRemove(RunningMIDletSuiteInfo suiteInfo) {
        Form confirmForm;
        StringBuffer temp = new StringBuffer(40);
        Item item;
        String extraConfirmMsg;
        String[] values = new String[1];
        MIDletSuiteImpl midletSuite = null;

        try {
            midletSuite = midletSuiteStorage.getMIDletSuite(suiteInfo.suiteId,
                                                            false);
            confirmForm = new Form(null);

            confirmForm.setTitle(Resource.getString
                                 (ResourceConstants.AMS_CONFIRMATION));

            if (suiteInfo.hasSingleMidlet()) {
                values[0] = suiteInfo.displayName;
            } else {
                values[0] =
                    midletSuite.getProperty(MIDletSuiteImpl.SUITE_NAME_PROP);
            }

            item = new StringItem(null, Resource.getString(
                       ResourceConstants.AMS_MGR_REMOVE_QUE,
                       values));
            item.setLayout(Item.LAYOUT_NEWLINE_AFTER | Item.LAYOUT_2);
            confirmForm.append(item);

            extraConfirmMsg =
                PAPICleanUp.checkMissedTransactions(midletSuite.getID());
            if (extraConfirmMsg != null) {
                temp.setLength(0);
                temp.append(" \n");
                temp.append(extraConfirmMsg);
                item = new StringItem(null, temp.toString());
                item.setLayout(Item.LAYOUT_NEWLINE_AFTER | Item.LAYOUT_2);
                confirmForm.append(item);
            }

            extraConfirmMsg = midletSuite.getProperty("MIDlet-Delete-Confirm");
            if (extraConfirmMsg != null) {
                temp.setLength(0);
                temp.append(" \n");
                temp.append(extraConfirmMsg);
                item = new StringItem(null, temp.toString());
                item.setLayout(Item.LAYOUT_NEWLINE_AFTER | Item.LAYOUT_2);
                confirmForm.append(item);
            }

            if (!suiteInfo.hasSingleMidlet()) {
                temp.setLength(0);
                temp.append(Resource.getString
                            (ResourceConstants.AMS_MGR_SUITE_CONTAINS));
                temp.append(": ");
                item = new StringItem(temp.toString(), "");
                item.setLayout(Item.LAYOUT_NEWLINE_AFTER | Item.LAYOUT_2);
                confirmForm.append(item);
                appendMIDletsToForm(midletSuite, confirmForm);
            }

            String[] recordStores =
                midletSuiteStorage.listRecordStores(suiteInfo.suiteId);
            if (recordStores != null) {
                temp.setLength(0);
                temp.append(Resource.getString
                            (ResourceConstants.AMS_MGR_SUITE_RECORD_STORES));
                temp.append(": ");
                item = new StringItem(temp.toString(), "");
                item.setLayout(Item.LAYOUT_NEWLINE_AFTER | Item.LAYOUT_2);
                confirmForm.append(item);
                appendRecordStoresToForm(recordStores, confirmForm);
            }

            temp.setLength(0);
            temp.append(" \n");
            temp.append(Resource.getString
                        (ResourceConstants.AMS_MGR_REM_REINSTALL, values));
            item = new StringItem("", temp.toString());
            confirmForm.append(item);
        } catch (Throwable t) {
            displayError.showErrorAlert(suiteInfo.displayName, t,
                                   Resource.getString
                                   (ResourceConstants.AMS_CANT_ACCESS),
                                   null);
            return;
        } finally {
            if (midletSuite != null) {
                midletSuite.close();
            }
        }

        confirmForm.addCommand(cancelCmd);
        confirmForm.addCommand(removeOkCmd);
        confirmForm.setCommandListener(this);
        removeMsi = suiteInfo;
        display.setCurrent(confirmForm);
    }

    /**
     * Appends a names of all the MIDlets in a suite to a Form, one per line.
     *
     * @param midletSuite information of a suite of MIDlets
     * @param form form to append to
     */
    private void appendMIDletsToForm(MIDletSuiteImpl midletSuite, Form form) {
        int numberOfMidlets;
        MIDletInfo midletInfo;
        StringItem item;

        numberOfMidlets = midletSuite.getNumberOfMIDlets();
        for (int i = 1; i <= numberOfMidlets; i++) {
            midletInfo = new MIDletInfo(
                             midletSuite.getProperty("MIDlet-" + i));

            item = new StringItem(null, midletInfo.name);
            item.setLayout(Item.LAYOUT_NEWLINE_AFTER | Item.LAYOUT_2);
            form.append(item);
        }
    }

    /**
     * Appends names of the record stores owned by the midlet suite
     * to a Form, one per line.
     *
     * @param recordStores list of the record store names
     * @param form form to append to
     */
    private void appendRecordStoresToForm(String[] recordStores, Form form) {
        StringItem item;

        for (int i = 0; i < recordStores.length; i++) {
            item = new StringItem(null, recordStores[i]);
            item.setLayout(Item.LAYOUT_NEWLINE_AFTER | Item.LAYOUT_2);
            form.append(item);
        }
    }

    /**
     * Open the settings database and retreive an id of the midlet suite
     * that was installed last.
     *
     * @return ID of the midlet suite that was installed last or
     * MIDletSuite.UNUSED_SUITE_ID.
     */
    private int getLastInstalledMIDlet() {
        ByteArrayInputStream bas;
        DataInputStream dis;
        byte[] data;
        RecordStore settings = null;
        int ret = MIDletSuite.UNUSED_SUITE_ID;

        try {
            settings = RecordStore.
                       openRecordStore(GraphicalInstaller.SETTINGS_STORE,
                                       false);

            /** we should be guaranteed that this is always the case! */
            if (settings.getNumRecords() > 0) {

                data = settings.getRecord(
                           GraphicalInstaller.SELECTED_MIDLET_RECORD_ID);

                if (data != null) {
                    bas = new ByteArrayInputStream(data);
                    dis = new DataInputStream(bas);
                    ret = dis.readInt();
                }
            }

        } catch (RecordStoreException e) {
            // ignore
        } catch (IOException e) {
            // ignore
        } finally {
            if (settings != null) {
                try {
                    settings.closeRecordStore();
                } catch (RecordStoreException e) {
                    // ignore
                }
            }
        }

        return ret;
    }

    /**
     * Finds a MidletCustomItem corresponding to the last installed
     * midlet suite.
     * @return the midlet custom item if it was found, null otherwise
     */
    private MidletCustomItem getLastInstalledMidletItem() {
        int installedMidlet = getLastInstalledMIDlet();

        if (installedMidlet != MIDletSuite.UNUSED_SUITE_ID &&
                installedMidlet != MIDletSuite.INTERNAL_SUITE_ID) {
            for (int i = 0; i < mciVector.size(); i++) {
                MidletCustomItem ci = (MidletCustomItem)mciVector.elementAt(i);
                if (ci.msi.suiteId == installedMidlet) {
                    return ci;
                }
            }
        }

        return null;
    }

    /**
     * Displayas AppManagerUI with a recently installed midlet hilighted.
     * @return true if display.setCurrentItem() was called,
     *              false - otherwise
     */
    private boolean displayLastInstalledMidlet() {
        MidletCustomItem ci = getLastInstalledMidletItem();

        if (ci != null) {
            display.setCurrentItem(ci);
            return true;
        }

        return false;
    }

    /**
     * Launches the midlet suite described by the given MIDletSuiteInfo.
     * @param msi a structure with information about the midlet suite
     * that must be launched
     */
    private void launchMidlet(RunningMIDletSuiteInfo msi) {
        if (msi.hasSingleMidlet()) {
            manager.launchSuite(msi, msi.midletToRun);
            display.setCurrent(this);
        } else {
            try {
                new MIDletSelector(msi, display, this, manager);
            } catch (Throwable t) {
                displayError.showErrorAlert(msi.displayName, t,
                                            null, null);
            }
        }
    }

    /**
     * Prompts the user to specify whether to launch a midlet from
     * the midlet suite that was just installed.
     */
    private void askUserIfLaunchMidlet() {
        // Ask the user if he wants to run a midlet from
        // the newly installed midlet suite
        String title = Resource.getString(
            ResourceConstants.AMS_MGR_RUN_THE_NEW_SUITE_TITLE, null);
        String msg = Resource.getString(
            ResourceConstants.AMS_MGR_RUN_THE_NEW_SUITE, null);

        Alert alert = new Alert(title, msg, null, AlertType.CONFIRMATION);
        alert.addCommand(runNoCmd);
        alert.addCommand(runYesCmd);
        alert.setCommandListener(this);
        alert.setTimeout(Alert.FOREVER);

        display.setCurrent(alert);
    }

    /**
     * Checks if the installer is currently running.
     *
     * @return true if the installer or discovery application is running,
     *         false otherwise
     */
    private boolean isInstallerRunning() {
        MidletCustomItem ci;
        RunningMIDletSuiteInfo msi;

        for (int i = 0; i < mciVector.size(); i++) {
            ci = (MidletCustomItem)mciVector.elementAt(i);
            msi = ci.msi;
            if (msi.suiteId == MIDletSuite.INTERNAL_SUITE_ID &&
                msi.proxy != null && (DISCOVERY_APP.equals(msi.midletToRun) ||
                                      INSTALLER.equals(msi.midletToRun))) {
                return true;
            }
        }

        return false;
    }

    /**
     * Called by Manager when destroyApp happens to clean up data.
     * Timer that shedules scrolling text repainting should be
     * canceled when AMS MIDlet is about to be destroyed to avoid
     * generation of repaint events.
     */
    void cleanUp() {
        textScrollTimer.cancel();
    }

    /** A Timer which will handle firing repaints of the ScrollPainter */
    protected static Timer textScrollTimer;

    /** Text auto-scrolling parameters */
    private static int SCROLL_RATE = 250;

    private static int SCROLL_DELAY = 500;

    private static int SCROLL_SPEED = 10;

    /**
     * Inner class used to display a running midlet in the AppSelector.
     * MidletCustomItem consists of an icon and name associated with the
     * corresponding midlet. In addition if a midlet requests to be
     * put into foreground (requires user attention) an additional
     * system provided icon will be displayed.
     */
    class MidletCustomItem extends CustomItem {

        /**
         * Constructs a midlet representation for the App Selector Screen.
         * @param msi The MIDletSuiteInfo for which representation has
         *            to be created
         */
        MidletCustomItem(RunningMIDletSuiteInfo msi) {
            super(null);
            this.msi = msi;
            icon = msi.icon;
            text = msi.displayName.toCharArray();
            textLen = msi.displayName.length();
            truncWidth = ICON_FONT.charWidth(truncationMark);
            truncated = false;
            if (textScrollTimer == null) {
                textScrollTimer = new Timer();
            }
            xScrollOffset = 0;

        }

        /**
         * Gets the minimum width of a midlet representation in
         * the App Selector Screen.
         * @return the minimum width of a midlet representation
         *         in the App Selector Screen.
         */
        protected int getMinContentWidth() {
            return AppManagerUI.this.getWidth();
        }

        /**
         * Gets the minimum height of a midlet representation in
         * the App Selector Screen.
         * @return the minimum height of a midlet representation
         *         in the App Selector Screen.
         */
        protected int getMinContentHeight() {
            return ICON_BG.getHeight() > ICON_FONT.getHeight() ?
                ICON_BG.getHeight() : ICON_FONT.getHeight();
        }

        /**
         * Gets the preferred width of a midlet representation in
         * the App Selector Screen based on the passed in height.
         * @param height the amount of height available for this Item
         * @return the minimum width of a midlet representation
         *         in the App Selector Screen.
         */
        protected int getPrefContentWidth(int height) {
            return AppManagerUI.this.getWidth();
        }

        /**
         * Gets the preferred height of a midlet representation in
         * the App Selector Screen based on the passed in width.
         * @param width the amount of width available for this Item
         * @return the minimum height of a midlet representation
         *         in the App Selector Screen.
         */
        protected int getPrefContentHeight(int width) {
            return ICON_BG.getHeight() > ICON_FONT.getHeight() ?
                ICON_BG.getHeight() : ICON_FONT.getHeight();
        }

        /**
         * On size change event we define the item's text
         * according to item's new width
         * @param w The current width of this Item
         * @param h The current height of this Item
         */
        protected void sizeChanged(int w, int h) {
            stopScroll();
            width = w;
            height = h;
            int widthForText = w - ITEM_PAD - ICON_BG.getWidth();
            int msiNameWidth = ICON_FONT.charsWidth(text, 0, textLen);
            scrollWidth = msiNameWidth - widthForText + w/5;
            truncated = msiNameWidth > widthForText;
        }

        /**
         * Paints the content of a midlet representation in
         * the App Selector Screen.
         * Note that icon representing that foreground was requested
         * is painted on to of the existing ickon.
         * @param g The graphics context where painting should be done
         * @param w The width available to this Item
         * @param h The height available to this Item
         */
        protected void paint(Graphics g, int w, int h) {
            int cX = g.getClipX();
            int cY = g.getClipY();
            int cW = g.getClipWidth();
            int cH = g.getClipHeight();

            if ((cW + cX) > bgIconW) {
                if (text != null && h > ICON_FONT.getHeight()) {

                    int color;
                    if (msi.proxy == null) {
                        color = hasFocus ? ICON_HL_TEXT : ICON_TEXT;
                    } else {
                        color = hasFocus ?
                                ICON_RUNNING_HL_TEXT : ICON_RUNNING_TEXT;
                    }

                    g.setColor(color);
                    g.setFont(ICON_FONT);


                    boolean truncate = (xScrollOffset == 0) && truncated;

                    g.clipRect(bgIconW + ITEM_PAD, 0,
                        truncate ? w - truncWidth - bgIconW - 2 * ITEM_PAD :
                                   w - bgIconW - 2 * ITEM_PAD, h);
                    g.drawChars(text, 0, textLen,
                        bgIconW + ITEM_PAD + xScrollOffset, (h - ICON_FONT.getHeight())/2,
                            Graphics.LEFT | Graphics.TOP);
                    g.setClip(cX, cY, cW, cH);

                    if (truncate) {
                        g.drawChar(truncationMark, w - truncWidth,
                            (h - ICON_FONT.getHeight())/2, Graphics.LEFT | Graphics.TOP);
                    }

                }
            }

            if (cX < bgIconW) {
                if (hasFocus) {
                    g.drawImage(ICON_BG, 0, (h - bgIconH)/2,
                                Graphics.TOP | Graphics.LEFT);
                }

                if (icon != null) {
                    g.drawImage(icon, (bgIconW - icon.getWidth())/2,
                                (bgIconH - icon.getHeight())/2,
                                Graphics.TOP | Graphics.LEFT);
                }

                // Draw special icon if user attention is requested and
                // that midlet needs to be brought into foreground by the user
                if (msi.proxy != null && msi.proxy.isAlertWaiting()) {
                    g.drawImage(FG_REQUESTED,
                                bgIconW - FG_REQUESTED.getWidth(), 0,
                                Graphics.TOP | Graphics.LEFT);
                }

                if (!msi.enabled) {
                    // indicate that this suite is disabled
                    g.drawImage(DISABLED_IMAGE,
                                (bgIconW - DISABLED_IMAGE.getWidth())/2,
                                (bgIconH - DISABLED_IMAGE.getHeight())/2,
                                Graphics.TOP | Graphics.LEFT);
                }
            }

        }

        /**
        * Start the scrolling of the text
        */
        protected void startScroll() {
            if (!hasFocus || !truncated) {
                return;
            }
            stopScroll();
            textScrollPainter = new TextScrollPainter();
            textScrollTimer.schedule(textScrollPainter, SCROLL_DELAY, SCROLL_RATE);
        }

        /**
        * Stop the scrolling of the text
        */
        protected void stopScroll() {
            if (textScrollPainter == null) {
                return;
            }
            xScrollOffset = 0;
            textScrollPainter.cancel();
            textScrollPainter = null;
            repaint(bgIconW, 0, width, height);
        }

        /**
        * Called repeatedly to animate a side-scroll effect for text
        */
        protected void repaintScrollText() {
            if (-xScrollOffset < scrollWidth) {
                xScrollOffset -= SCROLL_SPEED;
                repaint(bgIconW, 0, width, height);
            } else {
                // already scrolled to the end of text
                stopScroll();
            }
        }

        /**
         * Handles traversal.
         * @param dir The direction of traversal (Canvas.UP, Canvas.DOWN,
         *            Canvas.LEFT, Canvas.RIGHT)
         * @param viewportWidth The width of the viewport in the AppSelector
         * @param viewportHeight The height of the viewport in the AppSelector
         * @param visRect_inout The return array that tells AppSelector
         *        which portion of the MidletCustomItem has to be made visible
         * @return true if traversal was handled in this method
         *         (this MidletCustomItem just got focus or there was an
         *         internal traversal), otherwise false - to transfer focus
         *         to the next item
         */
        protected boolean traverse(int dir,
                                   int viewportWidth, int viewportHeight,
                                   int visRect_inout[]) {
            // entirely visible and hasFocus
            if (!hasFocus) {
                hasFocus = true;
                lastSelectedMsi = this.msi;
            }

            visRect_inout[0] = 0;
            visRect_inout[1] = 0;
            visRect_inout[2] = width;
            visRect_inout[3] = height;

            startScroll();

            return false;
        }

        /**
         * Handles traversal out. This method is called when this
         * MidletCustomItem looses focus.
         */
        protected void traverseOut() {
            hasFocus = false;
            stopScroll();
        }

        /**
         * Repaints MidletCustomItem. Called when internal state changes.
         */
        public void update() {
            repaint();
        }

        /**
         * Sets the owner (AppManagerUI) of this MidletCustomItem
         * @param hs The AppSelector in which this MidletCustomItem is shown
         */
        void setOwner(AppManagerUI hs) {
            owner = hs;
        }

        /**
         * Sets default <code>Command</code> for this <code>Item</code>.
         *
         * @param c the command to be used as this <code>Item's</code> default
         * <code>Command</code>, or <code>null</code> if there is to
         * be no default command
         */
        public void setDefaultCommand(Command c) {
            default_command = c;
            super.setDefaultCommand(c);
        }

        /**
         * Called when MidletCustomItem is shown.
         */
        public void showNotify() {

            // Unfortunately there is no Form.showNotify  method where
            // this could have been done.

            // When icon for the Installer
            // is shown we want to make sure
            // that there are no running midlets from the "internal" suite.
            // The only 2 midlets that can run in bg from
            // "internal" suite are the DiscoveryApp and the Installer.
            // Icon for the Installer will be shown each time
            // the AppSelector is made current since it is the top
            // most icon and we reset the traversal to start from the top
            if (msi.suiteId == MIDletSuite.INTERNAL_SUITE_ID &&
                    appManagerMidlet != null) {
                appManagerMidlet.destroyMidlet();
            }
        }

        /** A TimerTask which will repaint scrolling text  on a repeated basis */
        protected TextScrollPainter textScrollPainter;

        /**
         * Width of the scroll area for text
         */
        protected int scrollWidth;

        /**
         * If text is truncated
         */
        boolean truncated;

        /**
         * pixel offset to the start of the text field  (for example,  if
         * xScrollOffset is -60 it means means that the text in this
         * text field is scrolled 60 pixels left of the left edge of the
         * text field)
         */
        protected int xScrollOffset;

        /**
         * Helper class used to repaint scrolling text
         * if needed.
         */
        private class TextScrollPainter extends TimerTask {
            /**
             * Repaint the item text
             */
            public final void run() {
                repaintScrollText();
            }
        }

        /** True if this MidletCustomItem has focus, and false - otherwise */
        boolean hasFocus; // = false;

        /**
         * The owner of this MidletCustomItem
         * IMPL_NOTE:
         *   This field has the same name with package private
         *   Item.owner, however the field values are independent.
         */
        AppManagerUI owner; // = false

        /** The MIDletSuiteInfo associated with this MidletCustomItem */
        RunningMIDletSuiteInfo msi; // = null

        /** The width of this MidletCustomItem */
        int width; // = 0
        /** The height of this MIDletSuiteInfo */
        int height; // = 0

        /** Cashed width of the truncation mark */
        int truncWidth;

        /** The text of this MidletCustomItem */
        char[] text;

        /** Length of the text */
        int textLen;

        /**
         * The icon to be used to draw this midlet representation.
         */
        Image icon; // = null
        /** current default command */
        Command default_command; // = null
    }
}

