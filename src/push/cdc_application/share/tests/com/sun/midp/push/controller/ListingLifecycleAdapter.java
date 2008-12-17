/*
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

package com.sun.midp.push.controller;

import java.util.Vector;

/**
 * Simplistic implementation of lifecycle interface
 * that just lists all invocations.
 */
final class ListingLifecycleAdapter implements LifecycleAdapter {
    private final Vector apps = new Vector();

    public void launchMidlet(final int midletSuiteID, final String midlet) {
        apps.add(new MIDPApp(midletSuiteID, midlet));
    }

    boolean hasBeenInvokedOnceFor(final int midletSuiteID,
				  final String midlet) {
        return (apps.size() == 1)
            && new MIDPApp(midletSuiteID, midlet).equals(apps.get(0));
    }

    boolean hasNotBeenInvoked() {
        return apps.isEmpty();
    }
}
