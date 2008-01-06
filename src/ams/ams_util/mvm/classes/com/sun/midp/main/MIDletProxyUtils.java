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

import com.sun.cldc.isolate.Isolate;

/**
 * Utilities for the MIDletProxy.  Does nothing is SVM mode.
 */
public class MIDletProxyUtils {

    /*
     * Set the MIDletProxy to run with the Maximum Isolate Priority
     *
     * @param mp MIDletProxy
     */
//    public static void maxPriority(MIDletProxy mp) {
//        Isolate isolate = getIsolateFromId(mp.getIsolateId());
//        if (isolate != null) {
//            isolate.setPriority(Isolate.MAX_PRIORITY);
//        }
//    }

    /**
     * Set the MIDletProxy to run with the Minimum Isolate Priority
     *
     * @param mp MIDletProxy
     */
    public static void minPriority(MIDletProxy mp) {
        Isolate isolate = getIsolateFromId(mp.getIsolateId());
        if (isolate != null) {
            isolate.setPriority(Isolate.MIN_PRIORITY);
        }
    }

    /**
     * Set the MIDletProxy to run with the Normal Isolate Priority
     *
     * @param mp MIDletProxy
     */
    public static void normalPriority(MIDletProxy mp) {
        Isolate isolate = getIsolateFromId(mp.getIsolateId());
        if (isolate != null) {
          isolate.setPriority(Isolate.NORM_PRIORITY);
        }
    }

    /**
     * Get the Isolate from a MIDletProxy's IsolateId
     *
     * @param id MIDletProxy's Isolate Id
     * @return MIDletProxy's Isolate
     */
    static Isolate getIsolateFromId(int id) {
        if (id > 1) {
            Isolate[] isolate = Isolate.getIsolates();
            for (int i = 0; i < isolate.length; i++) {
                if (isolate[i].id() == id) {
                    return isolate[i];
                }
            }
        }
        return null;
    }

    /**
     * Terminates an isolate correspondent to the proxy given, resets
     * proxy termination timer and invokes proper proxy list updates.
     * Waits for termination completion.
     * @param mp MIDlet proxy for the isolate to be terminated
     * @param mpl the MIDlet proxy list
     */
    static void terminateMIDletIsolate(MIDletProxy mp, MIDletProxyList mpl) {
        Isolate isolate = getIsolateFromId(mp.getIsolateId());

        /* could not find an isolate that matches the MIDletProxy isolate ID
         * remove the MIDletProxy from the list as it is left over from 
         * previous run and the isolate is already terminated */
        mp.setTimer(null);
        if (isolate != null) {
            isolate.exit(0);
            // IMPL_NOTE: waiting for termination completion may be useless.
            isolate.waitForExit();
        }
        mpl.removeIsolateProxies(mp.getIsolateId());
    }

    /**
     * Change the MIDlet's state to suspended
     * 
     * @param isolateId the ID of the isolate to suspend
     * @return <code>true</code> if isolate is suspended, 
     *         <code>false</code> otherwise
     */
    public static boolean suspendIsolate(int isolateId) {
        if (isolateId == MIDletSuiteUtils.getAmsIsolateId()) {
            // AMS isolate must not be paused
            return false;
        }

        Isolate[] isolates = Isolate.getIsolates();
        int isolateNum = isolates.length;

        while (isolateNum-- > 0) {
            if (isolates[isolateNum].id() == isolateId) {
                isolates[isolateNum].suspend();
                return true;
            }
        }
        return false;
    }

    /**
     * Change the MIDlet's state to active
     * 
     * @param isolateId the ID of the isolate to resume
     * @return <code>true</code> if isolate is resumed, 
     *         <code>false</code> otherwise
     */
    public static boolean continueIsolate(int isolateId) {
        Isolate[] isolates = Isolate.getIsolates();
        int isolateNum = isolates.length;

        while (isolateNum-- > 0) {
            if (isolates[isolateNum].id() == isolateId) {
                isolates[isolateNum].resume();
                return true;
            }
        }
        return false;
    }
}
