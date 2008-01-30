/*
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

import java.util.*;
import javax.microedition.lcdui.*;
import com.sun.midp.midletsuite.*;
import com.sun.midp.i18n.Resource;
import com.sun.midp.i18n.ResourceConstants;
import com.sun.midp.configurator.Constants;
import com.sun.midp.main.Configuration;

/**
 * The Graphical MIDlet swicher.
 *
 * Switcher provides a simple user interface to select MIDlets to 
 * bring into foreground from the list of running midlets.
 */

class MIDletSwitcher extends javax.microedition.lcdui.List 
        implements CommandListener {
     /**
     * Number of midlets in minfo.
     */
    private int mcount;
    /**
     * MIDlet information, class, name, icon; one per MIDlet.
     */
    private RunningMIDletSuiteInfo[] minfo;
    
    /** Number of reserved elements in minfo array. */
    private final int pitch = 4;
 
    /** Application Manager. */
    ApplicationManager manager;

    /** Application Manager main form. */
    AppManagerUI managerUI;

    /** Display for the Manager MIDlet. */
    Display display; // = null

    /** Command object for "Bring to foreground". */
    private Command fgCmd = new Command(Resource.getString
                                        (ResourceConstants.AMS_SWITCHER_SEL),
                                        Command.ITEM, 1);
    /**
     * Create and initialize a new MIDlet Switcher.
     *
     * @param managerUI the aplication manager main form
     * @param manager the parent application manager
     * @param display the Display
     */
    MIDletSwitcher(AppManagerUI managerUI, ApplicationManager manager,
                   Display display) {
        super("", Choice.IMPLICIT);
        this.manager = manager;
        this.managerUI = managerUI;
        this.display = display;
        mcount = 0;
        minfo = new RunningMIDletSuiteInfo[Configuration.
            getPositiveIntProperty("MAX_ISOLATES", Constants.MAX_ISOLATES)];

        setSelectCommand(fgCmd);
        setFitPolicy(TEXT_WRAP_OFF);
        setCommandListener(this); // Listen for the selection
    }

    /**
     * Append launched suite info to the list.
     *
     * @param msi RunningMIDletSuiteInfo to append
     */
    synchronized void append(RunningMIDletSuiteInfo msi) {
        checkInfoArraySize();
        minfo[mcount++] = msi;
        append(msi.displayName, msi.icon);
    }

    /**
     * Updates the existing item in the list.
     * It should be used to update the reference to the previously added object.
     *
     * @param oldMsi entry to update 
     * @param newMsi new value of the entry
     */
    synchronized void update(RunningMIDletSuiteInfo oldMsi,
                             RunningMIDletSuiteInfo newMsi) {
        for (int i = 0; i < mcount; i++) {
            if (minfo[i] == oldMsi) {
                minfo[i] = newMsi;
                break;
            }
        }
    }

    /**
     * Remove suite info from the list.
     *
     * @param msi RunningMIDletSuiteInfo to remove
     */
    synchronized void remove(RunningMIDletSuiteInfo msi) {
        int pos = -1;
        for (int i = 0; i < mcount; i++) {
            if (minfo[i] == msi) {
                pos = i;
                break;
            }
        }
        if (pos >= 0) {
            for (int i = pos+1; i < mcount; i++) {
                minfo[i-1] = minfo[i];
            }
            mcount--;
            checkInfoArraySize();
            delete(pos);
        }
    }

    /**
     * Ensures that info array has enough capacity.
     */
    private void checkInfoArraySize() {
        if ((mcount+pitch < minfo.length) || (mcount >= minfo.length)) { 
            RunningMIDletSuiteInfo[] n =
                new RunningMIDletSuiteInfo[mcount+pitch];
            System.arraycopy(minfo, 0, n, 0, mcount);
            minfo = n;
        }
    }
    
    /**
     * If switcher hase any items.
     *
     * equivalent statement - if there is any launched MIDlet
     */
    synchronized boolean hasItems() {
        return (mcount > 0);
    }

    /**
     * Respond to a command issued on any Screen.
     *
     * @param c command activated by the user
     * @param s the Displayable the command was on.
     */
    public synchronized void commandAction(Command c, Displayable s) {
        if (c == fgCmd) {
            //bring to foreground appropriate midlet
            int ind = getSelectedIndex();
            if (ind != -1) {
                manager.moveToForeground(minfo[ind]);
            }
            display.setCurrent(managerUI);
        }
    }

}
