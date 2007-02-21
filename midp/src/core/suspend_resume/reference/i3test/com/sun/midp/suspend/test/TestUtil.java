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

package com.sun.midp.suspend.test;

import com.sun.midp.security.ImplicitlyTrustedClass;
import com.sun.midp.security.SecurityToken;
import com.sun.midp.security.SecurityInitializer;
import com.sun.midp.suspend.SuspendSystem;

/**
 * Utilities for suspend/resume testing.
 */
public class TestUtil {
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
