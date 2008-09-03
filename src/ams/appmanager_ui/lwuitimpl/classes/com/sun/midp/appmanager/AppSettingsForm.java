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


import java.util.Hashtable;

import com.sun.lwuit.*;
import com.sun.lwuit.ButtonGroup;
import com.sun.lwuit.ComboBox;
import com.sun.lwuit.RadioButton;
import com.sun.lwuit.events.ActionEvent;
import com.sun.lwuit.events.ActionListener;
import com.sun.midp.i18n.Resource;
import com.sun.midp.i18n.ResourceConstants;
import com.sun.midp.installer.*;
import com.sun.midp.io.j2me.push.*;
import com.sun.midp.log.LogChannels;
import com.sun.midp.log.Logging;
import com.sun.midp.midletsuite.*;
import com.sun.midp.security.*;

/**
 * The Graphical MIDlet suite settings form.
 */
public class AppSettingsForm extends Form
		    implements ActionListener {

    /** Command object for "OK" command for the form. */
    private Command saveAppSettingsCmd =
        new Command(Resource.getString(ResourceConstants.SAVE),
                    ResourceConstants.SAVE);
    /** Command object for "Cancel" command for the form. */
    private Command cancelCmd =
        new Command(Resource.getString(ResourceConstants.CANCEL),
                    ResourceConstants.CANCEL);

    /** Permission group information. */
    private PermissionGroup[] permissionGroupsArray;
    /** Installation information of the suite. */
    private InstallInfo installInfo;
    /** Interface to suite */
    private MIDletSuiteImpl midletSuite;
    /** Display name of the suite. */
    /** MIDlet Suite storage object. */
    MIDletSuiteStorage midletSuiteStorage;

    private String suiteDisplayName;
    /** Holds the maximum levels for permissions. */
    private byte[] maxLevels;
    /** Holds the updated permissions. */
    private byte[] curLevels;
    /** Holds the updated push interrupt level. */
    private byte pushInterruptSetting;
    /** Holds the updated push options. */
    private int pushOptions;
    /** Holds the PUSH permission index */
    private int PUSH_ID;

    private byte maxGroupSetting;

    private int suiteId;
    private int numOfSettings = 0;

    private Form mainForm;
    private Label preferencesLabel;

    private ComboBox questionsGroupsBox;
    private List questionsGroupsList;

    private String[] questionsArray;
    private Label currentQuestion;
    private ButtonGroup[] optionsArray;
    private ButtonGroup currentOptions;


    public AppSettingsForm(Form mainForm, int suiteId){

	/* initialize internal fields */
	this.mainForm = mainForm;
    	this.suiteId = suiteId;

	addCommand(cancelCmd);
	initializeData();
	createWidgets();
	initializeForm();
    }

    private void initializeData() {
	/* Get number of questions */
	permissionGroupsArray = Permissions.getSettingGroups();
	midletSuiteStorage = MIDletSuiteStorage.getMIDletSuiteStorage();
	try {
	    midletSuite = midletSuiteStorage.getMIDletSuite(suiteId, false);
	} catch ( Throwable t ) {
	    t.printStackTrace();
	}

	installInfo = midletSuite.getInstallInfo();

	/* set suite display name */
	if (midletSuite.getNumberOfMIDlets() == 1) {
            String value = midletSuite.getProperty("MIDlet-1");
            MIDletInfo temp = new MIDletInfo(value);
            suiteDisplayName = temp.name;
        } else {
            suiteDisplayName = midletSuite.getProperty(
                               MIDletSuiteImpl.SUITE_NAME_PROP);
        }

	maxLevels = (Permissions.forDomain(installInfo.getSecurityDomain()))
                   [Permissions.MAX_LEVELS];
	curLevels = midletSuite.getPermissions();
	pushInterruptSetting = midletSuite.getPushInterruptSetting();
	pushOptions = midletSuite.getPushOptions();

	PUSH_ID = Permissions.getId("javax.microedition.io.PushRegistry");
	if (maxLevels[PUSH_ID] == Permissions.ALLOW) {
	    maxGroupSetting = Permissions.BLANKET;
	} else {
	    maxGroupSetting = maxLevels[PUSH_ID];
	}
	if ((pushOptions &
	    PushRegistryInternal.PUSH_OPT_WHEN_ONLY_APP) != 0) {
	    //interruptSetting = PUSH_OPTION_1_ID;
	} else {
	    //interruptSetting = pushInterruptSetting;
	}
    }


    private void createWidgets() {
	int numOfPermGroups;

	numOfPermGroups = Permissions.getSettingGroups().length;
	if (0 == numOfPermGroups) {
	    /* TODO: throw "createWidgets(): error getting permissions groups"
		exception here */
	    return;
	}
	questionsGroupsList = new List();
	questionsArray = new String[numOfPermGroups];
	optionsArray = new ButtonGroup[numOfPermGroups];

	for (int i = 0; i < numOfPermGroups; i++) {
	    initializeWidgets(i);
	}

    }

    private void initializeWidgets(int i) {
	String[] values = {suiteDisplayName};
	ButtonGroup tmpButtonGroup;
	PermissionGroup currentGroup = Permissions.getSettingGroups()[i];
	byte maxGroupSetting = Permissions.getPermissionGroupLevel(
			       maxLevels, currentGroup);
	byte currentGroupSetting = Permissions.getPermissionGroupLevel(
				   curLevels, currentGroup);

	if (currentGroup.getSettingsQuestion() == null ||
	    maxGroupSetting == Permissions.NEVER ||
	    currentGroupSetting == Permissions.ALLOW ||
	    currentGroupSetting == Permissions.NEVER) {
	    /* TODO:  put NULL to i position in the data structures */
	    return;
	}


	questionsArray[i] = Resource.getString(
			    currentGroup.getSettingsQuestion(), values);

	questionsGroupsList.addItem(currentGroup.getName());
	tmpButtonGroup = new ButtonGroup();

        switch (maxGroupSetting) {
        case Permissions.BLANKET:
	    tmpButtonGroup.add(new RadioButton(Resource.getString(
                ResourceConstants.AMS_MGR_SETTINGS_BLANKET_ANSWER)));
            // fall through, since this security level permits the
            // next response.

	case Permissions.SESSION:
	    tmpButtonGroup.add(new RadioButton(Resource.getString(
                ResourceConstants.AMS_MGR_SETTINGS_SESSION_ANSWER)));
            // fall through, since this security level permits the
            // next response.

	default:
	    tmpButtonGroup.add(new RadioButton(Resource.getString(
                ResourceConstants.AMS_MGR_SETTINGS_ONE_SHOT_ANSWER)));

	    tmpButtonGroup.add(new RadioButton(
		currentGroup.getDisableSettingChoice()));

            break;
        }

	optionsArray[i] = tmpButtonGroup;

	/* TODO:  set default button */
    }


    private void initializeForm() {
	/* add preferences label */
	preferencesLabel = new Label(Resource.getString(
				ResourceConstants.AMS_MGR_PREFERENCES));

	/* add groups name combo box */
	questionsGroupsBox = new ComboBox();
	questionsGroupsBox.setModel(questionsGroupsList.getModel());
	addComponent(questionsGroupsBox);

	/* add current question label */
	currentQuestion = new Label(questionsArray[0]);
	addComponent(currentQuestion);
	System.out.println("addComponent(currentQuestion); called " + questionsArray[0]);

	/* add radio buttons group */
	int i;
	for (i = 0; i < optionsArray.length; i++) {
	    if (optionsArray[i] != null) {
		currentOptions = optionsArray[i];
		break;
	    }
	}
	if (i == optionsArray.length) {
	    return;
	}

	for (i = 0; i < currentOptions.getButtonCount(); i++) {
	    addComponent(currentOptions.getRadioButton(i));
	    System.out.println("addComponent(currentOptions.getRadioButton(i)) called");
	}
    }

    public void actionPerformed(ActionEvent evt) {
	Command cmd = evt.getCommand();

	switch (cmd.getId()) {
	case ResourceConstants.CANCEL:
	    mainForm.show();
	    break;
	default:
	    break;
	}
    }

}
