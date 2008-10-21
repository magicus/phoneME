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

import javax.microedition.lcdui.*;

import com.sun.midp.i18n.Resource;
import com.sun.midp.i18n.ResourceConstants;

import com.sun.midp.installer.*;

import com.sun.midp.midletsuite.*;

import com.sun.midp.security.*;

import com.sun.midp.io.j2me.push.*;

import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;

import java.util.Vector;

/**
 * The Graphical MIDlet suite settings form.
 */
class AppSettingsImpl implements AppSettings {

    /** Application settings UI */
    AppSettingsUI settingsUI;

    /** ID for the interrupt choice. */
    private static final int INTERRUPT_CHOICE_ID = 2000;

    /** ID for the first push option radio button. */
    private static final int PUSH_OPTION_1_ID = 1000;

    /** The settings choice group. */
    private ValueChoiceImpl groupChoice;
    /** The application interruption setting. */
    private ValueChoiceImpl interruptChoice;
    /** The application permission settings. */
    private ValueChoiceImpl[] groupSettings;
    /** The number of group permission settings. */
    private int numberOfSettings;
    /** The initial setting. */
    private ValueChoiceImpl initialSetting;

    /** Holds the maximum levels for permissions. */
    private byte[] maxLevels;
    /** Holds the updated permissions. */
    private byte[] curLevels;
    /** Holds the PUSH permission index */
	private int PUSH_ID;
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
    AppSettingsImpl(int suiteId,
                       Display display,
                       DisplayError displayError,
                       Displayable nextScreen)
            throws MIDletSuiteLockedException,
            MIDletSuiteCorruptedException
    {
        this.displayError = displayError;
        midletSuiteStorage = MIDletSuiteStorage.getMIDletSuiteStorage();

        this.display = display;
        this.nextScreen = nextScreen;
		PUSH_ID = Permissions.getId("javax.microedition.io.PushRegistry");
        loadApplicationSettings(suiteId);
        
        settingsUI = new AppSettingsUIImpl();
        settingsUI.showAppSettings(this, Resource.getString(
            ResourceConstants.AMS_MGR_SETTINGS), display, displayError);
    }


    /**
     * Called by UI when value of particular application setting has been
     * changed by user. AppSettings validates the user input and If value
     * is not acceptable, for example due to exclusive combination selected,
     * changeSettingValue method of AppSettingsUI could be called by AppSettings
     * to change settings to appropriate values. All necessary informational
     * alerts in this case are shown to the user by AppSettings and thus
     * AppSettingsUI has just to change UI accordingly when changeSettingValue
     * is called.
     *
     * @param settingID id of setting
     * @param valueID id of selected value
     */
    public void onSettingChanged(int settingID, int valueID) {

        if (settingID == INTERRUPT_CHOICE_ID) {
            interruptChoice.setSelectedID(valueID);
        } else {
            //else ID of group is equivalent to index in array
            groupSettings[settingID].setSelectedID(valueID);
        }

    }

    /**
     * Returns ValueChoice that contains set of available application
     * setting names and IDs. Selected ID represents the initial setting
     * to be shown to the user.
     * @return value choice
     */
    public ValueChoice getSettings() {
        return groupChoice;
    }

    /**
     * Returns ValueChoice that contains set of possible value' IDs and
     * lables for specified setting. Selected ID represents value that
     * is currently active for this setting.
     * @param settingID
     * @return available setting values
     */
    public ValueChoice getSettingValues(int settingID) {
        if (settingID == INTERRUPT_CHOICE_ID) {
            return interruptChoice;
        } else {
            //else ID of group is equivalent to index in array
            return groupSettings[settingID];
        }
    }

    /**
     * Initialize the MIDlet suite info fields for a given suite.
     *
     * @param midletSuite the MIDletSuiteImpl object instance
     *
     * @exception Exception if problem occurs while getting the suite info
     */
    private void initMidletSuiteInfo(MIDletSuiteImpl midletSuite) {

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
     * Load the MIDlet suite settings as choice group infos.
     *
     * @param suiteId ID for suite
     * @throws Throwable
     */
    private void loadApplicationSettings(int suiteId)
            throws MIDletSuiteLockedException, MIDletSuiteCorruptedException 
       {

        int maxLevel;
        int interruptSetting;
        boolean loadDone = false;

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

            groupChoice = new ValueChoiceImpl(
                Resource.getString(ResourceConstants.AMS_MGR_PREFERENCES));

            if (maxLevels[PUSH_ID] == Permissions.ALLOW) {
                maxLevel = Permissions.BLANKET;
            } else {
                maxLevel = maxLevels[PUSH_ID];
            }

            if ((pushOptions &
                PushRegistryInternal.PUSH_OPT_WHEN_ONLY_APP) != 0) {
                interruptSetting = PUSH_OPTION_1_ID;
            } else {
                interruptSetting = pushInterruptSetting;
            }

            interruptChoice =
                newSettingChoice(
                    Resource.getString(ResourceConstants.AMS_MGR_INTRUPT),
                    INTERRUPT_CHOICE_ID,
                    Resource.getString(ResourceConstants.AMS_MGR_INTRUPT_QUE),
                    Resource.getString(ResourceConstants.AMS_MGR_INTRUPT_QUE_DONT),
                    maxLevel,
                    interruptSetting, suiteDisplayName,
                    ResourceConstants.AMS_MGR_SETTINGS_PUSH_OPT_ANSWER,
                    PUSH_OPTION_1_ID);

           groupSettings = new ValueChoiceImpl[groups.length];

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
            loadDone = true;
        } finally {
            if (!loadDone) {
                if (midletSuite != null) {
                    midletSuite.close();
                    midletSuite = null;
                }
            }
        }
    }

    /**
     * Creates a new choice info if it is user settable,
     * with the 3 preset choices and a initial one selected.
     *
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
     * @return choice info or null if setting cannot be modified
     */
    private ValueChoiceImpl newSettingChoice(String groupName, int groupID,
            String question, String denyAnswer, int maxLevel, int level,
            String name, int extraAnswer, int extraAnswerId) {
        String[] values = {name};
        int initValue;
        ValueChoiceImpl choice;

        if (question == null ||
            maxLevel == Permissions.ALLOW || maxLevel == Permissions.NEVER ||
            level == Permissions.ALLOW || level == Permissions.NEVER) {

            return null;
        }

        choice = new ValueChoiceImpl(Resource.getString(question, values));

        groupChoice.append(groupName, groupID);

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

            choice.append(denyAnswer,
                          Permissions.BLANKET_DENIED);
            break;
        }

        if (Logging.REPORT_LEVEL <= Logging.INFORMATION) {
            Logging.report(Logging.INFORMATION, LogChannels.LC_AMS,
                "AppSettings: " + groupName +
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

        choice.setSelectedID(initValue);

        if (initialSetting == null) {
            initialSetting = choice;
            groupChoice.setSelectedID(groupID);
        }
        return choice;
    }

    /**
     * Cancel application settings the user entered and dismiss UI.
     * Called by AppSettingsUI as response to user request.
     */
    public void cancelApplicationSettings() {
        display.setCurrent(nextScreen);
        midletSuite.close();
    }

    /**
     * Save application settings the user entered and dismiss UI.
     * Called by AppSettingsUI as a response to user request.
     *
     * IMPL_NOTE: This method has no arguments as AppSettings is
     * aware of changes user made due to onSettingChanged calls.
     *
     */
    public void saveApplicationSettings() {
        try {
            if (midletSuite == null) {
                return;
            }
            if (interruptChoice != null) {
                byte maxInterruptSetting;
                int interruptSetting = interruptChoice.getSelectedID();

                if (maxLevels[PUSH_ID] == Permissions.ALLOW) {
                    maxInterruptSetting = Permissions.BLANKET_GRANTED;
                } else {
                    maxInterruptSetting = maxLevels[PUSH_ID];
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
                        (byte)groupSettings[i].getSelectedID();

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
        } finally {
            midletSuite.close();
            midletSuite = null;
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
