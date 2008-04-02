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

import com.sun.midp.main.*;
import com.sun.midp.midletsuite.*;
import com.sun.midp.installer.*;
import javax.microedition.lcdui.*;
import com.sun.midp.configurator.Constants;

import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;
import com.sun.midp.midlet.MIDletSuite;


/** Simple attribute storage for MIDlet suites */
public class RunningMIDletSuiteInfo extends MIDletSuiteInfo {
    /** Proxy if running. It is set from AppManagerUI.java. */
    public MIDletProxy proxy = null;
    /** Icon for this suite. */
    public Image icon = null;

    /**
     * Constructs a RunningMIDletSuiteInfo object for a suite.
     *
     * @param theID ID the system has for this suite
     */
    public RunningMIDletSuiteInfo(int theID) {
        super(theID);
    }

    /**
     * Constructs a RunningMIDletSuiteInfo object for a suite.
     *
     * @param theID ID the system has for this suite
     * @param theMidletToRun Class name of the only midlet in the suite
     * @param theDisplayName Name to display to the user
     * @param isEnabled true if the suite is enabled
     */
    public RunningMIDletSuiteInfo(int theID, String theMidletToRun,
            String theDisplayName, boolean isEnabled) {
        super(theID, theMidletToRun, theDisplayName, isEnabled);
        icon = getDefaultSingleSuiteIcon();
    }

    /**
     * Constructs a RunningMIDletSuiteInfo object for a suite.
     *
     * @param theID ID the system has for this suite
     * @param theMidletSuite MIDletSuite information
     * @param mss the midletSuite storage
     */
    public RunningMIDletSuiteInfo(int theID, MIDletSuiteImpl theMidletSuite,
                           MIDletSuiteStorage mss) {
        super(theID, theMidletSuite);

        icon = getIcon(theID, theMidletSuite.getProperty("MIDlet-Icon"), mss);
        if (icon == null && numberOfMidlets == 1) {
            MIDletInfo midlet =
                new MIDletInfo(theMidletSuite.getProperty("MIDlet-1"));

            // MIDlet icons are optional, so it the icon may be null
            icon = getIcon(theID, midlet.icon, mss);
        }

        if (icon == null) {
            icon = getDefaultSingleSuiteIcon();
        }
    }

    /**
     * Constructs a RunningMIDletSuiteInfo from MIDletSuiteInfo.
     *
     * @param info MIDletSuiteInfo reference
     * @param mss the midletSuite storage
     */
    public RunningMIDletSuiteInfo(MIDletSuiteInfo info,
                                  MIDletSuiteStorage mss) {
        super(info.suiteId, info.midletToRun, info.displayName,
              info.enabled);

        storageId = info.storageId;
        folderId = info.folderId;
        numberOfMidlets = info.numberOfMidlets;
        trusted = info.trusted;
        preinstalled = info.preinstalled;
        iconName = info.iconName;

        loadIcon(mss);
    }

    /**
     * Loads an icon for this suite.
     *
     * @param mss the midletSuite storage
     */
    public void loadIcon(MIDletSuiteStorage mss) {
        if (iconName != null) {
            icon = getIcon(suiteId, iconName, mss);
        }

        if (icon == null) {
            if (numberOfMidlets == 1) {
                icon = getDefaultSingleSuiteIcon();
            } else {
                icon = getDefaultMultiSuiteIcon();
            }
        }
    }

    /**
     * Gets suite icon either from image cache, or from the suite jar.
     *
     * @param theID the suite id that system has for this suite
     * @param iconName the name of the file where the icon is
     *     stored in the JAR
     * @param mss The midletSuite storage
     * @return Image provided by the application with
     *     the passed in iconName
     */
    public static Image getIcon(int theID, String iconName,
            MIDletSuiteStorage mss) {
        byte[] iconBytes;

        try {
            iconBytes = mss.getMIDletSuiteIcon(theID, iconName);

            if (iconBytes == null) {
                if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                    Logging.report(Logging.WARNING, LogChannels.LC_AMS,
                        "getIcon: iconBytes == null");
                }
                return null;
            }

            return Image.createImage(iconBytes, 0, iconBytes.length);
        } catch (Throwable t) {
            if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                Logging.report(Logging.WARNING, LogChannels.LC_AMS,
                    "getIcon threw an " + t.getClass());
            }
            return null;
        }
    }

    /**
     * Returns a string representation of the MIDletSuiteInfo object.
     * For debug only.
     */
    public String toString() {
        StringBuffer b = new StringBuffer();
        b.append("id = " + suiteId);
        b.append(", midletToRun = " + midletToRun);
        b.append(", proxy = " + proxy);
        return b.toString();
    }

    /**
     * Compares this MIDletSuiteInfo with the passed in MIDletProxy.
     * Returns true if both belong to the same suite and
     * if current proxy or midetToRun points to the same class as
     * in the passed in MIDletProxy.
     * @param midlet The MIDletProxy to compare with
     * @return true if The MIDletSuiteInfo points to the same midlet as
     *         the MIDletProxy, false - otherwise
     */
    public boolean equals(MIDletProxy midlet) {
        if (suiteId == midlet.getSuiteId()) {
            if (proxy != null) {
                return proxy == midlet;
            }

            if ((numberOfMidlets == 1 ||
                 suiteId == MIDletSuite.INTERNAL_SUITE_ID) &&
                    midletToRun != null) {
                return midletToRun.equals(midlet.getClassName());
            }

            return true;
        }

        return false;
    }

    /** Cache of the suite icon. */
    private static Image multiSuiteIcon;

    /** Cache of the single suite icon. */
    private static Image singleSuiteIcon;

    /**
     * Gets the single MIDlet suite icon from storage.
     *
     * @return icon image
     */
    private static Image getDefaultSingleSuiteIcon() {
        if (singleSuiteIcon == null) {
            singleSuiteIcon = GraphicalInstaller.
                getImageFromInternalStorage("_ch_single");
        }
        return singleSuiteIcon;
    }

    /**
     * Gets the MIDlet suite icon from storage.
     *
     * @return icon image
     */
    private static Image getDefaultMultiSuiteIcon() {
        if (multiSuiteIcon == null) {
            multiSuiteIcon = GraphicalInstaller.
                getImageFromInternalStorage("_ch_suite");
        }
        return multiSuiteIcon;
    }
}
