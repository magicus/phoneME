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

import com.sun.midp.main.MIDletSuiteLoader;

import com.sun.midp.midlet.MIDletSuite;

import com.sun.midp.midletsuite.*;

import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;

/**
 * Selector provides a simple user interface to select MIDlets to run.
 * It extracts the list of MIDlets from the attributes in the
 * descriptor file and presents them to the user using the MIDlet-&lt;n&gt;
 * name and icon if any. When the user selects a MIDlet an instance
 * of the class indicated by MIDlet-&lt;n&gt; classname is created.
 */
final class MIDletSelector implements CommandListener {
    /**
     * The List of all the MIDlets.
     */
    private List mlist;
    /**
     * Information needed to display a list of MIDlets.
     */
    private RunningMIDletSuiteInfo suiteInfo;
    /**
     * The Display.
     */
    private Display display;
    /**
     * The parent's display able.
     */
    private Displayable parentDisplayable;
    /**
     * Parent app manager.
     */
    ApplicationManager manager;
    /**
     * Number of midlets in minfo.
     */
    private int mcount;
    /**
     * MIDlet information, class, name, icon; one per MIDlet.
     */
    private MIDletInfo[] minfo;
    /**
     * the Command object to exit back to the MIDlet Suite Manager
     */
    private Command backCmd = new Command(Resource.getString
                                          (ResourceConstants.BACK),
                                          Command.BACK, 2);
    /**
     * the Command object for "Launch".
     */
    private Command launchCmd = new Command(Resource.getString
                                            (ResourceConstants.LAUNCH),
                                            Command.ITEM, 1);
    /**
     * Index of the selected MIDlet, starts at -1 for non-selected.
     */
    private int selectedMidlet = -1;

    /**
     * Create and initialize a new Selector MIDlet.
     * The Display is retrieved and the list of MIDlets read
     * from the descriptor file.
     *
     * @param theSuiteInfo information needed to display a list of MIDlets
     * @param theDisplay the Display
     * @param theParentDisplayable the parent's displayable
     * @param theManager the parent application manager
     */
    MIDletSelector(RunningMIDletSuiteInfo theSuiteInfo, Display theDisplay,
                   Displayable theParentDisplayable,
                   ApplicationManager theManager) throws Throwable {

        MIDletSuiteStorage mss;

        suiteInfo = theSuiteInfo;
        display = theDisplay;
        parentDisplayable = theParentDisplayable;
        manager = theManager;
        mcount = 0;
        minfo = new MIDletInfo[20];

        mss = MIDletSuiteStorage.getMIDletSuiteStorage();

        readMIDletInfo(mss);
        setupList(mss);

        mlist.addCommand(launchCmd);
        mlist.addCommand(backCmd);

        mlist.setCommandListener(this); // Listen for the selection

        display.setCurrent(mlist);
    }

    /**
     * Respond to a command issued on any Screen.
     * The commands on list is Select and About.
     * Select triggers the creation of the MIDlet of the same name.
     * About puts up the copyright notice.
     *
     * @param c command activated by the user
     * @param s the Displayable the command was on.
     */
    public void commandAction(Command c, Displayable s) {
        if ((s == mlist && c == List.SELECT_COMMAND) || (c == launchCmd)) {
            synchronized (this) {
                if (selectedMidlet != -1) {
                    // the previous selected MIDlet is being launched
                    return;
                }

                selectedMidlet = mlist.getSelectedIndex();
            }

            manager.launchSuite(suiteInfo, minfo[selectedMidlet].classname);
            if (parentDisplayable != null) {
                display.setCurrent(parentDisplayable);
            } else {
                selectedMidlet = -1;
                display.setCurrent(mlist);
            }
            return;
        }

        if (c == backCmd) {
            if (parentDisplayable != null) {
                display.setCurrent(parentDisplayable);
            } else {
                manager.shutDown();
            }
            return;
        }
    }

    /**
     * Read the set of MIDlet names, icons and classes
     * Fill in the list.
     *
     * @param mss the midlet suite storage
     */
    private void setupList(MIDletSuiteStorage mss) {
        if (mlist == null) {
            mlist = new List(Resource.getString
                             (ResourceConstants.AMS_SELECTOR_SEL_TO_LAUNCH),
                             Choice.IMPLICIT);

            // Add each midlet
            for (int i = 0; i < mcount; i++) {
                Image icon = null;
                if (minfo[i].icon != null) {
                    icon = RunningMIDletSuiteInfo.getIcon(suiteInfo.suiteId,
                        minfo[i].icon, mss);
                }

                mlist.append(minfo[i].name, icon);
            }
        }
    }

    /**
     * Read in and create a MIDletInfo for each MIDlet-&lt;n&gt;
     *
     * @param mss the midlet suite storage
     */
    private void readMIDletInfo(MIDletSuiteStorage mss) throws Throwable {
        try {
            MIDletSuite midletSuite =
                mss.getMIDletSuite(suiteInfo.suiteId, false);

            if (midletSuite == null) {
                return;
            }

            try {
                for (int n = 1; n < 100; n++) {
                    String nth = "MIDlet-"+ n;
                    String attr = midletSuite.getProperty(nth);
                    if (attr == null || attr.length() == 0)
                        break;

                    addMIDlet(new MIDletInfo(attr));
                }
            } finally {
                midletSuite.close();
            }
        } catch (Throwable t) {
            throw t;
        }
    }

    /**
     * Add a MIDlet to the list.
     * @param info MIDlet information to add to MIDlet
     */
    private void addMIDlet(MIDletInfo info) {
        if (mcount >= minfo.length) {
            MIDletInfo[] n = new MIDletInfo[mcount+4];
            System.arraycopy(minfo, 0, n, 0, mcount);
            minfo = n;
        }

        minfo[mcount++] = info;
    }
}



