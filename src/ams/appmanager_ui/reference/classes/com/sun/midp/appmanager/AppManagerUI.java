/*
 *
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

package com.sun.midp.appmanager;

import javax.microedition.lcdui.*;

import com.sun.midp.configurator.Constants;

import com.sun.midp.installer.*;
import com.sun.midp.main.*;
import com.sun.midp.midletsuite.*;

import com.sun.midp.i18n.Resource;
import com.sun.midp.i18n.ResourceConstants;

import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;

import com.sun.midp.payment.PAPICleanUp;

import java.io.*;
import javax.microedition.rms.*;

// import com.sun.midp.lcdui.Text;

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

    /** Constant for the graphical installer class name. */
    private static final String SUITE_SELECTOR =
        "com.sun.midp.midletsuite.Selector";

    /** 
     * The font used to paint midlet names in the AppSelector.
     * Inner class cannot have static variables thus it has to be here.
     */
    private static final Font ICON_FONT = Font.getFont(Font.FACE_SYSTEM, 
                                                         Font.STYLE_BOLD, 
                                                         Font.SIZE_SMALL);

    /** 
     * The image used to draw background for the midlet representation.
     */
    private static final Image ICON_BG = 
        GraphicalInstaller.getImageFromStorage("_ch_hilight_bg");

    /**
     * The icon used to display that user attention is requested
     * and that midlet needs to brought into foreground.
     */
    private static final Image FG_REQUESTED = 
        GraphicalInstaller.getImageFromStorage("_ch_fg_requested");

    /** 
     * The image used to draw disable midlet representation.
     */
    private static final Image DISABLED_IMAGE = 
        GraphicalInstaller.getImageFromStorage("_ch_disabled_large");

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

    /** Display for the Manager MIDlet. */
    ApplicationManager manager;

    /** MIDlet Suite storage object. */
    private MIDletSuiteStorage midletSuiteStorage;

    /** Display for the Manager MIDlet. */
    Display display; // = null

    /** Keeps track of when the display last changed, in milliseconds. */
    private long lastDisplayChange;

    /** MIDlet to be removed after confirmation screen was accepted */
    private MIDletSuiteInfo removeMsi;

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

    /**
     * Creates and populates the Application Selector Screen.
     * @param manager - The application manager that invoked it
     * @param displayError - The UI used to display error messages
     * @param display - The display instance associated with the manager
     * @param first - true if this is the first time AppSelector is being
     *                shown
     */
    AppManagerUI(ApplicationManager manager, Display display, 
                 DisplayError displayError, boolean first) {
        super(null);

        try {
            caManagerIncluded = Class.forName(CA_MANAGER) != null;
        } catch (ClassNotFoundException e) {
            // keep caManagerIncluded false
        }

        this.manager = manager;
        this.display = display;
        this.displayError = displayError;

        midletSuiteStorage = MIDletSuiteStorage.getMIDletSuiteStorage();

        updateContent();

        addCommand(exitCmd);
        setCommandListener(this);

        if (first) {
            display.setCurrent(new SplashScreen(display, this));

            // if a MIDlet was just installed 
            // displayInstalledMidlet() will return true and
            // make "this" visible with
            // the right MIDlet icon hilighted.
        } else if (!displayInstalledMidlet()) {
            display.setCurrent(this);
        }
    }
    

    /**
     * Respond to a command issued on any Screen.
     *
     * @param c command activated by the user
     * @param s the Displayable the command was on.
     */
    public void commandAction(Command c, Displayable s) {

        if (c == exitCmd) {

            if (s == this) {
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

        } else if (c == cancelCmd) {

            // null out removeMsi in remove confirmation screen
            removeMsi = null;

        } else if (c != backCmd) {
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
        MIDletSuiteInfo msi = ((MidletCustomItem)item).msi;
        
        if (msi == null) {
            return;
        } 
        
        if (c == launchInstallCmd) {

            manager.installSuite();
            
        } else if (c == launchCaManagerCmd) {

            manager.launchCaManager();
            
        } else if (c == launchCmd) {

            if (msi.singleMidlet) {
                manager.launchSuite(msi, msi.midletToRun);
                display.setCurrent(this);
            } else {
                try {
                    new MIDletSelector(msi, display, this, manager);
                } catch (Throwable t) {
                    t.printStackTrace();
                }
            }

        } else if (c == infoCmd) {

            try {
                AppInfo appInfo =  new AppInfo(msi.id);
                appInfo.addCommand(backCmd);
                appInfo.setCommandListener(this);
                display.setCurrent(appInfo);
            } catch (Exception ex) {
                displayError.showErrorAlert(msi.displayName, ex, null, null);
            }

        } else if (c == removeCmd) {
            
            confirmRemove(msi);
            
        } else if (c == updateCmd) {
            
            manager.updateSuite(msi);
            display.setCurrent(this);

        } else if (c == appSettingsCmd) {
            
            try {
                AppSettings appSettings = new AppSettings(msi.id, display,
                                                          displayError, this);
                display.setCurrent(appSettings);

            } catch (Exception ex) {
                displayError.showErrorAlert(msi.displayName, ex, null, null);
            }

        } else if (c == fgCmd) {
            
            manager.moveToForeground(msi);
            display.setCurrent(this);

        } else if (c == endCmd) {

            manager.exitMidlet(msi);
            display.setCurrent(this);

        }
    }
    
    /**
     * Called when a new midlet was launched.
     *
     * @param midlet proxy of a newly added MIDlet
     */    
    void notifyMidletStarted(MIDletProxy midlet) {
        if (midlet.getClassName().equals(manager.getClass().getName())) {
            return;
        }

        if (midlet.getSuiteId().equals("internal") &&
            !midlet.getClassName().equals(DISCOVERY_APP) &&
            !midlet.getClassName().equals(INSTALLER) &&
            !midlet.getClassName().equals(CA_MANAGER)) {
            appManagerMidlet = midlet;
        } else {
            MidletCustomItem ci;
            for (int i = 0; i < size(); i++) {
                ci = (MidletCustomItem)get(i);
                
                if (ci.msi.equals(midlet)) {
                    ci.removeCommand(launchCmd);
                    ci.removeCommand(launchInstallCmd);

                    if (caManagerIncluded) {
                        ci.removeCommand(launchCaManagerCmd);
                    }

                    ci.setDefaultCommand(fgCmd);
                    ci.addCommand(endCmd);
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

        for (int i = 0; i < size(); i++) {
            mci = (MidletCustomItem)get(i);
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

        if (midlet.getSuiteId().equals("internal") &&
            !midlet.getClassName().equals(DISCOVERY_APP) &&
            !midlet.getClassName().equals(INSTALLER) &&
            !midlet.getClassName().equals(CA_MANAGER)) {
            appManagerMidlet = null;
        } else {

            MidletCustomItem ci;
            
            for (int i = 0; i < size(); i++) {
                ci = (MidletCustomItem)get(i);
                
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
                    } else {
                        if (ci.msi.enabled) {
                            ci.setDefaultCommand(launchCmd);
                        }
                    }

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
                    if (INSTALLER.equals(midlet.getClassName())) {
                        updateContent();
                        /*
                         * if a MIDlet was just installed 
                         * displayInstalledMidlet() will return true and
                         * make "this" visible with
                         * the right MIDlet icon hilighted.
                         */
                        displayInstalledMidlet();
                    } else {
                        if (CA_MANAGER.equals(midlet.getClassName())) {
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
     * @param error error code
     */    
    void notifyMidletStartError(String suiteId, String className, int error) {
        Alert a;
        String errorMsg;

        switch (error) {
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

        default:
            errorMsg = Resource.getString(
                ResourceConstants.AMS_MIDLETSUITELDR_UNEXPECTEDLY_QUIT);
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
        String[] suiteIDs;
        MIDletSuiteImpl midletSuite = null;
        MIDletSuiteInfo msi = null;
        boolean newlyAdded;
        
        suiteIDs = midletSuiteStorage.getListOfSuites();

        // Add the Installer as the first installed midlet
        if (size() > 0) {
            msi = ((MidletCustomItem)get(0)).msi;
        }

        if (msi == null || msi.midletToRun == null ||
            !msi.midletToRun.equals(DISCOVERY_APP)) {

            msi = new MIDletSuiteInfo("internal", DISCOVERY_APP,
                  Resource.getString(ResourceConstants.INSTALL_APPLICATION),
                                      true) {
                    boolean equals(MIDletProxy midlet) {
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
            if (size() > 1) {
                msi = ((MidletCustomItem)get(1)).msi;
            }

            if (msi == null || msi.midletToRun == null ||
                !msi.midletToRun.equals(CA_MANAGER)) {
                msi = new MIDletSuiteInfo("internal", CA_MANAGER,
                  Resource.getString(ResourceConstants.CA_MANAGER_APP), true);
                append(msi);
            }
        }
    
        // Add the rest of the installed midlets
        for (int lowest, i = 0; i < suiteIDs.length; i++) {

            lowest = i;

            for (int k = i + 1; k < suiteIDs.length; k++) {
                if (suiteIDs[k].compareTo(suiteIDs[lowest]) < 0) {
                    lowest = k;
                }
            }

            try {
                midletSuite = 
                    midletSuiteStorage.getMIDletSuite(suiteIDs[lowest], false);

                newlyAdded = true;
                for (int k = 0; k < size(); k++) {
                    MidletCustomItem mci = (MidletCustomItem)get(k);
                    if (suiteIDs[lowest].equals(mci.msi.id)) {
                        newlyAdded = false;
                        // msi.enabled = midletSuite.isEnabled();
                        if (mci.msi.enabled != midletSuite.isEnabled()) {
                            mci.msi.enabled = midletSuite.isEnabled();
                        
                            // MIDlet suite being disabled
                            if (midletSuite.isEnabled()) {
                                mci.setDefaultCommand(launchCmd);
                                
                            } else { // MIDlet suite is being enabled
                                
                                if (mci.msi.proxy == null) { // Not running
                                    mci.removeCommand(launchCmd);
                                    
                                }
                            
                                // running MIDlets will continue to run
                                // even when disabled
                            }
                        }

                        break;
                    }
                }

                if (newlyAdded) {
                    msi = new MIDletSuiteInfo(suiteIDs[lowest],
                                              midletSuite,
                                              midletSuiteStorage);
                    msi.enabled = midletSuite.isEnabled();
                    append(msi);
                }

            } catch (Exception e) {
                // move on to the next suite
            } finally {
                if (midletSuite != null) {
                    midletSuite.close();
                }
            }

            suiteIDs[lowest] = suiteIDs[i];
        }
    }

    /**
     * Appends a MidletCustomItem to the App Selector Screen
     *
     * @param suiteInfo the midlet suite info 
     *                  of the recently started midlet
     */
    private void append(MIDletSuiteInfo suiteInfo) {
        
        MidletCustomItem ci = new MidletCustomItem(suiteInfo);
        
        if (suiteInfo.midletToRun != null &&
            suiteInfo.midletToRun.equals(DISCOVERY_APP)) {
            // setDefaultCommand will add default command first
            ci.setDefaultCommand(launchInstallCmd);
        } else if (caManagerIncluded && suiteInfo.midletToRun != null &&
            suiteInfo.midletToRun.equals(CA_MANAGER)) {
            // setDefaultCommand will add default command first
            ci.setDefaultCommand(launchCaManagerCmd);
        } else {
            ci.addCommand(infoCmd);
            ci.addCommand(removeCmd);
            ci.addCommand(updateCmd);
            ci.addCommand(appSettingsCmd);

            if (suiteInfo.enabled) {
                // setDefaultCommand will add default command first
                ci.setDefaultCommand(launchCmd);
            }
        }
        
        ci.setItemCommandListener(this);
        append(ci);
        ci.setOwner(this);
    }

    
    /**
     * Removes a midlet from the App Selector Screen
     *
     * @param suiteInfo the midlet suite info of a recently removed MIDlet
     */
    private void remove(MIDletSuiteInfo suiteInfo) {
        MIDletSuiteInfo msi;
        
        // the last item in AppSelector is time
        for (int i = 0; i < size(); i++) {
            msi = (MIDletSuiteInfo)((MidletCustomItem)get(i)).msi;
            if (msi == suiteInfo) {
                PAPICleanUp.removeMissedTransaction(suiteInfo.id);
                if (msi.proxy != null) {
                    msi.proxy.destroyMidlet();
                } else {

                    try {
                        if (suiteInfo != null) {
                            midletSuiteStorage.remove(suiteInfo.id);
                        }
                    } catch (Throwable t) {
                        if (t instanceof MIDletSuiteLockedException) {
                            displayError.showErrorAlert(suiteInfo.displayName,
                                                        null,
                                                        Resource.getString
                                                    (ResourceConstants.ERROR),
                                                        Resource.getString
                                       (ResourceConstants.AMS_CANNOT_START));
                        }
                    }

                    delete(i);
                    removeMsi = null;
                }
                return;
            }
        }
    }

    /**
     * Alert the user that an action was successful.
     * @param successMessage message to display to user
     */
    private void displaySuccessMessage(String successMessage) {
        Image icon;
        Alert successAlert;

        icon = GraphicalInstaller.getImageFromStorage("_dukeok8");

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
    private void confirmRemove(MIDletSuiteInfo suiteInfo) {
        Form confirmForm;
        StringBuffer temp = new StringBuffer(40);
        Item item;
        String extraConfirmMsg;
        String[] values = new String[1];
        MIDletSuiteImpl midletSuite = null;

        try {
            midletSuite = midletSuiteStorage.getMIDletSuite(suiteInfo.id,
                                                            false);
            confirmForm = new Form(null);

            confirmForm.setTitle(Resource.getString
                                 (ResourceConstants.AMS_CONFIRMATION));

            if (suiteInfo.singleMidlet) {
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

            if (!suiteInfo.singleMidlet) {
                temp.setLength(0);
                temp.append(Resource.getString
                            (ResourceConstants.AMS_MGR_SUITE_CONTAINS));
                temp.append(": ");
                item = new StringItem(temp.toString(), "");
                item.setLayout(Item.LAYOUT_NEWLINE_AFTER | Item.LAYOUT_2);
                confirmForm.append(item);
                appendMIDletsToForm(midletSuite, confirmForm);
            }

            temp.setLength(0);
            temp.append(" \n");
            temp.append(Resource.getString
                        (ResourceConstants.AMS_MGR_REM_REINSTALL, values));
            item = new StringItem("", temp.toString());
            confirmForm.append(item);
        } catch (Exception ex) {

            displayError.showErrorAlert(suiteInfo.displayName, ex,
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
     * Open the settings database and retreive midlet that was installed last.
     *
     * @return the storagename of the midlet that was installed last or null.
     * 
     */
    private String getLastInstalledMIDlet() {
        ByteArrayInputStream bas;
        DataInputStream dis;
        byte[] data;
        RecordStore settings = null;
        String ret = null;
        
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
                    ret = dis.readUTF();
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
     * Displayas AppManagerUI with a recently installed midlet hilighted.
     * @return true if display.setCurrentItem() was called,
     *              false - otherwise
     */
    private boolean displayInstalledMidlet() {

        String installedMidlet = getLastInstalledMIDlet();
        
        if (installedMidlet != null && 
            !installedMidlet.equals("internal")) {
            for (int i = 0; i < size(); i++) {
                MidletCustomItem ci = (MidletCustomItem)get(i);
                if (ci.msi.id.equals(installedMidlet)) {
                    display.setCurrentItem(ci);
                    return true;
                }
            }
        }

        return false;
    }

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
        MidletCustomItem(MIDletSuiteInfo msi) {
            super(null);
            this.msi = msi;
            icon = msi.icon;
        }

        /**
         * Gets the minimum width of a midlet representation in 
         * the App Selector Screen.
         * @return the minimum width of a midlet representation 
         *         in the App Selector Screen.
         */
        protected int getMinContentWidth() {
            return ICON_BG.getWidth();
        }

        /**
         * Gets the minimum height of a midlet representation in 
         * the App Selector Screen.
         * @return the minimum height of a midlet representation 
         *         in the App Selector Screen.
         */
        protected int getMinContentHeight() {
            return ICON_BG.getHeight() + ICON_FONT.getHeight();
        }

        /**
         * Gets the preferred width of a midlet representation in 
         * the App Selector Screen based on the passed in height.
         * @param height the amount of height available for this Item
         * @return the minimum width of a midlet representation 
         *         in the App Selector Screen.
         */
        protected int getPrefContentWidth(int height) {
            return ICON_BG.getWidth();
        }

        /**
         * Gets the preferred height of a midlet representation in 
         * the App Selector Screen based on the passed in width.
         * @param width the amount of width available for this Item
         * @return the minimum height of a midlet representation 
         *         in the App Selector Screen.
         */
        protected int getPrefContentHeight(int width) {
            return ICON_BG.getHeight() + ICON_FONT.getHeight();
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

            width = w;
            height = h;

            h -= ICON_FONT.getHeight();
            if (h > ICON_BG.getHeight()) {
                h = ICON_BG.getHeight();
            }

            if (hasFocus) {
                g.drawImage(ICON_BG, (width - ICON_BG.getWidth())/2, 0, 
                            Graphics.TOP | Graphics.LEFT);
            }

            // Center the icon horizontally and center it vertically
            // in the space available without the label
            if (icon != null) {
                if (icon.getHeight() < ICON_BG.getHeight()) {
                    g.drawImage(icon, (width - icon.getWidth())/2, 
                                (ICON_BG.getHeight() - icon.getHeight())/2, 
                                Graphics.TOP | Graphics.LEFT);
                } else {
                    g.clipRect(0, 0, width, h);
                    g.drawImage(icon, (width - icon.getWidth())/2, 0, 
                                Graphics.LEFT | Graphics.TOP);
                    g.setClip(0, 0, width, height);
                }
            }


            // Draw special icon if user attention is requested and
            // that midlet needs to be brought into foreground by the user
            if (msi.proxy != null && msi.proxy.isAlertWaiting()) {
                g.drawImage(FG_REQUESTED, 
                            width - FG_REQUESTED.getWidth(), 
                            (h - FG_REQUESTED.getHeight())/2,
                            Graphics.TOP | Graphics.LEFT);
            }

            if (!msi.enabled) {
                // indicate that this suite is disabled
                g.drawImage(DISABLED_IMAGE,
                    (width - DISABLED_IMAGE.getWidth())/2, 0, 
                    Graphics.TOP | Graphics.LEFT);
            }

            if (msi.displayName != null &&
                h > ICON_FONT.getHeight()) {

                h = ICON_BG.getHeight();

                int color;
                if (msi.proxy == null) {
                    color = hasFocus ? ICON_HL_TEXT : ICON_TEXT;
                } else {
                    color = hasFocus ? 
                            ICON_RUNNING_HL_TEXT : ICON_RUNNING_TEXT;
                }

                /*
                 * Text is not available in platform widget.
                 *
                // center the name if it fits
                if ((w = ICON_FONT.stringWidth(msi.displayName)) <= width) {
                    g.setColor(color);
                    g.setFont(ICON_FONT);
                    g.drawString(msi.displayName, (width - w) /2, h,
                                 Graphics.LEFT | Graphics.TOP);
                } else {
                    g.translate(0, h);
                    Text.paint(g, msi.displayName, ICON_FONT, color, color,
                               width, ICON_FONT.getHeight(), 0, 
                               Text.NORMAL | Text.TRUNCATE, null);
                    g.translate(0, -h);

                }
                */
                g.setColor(color);
                g.setFont(ICON_FONT);
                g.drawString(msi.displayName,
                             0, h, Graphics.LEFT | Graphics.TOP); 
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
            if (hasFocus) {
                // entirely visible and has focus => transfer focus
                if (visRect_inout[0] <= 0 && visRect_inout[1] <= 0 &&
                    visRect_inout[0] + viewportWidth >= width &&
                    visRect_inout[1] + viewportHeight >= height) {
                    return false;
                }

                // we assume that item is not wider or taller than viewport
                // and scrolling within the item is not an option

            } else {
                hasFocus = true;
            }

            visRect_inout[0] = 0;
            visRect_inout[1] = 0;
            visRect_inout[2] = width;
            visRect_inout[3] = height;

            return true;
        }

        /**
         * Handles traversal out. This method is called when this
         * MidletCustomItem looses focus.
         */
        protected void traverseOut() {
            hasFocus = false;
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
            if (msi.id.equals("internal") && appManagerMidlet != null) {
                appManagerMidlet.destroyMidlet();
            }
        }

        /** True if this MidletCustomItem has focus, and false - otherwise */
        boolean hasFocus; // = false;

        /** The owner of this MidletCustomItem */
        AppManagerUI owner; // = false

        /** The MIDletSuiteInfo associated with this MidletCustomItem */
        MIDletSuiteInfo msi; // = null

        /** The width of this MidletCustomItem */
        int width; // = 0
        /** The height of this MIDletSuiteInfo */
        int height; // = 0
        /** 
         * The icon to be used to draw this midlet representation.
         */
        Image icon; // = null
    }
}
