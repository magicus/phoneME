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

import com.sun.midp.i18n.Resource;
import com.sun.midp.i18n.ResourceConstants;

import com.sun.midp.installer.*;

import com.sun.midp.midletsuite.*;

import com.sun.midp.security.*;

import com.sun.midp.io.j2me.push.*;

import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;

/**
 * The Graphical MIDlet suite settings form.
 */
public class AppSettings extends Form
    implements CommandListener, ItemStateListener {

    /** ID for the interrupt choice. */
    private static final int INTERRUPT_CHOICE_ID = 2000;

    /** ID for the first push option radio button. */
    private static final int PUSH_OPTION_1_ID = 1000;

    /** Command object for "OK" command for the form. */
    private Command saveAppSettingsCmd =
        new Command(Resource.getString(ResourceConstants.SAVE),
                    Command.OK, 1);
    /** Command object for "Cancel" command for the form. */
    private Command cancelCmd =
        new Command(Resource.getString(ResourceConstants.CANCEL),
                    Command.CANCEL, 1);

    /** The ID of the setting displayed in the form. */
    private int displayedSettingID;
    /** The ID of the popup button selected. */
    private int lastPopupChoice;
    /** The initial setting to display. */
    private RadioButtonSet initialSetting;
    /** The settings popup choice group. */
    private RadioButtonSet settingsPopup;
    /** The application interruption setting. */
    private RadioButtonSet interruptChoice;
    /** The application permission settings. */
    private RadioButtonSet[] groupSettings;
    /** The number of group permission settings. */
    private int numberOfSettings;

    /** Holds the maximum levels for permissions. */
    private byte[] maxLevels;
    /** Holds the updated permissions. */
    private byte[] curLevels;
    /** Holds the updated push interrupt level. */
    private byte pushInterruptSetting;
    /** Holds the updated push options. */
    private int pushOptions;

    /** Permission group information. */
    private PermissionGroup[] groups;

    /** MIDlet Suite storage object. */
    MIDletSuiteStorage midletSuiteStorage;

    /** UI to display error alerts. */
    DisplayError displayError;

    /** The displayable to be displayed after dismissing AppSettings. */
    Displayable nextScreen;

    /** Display for the Manager midlet. */
    Display display;

    /** Display name of the suite. */
    String suiteDisplayName;

    /** Interface to suite */
    MIDletSuiteImpl midletSuite;

    /** Installation information of the suite. */
    InstallInfo installInfo;

    /** Icon to display for the suite */
    Image icon;

    /**
     * Create and initialize a new application settings MIDlet.
     * @param suiteId - the id of the suite for
     *                  which the App settings  should be displayed
     * @param display - the display instance to be used
     * @param displayError - the UI to be used to display error alerts.
     * @param nextScreen - the displayable to be shown after
     *                     this Form is dismissed
     */
    public AppSettings(int suiteId,
                       Display display,
                       DisplayError displayError,
                       Displayable nextScreen) throws Throwable {
        super(null);

        this.displayError = displayError;
        midletSuiteStorage = MIDletSuiteStorage.getMIDletSuiteStorage();

        this.display = display;
        this.nextScreen = nextScreen;

        displayApplicationSettings(suiteId);
    }

    /**
     * Respond to a command issued on any Screen.
     *
     * @param c command activated by the user
     * @param s the Displayable the command was on.
     */
    public void commandAction(Command c, Displayable s) {
        if (c == saveAppSettingsCmd) {
            saveApplicationSettings();
            midletSuite.close();
        } else if (c == cancelCmd) {
            display.setCurrent(nextScreen);
            midletSuite.close();
        }
    }

    /**
     * Called when internal state of an item in Settings form is
     * changed by the user. This is used to dynamically display
     * the setting the user chooses from the settings popup.
     *
     * @param item the item that was changed
     */
    public void itemStateChanged(Item item) {
        int selected;

        if (item != settingsPopup) {
            /* ignore the other items besides the popup */
            return;
        }

        selected = settingsPopup.getSelectedButton();
        if (selected == lastPopupChoice) {
            return;
        }

        lastPopupChoice = selected;

        delete(displayedSettingID);

        try {
            if (selected == INTERRUPT_CHOICE_ID) {
                displayedSettingID = append(interruptChoice);
            } else {
                displayedSettingID = append(groupSettings[selected]);
            }
        } catch (IndexOutOfBoundsException e) {
            // for safety/completeness.
            displayedSettingID = 0;
            Logging.report(Logging.ERROR, LogChannels.LC_AMS,
                "AppSettings: selected=" + selected);
        }
    }


    /**
     * Initialize the MIDlet suite info fields for a given suite.
     *
     * @param midletSuite the MIDletSuiteImpl object instance
     *
     * @exception Exception if problem occurs while getting the suite info
     */
    private void initMidletSuiteInfo(MIDletSuiteImpl midletSuite)
        throws Exception {

        int numberOfMidlets = midletSuite.getNumberOfMIDlets();
        installInfo = midletSuite.getInstallInfo();

        if (numberOfMidlets == 1) {
            String value = midletSuite.getProperty("MIDlet-1");
            MIDletInfo temp = new MIDletInfo(value);
            suiteDisplayName = temp.name;
        } else {
            suiteDisplayName = midletSuite.getProperty(
                               MIDletSuiteImpl.SUITE_NAME_PROP);
        }
    }

    /**
     * Display the MIDlet suite settings as choice groups.
     *
     * @param suiteId ID for suite to display
     */
    private void displayApplicationSettings(int suiteId)
        throws Throwable {

        int maxLevel;
        String[] values = new String[1];
        int interruptSetting;
        initialSetting = null;

        try {
            groups = Permissions.getSettingGroups();

            midletSuite = midletSuiteStorage.getMIDletSuite(suiteId, false);
            initMidletSuiteInfo(midletSuite);

            maxLevels =
                (Permissions.forDomain(installInfo.getSecurityDomain()))
                   [Permissions.MAX_LEVELS];
            curLevels = midletSuite.getPermissions();
            pushInterruptSetting = midletSuite.getPushInterruptSetting();
            pushOptions = midletSuite.getPushOptions();

            values[0] = suiteDisplayName;

            setTitle(Resource.getString(
                                        ResourceConstants.AMS_MGR_SETTINGS));
            settingsPopup = new RadioButtonSet(
                Resource.getString(ResourceConstants.AMS_MGR_PREFERENCES),
                    true);

            if (maxLevels[Permissions.PUSH] == Permissions.ALLOW) {
                maxLevel = Permissions.BLANKET;
            } else {
                maxLevel = maxLevels[Permissions.PUSH];
            }

            if ((pushOptions &
                PushRegistryInternal.PUSH_OPT_WHEN_ONLY_APP) != 0) {
                interruptSetting = PUSH_OPTION_1_ID;
            } else {
                interruptSetting = pushInterruptSetting;
            }

            interruptChoice =
                newSettingChoice(settingsPopup,
                    ResourceConstants.AMS_MGR_INTRUPT,
                    INTERRUPT_CHOICE_ID,
                    ResourceConstants.AMS_MGR_INTRUPT_QUE,
                    ResourceConstants.AMS_MGR_INTRUPT_QUE_DONT,
                    maxLevel,
                    interruptSetting, suiteDisplayName,
                    ResourceConstants.AMS_MGR_SETTINGS_PUSH_OPT_ANSWER,
                    PUSH_OPTION_1_ID);

           groupSettings = new RadioButtonSet[groups.length];

            if (interruptChoice != null) {
                numberOfSettings = 1;
            } else {
                numberOfSettings = 0;
            }

            for (int i = 0; i < groups.length; i++) {
                byte maxGroupSetting = Permissions.getPermissionGroupLevel(
                                       maxLevels, groups[i]);
                byte currentGroupSetting = Permissions.getPermissionGroupLevel(
                                           curLevels, groups[i]);

                groupSettings[i] = newSettingChoice(
                    settingsPopup,
                    groups[i].getName(),
                    i,
                    groups[i].getSettingsQuestion(),
                    groups[i].getDisableSettingChoice(),
                    maxGroupSetting,
                    currentGroupSetting,
                    suiteDisplayName,
                    0, 0);
                if (groupSettings[i] != null) {
                    numberOfSettings++;
                }
            }

            if (numberOfSettings > 1) {
                /*
                 * There is more then one setting to display so add the
                 * popup to the settings form
                 */
                append(settingsPopup);
            }

            if (initialSetting != null) {
                displayedSettingID = append(initialSetting);
            }
        } catch (Throwable t) {
            if (midletSuite != null) {
                midletSuite.close();
            }
            throw t;
        }

        addCommand(saveAppSettingsCmd);
        addCommand(cancelCmd);
        setCommandListener(this);

        setItemStateListener(this);
    }

    /**
     * Creates a new choice group in a form if it is user settable,
     * with the 3 preset choices and a initial one set.
     *
     * @param popup settings popup to append to
     * @param groupName name to add to popup
     *                i18N will be translated
     * @param groupID button ID of group in settings popup,
     * @param question label for the choice, i18N will be translated,
     *        if <= 0, then skip this choice
     * @param denyAnswer answer for the deny choice of this setting,
     *                   i18N will be translated
     * @param maxLevel maximum permission level
     * @param level current permission level
     * @param name name of suite
     * @param extraAnswer if > 0, add this extra answer before last
     *                    answer, i18N will be translated
     * @param extraAnswerId ID for the extra answer
     *
     * @return choice to put in the application settings form,
     *           or null if setting cannot be modified
     */
    private RadioButtonSet newSettingChoice(RadioButtonSet popup,
            int groupName, int groupID, int question, int denyAnswer,
            int maxLevel, int level, String name, int extraAnswer,
            int extraAnswerId) {
        String[] values = {name};
        int initValue;
        RadioButtonSet choice;

        if (question <= 0 ||
            maxLevel == Permissions.ALLOW || maxLevel == Permissions.NEVER ||
            level == Permissions.ALLOW || level == Permissions.NEVER) {

            return null;
        }

        choice = new RadioButtonSet(Resource.getString(question, values),
                                    false);

        settingsPopup.append(Resource.getString(groupName), groupID);

        switch (maxLevel) {
        case Permissions.BLANKET:
            choice.append(Resource.getString(
                ResourceConstants.AMS_MGR_SETTINGS_BLANKET_ANSWER),
                Permissions.BLANKET_GRANTED);
            // fall through, since this security level permits the
            // next response.

        case Permissions.SESSION:
            choice.append(Resource.getString(
                ResourceConstants.AMS_MGR_SETTINGS_SESSION_ANSWER),
                Permissions.SESSION);
            // fall through, since this security level permits the
            // next response.

        default:
            choice.append(Resource.getString(
                ResourceConstants.AMS_MGR_SETTINGS_ONE_SHOT_ANSWER),
                Permissions.ONESHOT);

            if (extraAnswer > 0) {
                choice.append(Resource.getString(extraAnswer), extraAnswerId);
            }

            choice.append(Resource.getString(denyAnswer),
                          Permissions.BLANKET_DENIED);
            break;
        }

        if (Logging.REPORT_LEVEL <= Logging.INFORMATION) {
            Logging.report(Logging.INFORMATION, LogChannels.LC_AMS,
                "AppSettings: " + Resource.getString(groupName) +
                " level = " + level);
        }

        switch (level) {
        case Permissions.BLANKET_GRANTED:
        case Permissions.BLANKET:
            initValue = Permissions.BLANKET_GRANTED;
            break;

        case Permissions.SESSION:
            initValue = Permissions.SESSION;
            break;

        case Permissions.ONESHOT:
            initValue = Permissions.ONESHOT;
            break;

        default:
            if (level == extraAnswerId) {
                initValue = extraAnswerId;
            } else {
                initValue = Permissions.BLANKET_DENIED;
            }
            break;
        }

        choice.setDefaultButton(initValue);

        choice.setPreferredSize(getWidth(), -1);

        if (initialSetting == null) {
            initialSetting = choice;
            lastPopupChoice = groupID;
        }

        return choice;
    }

    /** Save the application settings the user entered. */
    private void saveApplicationSettings() {
        try {
            if (interruptChoice != null) {
                byte maxInterruptSetting;
                int interruptSetting = interruptChoice.getSelectedButton();

                if (maxLevels[Permissions.PUSH] == Permissions.ALLOW) {
                    maxInterruptSetting = Permissions.BLANKET_GRANTED;
                } else {
                    maxInterruptSetting = maxLevels[Permissions.PUSH];
                }

                if (interruptSetting == PUSH_OPTION_1_ID) {
                    pushOptions = PushRegistryInternal.PUSH_OPT_WHEN_ONLY_APP;
                    pushInterruptSetting = maxInterruptSetting;
                } else {
                    pushOptions = 0;
                    Permissions.checkPushInterruptLevel(curLevels,
                        (byte)interruptSetting);
                    pushInterruptSetting = (byte)interruptSetting;
                }
            }

            for (int i = 0; i < groups.length; i++) {
                if (groupSettings[i] != null) {
                    byte newSetting =
                        (byte)groupSettings[i].getSelectedButton();

                    if (newSetting != Permissions.getPermissionGroupLevel(
                            curLevels, groups[i])) {
                        Permissions.setPermissionGroup(curLevels,
                            pushInterruptSetting, groups[i], newSetting);
                    }
                }
            }


            if (numberOfSettings > 0) {
                midletSuiteStorage.saveSuiteSettings(midletSuite.getID(),
                    pushInterruptSetting, pushOptions, curLevels);
                displaySuccessMessage(Resource.getString
                                      (ResourceConstants.AMS_MGR_SAVED));
            }
        } catch (SecurityException ex) {
            Alert a = new Alert(Resource.getString(ResourceConstants.ERROR),
                                ex.getMessage(), null,
                                AlertType.ERROR);
            a.setTimeout(Alert.FOREVER);
            display.setCurrent(a);
            throw ex;
        } catch (Throwable t) {
            t.printStackTrace();
            displayError.showErrorAlert(suiteDisplayName, t,
                                        Resource.getString
                                        (ResourceConstants.EXCEPTION), null);
        }
    }

    /**
     * Alert the user that an action was successful.
     * @param successMessage message to display to user
     */
    private void displaySuccessMessage(String successMessage) {

        Image icon = GraphicalInstaller.getImageFromInternalStorage("_dukeok8");

        Alert successAlert = new Alert(null, successMessage, icon, null);

        successAlert.setTimeout(GraphicalInstaller.ALERT_TIMEOUT);

        display.setCurrent(successAlert, nextScreen);
    }
}

/**
 * A <code>RadioButtonSet</code> is a group radio buttons intended to be
 * placed within a <code>Form</code>. However the radio buttons can be
 * accessed by a assigned ID instead of by index. This lets the calling
 * code be the same when dealing with dynamic sets.
 */
class RadioButtonSet extends ChoiceGroup {
    /** Size increment for the ID array. */
    private static final int SIZE_INCREMENT = 5;

    /** Keeps track of the button IDs. */
    private int[] ids;

    /**
     * Creates a new, empty <code>RadioButtonSet</code>, specifying its
     * title.
     *
     * @param label the item's label (see {@link Item Item})
     * @param popup true if the radio buttons should be popup
     */
    RadioButtonSet(String label, boolean popup) {
        super(label, popup ? Choice.POPUP : Choice.EXCLUSIVE);
        ids = new int[SIZE_INCREMENT];
    }

    /**
     * Appends choice to the set.
     *
     * @param stringPart the string part of the element to be added
     * @param id ID for the radio button
     *
     * @throws IllegalArgumentException if the image is mutable
     * @throws NullPointerException if <code>stringPart</code> is
     * <code>null</code>
     * @throws IndexOutOfBoundsException this call would exceed the maximum
     *         number of buttons for this set
     */
    public void append(String stringPart, int id) {
        int buttonNumber = append(stringPart, null);

        if (buttonNumber >= ids.length) {
            expandIdArray();
        }

        ids[buttonNumber] = id;
    }

    /**
     * Set the default button.
     *
     * @param id ID of default button
     *
     * @throws IndexOutOfBoundsException if <code>id</code> is invalid
     */
    public void setDefaultButton(int id) {
        setSelectedIndex(indexFor(id), true);
    }

    /**
     * Returns the ID of the selected radio button.
     *
     * @return ID of selected element
     */
    public int getSelectedButton() {
        return ids[getSelectedIndex()];
    }

    /**
     * Find the index for an ID.
     *
     * @param id button id
     *
     * @return index for a button
     *
     * @exception IndexOutOfBoundsException If no element exists with that ID
     */
    private int indexFor(int id) {
        for (int i = 0; i < ids.length; i++) {
            if (ids[i] == id) {
                return i;
            }
        }

        throw new IndexOutOfBoundsException();
    }

    /** Expands the ID array. */
    private void expandIdArray() {
        int[] prev = ids;

        ids = new int[prev.length + SIZE_INCREMENT];
        for (int i = 0; i < prev.length; i++) {
            ids[i] = prev[i];
        }
    }
}
