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
public class AppSettings {

    AppSettingsUI settingsUI;

    /** ID for the interrupt choice. */
    private static final int INTERRUPT_CHOICE_ID = 2000;

    /** ID for the first push option radio button. */
    private static final int PUSH_OPTION_1_ID = 1000;

    /** The settings choice group. */
    private ChoiceInfo groupChoice;
    /** The application interruption setting. */
    private ChoiceInfo interruptChoice;
    /** The application permission settings. */
    private ChoiceInfo[] groupSettings;
    /** The number of group permission settings. */
    private int numberOfSettings;
    /** The initial setting. */
    private ChoiceInfo initialSetting;

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
    public AppSettings(int suiteId,
                       Display display,
                       DisplayError displayError,
                       Displayable nextScreen) throws Throwable {
        this.displayError = displayError;
        midletSuiteStorage = MIDletSuiteStorage.getMIDletSuiteStorage();

        this.display = display;
        this.nextScreen = nextScreen;
		PUSH_ID = Permissions.getId("javax.microedition.io.PushRegistry");
        settingsUI = new AppSettingsUIImpl(this, Resource.getString(
                                        ResourceConstants.AMS_MGR_SETTINGS));
        loadApplicationSettings(suiteId);
    }


    /**
     * Called by UI when group level selected
     * @param groupID id of group
     * @param levelID id of selected level
     */
    public void onGroupLevelSelected(int groupID, int levelID) {

        if (groupID == INTERRUPT_CHOICE_ID) {
            interruptChoice.setSelected(levelID);
        } else {
            //else ID of group is equivalent to index in array
            groupSettings[groupID].setSelected(levelID);
        }

    }

    /**
     * Returns choice information containing all groups.
     * @return choice info
     */
    public ChoiceInfo getGroups() {
        return groupChoice;        
    }

    /**
     * Returns group settings for specified group.
     * @return choice info
     */
    public ChoiceInfo getGroupSettings(int groupID) {
        if (groupID == INTERRUPT_CHOICE_ID) {
            return interruptChoice;
        } else {
            //else ID of group is equivalent to index in array
            return groupSettings[groupID];
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
     * Load the MIDlet suite settings as choice group infos.
     *
     * @param suiteId ID for suite
     * @throws Throwable
     */
    private void loadApplicationSettings(int suiteId)
        throws Throwable {

        int maxLevel;
        int interruptSetting;

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

            groupChoice = new ChoiceInfo(
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

           groupSettings = new ChoiceInfo[groups.length];

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

            settingsUI.setGroups(groupChoice);

        } catch (Throwable t) {
            if (midletSuite != null) {
                midletSuite.close();
            }
            throw t;
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
    private ChoiceInfo newSettingChoice(String groupName, int groupID,
            String question, String denyAnswer, int maxLevel, int level,
            String name, int extraAnswer, int extraAnswerId) {
        String[] values = {name};
        int initValue;
        ChoiceInfo choice;

        if (question == null ||
            maxLevel == Permissions.ALLOW || maxLevel == Permissions.NEVER ||
            level == Permissions.ALLOW || level == Permissions.NEVER) {

            return null;
        }

        choice = new ChoiceInfo(Resource.getString(question, values));

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

        choice.setSelected(initValue);

        if (initialSetting == null) {
            initialSetting = choice;
            groupChoice.setSelected(groupID);
        }
        return choice;
    }

    /**
     * Cancel the application settings the user entered and dismiss UI.
     */
    public void cancelApplicationSettings() {
        display.setCurrent(nextScreen);
        midletSuite.close();
    }

    /** Save the application settings the user entered. */
    public void saveApplicationSettings() {
        try {
            if (interruptChoice != null) {
                byte maxInterruptSetting;
                int interruptSetting = interruptChoice.getSelected();

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
                        (byte)groupSettings[i].getSelected();

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
        midletSuite.close();
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

    /**
     * Returns the main displayable of the AppSettingsUI.
     * @return main screen
     */
    public Displayable getMainDisplayable() {
        return settingsUI.getMainDisplayable(); 
    }

}

class ChoiceInfo {

    /** Size increment for the ID array. */
    private static final int SIZE_INCREMENT = 5;

    /** Keeps track of the choice IDs. */
    private int[] ids;

    /** Choice title. */
    private String title;

    /** Choice lables. */
    private Vector itemLabels;

    /** Id of selected item. */
    int selectedID;

    /** Item count */
    int cnt;

    /**
     * Creates empty choice info
     * @param title of the choice
     */
    ChoiceInfo(String title) {
        cnt = 0;
        ids = new int[SIZE_INCREMENT];
        this.title = title;
        itemLabels = new Vector(5);
    }


    /**
     * Appends choice to the set.
     *
     * @param label the lable of the element to be added
     * @param id ID for the item
     *
     * @throws IllegalArgumentException if the image is mutable
     * @throws NullPointerException if <code>stringPart</code> is
     * <code>null</code>
     * @throws IndexOutOfBoundsException this call would exceed the maximum
     *         number of buttons for this set
     */
    public void append(String label, int id) {
        if (cnt >= ids.length) {
            expandIdArray();
        }
        ids[cnt] = id;
        cnt++;
        itemLabels.add(label);
    }

    /**
     * Set the selected item.
     *
     * @param id ID of selected item
     *
     * @throws IndexOutOfBoundsException if <code>id</code> is invalid
     */
    public void setSelected(int id) {
        selectedID = id;
    }

    /**
     * Returns the ID of the selected radio button.
     *
     * @return ID of selected element
     */
    public int getSelected() {
        return selectedID;
    }

    /**
     * Returns ID of specified item.
     * @param index item index
     * @return item ID
     */
    public int getID(int index) {
        return ids[index];
    }

    /**
     * Returns count of items.
     * @return item count
     */
    public int getCount() {
        return cnt;
    }

    /**
     * Returns label of cpecified choice items.
     * @param index item index
     * @return label
     */
    public String getLabel(int index) {
        return (String)itemLabels.elementAt(index);
    }

    /**
     * Returns choice title.
     * @return title
     */
    public String getTitle() {
        return title;        
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
