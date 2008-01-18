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

package com.sun.test;

import junit.framework.Test;
import junit.framework.TestSuite;

/** Various JUnit utilities. */
public final class JUnitUtils {
    /** Hidden ctor. */
    private JUnitUtils() { }

    /**
     * Forms a suite.
     *
     * @param allTestsClass AllTests class
     * @param tests tests of the suite
     *
     * @return test suite to run
     */
    public static Test createSuite(final Class allTestsClass,
				   final Class [] tests) {
        final String title = allTestsClass.getPackage().getName();
        final TestSuite suite = new TestSuite(title);
        for (int i = 0; i < tests.length; i++) {
            suite.addTestSuite(tests[i]);
        }
        return suite;
    }
}
