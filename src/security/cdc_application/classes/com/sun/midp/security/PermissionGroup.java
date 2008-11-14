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

package com.sun.midp.security;

import com.sun.midp.i18n.Resource;

/**
 * The attributes of a permission group.
 */
public final class PermissionGroup {
    /** Name string ID. */
    private String name;

    /** Settings question ID. */
    private String settingsQuestion;

    /** Disable setting choice string ID. */
    private String disableSettingChoice;

    /** Title string ID for the permission dialog. */
    private String runtimeDialogTitle;

    /** Question string ID for the permission dialog. */
    private String runtimeQuestion;

    /** Oneshot question string ID for the permission dialog. */
    private String runtimeOneshotQuestion;

    /** Identified Third Party domain maxium permission level. */
    private byte identifiedMaxiumLevel;

    /** Identified Third Party domain default permission level. */
    private byte identifiedDefaultLevel;

    /** Unidentified Third Party domain maxium permission level. */
    private byte unidentifiedMaxiumLevel;

    /** Unidentified Third Party domain default permission level. */
    private byte unidentifiedDefaultLevel;

    /**
     * Constructs a third party domain permission group.
     *
     * @param theName name of the group
     * @param theSettingsQuestion question for the settings dialog
     * @param theDisableSettingChoice disable setting choice
     * @param theRuntimeDialogTitle Title for the runtime permission dialog
     * @param theRuntimeQuestion Question for the runtime permission dialog
     * @param theRuntimeOneshotQuestion Oneshot question for the runtime
     *                                  permission dialog
     * @param theIdentifiedMaxiumLevel Identified Third Party domain
     *                                 maxium permission level
     * @param theIdentifiedDefaultLevel Identified Third Party domain
     *                                  default permission level
     * @param theUnidentifiedMaxiumLevel Unidentified Third Party domain
     *                                   maxium permission level
     * @param theUnidentifiedDefaultLevel Unidentified Third Party domain
     *                                    default permission level
     */
    PermissionGroup(int theName, int theSettingsQuestion,
        int theDisableSettingChoice, int theRuntimeDialogTitle,
        int theRuntimeQuestion, int theRuntimeOneshotQuestion,
        byte theIdentifiedMaxiumLevel, byte theIdentifiedDefaultLevel,
        byte theUnidentifiedMaxiumLevel, byte theUnidentifiedDefaultLevel) {

        name = Resource.getString(theName);
        settingsQuestion = Resource.getString(theSettingsQuestion);
        disableSettingChoice = Resource.getString(theDisableSettingChoice);
        runtimeDialogTitle = Resource.getString(theRuntimeDialogTitle);
        runtimeQuestion = Resource.getString(theRuntimeQuestion);
        runtimeOneshotQuestion = Resource.getString(theRuntimeOneshotQuestion); 
        identifiedMaxiumLevel = theIdentifiedMaxiumLevel;
        identifiedDefaultLevel = theIdentifiedDefaultLevel;
        unidentifiedMaxiumLevel = theUnidentifiedMaxiumLevel;
        unidentifiedDefaultLevel = theUnidentifiedDefaultLevel;
    }

    /**
     * Get the name.
     *
     * @return string ID or zero if there is no name for the settings dialog
     */
    public String getName() {
        return name;
    }

    /**
     * Get the settings question.
     *
     * @return question
     */
    public String getSettingsQuestion() {
        return settingsQuestion;
    }

    /**
     * Get the disable setting choice string.
     *
     * @return disable setting choice
     */
    public String getDisableSettingChoice() {
        return disableSettingChoice;
    }

    /**
     * Get the title string for the permission dialog.
     *
     * @return title
     */
    public String getRuntimeDialogTitle() {
        return runtimeDialogTitle;
    }

    /**
     * Get the question string for the permission dialog.
     *
     * @return question
     */
    public String getRuntimeQuestion() {
        return runtimeQuestion;
    }

    /**
     * Get the oneshot question string for the permission dialog.
     *
     * @return question
     */
    public String getRuntimeOneshotQuestion() {
        if (runtimeOneshotQuestion == null) {
            return runtimeQuestion;
        }

        return runtimeOneshotQuestion;
    }

    /**
     * Get the identified Third Party domain maxium permission level.
     *
     * @return permission level
     */
    byte getIdentifiedMaxiumLevel() {
        return identifiedMaxiumLevel;
    }

    /**
     * Get the identified Third Party domain default permission level.
     *
     * @return permission level
     */
    byte getIdentifiedDefaultLevel() {
        return identifiedDefaultLevel;
    }

    /**
     * Get the unidentified Third Party domain maxium permission level.
     *
     * @return permission level
     */
    byte getUnidentifiedMaxiumLevel() {
        return unidentifiedMaxiumLevel;
    }

    /**
     * Get the unidentified Third Party domain default permission level.
     *
     * @return permission level
     */
    byte getUnidentifiedDefaultLevel() {
        return unidentifiedDefaultLevel;
    }
}

