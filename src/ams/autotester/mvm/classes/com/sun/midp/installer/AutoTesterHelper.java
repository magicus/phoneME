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

package com.sun.midp.installer;

import java.io.IOException;
import com.sun.midp.midletsuite.MIDletSuiteLockedException;

import com.sun.cldc.isolate.*;

import com.sun.midp.i18n.Resource;
import com.sun.midp.i18n.ResourceConstants;

import com.sun.midp.main.AmsUtil;
import com.sun.midp.main.MIDletSuiteUtils;

import com.sun.midp.midletsuite.MIDletInfo;
import com.sun.midp.midletsuite.MIDletSuiteStorage;
import com.sun.midp.midlet.MIDletSuite;
import com.sun.midp.configurator.Constants;

import com.sun.midp.events.*;

/**
 * Implements auto testing functionality not covered by AutoTesterHelperBaseMVM.
 */
final class AutoTesterHelper extends AutoTesterHelperBaseMVM {
    /**
     * Constructor.
     *
     * @param theURL URL of the test suite
     * @param theDomain security domain to assign to unsigned suites
     * @param theLoopCount how many iterations to run the suite
     */
    AutoTesterHelper(String theURL, String theDomain, int theLoopCount) {
        super(theURL, theDomain, theLoopCount);
    }

    /**
     * Called after test suite has been installed (updated).
     */
    void onTestSuiteInstalled() {
        // not interested in
    }

    /**
     * Called after test suite has been removed from storage (uninstalled).
     */
    void onTestSuiteRemoved() {
        // not interested in
    }
}
