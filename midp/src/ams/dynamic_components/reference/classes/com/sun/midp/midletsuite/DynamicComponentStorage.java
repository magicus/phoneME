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

package com.sun.midp.midletsuite;

import java.io.IOException;

import com.sun.midp.util.Properties;

import com.sun.midp.midlet.MIDletSuite;

import com.sun.midp.amsservices.ComponentInfo;
import com.sun.midp.amsservices.ComponentInfoImpl;

import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;
import com.sun.midp.security.Permissions;
import com.sun.j2me.security.AccessController;

/**
 * Storage for Dynamically Loaded Components.
 *
 * This functionality is optional, the stubbed out implementation
 * is used until built with USE_DYNAMIC_COMPONENTS=true.
 */
public class DynamicComponentStorage {

    /** Holds an instance of DynamicComponentStorage. */
    private static DynamicComponentStorage componentStorage = null;

    /**
     * Private constructor to prevent direct instantiations.
     */
    private DynamicComponentStorage() {
    }

    /**
     * Returns a reference to the singleton MIDlet suite storage object.
     * <p>
     * Method requires the com.sun.midp.ams permission.
     *
     * @return the storage reference
     *
     * @exception SecurityException if the caller does not have permission
     *   to install software
     */
    public static DynamicComponentStorage getComponentStorage()
            throws SecurityException {
        AccessController.checkPermission(Permissions.AMS_PERMISSION_NAME);

        if (componentStorage == null) {
            componentStorage = new DynamicComponentStorage();
        }

        return componentStorage;
    }

    /**
     * Gets the unique identifier of MIDlet suite's dynamic component.
     *
     * @param suiteId ID of the suite the component belongs to
     * @param vendor name of the vendor that created the component, as
     *        given in a JAD file
     * @param name name of the component, as given in a JAD file
     *
     * @return ID of the midlet suite's component given by vendor and name
     *         or ComponentInfo.UNUSED_COMPONENT_ID if the component does
     *         not exist
     */
    public native int getComponentId(int suiteId, String vendor, String name);

    /**
     * Returns a unique identifier of a dynamic component.
     *
     * @return platform-specific id of the component
     */
    public native int createComponentId();

    /**
     * Stores or updates a midlet suite's dynamic component.
     *
     * @param suiteStorage suite storage used to store the component
     *
     * @param installInfo structure containing the following information:<br>
     * <pre>
     *     id - unique ID of the suite;
     *     jadUrl - where the JAD came from, can be null;
     *     jarUrl - where the JAR came from;
     *     jarFilename - name of the downloaded component's jar file;
     *     suiteName - name of the component;
     *     suiteVendor - vendor of the component;
     *     suiteVersion - version of the component;
     *     authPath - authPath if signed, the authorization path starting
     *                with the most trusted authority;
     *     domain - security domain of the component (currently unused);
     *     trusted - true if component is trusted;
     *     verifyHash - may contain hash value of the component with
     *                  preverified classes or may be NULL;
     * </pre>
     *
     * @param suiteSettings structure containing the following information:<br>
     * <pre>
     *     permissions - permissions for the component (currently unused);
     *     pushInterruptSetting - defines if this MIDlet component interrupt
     *                            other components;
     *     pushOptions - user options for push interrupts;
     *     suiteId - unique ID of the suite, must be equal to the one given
     *               in installInfo;
     *     boolean enabled - if true, MIDlet from this suite can be run;
     * </pre>
     *
     * @param displayName name of the component to display to user
     *
     * @param jadProps properties the JAD as an array of strings in
     *        key/value pair order, can be null if jadUrl is null
     *
     * @param jarProps properties of the manifest as an array of strings
     *        in key/value pair order
     *
     * @exception IOException is thrown, if an I/O error occurs during
     * storing the component
     * @exception MIDletSuiteLockedException is thrown, if the component is
     * locked
     */
    public synchronized void storeComponent(
            MIDletSuiteStorage suiteStorage, InstallInfo installInfo,
                SuiteSettings suiteSettings, String displayName,
                    Properties jadProps, Properties jarProps)
                        throws IOException, MIDletSuiteLockedException {
        ComponentInfoImpl ci = new ComponentInfoImpl(
                installInfo.componentId, installInfo.id, displayName,
                installInfo.suiteVersion, installInfo.trusted);

        /*
         * Convert the property args to String arrays to save
         * creating the native KNI code to access the object.
         */
        String[] strJadProperties =
                MIDletSuiteStorage.getPropertiesStrings(jadProps);
        String[] strJarProperties =
                MIDletSuiteStorage.getPropertiesStrings(jarProps);

        suiteStorage.nativeStoreSuite(installInfo, suiteSettings, null, ci,
            strJadProperties, strJarProperties);
    }

    /**
     * Removes a dynamic component given its ID.
     * <p>
     * If the component is in use it must continue to be available
     * to the other components that are using it.
     *
     * @param componentId ID of the component ot remove
     *
     * @throws IllegalArgumentException if the component cannot be found
     * @throws MIDletSuiteLockedException is thrown, if the component is
     *                                    locked
     */
    public native void removeComponent(int componentId)
            throws IllegalArgumentException, MIDletSuiteLockedException;

    /**
     * Removes all dynamic components belonging to the given suite.
     * <p>
     * If any component is in use, no components are removed, and
     * an exception is thrown.
     *
     * @param suiteId ID of the suite whose components must be removed
     *
     * @throws IllegalArgumentException if there is no suite with
     *                                  the specified ID
     * @throws MIDletSuiteLockedException is thrown, if any component is
     *                                    locked
     */
    public native void removeAllComponents(int suiteId)
            throws IllegalArgumentException, MIDletSuiteLockedException;

    /**
     * Get the midlet suite component's class path including a path to the MONET
     * image of the specified component and a path to the suite's jar file.
     *
     * @param componentId unique ID of the dynamic component
     *
     * @return class path or null if the component does not exist
     */
    public synchronized String[] getComponentClassPath(int componentId) {
        String jarFile = getComponentJarPath(componentId);

        /*
            IMPL_NOTE: currently MONET is not supported for dynamic components

            if (Constants.MONET_ENABLED) {
                String bunFile = getSuiteComponentAppImagePath(componentId);
                return new String[]{bunFile, jarFile};
            }
        */

        return new String[] {jarFile};
    }

    /**
     * Returns a list of all components belonging to the given midlet suite.
     *
     * @param suiteId ID of a MIDlet suite
     *
     * @return an array of ComponentInfoImpl structures filled with the
     *         information about the installed components, or null
     *         if there are no components belonging to the given suite
     *
     * @exception IllegalArgumentException if the given suite id is invalid
     * @exception SecurityException if the caller does not have permission
     *                              to access this API
     */
    public synchronized ComponentInfo[] getListOfSuiteComponents(int suiteId)
            throws IllegalArgumentException, SecurityException {
        int n = getNumberOfComponents(suiteId);
        if (n < 0) {
            if (Logging.REPORT_LEVEL <= Logging.ERROR) {
                Logging.report(Logging.ERROR, LogChannels.LC_AMS,
                    "Error in getNumberOfComponents(): returned " + n);
            }
            return null;
        }

        ComponentInfo[] components = null;
        if (n > 0) {
            components = new ComponentInfo[n];

            for (int i = 0; i < n; i++) {
                components[i] = new ComponentInfoImpl();
            }

            try {
                getSuiteComponentsList(suiteId, components);
            } catch (Exception e) {
                if (Logging.REPORT_LEVEL <= Logging.ERROR) {
                    Logging.report(Logging.ERROR, LogChannels.LC_AMS,
                        "Error in getSuiteComponentsList(): " + e.getMessage());
                }

                components = null;
            }
        }

        return components;
    }

    /**
     * Reads information about the installed midlet suite's components
     * from the storage.
     *
     * @param componentId unique ID of the component
     * @param ci ComponentInfo object to fill with the information about
     *           the midlet suite's component having the given ID
     *
     * @exception java.io.IOException if an the information cannot be read
     * @exception IllegalArgumentException if suiteId is invalid or ci is null
     */
    public native void getComponentInfo(int componentId,
        ComponentInfo ci) throws IOException, IllegalArgumentException;

    /**
     * Get the class path for the specified dynamic component.
     *
     * @param componentId unique ID of the component
     *
     * @return class path or null if the component does not exist
     */
    public native String getComponentJarPath(int componentId);

    /**
     * Reads information about the installed midlet suite's components
     * from the storage.
     *
     * @param suiteId unique ID of the suite
     * @param ci array of ComponentInfo objects to fill with the information
     *           about the installed midlet suite's components
     *
     * @exception IOException if an the information cannot be read
     * @exception IllegalArgumentException if suiteId is invalid or ci is null
     */
    private native void getSuiteComponentsList(int suiteId,
        ComponentInfo[] ci) throws IOException, IllegalArgumentException;

    /**
     * Returns the number of the installed components belonging to the given
     * MIDlet suite.
     *
     * @param suiteId ID of the MIDlet suite the information about whose
     *                components must be retrieved
     *
     * @return the number of components belonging to the given suite
     *         or -1 in case of error
     */
    private native int getNumberOfComponents(int suiteId);
}
