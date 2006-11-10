/*
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;


/** Simple attribute storage for MIDlet suites */
class MIDletSuiteInfo {
    /** ID of the MIDlet suite. */
    String id;
    /** Display name of the MIDlet suite. */
    String displayName;
    /** Name of the MIDlet to run. */
    String midletToRun;
    /** Icon for this suite. */
    Image icon;
    /** Is this single MIDlet MIDlet suite. */
    boolean singleMidlet = false;
    /** Proxy if running. */
    MIDletProxy proxy;
    /** Is this suite enabled. */
    boolean enabled;

    /**
     * Constructs a MIDletSuiteInfo object for an "internal" suite.
     *
     * @param theID ID the system has for this suite
     * @param theMidletToRun Class name of the only midlet in the suite
     * @param theDisplayName Name to display to the user
     * @param isEnabled true if the suite is enabled
     */
    MIDletSuiteInfo(String theID, String theMidletToRun,
                    String theDisplayName, boolean isEnabled) {
        id = theID;
        midletToRun = theMidletToRun;
        displayName = theDisplayName;
        icon = getDefaultSingleSuiteIcon();
        enabled = isEnabled;
    }

    /**
     * Constructs a MIDletSuiteInfo object for a suite.
     *
     * @param theID ID the system has for this suite
     * @param theMidletSuite MIDletSuite  information
     * @param mss the midletSuite storage
     */
    MIDletSuiteInfo(String theID, MIDletSuiteImpl theMidletSuite,
                    MIDletSuiteStorage mss) {
        init(theID, theMidletSuite);

        icon = getIcon(theID, theMidletSuite.getProperty("MIDlet-Icon"), mss);

        if (theMidletSuite.getNumberOfMIDlets() == 1) {
            MIDletInfo midlet =
                new MIDletInfo(theMidletSuite.getProperty("MIDlet-1"));

            singleMidlet = true;
            midletToRun = midlet.classname;

            if (icon == null) {
                icon = getIcon(theID, midlet.icon, mss);
            }
        }

        if (icon == null) {
            icon = getDefaultSingleSuiteIcon();
        }
    }

    /**
     * Initializes MIDletSuiteInfo object
     *
     * @param theID ID the system has for this suite
     * @param theMidletSuite MIDletSuite  information
     */
    void init(String theID, MIDletSuiteImpl theMidletSuite) {
        displayName =
            theMidletSuite.getProperty(MIDletSuiteImpl.SUITE_NAME_PROP);

        if (displayName == null) {
            displayName = theID;
        }

        id = theID;

        enabled = theMidletSuite.isEnabled();
    }

    /**
     * Gets suite icon either from image cache, or from the suite jar.
     *
     * @param suiteID the suite id that system has for this suite
     * @param iconName the name of the file where the icon is
     *     stored in the JAR
     * @param mss The midletSuite storage
     * @return Image provided by the application with
     *     the passed in iconName
     */
    Image getIcon(String suiteID, String iconName, MIDletSuiteStorage mss) {
        byte[] iconBytes;

        if (iconName == null) {
            return null;
        }

        try {
            /* Search for icon in the image cache */
            iconBytes = loadCachedIcon(suiteID, iconName);
            if (iconBytes == null) {
                /* Search for icon in the suite JAR */
                iconBytes = JarReader.readJarEntry(
                    mss.getMidletSuiteJarPath(suiteID), iconName);
            }
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
     * Compares this MIDletSuiteInfo with the passed in MIDletProxy.
     * Returns true if both belong to the same suite and 
     * if current proxy or midetToRun points to the same class as
     * in the passed in MIDletProxy.
     * @param midlet The MIDletProxy to compare with
     * @return true if The MIDletSuiteInfo points to the same midlet as
     *         the MIDletProxy, false - otherwise
     */
    boolean equals(MIDletProxy midlet) {
        if (id.equals(midlet.getSuiteId())) {

            if (proxy != null) {
                return proxy == midlet;
            }

            if (midletToRun != null) {
                return midletToRun.equals(midlet.getClassName());
            }

            return true;
        }

        return false;
    }

    /**
     * Loads suite icon data from image cache.
     *
     * @param suiteID the ID of suite the icon belongs to
     * @param iconName the name of the icon to be loaded
     * @return cached image data if available, otherwise null
     */
    private native byte[] loadCachedIcon(String suiteID, String iconName);

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
            singleSuiteIcon =
                GraphicalInstaller.getImageFromStorage("_ch_single_large");
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
            multiSuiteIcon = 
                GraphicalInstaller.getImageFromStorage("_ch_suite_small");
        }
        return multiSuiteIcon;
    }
}
