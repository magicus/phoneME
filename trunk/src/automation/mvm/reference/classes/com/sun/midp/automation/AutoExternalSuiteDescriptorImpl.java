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
import java.util.*;

/**
 *  AutoSuiteDescriptor interface implementation for external suite.
 */
class AutoExternalSuiteDescriptorImpl
    extends AutoSuiteDescriptorImpl {

    /** suite ID */
    private int suiteID;


    /**
     * Constructor
     *
     * @param suiteID suite ID
     * @param midletSuite internal MIDlet suite representation
     */
    AutoExternalSuiteDescriptorImpl(int suiteID,
            MIDletSuiteImpl midletSuite) {
        super(midletSuite);

        this.suiteID = suiteID;
        if (suiteName == null) {
            suiteName = String.valueOf(suiteID);
        }
    }

    /**
     * Tests if this suite is external
     *
     * @return true, if this suite is external
     */
    boolean isExternalSuite() {
        return true;
    }

    /**
     * Gets suite ID
     *
     * @return suite ID as String
     */
    int getSuiteID() {
        return suiteID;
    }

    /**
     * Updates list of suite's MIDlets
     */
    void updateMIDletsList() {
        suiteMIDlets = new Vector(totalMIDlets);

        for (int i = 1; i <= totalMIDlets; i++) {
            AutoMIDletDescriptorImpl midlet = null;
            String midletAttr = midletSuite.getProperty("MIDlet-" + i);
            midlet = new AutoMIDletDescriptorImpl(this, midletAttr);

            suiteMIDlets.addElement(midlet);
        }
    }
}
