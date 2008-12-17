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

import com.sun.test.JUnitUtils;

import junit.framework.Test;

/** Package test suite. */
public final class AllTests {
    /** Hidden ctor. */
    private AllTests() { }

    /**
     * Forms a suite.
     *
     * @return test suite to run
     */
    public static Test suite() {
        return JUnitUtils.createSuite(AllTests.class, new Class [] {
            AlarmControllerTest.class,
            ConnectionControllerTest.class,
            MIDPAppTest.class,
        });
    }

    /**
     * Stand-alone runner.
     *
     * @param args args
     */
    public static void main(final String[] args) {
        junit.textui.TestRunner.run(suite());
    }
}
