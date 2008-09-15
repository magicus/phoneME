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

package com.sun.midp.automation;

import com.sun.midp.midletsuite.*;
import com.sun.midp.midlet.MIDletSuite;
import java.util.*;
import java.io.*;

/**
 * AutoSuiteDescriptor interface implementation for internal suite.
 */
final class AutoInternalSuiteDescriptorImpl
    extends AutoSuiteDescriptorImpl {

    /** Name of the suite's MIDlet class */
    private String midletClassName;

    /** Suite ID for all internal suites */
    static final int INTERNAL_SUITE_ID = MIDletSuite.INTERNAL_SUITE_ID;


    /**
     * Constructor
     *
     * @param midletClassName name of the suite's MIDlet class
     * @param midletSuite internal MIDlet suite representation
     */
    AutoInternalSuiteDescriptorImpl(String midletClassName,
            MIDletSuiteImpl midletSuite) {
        super(midletSuite);

        this.midletClassName = midletClassName;

        if (suiteName == null) {
            suiteName = midletClassName;
        }

        // no support for internal suites with more than one MIDlet yet
        if (totalMIDlets > 1) {
            totalMIDlets = 1;
        }
    }


    /**
     * Tests if this suite is external
     *
     * @return true, if this suite is external
     */
    boolean isExternalSuite() {
        return false;
    }

    /**
     * Gets suite ID
     *
     * @return suite ID as String
     */
    int getSuiteID() {
        return INTERNAL_SUITE_ID;
    }

    /**
     * Updates list of suite's MIDlets
     */
    void updateMIDletsList() {
        suiteMIDlets = new Vector(totalMIDlets);

        AutoMIDletDescriptorImpl midlet = null;
        midlet = new AutoMIDletDescriptorImpl(this, suiteName,
                midletClassName);
        suiteMIDlets.addElement(midlet);
    }
}
