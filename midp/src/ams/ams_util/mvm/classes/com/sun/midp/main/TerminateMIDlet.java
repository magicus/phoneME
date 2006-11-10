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

package com.sun.midp.main;

import java.util.TimerTask;
import com.sun.cldc.isolate.Isolate;

/**
 * TimerTask for the MIDlet.  Does nothing is SVM mode.
 */
class TerminateMIDlet extends TimerTask {

    /** MIDletProxy object */
    MIDletProxy mp;

    /** MIDlet proxy list. */
    MIDletProxyList mpl;

    /**
     * Construct a TerminateMIDlet
     *
     * @param mp MIDletProxy
     * @param mpl MIDletProxyList
     */
    TerminateMIDlet(MIDletProxy mp, MIDletProxyList mpl) {
        this.mp = mp;
        this.mpl = mpl;
    }

    /**
     * Terminate the hanging midlet when the timer expires
     * and remove the proxy from the MIDletProxyList.
     * Cancel the timer if the midlet has already terminated.
     */
    public void run() {
        if (mp.getTimer() != null) {
            int id = mp.getIsolateId();
            Isolate[] isolate = Isolate.getIsolates();

            for (int i = 0; i < isolate.length; i++) {
                if (isolate[i].id() == id) {
                    mp.setTimer(null);
                    isolate[i].exit(0);
                    isolate[i].waitForExit();
                    mpl.removeIsolateProxies(id);
                    cancel();
                    break;
                }
            }
        } else {
            cancel();
        }
    }
}


