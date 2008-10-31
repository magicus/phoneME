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

import javax.microedition.lcdui.*;

import com.sun.midp.i18n.Resource;
import com.sun.midp.i18n.ResourceConstants;
import com.sun.midp.configurator.Constants;
import com.sun.midp.main.Configuration;
import com.sun.midp.main.MIDletProxy;

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
    private MidletListEntry[] minfo;
    
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
        minfo = new MidletListEntry[Configuration.
            getPositiveIntProperty("MAX_ISOLATES", Constants.MAX_ISOLATES)];

        setSelectCommand(fgCmd);
        setFitPolicy(TEXT_WRAP_OFF);
        setCommandListener(this); // Listen for the selection
    }

    /**
     * Append launched suite info to the list.
     *
     * @param msi RunningMIDletSuiteInfo to append
     * @param className the MIDlet class name
     */
    synchronized void append(RunningMIDletSuiteInfo msi, String className) {
        checkInfoArraySize();
        minfo[mcount++] = new MidletListEntry(msi, className);
        final MIDletProxy midletProxy = msi.getProxyFor(className);
        StringBuffer name = new StringBuffer(msi.displayName);
        if (midletProxy != null) {
            name.append('/');
            name.append(midletProxy.getDisplayName());
        }
        append(name.toString(), msi.icon);
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
        // IMPL_NOTE: our implementation stores a reference
        // to the RunningMIDletSuiteInfo object that gets modified elsewhere;
        // therefore, we do not need to copy any information from newMsi
        // to oldMsi.
        // Note also that our implementation implies that the
        // RunningMIDletSuiteInfo objects are unique, one per midlet suite,
        // and compares them using ==.

        // IMPL_NOTE: The fields that may be changed are all mentioned in
        // AppManagerPeer.updateContent().
    }

    /**
     * Remove suite info from the list.
     *
     * @param msi RunningMIDletSuiteInfo to remove
     * @param className the MIDlet class name
     */
    synchronized void remove(RunningMIDletSuiteInfo msi, String className) {
        int pos = -1;

        for (int i = 0; i < mcount; i++) {
            // IMPL_NOTE: the suiteId check will be removed as soon as we maintain all RunningMIDletSuiteInfo lists
            if ((minfo[i].suite == msi || minfo[i].suite.suiteId == msi.suiteId)
             && (minfo[i].className == null || className == null || minfo[i].className.equals(className))) {
                pos = i;
                break; // IMPL_NOTE: two instances of the same MIDlet cannot be running
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
            MidletListEntry[] n =
                new MidletListEntry[mcount+pitch];
            System.arraycopy(minfo, 0, n, 0, mcount);
            minfo = n;
        }
    }
    
    /**
     * Check if the switcher has any items, that is, if there are any running MIDlet(s).
     * @return true if MIDlet(s) are running
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
                manager.moveToForeground(minfo[ind].suite, minfo[ind].className);
            }
            display.setCurrent(managerUI.getMainDisplayable());
        }
    }

    private class MidletListEntry {
        RunningMIDletSuiteInfo suite;
        String className;

        public MidletListEntry(RunningMIDletSuiteInfo msi, String midletClassName) {
            suite = msi;
            className = midletClassName;
        }
    }
}
