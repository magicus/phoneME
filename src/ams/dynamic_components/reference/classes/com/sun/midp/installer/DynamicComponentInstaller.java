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
import com.sun.midp.midletsuite.DynamicComponentStorage;

import com.sun.midp.amsservices.ComponentInfo;
import com.sun.midp.amsservices.ComponentInfoImpl;

import com.sun.midp.configurator.Constants;
import com.sun.midp.util.Properties;

/**
 * Installer for dynamic components.
 */
public class DynamicComponentInstaller extends HttpInstaller {
    /** Dynamic Component property for the component name. */
    private static final String COMPONENT_NAME_PROP = "Component-Name";

    /** Dynamic Component property for the component vendor. */
    private static final String COMPONENT_VENDOR_PROP = "Component-Vendor";

    /** Dynamic Component property for the component version. */
    private static final String COMPONENT_VERSION_PROP = "Component-Version";

    /** Dynamic components storage */
    private DynamicComponentStorage componentStorage;

    /**
     * Constructor of the DynamicComponentInstaller.
     */
    public DynamicComponentInstaller() {
        super();
        componentStorage = DynamicComponentStorage.getComponentStorage();
    }

    /**
     * Installs the component pointed by the given URL.
     *
     * @param suiteId ID of the suite that owns the component being installed
     * @param url HTTP URL pointing to the application descriptor
     *            or to the jar file of the component that must
     *            be installed
     * @param name user-friendly name of the component
     *
     * @return unique component identifier
     *
     * @throws IllegalArgumentException if a suite with the given ID
     *                                  was not found
     * @throws java.io.IOException if the installation failed
     * @throws InvalidJadException if the downloaded JAD or JAR is invalid
     * @throws com.sun.midp.midletsuite.MIDletSuiteLockedException is thrown,
     *            if the MIDletSuite is locked
     * @throws SecurityException if the caller does not have permission
     *            to install components
     */
    public int installComponent(int suiteId, String url, String name)
            throws IOException, MIDletSuiteLockedException,
                InvalidJadException, SecurityException {
        int componentId;

        info.id = suiteId;
        info.isSuiteComponent = true;
        info.displayName = name;

        try {
            componentId = installJad(url, Constants.INTERNAL_STORAGE_ID,
                                     true, true, null);
        } catch (InvalidJadException ije) {
            int reason = ije.getReason();
            if (reason != InvalidJadException.INVALID_JAD_TYPE) {
                throw ije;
            }

            // media type of JAD was wrong, it could be a JAR
            String mediaType = ije.getExtraData();

            if (Installer.JAR_MT_1.equals(mediaType) ||
                Installer.JAR_MT_2.equals(mediaType)) {
                // re-run as a JAR only install
                componentId = installJar(url, name,
                    Constants.INTERNAL_STORAGE_ID, true, true, null);
            } else {
                throw ije;
            }
        }

        return componentId;
    }

    /**
     * Checks that all necessary attributes are present in JAD or JAR
     * and are valid.
     *
     * @param props properties to check
     *
     * @throws InvalidJadException if any mandatory attribute is missing in
     *                             the JAD file or its value is invalid
     */
    private void checkAttributesImpl(Properties props)
            throws InvalidJadException {
        info.suiteName = props.getProperty(COMPONENT_NAME_PROP);
        if (info.suiteName == null || info.suiteName.length() == 0) {
            throw new
                InvalidJadException(InvalidJadException.MISSING_SUITE_NAME);
        }

        // if a user-friendly name was not given, use the name from the property
        if (info.displayName == null || info.displayName.length() == 0) {
            info.displayName = info.suiteName;
        }

        info.suiteVendor = props.getProperty(COMPONENT_VENDOR_PROP);
        if (info.suiteVendor == null || info.suiteVendor.length() == 0) {
            throw new
                InvalidJadException(InvalidJadException.MISSING_VENDOR);
        }

        info.suiteVersion = props.getProperty(COMPONENT_VERSION_PROP);
        if (info.suiteVersion == null || info.suiteVersion.length() == 0) {
            throw new
                InvalidJadException(InvalidJadException.MISSING_VERSION);
        }

        try {
            checkVersionFormat(info.suiteVersion);
        } catch (NumberFormatException nfe) {
            throw new InvalidJadException(InvalidJadException.INVALID_VERSION);
        }
    }

    /**
     * Checks that all necessary attributes are present in JAD and are valid.
     *
     * May be overloaded by subclasses that require presence of
     * different attributes in JAD during the installation.
     *
     * @throws InvalidJadException if any mandatory attribute is missing in
     *                             the JAD file or its value is invalid
     */
    protected void checkJadAttributes() throws InvalidJadException {
        checkAttributesImpl(state.jadProps);
    }

    /**
     * Checks that all necessary attributes are present in the manifest
     * in the JAR file and are valid.
     *
     * May be overloaded by subclasses that require presence of
     * different attributes in manifest during the installation.
     *
     * @throws InvalidJadException if any mandatory attribute is missing in
     *                             the manifest or its value is invalid
     */
    protected void checkJarAttributes() throws InvalidJadException {
        checkAttributesImpl(state.jarProps);
    }

    /**
     * Assigns a new ID to the dynamic component being installed.
     */
    protected void assignNewId() {
        info.componentId = componentStorage.createComponentId();
    }

    /**
     * Stores the dynamic component being installed in the component storage.
     *
     * @throws IOException if an I/O error occured when storing the suite
     * @throws MIDletSuiteLockedException if the suite is locked
     */
    protected void storeUnit() throws IOException, MIDletSuiteLockedException {
        componentStorage.storeComponent(state.midletSuiteStorage, info,
            settings, state.getDisplayName(), state.jadProps, state.jarProps);
    }

    /**
     * See if there is an installed version of the component being installed
     * and if so, make an necessary checks. Will set state fields, including
     * the exception field for warning the user.
     *
     * @exception InvalidJadException if the new version is formated
     * incorrectly
     * @exception MIDletSuiteLockedException is thrown, if the MIDletSuite is
     * locked
     */
    protected void checkPreviousVersion()
            throws InvalidJadException, MIDletSuiteLockedException {
        int id;
        DynamicComponentStorage dcs =
                DynamicComponentStorage.getComponentStorage();

        id = dcs.getComponentId(info.id, info.suiteVendor, info.suiteName);
        if (id == ComponentInfo.UNUSED_COMPONENT_ID) {
            // there is no previous version
            return;
        }

        checkVersionFormat(info.suiteVersion);
        info.componentId = id;

        // If this component is already installed, check version information
        ComponentInfo ci = new ComponentInfoImpl();
        try {
            dcs.getComponentInfo(id, ci);
        } catch (Exception e) {
            // ignore and force an overwrite
            return;
        }

        String installedVersion = ci.getVersion();
        int cmpResult = vercmp(info.suiteVersion, installedVersion);

        if (cmpResult >= 0) {
            // the same or newer version, overwrite silently
            return;
        }

        // older version, stop the installation
        throw new InvalidJadException(InvalidJadException.NEW_VERSION,
                                      installedVersion);
    }
}
