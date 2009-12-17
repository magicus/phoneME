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

package com.sun.midp.main;

/**
 * IndicatorManager is a singleton class that controls the home icon
 * status in status bar.
 */
public class IndicatorManager implements MIDletProxyListListener {

    /** singleton variable */
    private static IndicatorManager singleton;

    /** boolean flag that indicated home icon status */
    private static boolean homeIconState;

    /** Internal midletProxyList that is used to add listener */
    private static MIDletProxyList midletProxyList;

    /**
     * Native method to toggle the home icon
     *
     * @param isHomeOn : boolean flag to indicate status of home icon
     *
     */
    private static native void toggleHomeIcon0(boolean isHomeOn);

    /**
     * IndicatorManager is a singleton.
     */
    private IndicatorManager() {
    }

    /**
     * A static method that initialize singleton IndicatorBar class
     *
     * @param theMidletProxyList a reference to the MIDlet proxy list
     */
    public static void init(MIDletProxyList theMidletProxyList) {
        if (singleton == null) {
            midletProxyList = theMidletProxyList;
            singleton = new IndicatorManager();
            midletProxyList.addListener(singleton);
        }
    }

    /**
     * This method is called whenever home icon needs to be
     * turned on/off
     *
     * @param newHomeIconState boolean flag to indicate status of home icon
     */
    private static void setHomeIconState(boolean newHomeIconState) {
        if (homeIconState != newHomeIconState) {
            homeIconState = newHomeIconState;
            toggleHomeIcon0(homeIconState);
        }
    }

    /**
     *  This function is public so that unit test can refer it
     *
     * @return state of the home icon
     */
    public static boolean getHomeIconState() {
        return homeIconState;
    }

    /**
     * Called when a MIDlet is added to the list.
     *
     * @param midlet The proxy of the MIDlet being added
     */
    public void midletAdded(MIDletProxy midlet) {
        // IndicatorManager does not care if midlet is added
    }

    /**
     * Called when a MIDlet is removed from the list.
     *
     * @param midlet The proxy of the MIDlet being removed
     */
    public void midletRemoved(MIDletProxy midlet) {
        setHomeIconState(midletProxyList.isAlertWaitingInBackground());
    }

    /**
     * Called when the state of a MIDlet in the list is updated.
     *
     * @param midlet The proxy of the MIDlet being updated
     * @param fieldId code for which field of the proxy was updated
     */
    public void midletUpdated(MIDletProxy midlet, int fieldId) {
        setHomeIconState(midletProxyList.isAlertWaitingInBackground());
    }

    /**
     * Called when error occurred while starting a MIDlet object.
     *
     * @param externalAppId ID assigned by the external application manager
     * @param suiteId Suite ID of the MIDlet
     * @param className Class name of the MIDlet
     * @param errorCode start error code
     * @param errorDetails start error details
     */
    public void midletStartError(int externalAppId, int suiteId,
                                 String className, int errorCode,
                                 String errorDetails) {}

}
