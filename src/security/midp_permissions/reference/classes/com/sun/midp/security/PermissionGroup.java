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

package com.sun.midp.security;

/**
 * The attributes of a permission group.
 */
public final class PermissionGroup {
    /** Name string ID. */
    private int name;

    /** Settings question ID. */
    private int settingsQuestion;

    /** Disable setting choice string ID. */
    private int disableSettingChoice;

    /** Title string ID for the permission dialog. */
    private int runtimeDialogTitle;

    /** Question string ID for the permission dialog. */
    private int runtimeQuestion;

    /** Oneshot question string ID for the permission dialog. */
    private int runtimeOneshotQuestion;

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

        name = theName;
        settingsQuestion = theSettingsQuestion;
        disableSettingChoice = theDisableSettingChoice;
        runtimeDialogTitle = theRuntimeDialogTitle;
        runtimeQuestion = theRuntimeQuestion;
        identifiedMaxiumLevel = theIdentifiedMaxiumLevel;
        identifiedDefaultLevel = theIdentifiedDefaultLevel;
        unidentifiedMaxiumLevel = theUnidentifiedMaxiumLevel;
        unidentifiedDefaultLevel = theUnidentifiedDefaultLevel;
    }

    /**
     * Get the name string ID.
     *
     * @return string ID or zero if there is no name for the settings dialog
     */
    public int getName() {
        return name;
    }

    /**
     * Get the settings question ID.
     *
     * @return stringID or 0 if there is no question
     */
    public int getSettingsQuestion() {
        return settingsQuestion;
    }

    /**
     * Get the disable setting choice string ID.
     *
     * @return string ID or 0 if there is not disable setting choice
     */
    public int getDisableSettingChoice() {
        return disableSettingChoice;
    }

    /**
     * Get the title string ID for the permission dialog.
     *
     * @return string ID
     */
    public int getRuntimeDialogTitle() {
        return runtimeDialogTitle;
    }

    /**
     * Get the question string ID for the permission dialog.
     *
     * @return string ID
     */
    public int getRuntimeQuestion() {
        return runtimeQuestion;
    }

    /**
     * Get the oneshot question string ID for the permission dialog.
     *
     * @return string ID
     */
    public int getRuntimeOneshotQuestion() {
        if (runtimeOneshotQuestion == 0) {
            return runtimeQuestion;
        }

        return runtimeOneshotQuestion;
    }

    /**
     * Get the identified Third Party domain maxium permission level.
     *
     * @return permission level
     */
    public byte getIdentifiedMaxiumLevel() {
        return identifiedMaxiumLevel;
    }

    /**
     * Get the identified Third Party domain default permission level.
     *
     * @return permission level
     */
    public byte getIdentifiedDefaultLevel() {
        return identifiedDefaultLevel;
    }

    /**
     * Get the unidentified Third Party domain maxium permission level.
     *
     * @return permission level
     */
    public byte getUnidentifiedMaxiumLevel() {
        return unidentifiedMaxiumLevel;
    }

    /**
     * Get the unidentified Third Party domain default permission level.
     *
     * @return permission level
     */
    public byte getUnidentifiedDefaultLevel() {
        return unidentifiedDefaultLevel;
    }
}

