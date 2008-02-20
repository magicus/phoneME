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

package com.sun.midp.suspend.test;

import com.sun.midp.security.ImplicitlyTrustedClass;
import com.sun.midp.security.SecurityToken;
import com.sun.midp.security.SecurityInitializer;
import com.sun.midp.suspend.SuspendSystem;
import com.sun.midp.midlet.MIDletPeer;
import com.sun.midp.midlet.MIDletStateHandler;

import javax.microedition.midlet.MIDlet;

/**
 * Utilities for suspend/resume testing.
 */
public class TestUtil {
    /** Utilities for getting MIDlet state. */
    public static class MidletState {
        /** MIDlet state can not be determined. */
        public static final int UNKNOWN = 0;
        /** Paused state. */
        public static final int PAUSED = 1;
        /** Active state. */
        public static final int ACTIVE = 2;
        /** Destroyed state. */
        public static final int DESTROYED = 3;
        /**
         * Retrieves current MIDlet state.
         * @param midlet the MIDlet to get state for.
         * @return one of UNKNOWN, PAUSED, ACTIVE, DESTROYED
         */
        public static int get(MIDlet midlet) {
            int state = MIDletStateHandler.getMIDletState(midlet);
            switch (state) {
                case MIDletPeer.PAUSED: return PAUSED;
                case MIDletPeer.ACTIVE: return ACTIVE;
                case MIDletPeer.DESTROYED: return DESTROYED;
                default: return UNKNOWN;
            }
        }
    }

    /** Class registered in SecurityInitializer. */
    private static class SecurityTrusted implements ImplicitlyTrustedClass {}

    /** Security token for provileged access to internal API's. */
    private static SecurityToken securityToken =
            SecurityInitializer.requestToken(new SecurityTrusted());

    /**
     * Default delay time.
     */
    public static final int DELAY = 3000;

    /**
     * Provides execution delay.
     * @param ms delay time in milliseconds
     */
    public static void sleep(long ms) {
        try {
            Thread.sleep(ms);
        } catch (InterruptedException e) {
            // ignoring
        }
    }

    /**
     * Retrieves SuspendSystem instance using privileged security token.
     * @return SuspendSystem singleton instance.
     */ 
    public static SuspendSystem getSuspendSystem() {
        return SuspendSystem.getInstance(securityToken);
    }
    
    /**
     * Provides default execution delay.
     */
    public static void sleep() {
        sleep(DELAY);
    }

    /**
     * Sends MIDP suspend request.
     */
    public static native void suspendMidp();

    /**
     * Sends MIDP resume request.
     */
    public static native void resumeMidp();

    /**
     * Requests MIDP to suspend and then resume by timeout.
     * @param timeout resume timeout.
     */
    public static native void suspendAndResumeMidp(int timeout);

    /**
     * Sets special testing suspend mode that does not suspend VM.
     */
    public static native void setNoVMSuspendMode();

    /**
     * Sets suspend mode that suspends both resources and VM.
     */
    public static native void setVMSuspendMode();
}
