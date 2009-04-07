/*
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

package com.sun.midp.jump.installer;

import com.sun.midp.security.SecurityHandler;
import com.sun.midp.security.SecurityToken;
import com.sun.midp.security.Permissions;

import com.sun.midp.midlet.MIDletSuite;

import com.sun.midp.midletsuite.MIDletSuiteStorage;
import com.sun.midp.midletsuite.MIDletSuiteImpl;
import com.sun.midp.midletsuite.MIDletInfo;
import com.sun.midp.midletsuite.MIDletSuiteInfo;
import com.sun.midp.midletsuite.InstallInfo;
import com.sun.midp.midletsuite.SuiteSettings;
import com.sun.midp.midletsuite.MIDletSuiteLockedException;
import com.sun.midp.midletsuite.MIDletSuiteCorruptedException;

import com.sun.midp.io.j2me.storage.RandomAccessStream;
import com.sun.midp.io.j2me.storage.File;

//import com.sun.midp.content.CHManager;

import com.sun.midp.installer.InvalidJadException;
import com.sun.midp.installer.ManifestProperties;
import com.sun.midp.installer.VerifierImpl;
import com.sun.midp.installer.InstallListener;
import com.sun.midp.installer.InstallState;
import com.sun.midp.installer.JadProperties;

/**
 * Holds the state of an installation, so it can restarted after it has
 * been stopped.
 */
public class InstallStateImpl implements InstallState, MIDletSuite {

    /** Location for the downloaded JAD, could be null */
    public String localJadUrl;

    /** Location for the downloaded JAR */
    public String localJarUrl;

    /** Contains the data obtained during the installation process */
    public InstallInfo installInfo;

    /** Contains the data obtained during the installation process */
    public SuiteSettings suiteSettings;

    /** ID of the storage where the new midlet suite will be installed. */
    public int storageId;

    /** Receives warnings and status. */
    public InstallListener listener;

    /** When the install started, in milliseconds. */
    public long startTime;

    /** What to do next. */
    public int nextStep;

    /** Signals the installation to stop. */
    public boolean stopInstallation;

    /**
     * Signals that installation is at a point where cancel
     * requests are ignored
     */
    public boolean ignoreCancel;

    /** exception that stopped the installation. */
    public InvalidJadException exception;

    /**
     * Option to force an overwrite of existing components without
     * any version comparison.
     */
    public boolean force;

    /**
     * Option to force the RMS data of the suite to be overwritten to
     * be removed without comparison to the new suite.
     */
    public boolean removeRMS;

    /** Raw JAD. */
    public byte[] jad;

    /** character encoding of the JAD. */
    public String jadEncoding;

    /** Parsed JAD. */
    public JadProperties jadProps;

    /** Parsed manifest. */
    public ManifestProperties jarProps;

    /** Cached File object. */
    public File file;

    /** User name for authentication. */
    protected String username;

    /** Password for authentication. */
    protected String password;

    /** User name for proxyAuthentication. */
    public String proxyUsername;

    /** Password for proxy authentication. */
    public String proxyPassword;

    /** Status to signal the beginning of the data transfer. */
    public int beginTransferDataStatus;

    /** Status for the data transfer method to give to the listener. */
    public int transferStatus;

    /** Security Handler. */
    public SecurityHandler securityHandler;

    /** Holds the unzipped JAR manifest to be saved. */
    public byte[] manifest;

    /** Cache of storage object. */
    public RandomAccessStream storage;

    /** Cache of MIDlet suite storage object. */
    public MIDletSuiteStorage midletSuiteStorage;

    /** The root of all MIDP persistent system data. */
    public String storageRoot;

    /** Signals that previous version exists. */
    public boolean isPreviousVersion;

    /** Previous MIDlet suite info. */
    public MIDletSuiteImpl previousSuite;

    /** Previous MIDlet suite install info. */
    public InstallInfo previousInstallInfo;

    /** The ContentHandler installer state. */
    // public CHManager chmanager;

    /** Constructor. */
    public InstallStateImpl() {
        installInfo   = new InstallInfo(UNUSED_SUITE_ID);
        suiteSettings = new SuiteSettings(UNUSED_SUITE_ID);
    }

    /**
     * Gets the last recoverable exception that stopped the install.
     * Non-recoverable exceptions are thrown and not saved in the state.
     *
     * @return last exception that stopped the install
     */
    public InvalidJadException getLastException() {
        return exception;
    }

    /**
     * Gets the unique ID that the installed suite was stored with.
     *
     * @return storage name that can be used to load the suite
     */
    public int getID() {
        return installInfo.id;
    }

    /**
     * Sets the username to be used for HTTP authentication.
     *
     * @param theUsername 8 bit username, cannot contain a ":"
     */
    public void setUsername(String theUsername) {
        username = theUsername;
    }

    /**
     * Sets the password to be used for HTTP authentication.
     *
     * @param thePassword 8 bit password
     */
    public void setPassword(String thePassword) {
        password = thePassword;
    }

    /**
     * Sets the username to be used for HTTP proxy authentication.
     *
     * @param theUsername 8 bit username, cannot contain a ":"
     */
    public void setProxyUsername(String theUsername) {
        proxyUsername = theUsername;
    }

    /**
     * Sets the password to be used for HTTP proxy authentication.
     *
     * @param thePassword 8 bit password
     */
    public void setProxyPassword(String thePassword) {
        proxyPassword = thePassword;
    }

    /**
     * Gets a property of the application to be installed.
     * First from the jaD, then if not found, the JAR manifeSt.
     *
     * @param key key of the property
     *
     * @return value of the property or null if not found
     */
    public String getAppProperty(String key) {
        String value;

        if (jadProps != null) {
            value = jadProps.getProperty(key);
            if (value != null) {
                return value;
            }
        }

        if (jarProps != null) {
            value = jarProps.getProperty(key);
            if (value != null) {
                return value;
            }
        }

        return null;
    }

    /**
     * Gets the URL of the JAR.
     *
     * @return URL of the JAR
     */
    public String getJarUrl() {
        return installInfo.jarUrl;
    }

    /**
     * Gets the label for the downloaded JAR.
     *
     * @return suite name
     */
    public String getSuiteName() {
        return installInfo.suiteName;
    }

    /**
     * Gets the expected size of the JAR.
     *
     * @return size of the JAR in K bytes rounded up
     */
    public int getJarSize() {
        return (installInfo.expectedJarSize + 1023) / 1024;
    }

    /**
     * Gets the authorization path of this suite. The path starts with
     * the most trusted CA that authorized this suite.
     *
     * @return array of CA names or null if the suite was not signed
     */
    public String[] getAuthPath() {
        /*
         * The auth path returned is no a copy because this object is
         * only available to callers with the AMS permission, which
         * have permission to build auth paths for new suites.
         */
        return installInfo.getAuthPath();
    }

    /**
     * Checks for permission and throw an exception if not allowed.
     * May block to ask the user a question.
     *
     * @param permission name of the permission to check
     * @param resource string to insert into the question, can be null if
     *        no %2 in the question
     *
     * @exception SecurityException if the permission is not
     *            allowed by this token
     * @exception InterruptedException if another thread interrupts the
     *   calling thread while this method is waiting to preempt the
     *   display.
     */
    public void checkForPermission(String permission, String resource)
            throws InterruptedException {
        checkForPermission(permission, resource, null);
    }

    /**
     * Checks for permission and throw an exception if not allowed.
     * May block to ask the user a question.
     *
     * @param permissionStr the permission name to check for,
     * @param resource string to insert into the question, can be null if
     *        no %2 in the question
     * @param extraValue string to insert into the question,
     *        can be null if no %3 in the question
     *
     * @exception SecurityException if the permission is not
     *            allowed by this token
     * @exception InterruptedException if another thread interrupts the
     *   calling thread while this method is waiting to preempt the
     *   display.
     */
    public void checkForPermission(String permissionStr, String resource,
            String extraValue) throws InterruptedException {

		int permission = Permissions.getId(permissionStr);
        securityHandler.checkForPermission(permissionStr,
            Permissions.getTitle(permission),
            Permissions.getQuestion(permission),
            Permissions.getOneshotQuestion(permission),
            installInfo.suiteName, resource, extraValue,
            permissionStr);
    }

    /**
     * Indicates if the named MIDlet is registered in the suite
     * with MIDlet-&lt;n&gt; record in the manifest or
     * application descriptor.
     * @param midletName class name of the MIDlet to be checked
     *
     * @return true if the MIDlet is registered
     */
    public boolean isRegistered(String midletName) {
        String midlet;
        MIDletInfo midletInfo;

        for (int i = 1; ; i++) {
            midlet = getAppProperty("MIDlet-" + i);
            if (midlet == null) {
                return false; // We went past the last MIDlet
            }

            /* Check if the names match. */
            midletInfo = new MIDletInfo(midlet);
            if (midletInfo.classname.equals(midletName)) {
                return true;
            }
        }
    }

    /**
     * Counts the number of MIDlets from its properties.
     * IMPL_NOTE: refactor to avoid duplication with MIDletSuiteImpl.
     *
     * @return number of midlet in the suite
     */
    public int getNumberOfMIDlets() {
        int i;

        for (i = 1; getProperty("MIDlet-" + i) != null; i++);

        return (i-1);
    }

    /**
     * Returns the suite's name to display to the user.
     *
     * @return suite's name that will be displayed to the user
     */
    public String getDisplayName() {
        String displayName = getAppProperty(MIDletSuite.SUITE_NAME_PROP);

        if (displayName == null) {
            displayName = String.valueOf(installInfo.id);
        }

        return displayName;
    }


    /**
     * Returns the information about the first midlet in the suite.
     *
     * @return MIDletInfo structure describing the first midlet
     * or null if it is not available
     */
    public MIDletInfo getMidletInfo() {
        String midlet;

        midlet = getAppProperty("MIDlet-1");
        if (midlet == null) {
            return null;
        }

        return new MIDletInfo(midlet);
    }

    /**
     * Indicates if this suite is trusted.
     * (not to be confused with a domain named "trusted",
     * this is used to determine if a trusted symbol should be displayed
     * to the user and not used for permissions)
     *
     * @return true if the suite is trusted false if not
     */
    public boolean isTrusted() {
        return installInfo.trusted;
    }


    /**
     * Check if the suite classes were successfully verified
     * during the suite installation.
     *
     * @return true if the suite classes are verified, false otherwise
     */
    public boolean isVerified() {
        return installInfo.verifyHash != null;
    }

    /**
     * Gets a property of the suite. A property is an attribute from
     * either the application descriptor or JAR Manifest.
     *
     * @param key the name of the property
     * @return A string with the value of the property.
     *    <code>null</code> is returned if no value
     *          is available for the key.
     */
    public String getProperty(String key) {
        return getAppProperty(key);
    }

    /**
     * Gets push setting for interrupting other MIDlets.
     * Reuses the Permissions.
     *
     * @return push setting for interrupting MIDlets the value
     *        will be permission level from {@link Permissions}
     */
    public byte getPushInterruptSetting() {
        return suiteSettings.getPushInterruptSetting();
    }

    /**
     * Gets push options for this suite.
     *
     * @return push options are defined in {@link PushRegistryImpl}
     */
    public int getPushOptions() {
        return suiteSettings.getPushOptions();
    }

    /**
     * Gets list of permissions for this suite.
     *
     * @return array of permissions from {@link Permissions}
     */
    public byte[] getPermissions() {
        return suiteSettings.getPermissions();
    }

    /**
     * Replace or add a property to the suite for this run only.
     *
     * @param token token with the AMS permission set to allowed
     * @param key the name of the property
     * @param value the value of the property
     *
     * @exception SecurityException if the caller's token does not have
     *            internal AMS permission
     */
    public void setTempProperty(SecurityToken token, String key,
                                String value) {
        throw new RuntimeException("Not Implemented");
    }

    /**
     * Get the name of a MIDlet.
     *
     * @param classname classname of a MIDlet in the suite
     *
     * @return name of a MIDlet to show the user
     */
    public String getMIDletName(String classname) {
        throw new RuntimeException("Not Implemented");
    }

    /**
     * Gets the MIDlet number as specified in the MIDlet-n entry in the JAD
     * file based on the MIDlet's class name.
     *
     * @param className class name of the MIDlet to be checked
     * @return the MIDlet's number, or 0 if it cannot be determined.
     */
    public int getMIDletNumber(String className) {
        throw new RuntimeException("Not Implemented");
    }

    /**
     * Checks to see the suite has the ALLOW level for specific permission.
     * This is used for by internal APIs that only provide access to
     * trusted system applications.
     * <p>
     * Only trust this method if the object has been obtained from the
     * MIDletStateHandler of the suite.
     *
     * @param permission permission name
     *
     * @exception SecurityException if the suite is not
     *            allowed to perform the specified action
     */
    public void checkIfPermissionAllowed(String permission) {
        throw new RuntimeException("Not Implemented");
    }

    /**
     * Gets the status of the specified permission.
     * If no API on the device defines the specific permission
     * requested then it must be reported as denied.
     * If the status of the permission is not known because it might
     * require a user interaction then it should be reported as unknown.
     *
     * @param permission to check if denied, allowed, or unknown
     * @return 0 if the permission is denied; 1 if the permission is
     *    allowed; -1 if the status is unknown
     */
    public int checkPermission(String permission) {
        throw new RuntimeException("Not Implemented");
    }

    /**
     * Saves any the settings (security or others) that the user may have
     * changed. Normally called by the scheduler after
     * the last running MIDlet in the suite is destroyed.
     * However it could be call during a suspend of the VM so
     * that persistent settings of the suite can be preserved.
     */
    public void saveSettings() {
        throw new RuntimeException("Not Implemented");
    }

    /**
     * Asks the user want to interrupt the current MIDlet with
     * a new MIDlet that has received network data.
     *
     * @param connection connection to place in the permission question or
     *        null for alarm
     *
     * @return true if the use wants interrupt the current MIDlet,
     * else false
     */
    public boolean permissionToInterrupt(String connection) {
        throw new RuntimeException("Not Implemented");
    }

    /**
     * Determine if the a MIDlet from this suite can be run. Note that
     * disable suites can still have their settings changed and their
     * install info displayed.
     *
     * @return true if suite is enabled, false otherwise
     */
    public boolean isEnabled() {
        throw new RuntimeException("Not Implemented");
    }

    /**
     * Gets a secure filename base (including path separator if needed)
     * for the suite. File build with the base will be automatically deleted
     * when the suite is removed.
     *
     * @return secure filename base for this suite
     */
    public String getSecureFilenameBase() {
        throw new RuntimeException("Not Implemented");
    }

    /**
     * Close the opened MIDletSuite
     */
    public void close() {
    }
}
