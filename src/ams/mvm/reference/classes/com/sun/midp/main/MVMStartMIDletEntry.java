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
import com.sun.cldc.isolate.Isolate;

/** Extends generic started MIDlet entry with MVM specific features */
class MVMStartMIDletEntry extends StartMIDletEntry {
    /** The IsolateID of the MIDlet being started. */
    private Isolate isolate;

    /**
     * Construct MIDlet start entry
     *
     * @param suiteId
     * @param midlet
     */
    protected MVMStartMIDletEntry(int suiteId, String midlet) {
        super(suiteId, midlet);
    }

    /**
     * Creates entry factory
     * @return new factory instance
     */
    static StartMIDletEntryFactory getFactory() {
        return new StartMIDletEntryFactory() {
            public StartMIDletEntry createInstance(
                    int suiteID, String midlet) {
                return new MVMStartMIDletEntry(suiteID, midlet);
            }
        };
    }

    /**
     * Sets the Isolate associated with this starting MIDlet.
     * It is used to cleanup the Isolate if the start does not
     * start correctly.
     *
     * @param newIsolate the Isolate used to start the MIDlet
     */
    void setIsolate(Isolate newIsolate) {
        isolate = newIsolate;
    }

    /**
     * Check whether the task MIDlet is started in has been
     * terminated in some way
     * @return true if terminated, false otherwise
     */
    boolean isTerminated() {
        return (isolate != null &&
            isolate.isTerminated());
    }

    /**
     * Called when a MIDlet is added to the list.
     * If there's a match in the startPending list clean it up.
     *
     * @param midlet The proxy of the MIDlet being added
     */
    public void midletAdded(MIDletProxy midlet) {
        IsolateMonitor.addIsolate(midlet, isolate);
        super.midletAdded(midlet);
    }
}
