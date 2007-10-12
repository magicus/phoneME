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

import junit.framework.*;

public class MIDPAppTest extends TestCase {
    public MIDPAppTest(final String testName) {
        super(testName);
    }

    public void testCheckEqual() {
        final MIDPApp app0 = new MIDPApp(239, "com.sun.Foo");
        final MIDPApp app1 = new MIDPApp(239, "com.sun.Foo");

        assertTrue(app0.equals(app0));
        assertTrue(app0.equals(app1));
        assertTrue(app1.equals(app0));
        assertTrue(app0.hashCode() == app1.hashCode());
    }

    public void testCheckDifferentSuites() {
        final int SUITE_ID_0 = 239;
        final int SUITE_ID_1 = 139;
        final String MIDLET = "com.sun.Foo";

        final MIDPApp app0 = new MIDPApp(SUITE_ID_0, MIDLET);
        final MIDPApp app1 = new MIDPApp(SUITE_ID_1, MIDLET);

        assertFalse(app0.equals(app1));
        assertFalse(app1.equals(app0));
        assertFalse(app0.hashCode() == app1.hashCode());
    }

    public void testCheckDifferentMidlets() {
        final int SUITE_ID = 239;
        final String MIDLET_0 = "com.sun.Foo";
        final String MIDLET_1 = "com.sun.Bar";

        final MIDPApp app0 = new MIDPApp(SUITE_ID, MIDLET_0);
        final MIDPApp app1 = new MIDPApp(SUITE_ID, MIDLET_1);

        assertFalse(app0.equals(app1));
        assertFalse(app1.equals(app0));
        assertFalse(app0.hashCode() == app1.hashCode());
    }

    public void testCheckTotallyDifferent() {
        final int SUITE_ID_0 = 239;
        final int SUITE_ID_1 = 139;
        final String MIDLET_0 = "com.sun.Foo";
        final String MIDLET_1 = "com.sun.Bar";

        final MIDPApp app0 = new MIDPApp(SUITE_ID_0, MIDLET_0);
        final MIDPApp app1 = new MIDPApp(SUITE_ID_1, MIDLET_1);

        assertFalse(app0.equals(app1));
        assertFalse(app1.equals(app0));
        assertFalse(app0.hashCode() == app1.hashCode());
    }
}
