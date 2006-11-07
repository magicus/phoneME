/*
 *
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

package com.sun.midp.installer;

import java.util.Vector;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.ByteArrayOutputStream;
import java.io.ByteArrayInputStream;

import java.lang.String;
import java.lang.IllegalArgumentException;

import javax.microedition.io.Connector;
import javax.microedition.io.Connection;
import javax.microedition.io.HttpConnection;
import javax.microedition.io.ConnectionNotFoundException;

import com.sun.midp.security.SecurityHandler;
import com.sun.midp.security.SecurityToken;
import com.sun.midp.security.Permissions;

import com.sun.midp.main.MIDletSuiteVerifier;
import com.sun.midp.main.MIDletAppImageGenerator;

import com.sun.midp.midlet.MIDletStateHandler;
import com.sun.midp.midlet.MIDletSuite;

import com.sun.midp.midletsuite.MIDletSuiteStorage;
import com.sun.midp.midletsuite.MIDletSuiteImpl;
import com.sun.midp.midletsuite.MIDletInfo;
import com.sun.midp.midletsuite.InstallInfo;

import com.sun.midp.io.Base64;
import com.sun.midp.io.HttpUrl;
import com.sun.midp.io.Util;

import com.sun.midp.io.j2me.push.PushRegistryImpl;

import com.sun.midp.io.j2me.storage.RandomAccessStream;
import com.sun.midp.io.j2me.storage.File;

import com.sun.midp.rms.RecordStoreFactory;

import com.sun.midp.content.CHManager;

import com.sun.midp.midletsuite.MIDletSuiteLockedException;
import com.sun.midp.midletsuite.MIDletSuiteCorruptedException;

import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;
import com.sun.midp.configurator.Constants;

/**
 * An Installer manages MIDlet suites and libraries
 * present in a Java application environment.  An MIDlet suite
 * distributed as a descriptor and JAR pair.
 * The descriptor identifies the configuration and contains security
 * information and the manifest of the JAR describes the contents.
 * The implementation of an Installer is
 * specific to the platform and provides access to
 * procedures that make an MIDlet suite visible to users.
 * <P>
 * Each installed package is uniquely identified by a storage name
 * constructed from the combination
 * of the values of the <code>MIDlet-Name</code> and
 * <code>MIDlet-Vendor</code> attributes.
 * The syntax and content of the strings used to identify
 * installed packages are implementation dependent.
 * Only packages installed or upgraded using this API appear
 * in the list of known packages.
 *
 */
public class Installer {
    /** Status code to signal connection to the JAD server was successful. */
    public static final int DOWNLOADING_JAD = 1;

    /** Status code to signal that another 1K of the JAD has been download. */
    public static final int DOWNLOADED_1K_OF_JAD = 2;

    /** Status code to signal connection to the JAR server was successful. */
    public static final int DOWNLOADING_JAR = 3;

    /** Status code to signal that another 1K of the JAR has been download. */
    public static final int DOWNLOADED_1K_OF_JAR = 4;

    /**
     * Status code to signal that download is done and the suite is being
     * verified.
     */
    public static final int VERIFYING_SUITE = 5;

    /**
     * Status code to signal that application image is being generating.
     */
    public static final int GENERATING_APP_IMAGE = 6;

    /**
     * Status code for local writing of the verified MIDlet suite.
     * Stopping the install at this point has no effect, so there user
     * should not be given a chance to stop the install.
     */
    public static final int STORING_SUITE = 7;

    /** Status code for corrupted suite */
    public static final int CORRUPTED_SUITE = 8;

    /** Filename of Manifest inside the application archive. */
    public static final String JAR_MANIFEST       = "META-INF/MANIFEST.MF";

    /** MIDlet property for the size of the application data. */
    public static final String DATA_SIZE_PROP     = "MIDlet-Data-Size";

    /** MIDlet property for the size of the application archive. */
    public static final String JAR_SIZE_PROP      = "MIDlet-Jar-Size";

    /** MIDlet property for the application archive URL. */
    public static final String JAR_URL_PROP       = "MIDlet-Jar-URL";

    /** MIDlet property for the suite name. */
    public static final String SUITE_NAME_PROP    = "MIDlet-Name";

    /** MIDlet property for the suite vendor. */
    public static final String VENDOR_PROP        = "MIDlet-Vendor";

    /** MIDlet property for the suite version. */
    public static final String VERSION_PROP       = "MIDlet-Version";

    /** MIDlet property for the suite description. */
    public static final String DESC_PROP        = "MIDlet-Description";

    /** MIDlet property for the microedition configuration. */
    public static final String CONFIGURATION_PROP =
        "MicroEdition-Configuration";

    /** MIDlet property for the profile. */
    public static final String PROFILE_PROP       = "MicroEdition-Profile";

    /** MIDlet property for the required permissions. */
    public static final String PERMISSIONS_PROP     = "MIDlet-Permissions";

    /** MIDlet property for the optional permissions. */
    public static final String PERMISSIONS_OPT_PROP = "MIDlet-Permissions-Opt";

    /** Media-Type for valid application descriptor files. */
    public static final String JAD_MT = "text/vnd.sun.j2me.app-descriptor";

    /** Media-Type for valid Jar file. */
    public static final String JAR_MT_1 = "application/java";

    /** Media-Type for valid Jar file. */
    public static final String JAR_MT_2 = "application/java-archive";

    /** Max number of bytes to download at one time (1K). */
    private static final int MAX_DL_SIZE = 1024;

    /** Tag that signals that the HTTP server supports basic auth. */
    private static final String BASIC_TAG = "basic";

    /**
     * Filename to save the JAR of the suite temporarily. This is used
     * to avoid overwriting an existing JAR prior to
     * verification.
     */
    static final String TMP_FILENAME      = "installer.tmp";

    /** Private reference to the singleton Installer class. */
    private static Installer myInstaller;

    /** HTTP connection to close when we stop the installation. */
    private HttpConnection httpConnection;

    /** HTTP stream to close when we stop the installation. */
    private InputStream httpInputStream;

    /** Holds the install state. */
    protected InstallStateImpl state;

    /** Holds the CLDC configuration string. */
    private String cldcConfig;

    /** Holds the MIDP supported profiles. */
    private Vector supportedProfiles;

    /** Use this to be the security domain for unsigned suites. */
    private String unsignedSecurityDomain =
        Permissions.UNIDENTIFIED_DOMAIN_BINDING;

    /**
     * Get an instance of an Installer. If the SecureInstaller class is
     * available then it will be returned else a basic Installer will be
     * returned.
     *
     * @return an Installer instance
     */
    protected static Installer getInstaller() {
        try {
            return (Installer)Class.forName(
                "com.sun.midp.installer.SecureInstaller").newInstance();
        } catch (Throwable t) {
            return new Installer();
        }
    }

    /**
     * Constructor of the Installer.
     */
    protected Installer() {
    }

    /**
     * Installs a software package from the given URL. The URL is assumed
     * refer to an application descriptor.
     * <p>
     * If the component to be installed is the same as an existing
     * component (by comparing the <code>MIDlet-Name</code>,
     * <code>MIDlet-Vendor</code> attributes)
     * then this install is an upgrade if the version number is greater
     * than the current version.  If so, the new version replaces in its
     * entirety the current version.
     * <p>
     * It is implementation dependent when the upgraded component is
     * made available for use.
     * <p>
     * The implementation of install must be robust in the presence
     * of failures such as running out of memory.  If this method
     * throws an exception then the package must <em>not</em> be installed
     * and any previous version of the component must be left intact
     * and operational.
     * <p>
     * To receive status updates and installer warnings, provide an install
     * listener. If no listener is provided all warnings will be thrown
     * as exceptions.
     *
     * @param location the URL from which the application descriptor can be
     *        updated
     * @param force if <code>true</code> the MIDlet suite components to be
     *              installed will overwrite any existing components without
     *              any version comparison
     * @param removeRMS if <code>true</code> and existing RMS data will be
     *              removed when overwriting an existing suite
     * @param installListener object to receive status updates and install
     *     warnings, can be null
     *
     * @return the unique ID of the installed package.
     *
     * @exception ConnectionNotFoundException if JAD URL is invalid
     * @exception IOException is thrown if any error prevents the installation
     *   of the MIDlet suite, including being unable to access the application
     *   descriptor or JAR
     * @exception InvalidJadException if the downloaded application descriptor
     *   is invalid
     * @exception MIDletSuiteLockedException is thrown, if the MIDletSuite is
     * locked
     * @exception SecurityException if the caller does not have permission
     *   to install software
     * @exception IllegalArgumentException is thrown, if the location of the
     * descriptor file is not specified
     */
    public String installJad(String location, boolean force, boolean removeRMS,
            InstallListener installListener)
            throws IOException, InvalidJadException,
                   MIDletSuiteLockedException, SecurityException {

        state = new InstallStateImpl();

        state.jadUrl = location;
        state.force = force;
        state.removeRMS = removeRMS;
        state.nextStep = 1;
        state.listener = installListener;
        state.chmanager = CHManager.getManager(null);

        return performInstall();
    }

    /**
     * Installs a software package from the given URL. The URL is assumed
     * refer to a JAR.
     * <p>
     * If the component to be installed is the same as an existing
     * component (by comparing the <code>MIDlet-Name</code>,
     * <code>MIDlet-Vendor</code> attributes)
     * then this install is an upgrade if the version number is greater
     * than the current version.  If so, the new version replaces in its
     * entirety the current version.
     * <p>
     * It is implementation dependent when the upgraded component is
     * made available for use.
     * <p>
     * The implementation of install must be robust in the presence
     * of failures such as running out of memory.  If this method
     * throws an exception then the package must <em>not</em> be installed
     * and any previous version of the component must be left intact
     * and operational.
     * <p>
     * To receive status updates and installer warnings, provide an install
     * listener. If no listener is provided all warnings will be thrown
     * as exceptions.
     *
     * @param location the URL from which the JAR can be updated
     * @param name the name of the suite to be updated
     * @param force if <code>true</code> the MIDlet suite components to be
     *              installed will overwrite any existing components without
     *              any version comparison
     * @param removeRMS if <code>true</code> and existing RMS data will be
     *              removed when overwriting an existing suite
     * @param installListener object to receive status updates and install
     *     warnings, can be null
     *
     * @return the unique ID of the installed package.
     *
     * @exception IOException is thrown if any error prevents the installation
     *   of the MIDlet suite, including being unable to access the JAR
     * @exception InvalidJadException if the downloaded JAR is invalid
     * @exception MIDletSuiteLockedException is thrown, if the MIDletSuite is
     * locked
     * @exception SecurityException if the caller does not have permission
     *   to install software
     * @exception IllegalArgumentException is thrown, if the location of the
     * JAR specified
     */
    public String installJar(String location, String name, boolean force,
           boolean removeRMS, InstallListener installListener)
            throws IOException, InvalidJadException,
                   MIDletSuiteLockedException {

        if (location == null || location.length() == 0) {
            throw
                new IllegalArgumentException("Must specify URL of .jar file");
        }

        state = new InstallStateImpl();

        state.jarUrl = location;
        state.suiteName = name;
        state.force = force;
        state.removeRMS = removeRMS;
        state.file = new File();
        state.nextStep = 5;
        state.listener = installListener;
        state.chmanager = CHManager.getManager(null);

        return performInstall();
    }

    /**
     * Performs an install.
     *
     * @return the unique name of the installed package
     *
     * @exception IOException is thrown, if an I/O error occurs during
     * descriptor or jar file download
     * @exception InvalidJadException is thrown, if the descriptor file is not
     * properly formatted or does not contain the required
     * information
     * @exception MIDletSuiteLockedException is thrown, if the MIDletSuite is
     * locked
     * @exception IllegalArgumentException is thrown, if the
     * descriptor file is not specified
     */
    private String performInstall()
            throws IOException, InvalidJadException,
                   MIDletSuiteLockedException {

        state.midletSuiteStorage = MIDletSuiteStorage.getMIDletSuiteStorage();

        /* Disable push interruptions during install. */
        PushRegistryImpl.enablePushLaunch(false);

        try {
            state.startTime = System.currentTimeMillis();

            while (state.nextStep < 8) {
                /*
                 * clear the previous warning, so we can tell if another has
                 * happened
                 */
                state.exception = null;

                if (state.stopInstallation) {
                    postInstallMsgBackToProvider(
                        OtaNotifier.USER_CANCELLED_MSG);
                    throw new IOException("stopped");
                }

                switch (state.nextStep) {
                case 1:
                    installStep1();
                    break;

                case 2:
                    installStep2();
                    break;

                case 3:
                    installStep3();
                    break;

                case 4:
                    installStep4();
                    break;

                case 5:
                    installStep5();
                    break;

                case 6:
                    installStep6();
                    break;

                case 7:
                    installStep7();
                    break;

                default:
                    // for safety/completeness.
                    Logging.report(Logging.CRITICAL, LogChannels.LC_AMS,
                        "Installer: Unknown step: " + state.nextStep);
                    break;
                }

                if (state.exception != null) {
                    if (state.listener == null) {
                        throw state.exception;
                    }

                    if (!state.listener.warnUser(state)) {
                        state.stopInstallation = true;
                        postInstallMsgBackToProvider(
                            OtaNotifier.USER_CANCELLED_MSG);
                        throw state.exception;
                    }
                }
            }
        } finally {
            if (state.previousSuite != null) {
                state.previousSuite.close();
            }
            if (state.tempFilename != null) {
                if (state.file.exists(state.tempFilename)) {
                    try {
                        state.file.delete(state.tempFilename);
                    } catch (Exception e) {
                        if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                            Logging.report(Logging.WARNING, LogChannels.LC_AMS,
                            "delete file  threw an Exception");
                        }
                    }
                }
            }

            PushRegistryImpl.enablePushLaunch(true);
        }

        return state.id;
    }

    /**
     * Downloads the JAD, save it in the install state.
     * Parse the JAD, make sure it has
     * the required properties, and save them in the install state.
     *
     * @exception IOException is thrown, if an I/O error occurs during
     * descriptor or jar file download
     * @exception InvalidJadException is thrown, if the descriptor file is not
     * properly formatted or does not contain the required attributes
     * @exception MIDletSuiteLockedException is thrown, if the MIDletSuite is
     * locked
     * @exception IllegalArgumentException is thrown, if the
     * descriptor file is not specified
     */
    private void installStep1()
        throws IOException, InvalidJadException, MIDletSuiteLockedException {

        if (state.jadUrl == null || state.jadUrl.length() == 0) {
            throw
                new IllegalArgumentException("Must specify URL of .jad file");
        }

        try {
            state.jad = downloadJAD();
        } catch (OutOfMemoryError e) {
            try {
                postInstallMsgBackToProvider(
                    OtaNotifier.INSUFFICIENT_MEM_MSG);
            } catch (Throwable t) {
                if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                    Logging.report(Logging.WARNING, LogChannels.LC_AMS,
                    "Throwable during posting install message");
                }
            }

            throw new
                InvalidJadException(InvalidJadException.TOO_MANY_PROPS);
        }

        if (state.exception != null) {
            return;
        }

        state.jadProps = new JadProperties();
        try {
            state.jadProps.load(new ByteArrayInputStream(state.jad),
                                state.jadEncoding);
        } catch (OutOfMemoryError e) {
            state.jad = null;
            try {
                postInstallMsgBackToProvider(
                    OtaNotifier.INSUFFICIENT_MEM_MSG);
            } catch (Throwable t) {
                if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                    Logging.report(Logging.WARNING, LogChannels.LC_AMS,
                    "Throwable during posting install message");
                }
            }

            throw new
                InvalidJadException(InvalidJadException.TOO_MANY_PROPS);
        } catch (InvalidJadException ije) {
            state.jad = null;
            postInstallMsgBackToProvider(OtaNotifier.INVALID_JAD_MSG);
            throw ije;
        }

        state.suiteName = state.jadProps.getProperty(SUITE_NAME_PROP);
        if (state.suiteName == null || state.suiteName.length() == 0) {
            postInstallMsgBackToProvider(OtaNotifier.INVALID_JAD_MSG);
            throw new
                InvalidJadException(InvalidJadException.MISSING_SUITE_NAME);
        }

        state.vendor = state.jadProps.getProperty(VENDOR_PROP);
        if (state.vendor == null || state.vendor.length() == 0) {
            postInstallMsgBackToProvider(OtaNotifier.INVALID_JAD_MSG);
            throw new
                InvalidJadException(InvalidJadException.MISSING_VENDOR);
        }

        state.version = state.jadProps.getProperty(VERSION_PROP);
        if (state.version == null || state.version.length() == 0) {
            postInstallMsgBackToProvider(OtaNotifier.INVALID_JAD_MSG);
            throw new
                InvalidJadException(InvalidJadException.MISSING_VERSION);
        }

        try {
            checkVersionFormat(state.version);
        } catch (NumberFormatException nfe) {
            postInstallMsgBackToProvider(OtaNotifier.INVALID_JAD_MSG);
            throw new InvalidJadException(
                  InvalidJadException.INVALID_VERSION);
        }

        state.id = state.midletSuiteStorage.createSuiteID(state.vendor,
                   state.suiteName);

        checkPreviousVersion();
        state.nextStep++;
    }

    /**
     * If the JAD belongs to an installed suite, check the URL against the
     * installed one.
     */
    private void installStep2() {
        state.nextStep++;

        if (state.isPreviousVersion) {
            checkForDifferentDomains(state.jadUrl);
        }
    }

    /**
     * Makes sure the suite can fit in storage.
     *
     * @exception IOException is thrown, if an I/O error occurs during
     * descriptor or jar file download
     * @exception InvalidJadException is thrown, if the descriptor file is not
     * properly formatted or does not contain the required
     */
    private void installStep3()
            throws IOException, InvalidJadException {
        String sizeString;
        int dataSize;
        int suiteSize;

        sizeString = state.jadProps.getProperty(JAR_SIZE_PROP);
        if (sizeString == null) {
            postInstallMsgBackToProvider(OtaNotifier.INVALID_JAD_MSG);
            throw new
                InvalidJadException(InvalidJadException.MISSING_JAR_SIZE);
        }

        try {
            state.expectedJarSize = Integer.parseInt(sizeString);
        } catch (NumberFormatException e) {
            postInstallMsgBackToProvider(OtaNotifier.INVALID_JAD_MSG);
            throw new
                InvalidJadException(InvalidJadException.INVALID_VALUE);
        }

        sizeString = state.jadProps.getProperty(DATA_SIZE_PROP);
        if (sizeString == null) {
            dataSize = 0;
        } else {
            try {
                dataSize = Integer.parseInt(sizeString);
            } catch (NumberFormatException e) {
                postInstallMsgBackToProvider(
                    OtaNotifier.INVALID_JAD_MSG);
                throw new
                    InvalidJadException(InvalidJadException.INVALID_VALUE);
            }
        }

        /*
         * A suite is a jad + jar + manifest + url + data size.
         * lets say the manifest is the same size as the jad
         * since we do know at this point. the size is in bytes,
         * UTF-8 chars can be upto 3 bytes
         */
        suiteSize = state.expectedJarSize + (state.jad.length * 2) +
                    (state.jadUrl.length() * 3) + dataSize;
        state.jad = null;

        state.file = new File();

        if (suiteSize > state.file.getBytesAvailableForFiles()) {
            postInstallMsgBackToProvider(
                OtaNotifier.INSUFFICIENT_MEM_MSG);

            // the size reported to the user should be in K and rounded up
            throw new
                InvalidJadException(InvalidJadException.INSUFFICIENT_STORAGE,
                    Integer.toString((suiteSize + 1023)/ 1024));
        }

        state.jarUrl = state.jadProps.getProperty(JAR_URL_PROP);
        if (state.jarUrl == null || state.jarUrl.length() == 0) {
            postInstallMsgBackToProvider(OtaNotifier.INVALID_JAD_MSG);
            throw new
                InvalidJadException(InvalidJadException.MISSING_JAR_URL);
        }

        state.nextStep++;
    }

    /**
     * Confirm installation with the user.
     *
     * @exception IOException is thrown, if the user cancels installation
     */
    private void installStep4()
            throws IOException {

        synchronized (state) {
            /* One more check to see if user has already canceled */
            if (state.stopInstallation) {
                postInstallMsgBackToProvider(
                    OtaNotifier.USER_CANCELLED_MSG);
                throw new IOException("stopped");
            }
            /*
             * Not canceled, so ignore cancel requests for now because below we
             * are going to ask anyway if user wants to install suite
             */
            state.ignoreCancel = true;
        }

        if (state.listener != null &&
            !state.listener.confirmJarDownload(state)) {
            state.stopInstallation = true;
            postInstallMsgBackToProvider(
                OtaNotifier.USER_CANCELLED_MSG);
            throw new IOException("stopped");
        }

        synchronized (state) {
            /* Allow cancel requests again */
            state.ignoreCancel = false;
        }
        state.nextStep++;
    }

    /**
     * Downloads the JAR, make sure it is the correct size, make sure
     * the required attributes match the JAD's. Then store the
     * application.
     *
     * @exception IOException is thrown, if an I/O error occurs during
     * descriptor or jar file download
     * @exception InvalidJadException is thrown, if the descriptor file is not
     * properly formatted or does not contain the required
     */
    private void installStep5()
            throws IOException, InvalidJadException {
        int bytesDownloaded;
        MIDletInfo midletInfo;
        String midlet;

        // Send out delete notifications that have been queued, first
        OtaNotifier.postQueuedDeleteMsgsBackToProvider(state.proxyUsername,
            state.proxyPassword);

        // Save jar file to temp name; we need to do this to read the
        // manifest entry, but, don't want to overwrite an existing
        // application in case there are problems with the manifest
        state.storageRoot = File.getStorageRoot();
        state.tempFilename = state.storageRoot + TMP_FILENAME;

        bytesDownloaded = downloadJAR(state.tempFilename);

        if (state.exception != null) {
            return;
        }

        try {
            state.storage = new RandomAccessStream();

            verifyJar(state.storage, state.tempFilename);

            if (state.listener != null) {
                state.listener.updateStatus(VERIFYING_SUITE, state);
            }

            // Preverify all suite classes and if successful
            // return the hash value of the suite package
            if (Constants.VERIFY_ONCE && !Constants.MONET_ENABLED) {
                state.verifyHash =
                    MIDletSuiteVerifier.verifyJarClasses(state.tempFilename);
                if (state.verifyHash == null) {
                    throw new InvalidJadException(
                        InvalidJadException.JAR_CLASSES_VERIFICATION_FAILED,
                        JAR_MANIFEST);
                }
            } else {
                state.verifyHash = null;
            }

            // Create JAR Properties (From .jar file's MANIFEST)
            try {
                state.manifest = JarReader.readJarEntry(state.tempFilename,
                                                        JAR_MANIFEST);
                if (state.manifest == null) {
                    postInstallMsgBackToProvider(
                        OtaNotifier.INVALID_JAR_MSG);
                    throw new
                        InvalidJadException(InvalidJadException.CORRUPT_JAR,
                                            JAR_MANIFEST);
                }
            } catch (OutOfMemoryError e) {
                try {
                    postInstallMsgBackToProvider(
                        OtaNotifier.INSUFFICIENT_MEM_MSG);
                } catch (Throwable t) {
                    if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                        Logging.report(Logging.WARNING, LogChannels.LC_AMS,
                        "Throwable during posting the install message");
                    }
                }

                throw new
                    InvalidJadException(InvalidJadException.TOO_MANY_PROPS);
            } catch (IOException ioe) {
                postInstallMsgBackToProvider(
                    OtaNotifier.INVALID_JAR_MSG);
                throw new
                    InvalidJadException(InvalidJadException.CORRUPT_JAR,
                                        JAR_MANIFEST);
            }

            state.jarProps = new ManifestProperties();

            try {
                state.jarProps.load(new ByteArrayInputStream(state.manifest));
                state.manifest = null;
            } catch (OutOfMemoryError e) {
                state.manifest = null;
                try {
                    postInstallMsgBackToProvider(
                        OtaNotifier.INSUFFICIENT_MEM_MSG);
                } catch (Throwable t) {
                    if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                        Logging.report(Logging.WARNING, LogChannels.LC_AMS,
                        "Throwable while posting install message ");
                    }
                }

                throw new
                    InvalidJadException(InvalidJadException.TOO_MANY_PROPS);
            } catch (InvalidJadException ije) {
                state.manifest = null;

                try {
                    postInstallMsgBackToProvider(
                        OtaNotifier.INVALID_JAR_MSG);
                } catch (Throwable t) {
                    // ignore
                }

                throw ije;
            }

            for (int i = 1; ; i++) {
                midlet = state.getAppProperty("MIDlet-" + i);
                if (midlet == null) {
                    break;
                }

                /*
                 * Verify the MIDlet class is present in the JAR
                 * An exception thrown if not.
                 * Do the proper install notify on an exception
                 */
                try {
                    midletInfo = new MIDletInfo(midlet);

                    verifyMIDlet(midletInfo.classname);
                } catch (InvalidJadException ije) {
                    if (ije.getReason() == InvalidJadException.INVALID_VALUE) {
                        postInstallMsgBackToProvider(
                            OtaNotifier.INVALID_JAD_MSG);
                    } else {
                        postInstallMsgBackToProvider(
                            OtaNotifier.INVALID_JAR_MSG);
                    }
                    throw ije;
                }
            }

            // move on to the next step after a warning
            state.nextStep++;

            // Check Manifest entries against .jad file
            if (state.jadUrl != null) {
                if (bytesDownloaded != state.expectedJarSize) {
                    postInstallMsgBackToProvider(
                        OtaNotifier.JAR_SIZE_MISMATCH_MSG);
                    throw new  InvalidJadException(
                        InvalidJadException.JAR_SIZE_MISMATCH);
                }

                if (!state.suiteName.equals(state.jarProps.getProperty(
                        SUITE_NAME_PROP))) {
                    postInstallMsgBackToProvider(
                        OtaNotifier.ATTRIBUTE_MISMATCH_MSG);
                    throw new InvalidJadException(
                        InvalidJadException.SUITE_NAME_MISMATCH);
                }

                if (!state.version.equals(
                        state.jarProps.getProperty(VERSION_PROP))) {
                    postInstallMsgBackToProvider(
                        OtaNotifier.ATTRIBUTE_MISMATCH_MSG);
                    throw new InvalidJadException(
                         InvalidJadException.VERSION_MISMATCH);
                }

                if (!state.vendor.equals(
                        state.jarProps.getProperty(VENDOR_PROP))) {
                    postInstallMsgBackToProvider(
                        OtaNotifier.ATTRIBUTE_MISMATCH_MSG);
                    throw new InvalidJadException(
                         InvalidJadException.VENDOR_MISMATCH);
                }
            } else {
                state.suiteName = state.jarProps.getProperty(
                                  SUITE_NAME_PROP);
                if (state.suiteName == null ||
                    state.suiteName.length() == 0) {
                    postInstallMsgBackToProvider(
                        OtaNotifier.INVALID_JAR_MSG);
                    throw new InvalidJadException(
                         InvalidJadException.MISSING_SUITE_NAME);
                }

                state.vendor = state.jarProps.getProperty(VENDOR_PROP);
                if (state.vendor == null || state.vendor.length() == 0) {
                    postInstallMsgBackToProvider(
                        OtaNotifier.INVALID_JAR_MSG);
                    throw new InvalidJadException(
                         InvalidJadException.MISSING_VENDOR);
                }

                state.version = state.jarProps.getProperty(VERSION_PROP);
                if (state.version == null || state.version.length() == 0) {
                    postInstallMsgBackToProvider(
                        OtaNotifier.INVALID_JAR_MSG);
                    throw new InvalidJadException(
                         InvalidJadException.MISSING_VERSION);
                }

                try {
                    checkVersionFormat(state.version);
                } catch (NumberFormatException nfe) {
                    postInstallMsgBackToProvider(
                        OtaNotifier.INVALID_JAR_MSG);
                    throw new InvalidJadException(
                         InvalidJadException.INVALID_VERSION);
                }

                // need revisit if already installed, check the domain of the JAR URL
                
                state.id = state.midletSuiteStorage.createSuiteID(state.vendor,
                                                    state.suiteName);

                checkPreviousVersion();
            }
        } catch (Exception e) {
            state.file.delete(state.tempFilename);

            if (e instanceof IOException) {
                throw (IOException)e;
            }

            throw (RuntimeException)e;
        }
    }

    /**
     * If the JAR belongs to an installed suite if there was
     * no JAD, check the URL against the installed one.
     */
    private void installStep6() {
        state.nextStep++;

        if (state.jadUrl == null && state.isPreviousVersion) {
            checkForDifferentDomains(state.jarUrl);
        }
    }

    /**
     * Checks the permissions and store the suite.
     *
     * @exception IOException is thrown, if an I/O error occurs during
     * storing the suite
     * @exception InvalidJadException is thrown, if the there is
     * permission problem
     */
    private void installStep7() throws IOException, InvalidJadException {

        try {
            if (state.authPath != null) {
                // suite was signed
                state.domain = getSecurityDomainName(state.storageRoot,
                                                     state.authPath[0]);
                if (state.listener != null &&
                    !state.listener.confirmAuthPath(state)) {
                    state.stopInstallation = true;
                    postInstallMsgBackToProvider(
                        OtaNotifier.USER_CANCELLED_MSG);
                    throw new IOException("stopped");
                }
            } else {
                state.domain = unsignedSecurityDomain;
            }

            state.trusted = Permissions.isTrusted(state.domain);

            // Do not overwrite trusted suites with untrusted ones
            if ((!state.trusted) && state.isPreviousVersion &&
                    state.previousSuite.isTrusted()) {

                postInstallMsgBackToProvider(
                    OtaNotifier.AUTHORIZATION_FAILURE_MSG);

                throw new InvalidJadException(
                    InvalidJadException.TRUSTED_OVERWRITE_FAILURE,
                        state.previousInstallInfo.getAuthPath()[0]);
            }

            /*
             * The unidentified suites do not get checked for requested
             * permissions.
             */
            if (Permissions.UNIDENTIFIED_DOMAIN_BINDING.equals(state.domain)) {

                state.permissions = (Permissions.forDomain(state.domain))
                                        [Permissions.CUR_LEVELS];

                /*
                 * To keep public key management simple, there is only one
                 * trusted keystore. So it is possible that the CA for
                 * the suite is untrusted. This may be done on purpose for
                 * testing. This is OK, but do not confuse the user by saying
                 * the untrusted suite is authorized, so set the CA name to
                 * null.
                 */
                state.authPath = null;
            } else {
                /*
                 * For identified suites, make sure an properties duplicated in
                 * both the manifest and JAD are the same.
                 */
                if (state.jadUrl != null) {
                    checkForJadManifestMismatches();
                }

                state.permissions = getInitialPermissions(state.domain);
            }

            if (state.isPreviousVersion) {
                applyCurrentUserLevelPermissions(
                  state.previousInstallInfo.
                    getPermissions(),
                    (Permissions.forDomain(state.domain))
                        [Permissions.MAX_LEVELS],
                    state.permissions);

                if (state.removeRMS) {
                    // override suite comparisons, just remove RMS
                    RecordStoreFactory.removeRecordStoresForSuite(null,
                        state.id);
                } else {
                    processPreviousRMS();
                }
            }

            state.securityHandler = new SecurityHandler(state.permissions,
                                                        state.domain);

            checkConfiguration();
            matchProfile();

            try {
                state.chmanager.preInstall(this,
                       (InstallState)state,
                       (MIDletSuite)state,
                       (state.authPath == null ? null : state.authPath[0]));
            } catch (InvalidJadException jex) {
                // Post the correct install notify msg back to the server
                String msg = OtaNotifier.INVALID_CONTENT_HANDLER;
                if (jex.getReason() ==
                    InvalidJadException.CONTENT_HANDLER_CONFLICT) {
                    msg = OtaNotifier.CONTENT_HANDLER_CONFLICT;
                }

                postInstallMsgBackToProvider(msg);
                throw jex;
            } catch (SecurityException se) {
                postInstallMsgBackToProvider(
                    OtaNotifier.AUTHORIZATION_FAILURE_MSG);

                // since our state object put the permission in message
                throw new InvalidJadException(
                    InvalidJadException.AUTHORIZATION_FAILURE,
                    se.getMessage());
            }

            // make sure at least 1 second has passed
            try {
                long waitTime = 1000 -
                    (System.currentTimeMillis() - state.startTime);

                if (waitTime > 0) {
                    Thread.sleep(waitTime);
                }
            } catch (InterruptedException ie) {
                // ignore
            }

            synchronized (state) {
                // this is the point of no return, one last check
                if (state.stopInstallation) {
                    postInstallMsgBackToProvider(
                        OtaNotifier.USER_CANCELLED_MSG);
                    throw new IOException("stopped");
                }

                state.ignoreCancel = true;
            }

            /**
             * In case of installation from JAR the suite id as well as other 
             * properties are read from .jar file's MANIFEST rather then from 
             * JAD file. Only after that application image can be generated.
             */
            if (Constants.MONET_ENABLED) {
                if (state.listener != null) {
                    state.listener.updateStatus(GENERATING_APP_IMAGE, state);
                }
                MIDletAppImageGenerator.createAppImage(state.id,
                        state.tempFilename, state.midletSuiteStorage);
            }

            if (state.listener != null) {
                state.listener.updateStatus(STORING_SUITE, state);
            }
            
            registerPushConnections();

            /** Do the Content Handler registration updates now */
            state.chmanager.install();

            /*
             * Store suite will remove the suite including push connections,
             * if there an error, but may not remove the temp jar file.
             */
            state.midletSuiteStorage.storeSuite(state.id, state.jadUrl,
                state.jadProps, state.jarUrl, state.tempFilename,
                state.jarProps, state.authPath, state.domain, state.trusted,
                state.permissions, state.pushInterruptSetting,
                state.pushOptions, state.verifyHash);
        } catch (Throwable e) {
            state.file.delete(state.tempFilename);
            if (e instanceof IOException) {
                throw (IOException)e;
            }

            if (e instanceof OutOfMemoryError) {
                try {
                    postInstallMsgBackToProvider(
                        OtaNotifier.INSUFFICIENT_MEM_MSG);
                } catch (Throwable t) {
                    if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                        Logging.report(Logging.WARNING, LogChannels.LC_AMS,
                        "Throwable during posting install message");
                    }
                }

                throw new
                    InvalidJadException(InvalidJadException.TOO_MANY_PROPS);
            }

            throw (RuntimeException)e;
        }

        state.nextStep++;

        try {
            postInstallMsgBackToProvider(OtaNotifier.SUCCESS_MSG);
        } catch (Throwable t) {
            /*
             * The suite is successfully installed, but the post of the
             * status message failed. Do not let this failure prevent
             * the suite from being used.
             */
        }
    }

    /**
     * Verify that a class is present in the JAR file.
     * If the classname is invalid or is not found an
     * InvalidJadException is thrown.
     * @param classname the name of the class to verify
     * @exception InvalidJadException is thrown if the name is null or empty
     * or if the file is not found
     */
    public void verifyMIDlet(String classname)
        throws InvalidJadException
    {
        if (classname == null ||
            classname.length() == 0) {
            throw new
                InvalidJadException(InvalidJadException.INVALID_VALUE);
        }

        String file = classname.replace('.', '/').concat(".class");

        try {
            /* Attempt to read the MIDlet from the JAR file. */
            if (JarReader.readJarEntry(state.tempFilename, file) != null) {
                return;                // File found, normal return
            }
            // Fall into throwing the exception
        } catch (IOException ioe) {
            // Fall into throwing the exception
        }
        // Throw the InvalidJadException
        throw new InvalidJadException(InvalidJadException.CORRUPT_JAR, file);
    }

    /**
     * Downloads an application descriptor file from the given URL.
     *
     * @return a byte array representation of the file or null if not found
     *
     * @exception IOException is thrown if any error prevents the download
     *   of the JAD
     */
    private byte[] downloadJAD() throws IOException {
        String[] encoding = new String[1];
        ByteArrayOutputStream bos = new ByteArrayOutputStream(MAX_DL_SIZE);
        String[] acceptableTypes = {JAD_MT};
        String[] extraFieldKeys = new String[3];
        String[] extraFieldValues = new String[3];
        String locale;
        String prof = System.getProperty("microedition.profiles");
        int space = prof.indexOf(' ');
        if (space != -1) {
            prof = prof.substring(0, space);
        }

        extraFieldKeys[0] = "User-Agent";
        extraFieldValues[0] = "Profile/" + prof
                              + " Configuration/" +
                              System.getProperty("microedition.configuration");

        extraFieldKeys[1] = "Accept-Charset";
        extraFieldValues[1] = "UTF-8, ISO-8859-1";

        /* locale can be null */
        locale = System.getProperty("microedition.locale");
        if (locale != null) {
            extraFieldKeys[2] = "Accept-Language";
            extraFieldValues[2] = locale;
        }

        state.beginTransferDataStatus = DOWNLOADING_JAD;
        state.transferStatus = DOWNLOADED_1K_OF_JAD;

        /*
         * Do not send the list of acceptable types because some servers
         * will send a 406 if the URL is to a JAR. It is better to
         * reject the resource at the client after check the media-type so
         * if the type is JAR a JAR only install can be performed.
         */
        downloadResource(state.jadUrl, extraFieldKeys, extraFieldValues,
                         acceptableTypes, false, false, bos, encoding,
                         InvalidJadException.INVALID_JAD_URL,
                         InvalidJadException.JAD_SERVER_NOT_FOUND,
                         InvalidJadException.JAD_NOT_FOUND,
                         InvalidJadException.INVALID_JAD_TYPE);

        state.jadEncoding = encoding[0];
        return bos.toByteArray();
    }

    /**
     * Downloads an application archive file from the given URL into the
     * given file. Automatically handle re-tries.
     *
     * @param filename name of the file to write. This file resides
     *          in the storage area of the given application
     *
     * @return size of the JAR
     *
     * @exception IOException is thrown if any error prevents the download
     *   of the JAR
     */
    private int downloadJAR(String filename)
            throws IOException {
        HttpUrl parsedUrl;
        String url;
        String[] acceptableTypes = {JAR_MT_1, JAR_MT_2};
        String[] extraFieldKeys = new String[3];
        String[] extraFieldValues = new String[3];
        int jarSize;
        String locale;
        String prof;
        int space;
        RandomAccessStream jarOutputStream = null;
        OutputStream outputStream = null;

        parsedUrl = new HttpUrl(state.jarUrl);
        if (parsedUrl.authority == null && state.jadUrl != null) {
            // relative URL, add the JAD URL as the base
            try {
                parsedUrl.addBaseUrl(state.jadUrl);
            } catch (IOException e) {
                postInstallMsgBackToProvider(
                    OtaNotifier.INVALID_JAD_MSG);
                throw new InvalidJadException(
                         InvalidJadException.INVALID_JAR_URL);
            }

            url = parsedUrl.toString();

            // The JAR URL saved to storage MUST be absolute
            state.jarUrl = url;
        } else {
            url = state.jarUrl;
        }

        jarOutputStream = new RandomAccessStream();
        jarOutputStream.connect(filename,
                                RandomAccessStream.READ_WRITE_TRUNCATE);
        outputStream = jarOutputStream.openOutputStream();

        prof = System.getProperty("microedition.profiles");
        space = prof.indexOf(' ');
        if (space != -1) {
            prof = prof.substring(0, space);
        }

        extraFieldKeys[0] = "User-Agent";
        extraFieldValues[0] = "Profile/" + prof
                              + " Configuration/" +
                              System.getProperty("microedition.configuration");

        extraFieldKeys[1] = "Accept-Charset";
        extraFieldValues[1] = "UTF-8, ISO-8859-1";

        /* locale can be null */
        locale = System.getProperty("microedition.locale");
        if (locale != null) {
            extraFieldKeys[2] = "Accept-Language";
            extraFieldValues[2] = locale;
        }

        try {
            state.beginTransferDataStatus = DOWNLOADING_JAR;
            state.transferStatus = DOWNLOADED_1K_OF_JAR;
            jarSize = downloadResource(url, extraFieldKeys, extraFieldValues,
                         acceptableTypes, true, true, outputStream, null,
                         InvalidJadException.INVALID_JAR_URL,
                         InvalidJadException.JAR_SERVER_NOT_FOUND,
                         InvalidJadException.JAR_NOT_FOUND,
                         InvalidJadException.INVALID_JAR_TYPE);
            return jarSize;
        } catch (InvalidJadException ije) {
            switch (ije.getReason()) {
            case InvalidJadException.INVALID_JAR_URL:
            case InvalidJadException.JAR_SERVER_NOT_FOUND:
            case InvalidJadException.JAR_NOT_FOUND:
            case InvalidJadException.INVALID_JAR_TYPE:
                postInstallMsgBackToProvider(
                    OtaNotifier.INVALID_JAR_MSG);
                break;

            default:
                // for safety/completeness.
                if (Logging.REPORT_LEVEL <= Logging.ERROR) {
                    Logging.report(Logging.ERROR, LogChannels.LC_AMS,
                    "Installer InvalidJadException: " + ije.getMessage());
                }
                break;
            }

            throw ije;
        } finally {
            try {
                jarOutputStream.disconnect();
            } catch (Exception e) {
                 if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                     Logging.report(Logging.WARNING, LogChannels.LC_AMS,
                     "disconnect  threw a  Exception");
                 }
            }
        }
    }

    /**
     * Downloads an resource from the given URL into the output stream.
     *
     * @param url location of the resource to download
     * @param extraFieldKeys keys to the extra fields to put in the request
     * @param extraFieldValues values to the extra fields to put in the request
     * @param acceptableTypes list of acceptable media types for this resource,
     *                        there must be at least one
     * @param sendAcceptableTypes if true the list of acceptable media types
     *       for this resource will be sent in the request
     * @param allowNoMediaType if true it is not consider an error if
     *       the media type is not in the response
     *       for this resource will be sent in the request
     * @param output output stream to write the resource to
     * @param encoding an array to receive the character encoding of resource,
     *                 can be null
     * @param invalidURLCode reason code to use when the URL is invalid
     * @param serverNotFoundCode reason code to use when the server is not
     *     found
     * @param resourceNotFoundCode reason code to use when the resource is not
     *     found on the server
     * @param invalidMediaTypeCode reason code to use when the media type of
     *     the resource is not valid
     *
     * @return size of the resource
     *
     * @exception IOException is thrown if any error prevents the download
     *   of the resource
     */
    private int downloadResource(String url, String[] extraFieldKeys,
            String[] extraFieldValues, String[] acceptableTypes,
            boolean sendAcceptableTypes, boolean allowNoMediaType,
            OutputStream output, String[] encoding, int invalidURLCode,
            int serverNotFoundCode, int resourceNotFoundCode,
            int invalidMediaTypeCode) throws IOException {
        Connection conn = null;
        StringBuffer acceptField;
        int responseCode;
        String retryAfterField;
        int retryInterval;
        String mediaType;

        try {
            for (; ; ) {
                try {
                    conn = Connector.open(url, Connector.READ);
                } catch (IllegalArgumentException e) {
                    throw new InvalidJadException(invalidURLCode, url);
                } catch (ConnectionNotFoundException e) {
                    // protocol not found
                    throw new InvalidJadException(invalidURLCode, url);
                }

                if (!(conn instanceof HttpConnection)) {
                    // only HTTP or HTTPS are supported
                    throw new InvalidJadException(invalidURLCode, url);
                }

                httpConnection = (HttpConnection)conn;

                if (extraFieldKeys != null) {
                    for (int i = 0; i < extraFieldKeys.length &&
                                    extraFieldKeys[i] != null; i++) {
                        httpConnection.setRequestProperty(extraFieldKeys[i],
                                                          extraFieldValues[i]);
                    }
                }

                // 256 is given to avoid resizing without adding lengths
                acceptField = new StringBuffer(256);

                if (sendAcceptableTypes) {
                    // there must be one or more acceptable media types
                    acceptField.append(acceptableTypes[0]);
                    for (int i = 1; i < acceptableTypes.length; i++) {
                        acceptField.append(", ");
                        acceptField.append(acceptableTypes[i]);
                    }
                } else {
                    /* Send at least a wildcard to satisfy WAP gateways. */
                    acceptField.append("*/*");
                }

                httpConnection.setRequestProperty("Accept",
                                                  acceptField.toString());
                httpConnection.setRequestMethod(HttpConnection.GET);

                if (state.username != null &&
                    state.password != null) {
                    httpConnection.setRequestProperty("Authorization",
                        formatAuthCredentials(state.username,
                                              state.password));
                }

                if (state.proxyUsername != null &&
                    state.proxyPassword != null) {
                    httpConnection.setRequestProperty("Proxy-Authorization",
                        formatAuthCredentials(state.proxyUsername,
                                              state.proxyPassword));
                }

                try {
                    responseCode = httpConnection.getResponseCode();
                } catch (IOException ioe) {
                    if (httpConnection.getHost() == null) {
                        throw new InvalidJadException(invalidURLCode, url);
                    }

                    throw new InvalidJadException(serverNotFoundCode, url);
                }

                if (responseCode != HttpConnection.HTTP_UNAVAILABLE) {
                    break;
                }

                retryAfterField = httpConnection.getHeaderField("Retry-After");
                if (retryAfterField == null) {
                    break;
                }

                try {
                    /*
                     * see if the retry interval is in seconds, and
                     * not an absolute date
                     */
                    retryInterval = Integer.parseInt(retryAfterField);
                    if (retryInterval > 0) {
                        if (retryInterval > 60) {
                            // only wait 1 min
                            retryInterval = 60;
                        }

                        Thread.sleep(retryInterval * 1000);
                    }
                } catch (InterruptedException ie) {
                    // ignore thread interrupt
                    break;
                } catch (NumberFormatException ne) {
                    // ignore bad format
                    break;
                }

                httpConnection.close();

                if (state.stopInstallation) {
                    postInstallMsgBackToProvider(
                        OtaNotifier.USER_CANCELLED_MSG);
                    throw new IOException("stopped");
                }
            } // end for

            if (responseCode == HttpConnection.HTTP_NOT_FOUND) {
                throw new InvalidJadException(resourceNotFoundCode);
            }

            if (responseCode == HttpConnection.HTTP_NOT_ACCEPTABLE) {
                throw new InvalidJadException(invalidMediaTypeCode, "");
            }

            if (responseCode == HttpConnection.HTTP_UNAUTHORIZED) {
                // automatically throws the correct exception
                checkIfBasicAuthSupported(
                     httpConnection.getHeaderField("WWW-Authenticate"));

                state.exception =
                    new InvalidJadException(InvalidJadException.UNAUTHORIZED);
                return 0;
            }

            if (responseCode == HttpConnection.HTTP_PROXY_AUTH) {
                // automatically throws the correct exception
                checkIfBasicAuthSupported(
                     httpConnection.getHeaderField("WWW-Authenticate"));

                state.exception =
                    new InvalidJadException(InvalidJadException.PROXY_AUTH);
                return 0;
            }

            if (responseCode != HttpConnection.HTTP_OK) {
                throw new IOException("Failed to download " + url +
                                      " HTTP response code: " + responseCode);
            }

            mediaType = Util.getHttpMediaType(httpConnection.getType());
            if (mediaType != null) {
                boolean goodType = false;

                for (int i = 0; i < acceptableTypes.length; i++) {
                    if (mediaType.equals(acceptableTypes[i])) {
                        goodType = true;
                        break;
                    }
                }

                if (!goodType) {
                    throw new InvalidJadException(invalidMediaTypeCode,
                                                  mediaType);
                }
            } else if (!allowNoMediaType) {
                throw new InvalidJadException(invalidMediaTypeCode, "");
            }

            if (encoding != null) {
                encoding[0] = getCharset(httpConnection.getType());
            }

            httpInputStream = httpConnection.openInputStream();
            return transferData(httpInputStream, output);
        } finally {
            // Close the streams or connections this method opened.
            try {
                httpInputStream.close();
            } catch (Exception e) {
                if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                    Logging.report(Logging.WARNING, LogChannels.LC_AMS,
                    "stream close  threw an Exception");
                }
            }

            try {
                conn.close();
            } catch (Exception e) {
                if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                    Logging.report(Logging.WARNING, LogChannels.LC_AMS,
                    "connection close  threw an Exception");
                }
            }
        }
    }

    /**
     * If the JAD belongs to an installed suite, check the URL against the
     * installed one. Set the state.exception if the user needs to be warned.
     *
     * @param url JAD or JAR URL of the suite being installed
     */
    private void checkForDifferentDomains(String url) {
        String previousUrl = state.previousInstallInfo.getDownloadUrl();
        // perform a domain check not a straight compare
        if (state.authPath == null && previousUrl != null) {
            HttpUrl old = new HttpUrl(previousUrl);
            HttpUrl current = new HttpUrl(url);

            if ((current.domain != null && old.domain == null) ||
                (current.domain == null && old.domain != null) ||
                (current.domain != null && old.domain != null &&
                 !current.domain.regionMatches(true, 0, old.domain, 0,
                                           old.domain.length()))) {
                /*
                 * The jad is at new location, could be bad,
                 * let the user decide
                 */
                state.exception = new InvalidJadException(
                    InvalidJadException.JAD_MOVED, previousUrl);
                return;
            }
        }
    }

    /**
     * See if there is an installed version of the suite being installed and
     * if so, make an necessary checks. Will set state fields, including
     * the exception field for warning the user.
     *
     * @exception InvalidJadException if the new version is formated
     * incorrectly
     * @exception MIDletSuiteLockedException is thrown, if the MIDletSuite is
     * locked
     */
    private void checkPreviousVersion()
        throws InvalidJadException, MIDletSuiteLockedException {

        String id;
        MIDletSuiteImpl midletSuite;
        String installedVersion;
        int cmpResult;

        // Check if app already exists
        id = MIDletSuiteStorage.getSuiteID(state.vendor, state.suiteName);
        if (id == null) {
            // there is no previous version
            return;
        }

        try {
            midletSuite =
              state.midletSuiteStorage.getMIDletSuite(id, true);

            if (midletSuite == null) {
                // there is no previous version
                return;
            }
            checkVersionFormat(state.version);

            state.isPreviousVersion = true;

            // This is now an update, use the old ID
            state.id = id;

            state.previousSuite = midletSuite;
            state.previousInstallInfo = midletSuite.getInstallInfo();

            if (state.force) {
                // do not ask questions, force an overwrite
                return;
            }

            // If it does, check version information
            installedVersion = midletSuite.getProperty(VERSION_PROP);
            cmpResult = vercmp(state.version, installedVersion);
            if (cmpResult < 0) {
                // older version, warn user
                state.exception = new InvalidJadException(
                                  InvalidJadException.OLD_VERSION,
                                  installedVersion);
                return;
            }

            if (cmpResult == 0) {
                // already installed, warn user
                state.exception = new InvalidJadException(
                                  InvalidJadException.ALREADY_INSTALLED,
                                  installedVersion);
                return;
            }

            // new version, warn user
            state.exception = new InvalidJadException(
                                  InvalidJadException.NEW_VERSION,
                                  installedVersion);
            return;
        } catch (MIDletSuiteCorruptedException mce) {
            if (state.listener != null) {
                state.listener.updateStatus(CORRUPTED_SUITE, state);
            }
        } catch (NumberFormatException nfe) {
            postInstallMsgBackToProvider(
                OtaNotifier.INVALID_JAD_MSG);
            throw new
                InvalidJadException(InvalidJadException.INVALID_VERSION);
        }
    }

    /**
     * If this is an update, make sure the RMS data is handle correctly
     * according to the OTA spec.
     * <p>
     * From the OTA spec:
     * <blockquote>
     * The RMS record stores of a MIDlet suite being updated MUST be
     * managed as follows:</p>
     * <ul>
     * <li>
     *   If the cryptographic signer of the new MIDlet suite and the
     *   original MIDlet suite are identical, then the RMS record
     *   stores MUST be retained and made available to the new MIDlet
     *   suite.</li>
     * <li>
     *   If the scheme, host, and path of the URL that the new
     *   Application Descriptor is downloaded from is identical to the
     *   scheme, host, and path of the URL the original Application
     *   Descriptor was downloaded from, then the RMS MUST be retained
     *   and made available to the new MIDlet suite.</li>
     * <li>
     *   If the scheme, host, and path of the URL that the new MIDlet
     *   suite is downloaded from is identical to the scheme, host, and
     *   path of the URL the original MIDlet suite was downloaded from,
     *   then the RMS MUST be retained and made available to the new
     *   MIDlet suite.</li>
     * <li>
     *   If the above statements are false, then the device MUST ask
     *   the user whether the data from the original MIDlet suite
     *   should be retained and made available to the new MIDlet
     *   suite.</li>
     * </ul>
     * </blockquote>
     *
     * @exception IOException if the install is stopped
     */
    private void processPreviousRMS() throws IOException {
        HttpUrl newUrl;
        HttpUrl originalUrl;

        if (!RecordStoreFactory.suiteHasRmsData(state.id)) {
            return;
        }

        if (state.previousInstallInfo.getAuthPath() != null &&
            state.authPath != null &&
            state.authPath[0].equals(
                state.previousInstallInfo.getAuthPath()[0])) {
            // signers the same
            return;
        }

        try {
            newUrl = new HttpUrl(state.jadUrl);
            originalUrl = new HttpUrl(state.previousInstallInfo.getJadUrl());

            if (newUrl.scheme.equals(originalUrl.scheme) &&
                newUrl.host.equals(originalUrl.host) &&
                newUrl.path.equals(originalUrl.path)) {
                return;
            }
        } catch (NullPointerException npe) {
            // no match, fall through
        }

        try {
            newUrl = new HttpUrl(state.jarUrl);
            originalUrl = new HttpUrl(state.previousInstallInfo.getJarUrl());

            if (newUrl.scheme.equals(originalUrl.scheme) &&
                newUrl.host.equals(originalUrl.host) &&
                newUrl.path.equals(originalUrl.path)) {
                return;
            }
        } catch (NullPointerException npe) {
            // no match, fall through
        }

        // ask the user, if no listener assume no for user's answer

        if (state.listener != null) {
            if (state.listener.keepRMS(state)) {
                // user wants to keep the data
                return;
            }
        }

        // this is a good place to check for a stop installing call
        if (state.stopInstallation) {
            postInstallMsgBackToProvider(
                OtaNotifier.USER_CANCELLED_MSG);
            throw new IOException("stopped");
        }

        RecordStoreFactory.removeRecordStoresForSuite(null, state.id);
    }

    /**
     * Checks to make sure the HTTP server will support Basic authentication.
     *
     * @param wwwAuthField WWW-Authenticate field from the response header
     *
     * @exception InvalidJadException if server does not support Basic
     *                                authentication
     */
    private void checkIfBasicAuthSupported(String wwwAuthField)
            throws InvalidJadException {
        wwwAuthField = wwwAuthField.trim();

        if (!wwwAuthField.regionMatches(true, 0, BASIC_TAG, 0,
                                        BASIC_TAG.length())) {
            throw new InvalidJadException(InvalidJadException.CANNOT_AUTH);
        }
    }

    /**
     * Parses out the charset from the content-type field.
     * The charset parameter is after the ';' in the content-type field.
     *
     * @param contentType value of the content-type field
     *
     * @return charset
     */
    private static String getCharset(String contentType) {
        int start;
        int end;

        if (contentType == null) {
            return null;
        }

        start = contentType.indexOf("charset");
        if (start < 0) {
            return null;
        }

        start = contentType.indexOf('=', start);
        if (start < 0) {
            return null;
        }

        // start past the '='
        start++;

        end = contentType.indexOf(';', start);
        if (end < 0) {
            end = contentType.length();
        }

        return contentType.substring(start, end).trim();
    }

    /**
     * Formats the username and password for HTTP basic authentication
     * according RFC 2617.
     *
     * @param username for HTTP authentication
     * @param password for HTTP authentication
     *
     * @return properly formated basic authentication credential
     */
    private static String formatAuthCredentials(String username,
                                                String password) {
        byte[] data = new byte[username.length() + password.length() + 1];
        int j = 0;

        for (int i = 0; i < username.length(); i++, j++) {
            data[j] = (byte)username.charAt(i);
        }

        data[j] = (byte)':';
        j++;

        for (int i = 0; i < password.length(); i++, j++) {
            data[j] = (byte)password.charAt(i);
        }

        return "Basic " + Base64.encode(data, 0, data.length);
    }

    /**
     * Posts a status message back to the provider's URL in JAD.
     *
     * @param message status message to post
     */
    private void postInstallMsgBackToProvider(String message) {
        OtaNotifier.postInstallMsgBackToProvider(message, state,
            state.proxyUsername, state.proxyPassword);
    }

    /**
     * Function that actually does the work of transferring file data.
     * <p>
     * Updates the listener every 1 K bytes.
     * <p>
     * If the amount of data to be read is larger than <code>maxDLSize</code>
     * we will break the input into chunks no larger than
     * <code>maxDLSize</code>. This prevents the VM from running out of
     * memory when processing large files.
     *
     * @param in the input stream to read from
     * @param out the output stream to write to
     *
     * @return number of bytes written to the output stream
     *
     * @exception IOException if any exceptions occur during transfer
     * of data
     */
    private int transferData(InputStream in, OutputStream out)
            throws IOException {
        byte[] buffer = new byte[MAX_DL_SIZE];
        int bytesRead;
        int totalBytesWritten = 0;

        if (state.listener != null) {
            state.listener.updateStatus(state.beginTransferDataStatus, state);
        }

        try {
            for (int nextUpdate = totalBytesWritten + 1024; ; ) {
                bytesRead = in.read(buffer);

                if (state.listener != null && (bytesRead == -1 ||
                        totalBytesWritten + bytesRead >= nextUpdate)) {

                    synchronized (state) {
                        if (state.stopInstallation) {
                            throw new IOException("stopped");
                        }

                        state.listener.updateStatus(state.transferStatus,
                                                    state);
                    }

                    nextUpdate = totalBytesWritten + 1024;
                }

                if (bytesRead == -1) {
                    return totalBytesWritten;
                }

                out.write(buffer, 0, bytesRead);
                totalBytesWritten += bytesRead;
            }
        } catch (IOException ioe) {
            if (state.stopInstallation) {
                postInstallMsgBackToProvider(
                    OtaNotifier.USER_CANCELLED_MSG);
                throw new IOException("stopped");
            } else {
                throw ioe;
            }
        }
    }

    /**
     * Stops the installation. If installer is not installing then this
     * method has no effect. This will cause the install method to
     * throw an IOException if the install is not writing the suite
     * to storage which is the point of no return.
     *
     * @return true if the install will stop, false if it is too late
     */
    public boolean stopInstalling() {
        if (state == null) {
            return false;
        }

        synchronized (state) {
            if (state.ignoreCancel) {
                return false;
            }

            state.stopInstallation = true;

            try {
                httpInputStream.close();
            } catch (Exception e) {
                if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                    Logging.report(Logging.WARNING, LogChannels.LC_AMS,
                    "stream close  threw an Exception");
                }
            }

            try {
                httpConnection.close();
            } catch (Exception e) {
                if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                    Logging.report(Logging.WARNING, LogChannels.LC_AMS,
                    "stream close  threw an Exception");
                }
            }

            return true;
        }
    }

    /**
     * Tells if the installation was stopped by another thread.
     * @return true if the installation was stopped by another thread
     */
    public boolean wasStopped() {
        if (state == null) {
            return false;
        }

        return state.stopInstallation;
    }

    /**
     * Builds the initial API permission for suite currently being installed.
     *
     * @param domain security domain name for the CA of the suite
     *
     * @return current level of permissions
     *
     * @exception InvalidJadException if a permission attribute is not
     *     formatted properly or a required permission is denied
     */
    protected byte[] getInitialPermissions(String domain)
            throws InvalidJadException {
        byte[][] domainPermissions = Permissions.forDomain(domain);
        byte[] permissions = Permissions.getEmptySet();

        // only the current level of each permission has to be adjusted
        getRequestedPermissions(PERMISSIONS_PROP,
                                domainPermissions[Permissions.CUR_LEVELS],
                                permissions, true);

        getRequestedPermissions(PERMISSIONS_OPT_PROP,
                                domainPermissions[Permissions.CUR_LEVELS],
                                permissions, false);

        return permissions;
    }

    /**
     * Gets the permissions for a domain that are requested the manifest.
     *
     * @param propName name of the property in the manifest
     * @param domainPermissions array of the starting levels for permissions
     *        of a domain
     * @param permissions array to put the permissions from the domain in
     *        when found in the manifest property
     * @param required if set to true the manifest permissions are required
     *
     * @exception InvalidJadException if a permission attribute is not
     *     formatted properly or a required permission is denied
     */
    private void getRequestedPermissions(String propName,
            byte[] domainPermissions, byte[] permissions, boolean required)
            throws InvalidJadException {

        String jadPermissionLine;
        String reqPermissionLine;
        Vector reqPermissions;
        String permission;
        boolean found;
        int i;

        reqPermissionLine = state.getAppProperty(propName);
        if (reqPermissionLine == null || reqPermissionLine.length() == 0) {
            // Zero properties are allowed.
            return;
        }

        reqPermissions = Util.getCommaSeparatedValues(reqPermissionLine);
        if (reqPermissions.size() == 0) {
            postInstallMsgBackToProvider(OtaNotifier.INVALID_JAD_MSG);
            throw new InvalidJadException(InvalidJadException.INVALID_VALUE);
        }

        for (int j = 0; j < reqPermissions.size(); j++) {
            permission = (String)reqPermissions.elementAt(j);

            if (permission.length() == 0) {
                postInstallMsgBackToProvider(
                    OtaNotifier.INVALID_JAD_MSG);
                throw new
                    InvalidJadException(InvalidJadException.INVALID_VALUE);
            }

            found = false;
            for (i = 0; i < Permissions.NUMBER_OF_PERMISSIONS; i++) {
                if (Permissions.getName(i).equals(permission)) {
                    if (domainPermissions[i] != Permissions.NEVER) {
                        found = true;
                    }

                    break;
                }
            }

            if (!found) {
                if (required) {
                    postInstallMsgBackToProvider(
                        OtaNotifier.AUTHORIZATION_FAILURE_MSG);
                    throw new InvalidJadException(
                        InvalidJadException.AUTHORIZATION_FAILURE, permission);
                }

                continue;
            }

            permissions[i] = domainPermissions[i];
        }
    }

    /**
     * Apply the previous user level permission of the currently installed
     * version of a suite to the next version of the suite in a secure way.
     *
     * @param current array permissions for the current version
     * @param domainPermissions array of the starting levels for permissions
     *        of the new domain
     * @param next array permissions for the next version
     */
    private void applyCurrentUserLevelPermissions(byte[] current,
            byte[] domainPermissions, byte[] next) {

        for (int i = 0; i < current.length && i < next.length; i++) {
            switch (current[i]) {
            case Permissions.ALLOW:
            case Permissions.DENY:
                // not a user level permission
                continue;
            }

            switch (domainPermissions[i]) {
            case Permissions.ALLOW:
            case Permissions.DENY:
                // not a user level permission
                continue;

            case Permissions.ONE_SHOT:
                if (current[i] == Permissions.SESSION ||
                    current[i] == Permissions.DENY_SESSION) {
                    // do not apply
                    continue;
                }
                // fall through; per-session permissions may be permitted.

            case Permissions.SESSION:
                if (current[i] == Permissions.BLANKET ||
                    current[i] == Permissions.BLANKET_GRANTED) {
                    // do not apply
                    continue;
                }
                // fall through to store the permission for the next version.

            default:
                next[i] = current[i];
                continue;
            }
        }
    }

    /**
     * Checks to see if the JAD has a signature, but does not verify the
     * signature. This is a place holder the the Secure Installer and
     * just returns false.
     *
     * @return true if the JAD has a signature
     */
    public boolean isJadSigned() {
        return false;
    }

    /**
     * Gets the security domain name for this MIDlet Suite from storage.
     *
     * @param id ID given to the suite by the installer when it was
     *        downloaded
     * @param ca CA of an installed suite
     *
     * @return name of the security domain for the MIDlet Suite
     */
    protected String getSecurityDomainName(String id, String ca) {
        // This is a method that is overridden by a secure installer.
        return Permissions.UNIDENTIFIED_DOMAIN_BINDING;
    }

    /**
     * Sets security domain for unsigned suites. The default is untrusted.
     * Can only be called by JAM for testing.
     *
     * @param domain name of a security domain
     */
    public void setUnsignedSecurityDomain(String domain) {
        MIDletSuite midletSuite = MIDletStateHandler.
            getMidletStateHandler().getMIDletSuite();

        // if a MIDlet suite is not started, assume the JAM is calling.
        if (midletSuite != null) {
            midletSuite.checkIfPermissionAllowed(Permissions.MIDP);
        }

        unsignedSecurityDomain = domain;
    }

    /**
     * Checks to see that if any properties that are both in the JAD and
     * JAR manifest are not equal and throw a exception and notify the
     * server when a mismatch is found. Only used for trusted suites.
     * @exception InvalidJadException  if the properties do not match
     */
    protected void checkForJadManifestMismatches()
            throws InvalidJadException {

        for (int i = 0; i < state.jarProps.size(); i++) {
            String key = state.jarProps.getKeyAt(i);
            String value = state.jarProps.getValueAt(i);
            String dup = state.jadProps.getProperty(key);

            if (dup == null) {
                continue;
            }

            if (!dup.equals(value)) {
                postInstallMsgBackToProvider(
                    OtaNotifier.ATTRIBUTE_MISMATCH_MSG);
                throw new InvalidJadException(
                    InvalidJadException.ATTRIBUTE_MISMATCH, key);
            }
        }
    }

    /**
     * Verifies a Jar. Post any error back to the server.
     *
     * @param jarStorage System store for applications
     * @param jarFilename name of the jar to read
     *
     * @exception IOException if any error prevents the reading
     *   of the JAR
     * @exception InvalidJadException if the JAR is not valid
     */
    protected void verifyJar(RandomAccessStream jarStorage,
                             String jarFilename)
        // This is a place holder for a secure installer.
        throws IOException, InvalidJadException {
    }

    /**
     * Compares two version strings. The return values are very similar to
     * that of strcmp() in 'C'. If the first version is less than the second
     * version, a negative number will be returned. If the first version is
     * greater than the second version, a positive number will be returned.
     * If the two versions are equal, zero is returned.
     * <p>
     * Versions must be in the form <em>xxx.yyy.zzz</em>, where:
     * <pre>
     *     <em>xxx</em> is the major version
     *     <em>yyy</em> is the minor version
     *     <em>zzz</em> is the micro version
     * </pre>
     * It is acceptable to omit the micro and possibly the minor versions.
     * If these are not included in the version string, the period immediately
     * preceding the number must also be removed. So, the versions
     * <em>xxx.yyy</em> or <em>xxx</em> are also valid.
     * <p>
     * Version numbers do not have to be three digits wide. However, you may
     * pad versions with leading zeros if desired.
     * <p>
     * If a version number is omitted, its value is assumed to be zero. All
     * tests will be based on this assumption.
     * <p>
     * For example:
     * <pre>
     *    1.04  >  1.
     *    1.04  <  1.4.1
     *    1.04  =  1.4.0
     * </pre>
     * <p>
     *
     * @param ver1 the first version to compare
     * @param ver2 the second version to compare
     *
     * @return  1 if <code>ver1</code> is greater than <code>ver2</code>
     *          0 if <code>ver1</code> is equal to <code>ver2</code>
     *         -1 if <code>ver1</code> is less than <code>ver2</code>
     *
     * @exception NumberFormatException if either <code>ver1</code> or
     * <code>ver2</code> contain characters that are not numbers or periods
     */
    private int vercmp(String ver1, String ver2)
            throws NumberFormatException {
        String strVal1;
        String strVal2;
        int    intVal1;
        int    intVal2;
        int    idx1 = 0;
        int    idx2 = 0;
        int    newidx;

        if ((ver1 == null) && (ver2 == null)) {
            return 0;
        }

        if (ver1 == null) {
            return -1;
        }

        if (ver2 == null) {
            return 1;
        }

        for (int i = 0; i < 3; i++) {
            strVal1 = "0"; // Default value
            strVal2 = "0"; // Default value
            if (idx1 >= 0) {
                newidx = ver1.indexOf('.', idx1);
                if (newidx < 0) {
                    strVal1 = ver1.substring(idx1);
                } else {
                    strVal1 = ver1.substring(idx1, newidx);
                    newidx++; // Idx of '.'; need to go to next char
                }

                idx1 = newidx;
            }

            if (idx2 >= 0) {
                newidx = ver2.indexOf('.', idx2);
                if (newidx < 0) {
                    strVal2 = ver2.substring(idx2);
                } else {
                    strVal2 = ver2.substring(idx2, newidx);
                    newidx++;
                }

                idx2 = newidx;
            }

            intVal1 = Integer.parseInt(strVal1); // May throw NFE
            intVal2 = Integer.parseInt(strVal2); // May throw NFE

            if (intVal1 > intVal2) {
                return 1;
            }

            if (intVal1 < intVal2) {
                return -1;
            }
        }

        return 0;
    }

    /**
     * Checks the format of a version string.
     * <p>
     * Versions must be in the form <em>xxx.yyy.zzz</em>, where:
     * <pre>
     *     <em>xxx</em> is the major version
     *     <em>yyy</em> is the minor version
     *     <em>zzz</em> is the micro version
     * </pre>
     * It is acceptable to omit the micro and possibly the minor versions.
     * If these are not included in the version string, the period immediately
     * preceding the number must also be removed. So, the versions
     * <em>xxx.yyy</em> or <em>xxx</em> are also valid.
     * <p>
     * Version numbers do not have to be three digits wide. However, you may
     * pad versions with leading zeros if desired.
     *
     * @param ver the version to check
     *
     * @exception NumberFormatException if <code>ver</code>
     *     contains any characters that are not numbers or periods
     */
    private void checkVersionFormat(String ver) throws NumberFormatException {
        int length;
        int start = 0;
        int end;

        length = ver.length();
        for (int i = 0; ; i++) {
            // check for more than 3 parts or a trailing '.'
            if (i == 3 || start == length) {
                throw new NumberFormatException();
            }

            end = ver.indexOf('.', start);
            if (end == -1) {
                end = length;
            }

            // throws NFE if the substring is not all digits
            Integer.parseInt(ver.substring(start, end));

            if (end == length) {
                // we are done
                return;
            }

            // next time around start after the index of '.'
            start = end + 1;
        }
    }

    /**
     * Match the name of the configuration or profile, and return
     * true if the first name has a greater or equal version than the
     * second. The names of the format "XXX-Y.Y" (e.g. CLDC-1.0, MIDP-2.0)
     * as used in the system properties (microedition.configuration &
     * microedition.profiles).
     *
     * This is used for checking both configuration and profiles.
     *
     * @param name1 name of configuration or profile
     * @param name2 name of configuration or profile
     * @return  true is name1 matches name2 and is greater or equal in
     *          version number. false otherwise
     */
    private boolean matchVersion(String name1, String name2) {
        String base1 = name1.substring(0, name1.indexOf('-'));
        String base2 = name2.substring(0, name1.indexOf('-'));

        if (base1.equals(base2)) {
            String ver1 = name1.substring(name1.indexOf('-') + 1,
                                          name1.length());
            String ver2 = name2.substring(name2.indexOf('-') + 1,
                                          name2.length());
            return (vercmp(ver1, ver2) >= 0);
        } else {
            return false;
        }
    }

    /**
     * Checks to make sure the configration need by the application
     * is supported.
     * Send a message back to the server if the check fails and
     * throw an exception.
     *
     * @exception InvalidJadException if the check fails
     */
    private void checkConfiguration() throws InvalidJadException {
        String config;

        config = state.getAppProperty(CONFIGURATION_PROP);
        if (config == null || config.length() == 0) {
            postInstallMsgBackToProvider(
                OtaNotifier.INVALID_JAR_MSG);
            throw new InvalidJadException(
                InvalidJadException.MISSING_CONFIGURATION);
        }

        if (cldcConfig == null) {
            // need to call trim to remove trailing spaces
            cldcConfig =
                System.getProperty("microedition.configuration").trim();
        }

        if (matchVersion(cldcConfig, config)) {
            // success, done
            return;
        }

        postInstallMsgBackToProvider(OtaNotifier.INCOMPATIBLE_MSG);
        throw new InvalidJadException(InvalidJadException.DEVICE_INCOMPATIBLE);
    }

    /**
     * Tries to match one of the supported profiles with a profile
     * listed in string of profiles separated by a space.
     * Send a message back to the server if a match is not found and
     * throw an exception.
     *
     * @exception InvalidJadException if there is no match
     */
    private void matchProfile() throws InvalidJadException {
        String profiles = state.getAppProperty(PROFILE_PROP);

        if (profiles == null || profiles.length() == 0) {
            postInstallMsgBackToProvider(OtaNotifier.INVALID_JAR_MSG);
            throw new
                InvalidJadException(InvalidJadException.MISSING_PROFILE);
        }

        // build the list of supported profiles if needed
        if (supportedProfiles == null) {
            int start;
            int nextSpace = -1;
            // need to call trim to remove trailing spaces
            String meProfiles =
                System.getProperty("microedition.profiles").trim();
            supportedProfiles = new Vector();

            for (; ; ) {
                start = nextSpace + 1;
                nextSpace = meProfiles.indexOf(' ', start);

                // consecutive spaces, keep searching
                if (nextSpace == start) {
                    continue;
                }

                if ((nextSpace < 0)) {
                    supportedProfiles.addElement(
                        meProfiles.substring(start, meProfiles.length()));
                    break;
                }

                supportedProfiles.addElement(
                    meProfiles.substring(start, nextSpace));

            }
        }

        /*
         * for each profiles listed in MicroEdition-Profile, we need to
         * find a matching profile in microedition.profiles.
         */
        int current = 0;
        int nextSeparatorIndex = 0;
        String requestedProfile;
        boolean supported = false;

        // convert tab to space so that the parsing later is simplified
        StringBuffer tmp = new StringBuffer(profiles);
        boolean modified = false;
        while ((nextSeparatorIndex = profiles.indexOf('\t', current)) != -1) {
            tmp.setCharAt(nextSeparatorIndex, ' ');
            current++;
            modified = true;
        }

        if (modified) {
            profiles = tmp.toString();
        }

        // reset the indices
        current = nextSeparatorIndex = 0;
        do {
            // get the next requested profiles
            nextSeparatorIndex = profiles.indexOf(' ', current);

            if (nextSeparatorIndex == current) {
                // consecutive spaces, keep searching
                current++;
                continue;
            }

            if (nextSeparatorIndex == -1) {
                // last (or the only one) value in the list
                requestedProfile =
                   profiles.substring(current, profiles.length());
            } else {
                requestedProfile =
                    profiles.substring(current, nextSeparatorIndex);
                current = nextSeparatorIndex + 1;
            }

            /*
             * try to match each requested profiles against the supported
             * ones.
             */
            supported = false;
            for (int i = 0; i < supportedProfiles.size(); i++) {
                String supportedProfile =
                    (String)supportedProfiles.elementAt(i);
                if (matchVersion(supportedProfile, requestedProfile)) {
                     supported = true;
                     break;
                }
            }

            // short circuit the test if there is one mismatch
            if (!supported) {
                break;
            }
        } while (nextSeparatorIndex != -1);

        // matched all requested profiles against supported ones
        if (supported) {
            return;
        }

        postInstallMsgBackToProvider(OtaNotifier.INCOMPATIBLE_MSG);
        throw new InvalidJadException(InvalidJadException.DEVICE_INCOMPATIBLE);
    }

    /**
     * Registers the push connections for the application.
     * Send a message back to the server if a connection cannot be
     * registered and throw an exception.
     *
     * @exception InvalidJadException if a connection cannot be registered
     */
    private void registerPushConnections()
            throws InvalidJadException {
        byte[] curLevels = state.permissions;

        if (state.isPreviousVersion) {
            PushRegistryImpl.unregisterConnections(state.id);
        }

        for (int i = 1; ; i++) {
            String pushProp;

            pushProp = state.getAppProperty("MIDlet-Push-" + i);
            if (pushProp == null) {
                break;
            }

            /*
             * Parse the comma separated values  -
             *  " connection, midlet, role, filter"
             */
            int comma1 = pushProp.indexOf(',', 0);
            int comma2 = pushProp.indexOf(',', comma1 + 1);

            String conn = pushProp.substring(0, comma1).trim();
            String midlet = pushProp.substring(comma1+1, comma2).trim();
            String filter = pushProp.substring(comma2+1).trim();

            /* Register the new push connection string. */
            try {
                PushRegistryImpl.registerConnectionInternal(null, state,
                    conn, midlet, filter, false);
            } catch (Exception e) {
                /* If already registered, abort the installation. */
                PushRegistryImpl.unregisterConnections(state.id);

                if (state.isPreviousVersion) {
                    // put back the old ones, removed above
                    redoPreviousPushConnections();
                }

                if (e instanceof SecurityException) {
                    postInstallMsgBackToProvider(
                        OtaNotifier.AUTHORIZATION_FAILURE_MSG);

                    // since our state object put the permission in message
                    throw new InvalidJadException(
                        InvalidJadException.AUTHORIZATION_FAILURE,
                        e.getMessage());
                }

                postInstallMsgBackToProvider(
                    OtaNotifier.PUSH_REG_FAILURE_MSG);

                if (e instanceof IllegalArgumentException) {
                    throw new InvalidJadException(
                        InvalidJadException.PUSH_FORMAT_FAILURE, pushProp);
                }

                if (e instanceof ConnectionNotFoundException) {
                    throw new InvalidJadException(
                        InvalidJadException.PUSH_PROTO_FAILURE, pushProp);
                }

                if (e instanceof IOException) {
                    throw new InvalidJadException(
                        InvalidJadException.PUSH_DUP_FAILURE, pushProp);
                }

                if (e instanceof ClassNotFoundException) {
                    throw new InvalidJadException(
                        InvalidJadException.PUSH_CLASS_FAILURE, pushProp);
                }

                // error in the implementation code
                throw (RuntimeException)e;
            }
        }

        if (state.isPreviousVersion) {
            // preserve the push options when updating
            state.pushOptions = state.previousSuite.getPushOptions();

            // use the old setting
            state.pushInterruptSetting =
                (byte)state.previousInstallInfo.getPushInterruptSetting();

            // The old suite may have not had push connections
            if (state.pushInterruptSetting != Permissions.NEVER) {
                return;
            }
        }

        if (curLevels[Permissions.PUSH] == Permissions.NEVER) {
            state.pushInterruptSetting = Permissions.NEVER;
        } else if (curLevels[Permissions.PUSH] == Permissions.ALLOW) {
            // Start the default at session for usability when denying.
            state.pushInterruptSetting = Permissions.SESSION;
        } else {
            state.pushInterruptSetting = curLevels[Permissions.PUSH];
        }
    }

    /**
     * Registers the push connections for previous version after
     * and aborted update.
     */
    private void redoPreviousPushConnections() {
        for (int i = 1; ; i++) {
            String pushProp;

            pushProp = state.previousSuite.getProperty("MIDlet-Push-" + i);
            if (pushProp == null) {
                break;
            }

            /*
             * Parse the comma separated values  -
             *  " connection, midlet, role, filter"
             */
            int comma1 = pushProp.indexOf(',', 0);
            int comma2 = pushProp.indexOf(',', comma1 + 1);

            String conn = pushProp.substring(0, comma1).trim();
            String midlet = pushProp.substring(comma1+1, comma2).trim();
            String filter = pushProp.substring(comma2+1).trim();

            /* Register the new push connection string. */
            try {
                PushRegistryImpl.registerConnectionInternal(null,
                    state, conn, midlet, filter, true);
            } catch (IOException e) {
                if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                    Logging.report(Logging.WARNING, LogChannels.LC_AMS,
                    "registerConnectionInternal  threw an IOException");
                }
            } catch (ClassNotFoundException e) {
                if (Logging.REPORT_LEVEL <= Logging.WARNING) {
                    Logging.report(Logging.WARNING, LogChannels.LC_AMS,
                "registerConnectionInternal threw a ClassNotFoundException");
                }
            }
        }
    }

    /**
     * Holds the state of an installation, so it can restarted after it has
     * been stopped.
     */
    protected class InstallStateImpl implements InstallState, MIDletSuite {

        /** Receives warnings and status. */
        protected InstallListener listener;

        /** When the install started, in milliseconds. */
        protected long startTime;

        /** What to do next. */
        protected int nextStep;

        /** Signals the installation to stop. */
        protected boolean stopInstallation;

        /**
         * Signals that installation is at a point where cancel
         * requests are ignored
         */
        protected boolean ignoreCancel;

        /** exception that stopped the installation. */
        protected InvalidJadException exception;

        /** URL of the JAD. */
        protected String jadUrl;

        /**
         * Option to force an overwrite of existing components without
         * any version comparison.
         */
        protected boolean force;

        /**
         * Option to force the RMS data of the suite to be overwritten to
         * be removed without comparison to the new suite.
         */
        protected boolean removeRMS;

        /** Raw JAD. */
        protected byte[] jad;

        /** character encoding of the JAD. */
        protected String jadEncoding;

        /** Parsed JAD. */
        protected JadProperties jadProps;

        /** Parsed manifest. */
        protected ManifestProperties jarProps;

        /** Cached File object. */
        protected File file;

        /** User name for authentication. */
        protected String username;

        /** Password for authentication. */
        protected String password;

        /** User name for proxyAuthentication. */
        protected String proxyUsername;

        /** Password for proxy authentication. */
        protected String proxyPassword;

        /** Name of the suite. */
        protected String suiteName;

        /** Vendor of the suite. */
        protected String vendor;

        /** Version of the suite. */
        protected String version;

        /** Description of the suite. */
        protected String description;

        /** What ID the installed suite is stored by. */
        protected String id;

        /**
         * Authorization path, staring with the most trusted CA authorizing
         * the suite, for secure installing.
         */
        protected String[] authPath;

        /** How big the JAD says the JAR is. */
        protected int expectedJarSize;

        /** URL of the JAR. */
        protected String jarUrl;

        /** Status to signal the beginning of the data transfer. */
        protected int beginTransferDataStatus;

        /** Status for the data transfer method to give to the listener. */
        protected int transferStatus;

        /** Push interrupt setting. */
        private byte pushInterruptSetting;

        /** Push Options. */
        private int pushOptions;

        /** Permissions. */
        private byte[] permissions;

        /** Security domain. */
        private String domain;

        /** Security Handler. */
        private SecurityHandler securityHandler;

        /** Flag for trusted suites. */
        private boolean trusted;

        /** Holds the unzipped JAR manifest to be saved. */
        protected byte[] manifest;

        /** The JAR file name before it is verified. */
        protected String tempFilename;

        /** Cache of storage object. */
        protected RandomAccessStream storage;

        /** Cache of MIDlet suite storage object. */
        protected MIDletSuiteStorage midletSuiteStorage;

        /** The root of all MIDP persistent system data. */
        protected String storageRoot;

        /** Signals that previous version exists. */
        protected boolean isPreviousVersion;

        /** Previous MIDlet suite info. */
        protected MIDletSuiteImpl previousSuite;

        /** Previous MIDlet suite install info. */
        protected InstallInfo previousInstallInfo;

        /** The ContentHandler installer state. */
        protected CHManager chmanager;

        /** Hash value of the suite with preverified classes. */
        protected byte[] verifyHash;

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
        public String getID() {
            return id;
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
         * First from the JAD, then if not found, the JAR manifest.
         *
         * @param key key of the property
         *
         * @return value of the property or null if not found
         */
        public String getAppProperty(String key) {
            String value;

            if (state.jadProps != null) {
                value = state.jadProps.getProperty(key);
                if (value != null) {
                    return value;
                }
            }

            if (state.jarProps != null) {
                value = state.jarProps.getProperty(key);
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
            return jarUrl;
        }

        /**
         * Gets the label for the downloaded JAR.
         *
         * @return suite name
         */
        public String getSuiteName() {
            return suiteName;
        }

        /**
         * Gets the expected size of the JAR.
         *
         * @return size of the JAR in K bytes rounded up
         */
        public int getJarSize() {
            return (expectedJarSize + 1023) / 1024;
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
            return authPath;
        }

        /**
         * Checks for permission and throw an exception if not allowed.
         * May block to ask the user a question.
         *
         * @param permission ID of the permission to check for,
         *      the ID must be from
         *      {@link com.sun.midp.security.Permissions}
         * @param resource string to insert into the question, can be null if
         *        no %2 in the question
         *
         * @exception SecurityException if the permission is not
         *            allowed by this token
         * @exception InterruptedException if another thread interrupts the
         *   calling thread while this method is waiting to preempt the
         *   display.
         */
        public void checkForPermission(int permission, String resource)
                throws InterruptedException {
            checkForPermission(permission, resource, null);
        }

        /**
         * Checks for permission and throw an exception if not allowed.
         * May block to ask the user a question.
         *
         * @param permission ID of the permission to check for,
         *      the ID must be from
         *      {@link com.sun.midp.security.Permissions}
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
        public void checkForPermission(int permission, String resource,
                String extraValue) throws InterruptedException {

            securityHandler.checkForPermission(permission,
                Permissions.getTitle(permission),
                Permissions.getQuestion(permission),
                Permissions.getOneshotQuestion(permission),
                state.suiteName, resource, extraValue,
                Permissions.getName(permission));
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
         * Indicates if this suite is trusted.
         * (not to be confused with a domain named "trusted",
         * this is used to determine if a trusted symbol should be displayed
         * to the user and not used for permissions)
         *
         * @return true if the suite is trusted false if not
         */
        public boolean isTrusted() {
            return trusted;
        }


        /**
         * Check if the suite classes were successfully verified
         * during the suite installation.
         *
         * @return true if the suite classes are verified, false otherwise
         */
        public boolean isVerified() {
            return verifyHash != null;
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
        public int getPushInterruptSetting() {
            return pushInterruptSetting;
        }

        /**
         * Gets push options for this suite.
         *
         * @return push options are defined in {@link PushRegistryImpl}
         */
        public int getPushOptions() {
            return pushOptions;
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
         * Checks to see the suite has the ALLOW level for specific permission.
         * This is used for by internal APIs that only provide access to
         * trusted system applications.
         * <p>
         * Only trust this method if the object has been obtained from the
         * MIDletStateHandler of the suite.
         *
         * @param permission permission ID from
         *      {@link com.sun.midp.security.Permissions}
         *
         * @exception SecurityException if the suite is not
         *            allowed to perform the specified action
         */
        public void checkIfPermissionAllowed(int permission) {
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
         * Close the opened MIDletSuite
         */
        public void close() {
        }
    }
}
