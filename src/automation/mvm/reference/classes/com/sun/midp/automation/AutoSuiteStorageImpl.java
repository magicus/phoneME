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

import java.io.*;
import java.util.*;
import com.sun.midp.midletsuite.*;
import com.sun.midp.installer.*;
import com.sun.midp.configurator.Constants;

/**
 * AutoSuiteStorage interface implementation.
 */
final class AutoSuiteStorageImpl implements AutoSuiteStorage {
    /** Suites storage */
    private MIDletSuiteStorage storage;

    /** Suites installer */
    private Installer installer;

    /** Vector containing decriptors of installed suites */
    private Vector installedSuites;

    /** The one and only class instance */
    private static AutoSuiteStorageImpl instance = null;


    /**
     * Private constructor: this class is singleton
     */
    private AutoSuiteStorageImpl() {
        storage = MIDletSuiteStorage.getMIDletSuiteStorage();
        installedSuites = new Vector();
    }

    /**
     * Updates vector of installed suites
     */
    private void updateInstalledSuites() {
        int[] suiteIDs;

        suiteIDs = storage.getListOfSuites();
        installedSuites = new Vector(suiteIDs.length);

        for (int i = 0; i < suiteIDs.length; i++) {
            AutoSuiteDescriptor suite;
            try {
                suite = AutoSuiteDescriptorImpl.getInstanceBySuiteID(
                        suiteIDs[i], storage);
                installedSuites.addElement(suite);
            } catch (Exception e) {
                // ignore and move to next suite
            }
        }
    }


    /**
     * Gets class instance
     *
     * @return single AutoSuiteStorage instance
     */
    synchronized static AutoSuiteStorageImpl getInstance() {
        if (instance == null) {
            instance = new AutoSuiteStorageImpl();
        }

        return instance;
    }


    /**
     * AutoSuiteStorage implementation
     */

    /**
     * Installs suite from specified location.
     *
     * @param location URI pointing to suite jad/jar
     *
     * @return AutoSuiteDescriptor representing installed suit
     */
    public AutoSuiteDescriptor installSuite(String location)
        throws IOException, MIDletSuiteLockedException,
               MIDletSuiteCorruptedException, InvalidJadException,
               SecurityException {

        AutoSuiteDescriptor suite = null;
        int suiteID;

        installer = AutoGetInstallerTunnel.getInstaller(location);
        suiteID = installer.installJad(location, Constants.INTERNAL_STORAGE_ID,
            true, true, null);
        suite = AutoSuiteDescriptorImpl.getInstanceBySuiteID(suiteID,
                storage);

        return suite;
    }

    /**
     * Uninstalls specified suite.
     *
     * @param suite suite to uninstall
     */
    public void uninstallSuite(AutoSuiteDescriptor suite)
        throws MIDletSuiteLockedException, IllegalArgumentException {

        AutoSuiteDescriptorImpl suiteImpl = (AutoSuiteDescriptorImpl)suite;
        suiteImpl.guaranteeSuiteValid("uninstallSuite");

        if (suiteImpl.isExternalSuite()) {
            int suiteID = suiteImpl.getSuiteID();
            storage.remove(suiteID);
        }

        /** uninstalling suite makes it invalid */
        suiteImpl.invalidate();
    }

    /**
     * Gets a vector of installed suites.
     *
     * @return vector with suites descriptors
     */
    public Vector getInstalledSuites() {
        updateInstalledSuites();

        /** copy vector of installed suites into new vector */
        int totalSuites = installedSuites.size();
        Vector suites = new Vector(totalSuites);

        for (int i = 0; i < totalSuites; i++) {
            Object o = installedSuites.elementAt(i);
            suites.addElement(o);
        }

        return suites;
    }
}
