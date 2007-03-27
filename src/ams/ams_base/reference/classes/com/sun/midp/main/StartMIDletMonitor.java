/*
 *
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

package com.sun.midp.main;

import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;
import java.util.Vector;

/**
 * Implements the mechanism to monitor the startup of a MIDlet.
 * It keeps track of which MIDlets are being started
 * and are not yet in the MIDletProxyList.
 * The information is used to avoid starting the same
 * MIDlet twice.
 */
class StartMIDletMonitor {
    /** Reference to the ProxyList */
    private static MIDletProxyList midletProxyList;
    /** Vector of pending start requests */
    private static Vector startPending = new Vector();
    /** Factory to create new MIDlet start entries */
    private static StartMIDletEntryFactory entryFactory;

    /**
     * Initializes StartMIDletMonitor class.
     * Shall only be called from AmsUtil.
     * No need in security checks since it is package private method.
     *
     * @param theMIDletProxyList MIDletController's container
     */
    static void initClass(MIDletProxyList theMIDletProxyList,
            StartMIDletEntryFactory theEntryFactory) {
        midletProxyList = theMIDletProxyList;
        entryFactory = theEntryFactory;
    }

    /**
     * Check if the MIDlet is already in the ProxyList or is
     * already being started.  If so, return.
     * If not, start it. Register with the proxy list and
     * cleanup when the start of the MIDlet
     * succeeds (and is now in the ProxyList) or
     * fails (and is eligible to be started again).
     *
     * @param id ID of an installed suite
     * @param midlet class name of MIDlet to invoke; may be null
     * @return the new StartMIDletMonitor to allow the MIDlet to be started;
     *    null if the MIDlet is already active or being started
     */
    static StartMIDletEntry okToStart(int id, String midlet) {
        synchronized (startPending) {

            // Verify that the requested MIDlet is not already running
            // (is not in the MIDletProxyList)
            if (midletProxyList.isMidletInList(id, midlet)) {
                if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                    Logging.report(Logging.WARNING, LogChannels.LC_CORE,
                        "MIDlet already running; execute ignored");
                }
                return null;
            }

            // Find the StartMIDletMonitor instance
            // to track the startup, (if any)
            StartMIDletEntry start = findPending(id, midlet);
            if (start == null) {
                // Not already starting; register new start
                start = entryFactory.createInstance(id, midlet);
                addPending(start);
            } else {
                // MIDlet is already started; return null
                start = null;
                if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                    Logging.report(Logging.WARNING, LogChannels.LC_CORE,
                        "MIDlet already started; execute ignored");
                }
            }
            return start;
        }
    }

    /**
     * Scan the startPending list for a matching id and MIDlet.
     * The caller must synchronize using {@link #startPending}.
     * If <code>midlet</code> is null then it only checks to see
     * if the suite is started and returns any monitor for the suite.
     * To prevent using stale Isolate state; check that the Isolate (if any)
     * has not terminated.
     *
     * @param id ID of an installed suite
     * @param midlet class name of MIDlet to invoke
     * @return a StartMIDletMonitor entry with id and midlet;
     *    otherwise <code>null</code>
     */
    private static StartMIDletEntry findPending(int id, String midlet) {
        for (int i = 0; i < startPending.size(); i++) {
            StartMIDletEntry pending =
                (StartMIDletEntry)startPending.elementAt(i);

            // If there is a terminated entry in the list, clean it up
            if (pending.isTerminated()) {
                // The MIDlet start is terminated, clean the pending entry
                startPending.removeElementAt(i);
                midletProxyList.removeListener(pending);
                // Recheck the element at the same index
                i--;
                continue; // keep looking
            }
            if (id == pending.getSuiteId() &&
                    (midlet == null || midlet.equals(pending.getMIDlet()))) {
                return pending;
            }
        }
        return null;
    }

    /**
     * Construct a new StartMIDletMonitor instance to track the
     * process of starting a MIDlet in a new Isolate.
     * The new instance is appended to the startPending vector
     * and is registered with the MIDletProxyList to receive
     * notifications if/when the MIDlet starts/fails to start.
     *
     * @param pending entry for started MIDlet
     */
    private static void addPending(StartMIDletEntry pending) {
        startPending.addElement(pending);
        midletProxyList.addListener(pending);
    }

    /**
     * Cleanup the matching entry in the startPending list.
     * Once removed; the MIDlet will be eligible to be started
     * again.
     * @param pending entry of the started MIDlet
     */
    static void cleanupPending(StartMIDletEntry pending) {
        synchronized (startPending) {
            // Remove from the startPending list
            startPending.removeElement(pending);
            // Remove the instance as a listener of the MIDletProxyList
            midletProxyList.removeListener(pending);
        }
    }
}

/** Factory interface to create new MIDlet start entries */
interface StartMIDletEntryFactory {
    public StartMIDletEntry createInstance(
        int suiteID, String midlet);
}
