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

import com.sun.midp.i18n.Resource;
import com.sun.midp.log.Logging;

import javax.microedition.lcdui.*;

public class AppSettingsUIImpl extends Form
        implements AppSettingsUI, CommandListener, ItemStateListener {

    AppSettings appSettings;
    
    /** Command object for "OK" command for the form. */
    private Command saveAppSettingsCmd =
        new Command(Resource.getString(ResourceConstants.SAVE),
                    Command.OK, 1);
    /** Command object for "Cancel" command for the form. */
    private Command cancelCmd =
        new Command(Resource.getString(ResourceConstants.CANCEL),
                    Command.CANCEL, 1);

    /**
     * Create and initialize a new application settings MIDlet.
     */
    public AppSettingsUIImpl(AppSettings appSettings) throws Throwable {
        super(null);
        
        this.appSettings = appSettings;
    }

    /**
     * Respond to a command issued on any Screen.
     *
     * @param c command activated by the user
     * @param s the Displayable the command was on.
     */
    public void commandAction(Command c, Displayable s) {
        if (c == saveAppSettingsCmd) {
            appSettings.saveApplicationSettings();
        } else if (c == cancelCmd) {
            appSettings.cancelApplicationSettings();
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

        if (item == settingsPopup) {
            selected = settingsPopup.getSelectedButton();
            if (selected == lastPopupChoice) {
                return;
            }

            lastPopupChoice = selected;

            appSettings.onGroupChanged();

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
        } else {
            
        }
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

