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

package com.sun.midp.suspend;

import com.sun.midp.main.MIDletProxyList;
import com.sun.midp.main.Configuration;
import com.sun.midp.security.SecurityToken;
import com.sun.midp.security.Permissions;

import java.util.Timer;
import java.util.TimerTask;

/**
 * Timer for terminating MIDlets that have not completed
 * their pase routines within suspend timeout.
 */
public class SuspendTimer extends Timer {
    /**
     * The timeout within which MIDlets have chance to complete.
     */
    private static final long TIMEOUT =
            Configuration.getIntProperty("suspendAppTimeout", 2000);

    /**
     * The only instance if suspend timer.
     */
    private static SuspendTimer timer;

    /**
     * MIDlet proxy list.
     */
    private static MIDletProxyList midletList;

    /** Constructs an instance. */
    private SuspendTimer() {}

    /**
     * Retrieves the timer.
     * @return token security token to guard this restricted API.
     */
    public static synchronized SuspendTimer getInstance(SecurityToken token) {
        token.checkIfPermissionAllowed(Permissions.AMS);

        if (null == timer) {
            midletList = MIDletProxyList.getMIDletProxyList(token);
            timer = new SuspendTimer();
        }

        return timer;
    }

    /**
     * Schedules standard MIDlets termination task to sandard timeout.
     */
    public void start() {

        TimerTask task = new TimerTask() {
            public void run() {
                midletList.terminatePauseAll();
                cancel();
            }
        };

        schedule(task, TIMEOUT);
    }
}
