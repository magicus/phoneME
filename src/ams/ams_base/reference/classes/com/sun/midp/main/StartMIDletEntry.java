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

/** Implements generic monitor entry for a started MIDlet */
class StartMIDletEntry implements MIDletProxyListListener {
    /** The id of the MIDlet suite being started. */
    private int suiteId;
    /** The midlet of the MIDlet being started. */
    private String midlet;

    /**
     * Construct MIDlet start entry
     *
     * @param suiteId suite ID
     * @param midlet midlet class name
     */
    protected StartMIDletEntry(int suiteId, String midlet) {
        this.suiteId = suiteId;
        this.midlet = midlet;
    }

    /**
     * Creates entry factory
     * @return new factory instance 
     */
    static StartMIDletEntryFactory getFactory() {
        return new StartMIDletEntryFactory() {
            public StartMIDletEntry createInstance(
                    int suiteID, String midlet) {
                return new StartMIDletEntry(suiteID, midlet);
            }
        };
    }

    /**
     * Returns suite ID of the started MIDlet
     * @return suite ID
     */
    int getSuiteId() {
        return suiteId;
    }

    /**
     * Returns classname of the started MIDlet
     * @return MIDlet class name
     */
    String getMIDlet() {
        return midlet;
    }

    /**
     * Check whether the task MIDlet is started in has been
     * terminated in some way
     * @return true if terminated, false otherwise
     */
    boolean isTerminated() {
        return false;
    }

    /**
     * Unregister this start MIDlet entry in the case it
     * matches specified suite ID and classname  
     *
     * @param suiteId suite ID
     * @param midlet MIDlet classname
     */
    private void cleanupEntry(int suiteId, String midlet) {
        // If the notification is for this entry
        if (suiteId == this.suiteId &&
                (midlet == null || midlet.equals(this.midlet))) {
            // Cleanup pending entry of the start monitor
            StartMIDletMonitor.cleanupPending(this);
        }
    }

    /**
     * Called when a MIDlet is added to the list.
     * If there's a match in the startPending list clean it up.
     *
     * @param midlet The proxy of the MIDlet being added
     */
    public void midletAdded(MIDletProxy midlet) {
        cleanupEntry(midlet.getSuiteId(), midlet.getClassName());
    }

    /**
     * Called when the state of a MIDlet in the list is updated.
     * If there's a match in the startPending list clean it up.
     *
     * @param midlet The proxy of the MIDlet that was updated
     * @param fieldId code for which field of the proxy was updated
     */
    public void midletUpdated(MIDletProxy midlet, int fieldId) {
    }

    /**
     * Called when a MIDlet is removed from the list.
     * If there's a match in the startPending list clean it up.
     *
     * @param midlet The proxy of the removed MIDlet
     */
    public void midletRemoved(MIDletProxy midlet) {
        cleanupEntry(midlet.getSuiteId(), midlet.getClassName());
    }

    /**
     * Called when error occurred while starting a MIDlet object.
     * If there's a match in the startPending list clean it up.
     *
     * @param externalAppId ID assigned by the external application manager
     * @param suiteId Suite ID of the MIDlet
     * @param className Class name of the MIDlet
     * @param errorCode start error code
     * @param errorDetails start error details
     */
    public void midletStartError(int externalAppId, int suiteId,
            String className, int errorCode, String errorDetails) {
        cleanupEntry(suiteId, className);
    }

}
