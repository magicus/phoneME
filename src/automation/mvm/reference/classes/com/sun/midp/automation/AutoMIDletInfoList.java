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

package com.sun.midp.automation;

import java.util.*;
import com.sun.midp.main.*;

/**
 * AutoMIDletInfo list.
 */
class AutoMIDletInfoList {
    /** Vector of AutoMIDletInfo objects */
    private Vector midletsInfo;

    /** The one and only instance of AutoMIDletInfoList */
    private static AutoMIDletInfoList midletInfoList = null;

    /**
     * Private constructor to prevent direct creation of instances.
     */
    AutoMIDletInfoList() {
        midletsInfo = new Vector();
    }


    /**
     * Gets AutoMIDletInfoList instance.
     *
     * @return AutoMIDletInfoList instance
     */
    synchronized static AutoMIDletInfoList getMIDletInfoList() {
        if (midletInfoList == null) {
            midletInfoList = new AutoMIDletInfoList();
        }

        return midletInfoList;
    }


    /**
     * Creates AutoMIDletInfo and adds it to the list.
     *
     * @param suiteID suite ID
     * @param midletClassName MIDlet's class name
     * @return created AutoMIDletInfo instance
     */
    AutoMIDletInfo addToList(int suiteID, String midletClassName) {
        synchronized (this) {
            AutoMIDletInfo info =
                new AutoMIDletInfo(suiteID, midletClassName);
            midletsInfo.addElement(info);

            return info;
        }
    }


    /**
     * Finds MIDlet info by MIDlet's suite ID and class name.
     *
     * @param suiteID suite ID
     * @param midletClassName MIDlet's class name
     * @return corresponding AutoMIDletInfo instance or null,
     *         if it hasn't been found
     */
    AutoMIDletInfo findMIDletInfo(int suiteID, String midletClassName) {
        synchronized (this) {
            for (int i = 0; i < midletsInfo.size(); ++i) {
                AutoMIDletInfo info =
                    (AutoMIDletInfo)midletsInfo.elementAt(i);
                if (info.suiteID == suiteID &&
                    info.midletClassName.equals(midletClassName)) {
                    return info;
                }
            }
        }

        return null;
    }

    /**
     * Finds MIDlet info by AutoMIDletImpl.
     *
     * @param midlet AutoMIDletImpl reference to be used as key
     * @return corresponding AutoMIDletInfo instance or null,
     *         if it hasn't been found
     */
    AutoMIDletInfo findMIDletInfo(AutoMIDletImpl midlet) {
        synchronized (this) {
            for (int i = 0; i < midletsInfo.size(); ++i) {
                AutoMIDletInfo info =
                    (AutoMIDletInfo)midletsInfo.elementAt(i);
                if (info.midlet == midlet) {
                    return info;
                }
            }
        }

        return null;
    }

    /**
     * Finds MIDlet info by MIDletProxy.
     *
     * @param midletProxy MIDletProxy reference to be used as key
     * @return corresponding AutoMIDletInfo instance or null,
     *         if it hasn't been found
     */
    AutoMIDletInfo findMIDletInfo(MIDletProxy midletProxy) {
        AutoMIDletInfo info = findMIDletInfo(
                midletProxy.getSuiteId(), midletProxy.getClassName());

        return info;
    }


    /**
     * Finds AutoMIDlet corresponding to specified MIDletProxy.
     *
     * @param midletProxy MIDletProxy instance
     * @return AutoMIDletImpl instance corresponding to MIDletProxy,
     *         or null if it hasn't been found
     */
    AutoMIDletImpl findMIDlet(MIDletProxy midletProxy) {
        AutoMIDletImpl midlet = null;
        AutoMIDletInfo info = findMIDletInfo(midletProxy);
        if (info != null) {
            midlet = info.midlet;
        }

        return midlet;
    }

    /**
     * Finds MIDletProxy corresponding to specified AutoMIDlet
     *
     * @param midlet AutoMIDletImpl instance
     * @return MIDletProxy instance corresponding to AutoMIDletImpl,
     *         or null if it hasn't been found
     */
    MIDletProxy findMIDletProxy(AutoMIDletImpl midlet) {
        MIDletProxy midletProxy = null;
        AutoMIDletInfo info = findMIDletInfo(midlet);
        if (info != null) {
            midletProxy = info.midletProxy;
        }

        return midletProxy;
    }

    /**
     * Removes specified AutoMIDletInfo instance from list.
     *
     * @param info AutoMIDletInfo instance to be removed
     */
    void removeFromList(AutoMIDletInfo info) {
        midletsInfo.removeElement(info);
    }
}
