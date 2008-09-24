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

import javax.microedition.lcdui.Displayable;

public class AppSettingsUIImpl implements AppSettingsUI {

    /**
     * Create and initialize a new application settings MIDlet.
     * @param appSettings AppSettings peer, where information
     *   regarding available setting groups and current security
     *   levels for each group could be found.
     *   Also appSettings is used to change application settings
     *   or to cancel the process and dismiss this form.
     *   Method onGroupLevelSelected of appSettings should be called
     *   when attempt to change level for particular group occures.
     *   As a result selectGroupLevel could be called by appSettings
     *   when proposed level change leads to changes in other groups
     *   or is not allowed. This may happen for example when mutual
     *   exclusive combinations selected. All necessary alerts
     *   in this case are shown to the user by AppSettings and thus
     *   AppSettingsUIImpl has just to change UI accordingly when
     *   selectGroupLevel is called. 
     * @param title
     * @throws Throwable
     */
    public AppSettingsUIImpl(AppSettings appSettings, String title)
            throws Throwable {
    }

    /**
     * Called by AppSettings to  initialize UI content.
     * Available group levels and current level for particular
     * group can be accecssed by method getGroupSettings of AppSettings.
     * @param groupsChoice
     */
    public void setGroups(ChoiceInfo groupsChoice) {
        
    }

    /**
     * Called by AppSettings to select specified group level
     * @param groupId id of group
     * @param levelId id of selected level
     */
    public void selectGroupLevel(int groupId, int levelId) {

    }

    /**
     * Returns the main displayable of the AppSettingsUI.
     * @return main screen
     */
    public Displayable getMainDisplayable() {
        return null;
    }

}
