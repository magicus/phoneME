/*
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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

import java.util.*;
import java.io.*;
import com.sun.midp.midletsuite.*;
import com.sun.midp.security.*;
import com.sun.midp.installer.*;
import com.sun.midp.i18n.*;

/**
 *  Base class for AutoSuiteDescriptor interface implementation.
 */
abstract class AutoSuiteDescriptorImpl
    extends AutoSuiteDescriptor {

    /** Internal representation of MIDlet suite */
    protected MIDletSuiteImpl midletSuite;

    /** Vector containing descriptors of suite's MIDlets */
    protected Vector suiteMIDlets = null;

    /** Number of MIDlets in the suite */
    protected int totalMIDlets;

    /** Name of the suite */
    protected String suiteName;

    /** Flag indicating whether this suite valid or not */
    protected boolean isValid = false;


    /**
     * Constructor
     *
     * @param midletSuite internal representation of MIDlet suite
     */
    protected AutoSuiteDescriptorImpl(MIDletSuiteImpl midletSuite) {
        this.midletSuite = midletSuite;
        this.isValid = true;
        this.totalMIDlets = midletSuite.getNumberOfMIDlets();
        this.suiteName = midletSuite.getProperty(
                MIDletSuiteImpl.SUITE_NAME_PROP);

        updateMIDletsList();
    }


    /**
     * Invalidates suite
     */
    final void invalidate() {
        midletSuite = null;
        suiteMIDlets = null;
        totalMIDlets = 0;

        isValid = false;
    }

    /**
     * Guarantees suite validness: if suite is not valid,
     * exception is thrown
     *
     * @param s error string
     */
    final void guaranteeSuiteValid(String s)
        throws IllegalStateException {

        if (!isValid) {
            throw new IllegalStateException(s);
        }
    }

    /**
     * Tests if this suite is external
     *
     * @return true, if this suite is internal
     */
    abstract boolean isExternalSuite();

    /**
     * Gets suite ID
     *
     * @return suite ID as String
     */
    abstract int getSuiteID();

    /**
     * Updates list of suite's MIDlets
     */
    abstract void updateMIDletsList();


    /**
     * Factory method: constructs suite descriptor from suite ID
     *
     * @param suiteID ID ot the suite
     * @param storage suite's storage
     * @return suite descriptor
     */
    final static AutoSuiteDescriptor getInstanceBySuiteID(int suiteID,
            MIDletSuiteStorage storage)
        throws MIDletSuiteLockedException, MIDletSuiteCorruptedException {

        AutoSuiteDescriptor suite = null;
        MIDletSuiteImpl suiteImpl = null;
        try {
            // get internal suite representation from storage
            suiteImpl = storage.getMIDletSuite(suiteID, false);
            if (suiteImpl == null) {
                throw new IllegalArgumentException("Invalid suite ID");
            }
            suite = new AutoExternalSuiteDescriptorImpl(suiteID, suiteImpl);
        } finally {
            if (suiteImpl != null) {
                suiteImpl.close();
            }
        }

        return suite;
    }

    /**
     * Factory method: constructs suite descriptor from MIDlet's class name
     *
     * @param className class name of internal MIDlet
     * @return suite decriptor
     */
    final static AutoSuiteDescriptor getInstanceByClassName(String className)
        throws IOException {

        AutoSuiteDescriptor suite = null;
        MIDletSuiteImpl suiteImpl = null;
        MIDletSuiteStorage storage = MIDletSuiteStorage.getMIDletSuiteStorage();
        try {
            // create internal suite representation
            suiteImpl = (MIDletSuiteImpl)InternalMIDletSuiteImpl.create(storage,
                className, AutoInternalSuiteDescriptorImpl.INTERNAL_SUITE_ID);

            suite = new AutoInternalSuiteDescriptorImpl(className, suiteImpl);
        } finally {
            if (suiteImpl != null) {
                suiteImpl.close();
            }
        }

        return suite;
    }



    /**
     * AutoSuiteDescriptor implementation
     */

    /**
     * Gets name of the suite.
     *
     * @return name of the suite as specified in jar/jad
     */
    public String getSuiteName() {
        guaranteeSuiteValid("getSuiteName");
        return suiteName;
    }

    /**
     * Gets MIDlet which should be started by default for this
     * suite, if any.
     *
     * @return AutoMIDletDescriptor representing default
     * MIDlet
     */
    public AutoMIDletDescriptor getInitialMIDlet() {
        guaranteeSuiteValid("getDefaultMIDlet");
        return (AutoMIDletDescriptor)suiteMIDlets.elementAt(0);
    }

    /**
     * Gets suite's MIDlets.
     *
     * @return vector of AutoMIDletDescriptor objects representing
     * suite's MIDlets,
     * null if there is no default MIDlet for this suite
     */
    public Vector getSuiteMIDlets() {
        guaranteeSuiteValid("getSuiteMIDlets");

        // copy suite's MIDlets vector into another
        Vector midlets = new Vector(totalMIDlets);

        for (int i = 0; i < totalMIDlets; i++) {
            Object o = suiteMIDlets.elementAt(i);
            midlets.addElement(o);
        }

        return midlets;
    }

    /**
     * Starts this suite's initial MIDlet.
     *
     * @param args MIDlet's arguments
     * @return AutoMIDlet representing started MIDlet
     */
    public AutoMIDlet start(String[] args) {
        AutoMIDletDescriptor midlet = getInitialMIDlet();
        return midlet.start(args);
    }
}
