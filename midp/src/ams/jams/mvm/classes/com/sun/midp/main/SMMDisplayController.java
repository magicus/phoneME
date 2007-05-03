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

import java.util.*;

/**
 * This class controls which MIDlet's display is in the foreground.
 * Running only in the AMS Isolate (0) the controller consulted by the MIDlet
 * proxy list for any foreground when various state changes occur in a MIDlet.
 * The display controller automatically selects the next foreground if
 * needed.
 * <p>
 * From the user perspective when the last MIDlet the user launched is created,
 * that MIDlet should automatically get the foreground (see the midletCreated
 * and foregroundRequest methods).
 * <p>
 * A MIDlet that is paused or destroyed is treated as if it has requested the
 * background as described above.
 */
public class SMMDisplayController extends DisplayController {
    /** Suite ID of the last MIDlet in foreground. */
    private int lastMidletSuiteId;

    /** Class name of the last MIDlet in foreground. */
    private String lastMidletClassName;

    /** This MIDlet should only get the foreground if it is the last MIDlet. */
    private MIDletProxy lastMidletInForeground;

    /**
     * Construct a DisplayController with a reference to the ProxyList.
     *
     * @param theMIDletProxyList reference to the MIDlet proxy list
     * @param suiteId the suiteId of the last MIDlet in the foreground
     * @param classname classname of the last MIDlet in the foreground
     */
    public SMMDisplayController(MIDletProxyList theMIDletProxyList,
                                int suiteId, String classname) {
        super(theMIDletProxyList);
        midletProxyList = theMIDletProxyList;
        lastMidletSuiteId = suiteId;
        lastMidletClassName = classname;
    }

    /**
     * Handles MIDlet background requests.
     * <p>
     * If the MIDlet requesting to be put in the background is the foreground
     * MIDlet, then find a MIDlet to bring to the foreground (see the
     * findNextForeground method).
     *
     * @param midlet The proxy of the MIDlet that was updated
     *
     * @return Proxy of the next foreground MIDlet, may be the foreground
     *         MIDlet if the foreground should not change
     */
    MIDletProxy backgroundRequest(MIDletProxy midlet) {
        MIDletProxy foreground = midletProxyList.getForegroundMIDlet();

        if (midlet != foreground) {
            // not in the foreground, so don't change the foreground
            return foreground;
        }

        return findNextForegroundMIDlet();
    }

    /**
     * Find a MIDlet that wants the foreground. If none wants the foreground
     * then find one that is not paused, if no find one that is paused
     * and wants the foreground, then find one that is paused. Only return the
     * "last MIDlet to have the foreground" if there are not other MIDlets.
     *
     * @return new foreground task or null
     */
    private MIDletProxy findNextForegroundMIDlet() {
        Enumeration midlets;

        // find the first task that is active and wants foreground
        midlets = midletProxyList.getMIDlets();
        while (midlets.hasMoreElements()) {
            MIDletProxy current = (MIDletProxy)midlets.nextElement();

            if (current.getMidletState() != MIDletProxy.MIDLET_ACTIVE) {
                continue;
            }

            if (current.wantsForeground() &&
                    current != getLastMidletInForeground()) {
                return current;
            }
        }

        // find the first task that is active
        midlets = midletProxyList.getMIDlets();
        while (midlets.hasMoreElements()) {
            MIDletProxy current = (MIDletProxy)midlets.nextElement();

            if (current.getMidletState() != MIDletProxy.MIDLET_ACTIVE &&
                    current != getLastMidletInForeground()) {
                return current;
            }
        }

        // find the first task that is paused and wants the foreground
        midlets = midletProxyList.getMIDlets();
        while (midlets.hasMoreElements()) {
            MIDletProxy current = (MIDletProxy)midlets.nextElement();

            if (current.getMidletState() != MIDletProxy.MIDLET_PAUSED &&
                    current != getLastMidletInForeground()) {
                continue;
            }

            if (current.wantsForeground() &&
                    current != getLastMidletInForeground()) {
                return current;
            }
        }

        // find the first task that is paused
        midlets = midletProxyList.getMIDlets();
        while (midlets.hasMoreElements()) {
            MIDletProxy current = (MIDletProxy)midlets.nextElement();

            if (current.getMidletState() != MIDletProxy.MIDLET_PAUSED &&
                    current != getLastMidletInForeground()) {
                return current;
            }
        }

        return getLastMidletInForeground();
    }

    /**
     * Returns the last MIDlet to that should get the foreground.
     *
     * @return last MIDlet in foreground
     */
    private MIDletProxy getLastMidletInForeground() {
        if (lastMidletInForeground == null) {
            lastMidletInForeground =
                midletProxyList.findMIDletProxy(lastMidletSuiteId,
                                                lastMidletClassName);
        }

        return lastMidletInForeground;
    }
}
